#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "wifi_sta.h"

static const char *TAG = "main";

static void http_get_task(void *pvParameters)
{
    esp_http_client_config_t config = {
        .url = "https://svatkyapi.cz/api/day",
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        printf("HTTP GET Status = %d\n", esp_http_client_get_status_code(client));
        printf("HTTP GET Content Length = %lld\n", esp_http_client_get_content_length(client));

        char *response_buf = malloc(esp_http_client_get_content_length(client) + 1);
        if (response_buf)
        {
            printf("Memory allocated\n");

            esp_http_client_read(client, response_buf, esp_http_client_get_content_length(client));
            response_buf[esp_http_client_get_content_length(client)] = '\0';

            printf("HTTP Response: %s\n", response_buf);

            cJSON *json = cJSON_Parse(response_buf);
            if (json == NULL)
            {
                printf("JSON Parse Error\n");
            }
            else
            {
                cJSON *name_item = cJSON_GetObjectItem(json, "name");
                if (cJSON_IsString(name_item))
                {
                    printf("Name: %s\n", name_item->valuestring);
                }
                cJSON_Delete(json);
            }
            free(response_buf);
        }
        else
        {
            printf("Failed to allocate memory for response buffer\n");
        }
    }
    else
    {
        printf("HTTP GET request failed: %s\n", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

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

    xTaskCreate(&http_get_task, "http_get_task", 8192, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
