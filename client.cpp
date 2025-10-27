#include <iostream>
#include <string>
#include <vector>
#include <set>
#include "helpers.h"
#include "http_requests.h"
#include "client.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

std::string admin_cookie;
std::string user_cookie;
std::string jwt_token;
std::string admin_username;
int sockfd = -1; // Global socket descriptor
std::vector<int> movie_ids, collection_ids;
std::set<std::string> logged_users;

void close_server_connection() {
    if (sockfd >= 0) {
        close_connection(sockfd);
        sockfd = -1;
    }
}

bool validate_credentials(const std::string &username, const std::string &password)
{
    if (username.empty()) {
        print_error("Username cannot be empty.");
        return false;
    }
    if (password.empty()) {
        print_error("Password cannot be empty.");
        return false;
    }
    if (username.find(' ') != std::string::npos || password.find(' ') != std::string::npos) {
        print_error("Username and password cannot contain spaces.");
        return false;
    }

    return true;
}

void build_error_message(HttpResponse response, const std::string &command_name) {
    std::string error_msg = "Failed to " + command_name + ". HTTP " + std::to_string(response.status_code);
    if (!response.body.empty()) {
        json err_json = json::parse(response.body);
        if (err_json.contains("error")) error_msg += " Server: " + err_json["error"].get<std::string>();
        else error_msg += " Server response body: " + response.body;
    }
    print_error(error_msg);
}

int main() {
    std::string command;

    while (1) {
        std::cin >> command;
        if (std::cin.eof() || command == "exit") {
            break;
        }
        std::cin.ignore(); // Consume the newline after reading the command

        sockfd = open_connection(HOST, PORT);
        if (sockfd < 0) {
            print_error("Connection with server failed. Closing");
            return 1;
        }

        if (command == "login_admin") handle_login_admin();
        else if (command == "add_user") handle_add_user();
        else if (command == "get_users") handle_get_users();
        else if (command == "delete_user") handle_delete_user();
        else if (command == "logout_admin") handle_logout_admin();
        else if (command == "login") handle_login();
        else if (command == "get_access") handle_get_access();
        else if (command == "get_movies") handle_get_movies();
        else if (command == "get_movie") handle_get_movie();
        else if (command == "add_movie") handle_add_movie();
        else if (command == "delete_movie") handle_delete_movie();
        else if (command == "update_movie") handle_update_movie();
        else if (command == "get_collections") handle_get_collections();
        else if (command == "get_collection") handle_get_collection();
        else if (command == "add_collection") handle_add_collection();
        else if (command == "delete_collection") handle_delete_collection();
        else if (command == "add_movie_to_collection") handle_add_movie_to_collection();
        else if (command == "delete_movie_from_collection") handle_delete_movie_from_collection();
        else if (command == "logout") handle_logout();
        else print_error("Unknown command: " + command);
    }

    close_server_connection();
    return 0;
}

// Commands Handlers

void handle_login_admin() {
    std::string username = read_line_with_prompt("username=");
    std::string password = read_line_with_prompt("password=");

    if (!validate_credentials(username, password))
        return;
    if (!admin_cookie.empty()) {
        print_error("Admin already logged in.");
        return;
    }

    json payload = {
        {"username", username},
        {"password", password}
    };

    std::string request = compute_post_request(HOST, "/api/v1/tema/admin/login", "application/json", payload, {}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "login admin");
    } else {
        admin_cookie = get_cookie_value(res.full_response, "session");
        if (admin_cookie.empty()) {
            print_error("Admin login succeeded but no session cookie received.");
        } else {
            admin_username = username;
            std::cout << "Admin username: " + admin_username << "\n";
            print_success("Admin authenticated successfully.");
        }
    }
}

void handle_add_user() {
    if (admin_cookie.empty()) {
        print_error("Admin not logged in. Please login_admin first.");
        return;
    }

    std::string username = read_line_with_prompt("username=");
    std::string password = read_line_with_prompt("password=");

    json payload = {
        {"username", username},
        {"password", password}
    };

    std::string request = compute_post_request(HOST, "/api/v1/tema/admin/users", "application/json", payload, {admin_cookie}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) build_error_message(res, "add user");
    else print_success("User added.");
}

void handle_get_users() {
    if (admin_cookie.empty()) {
        print_error("Admin not logged in.");
        return;
    }

    std::string request = compute_get_request(HOST, "/api/v1/tema/admin/users", "", {admin_cookie}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "get users");
    } else {
        json users_json = json::parse(res.body);
        if (users_json.contains("users") && users_json["users"].is_array()) {
            print_success("User list:");
            int users_count = 0;
            for (const auto& user_entry : users_json["users"]) {
                std::cout << "#" << ++users_count << " "
                        << user_entry["username"].get<std::string>() << ":"
                        << user_entry["password"].get<std::string>() << std::endl;
            }
        } else {
            print_error("User list format unexpected.");
        }
    }
}

void handle_delete_user() {
    if (admin_cookie.empty()) {
        print_error("Admin not logged in. Please login_admin first.");
        return;
    }
    std::string username = read_line_with_prompt("username=");
    std::string url = "/api/v1/tema/admin/users/" + username;

    std::string request = compute_delete_request(HOST, url, {admin_cookie}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "delete user");
    } else {
        print_success("User " + username + " deleted.");
    }
}

void handle_logout_admin() {
    if (admin_cookie.empty()) {
        print_error("Admin not logged in. Nothing to logout from.");
        return;
    }
    std::string request = compute_get_request(HOST, "/api/v1/tema/admin/logout", "", {admin_cookie}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "logout admin");
    } else {
        print_success("Admin logged out successfully.");
    }

    admin_cookie.clear(); // Cookie shouldn't exist no more regardless of server response
}

void handle_login() {
    std::string admin_username_stdin = read_line_with_prompt("admin_username=");
    if (admin_username_stdin != admin_username) {
        print_error("Wrong admin username.");
        return;
    }
    std::string username = read_line_with_prompt("username=");
    std::string password = read_line_with_prompt("password=");

    if (!validate_credentials(username, password))
        return;

    json payload = {
        {"admin_username", admin_username},
        {"username", username},
        {"password", password}
    };
    
    std::string request = compute_post_request(HOST, "/api/v1/tema/user/login", "application/json", payload, {}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "login");
    } else {
        user_cookie = get_cookie_value(res.full_response, "session");
        if (user_cookie.empty()) {
            print_error("User login succeeded but no session cookie received.");
        } else {
            print_success("User authenticated successfully.");
        }
    }
}

void handle_get_access() {
    if (user_cookie.empty()) {
        print_error("User not logged in. Please login first.");
        return;
    }
    std::string request = compute_get_request(HOST, "/api/v1/tema/library/access", "", {user_cookie}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "get access");
    } else {
        json access_json = json::parse(res.body);
        if (access_json.contains("token")) {
            jwt_token = access_json["token"].get<std::string>();
            print_success("Library access granted. JWT token received.");
        } else {
            print_error("Library access response did not contain a token.");
        }
    }
}

void handle_get_movies() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }
    std::string request = compute_get_request(HOST, "/api/v1/tema/library/movies", "", {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "get movies");
    } else {
        json response_json = json::parse(res.body);
        json movies_array = response_json["movies"];

        print_success("Movies list:");
        int movie_counter = 0;
        for (const auto& movie : movies_array) {
            std::string title = movie.value("title", "N/A");
            std::cout << "#" << ++movie_counter << " " << title << std::endl;
        }
    }
}

void handle_get_movie() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }
    std::string id = read_line_with_prompt("id=");
    if (std::stoi(id) > (int)movie_ids.size()) {
        print_error("Invalid movie ID.");
        return;
    }

    std::string movie_id = std::to_string(movie_ids[std::stoi(id) - 1]);
    std::string url = "/api/v1/tema/library/movies/" + movie_id;

    std::string request = compute_get_request(HOST, url, "", {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "get movie");
    } else {
        json movie_json = json::parse(res.body);
        print_success("Movie details (ID: " + movie_id + "):");
        std::cout << "title: " << movie_json["title"].get<std::string>() << std::endl;
        std::cout << "year: " << movie_json["year"].get<int>() << std::endl;
        std::cout << "description: " << movie_json["description"].get<std::string>() << std::endl;
        std::cout << "rating: " << movie_json["rating"].get<std::string>() << std::endl;
    }
}

void handle_add_movie() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string title = read_line_with_prompt("title=");
    std::string year = read_line_with_prompt("year=");
    std::string description = read_line_with_prompt("description=");
    std::string rating = read_line_with_prompt("rating=");

    if (!is_number(year) || !is_number(rating)) {
        print_error("Year and rating must be numbers.");
        return;
    }

    json payload = {
        {"title", title},
        {"year", std::stoi(year)},
        {"description", description},
        {"rating", std::stod(rating)}
    };

    std::string request = compute_post_request(HOST, "/api/v1/tema/library/movies", "application/json", payload, {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "add movie");
    } else {
        int movie_id = json::parse(res.body)["id"].get<int>();
        movie_ids.push_back(movie_id);
        print_success("Movie added successfully.");
    }
}

void handle_delete_movie() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string id = read_line_with_prompt("id=");
    if (!is_number(id) || std::stoi(id) > (int)movie_ids.size()) {
        print_error("Invalid ID.");
        return;
    }
    
    movie_ids.erase(movie_ids.begin() + std::stoi(id) - 1);
    std::string movie_id = std::to_string(movie_ids[std::stoi(id) - 1]);
    std::string url = "/api/v1/tema/library/movies/" + movie_id;

    std::string request = compute_delete_request(HOST, url, {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) build_error_message(res, "delete movie");
    else print_success("Movie " + movie_id + " deleted successfully.");
}

void handle_update_movie() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string id = read_line_with_prompt("id=");
    std::string new_title = read_line_with_prompt("title=");
    std::string year = read_line_with_prompt("year=");
    std::string description = read_line_with_prompt("description=");
    std::string rating = read_line_with_prompt("rating=");

    if (!is_number(year) || !is_number(rating)) {
        print_error("Year and rating must be numbers.");
        return;
    }

    if (!is_number(id) || std::stoi(id) > (int)movie_ids.size()) {
        print_error("Invalid ID.");
        return;
    }

    std::string movie_id = std::to_string(movie_ids[std::stoi(id) - 1]);
    std::string url = "/api/v1/tema/library/movies/" + movie_id;

    json payload = {
        {"title", new_title},
        {"year", year},
        {"description", description},
        {"rating", std::stod(rating)}
    };
    
    std::string request = compute_put_request(HOST, url, "application/json", payload, {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) build_error_message(res, "update movie");
    else print_success("Movie " + movie_id + " updated successfully.");
}

void handle_get_collections() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string request = compute_get_request(HOST, "/api/v1/tema/library/collections", "", {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "get collections");
    } else {
        json response_json = json::parse(res.body);
        json collections = response_json["collections"];
        
        print_success("Collections list:");
        int collection_count = 0;
        for (const auto& coll : collections) { 
            std::string title = coll.value("title", "N/A");
            std::cout << "#" << ++collection_count << ": " << title << std::endl;
        }
    }
}

void handle_get_collection() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string id = read_line_with_prompt("id=");
    if (!is_number(id) || std::stoi(id) <= 0 || std::stoi(id) > (int)collection_ids.size()) {
        print_error("Invalid ID.");
        return;
    }

    int coll_id = collection_ids[std::stoi(id) - 1];
    std::string url = "/api/v1/tema/library/collections/" + std::to_string(coll_id);

    std::string request = compute_get_request(HOST, url, "", {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "get collection");
    } else {
        json coll_json = json::parse(res.body);
        print_success("Collection details (ID: " + std::to_string(coll_id) + "):");
        std::cout << "title: " << coll_json["title"].get<std::string>() << std::endl;
        std::cout << "owner: " << coll_json["owner"].get<std::string>() << std::endl;
        if (coll_json.contains("movies") && coll_json["movies"].is_array()) {
            std::cout << "Movies in collection:" << std::endl;
            for (const auto& movie : coll_json["movies"]) {
                std::cout << "#" << movie["id"].get<int>() << ": " << movie["title"].get<std::string>() << std::endl;
            }
        }
    }
}

void handle_add_collection() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string title = read_line_with_prompt("title=");
    std::string num_movies = read_line_with_prompt("num_movies=");
    if (!is_number(num_movies)) {
        print_error("num_movies must be a number");
        return;
    }

    std::vector<int> ids;
    for (int i = 0; i < std::stoi(num_movies); i++) {
        std::string prompt = "movie_id[" + std::to_string(i) + "]=";
        std::string movie_id = read_line_with_prompt(prompt);
        if (!is_number(movie_id) || std::stoi(movie_id) > (int)movie_ids.size()) {
            print_error("Invalid ID.");
            return;
        }
        ids.push_back(std::stoi(movie_id));
    }

    json payload = { {"title", title} };
    std::string request = compute_post_request(HOST, "/api/v1/tema/library/collections", "application/json", payload, {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) {
        build_error_message(res, "add collection");
    } else {
        int coll_id = json::parse(res.body)["id"].get<int>();
        collection_ids.push_back(coll_id);
        std::string url = "/api/v1/tema/library/collections/" + std::to_string(coll_id) + "/movies";

        for (int i = 0; i < std::stoi(num_movies); i++) {
            json payload = { {"id", movie_ids[ids[i] - 1]} }; // Payload is {"id": Number} for movie ID
            std::string request = compute_post_request(HOST, url, "application/json", payload, {}, jwt_token);
            send_request_get_reply(sockfd, request); // Send POST for every movie added in collection
        }
        print_success("Collection added successfully.");
    }
}

void handle_delete_collection() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string coll_id = read_line_with_prompt("id=");
    if (!is_number(coll_id) || std::stoi(coll_id) <= 0 || std::stoi(coll_id) > (int)collection_ids.size()) {
        print_error("Invalid ID.");
        return;
    }
    std::string url = "/api/v1/tema/library/collections/" + std::to_string(collection_ids[std::stoi(coll_id) - 1]);

    std::string request = compute_delete_request(HOST, url, {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);
    collection_ids.erase(collection_ids.begin() + std::stoi(coll_id) - 1);

    if (res.is_error()) build_error_message(res, "delete collection");
    else print_success("Collection " + coll_id + " deleted successfully.");
}

void handle_add_movie_to_collection() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string coll_id = read_line_with_prompt("collection_id=");
    std::string movie_id = read_line_with_prompt("movie_id=");
    if (!is_number(coll_id) || !is_number(movie_id) || std::stoi(coll_id) <= 0 || std::stoi(coll_id) > (int)collection_ids.size()
        || std::stoi(movie_id) <= 0 || std::stoi(movie_id) > (int)movie_ids.size()) {
        print_error("Collection and movie ids must be valid numbers");
        return;
    }
    std::string url = "/api/v1/tema/library/collections/" + std::to_string(collection_ids[std::stoi(coll_id) - 1]) + "/movies";
    
    json payload = { {"id", movie_ids[std::stoi(movie_id) - 1]} }; // Payload is {"id": Number} for movie ID

    std::string request = compute_post_request(HOST, url, "application/json", payload, {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) build_error_message(res, "add movie to collection");
    else print_success("Movie added to collection successfully.");
}

void handle_delete_movie_from_collection() {
    if (jwt_token.empty()) {
        print_error("No library access.");
        return;
    }

    std::string coll_id = read_line_with_prompt("collection_id=");
    std::string movie_id = read_line_with_prompt("movie_id=");
    if (!is_number(coll_id) || !is_number(movie_id) || std::stoi(coll_id) <= 0 || std::stoi(coll_id) > (int)collection_ids.size()
        || std::stoi(movie_id) <= 0 || std::stoi(movie_id) > (int)movie_ids.size()) {
        print_error("Collection and movie ids must be valid numbers.");
        return;
    }

    std::string url = "/api/v1/tema/library/collections/" + std::to_string(collection_ids[std::stoi(coll_id) - 1])
                        + "/movies/" + std::to_string(movie_ids[std::stoi(movie_id) - 1]);

    std::string request = compute_delete_request(HOST, url, {}, jwt_token);
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) build_error_message(res, "delete movie from collection");
    else print_success("Movie deleted from collection successfully.");
}

void handle_logout() {
    jwt_token.clear(); // Access to JWT token must be lost
    if (user_cookie.empty()) {
        print_error("User not logged in. Nothing to logout from.");
        return;
    }

    std::string request = compute_get_request(HOST, "/api/v1/tema/user/logout", "", {user_cookie}, "");
    HttpResponse res = send_request_get_reply(sockfd, request);

    if (res.is_error()) build_error_message(res, "logout");
    else print_success("User logged out successfully.");

    user_cookie.clear();
}