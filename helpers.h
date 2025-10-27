#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>
#include <iostream>

// Server connection details
#define HOST "63.32.125.183"
#define PORT 8081

#define BUFLEN 4096
#define LINELEN 1000

// Error handling
void error(const char *msg);

// Offers prompt and returns what the user has typed
std::string read_line_with_prompt(const std::string& prompt);

// Extract specific cookie value from HTTP response headers
std::string get_cookie_value(const std::string& response, const std::string& cookie_name);

// Extract JSON body from HTTP response
std::string extract_json_body(const std::string& response);

// Basic input validation
bool is_number(const std::string& s);

// For ERROR or SUCCESS messages
void print_success(const std::string& message);
void print_error(const std::string& message);


#endif // HELPERS_H