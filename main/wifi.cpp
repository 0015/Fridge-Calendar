#include "wifi.hpp"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "[WiFi]";

WiFi::WiFi(const std::string& ssid, const std::string& password, EventGroupHandle_t event_group, EventBits_t connected_bit)
    : ssid(ssid), password(password), event_group(event_group), connected_bit(connected_bit) {}

WiFi::~WiFi() {
    esp_wifi_stop();
    esp_wifi_deinit();
}

void WiFi::init() {
    ESP_LOGI(TAG, "Initializing WiFi...");

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::event_handler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi::event_handler, this, nullptr));
}

void WiFi::start() {
    ESP_LOGI(TAG, "Starting WiFi...");

    // Configure WiFi
    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
    strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password.c_str(), sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization complete.");
}

void WiFi::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto* wifi = static_cast<WiFi*>(arg);

    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "Connecting to WiFi...");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "Disconnected from WiFi. Reconnecting...");
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Got IP address!");
        xEventGroupSetBits(wifi->event_group, wifi->connected_bit);
    }
}
