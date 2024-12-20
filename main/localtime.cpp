#include "localtime.hpp"

static const char* TAG = "[LocalTime]";

LocalTime* LocalTime::instance = nullptr;

LocalTime::LocalTime(const char* timezone, EventGroupHandle_t event_group, EventBits_t connected_bit)
    :timezone(timezone), event_group(event_group), connected_bit(connected_bit){
    instance = this;
}

void LocalTime::timeSyncNotificationCb(struct timeval* tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    if(instance)
        xEventGroupSetBits(instance->event_group, instance->connected_bit);
}

void LocalTime::initializeSNTP() {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(timeSyncNotificationCb);
    esp_sntp_init();
}

void LocalTime::getCurrentTimeInfo(struct tm& timeinfo) {
    time_t now = 0;
    time(&now);
    localtime_r(&now, &timeinfo);
}

std::string LocalTime::obtainTime() {
    initializeSNTP();

    int retry = 0;
    const int retry_count = 10;

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    setenv("TZ", timezone, 1);
    tzset();

    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in your local time zone is: %s", strftime_buf);

    // Return the formatted date/time as a string
    return std::string(strftime_buf);
}

int LocalTime::getFirstDayOfMonth() {
    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);

    // Adjust to the 1st day of the month
    timeinfo.tm_mday = 1; // Set day to 1
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;

    // Normalize the timeinfo structure
    mktime(&timeinfo);

    ESP_LOGI(TAG, "The offset for the 1st day of the month: %d", timeinfo.tm_wday);
    return timeinfo.tm_wday; // Return day of the week
}

int LocalTime::getLastDayOfMonth() {
    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);

    // Move to the next month
    timeinfo.tm_mday = 1; // Set day to 1
    timeinfo.tm_mon++;       // Increment month by 1

    // Handle December (month 11, as months are 0-indexed)
    if (timeinfo.tm_mon > 11) {
        timeinfo.tm_mon = 0; // January
        timeinfo.tm_year++; // Increment year
    }

    // Convert back to time_t and subtract one day to get the last day of the current month
    time_t nextMonth = mktime(&timeinfo);
    nextMonth -= 86400; // Subtract 24 hours in seconds

    struct tm lastDay;
    localtime_r(&nextMonth, &lastDay);

    return lastDay.tm_mday;
}

std::string LocalTime::getCurrentMonthYear() {
    // Get the current time
    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);

    // Format the month and year
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%B %Y", &timeinfo); // "%B" for full month name, "%Y" for year

    // Return the formatted string
    return std::string(buffer);
}

int LocalTime::getTodayDay() {
    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);
    return timeinfo.tm_mday;
}

std::string LocalTime::getTodayDate() {
    // Get the current time
    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);

    // Format the date as "YYYY-MM-DD"
    char buffer[11]; // Enough space for "YYYY-MM-DD\0"
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);

    // Return the formatted date as a string
    return std::string(buffer);
}

std::pair<std::string, std::string> LocalTime::getStartAndEndDates() {
    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);

    // Calculate start date (1st day of the month)
    timeinfo.tm_mday = 1; // Set day to 1
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    mktime(&timeinfo); // Normalize timeinfo

    char startDate[11]; // Format YYYY-MM-DD + null terminator
    strftime(startDate, sizeof(startDate), "%Y-%m-%d", &timeinfo);

    // Calculate end date (last day of the month)
    timeinfo.tm_mon++; // Move to the next month
    if (timeinfo.tm_mon > 11) { // Handle December to January transition
        timeinfo.tm_mon = 0;
        timeinfo.tm_year++;
    }
    mktime(&timeinfo); // Normalize timeinfo
    timeinfo.tm_mday--; // Go back one day to get the last day of the current month
    mktime(&timeinfo); // Normalize timeinfo again

    char endDate[11];
    strftime(endDate, sizeof(endDate), "%Y-%m-%d", &timeinfo);

    // Return the start and end dates as strings
    return {std::string(startDate), std::string(endDate)};
}


std::string LocalTime::preprocessTimestamp(const std::string& input) {
    std::string result = input;
    // Remove the colon in the timezone offset
    size_t pos = result.find_last_of(':');
    if (pos != std::string::npos && pos > result.find('T')) {
        result.erase(pos, 1); // Remove the colon
    }
    return result;
}

bool LocalTime::isDateOnly(const std::string& input) {
    // Check if the string contains a 'T' (indicates full timestamp)
    return input.find('T') == std::string::npos;
}

std::string LocalTime::formatRangeToCustomDate(const std::string& start, const std::string& end, bool isAllDayEvent) {
    struct tm startTimeinfo = {}, endTimeinfo = {};
    char formattedStart[64], formattedEnd[64];

    // Helper to preprocess the timezone format
    auto preprocessTimestamp = [](const std::string& timestamp) -> std::string {
        std::string result = timestamp;
        size_t pos = result.find_last_of(':');
        if (pos != std::string::npos && pos > result.size() - 6) {
            // Convert -08:00 to -0800
            result.erase(pos, 1);
        }
        return result;
    };

    // Preprocess timestamps to handle timezone offsets
    std::string processedStart = preprocessTimestamp(start);
    std::string processedEnd = preprocessTimestamp(end);

    // Parse start time
    if (processedStart.find('T') == std::string::npos) {  // Date only
        if (strptime(processedStart.c_str(), "%Y-%m-%d", &startTimeinfo) == nullptr) {
            return "Invalid start date format";
        }
    } else {  // Full timestamp
        if (strptime(processedStart.c_str(), "%Y-%m-%dT%H:%M:%S", &startTimeinfo) == nullptr) {
            return "Invalid start timestamp format";
        }
    }

    // Parse end time
    if (processedEnd.find('T') == std::string::npos) {  // Date only
        if (strptime(processedEnd.c_str(), "%Y-%m-%d", &endTimeinfo) == nullptr) {
            return "Invalid end date format";
        }
    } else {  // Full timestamp
        if (strptime(processedEnd.c_str(), "%Y-%m-%dT%H:%M:%S", &endTimeinfo) == nullptr) {
            return "Invalid end timestamp format";
        }
    }

    // Adjust for all-day events
    if (isAllDayEvent) {
        time_t endTime = mktime(&endTimeinfo) - 86400;  // Subtract 24 hours
        localtime_r(&endTime, &endTimeinfo);
        if (startTimeinfo.tm_year == endTimeinfo.tm_year &&
            startTimeinfo.tm_mon == endTimeinfo.tm_mon &&
            startTimeinfo.tm_mday == endTimeinfo.tm_mday) {
            endTimeinfo = startTimeinfo;  // One-day event
        }
        strftime(formattedStart, sizeof(formattedStart), "%b %d, %Y", &startTimeinfo);
        strftime(formattedEnd, sizeof(formattedEnd), "%b %d, %Y", &endTimeinfo);

        return std::string(formattedStart) + " to " + std::string(formattedEnd);
    } else {
        // Format for non-all-day events
        strftime(formattedStart, sizeof(formattedStart), "%b %d, %Y %I:%M %p", &startTimeinfo);
        strftime(formattedEnd, sizeof(formattedEnd), "%b %d, %Y %I:%M %p", &endTimeinfo);

        return std::string(formattedStart) + " to " + std::string(formattedEnd);
    }
}

int LocalTime::scheduleHibernationUntilMidnight30(){
    struct tm timeinfo = {};
    getCurrentTimeInfo(timeinfo);

    // Calculate seconds until midnight
    int secondsUntilMidnight30 = (23 - timeinfo.tm_hour) * 3600 
                             + (59 - timeinfo.tm_min) * 60 
                             + (60 - timeinfo.tm_sec) 
                             + 1800; // Add 30 minutes (1800 seconds)
    ESP_LOGI(TAG, "System will hibernate for %d seconds.", secondsUntilMidnight30);
    
    return secondsUntilMidnight30;
}