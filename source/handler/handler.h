#pragma once 


#include "format"
#include "postgresql.h"
#include "url.h"
#include "random.h"
#include <nlohmann/json.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/async.h" 
#include "spdlog/sinks/basic_file_sink.h"
#include <boost/beast/http.hpp>


using json = nlohmann::json;
namespace beast = boost::beast;      
namespace http = beast::http; 
using LoggerPtr = std::shared_ptr<spdlog::logger>;

template <class Body, class Allocator = http::basic_fields<std::allocator<char>>>
class HttpHandler {
public:

    HttpHandler(std::unique_ptr<IDatabase> database,
        std::string loggerName,
        const Random::StringGenerator& generator);

    http::message_generator operator()(http::request<Body, Allocator>&& req);

private:

    http::message_generator CreateStandardResponse(
        http::request<Body, Allocator>&& req, http::status status, json&& body);

    http::message_generator CreateStandardResponse(
        http::request<Body, Allocator>&& req, http::status status);

    // 400 Returns a bad request response
    http::message_generator GenerateBadRequest(
        http::request<Body, Allocator>&& req, std::string_view why);

    // 404 NOT FOUND
    http::message_generator GenerateNotFound(
        http::request<Body, Allocator>&& req, std::string_view why);

    // 405 Method Not Allowed
    http::message_generator GenerateMethodNotAllowed(
        http::request<Body, Allocator>&& req);
  
    std::string QuerySelectByShortCode(std::string_view shortCode);

    // Handle POST /shorten (create a new url shorten)
    http::message_generator CreateShortenUrl(
        http::request<Body, Allocator>&& req);

    http::message_generator HandlerMethodPost(http::request<Body, Allocator>&& req);

    // Handle GET starts with /shorten/..
    http::message_generator FindUrlByShortCode(
        http::request<Body, Allocator>&& req, std::string_view shortCode);

    std::string QueryFullStatsByShortCode(std::string_view shortCode);

    // Handle GET starts with /shorten/../stats
    http::message_generator GetFullStatsByShortCode(
        http::request<Body, Allocator>&& req, std::string_view shortCode);

    http::message_generator HandlerMethodGet(http::request<Body, Allocator>&& req);

    std::string QueryUpdateUrlByShortCode(std::string_view url, std::string_view shortCode);

    // Handle PUT starts with /shorten/..
    http::message_generator UpdateByShortCode(
        http::request<Body, Allocator>&& req, std::string_view shortCode);

    http::message_generator HandlerMethodPut(http::request<Body, Allocator>&& req);

    bool QueryDeleteByShortCode(std::string_view shortCode) {
        return m_database -> Query<bool>(
            "DELETE FROM urls AS u WHERE u.shortcode = $1 RETURNING to_json(u.*);",
            IDatabase::SqlParams{ { "$1", shortCode.data() } },
            [](std::vector<std::string>&& data) -> bool {
                if (data.empty()) {
                    return false;
                }

                return true;
            });
    }

    // Handle DELETE starts with /shorten/...
    http::message_generator DeleteByShortCode(
        http::request<Body, Allocator>&& req, std::string_view shortCode) {
        try {
            bool isDeleted = QueryDeleteByShortCode(shortCode);

            if (!isDeleted) {
                return GenerateNotFound(std::move(req), "The short URL was not found.");
            }

            return CreateStandardResponse(
                std::move(req), 
                http::status::no_content);
        }
        catch (const PostgreSQL::PostgreSQLError& e) {
            m_logger -> error("Exception: To process Database: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process Database.");
        }
        catch (const std::exception& e) {
            m_logger -> error("Exception: during Delete /shorten/ request processing: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process request.");
        }
    }

    http::message_generator HandlerMethodDelete(http::request<Body, Allocator>&& req) {
        std::string target{ req.target() };

        const std::string pattern{ "/shorten/" };
        if (target.starts_with(pattern)) {
            std::string shortCode = target.substr(pattern.size());
            return DeleteByShortCode(std::move(req), shortCode);
        }
        else {
            return GenerateNotFound(std::move(req), "Endpoint was not found.");
        }
    }


private:
    std::unique_ptr<IDatabase> m_database;
    LoggerPtr m_logger;
    Random::StringGenerator m_generator;
};


// public interface 
template <class Body, class Allocator>
HttpHandler<Body, Allocator>::HttpHandler(std::unique_ptr<IDatabase> database,
    std::string loggerName,
    const Random::StringGenerator& generator)
    : m_database{ std::move(database) }
    , m_generator{ generator }
{
    std::string dir = std::format("logs/{}.txt", loggerName);
    m_logger = spdlog::get(dir);
    if (!m_logger) {
        m_logger = spdlog::basic_logger_mt<spdlog::async_factory>(loggerName.c_str(), dir.c_str());
    }
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::operator()(http::request<Body, Allocator>&& req) {
    http::verb method{ req.method() };
    if (method == http::verb::post) {
        return HandlerMethodPost(std::move(req));
    }
    else if (method == http::verb::get) {
        return HandlerMethodGet(std::move(req));
    }
    else if (method == http::verb::put) {
        return HandlerMethodPut(std::move(req));
    }
    else if (method == http::verb::delete_) {
        return HandlerMethodDelete(std::move(req));
    }
    else {
        return GenerateMethodNotAllowed(std::move(req));
    }
}

// private logic
template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::CreateStandardResponse(
    http::request<Body, Allocator>&& req, http::status status, json&& body) {

    http::response<http::string_body> res{ status, req.version() };
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump(4);
    res.prepare_payload();

    return res;
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::CreateStandardResponse(
    http::request<Body, Allocator>&& req, http::status status) {

    http::response<http::empty_body> res{ status, req.version() };
    res.keep_alive(req.keep_alive());
    res.prepare_payload();

    return res;
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::GenerateBadRequest(
    http::request<Body, Allocator>&& req, std::string_view why) {
    json json;
    json["error"] = why;
    return CreateStandardResponse(std::move(req),
        http::status::bad_request,
        std::move(json));
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::GenerateNotFound(
    http::request<Body, Allocator>&& req, std::string_view why) {
    json json;
    json["error"] = why;
    return CreateStandardResponse(std::move(req),
        http::status::not_found,
        std::move(json));
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::GenerateMethodNotAllowed(
    http::request<Body, Allocator>&& req) {
    json json;
    json["error"] = "Method not allowed for this endpoint.";
    return CreateStandardResponse(std::move(req),
        http::status::method_not_allowed,
        std::move(json));
}

template <class Body, class Allocator>
std::string HttpHandler<Body, Allocator>::QuerySelectByShortCode(std::string_view shortCode) {
    return m_database->Query<std::string>(
        "UPDATE urls SET accesscount = accesscount + 1 WHERE shortcode = $1 "
        "RETURNING to_json(urls.*)::jsonb - 'accesscount';",
        IDatabase::SqlParams{ { "$1", shortCode.data() } },
        [](std::vector<std::string>&& data) -> std::string {
            if (data.empty()) {
                return { }; // Return an empty string if nothing is found.
            }

            return std::move(data.front()); // Return JSON if found.
        });
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::CreateShortenUrl(
    http::request<Body, Allocator>&& req) {
    if (req.body().empty()) {
        return GenerateBadRequest(std::move(req), "Empty request body.");
    }

    try {
        json j{ json::parse(req.body()) };
        std::string url{ j.at("url").get<std::string>() };

        std::string body = m_database->Query<std::string>(
            "UPDATE urls SET accesscount = accesscount + 1 WHERE url = $1 "
            "RETURNING to_json(urls.*)::jsonb - 'accesscount';",
            IDatabase::SqlParams{ std::make_pair(std::string{ "$1" }, url) },
            [](std::vector<std::string>&& data) -> std::string {
                if (data.empty()) {
                    return { };
                }

                return std::move(data.front()); // returns a single string containing json
            });

        bool notCreated = body.empty();
        http::status status = http::status::created; // Default status is created

        if (notCreated) {
            bool isFound{ false };
            std::string shortCode{ };
            while (!isFound) {
                shortCode = m_generator.Generate();

                isFound = QuerySelectByShortCode(shortCode).empty();
            }

            // If the shortcode is missing, we can bind it to the url.
            body = m_database->Query<std::string>(
                "INSERT INTO urls (url, shortcode) VALUES ($1, $2) "
                "RETURNING to_json(urls.*)::jsonb - 'accesscount';",
                IDatabase::SqlParams{
                    std::make_pair(std::string{ "$1" }, std::move(url)),
                    std::make_pair(std::string{ "$2" }, std::move(shortCode)) },
                    [](std::vector<std::string>&& data) -> std::string {
                    return std::move(data.front()); // returns a single string containing json
                });
        }
        else {
            status = http::status::ok; // Set status to OK if the URL already exists
        }

        return CreateStandardResponse(std::move(req),
            status,
            json::parse(std::move(body)));
    }
    catch (const PostgreSQL::PostgreSQLError& e) {
        m_logger->error("Exception: To process Database: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process Database.");
    }
    catch (const json::parse_error& e) {
        m_logger->error("Exception: JSON parsing error: {}", e.what());
        return GenerateBadRequest(std::move(req), "Make sure that the request body has the correct JSON format.");
    }
    catch (const json::type_error& e) {
        m_logger->error("Exception: Data type error: {}", e.what());
        return GenerateBadRequest(std::move(req), "Check that the value of the 'url' key is a string.");
    }
    catch (const std::exception& e) {
        m_logger->error("Exception: during POST /shorten request processing: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process request.");
    }
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::HandlerMethodPost(http::request<Body, Allocator>&& req) {
    std::string target{ req.target() };
    if (target == "/shorten") {
        return CreateShortenUrl(std::move(req));
    }
    else {
        return GenerateNotFound(std::move(req), "Endpoint was not found.");
    }
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::FindUrlByShortCode(
    http::request<Body, Allocator>&& req, std::string_view shortCode) {
    try {
        std::string body = QuerySelectByShortCode(shortCode);

        if (body.empty()) {
            return GenerateNotFound(std::move(req), "The short URL was not found.");
        }

        return CreateStandardResponse(std::move(req),
            http::status::ok,
            json::parse(std::move(body)));
    }
    catch (const PostgreSQL::PostgreSQLError& e) {
        m_logger->error("Exception: To process Database: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process Database.");
    }
    catch (const std::exception& e) {
        m_logger->error("Exception: during GET /shorten/ request processing: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process request.");
    }
}

template <class Body, class Allocator>
std::string HttpHandler<Body, Allocator>::QueryFullStatsByShortCode(std::string_view shortCode) {
    return m_database->Query<std::string>(
        "UPDATE urls SET accesscount = accesscount + 1 WHERE shortcode = $1 RETURNING to_json(urls.*);",
        IDatabase::SqlParams{ { "$1", shortCode.data() } },
        [](std::vector<std::string>&& data) -> std::string {
            if (data.empty()) {
                return { }; // Return an empty string if nothing is found.
            }

            return std::move(data.front()); // Return JSON if found.
        });
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::GetFullStatsByShortCode(
    http::request<Body, Allocator>&& req, std::string_view shortCode) {
    try {
        std::string body = QueryFullStatsByShortCode(shortCode);

        if (body.empty()) {
            return GenerateNotFound(std::move(req), "The short URL was not found.");
        }

        return CreateStandardResponse(std::move(req),
            http::status::ok,
            json::parse(std::move(body)));
    }
    catch (const PostgreSQL::PostgreSQLError& e) {
        m_logger->error("Exception: To process Database: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process Database.");
    }
    catch (const std::exception& e) {
        m_logger->error("Exception: during GET /shorten/ request processing: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process request.");
    }
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::HandlerMethodGet(http::request<Body, Allocator>&& req) {
    std::string target{ req.target() };

    const std::string patternStart{ "/shorten/" };
    const std::string patternEnd{ "/stats" };
    if (target.starts_with(patternStart) && !target.ends_with(patternEnd)) {
        std::string shortCode = target.substr(patternStart.size());
        return FindUrlByShortCode(std::move(req), shortCode);
    }
    else if (target.starts_with(patternStart) && target.ends_with(patternEnd)) {
        std::string shortCode = target.substr(patternStart.size(),
            target.size() - (patternStart.size() + patternEnd.size()));

        return GetFullStatsByShortCode(std::move(req), shortCode);
    }
    else {
        return GenerateNotFound(std::move(req), "Endpoint was not found.");
    }
}

template <class Body, class Allocator>
std::string HttpHandler<Body, Allocator>::QueryUpdateUrlByShortCode(std::string_view url, std::string_view shortCode) {
    return m_database->Query<std::string>(
        "UPDATE urls SET accesscount = accesscount + 1, url = $1 WHERE shortcode = $2 "
        "RETURNING to_json(urls.*)::jsonb - 'accesscount';",
        IDatabase::SqlParams{ { "$1", url.data() }, { "$2", shortCode.data() } },
        [](std::vector<std::string>&& data) -> std::string {
            if (data.empty()) {
                return { }; // Return an empty string if nothing is found.
            }

            return std::move(data.front()); // Return JSON if found.
        });
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::UpdateByShortCode(
    http::request<Body, Allocator>&& req, std::string_view shortCode) {
    if (req.body().empty()) {
        return GenerateBadRequest(std::move(req), "Empty request body.");
    }

    try {
        json j{ json::parse(req.body()) };
        std::string url{ j.at("url").get<std::string>() };

        std::string body = QueryUpdateUrlByShortCode(url, shortCode);

        if (body.empty()) {
            return GenerateNotFound(std::move(req), "The short code was not found.");
        }

        return CreateStandardResponse(std::move(req),
            http::status::ok,
            json::parse(std::move(body)));
    }
    catch (const PostgreSQL::PostgreSQLError& e) {
        m_logger->error("Exception: To process Database: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process Database.");
    }
    catch (const json::parse_error& e) {
        m_logger->error("Exception: JSON parsing error: {}", e.what());
        return GenerateBadRequest(std::move(req), "Make sure that the request body has the correct JSON format.");
    }
    catch (const json::type_error& e) {
        m_logger->error("Exception: Data type error: {}", e.what());
        return GenerateBadRequest(std::move(req), "Check that the value of the 'url' key is a string.");
    }
    catch (const std::exception& e) {
        m_logger->error("Exception: during PUT /shorten/ request processing: {}", e.what());
        return GenerateBadRequest(std::move(req), "Failed to process request.");
    }
}

template <class Body, class Allocator>
http::message_generator HttpHandler<Body, Allocator>::HandlerMethodPut(http::request<Body, Allocator>&& req) {
    std::string target{ req.target() };

    const std::string pattern{ "/shorten/" };
    if (target.starts_with(pattern)) {
        std::string shortCode = target.substr(pattern.size());
        return UpdateByShortCode(std::move(req), shortCode);
    }
    else {
        return GenerateNotFound(std::move(req), "Endpoint was not found.");
    }
}