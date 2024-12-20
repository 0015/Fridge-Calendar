#ifndef WIFI_HPP
#define WIFI_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <string>

class WiFi {
public:
    WiFi(const std::string& ssid, const std::string& password, EventGroupHandle_t event_group, EventBits_t connected_bit);
    ~WiFi();

    void init();
    void start();

private:
    std::string ssid;
    std::string password;
    EventGroupHandle_t event_group;
    EventBits_t connected_bit;

    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

#endif // WIFI_HPP
