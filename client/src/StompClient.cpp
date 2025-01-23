#include "../include/ConnectionHandler.h"
#include "../include/event.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

std::mutex connectionMutex;
bool shouldTerminate = false;

// Function to handle user input
void handleUserInput(ConnectionHandler &connectionHandler) {
    while (!shouldTerminate) {
        std::string input;
        std::getline(std::cin, input);

        if (input.find("login") == 0) {
            std::stringstream ss(input);
            std::string command, hostPort, username, password;
            ss >> command >> hostPort >> username >> password;

            std::string frame = "CONNECT\naccept-version:1.2\nhost:" + hostPort +
                                "\nlogin:" + username + "\npasscode:" + password + "\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame)) {
                std::cerr << "Failed to send login frame." << std::endl;
                break;
            }
        } else if (input.find("join") == 0) {
            std::stringstream ss(input);
            std::string command, channel;
            ss >> command >> channel;

            static int subscriptionId = 0;
            subscriptionId++;

            std::string frame = "SUBSCRIBE\ndestination:/" + channel + "\nid:" +
                                std::to_string(subscriptionId) + "\nreceipt:" +
                                std::to_string(subscriptionId + 100) + "\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame)) {
                std::cerr << "Failed to send join frame." << std::endl;
                break;
            }
        } else if (input.find("exit") == 0) {
            std::stringstream ss(input);
            std::string command, channel;
            ss >> command >> channel;

            static int subscriptionId = 0;
            subscriptionId++;

            std::string frame = "UNSUBSCRIBE\nid:" + std::to_string(subscriptionId) + "\nreceipt:" +
                                std::to_string(subscriptionId + 200) + "\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame)) {
                std::cerr << "Failed to send exit frame." << std::endl;
                break;
            }
        } else if (input.find("report") == 0) {
            std::stringstream ss(input);
            std::string command, filePath;
            ss >> command >> filePath;

            names_and_events parsedEvents = parseEventsFile(filePath);
            for (const Event &event : parsedEvents.events) {
                std::string frame = "SEND\ndestination:/" + parsedEvents.channel_name +
                                    "\nuser:" + event.getEventOwnerUser() +
                                    "\ncity:" + event.get_city() +
                                    "\nevent name:" + event.get_name() +
                                    "\ndate time:" + std::to_string(event.get_date_time()) +  // Convert to string
                                    "\n\n" + event.get_description() + "\0";

                std::lock_guard<std::mutex> lock(connectionMutex);
                if (!connectionHandler.sendLine(frame)) {
                    std::cerr << "Failed to send report frame." << std::endl;
                    break;
                }
            }
        } else if (input.find("summary") == 0) {
            std::stringstream ss(input);
            std::string command, channel, user, filePath;
            ss >> command >> channel >> user >> filePath;

            std::ofstream outFile(filePath);
            if (!outFile.is_open()) {
                std::cerr << "Failed to open file: " << filePath << std::endl;
                continue;
            }

            outFile << "Channel: " << channel << "\nStats:\n";
            outFile.close();
        } else if (input == "logout") {
            std::string frame = "DISCONNECT\nreceipt:999\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame)) {
                std::cerr << "Failed to send logout frame." << std::endl;
            }
            shouldTerminate = true;
            break;
        } else {
            std::cerr << "Unknown command." << std::endl;
        }
    }
}

// Function to handle server responses
void handleServerResponses(ConnectionHandler &connectionHandler) {
    while (!shouldTerminate) {
        std::string response;
        {
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.getLine(response)) {
                std::cerr << "Disconnected from server." << std::endl;
                shouldTerminate = true;
                break;
            }
        }

        std::cout << "Server: " << response << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }

    std::string host = argv[1];
    short port = std::stoi(argv[2]);

    ConnectionHandler connectionHandler(host, port);
    if (!connectionHandler.connect()) {
        std::cerr << "Could not connect to server." << std::endl;
        return 1;
    }

    std::thread inputThread(handleUserInput, std::ref(connectionHandler));
    std::thread responseThread(handleServerResponses, std::ref(connectionHandler));

    inputThread.join();
    responseThread.join();

    return 0;
}
