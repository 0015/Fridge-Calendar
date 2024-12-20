#ifndef G_CALENDAR_CONFIG_HPP
#define G_CALENDAR_CONFIG_HPP

#include <string>
#include <vector>

class CalendarConfig {
public:
     static const std::vector<std::string> calendarIds;

    // Getters and Setters for OAuth credentials and tokens
    static void setClientId(const std::string& id);
    static const std::string& getClientId();

    static void setClientSecret(const std::string& secret);
    static const std::string& getClientSecret();

    static void setAccessToken(const std::string& token);
    static const std::string& getAccessToken();

    static void setRefreshToken(const std::string& token);
    static const std::string& getRefreshToken();

private:
    CalendarConfig() = default; // Prevent instantiation

    // OAuth credentials and tokens
    static std::string clientId;
    static std::string clientSecret;
    static std::string accessToken; // Continuously updated
    static std::string refreshToken;

    // List of calendar IDs
    //static const std::vector<std::string> calendarIds;
};

#endif // CALENDAR_CONFIG_HPP
