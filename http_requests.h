#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

#include <string>
#include <vector>
#include "nlohmann/json.hpp"

// Structure to hold HTTP response
struct HttpResponse {
    int status_code = 0;
    std::string headers;
    std::string body;
    std::string full_response;

    bool is_error() { 
        return status_code < 200 || status_code >= 300;
    }
};

// Opens a connection to the server
int open_connection(const char* host_ip, int portno);

// Closes the connection
void close_connection(int sockfd);

// Sends an HTTP request and receives the response
HttpResponse send_request_get_reply(int sockfd, const std::string& request_str);


// Request computation functions
std::string compute_get_request(const std::string& host, const std::string& url,
                                const std::string& query_params,
                                const std::vector<std::string>& cookies,
                                const std::string& jwt_token);

std::string compute_post_request(const std::string& host, const std::string& url,
                                const std::string& content_type,
                                const nlohmann::json& body_data,
                                const std::vector<std::string>& cookies,
                                const std::string& jwt_token);

std::string compute_delete_request(const std::string& host, const std::string& url,
                                const std::vector<std::string>& cookies,
                                const std::string& jwt_token);

std::string compute_put_request(const std::string& host, const std::string& url,
                                const std::string& content_type,
                                const nlohmann::json& body_data,
                                const std::vector<std::string>& cookies,
                                const std::string& jwt_token);

#endif // HTTP_REQUESTS_H