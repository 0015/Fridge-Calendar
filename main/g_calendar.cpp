#include "g_calendar.hpp"
#include "g_calendar_config.hpp"
#include <esp_http_client.h>
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>
#include "esp_log.h"
#include "app_config.hpp"

static const char* TAG = "[Google Calendar]";

extern const char server_googleapis_root_cert_pem_start[] asm("_binary_server_googleapis_root_cert_pem_start");
extern const char server_googleapis_root_cert_pem_end[] asm("_binary_server_googleapis_root_cert_pem_end");

using json = nlohmann::json;

GoogleCalendar::GoogleCalendar(const std::string& clientId, const std::string& clientSecret, const std::string& refreshToken)
    : clientId(clientId), clientSecret(clientSecret), refreshToken(refreshToken) {}

static esp_err_t _http_event_handler(esp_http_client_event_t* evt) {
      static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }

            if (evt->user_data) {
                char* buffer = (char*)evt->user_data;
                if (output_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                    // Append data to the user-provided buffer
                    memcpy(buffer + output_len, evt->data, evt->data_len);
                } else {
                    ESP_LOGE(TAG, "Buffer overflow, data truncated!");
                }
            }

              output_len += evt->data_len;
            break;
        case HTTP_EVENT_ON_HEADER:
            if (strcasecmp(evt->header_key, "Content-Length") == 0) {
                ESP_LOGI(TAG, "Content-Length: %s", evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
             ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        default:
            break;
    }
    return ESP_OK;
}

std::string GoogleCalendar::refreshAccessToken() {
    const std::string url = "https://oauth2.googleapis.com/token";
    std::string accessToken;

    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.cert_pem = server_googleapis_root_cert_pem_start;
    config.cert_len = server_googleapis_root_cert_pem_end - server_googleapis_root_cert_pem_start;
    config.event_handler = _http_event_handler;
    config.user_data = local_response_buffer;        // Pass address of local buffer to get response
    config.disable_auto_redirect = true;
    esp_log_level_set("*", ESP_LOG_DEBUG);
    esp_http_client_handle_t client = esp_http_client_init(&config);

    std::string postData = 
        "client_id=" + CalendarConfig::getClientId() +
        "&client_secret=" + CalendarConfig::getClientSecret() +
        "&refresh_token=" + CalendarConfig::getRefreshToken() +
        "&grant_type=refresh_token";

    esp_http_client_set_url(client, url.c_str());
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, postData.c_str(), postData.length());

    ESP_LOGI(TAG, "Sending POST Data: %s", postData.c_str());
    ESP_LOGI(TAG, "Sending POST length: %d", postData.length());

    if (esp_http_client_perform(client) == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        if (statusCode == 200) {
            json jsonResponse = json::parse(local_response_buffer);
            if (jsonResponse.contains("access_token")) {
                accessToken = jsonResponse["access_token"].get<std::string>();
                CalendarConfig::setAccessToken(accessToken); // Update the access token
            } else {
                ESP_LOGE(TAG, "Missing 'access_token' in the response.");
            }  
        }else{
            ESP_LOGE(TAG, "HTTP status code: %d", statusCode);
        }
    }else{
        ESP_LOGE(TAG, "HTTP request failed.");
    }

    esp_http_client_cleanup(client);
    return accessToken;
}


esp_err_t GoogleCalendar::getEvents(const std::string& accessToken, const std::string& calendarId, std::vector<CalendarEvent>& events) { 
    
    const std::string baseUrl = "https://www.googleapis.com/calendar/v3/calendars/";
    const std::string timeRange = createTimeRange();
    const std::string url = baseUrl + calendarId + "/events" + timeRange;

    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "accessToken: %s", accessToken.c_str());

    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.timeout_ms = 10000;
    config.cert_pem = server_googleapis_root_cert_pem_start;
    config.cert_len = server_googleapis_root_cert_pem_end - server_googleapis_root_cert_pem_start;
    config.event_handler = _http_event_handler;
    config.user_data = local_response_buffer;        // Pass address of local buffer to get response
    config.buffer_size_tx = MAX_HTTP_TX_BUFFER;
    config.disable_auto_redirect = true;
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Authorization", ("Bearer " + accessToken).c_str());

    if (esp_http_client_perform(client) == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        if (statusCode == 200) {
            parseEvents(local_response_buffer, events);
        }else{
            ret = ESP_ERR_HTTP_INVALID_TRANSPORT;
        }
    }else{
            ret = ESP_ERR_HTTP_CONNECT;
    }

    esp_http_client_cleanup(client);
    return ret;
}

void GoogleCalendar::parseEvents(const std::string& payload, std::vector<CalendarEvent>& eventsVector) {
    json response = json::parse(payload);

    if (response.contains("items")) {
        for (const auto& item : response["items"]) {
            CalendarEvent calEvent;
            calEvent.summary = item.value("summary", "");
            calEvent.description = item.value("description", "");
            calEvent.creatorEmail = item["creator"].value("email", "");
            calEvent.organizerDisplayName = item["organizer"].value("displayName", "");

            if (item["start"].contains("date")) {
                calEvent.start = item["start"]["date"].get<std::string>();
                calEvent.end = item["end"]["date"].get<std::string>();
                calEvent.isAllDayEvent = true;
            } else {
                calEvent.start = item["start"]["dateTime"].get<std::string>();
                calEvent.end = item["end"]["dateTime"].get<std::string>();
                calEvent.isAllDayEvent = false;
            }

            eventsVector.push_back(calEvent);
        }
    }
}

std::string GoogleCalendar::createTimeRange() {
    time_t now;
    struct tm timeinfo;
    char timeMin[40], timeMax[40];

    time(&now);
    localtime_r(&now, &timeinfo);

    // First day of the current month
    snprintf(timeMin, sizeof(timeMin), "%04d-%02d-01T00:00:00Z", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);

    // Last day of the current month
    int lastDay = 31;
    if (timeinfo.tm_mon + 1 == 4 || timeinfo.tm_mon + 1 == 6 || 
        timeinfo.tm_mon + 1 == 9 || timeinfo.tm_mon + 1 == 11) {
        lastDay = 30;
    } else if (timeinfo.tm_mon + 1 == 2) {
        lastDay = (timeinfo.tm_year % 4 == 0 && (timeinfo.tm_year % 100 != 0 || timeinfo.tm_year % 400 == 0)) ? 29 : 28;
    }
    snprintf(timeMax, sizeof(timeMax), "%04d-%02d-%02dT23:59:59Z", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, lastDay);

    return "?timeMin=" + std::string(timeMin) + "&timeMax=" + std::string(timeMax);
}
