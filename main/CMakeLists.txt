# List of source files
set(SOURCES
    "main.cpp"
    "application.cpp"
    "wifi.cpp"
    "localtime.cpp"
    "epaper.cpp"
    "g_calendar.cpp"
    "g_calendar_config.cpp"
)

# List of include directories
set(INCLUDE_DIRS
    "include"
)

# Required components
set(REQUIRED_COMPONENTS
    nvs_flash
    epdiy
    esp_wifi
    lwip
    esp_http_client
)

# Embedding files
set(EMBED_FILES
    server_certs/server_googleapis_root_cert.pem
)

# Register the component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${REQUIRED_COMPONENTS}
    EMBED_TXTFILES ${EMBED_FILES}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++11)