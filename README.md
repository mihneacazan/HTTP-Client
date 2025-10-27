# HTTP Client for managing an Online Movie Library

This project implements an HTTP client in C++ capable of interacting with a REST API to manage an online movie library. The client communicates with the server via TCP sockets, manually building and parsing HTTP requests and responses. The application provides a command-line interface (CLI) for various operations, including admin and user authentication, user management, and movie and collection management.

## Project Structure

The project is structured into the following main modules:

*   `client.cpp`: Contains the `main` function and the core logic of the client, including the command-reading loop and dispatch to the corresponding handler functions. It also manages the client's global state (cookies, JWT token, ID lists).
*   `client.h`: Header file for `client.cpp`, containing declarations for the handler functions and global variables.
*   `http_requests.cpp`: Responsible for building HTTP request strings (GET, POST, PUT, DELETE), sending them to the server, and receiving responses. It also includes functions for opening and closing the connection.
*   `http_requests.h`: Header file for `http_requests.cpp`, declaring the request-building functions and the `HttpResponse` structure.
*   `helpers.cpp`: Contains various helper functions, such as reading user input, parsing HTTP responses (extracting cookies, extracting the JSON body), validating data (e.g., `is_number`), and functions for displaying success/error messages.
*   `helpers.h`: Header file for `helpers.cpp`.
*   `nlohmann/json.hpp`: The [nlohmann/json](https://github.com/nlohmann/json) single-header library used for parsing and generating JSON objects.

## JSON Library Used: nlohmann/json

For handling data in JSON format, both for sending payloads to the server and for parsing the received responses, the **nlohmann/json** library was chosen.

**Justification for the choice:**

1.  **Ease of Use:** nlohmann/json offers a C++ friendly interface that integrates naturally with standard data types (e.g., `std::string`, `int`, `double`, `std::vector`, `std::map`).
2.  **Single-Header:** The library consists of a single header file (`json.hpp`), which enormously simplifies integration into the project. There is no need for separate compilation of the library or linking with `.lib` or `.so` files. Including the header in the source files where JSON object manipulation is needed is sufficient.

**Integration into the Project:**
The `nlohmann/json.hpp` file is included in the `nlohmann/` directory within the project's source files. The Makefile is configured to find this header (`CXXFLAGS = -Wall -std=c++17 -I.`).

## Implemented Functionalities

The client supports the following commands, each corresponding to a specific interaction with the server's API:

### Admin Authentication and Session Management
*   **`login_admin`**: Authenticates a user with an admin role. Stores the received session cookie.
*   **`logout_admin`**: Logs out the current admin. Invalidates the session cookie locally.

### User Management (Requires Admin role)
*   **`add_user`**: Adds a new regular user to the server.
*   **`get_users`**: Lists all regular users created by the current admin.
*   **`delete_user`**: Deletes a specified regular user.

### Regular User Authentication and Session Management
*   **`login`**: Authenticates a regular user. Requires specifying the username of the admin who created the user. Stores the session cookie.
*   **`logout`**: Logs out the current regular user. Invalidates the local session cookie and JWT token.

### Access to the Movie Library (Requires Regular User authentication)
*   **`get_access`**: Obtains a JWT token from the server, required to access the movie library functionalities. Requires a user session cookie. The JWT token is stored locally.

### Movie Management (Requires JWT token)
*   **`get_movies`**: Lists all available movies in the library.
*   **`add_movie`**: Adds a new movie to the library. The server returns the ID of the created movie, which is stored locally in a vector (`movie_ids`) to map the indexes displayed to the user (and expected by the checker) to the actual server-side IDs.
*   **`get_movie`**: Displays details about a movie specified by its index (which is internally converted to the actual server-side ID).
*   **`delete_movie`**: Deletes a movie specified by its index (internally converted to the actual ID). The local `movie_ids` vector is updated.
*   **`update_movie`**: Updates the details of a movie specified by its index (internally converted to the actual ID).

### Movie Collection Management (Requires JWT token)
*   **`get_collections`**: Lists all movie collections.
*   **`add_collection`**: Creates a new collection. The server API allows creation with just a title, but the client also reads movie IDs (indexes) which are then added to the collection through separate POST requests to the appropriate route, using the newly created collection's ID. The collection ID is stored locally in a vector (`collection_ids`).
*   **`get_collection`**: Displays details about a collection specified by its index (internally converted to the actual ID).
*   **`delete_collection`**: Deletes a collection specified by its index (internally converted to the actual ID). Requires the user to be the owner of the collection (a check performed by the server). The local `collection_ids` vector is updated.
*   **`add_movie_to_collection`**: Adds a movie (specified by index) to a collection (specified by index). Requires the user to be the owner of the collection.
*   **`delete_movie_from_collection`**: Deletes a movie (specified by index) from a collection (specified by index). Requires the user to be the owner of the collection.

### Other Commands
*   **`exit`**: Closes the client.

## Specific Implementation Details

*   **Managing IDs vs. Indexes:** Because the checker, for certain commands (e.g., `get_movie`, `delete_movie`, `add_collection`), sends IDs that are actually 1-based indexes (relative to the output of `get_movies` commands or the order of addition), the client maintains local vectors (`movie_ids`, `collection_ids`). These vectors store the *actual* IDs returned by the server upon resource creation. When a command receives an "ID" from the checker that is an index, the client uses this index to look up the real ID in the corresponding vector and sends the real ID to the server. Upon deletion, the corresponding element is also removed from the local vector.
*   **Parsing Responses:**
    *   Cookies are extracted from the `Set-Cookie` header.
    *   The JSON body is extracted by considering the `Content-Length` header or, as a fallback, by searching for the first `{` or `[` character.
    *   The HTTP status code is parsed from the first line of the response.
*   **Error Handling:** Validations are implemented for user input (e.g., empty fields, numeric types, the presence of spaces in usernames/passwords).
