#include "app_config.hpp"
#include "g_calendar_config.hpp"

// Define static variables
std::string CalendarConfig::clientId = _clientId;
std::string CalendarConfig::clientSecret = _clientSecret;
std::string CalendarConfig::accessToken = _accessToken;
std::string CalendarConfig::refreshToken = _refreshToken;

// Getters and Setters for clientId
void CalendarConfig::setClientId(const std::string& id) {
    clientId = id;
}

const std::string& CalendarConfig::getClientId() {
    return clientId;
}

// Getters and Setters for clientSecret
void CalendarConfig::setClientSecret(const std::string& secret) {
    clientSecret = secret;
}

const std::string& CalendarConfig::getClientSecret() {
    return clientSecret;
}

// Getters and Setters for accessToken
void CalendarConfig::setAccessToken(const std::string& token) {
    accessToken = token;
}

const std::string& CalendarConfig::getAccessToken() {
    return accessToken;
}

// Getters and Setters for refreshToken
void CalendarConfig::setRefreshToken(const std::string& token) {
    refreshToken = token;
}

const std::string& CalendarConfig::getRefreshToken() {
    return refreshToken;
}
