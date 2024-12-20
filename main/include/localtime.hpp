#ifndef LOCALTIME_HPP
#define LOCALTIME_HPP

#include <functional>
#include <ctime>
#include <string>
#include <esp_sntp.h>
#include <esp_log.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

class LocalTime {
public:
    LocalTime(const char* timezone, EventGroupHandle_t event_group, EventBits_t connected_bit);
    ~LocalTime() = default;

    void initializeSNTP();
    void getCurrentTimeInfo(struct tm& timeinfo);
    std::string obtainTime();
    int getFirstDayOfMonth();
    int getLastDayOfMonth();
    std::string getCurrentMonthYear();
    int getTodayDay();
    std::string getTodayDate();
    std::pair<std::string, std::string> getStartAndEndDates();
    std::string formatRangeToCustomDate(const std::string& start, const std::string& end, bool isAllDayEvent);
    int scheduleHibernationUntilMidnight30();
    
private:
    const char* timezone;
    EventGroupHandle_t event_group;
    EventBits_t connected_bit;
    static void timeSyncNotificationCb(struct timeval* tv);
    static LocalTime* instance;
    std::string preprocessTimestamp(const std::string& input);
    bool isDateOnly(const std::string& input);
};

#endif // LOCALTIME_HPP
