#include <esp_http_client.h>
#include "https_client.h"
#include "esp32_time.h"

#define MAX_HTTP_RECV_BUFFER 512
static const char *TAG = "HTTPS_CLIENT";

int post_res = -1;
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
        ESP_LOGI(TAG, "recv msg %.*s  %d\n", evt->data_len, (char *)evt->data, esp_http_client_is_chunked_response(evt->client));
        if (esp_http_client_is_chunked_response(evt->client) == 0 || esp_http_client_is_chunked_response(evt->client) == 1)
        {
            if (strstr((char *)evt->data, "created"))
                post_res = 1;         
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}


/* kaco-lanstick 20220802 + */
void https_postdata(char *data, int *code, int *res)
{
    esp_http_client_config_t config = {
        .url = "https://mii-csv.meteocontrol.de/v2",
        .event_handler = _http_event_handler,
        .is_async = false,
        .timeout_ms = 5000,
    }; // true
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;

    // printf("pubqqqqqqqqqqqqqqqqqqqqqqqq\n");
    esp_http_client_set_header(client, "Content-Type", "text/csv");
    esp_http_client_set_header(client, "API-Key", "isejgjsnl4j05bc");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, data, strlen(data));

    int http_con_time = get_second_sys_time();
    while (1)
    {
        // printf("pubwwwwwwwwwwwwwwwwwwwww\n");
        err = esp_http_client_perform(client);
        if (err != ESP_ERR_HTTP_EAGAIN)
        {
            break;
        }

        if (get_second_sys_time() - http_con_time > 200)
        {
            printf("pub timeout++++%lld++%d+++\n", get_second_sys_time(), http_con_time);
            break;
        }
    }

    *code = esp_http_client_get_status_code(client);
    *res = post_res;
    if (err == ESP_OK)
    {
        if (post_res == 1 && esp_http_client_get_status_code(client) == 202)
            printf("recv ok \n");

        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    post_res = -1;
    esp_http_client_cleanup(client);
}
