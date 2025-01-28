#include "../include/event.h"
#include "../include/json.hpp"
// #include "../include/keyboardInput.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>

using namespace std;
using json = nlohmann::json;

Event::Event(std::string channel_name, std::string city, std::string name, int date_time,
             std::string description, std::map<std::string, std::string> general_information, std::string user)
    : channel_name(channel_name), city(city), name(name),
      date_time(date_time), description(description),
      general_information(general_information), eventOwnerUser(user)
{
}

Event::~Event()
{
}

void Event::setEventOwnerUser(std::string setEventOwnerUser)
{
    eventOwnerUser = setEventOwnerUser;
}

const std::string &Event::getEventOwnerUser() const
{
    return eventOwnerUser;
}

const std::string &Event::get_channel_name() const
{
    return this->channel_name;
}

const std::string &Event::get_city() const
{
    return this->city;
}

const std::string &Event::get_name() const
{
    return this->name;
}

int Event::get_date_time() const
{
    return this->date_time;
}

const std::map<std::string, std::string> &Event::get_general_information() const
{
    return this->general_information;
}

const std::string &Event::get_description() const
{
    return this->description;
}

void split_str(const std::string &str, char delimiter, std::vector<std::string> &output)
{
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter))
    {
        output.push_back(item);
    }
}

static std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return ""; // If no non-whitespace found, return empty string
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

Event::Event(const std::string &frame_body) : channel_name(""), city(""),
                                              name(""), date_time(0), description(""), general_information(),
                                              eventOwnerUser("")
{
    std::stringstream ss(frame_body);
    std::string line;
    bool inGeneralInformation = false;

    while (std::getline(ss, line, '\n'))
    {
        if (line.empty())
        {
            continue; // Skip empty lines
        }

        // Trim the line to avoid extra whitespace
        line = trim(line);

        // Check if we are inside the "General Information" block
        if (inGeneralInformation)
        {
            if (line.find(':') != std::string::npos)
            {
                // Parse key-value pairs inside "General Information"
                std::vector<std::string> lineArgs;
                split_str(line, ':', lineArgs);

                if (lineArgs.size() >= 2)
                {
                    std::string key = trim(lineArgs[0]);
                    std::string val = trim(lineArgs[1]);

                    // Add to general_information map
                    general_information[key] = val;
                }
            }
            else
            {
                // If the line doesn't contain ":", assume we are exiting the block
                inGeneralInformation = false;
            }

            continue; // Skip further processing of this line
        }

        // Handle the "General Information:" line specifically
        if (line == "General Information:")
        {
            inGeneralInformation = true; // Start parsing the "General Information" block
            continue;
        }

        // Parse standard key-value pairs
        if (line.find(':') != std::string::npos)
        {
            std::vector<std::string> lineArgs;
            split_str(line, ':', lineArgs);

            // Ensure the line is valid
            if (lineArgs.size() < 2)
            {
                // Log and skip malformed lines
                std::cerr << "[DEBUG] Malformed line: " << line << std::endl;
                continue;
            }

            std::string key = trim(lineArgs[0]);
            std::string val = trim(lineArgs[1]);

            if (key == "user")
            {
                eventOwnerUser = val;
            }
            else if (key == "destination")
            {
                channel_name = val;
            }
            else if (key == "city")
            {
                city = val;
            }
            else if (key == "event name")
            {
                name = val;
            }
            else if (key == "date time")
            {
                try
                {
                    date_time = std::stoi(val);
                }
                catch (const std::invalid_argument &)
                {
                    std::cerr << "[DEBUG] Invalid date_time value: " << val << std::endl;
                }
            }
            else if (key == "description")
            {
                description = val; // Ensure description is stored separately
                continue;          // Skip further processing for this line
            }
        }
        else
        {
            // Log and skip malformed lines
            std::cerr << "[DEBUG] Skipping malformed line: " << line << std::endl;
        }
    }
}
names_and_events parseEventsFile(std::string json_path)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    std::string channel_name = data["channel_name"];

    // Run over all the events and convert them to Event objects
    std::vector<Event> events;
    for (auto &event : data["events"])
    {
        std::string name = event["event_name"];
        std::string city = event["city"];
        int date_time = event["date_time"];
        std::string description = event["description"];
        std::string user = event.contains("user") ? event["user"] : "";

        std::map<std::string, std::string> general_information;
        for (auto &update : event["general_information"].items())
        {
            std::string value;
            if (update.value().is_boolean())
            {
                value = update.value().get<bool>() ? "true" : "false";
            }
            else if (update.value().is_string())
            {
                value = update.value();
            }
            else
            {
                value = update.value().dump();
            }
            general_information[update.key()] = value;
        }

        events.push_back(Event(channel_name, city, name, date_time, description, general_information, user));
    }

    names_and_events events_and_names{channel_name, events};
    return events_and_names;
}
