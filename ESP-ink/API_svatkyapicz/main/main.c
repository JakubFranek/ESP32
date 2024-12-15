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

extern const uint8_t svatkyapicz_cert_pem_start[] asm("_binary_svatkyapicz_cert_pem_start");
extern const uint8_t svatkyapicz_cert_pem_end[] asm("_binary_svatkyapicz_cert_pem_end");

static const char *TAG = "main";

static void http_get_task(void *pvParameters)
{
    esp_http_client_config_t config = {
        .url = "https://svatkyapi.cz/api/day",
        .cert_pem = (const char *)svatkyapicz_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        printf("HTTP GET Status = %d\n", esp_http_client_get_status_code(client));

        char *buffer = calloc(512, sizeof(char)); // Allocate a buffer for chunks
        if (buffer == NULL)
        {
            printf("Failed to allocate memory for buffer\n");
            esp_http_client_cleanup(client);
            vTaskDelete(NULL);
            return;
        }

        int total_read_len = 0;
        int read_len;
        char *response = NULL;

        // Read response in chunks
        while (true)
        {
            read_len = esp_http_client_read(client, buffer, 512);
            printf("read_len: %d\n", read_len);
            for (int i = 0; i < 512; i++)
            {
                printf("%d", buffer[i]);
            }
            printf("\n");

            if (read_len <= 0)
            {
                break;
            }

            buffer[read_len] = '\0'; // Null-terminate the received chunk

            // Dynamically grow response buffer
            char *new_response = realloc(response, total_read_len + read_len + 1);
            if (new_response == NULL)
            {
                printf("Failed to allocate memory for response\n");
                free(buffer);
                free(response);
                esp_http_client_cleanup(client);
                vTaskDelete(NULL);
                return;
            }
            response = new_response;

            memcpy(response + total_read_len, buffer, read_len);
            total_read_len += read_len;
            response[total_read_len] = '\0'; // Null-terminate the entire response
        }
        free(buffer);

        if (response)
        {
            printf("HTTP Response: %s\n", response);

            // Parse JSON
            cJSON *json = cJSON_Parse(response);
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
            free(response);
        }
        else
        {
            printf("No response received or response was empty.\n");
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
