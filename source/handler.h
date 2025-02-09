#pragma once 


#include "format"
#include "postgresql.h"
#include "url.h"
#include "random.h"
#include <nlohmann/json.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/async.h" 
#include "spdlog/sinks/basic_file_sink.h"


using json = nlohmann::json;

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

    // 400 Returns a bad request response
    http::response<http::string_body> GenerateBadRequest(
        http::request<Body, Allocator>&& req, std::string_view why) {

        http::response<http::string_body> res{ http::status::bad_request, req.version() };

        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = nlohmann::json{ why }.dump(4);
        res.prepare_payload();

        return res;
    }

    // 404 NOT FOUND
    http::response<http::string_body> GenerateNotFound(
        http::request<Body, Allocator>&& req, std::string_view why) {

        http::response<http::string_body> res{ http::status::not_found, req.version() };

        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.prepare_payload();

        res.body() = nlohmann::json{ why }.dump(4);

        return res;
    }

    // 405 Method Not Allowed
    http::response<http::string_body> GenerateMethodNotAllowed(
        http::request<Body, Allocator>&& req) {

        http::response<http::string_body> res{ http::status::method_not_allowed, req.version() };

        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.prepare_payload();

        std::string message = "Method not allowed for this endpoint.";
        res.body() = nlohmann::json{ message }.dump(4);

        return res;
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
    http::response<http::string_body> CreateShortenUrl(
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

            http::response<http::string_body> res{ status, req.version() };

            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = json::parse(std::move(body)).dump(4);
            res.prepare_payload();

            return res;
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


    // Handle GET starts with /shorten/
    http::response<http::string_body> FindUrlByShortCode(
        http::request<Body, Allocator>&& req, std::string_view shortCode) {
        try {
            std::string body = QuerySelectByShortCode(shortCode);

            if (body.empty()) {
                return GenerateNotFound(std::move(req), "The short URL was not found.");
            }

            http::response<http::string_body> res{ http::status::ok, req.version() };
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = json::parse(std::move(body)).dump(4);
            res.prepare_payload();

            return res;
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

        const std::string pattern{ "/shorten/" };
        if (target.starts_with(pattern)) {
            std::string shortCode = target.substr(pattern.size());
            return FindUrlByShortCode(std::move(req), shortCode);
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

    // Handle PUT starts with /shorten/
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

            http::response<http::string_body> res{ http::status::ok, req.version() };
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = json::parse(std::move(body)).dump(4);
            res.prepare_payload();

            return res;
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


    // Handle DELETE starts with /shorten/
    http::message_generator DeleteByShortCode(http::request<Body, Allocator>&& req, std::string_view shortCode) {
        try {
            std::string body = QueryDeleteByShortCode(shortCode);

            if (body.empty()) {
                return GenerateNotFound(std::move(req), "The short URL was not found.");
            }

            http::response<http::empty_body> res{ http::status::no_content, req.version() };
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.prepare_payload();

            return res;
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