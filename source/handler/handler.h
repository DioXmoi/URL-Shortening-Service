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
        const Random::StringGenerator& generator)
        : m_database{ std::move(database) }
        , m_generator{ generator }
    {
        std::string dir = std::format("logs/{}.txt", loggerName);
        m_logger = spdlog::basic_logger_mt<spdlog::async_factory>(loggerName.c_str(), dir.c_str());
    }

    http::message_generator operator()(http::request<Body, Allocator>&& req) {
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

private:

    http::message_generator CreateStandardResponse(
        http::request<Body, Allocator>&& req, http::status status, json&& body) {

        http::response<http::string_body> res{ status, req.version() };
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = body.dump(4);
        res.prepare_payload();

        return res;
    }

    http::message_generator CreateStandardResponse(
        http::request<Body, Allocator>&& req, http::status status) {

        http::response<http::empty_body> res{ status, req.version() };
        res.keep_alive(req.keep_alive());
        res.prepare_payload();

        return res;
    }


    // 400 Returns a bad request response
    http::message_generator GenerateBadRequest(
        http::request<Body, Allocator>&& req, std::string_view why) {
        json json;
        json["Error"] = why;
        return CreateStandardResponse(std::move(req),
            http::status::bad_request,
            std::move(json));
    }

    // 404 NOT FOUND
    http::message_generator GenerateNotFound(
        http::request<Body, Allocator>&& req, std::string_view why) {
        json json;
        json["Error"] = why;
        return CreateStandardResponse(std::move(req),
            http::status::not_found,
            std::move(json));
    }

    // 405 Method Not Allowed
    http::message_generator GenerateMethodNotAllowed(
        http::request<Body, Allocator>&& req) {
        json json;
        json["Error"] = "Method not allowed for this endpoint.";
        return CreateStandardResponse(std::move(req),
            http::status::method_not_allowed,
            std::move(json));
    }


    std::string QuerySelectByShortCode(std::string_view shortCode) {
        return m_database -> Query<std::string>(
            "UPDATE urls SET accesscount = accesscount + 1 WHERE shortcode = $1 RETURNING to_json(urls.id, urls.url, urls.shortcode, urls.createdat, urls.updatedat)",
            IDatabase::SqlParams{ { "$1", shortCode.data() } },
            [](const std::vector<std::string>& data) -> std::string {
                if (data.empty()) {
                    return { }; // Return an empty string if nothing is found.
                }

                return std::move(data.front()); // Return JSON if found.
            } );
    }

    // Handle POST /shorten (create a new url shorten)
    http::message_generator CreateShortenUrl(
        http::request<Body, Allocator>&& req) {
        if (req.body().empty()) {
            return GenerateBadRequest(std::move(req), "Empty request body.");
        }

        try {
            json j{ json::parse(req.body()) };
            std::string url{ j.at("url").get<std::string>() };

            std::string body = m_database -> Query<std::string>("SELECT to_json(u) FROM urls AS u WHERE u.url = $1",
                IDatabase::SqlParams{ std::make_pair(std::string{ "$1" }, url) },
                [](const std::vector<std::string>& data) -> std::string {
                    if (data.empty()) {
                        return { };
                    }

                    return data.front(); // returns a single string containing json
                });

            bool notCreated = body.empty();
            http::status status = http::status::created; // Default status is created

            if (notCreated) {
                bool isFound{ true };
                std::string shortCode{ };
                while (isFound) {
                    shortCode = m_generator.Generate();

                    isFound = QuerySelectByShortCode(shortCode).empty();
                }

                // If the shortcode is missing, we can bind it to the url.
                body = m_database -> Query<std::string>("INSERT INTO urls (url, shortcode) VALUES ($1, $2) RETURNING to_json(urls.*) AS json_data",
                    IDatabase::SqlParams{ std::make_pair(std::string{ "$1" }, url), std::make_pair(std::string{ "$2" }, shortCode) },
                    [](const std::vector<std::string>& data) -> std::string {
                        return data.front(); // returns a single string containing json
                    });
            }
            else {
                status = http::status::ok; // Set status to OK if the URL already exists
            }

            return CreateStandardResponse(std::move(req),
                status,
                json::parse(std::move(body)));
        }
        catch (const PostgreSQLError::PostgreSQLError& e) {
            m_logger -> error("Exception: To process Database: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process Database.");
        }
        catch (const json::parse_error& e) {
            m_logger -> error("Exception: JSON parsing error: {}", e.what());
            return GenerateBadRequest(std::move(req), "Make sure that the request body has the correct JSON format.");
        }
        catch (const json::type_error& e) {
            m_logger -> error("Exception: Data type error: {}", e.what());
            return GenerateBadRequest(std::move(req), "Check that the value of the 'url' key is a string.");
        }
        catch (const std::exception& e) {
            m_logger -> error("Exception: during POST /shorten request processing: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process request.");
        }
    }

    http::message_generator HandlerMethodPost(http::request<Body, Allocator>&& req) {
        std::string target{ req.target() };
        if (target == "/shorten") {
            return CreateShortenUrl(std::move(req));
        }
        else {
            return GenerateNotFound(std::move(req), "Endpoint was not found.");
        }
    }


    // Handle GET starts with /shorten/...
    http::message_generator FindUrlByShortCode(
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
        catch (const PostgreSQLError::PostgreSQLError& e) {
            m_logger -> error("Exception: To process Database: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process Database.");
        }
        catch (const std::exception& e) {
            m_logger -> error("Exception: during GET /shorten/ request processing: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process request.");
        }
    }


    std::string QueryFullStatsByShortCode(std::string_view shortCode) {
        return m_database->Query<std::string>(
            "UPDATE urls SET accesscount = accesscount + 1 WHERE shortcode = $1 RETURNING to_json(urls.*)",
            IDatabase::SqlParams{ { "$1", shortCode.data() } },
            [](const std::vector<std::string>& data) -> std::string {
                if (data.empty()) {
                    return { }; // Return an empty string if nothing is found.
                }

                return std::move(data.front()); // Return JSON if found.
            });
    }

    // Handle GET starts with /shorten/.../stats
    http::message_generator GetFullStatsByShortCode(
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
        catch (const PostgreSQLError::PostgreSQLError& e) {
            m_logger -> error("Exception: To process Database: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process Database.");
        }
        catch (const std::exception& e) {
            m_logger -> error("Exception: during GET /shorten/ request processing: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process request.");
        }
    }

    http::message_generator HandlerMethodGet(http::request<Body, Allocator>&& req) {
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

    std::string QueryUpdateUrlByShortCode(std::string_view url, std::string_view shortCode) {
        return m_database -> Query<std::string>(
            "UPDATE urls SET accesscount = accesscount + 1, url = $1 WHERE shortcode = $2 RETURNING to_json(urls.id, urls.url, urls.shortcode, urls.createdat, urls.updatedat)",
            IDatabase::SqlParams{ { "$1", url.data() }, { "$2", shortCode.data() } },
            [](const std::vector<std::string>& data) -> std::string {
                if (data.empty()) {
                    return { }; // Return an empty string if nothing is found.
                }

                return std::move(data.front()); // Return JSON if found.
            });
    }

    // Handle PUT starts with /shorten/...
    http::message_generator UpdateByShortCode(http::request<Body, Allocator>&& req, std::string_view shortCode) {
        if (req.body().empty()) {
            return GenerateBadRequest(std::move(req), "Empty request body.");
        }
       
        try {
            json j{ json::parse(req.body()) };
            std::string url{ j.at("url").get<std::string>() };

            std::string body = QueryUpdateUrlByShortCode(shortCode, url);

            if (body.empty()) {
                return GenerateNotFound(std::move(req), "The short URL was not found.");
            }

            return CreateStandardResponse(std::move(req), 
                http::status::ok, 
                json::parse(std::move(body)));
        }
        catch (const PostgreSQLError::PostgreSQLError& e) {
            m_logger -> error("Exception: To process Database: {}", e.what());
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
            m_logger -> error("Exception: during PUT /shorten/ request processing: {}", e.what());
            return GenerateBadRequest(std::move(req), "Failed to process request.");
        }
    }

    http::message_generator HandlerMethodPut(http::request<Body, Allocator>&& req) {
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

    std::string QueryDeleteByShortCode(std::string_view shortCode) {
        return m_database -> Query<std::string>(
            "DELETE FROM urls AS u WHERE u.shortcode = $1 RETURNING to_json(u.id)",
            IDatabase::SqlParams{ { "$1", shortCode.data() } },
            [](const std::vector<std::string>& data) -> std::string {
                if (data.empty()) {
                    return { }; // Return an empty string if nothing is found.
                }

                return std::move(data.front()); // Return JSON if found.
            });
    }

    // Handle DELETE starts with /shorten/...
    http::message_generator DeleteByShortCode(http::request<Body, Allocator>&& req, std::string_view shortCode) {
        try {
            std::string body = QueryDeleteByShortCode(shortCode);

            if (body.empty()) {
                return GenerateNotFound(std::move(req), "The short URL was not found.");
            }

            return CreateStandardResponse(std::move(req), http::status::no_content);
        }
        catch (const PostgreSQLError::PostgreSQLError& e) {
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