#include "../include/ConnectionHandler.h"
#include "../include/event.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <chrono>
#include <iomanip>

// Mutex for thread safety
std::mutex connectionMutex;
bool shouldTerminate = false;

// Data structure to store received messages for summary
std::unordered_map<std::string, std::map<std::string, std::vector<Event>>> receivedMessages;
static int nextsubscriptionId = 1;
std::unordered_map<std::string, int> channelSubscriptions;
std::string currUserName;

// Debugging helper function
void debugPrint(const std::string &msg)
{
    std::cout << "[DEBUG] " << msg << std::endl;
}

std::string epoch_to_date(long long epoch_time)
{
    // Convert epoch time to a time_point
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(epoch_time);

    // Convert to a time_t to work with localtime
    std::time_t time_t_epoch = std::chrono::system_clock::to_time_t(tp);

    // Convert to local time
    std::tm local_time;
    localtime_r(&time_t_epoch, &local_time);

    // Format the time into a string
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%d/%m/%y %H:%M");
    return oss.str();
}

// Function to handle user input
void handleUserInput(ConnectionHandler &connectionHandler)
{
    while (!shouldTerminate)
    {
        std::string input;
        std::getline(std::cin, input);
        debugPrint("Received user input: " + input);

        if (input.find("login") == 0)
        {
            std::stringstream ss(input);
            std::string command, hostPort, username, password;
            ss >> command >> hostPort >> username >> password;

            currUserName = username;
            std::string frame = "CONNECT\naccept-version:1.2\nhost:" + hostPort +
                                "\nlogin:" + username + "\npasscode:" + password + "\u0000";
            debugPrint("Sending login frame:\n" + frame);
            {
                // std::lock_guard<std::mutex> lock(connectionMutex); // Guard critical section
                if (!connectionHandler.sendLine(frame))
                {
                    std::cerr << "Failed to send login frame." << std::endl;
                    break;
                }
            }
        }
        else if (input.find("join") == 0)
        {
            std::stringstream ss(input);
            std::string command, channel;
            ss >> command >> channel;

            if (channelSubscriptions.find(channel) != channelSubscriptions.end())
            {
                std::cerr << "Already subscribed to channel: " << channel << std::endl;
                continue;
            }
            int subscriptionId = nextsubscriptionId++;
            channelSubscriptions[channel] = subscriptionId;

            std::string frame = "SUBSCRIBE\ndestination:/" + channel + "\nid:" +
                                std::to_string(subscriptionId) + "\nreceipt:" +
                                std::to_string(subscriptionId + 100) + "\n\n\u0000";
            debugPrint("Sending join frame:\n" + frame);
            // std::lock_guard<std::mutex> lock(connectionMutex);
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

            auto it = channelSubscriptions.find(channel);
            if (it == channelSubscriptions.end())
            {
                std::cerr << "Channel not found: " << channel << std::endl;
                continue;
            }
            int subscriptionId = it->second;
            channelSubscriptions.erase(it);

            std::string frame = "UNSUBSCRIBE\nid:" + std::to_string(subscriptionId) + "\nreceipt:" +
                                std::to_string(subscriptionId + 200) + "\n\n\u0000";
            debugPrint("Sending exit frame:\n" + frame);
            // std::lock_guard<std::mutex> lock(connectionMutex);
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

            debugPrint("Parsing events from file: " + filePath);
            names_and_events parsedEvents = parseEventsFile(filePath);
            for (const Event &event : parsedEvents.events)
            {
                std::string generalInfo;
                for (const auto &info : event.get_general_information())
                {
                    generalInfo += "  " + info.first + ": " + info.second + "\n";
                }

                std::string frame = "SEND\ndestination:/" + parsedEvents.channel_name +
                                    "\nuser:" + currUserName +
                                    "\ncity:" + event.get_city() +
                                    "\nevent name:" + event.get_name() +
                                    "\ndate time:" + std::to_string(event.get_date_time()) +
                                    "\nGeneral Information:\n" + generalInfo +
                                    "\ndescription:" + event.get_description() + "\n\n";

                debugPrint("Sending report frame:\n" + frame);
                // std::lock_guard<std::mutex> lock(connectionMutex);
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

            debugPrint("Generating summary for channel: " + channel + ", user: " + user);

            // Try to open or create the file
            std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
            if (!outFile.is_open())
            {
                std::cerr << "[ERROR] Failed to open or create file: " << filePath << std::endl;
                continue;
            }

            // Acquire lock for thread safety
            std::lock_guard<std::mutex> lock(connectionMutex);
            std::string normalizedChannel = channel[0] == '/' ? channel.substr(1) : channel; // Normalize channel
            if (receivedMessages.find(normalizedChannel) != receivedMessages.end() &&
                receivedMessages[normalizedChannel].find(user) != receivedMessages[normalizedChannel].end())
            {
                const auto &events = receivedMessages[normalizedChannel][user];

                // Count active events
                int activeEventCount = 0;
                for (const auto &event : events)
                {
                    auto activeIt = event.get_general_information().find("active");
                    if (activeIt != event.get_general_information().end())
                    {
                        // Check if the value is "true" or matches true if parsed as a boolean
                        if (activeIt->second == "true")
                        {
                            ++activeEventCount;
                        }
                    }
                }

                // Write to the file
                std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
                if (!outFile.is_open())
                {
                    std::cerr << "[ERROR] Failed to open or create file: " << filePath << std::endl;
                    return;
                }

                outFile << "{\n";
                outFile << "  \"channel\": \"" << normalizedChannel << "\",\n";
                outFile << "  \"stats\": {\n";
                outFile << "    \"total\": " << events.size() << ",\n";
                outFile << "    \"active\": " << activeEventCount << "\n";
                outFile << "  },\n";
                outFile << "  \"event_reports\": [\n";
                for (size_t i = 0; i < events.size(); ++i)
                {
                    const Event &event = events[i];
                    outFile << "    {\n";
                    outFile << "      \"report_id\": " << i + 1 << ",\n";
                    outFile << "      \"city\": \"" << event.get_city() << "\",\n";
                    outFile << "      \"date_time\": " << epoch_to_date(event.get_date_time()) << ",\n";
                    outFile << "      \"event_name\": \"" << event.get_name() << "\",\n";
                    outFile << "      \"summary\": \"" << event.get_description().substr(0, 27) << "...\"\n";
                    outFile << "    }";
                    if (i < events.size() - 1)
                        outFile << ",";
                    outFile << "\n";
                }

                outFile << "  ]\n";
                outFile << "}\n";

                debugPrint("Summary written to file: " + filePath);
                outFile.close();
            }
            else
            {
                std::cerr << "[ERROR] No events found for channel: " << channel << ", user: " << user << std::endl;
            }

            outFile.close();
        }
        else if (input == "logout")
        {
            std::string frame = "DISCONNECT\nreceipt:-1\n\n\u0000";
            debugPrint("Sending logout frame:\n" + frame);
            // std::lock_guard<std::mutex> lock(connectionMutex);
            if (!connectionHandler.sendLine(frame))
            {
                std::cerr << "Failed to send logout frame." << std::endl;
            }
            break;
        }
        else
        {
            std::cerr << "Unknown command." << std::endl;
        }
    }
}

std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return ""; // Empty string if only whitespace
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Function to handle server responses
void handleServerResponses(ConnectionHandler &connectionHandler)
{
    while (!shouldTerminate)
    {
        std::string response;
        {
            // Attempt to read a line from the server
            if (!connectionHandler.getLine(response))
            {
                std::cerr << "Disconnected from server." << std::endl;
                shouldTerminate = true;
                break;
            }
        }

        // Trim and sanitize the response
        // response = trim(response); // Ensure proper trimming of whitespace, newlines, and null characters
        debugPrint("Received server's response: [" + response + "]");

        if (response.empty())
        {
            debugPrint("Ignored empty response from server.");
            continue;
        }

        // Handle different frame types
        if (response.find("ERROR") == 0)
        {
            std::cerr << "Server Error: " << response << std::endl;
            shouldTerminate = true;
            connectionHandler.close();
            break;
        }
        else if (response.find("RECEIPT") == 0)
        {
            size_t receiptPos = response.find("receipt-id:");
            if (receiptPos != std::string::npos)
            {
                std::string receiptId = response.substr(receiptPos + 11); // Extract receipt-id value
                receiptId = receiptId.substr(0, receiptId.find("\n"));    // Trim further

                if (receiptId == "-1")
                {
                    // Receipt for DISCONNECT command
                    std::cout << "Logout successful." << std::endl;
                    shouldTerminate = true;
                    connectionHandler.close();
                    break;
                }
                else
                {
                    std::cout << "Received receipt-id: " << receiptId << std::endl;
                }
            }
        }
        else if (response.find("MESSAGE") == 0)
        {
            try
            {
                // Parse the MESSAGE frame into an Event
                Event event(response);
                const std::string &channel = event.get_channel_name();
                const std::string &normalizedChannel = channel[0] == '/' ? channel.substr(1) : channel; // Normalize channel
                const std::string &user = event.getEventOwnerUser();
                std::cerr << "channel: " << channel << "NormalizedChannel:" << normalizedChannel;

                // Update the received messages map
                {
                    std::lock_guard<std::mutex> lock(connectionMutex);
                    receivedMessages[normalizedChannel][user].push_back(event);
                }

                debugPrint("Updated receivedMessages:");
                for (const auto &channelPair : receivedMessages)
                {
                    std::cerr << "[DEBUG] Channel: " << channelPair.first << std::endl;
                    for (const auto &userPair : channelPair.second)
                    {
                        std::cerr << "[DEBUG]   User: " << userPair.first << ", Events Count: " << userPair.second.size() << std::endl;
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to parse MESSAGE frame: " << e.what() << std::endl;
                debugPrint("Malformed MESSAGE frame: " + response);
            }
        }
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
