#include "http_requests.h"
#include "helpers.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

int open_connection (const char* host_ip, int portno) {
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    if (inet_pton(AF_INET, host_ip, &serv_addr.sin_addr) <= 0) {
        error("ERROR inet_pton");
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
    return sockfd;
}

void close_connection(int sockfd) {
    close(sockfd);
}

HttpResponse send_request_get_reply (int sockfd, const std::string& request_str) {
    // Send message
    int bytes, sent = 0;
    int total = request_str.length();
    do {
        bytes = write(sockfd, request_str.c_str() + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }
        if (bytes == 0) {
            break;
        }
        sent += bytes;
    } while (sent < total);

    // Receive response
    std::string response_str;
    char buffer[BUFLEN];
    int header_end_pos = -1;
    unsigned long content_length = 0;
    bool header_parsed = false;

    // Read headers first
    while (true) {
        bytes = read(sockfd, buffer, BUFLEN - 1);
        if (bytes < 0) {
            error("ERROR reading response from socket");
        }
        if (bytes == 0) {
            break;
        }
        buffer[bytes] = '\0';
        response_str.append(buffer, bytes);

        header_end_pos = response_str.find("\r\n\r\n");
        if (header_end_pos != -1) {
            // Headers received, parse Content-Length
            header_parsed = true;
            std::string headers_part = response_str.substr(0, header_end_pos);
            size_t content_length_pos = headers_part.find("Content-Length: ");
             if (content_length_pos == std::string::npos) { // Try case insensitive for Content-Length
                std::string lower_headers = headers_part;
                std::transform(lower_headers.begin(), lower_headers.end(), lower_headers.begin(), ::tolower);
                content_length_pos = lower_headers.find("content-length: ");
                if (content_length_pos != std::string::npos) {
                    // find original case start
                    size_t original_content_length_pos = response_str.find(headers_part.substr(content_length_pos, 16));
                    if(original_content_length_pos != std::string::npos) content_length_pos = original_content_length_pos; else content_length_pos = std::string::npos;
                }
            }

            if (content_length_pos != std::string::npos) {
                size_t content_length_val_start = content_length_pos + strlen("Content-Length: ");
                size_t content_length_val_end = headers_part.find("\r\n", content_length_val_start);
                if (content_length_val_end != std::string::npos)
                    content_length = std::stoul(headers_part.substr(content_length_val_start, content_length_val_end - content_length_val_start));
            }
            break; 
        }
    }
    
    // Read body based on Content-Length
    if (header_parsed) {
        size_t current_body_length = response_str.length() - (header_end_pos + 4);
        while (current_body_length < content_length) {
            bytes = read(sockfd, buffer, BUFLEN - 1);
            if (bytes < 0) {
                error("ERROR reading response body from socket");
            }
            if (bytes == 0) {
                break;
            }
            buffer[bytes] = '\0';
            response_str.append(buffer, bytes);
            current_body_length += bytes;
        }
    }


    HttpResponse response;
    response.full_response = response_str;

    // Status code parsing
    size_t first_space = response_str.find(" ");
    if (first_space != std::string::npos) {
        size_t second_space = response_str.find(" ", first_space + 1);
        if (second_space != std::string::npos) {
            try {
                response.status_code = std::stoi(response_str.substr(first_space + 1, second_space - (first_space + 1)));
            } catch (const std::exception& e) {
                response.status_code = 0; // Error parsing
            }
        }
    }
    
    if (header_end_pos != -1) {
        response.headers = response_str.substr(0, header_end_pos);
        response.body = extract_json_body(response_str);
    } else {
        response.body = response_str;
    }

    return response;
}


std::string compute_get_request (const std::string& host, const std::string& url,
                                const std::string& query_params,
                                const std::vector<std::string>& cookies,
                                const std::string& jwt_token) {
    std::ostringstream request_string;
    request_string << "GET " << url;
    if (!query_params.empty()) {
        request_string << "?" << query_params;
    }
    request_string << " HTTP/1.1\r\n";
    request_string << "Host: " << host << "\r\n";
    if (!cookies.empty()) {
        request_string << "Cookie: ";
        for (size_t i = 0; i < cookies.size(); ++i) {
            request_string << cookies[i] << (i == cookies.size() - 1 ? "" : "; ");
        }
        request_string << "\r\n";
    }
    if (!jwt_token.empty()) {
        request_string << "Authorization: Bearer " << jwt_token << "\r\n";
    }
    request_string << "Connection: keep-alive\r\n";
    request_string << "\r\n"; // End of headers
    return request_string.str();
}

std::string compute_post_request (const std::string& host, const std::string& url,
                                const std::string& content_type,
                                const nlohmann::json& body_data,
                                const std::vector<std::string>& cookies,
                                const std::string& jwt_token) {
    std::ostringstream request_string;
    std::string body_str = body_data.dump();

    request_string << "POST " << url << " HTTP/1.1\r\n";
    request_string << "Host: " << host << "\r\n";
    request_string << "Content-Type: " << content_type << "\r\n";
    request_string << "Content-Length: " << body_str.length() << "\r\n";
    if (!cookies.empty()) {
        request_string << "Cookie: ";
        for (size_t i = 0; i < cookies.size(); ++i) {
            request_string << cookies[i] << (i == cookies.size() - 1 ? "" : "; ");
        }
        request_string << "\r\n";
    }
    if (!jwt_token.empty()) {
        request_string << "Authorization: Bearer " << jwt_token << "\r\n";
    }
    request_string << "Connection: keep-alive\r\n";
    request_string << "\r\n"; // End of headers
    request_string << body_str;
    return request_string.str();
}

std::string compute_delete_request (const std::string& host, const std::string& url,
                                    const std::vector<std::string>& cookies,
                                    const std::string& jwt_token) {
    std::ostringstream request_string;
    request_string << "DELETE " << url << " HTTP/1.1\r\n";
    request_string << "Host: " << host << "\r\n";
    if (!cookies.empty()) {
        request_string << "Cookie: ";
        for (size_t i = 0; i < cookies.size(); ++i) {
            request_string << cookies[i] << (i == cookies.size() - 1 ? "" : "; ");
        }
        request_string << "\r\n";
    }
    if (!jwt_token.empty()) {
        request_string << "Authorization: Bearer " << jwt_token << "\r\n";
    }
    request_string << "Connection: keep-alive\r\n";
    request_string << "\r\n"; // End of headers
    return request_string.str();
}

std::string compute_put_request (const std::string& host, const std::string& url,
                                const std::string& content_type,
                                const nlohmann::json& body_data,
                                const std::vector<std::string>& cookies,
                                const std::string& jwt_token) {
    std::ostringstream request_string;
    std::string body_str = body_data.dump();

    request_string << "PUT " << url << " HTTP/1.1\r\n";
    request_string << "Host: " << host << "\r\n";
    request_string << "Content-Type: " << content_type << "\r\n";
    request_string << "Content-Length: " << body_str.length() << "\r\n";
    if (!cookies.empty()) {
        request_string << "Cookie: ";
        for (size_t i = 0; i < cookies.size(); ++i) {
            request_string << cookies[i] << (i == cookies.size() - 1 ? "" : "; ");
        }
        request_string << "\r\n";
    }
    if (!jwt_token.empty()) {
        request_string << "Authorization: Bearer " << jwt_token << "\r\n";
    }
    request_string << "Connection: keep-alive\r\n";
    request_string << "\r\n"; // End of headers
    request_string << body_str;
    return request_string.str();
}