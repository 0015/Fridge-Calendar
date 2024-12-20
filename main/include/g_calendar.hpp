#ifndef G_CALENDAR_HPP
#define G_CALENDAR_HPP

#include <string>
#include <vector>
#include "esp_err.h"

// Structure to hold calendar event details
struct CalendarEvent {
    std::string summary;
    std::string description;
    std::string creatorEmail;        // To store the creator's email
    std::string organizerDisplayName; // To store the organizer's display name
    std::string start;               // Can store either date or dateTime
    std::string end;                 // Can store either date or dateTime
    bool isAllDayEvent;              // Flag to indicate if it's an all-day event
};

// Class to manage Google Calendar API
class GoogleCalendar {
public:
    GoogleCalendar(const std::string& clientId, const std::string& clientSecret, const std::string& refreshToken);

    // Refreshes the access token using the refresh token
    std::string refreshAccessToken();

    // Fetches events for the current month
    esp_err_t getEvents(const std::string& accessToken, const std::string& calendarId, std::vector<CalendarEvent>& events);

private:
    std::string clientId;
    std::string clientSecret;
    std::string refreshToken;

    // Parses the JSON response and extracts events
    void parseEvents(const std::string& payload, std::vector<CalendarEvent>& eventsVector);

    // Creates a time range for this month's events
    std::string createTimeRange();
};

#endif // GCALENDAR_HPP
