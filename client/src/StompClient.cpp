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
#include <unordered_map>

// Global map to store the mapping of channel names to subscription IDs
std::unordered_map<std::string, int> channelSubscriptions;

// Global variable to manage subscription IDs
int nextSubscriptionId = 1;

void handleUserInput(ConnectionHandler &connectionHandler)
{
    while (!shouldTerminate)
    {
        std::string input;
        std::getline(std::cin, input);

        if (input.find("login") == 0)
        {
            std::stringstream ss(input);
            std::string command, hostPort, username, password;
            ss >> command >> hostPort >> username >> password;

            std::string frame = "CONNECT\naccept-version:1.2\nhost:" + hostPort +
                                "\nlogin:" + username + "\npasscode:" + password + "\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame))
            {
                std::cerr << "Failed to send login frame." << std::endl;
                break;
            }
        }
        else if (input.find("join") == 0)
        {
            std::stringstream ss(input);
            std::string command, channel;
            ss >> command >> channel;

            // Check if the channel is already subscribed
            if (channelSubscriptions.find(channel) != channelSubscriptions.end())
            {
                std::cerr << "Already subscribed to channel: " << channel << std::endl;
                continue;
            }

            // Generate a new subscription ID and add it to the map
            int subscriptionId = nextSubscriptionId++;
            channelSubscriptions[channel] = subscriptionId;

            std::string frame = "SUBSCRIBE\ndestination:/" + channel + "\nid:" +
                                std::to_string(subscriptionId) + "\nreceipt:" +
                                std::to_string(subscriptionId + 100) + "\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame))
            {
                std::cerr << "Failed to send join frame." << std::endl;
                break;
            }
        }
        else if (input.find("exit") == 0)
        {
            std::stringstream ss(input);
            std::string command, channel;
            ss >> command >> channel;

            // Find the subscription ID for the given channel
            auto it = channelSubscriptions.find(channel);
            if (it == channelSubscriptions.end())
            {
                std::cerr << "Channel not found: " << channel << std::endl;
                continue;
            }

            int subscriptionId = it->second;
            channelSubscriptions.erase(it); // Remove the channel from the map

            std::string frame = "UNSUBSCRIBE\nid:" + std::to_string(subscriptionId) +
                                "\nreceipt:" + std::to_string(subscriptionId + 200) + "\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame))
            {
                std::cerr << "Failed to send exit frame." << std::endl;
                break;
            }
        }
        else if (input.find("report") == 0)
        {
            std::stringstream ss(input);
            std::string command, filePath;
            ss >> command >> filePath;

            names_and_events parsedEvents = parseEventsFile(filePath);
            for (const Event &event : parsedEvents.events)
            {
                // Convert the map of general information to a string
                std::string generalInfoString;
                const std::map<std::string, std::string> &generalInfo = event.get_general_information();
                for (const auto &pair : generalInfo)
                {
                    generalInfoString += pair.first + ":" + pair.second + "\n";
                }

                // Create the SEND frame
                std::string frame = "SEND\ndestination:/" + parsedEvents.channel_name +
                                    "\nuser:" + event.getEventOwnerUser() +
                                    "\ncity:" + event.get_city() +
                                    "\nevent name:" + event.get_name() +
                                    "\ndate time:" + std::to_string(event.get_date_time()) +
                                    "\ngeneral information:\n" + generalInfoString +
                                    "\ndescription:" + event.get_description() + "\0";

                // Send the frame
                std::lock_guard<std::mutex> lock(connectionMutex);
                if (!connectionHandler.sendLine(frame))
                {
                    std::cerr << "Failed to send report frame." << std::endl;
                    break;
                }
            }
        }
        else if (input.find("summary") == 0)
        {
            std::stringstream ss(input);
            std::string command, channel, user, filePath;
            ss >> command >> channel >> user >> filePath;

            std::ofstream outFile(filePath);
            if (!outFile.is_open())
            {
                std::cerr << "Failed to open file: " << filePath << std::endl;
                continue;
            }

            outFile << "Channel: " << channel << "\nStats:\n";
            outFile.close();
        }
        else if (input == "logout")
        {
            std::string frame = "DISCONNECT\nreceipt:999\n\n\0";
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame))
            {
                std::cerr << "Failed to send logout frame." << std::endl;
            }
            shouldTerminate = true;
            break;
        }
        else
        {
            std::cerr << "Unknown command." << std::endl;
        }
    }
}

// Function to handle server responses
void handleServerResponses(ConnectionHandler &connectionHandler)
{
    while (!shouldTerminate)
    {
        std::string response;
        {
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.getLine(response))
            {
                std::cerr << "Disconnected from server." << std::endl;
                shouldTerminate = true;
                break;
            }
        }

        std::cout << "Server: " << response << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }

    std::string host = argv[1];
    short port = std::stoi(argv[2]);

    ConnectionHandler connectionHandler(host, port);
    if (!connectionHandler.connect())
    {
        std::cerr << "Could not connect to server." << std::endl;
        return 1;
    }

    std::thread inputThread(handleUserInput, std::ref(connectionHandler));
    std::thread responseThread(handleServerResponses, std::ref(connectionHandler));

    inputThread.join();
    responseThread.join();

    return 0;
}
