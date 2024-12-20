#include <esp_log.h>
#include "application.hpp"
#include "app_config.hpp"
#include "g_calendar_config.hpp"
#include <algorithm>
#include <set>
#include <esp_sleep.h>

#define WIFI_CONNECTED_BIT BIT0
#define LOCALTIME_SET_BIT BIT1

#define ACCESS_TOKEN_KEY "access_token"
#define FIRST_RUN_KEY    "first_run"
#define RETRY_KEY        "retry_count"

static const char* TAG = "[App]";

Application::Application()
    : event_group(xEventGroupCreate()),                             // Create the event group
      wifi(WIFI_SSID, WIFI_PASS, event_group, WIFI_CONNECTED_BIT),  // Initialize WiFi
      epaper(),                                                     // Default constructor for EPaper
        localTime(TimeZone,event_group, LOCALTIME_SET_BIT)          // Initialize LocalTime with TimeZone
{
    // Any additional setup needed for members can go here
}

Application::~Application() {
    // Destructor
}

void Application::run() {
    ESP_LOGI(TAG, "Applcation Run");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated, erase and retry
        ESP_LOGW(TAG, "NVS Partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_LOGI(TAG, "NVS Initialized successfully.");

    bool isFirstRun = true;
    std::string nvsState = getDataFromNVS(FIRST_RUN_KEY);
    if (nvsState == "updated") {
        isFirstRun = false;
        ESP_LOGI(TAG, "Skipping splash screen and progress bar on reboot.");
    }

    std::string retryValue = getDataFromNVS(RETRY_KEY);
    int retryCount = retryValue.empty() ? 0 : std::stoi(retryValue);

    epaper.initialize();
    int epaper_x_center = epaper.getWidth() / 2;
    int epaper_y_center = epaper.getHeight() / 2;
    int bar_x = epaper_x_center - 200;
    int bar_y = epaper_y_center + 100;

   if (isFirstRun) {
        // Show splash screen and progress bar
        epaper.splash();

        epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Center, epaper_x_center, epaper_y_center + 60, "System Loading");
        epaper.drawProgressBar(bar_x, bar_y, 30);
        epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Left, epaper_x_center - 200, epaper_y_center + 150, "[OK] E-Paper Display");

        wifi.init();
        wifi.start();

        ESP_LOGI(TAG, "Waiting for WiFi connection...");
        xEventGroupWaitBits(event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "WiFi Connected!");

        epaper.drawProgressBar(bar_x, bar_y, 60);
        epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Left, epaper_x_center - 200, epaper_y_center + 180, "[OK] WIFI Connected");
    } else {
        // Skip splash and directly initialize Wi-Fi
        wifi.init();
        wifi.start();
        xEventGroupWaitBits(event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    }

    std::string currentDateTime = localTime.obtainTime();
    xEventGroupWaitBits(event_group, LOCALTIME_SET_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    
    if (isFirstRun) {
        epaper.drawProgressBar(bar_x, bar_y, 80);
        epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Left, epaper_x_center - 200, epaper_y_center + 210, currentDateTime.c_str());
    }

    //Need to complete Screen drawing first
    std::vector<CalendarEvent> events;
    if(fetchCalendarEvents(events) == ESP_OK){
        if (isFirstRun) {
            epaper.drawProgressBar(bar_x, bar_y, 100);
            epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Left, epaper_x_center - 200, epaper_y_center + 240, "[OK] Fectching Calendar Events");

            // Mark the state in NVS
            storeDataInNVS(FIRST_RUN_KEY, "updated");
        }

        // Reset retry counter on success   
        storeDataInNVS(RETRY_KEY, "0");

        std::reverse( events.begin(), events.end() );

        auto [startDate, endDate] = localTime.getStartAndEndDates();
        ESP_LOGI(TAG, "Start Date: %s", startDate.c_str());
        ESP_LOGI(TAG, "End Date: %s", endDate.c_str());

        // Need to make another function   
        int offset_pos = localTime.getFirstDayOfMonth();
        int max_date = localTime.getLastDayOfMonth();
        ESP_LOGI(TAG, "offset_pos: %d, max_date: %d", offset_pos, max_date);

        epaper.drawCalendarBase(offset_pos, max_date, localTime.getCurrentMonthYear().c_str(), localTime.getTodayDay());

        printEventsInRange(events, startDate, endDate);

        int epaper_y2 = epaper.getHeight() - (epaper.getHeight() / 3);
        epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Center, epaper_x_center, epaper_y2, "Upcoming Events");
        epaper.drawBar(20, epaper_y2 + 10, epaper.getWidth() - 40, 2);
        epaper.drawBar(epaper.getWidth()/2 - 1, epaper_y2 + 10, 2, epaper.getHeight() / 3 - 30);
        epaper.invalidate();

        // Sort events by their start date
        sortEventsByStartDate(events);
        printEventSummary(events, localTime.getTodayDate().c_str());

    }else{
        epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Left, epaper_x_center - 200, epaper_y_center + 240, "[Fail] Fectching Calendar Events");
        epaper.drawText(epaper.font_mid, EPaper::TEXT_ALIGN::Left, epaper_x_center - 200, epaper_y_center + 270, "Check the AccessToken and RefreshToken");

        storeDataInNVS(FIRST_RUN_KEY, "");
        storeDataInNVS(RETRY_KEY, std::to_string(++retryCount)); // Increment retry counter and reboot

        if(retryCount >= 2){
            esp_deep_sleep_start();
        }else{
            esp_restart();
        }
        
    }

    epaper.drawText(epaper.font_tiny, EPaper::TEXT_ALIGN::Right, epaper.getWidth() - 20 , epaper.getHeight() - 8, ("Updated: " + currentDateTime).c_str());

    esp_sleep_enable_timer_wakeup(localTime.scheduleHibernationUntilMidnight30() * 1000000ULL);
    esp_deep_sleep_start(); 
}

// Function to increment date by one day
std::string Application::incrementDate(const std::string& date) {
    // Parse the input date (YYYY-MM-DD) using sscanf
    struct tm timeStruct = {};
    if (sscanf(date.c_str(), "%4d-%2d-%2d", &timeStruct.tm_year, &timeStruct.tm_mon, &timeStruct.tm_mday) != 3) {
         ESP_LOGE("Main", "Failed to parse date: %s", date.c_str());
    }

    // Adjust the tm_year and tm_mon since sscanf uses 0-based months and years
    timeStruct.tm_year -= 1900;  // Years since 1900
    timeStruct.tm_mon -= 1;      // Months are 0-11, so subtract 1

    // Add one day (86400 seconds) to the time
    timeStruct.tm_mday += 1;

    // Normalize the time structure
    if (mktime(&timeStruct) == -1) {
         ESP_LOGE("Main", "Failed to increment date: %s", date.c_str());
    }

    // Convert the updated tm structure back to a string (YYYY-MM-DD) using strftime
    char newDate[11];  // Buffer for the new date string in the format YYYY-MM-DD
    if (strftime(newDate, sizeof(newDate), "%Y-%m-%d", &timeStruct) == 0) {
        ESP_LOGE("Main", "Failed to format incremented date.");
    }

    return std::string(newDate);
}

bool Application::isDateWithinRange(const std::string& date, const CalendarEvent& event) {
      if (event.isAllDayEvent) {
            // For all-day events, exclude the end date
            return date >= event.start.substr(0, 10) && date < event.end.substr(0, 10);
      } else {
            // For non-all-day events, include the end date
            return date >= event.start.substr(0, 10) && date <= event.end.substr(0, 10);
      }
}

void Application::printEventsInRange(const std::vector<CalendarEvent>& events, const std::string& start, const std::string& end) {
      std::string currentDate = start;
      while (currentDate <= end) {
            // Extract the day number from the current date
            int day = std::stoi(currentDate.substr(8, 2)); // Extract "DD" and convert to integer
            ESP_LOGI(TAG, "Event on %s (Day %d):", currentDate.c_str(), day);

            EPaper::Coordinates coords = epaper.getCoordinatesForDay(day);
            if (coords.x != -1 && coords.y != -1) {
                printf("Coordinates for day %d: x = %d, y = %d\n", day, coords.x, coords.y);
            } else {
                printf("Invalid day: %d\n", day);
            }
            //ESP_LOGI(TAG, "Event on %s:", currentDate.c_str());
            bool found = false;
            int slot = 1;

            for (const auto& event : events) {
                  if (isDateWithinRange(currentDate, event)) {
                        found = true;
                        epaper.drawTextInSlot(slot++, coords.x, coords.y, event.organizerDisplayName.substr(0, 4).c_str());
                        //std::cout << " - " << event.summary << " (Start: " << event.start << ", End: " << event.end << ")\n";
                        ESP_LOGI(TAG, "Event: %s", event.summary.c_str());
                        ESP_LOGI(TAG, "Description: %s", event.description.c_str());
                        ESP_LOGI(TAG, "Start: %s", event.start.c_str());
                        ESP_LOGI(TAG, "End: %s\n\n", event.end.c_str());
                  }
            }
            

            if (!found) {
                  ESP_LOGI(TAG, "No events\n");
            }else{
                epaper.invalidate();
            }
            currentDate = incrementDate(currentDate);
      }
}

// Bubble sort for CalendarEvent objects
void Application::sortEventsByStartDate(std::vector<CalendarEvent>& events) {
    for (size_t i = 0; i < events.size(); ++i) {
        for (size_t j = 0; j < events.size() - i - 1; ++j) {
            if (events[j].start > events[j + 1].start) {
                // Swap elements if they are out of order
                CalendarEvent temp = events[j];
                events[j] = events[j + 1];
                events[j + 1] = temp;
            }
        }
    }
}

std::vector<std::string> Application::truncateString(const std::string& str, size_t maxLength = 54, size_t maxLines = 4) {
    std::vector<std::string> lines;
    std::string currentLine;
    size_t i = 0;

    while (i < str.size() && lines.size() < maxLines) {
        // Check for line breaks (\n or \r\n)
        if (str[i] == '\n') {
            // Add the current line to the result and start a new line
            lines.push_back(currentLine);
            currentLine.clear();
            i++;
        } else if (str[i] == '\r' && i + 1 < str.size() && str[i + 1] == '\n') {
            // Handle Windows-style line breaks (\r\n)
            lines.push_back(currentLine);
            currentLine.clear();
            i += 2;
        } else {
            // Add the character to the current line
            currentLine += str[i];
            i++;

            // If the current line exceeds maxLength, split it
            if (currentLine.size() >= maxLength) {
                lines.push_back(currentLine);
                currentLine.clear();

                // Stop if we've reached the max number of lines
                if (lines.size() >= maxLines) {
                    break;
                }
            }
        }
    }

    // Add any remaining content in the current line (if under maxLines)
    if (!currentLine.empty() && lines.size() < maxLines) {
        lines.push_back(currentLine);
    }

    return lines;
}


void Application::printEventSummary(const std::vector<CalendarEvent>& events, const std::string& startDate) {
    std::set<std::string> printedEvents; // Set to track processed events
    const int maxEventsToDisplay = 6;    // Limit to 6 events
    int eventCount = 0;                  // Counter to track the number of events printed
    int epaper_x2 = 20;
    int epaper_y2 = epaper.getHeight() - (epaper.getHeight() / 3) + 40;

    ESP_LOGI(TAG, "Event Summary (After %s):", startDate.c_str());
    for (const auto& event : events) {
        // Stop processing if we've reached the max number of events
        if (eventCount >= maxEventsToDisplay) {
            break;
        }

        // Check if the event starts on or after the given startDate
        if (event.start < startDate) {
            continue; // Skip events that start before the given date
        }

        // Use a unique identifier for the event (e.g., start + end date)
        std::string uniqueID = event.start + "-" + event.end;

        // Skip if this event has already been processed
        if (printedEvents.find(uniqueID) != printedEvents.end()) {
            continue;
        }

        // Add the event's unique ID to the set
        printedEvents.insert(uniqueID);
        
        // Print the event summary
        ESP_LOGI(TAG, "organizerDisplayName: %s", event.organizerDisplayName.c_str());
        ESP_LOGI(TAG, "Event: %s", event.summary.c_str());
        ESP_LOGI(TAG, "Description: %s", event.description.c_str());
        ESP_LOGI(TAG, "Start: %s", event.start.c_str());
        ESP_LOGI(TAG, "End: %s\n", event.end.c_str());
        ESP_LOGI(TAG, "isAllEvent: %d\n", event.isAllDayEvent);

        epaper.drawText(epaper.font_sml, EPaper::TEXT_ALIGN::Left, epaper_x2, epaper_y2, (event.organizerDisplayName + " - " +event.summary).c_str());
        epaper_y2 += 20;
        epaper.drawText(epaper.font_tiny, EPaper::TEXT_ALIGN::Left, epaper_x2, epaper_y2, (localTime.formatRangeToCustomDate(event.start, event.end, event.isAllDayEvent)).c_str());
        epaper_y2 += 18;

        if(!event.description.empty()){

            // Get the truncated description lines
            std::vector<std::string> descriptionLines = truncateString(event.description);
              // Print each line of the description
            for (const auto& line : descriptionLines) {
                ESP_LOGI(TAG, "Description: %s", line.c_str());
                epaper.drawText(epaper.font_tiny, EPaper::TEXT_ALIGN::Left, epaper_x2, epaper_y2, line.c_str());
                epaper_y2 += 16;
            }
        }
        
        epaper_y2 += 20;


        ESP_LOGI(TAG, "epaper_y2: %d\n", epaper_y2);

        if(eventCount == 2){
            epaper_x2 = epaper.getWidth()/2 + 20;
            epaper_y2 = epaper.getHeight() - (epaper.getHeight() / 3) + 40;
        }

          // Increment the event counter
        ++eventCount;
    }

    epaper.invalidate();
}

void Application::storeDataInNVS(const std::string& key, const std::string& data) {
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (err == ESP_OK) {
        err = nvs_set_str(nvsHandle, key.c_str(), data.c_str());
        if (err == ESP_OK) {
            nvs_commit(nvsHandle);
            ESP_LOGI("NVS", "Access token stored successfully under key '%s'.", key.c_str());
        } else {
            ESP_LOGE("NVS", "Failed to store access data under key '%s': %s", key.c_str(), esp_err_to_name(err));
        }
        nvs_close(nvsHandle);
    } else {
        ESP_LOGE("NVS", "Failed to open NVS: %s", esp_err_to_name(err));
    }
}

std::string Application::getDataFromNVS(const std::string& key) {
    nvs_handle_t nvsHandle;
    char dataBuffer[256]; // Adjust size as needed
    size_t dataSize = sizeof(dataBuffer);

    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvsHandle);
    if (err == ESP_OK) {
        err = nvs_get_str(nvsHandle, key.c_str(), dataBuffer, &dataSize);
        if (err == ESP_OK) {
            ESP_LOGI("NVS", "Access token retrieved from NVS using key '%s'.", key.c_str());
            nvs_close(nvsHandle);
            return std::string(dataBuffer);
        } else {
            ESP_LOGW("NVS", "Access token not found in NVS under key '%s': %s", key.c_str(), esp_err_to_name(err));
        }
        nvs_close(nvsHandle);
    } else {
        ESP_LOGE("NVS", "Failed to open NVS: %s", esp_err_to_name(err));
    }
    return "";
}

esp_err_t Application::fetchCalendarEvents(std::vector<CalendarEvent>& events) {
    esp_err_t ret = ESP_OK;

    GoogleCalendar gCalendar(
        CalendarConfig::getClientId(),
        CalendarConfig::getClientSecret(),
        CalendarConfig::getRefreshToken()
    );

    const std::vector<std::string>& calendarIds = _CALENDAR_IDS;
    
    // Attempt to fetch events with the existing access token
    std::string currentAccessToken = getDataFromNVS(ACCESS_TOKEN_KEY);
    if (currentAccessToken.empty()) {
        currentAccessToken = CalendarConfig::getAccessToken();
    }

    ESP_LOGI(TAG, "currentAccessToken: %s", currentAccessToken.c_str());

    for (const auto& calendarId : calendarIds) {
        ESP_LOGI(TAG, "Fetching events for calendar ID: %s", calendarId.c_str());

        ret = gCalendar.getEvents(currentAccessToken, calendarId, events);

        if (ret != ESP_OK && events.empty() ) {
            ESP_LOGW(TAG, "No events or token might be invalid. Refreshing access token...");

            // Refresh the access token
            std::string newAccessToken = gCalendar.refreshAccessToken();
            if (!newAccessToken.empty()) {
                ESP_LOGI(TAG, "Access token refreshed: %s", newAccessToken.c_str());

                // Store the new access token in NVS
                storeDataInNVS(ACCESS_TOKEN_KEY, newAccessToken);

                // Retry fetching events with the new token
                currentAccessToken = newAccessToken;
                ret = gCalendar.getEvents(currentAccessToken, calendarId, events);

                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "Events retrieved successfully for calendar ID: %s", calendarId.c_str());
                } else {
                    ret = ESP_ERR_INVALID_RESPONSE;    
                    ESP_LOGE(TAG, "Failed to retrieve events even after refreshing token for calendar ID: %s", calendarId.c_str());
                }
            } else {
                ret = ESP_ERR_NOT_ALLOWED;
                ESP_LOGE(TAG, "Failed to refresh access token. Cannot fetch events for calendar ID: %s", calendarId.c_str());
                break; // Exit loop as token refresh failed
            }
        } else {
            ESP_LOGI(TAG, "Events retrieved successfully for calendar ID: %s", calendarId.c_str());
        }
    }

    return ret; 
}