#include <string>
#include <memory>

#include <gtest/gtest.h>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

#include "TestHandler.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;


void ParseJSONWithErrorHandling(const std::string& str_json, json& result) {
	try {
		result = json::parse(str_json);
	}
	catch (const nlohmann::json::parse_error& e) {
		FAIL() << "JSON Parse Error:\n"
			   << "  Message: " << e.what() << "\n"
			   << "  Exception id: " << e.id << "\n"
			   << "  Byte position: " << e.byte << "\n"
			   << "  String JSON: " << str_json << "\n";
	}
}

std::string CheckStandardBody(const json& body) {
	std::string errorMessage{ };

	if (!body.contains("id")) {
		errorMessage += "Missing 'id' field. ";
	}
	else if (!body.at("id").is_number_integer()) { 
		errorMessage += "'id' field must be a string or an integer. ";
	}

	if (!body.contains("url")) {
		errorMessage += "Missing 'url' field. ";
	}
	else if (!body.at("url").is_string()) {
		errorMessage += "'url' field must be a string. ";
	}

	if (!body.contains("shortcode")) {
		errorMessage += "Missing 'shortcode' field. ";
	}
	else if (!body.at("shortcode").is_string()) {
		errorMessage += "'shortcode' field must be a string. ";
	}

	if (!body.contains("createdat")) {
		errorMessage += "Missing 'createdat' field. ";
	}
	else if (!body.at("createdat").is_string()) {
		errorMessage += "'createdat' field must be a string. ";
	}

	if (!body.contains("updatedat")) {
		errorMessage += "Missing 'updatedat' field. ";
	}
	else if (!body.at("updatedat").is_string()) {
		errorMessage += "'updatedat' field must be a string. ";
	}

	return errorMessage; // "" if everything is fine, otherwise - an error message
}

std::string CheckFullStatsBody(const json& body) {
	std::string errorMessage{ CheckStandardBody(body) };
	if (!body.contains("accesscount")) {
		errorMessage += "Missing 'accesscount' field. ";
	}
	else if (!body.at("accesscount").is_number_integer()) {
		errorMessage += "'accesscount' field must be a string or an integer. ";
	}

	return errorMessage;
}

std::string CheckStandardError(const json& body) {
	std::string errorMessage{ };

	if (!body.contains("error")) {
		errorMessage += "Missing 'error' field. ";
	}
	else if (!body.at("error").is_string()) {
		errorMessage += "'url' field must be a string. ";
	}

	return errorMessage; // "" if everything is fine, otherwise - an error message
}


// TEST endpoint --- POST /shorten                                       ---  ..  = shortcode
TEST_F(HttpHandlerTest, HandlerMethodPOST) {
	std::string errorMessage{ };
	Random::StringGenerator generator{ 40, 70 };
	std::string url{ generator.Generate() };
	// created 
	json firstBody;
	firstBody["url"] = url;
	auto request = Request::CreateStandard(http::verb::post, "/shorten", std::move(firstBody));
	auto response = Client.Query(std::move(request), Handler);
	EXPECT_EQ(response.result_int(), 201);

	ParseJSONWithErrorHandling(response.body(), firstBody);
	errorMessage = CheckStandardBody(firstBody);
	EXPECT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// ok
	json secondBody;
	secondBody["url"] = url;
	request = Request::CreateStandard(http::verb::post, "/shorten", std::move(secondBody));
	response = Client.Query(std::move(request), Handler);
	EXPECT_EQ(response.result_int(), 200);

	ParseJSONWithErrorHandling(response.body(), secondBody);
    errorMessage = CheckStandardBody(secondBody);
	EXPECT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// Empty body
	json body;
	request = Request::CreateStandard(http::verb::post, "/shorten", std::move(body));
	response = Client.Query(std::move(request), Handler);
	EXPECT_EQ(response.result_int(), 400);

	ParseJSONWithErrorHandling(response.body(), body);
	errorMessage = CheckStandardError(body);
	EXPECT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// non-existent target
	request = Request::CreateStandard(http::verb::post, url, std::move(body));
	response = Client.Query(std::move(request), Handler);
	EXPECT_EQ(response.result_int(), 404);

	ParseJSONWithErrorHandling(response.body(), body);
	errorMessage = CheckStandardError(body);
	EXPECT_TRUE(CheckStandardError(body).empty()) << errorMessage << "\n";
}


// TEST endpoints --- GET /shorten/.. --- AND --- GET /shorten/../stats  ---  ..  = shortcode
TEST_F(HttpHandlerTest, HandlerMethodGET) {
	std::string errorMessage{ };
	Random::StringGenerator generator{ 40, 70 };
	std::string url{ generator.Generate() };

	// created 
	json json;
	json["url"] = url;
	auto request = Request::CreateStandard(http::verb::post, "/shorten", std::move(json));
	auto response = Client.Query(std::move(request), Handler);

	ASSERT_EQ(response.result_int(), 201)
		<< "Bad response from the server:\n"
		<< "  Expected status code: 201\n"
		<< "  Response: " << response << "\n";

	ParseJSONWithErrorHandling(response.body(), json);

	auto shortCode{ json.at("shortcode").get<std::string>() };

	// get 
	json = nlohmann::json();
	std::string target{ "/shorten/" + shortCode }; // existing shortCode
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 200);

	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckStandardBody(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// get
	json = nlohmann::json();
	target = "/shorten/" + url + url; // non-existent shortCode
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 404);

	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// get stats
	json = nlohmann::json();
	target = "/shorten/" + shortCode + "/stats"; // existing shortCode
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 200);
	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckFullStatsBody(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// access count increases
	int accesscount{ json.at("accesscount").get<int>() };
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);
	json = nlohmann::json();

	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckFullStatsBody(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	EXPECT_GE(json.at("accesscount").get<int>(), accesscount);

	json = nlohmann::json();
	target = "/shorten/" + url + url + "/stats"; // non-existent shortCode
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 404);

	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	json = nlohmann::json();
	target = "/shorten/stats"; // without shortCode
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 404);

	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	json = nlohmann::json();
	target = "/"; // empty target
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 404);

	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";


	json = nlohmann::json();
	target = url + url + "/stats"; // not  -  /shorten/ 
	request = Request::CreateStandard(http::verb::get, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 404);

	ParseJSONWithErrorHandling(response.body(), json);
	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";
}


// TEST endpoints --- PUT /shorten/..
TEST_F(HttpHandlerTest, HandlerMethodPUT) {
	std::string errorMessage{ };
	Random::StringGenerator generator{ 40, 70 };

	// created 
	std::string url{ generator.Generate() };
	json json;
	json["url"] = url;
	auto request = Request::CreateStandard(http::verb::post, "/shorten", std::move(json));
	auto response = Client.Query(std::move(request), Handler);

	ASSERT_EQ(response.result_int(), 201)
		<< "Bad response from the server:\n"
		<< "  Expected status code: 201\n"
		<< "  Response: " << response << "\n";

	ParseJSONWithErrorHandling(response.body(), json);

	auto shortCode{ json.at("shortcode").get<std::string>() };

	// put existing shortCode
	std::string newurl{ generator.Generate() };
	json = nlohmann::json();
	json["url"] = newurl;
	std::string target{ "/shorten/" + shortCode };
	request = Request::CreateStandard(http::verb::put, target, std::move(json));
	response = Client.Query(std::move(request), Handler);

	ParseJSONWithErrorHandling(response.body(), json);

	EXPECT_EQ(response.result_int(), 200);

	errorMessage = CheckStandardBody(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	EXPECT_EQ(newurl, json.at("url").get<std::string>());

	// put "/shorten/" doesn't start
	json = nlohmann::json();
	json["url"] = newurl;
	target = shortCode;
	request = Request::CreateStandard(http::verb::put, target, std::move(json));
	response = Client.Query(std::move(request), Handler);

	ParseJSONWithErrorHandling(response.body(), json);

	EXPECT_EQ(response.result_int(), 404);

	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// put empty body response
	json = nlohmann::json();
	target = "/shorten/" + shortCode;
	request = Request::CreateStandard(http::verb::put, target);
	response = Client.Query(std::move(request), Handler);

	ParseJSONWithErrorHandling(response.body(), json);

	EXPECT_EQ(response.result_int(), 400);

	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";

	// put non-existent short code
	json = nlohmann::json();
	json["url"] = newurl;
	target = "/shorten/" + shortCode + newurl;
	request = Request::CreateStandard(http::verb::put, target, std::move(json));
	response = Client.Query(std::move(request), Handler);

	ParseJSONWithErrorHandling(response.body(), json);

	EXPECT_EQ(response.result_int(), 404);

	errorMessage = CheckStandardError(json);
	ASSERT_TRUE(errorMessage.empty()) << errorMessage << "\n";
}


//TEST endpoints --- DELETE//shorten/..                                  ---  ..  = shortcode
TEST_F(HttpHandlerTest, HandlerMethodDELETE) {
	std::string errorMessage{ };
	Random::StringGenerator generator{ 40, 70 };

	// created 
	std::string url{ generator.Generate() };
	json json;
	json["url"] = url;
	auto request = Request::CreateStandard(http::verb::post, "/shorten", std::move(json));
	auto response = Client.Query(std::move(request), Handler);

	ASSERT_EQ(response.result_int(), 201)
		<< "Bad response from the server:\n"
		<< "  Expected status code: 201\n"
		<< "  Response: " << response << "\n";

	ParseJSONWithErrorHandling(response.body(), json);

	auto shortCode{ json.at("shortcode").get<std::string>() };

	// delete existing shortCode
	std::string target{ "/shorten/" + shortCode };
	request = Request::CreateStandard(http::verb::delete_, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 204);

	// the short code has already been deleted non-existent short code;
	request = Request::CreateStandard(http::verb::delete_, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 404);


	// delete "/shorten/" doesn't start
	target = shortCode;
	request = Request::CreateStandard(http::verb::put, target);
	response = Client.Query(std::move(request), Handler);

	EXPECT_EQ(response.result_int(), 404);
}