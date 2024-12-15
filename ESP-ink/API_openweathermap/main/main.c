#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h> // includes MIN macro

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "secrets.h"
#include "wifi_sta.h"

#define MAX_HTTP_OUTPUT_BUFFER 8192

extern const char openweathermaporg_cert_pem_start[] asm("_binary_openweathermaporg_cert_pem_start");
extern const char openweathermaporg_cert_pem_end[] asm("_binary_openweathermaporg_cert_pem_end");

static const char *TAG = "main";

static char received_data[MAX_HTTP_OUTPUT_BUFFER + 1] = {0}; // Extra byte for the NULL character
static int received_data_len = 0;
static bool received_data_valid = false;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;

    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;

    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

        // Clean the buffer in case of a new request
        if (received_data_len == 0 && evt->user_data)
        {
            // we are just starting to copy the output data into the use
            memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
        }

        if (!esp_http_client_is_chunked_response(evt->client))
        {
            int copy_len = 0;

            if (evt->user_data) // If user_data buffer is configured, copy the response into the buffer
            {
                // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - received_data_len));
                if (copy_len)
                {
                    memcpy(evt->user_data + received_data_len, evt->data, copy_len);
                }
            }
            else // If user_data buffer is not configured, accumulate the response in output_buffer
            {
                int content_len = esp_http_client_get_content_length(evt->client);

                if (received_data_len == 0)
                {
                    memset(received_data, 0, MAX_HTTP_OUTPUT_BUFFER + 1);
                }

                copy_len = MIN(evt->data_len, (content_len - received_data_len));
                if (copy_len)
                {
                    memcpy(received_data + received_data_len, evt->data, copy_len);
                }
            }
            received_data_len += copy_len;
        }
        else // Chunked encoding
        {
            ESP_LOGI(TAG, "Received data chunk = %.*s", evt->data_len, (char *)evt->data);

            if (received_data_len == 0)
            {
                memset(received_data, 0, MAX_HTTP_OUTPUT_BUFFER + 1);
            }

            // Check if we have enough space in the buffer
            if (received_data_len + evt->data_len <= MAX_HTTP_OUTPUT_BUFFER)
            {
                // Append the received data to the global buffer
                memcpy(received_data + received_data_len, evt->data, evt->data_len);
                received_data_len += evt->data_len;
            }
            else
            {
                ESP_LOGW(TAG, "Not enough space to store all data in buffer.");
                return ESP_FAIL;
            }
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");

        if (received_data_len > 0)
        {
            // Add null-termination character to the end of the string
            if (received_data_len <= MAX_HTTP_OUTPUT_BUFFER)
            {
                received_data[received_data_len] = '\0';
            }
            ESP_LOGI(TAG, "Accumulated Response = %.*s", received_data_len, received_data);
            received_data_len = 0;
            received_data_valid = true;
        }
        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;

    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_header(evt->client, "From", "user@example.com");
        esp_http_client_set_header(evt->client, "Accept", "text/html");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

static void https_with_url(void)
{
    char url[200];
    snprintf(url, sizeof(url), "%s%s%s%s%s%s%s",
             "https://api.openweathermap.org/data/3.0/onecall?lat=",
             OPENWEATHERMAP_LATITUDE, "&lon=", OPENWEATHERMAP_LONGITUDE,
             "&appid=", OPENWEATHERMAP_API_KEY,
             "&units=metric&exclude=minutely,hourly");

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .cert_pem = openweathermaporg_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    // In ESP-IDF case, word = uint8_t even though it's a 32-bit system, so uxTaskGetStackHighWaterMark returns number of bytes
    ESP_LOGI(TAG, "Stack high water mark: %d bytes", uxTaskGetStackHighWaterMark(NULL));

    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_DEBUG);

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

    xTaskCreate(&https_with_url, "https_with_url", 8192, NULL, 5, NULL);

    while (!received_data_valid)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Desired quantities to parse:
    // current/temp
    // current/feels_like
    // current/weather/main
    // daily/0/temp/day
    // daily/0/pop
    // daily/0/rain
    // daily/0/weather/0/main

    cJSON *json = cJSON_Parse(received_data); // Watch out, this contains dynamic memory allocation
    if (json != NULL)
    {
        // Process the JSON object
        ESP_LOGI(TAG, "JSON parsed successfully");
        // Don't forget to free the JSON object when done

        cJSON *current = cJSON_GetObjectItem(json, "current");
        if (current != NULL)
        {
            cJSON *temp = cJSON_GetObjectItem(current, "temp");
            if (temp != NULL)
            {
                ESP_LOGI(TAG, "Current temperature: %f", temp->valuedouble);
            }
            cJSON *feels_like = cJSON_GetObjectItem(current, "feels_like");
            if (feels_like != NULL)
            {
                ESP_LOGI(TAG, "Feels like: %f", feels_like->valuedouble);
            }
            cJSON *weather = cJSON_GetObjectItem(current, "weather");
            if (weather != NULL)
            {
                cJSON *weather_0 = cJSON_GetArrayItem(weather, 0);
                if (weather_0 != NULL)
                {
                    cJSON *main = cJSON_GetObjectItem(weather_0, "main");
                    if (main != NULL)
                    {
                        ESP_LOGI(TAG, "Current weather: %s", main->valuestring);
                    }
                }
            }
        }

        cJSON *daily = cJSON_GetObjectItem(json, "daily");
        if (daily != NULL)
        {
            cJSON *daily_0 = cJSON_GetArrayItem(daily, 0);
            if (daily_0 != NULL)
            {
                cJSON *temp = cJSON_GetObjectItem(daily_0, "temp");
                if (temp != NULL)
                {
                    cJSON *day = cJSON_GetObjectItem(temp, "day");
                    if (day != NULL)
                    {
                        ESP_LOGI(TAG, "Daily temperature: %f", day->valuedouble);
                    }
                }
                cJSON *pop = cJSON_GetObjectItem(daily_0, "pop");
                if (pop != NULL)
                {
                    ESP_LOGI(TAG, "Daily pop: %f", pop->valuedouble);
                }
                cJSON *rain = cJSON_GetObjectItem(daily_0, "rain");
                if (rain != NULL)
                {
                    ESP_LOGI(TAG, "Daily rain: %f", rain->valuedouble);
                }
                cJSON *weather = cJSON_GetObjectItem(daily_0, "weather");
                if (weather != NULL)
                {
                    cJSON *weather_0 = cJSON_GetArrayItem(weather, 0);
                    if (weather_0 != NULL)
                    {
                        cJSON *main = cJSON_GetObjectItem(weather_0, "main");
                        if (main != NULL)
                        {
                            ESP_LOGI(TAG, "Daily weather: %s", main->valuestring);
                        }
                    }
                }
            }
        }

        cJSON_Delete(json); // Deallocate the JSON object
    }
    else
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
    }

    return;
}
