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

Event::Event(const std::string &frame_body) : channel_name(""), city(""),
                                              name(""), date_time(0), description(""), general_information(),
                                              eventOwnerUser("")
{
    std::stringstream ss(frame_body);
    std::string line;
    std::string eventDescription;
    std::map<std::string, std::string> general_information_from_string;
    bool inGeneralInformation = false;

    while (std::getline(ss, line, '\n'))
    {
        if (line.empty())
        {
            continue; // Skip empty lines
        }

        if (line.find(':') != std::string::npos)
        {
            std::vector<std::string> lineArgs;
            split_str(line, ':', lineArgs);

            if (lineArgs.size() < 2)
            {
                std::cerr << "[DEBUG] Malformed line: " << line << std::endl;
                continue; // Skip malformed lines
            }

            std::string key = lineArgs[0];
            std::string val = lineArgs[1];

            // Handle key-value pairs
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
            else if (key == "General Information")
            {
                inGeneralInformation = true;
                continue;
            }
            else if (key == "description")
            {
                // Accumulate the description from subsequent lines
                eventDescription = val;
                while (std::getline(ss, line, '\n') && !line.empty())
                {
                    eventDescription += "\n" + line;
                }
                description = eventDescription;
            }

            if (inGeneralInformation)
            {
                general_information_from_string[key] = val;
            }
        }
    }

    general_information = general_information_from_string;
}

// Helper function to convert "DD/MM/YY HH:MM" to epoch time
int parseDateTimeToEpoch(const std::string &date_time)
{
    std::tm tm = {};
    std::istringstream ss(date_time);

    // Parse the date and time string into a tm structure
    ss >> std::get_time(&tm, "%d/%m/%y %H:%M");
    if (ss.fail())
    {
        throw std::runtime_error("[ERROR] Failed to parse date_time: " + date_time);
    }

    // Convert tm to epoch time
    return std::mktime(&tm);
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
        std::string date_time_str = event["date_time"];
        std::string description = event["description"];
        std::string user = event.contains("user") ? event["user"] : "";

        // Convert date_time string to epoch time
        int date_time = 0;
        try
        {
            date_time = parseDateTimeToEpoch(date_time_str);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            continue; // Skip this event if parsing fails
        }

        std::map<std::string, std::string> general_information;
        for (auto &update : event["general_information"].items())
        {
            std::string value;
            if (update.value().is_boolean())
            {
                // Normalize boolean to string
                value = update.value().get<bool>() ? "true" : "false";
            }
            else if (update.value().is_string())
            {
                value = update.value();
            }
            else
            {
                value = update.value().dump(); // Handle other types
            }
            general_information[update.key()] = value;
        }

        events.push_back(Event(channel_name, city, name, date_time, description, general_information, user));
    }

    names_and_events events_and_names{channel_name, events};
    return events_and_names;
}
