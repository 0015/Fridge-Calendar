#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "g_calendar.hpp"
#include "epaper.hpp"
#include "wifi.hpp"
#include "localtime.hpp"

class Application {
public:
    Application();
    ~Application();
    void run(); // Main application logic

private:
    EventGroupHandle_t event_group;
    WiFi wifi;
    EPaper epaper;
    LocalTime localTime;

    std::string incrementDate(const std::string& date);
    bool isDateWithinRange(const std::string& date, const CalendarEvent& event);
    void printEventsInRange(const std::vector<CalendarEvent>& events, const std::string& start, const std::string& end);
    void printEventSummary(const std::vector<CalendarEvent>& events, const std::string& startDate);
    void storeDataInNVS(const std::string& key, const std::string& data);
    std::string getDataFromNVS(const std::string& key);
    esp_err_t fetchCalendarEvents(std::vector<CalendarEvent>& events);
    void sortEventsByStartDate(std::vector<CalendarEvent>& events);
    std::vector<std::string> truncateString(const std::string& str,  size_t maxLength, size_t maxLines);
};
