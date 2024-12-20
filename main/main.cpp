// ESP-IDF Version 5.3.1

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "application.hpp"

void app_task(void *param) {
    Application *app = static_cast<Application*>(param);
    app->run();
    vTaskDelete(NULL);
}

extern "C" void app_main() {
    static Application app;
    xTaskCreate(app_task, "app_task", 20240, &app, 5, NULL);
}