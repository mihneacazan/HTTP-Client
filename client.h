#ifndef CLIENT_H
#define CLIENT_H

// Command handlers
void handle_login_admin();
void handle_add_user();
void handle_get_users();
void handle_delete_user();
void handle_logout_admin();
void handle_login();
void handle_get_access();
void handle_get_movies();
void handle_get_movie();
void handle_add_movie();
void handle_delete_movie();
void handle_update_movie();
void handle_get_collections();
void handle_get_collection();
void handle_add_collection();
void handle_delete_collection();
void handle_add_movie_to_collection();
void handle_delete_movie_from_collection();
void handle_logout();

// Helpers
void close_server_connection();

// Check if credentials given by user are valid
bool validate_credentials(const std::string &username, const std::string &password);

// Print error message received from server
void build_error_message(HttpResponse response, const std::string &command_name);

#endif