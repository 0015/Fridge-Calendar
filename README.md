# Fridge Calendar (E-Paper Google Calendar Display)

This project is an e-paper-based calendar display that integrates with Google Calendar to fetch and display daily events. It uses the ESP32-S3 microcontroller, the EPDiy library for e-paper control, and the Google Calendar API to retrieve event data. The device connects to WiFi for API calls and keeps track of local time for accurate scheduling and updates.

[![Showcase](https://raw.githubusercontent.com/0015/Fridge-Calendar/refs/heads/main/misc/fridge_calendar.jpeg)](https://youtu.be/2Iy_9JYkWGs)

---

## Features

- **9.7-inch Black and White E-Paper Display**: Displays a monthly calendar and upcoming events using the EPDiy library.
- **WiFi Connectivity**: Required for API calls to fetch Google Calendar events.
- **Local Time Management**: Synchronizes and maintains local time for accurate scheduling.
- **Google Calendar Integration**: Fetches events using Google Calendar API in JSON format.
- **Automatic Daily Updates**: Refreshes the display at 00:10 every day.
- **Deep Sleep Mode**: Saves power by putting the ESP32 into deep sleep between updates.
- **Robust Retry Mechanism**: Handles API call failures with a retry mechanism and enters indefinite deep sleep after repeated failures.

---

## Table of Contents

- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [System Workflow](#system-workflow)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Hardware Requirements

- ESP32-S3 Based, [Epdiy V7 board](https://vroland.github.io/epdiy-hardware/) 
   - Support for high-resolution displays with 16 data lines
   - Much faster updates thanks to the ESP32S3 SOC and the LCD peripheral
   - An on-board RTC, LiPo charging   
- 9.7-inch black and white e-paper display(ED097TC2) compatible with the [EPDiy library](https://github.com/vroland/epdiy)
- Power supply (battery or USB)

[![hardware](https://raw.githubusercontent.com/0015/Fridge-Calendar/refs/heads/main/misc/hardware.jpeg)]


---

## Software Requirements

- [ESP-IDF](https://github.com/espressif/esp-idf): ESP32 development framework (v5.3.1)
- [EPDiy library](https://github.com/vroland/epdiy): For e-paper control
- [nlohmann/json](https://github.com/nlohmann/json): For JSON parsing
- Google Calendar API key and OAuth credentials
   - [Learn about authentication and authorization](https://developers.google.com/workspace/guides/auth-overview)
   - [How to access Google APIs using OAuth 2.0 in Postman](https://blog.postman.com/how-to-access-google-apis-using-oauth-in-postman/)

---

## Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/<your-username>/epaper-google-calendar.git
   cd epaper-google-calendar
   ```

2. **Set up ESP-IDF**:
   Follow the [ESP-IDF setup guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for your platform.

3. **Install Dependencies**:
   - Install the [EPDiy library](https://github.com/vroland/epdiy) into your ESP-IDF project.
   - Include the [nlohmann/json](https://github.com/nlohmann/json) header file in your project.

4. **Configure Project**:
   - Set your WiFi credentials, Google API credentials, and other configurations in `app_config.h`:
     ```cpp
     #define WIFI_SSID "your_wifi_ssid"
     #define WIFI_PASS "your_wifi_password"
     #define CLIENT_ID "your_client_id"
     #define CLIENT_SECRET "your_client_secret"
     #define ACCESS_TOKEN "your_access_token"
     #define REFRESH_TOKEN "your_refresh_token"
     #define CALENDAR_IDS "<multiple_calendar_ids>"
     #define TIMEZONE "your_time_zone"
     ```

5. **Build and Flash**:
   ```bash
   idf.py build
   idf.py flash
   ```

---

## Usage

- Upon powering up, the device connects to WiFi, synchronizes local time, and fetches Google Calendar events.
- Displays a monthly calendar and a list of upcoming events.
- Automatically updates at 00:30 every day or after a successful fetch of calendar events.
- If the API call fails twice in a row, the device enters indefinite deep sleep.

---

## Configuration

### Google Calendar API Setup

1. Go to the [Google Cloud Console](https://console.cloud.google.com/).
2. Create a new project and enable the **Google Calendar API**.
3. Create OAuth credentials for your project.
4. Obtain the API key(Access Token), Refresh Token, client ID, and client secret.
5. Add the credentials to your `app_config.h` file.

[![Renew Access Token](https://raw.githubusercontent.com/0015/Fridge-Calendar/refs/heads/main/misc/renewed_access_token.png)]


### WiFi Credentials

- Update the WiFi SSID and password in the `app_config.h` file.

---

## System Workflow

### Initialization
1. Initialize Non-Volatile Storage (NVS) to store state information.
2. Display a splash screen and progress bar if this is the first run.
3. Connect to WiFi and synchronize local time.

### Main Operation
1. Fetch events from Google Calendar using the REST API.
2. Parse JSON data into a usable format.
3. Display the calendar and events on the e-paper screen.

### Retry and Sleep Logic
- On a failed API call:
  - Increment a retry counter stored in NVS.
  - Reboot and retry the operation.
- After two failed attempts:
  - Enter deep sleep indefinitely to save power.
- On success:
  - Reset the retry counter and schedule a wakeup for 00:30 the next day.

---

## Troubleshooting

### Common Issues

1. **WiFi Connection Fails**:
   - Check your WiFi SSID and password in `app_config.h`.
   - Ensure the ESP32 is within range of the WiFi router.

2. **Google API Fails**:
   - Verify your Access Token, Refresh Token and OAuth credentials.
   - Check if the Google Calendar API is enabled in your Google Cloud Console.

3. **NVS Errors**:
   - Ensure NVS is initialized properly during startup.
   - Erase NVS if necessary using `idf.py erase_flash`.

4. **E-paper Display Issues**:
   - Verify the connection between the ESP32 and e-paper display.
   - Check if the EPDiy library is installed correctly.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.