# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -I. # -I. for nlohmann/json.hpp in a subdirectory
LDFLAGS =

# Source files
SRCS = client.cpp http_requests.cpp helpers.cpp
OBJS = $(SRCS:.cpp=.o)

# Executable name
TARGET = client

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile .cpp files to .o files
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean