#pragma once

// WiFi Configuration
#define WIFI_SSID   "<your_wifi_ssid>"
#define WIFI_PASS   "<your_wifi_password>"

// HTTP Configuration
#define MAX_HTTP_RECV_BUFFER    512
#define MAX_HTTP_TX_BUFFER      2048
#define MAX_HTTP_OUTPUT_BUFFER  12800

// Google Calendar 
#define _clientId        "<your_client_id>"
#define _clientSecret    "<your_client_secret>"
#define _accessToken     "<your_access_token>";
#define _refreshToken    "<your_refresh_token>"
#define _CALENDAR_IDS { \
    "<calendar_id>@group.calendar.google.com", \
    "<calendar_id>@group.calendar.google.com", \
    "<calendar_id>@group.calendar.google.com", \
}

// LocalTime
#define TimeZone "PST8PDT" // Pacific Standard Time