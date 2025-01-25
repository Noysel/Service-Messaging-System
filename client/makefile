CFLAGS := -c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS := -lboost_system -lpthread

# Target to build all executables
all: StompEMIClient

# Build the StompEMIClient executable
StompEMIClient: bin/ConnectionHandler.o bin/event.o bin/StompClient.o
	g++ -o bin/StompEMIClient bin/ConnectionHandler.o bin/event.o bin/StompClient.o $(LDFLAGS)

# Compile the ConnectionHandler object
bin/ConnectionHandler.o: src/ConnectionHandler.cpp
	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

# Compile the Event object
bin/event.o: src/event.cpp
	g++ $(CFLAGS) -o bin/event.o src/event.cpp

# Compile the StompClient object
bin/StompClient.o: src/StompClient.cpp
	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

# Clean build artifacts
.PHONY: clean
clean:
	rm -f bin/*
