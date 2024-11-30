#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "wifi_sta.h"
#include "sntp_api.h"

static const char *TAG = "main";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_sta();
    initialize_sntp();

    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    struct timeval tv;
    struct tm timeinfo;
    char time_str[64];

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);

    uint64_t time_start_us = ((uint64_t)tv.tv_sec * 1000000) + (uint64_t)tv.tv_usec;
    uint64_t time_now_us, time_diff_us;

    while (true)
    {
        /*time_t now;
        struct tm timeinfo;

        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

        time(&now);
        char *time_str = asctime(localtime(&now));
        time_str[strcspn(time_str, "\n")] = '\0'; // Remove the newline
        ESP_LOGI(TAG, "Current time: %s", time_str);
        ESP_LOGI(TAG, "Time in microseconds: %lld", time_us);*/

        gettimeofday(&tv, NULL);                                              // Get the current time
        localtime_r(&tv.tv_sec, &timeinfo);                                   // Convert seconds to local time
        time_now_us = ((uint64_t)tv.tv_sec * 1000000) + (uint64_t)tv.tv_usec; // Get current time in microseconds

        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo); // Format time
        time_str[strcspn(time_str, "\n")] = '\0';                             // Remove the newline
        ESP_LOGI(TAG, "Current time: %s.%06ld, microseconds since start: %lld", time_str, tv.tv_usec, time_now_us - time_start_us);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
