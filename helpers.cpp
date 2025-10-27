#include "helpers.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cctype>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

std::string read_line_with_prompt(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::string get_cookie_value(const std::string& response, const std::string& cookie_name) {
    std::string cookie_header_start = "Set-Cookie: ";
    size_t start_pos = 0;

    while ((start_pos = response.find(cookie_header_start, start_pos)) != std::string::npos) {
        start_pos += cookie_header_start.length();
        size_t end_pos = response.find(";", start_pos);
        if (end_pos == std::string::npos) { // Cookie might be last part of header line
            end_pos = response.find("\r\n", start_pos);
        }
        if (end_pos == std::string::npos) continue; // Should not happen

        std::string cookie_full = response.substr(start_pos, end_pos - start_pos);
        size_t name_end_pos = cookie_full.find("=");
        if (name_end_pos != std::string::npos) {
            std::string current_cookie_name = cookie_full.substr(0, name_end_pos);
            if (current_cookie_name == cookie_name) {
                return cookie_full; // Return "name=value"
            }
        }
    }
    return ""; // Cookie not found
}

std::string extract_json_body(const std::string& response) {
    size_t body_start = response.find("\r\n\r\n");
    if (body_start == std::string::npos) {
        return "";
    }

    body_start += 4; // Length of "\r\n\r\n"
    std::string content_length_header = "Content-Length: ";
    size_t content_length_pos = response.find(content_length_header);
    if (content_length_pos == std::string::npos) {
        // Try case-insensitive
        std::string lower_response = response;
        std::transform(lower_response.begin(), lower_response.end(), lower_response.begin(), ::tolower);
        std::string lower_cl_header = "content-length: ";
        content_length_pos = lower_response.find(lower_cl_header);
        if (content_length_pos == std::string::npos) {
            // If no Content-Length, try to find the first '{' or '['
            size_t json_actual_start = response.find_first_of("{[", body_start);
            if (json_actual_start != std::string::npos) {
                return response.substr(json_actual_start);
            }
            return response.substr(body_start);
        }
        content_length_pos = response.find(content_length_header.substr(0,14), content_length_pos); // find original case "Content-Length: "
    }
    
    if (content_length_pos != std::string::npos && content_length_pos < body_start) { // Ensure Content-Length is in headers
        size_t content_length_value_start = content_length_pos + content_length_header.length();
        size_t content_length_value_end = response.find("\r\n", content_length_value_start);
        if (content_length_value_end != std::string::npos) {
            int length = std::stoi(response.substr(content_length_value_start, content_length_value_end - content_length_value_start));
            if (body_start + length <= response.length()) {
                // Ensure the body actually starts with { or [ if expecting JSON
                size_t json_actual_start = response.find_first_of("{[", body_start);
                if (json_actual_start != std::string::npos && json_actual_start < body_start + 10) { // within first few chars
                    return response.substr(json_actual_start, length - (json_actual_start - body_start));
                }
                return response.substr(body_start, length);
            }
        }
    }

    // if Content-Length is missing , find the first '{' or '[' indicating
    // start of JSON (this shouldn't happen, just a safety measure)
    size_t json_actual_start = response.find_first_of("{[", body_start);
    if (json_actual_start != std::string::npos) {
        return response.substr(json_actual_start);
    }

    return response.substr(body_start);
}

bool is_number(const std::string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    strtod(s.c_str(), &end); // Try to convert to double
    // Check if the entire string was consumed and it's not just whitespace
    return end != s.c_str() && *end == '\0';
}


void print_success(const std::string& message) {
    std::cout << "SUCCESS: " << message << std::endl;
}

void print_error(const std::string& message) {
    std::cout << "ERROR: " << message << std::endl;
}