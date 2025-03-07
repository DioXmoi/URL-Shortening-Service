# URL Shortening Service

A simple and efficient URL shortening service built using C++ with Boost.  This service allows users to shorten long URLs into shorter, manageable URLs and retrieve the original long URL from the shortened version.

## Features

*   **Shortens long URLs:** Transforms long URLs into unique, short codes.
*   **Persistent Storage:**  Stores URLs and their corresponding short codes in a persistent PostgreSQL database (using `libpq`).
*   **RESTful API:** Provides a RESTful API for creating, retrieving, updating, and deleting shortened URLs.
*   **Error Handling:** Implements proper HTTP status codes for various scenarios, including validation errors and resource not found.
*   **Statistics Tracking:** Tracks access counts for each shortened URL.

## Technologies Used

*   **C++:**  The core language for the service.
*   **Boost:**  Used for networking (Asio), JSON handling (JSON), date/time (DateTime), and more.
*   **gtest:**  Google Test framework for unit and integration testing.
*   **CMake:**  Cross-platform build system for easy compilation and management.
*   **libpq:**  PostgreSQL client library for interacting with the database.

## Prerequisites

*   C++ compiler (e.g., g++)
*   Boost libraries (version compatible with your compiler)
*   CMake
*   PostgreSQL database and `libpq`
*   gtest (optional, for running tests)

## API Endpoints

The service exposes the following RESTful API endpoints:

### Create Short URL

*   **Method:** `POST`
*   **Endpoint:** `/shorten`
*   **Body:**

    ```json
    {
      "url": "https://www.example.com/some/long/url"
    }
    ```

*   **Success Response (201 Created):**

    ```json
    {
      "id": "1",
      "url": "https://www.example.com/some/long/url",
      "shortCode": "abc123",
      "createdAt": "2021-09-01T12:00:00Z",
      "updatedAt": "2021-09-01T12:00:00Z"
    }
    ```

*   **Error Response (400 Bad Request):** Returns error messages for invalid input (e.g., invalid URL).

### Retrieve Original URL

*   **Method:** `GET`
*   **Endpoint:** `/shorten/{shortCode}` (e.g., `/shorten/abc123`)
*   **Success Response (200 OK):**

    ```json
    {
      "id": "1",
      "url": "https://www.example.com/some/long/url",
      "shortCode": "abc123",
      "createdAt": "2021-09-01T12:00:00Z",
      "updatedAt": "2021-09-01T12:00:00Z"
    }
    ```

*   **Error Response (404 Not Found):** Returned if the short code does not exist. **Important:** The frontend is responsible for redirecting to the `url` returned in the response.

### Update Short URL

*   **Method:** `PUT`
*   **Endpoint:** `/shorten/{shortCode}` (e.g., `/shorten/abc123`)
*   **Body:**

    ```json
    {
      "url": "https://www.example.com/some/updated/url"
    }
    ```

*   **Success Response (200 OK):**

    ```json
    {
      "id": "1",
      "url": "https://www.example.com/some/updated/url",
      "shortCode": "abc123",
      "createdAt": "2021-09-01T12:00:00Z",
      "updatedAt": "2021-09-01T12:30:00Z"
    }
    ```

*   **Error Response (400 Bad Request):** Returns error messages for invalid input (e.g., invalid URL).
*   **Error Response (404 Not Found):** Returned if the short code does not exist.

### Delete Short URL

*   **Method:** `DELETE`
*   **Endpoint:** `/shorten/{shortCode}` (e.g., `/shorten/abc123`)
*   **Success Response (204 No Content):** Returned if the short code is successfully deleted.
*   **Error Response (404 Not Found):** Returned if the short code does not exist.

### Get URL Statistics

*   **Method:** `GET`
*   **Endpoint:** `/shorten/{shortCode}/stats` (e.g., `/shorten/abc123/stats`)
*   **Success Response (200 OK):**

    ```json
    {
      "id": "1",
      "url": "https://www.example.com/some/long/url",
      "shortCode": "abc123",
      "createdAt": "2021-09-01T12:00:00Z",
      "updatedAt": "2021-09-01T12:00:00Z",
      "accessCount": 10
    }
    ```

*   **Error Response (404 Not Found):** Returned if the short code does not exist.


## Contributing

Contributions are welcome! This project idea is based on the [URL Shortening Service project](https://roadmap.sh/projects/url-shortening-service) from roadmap.sh. Please submit pull requests with clear descriptions of the changes you're proposing. When contributing, please consider the design and requirements outlined in the roadmap.sh project description to ensure alignment with the overall goals.
