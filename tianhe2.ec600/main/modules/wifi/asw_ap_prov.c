/* SoftAP based Custom Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <lwip/err.h>
#include <lwip/sys.h>

//#include "app_prov.h"
#include "esp_wifi_types.h"

#include "freertos/event_groups.h"
#include "esp_log.h"

#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include "tcpip_adapter.h"
#include "esp_eth.h"

#include <esp_http_server.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_event.h>
#include "cJSON.h"
#include "data_process.h"
#include "asw_store.h"

#define CONFIG_AP_SSID "AISWEI-9999"
#define CONFIG_AP_PASS "65468798"

#define EXAMPLE_AP_RECONN_ATTEMPTS CONFIG_EXAMPLE_AP_RECONN_ATTEMPTS
static const char *TAG = "app";

httpd_handle_t server = {0};

extern const httpd_uri_t asw_get_callback;
extern const httpd_uri_t asw_post_callback;

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &asw_get_callback);
        httpd_register_uri_handler(server, &asw_post_callback);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void start_new_server(void)
{
    start_webserver();
}

static char g_ap[32] = {0};
static char g_psw[64] = {0};
void get_ap_psw(char *ap, char *psw)
{
    memcpy(ap, g_ap, strlen(g_ap));
    memcpy(psw, g_psw, strlen(g_psw));
}

/* Initialize Wi-Fi as sta and set scan method */
void scan_ap_ini(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    // wifi_ap_para_t ap_para = {0};
    // general_query(NVS_AP_PARA, &ap_para);

    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);

    wifi_config_t ap_config = {
        .ap = {
            .channel = 5,
            .password = "12345678",
            .ssid = CONFIG_AP_SSID,
            .ssid_len = 0,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_PSK}};

    if (strlen(setting.psn) > 0)
    {
        memset(ap_config.ap.ssid, 0, sizeof(ap_config.ap.ssid));
        memset(ap_config.ap.password, 0, sizeof(ap_config.ap.password));

        memcpy(ap_config.ap.ssid, setting.psn, strlen(setting.psn));
        memcpy(ap_config.ap.password, CONFIG_AP_PASS, strlen(CONFIG_AP_PASS));

        ap_config.ap.authmode = WIFI_AUTH_WPA_PSK; //WIFI_AUTH_OPEN;
    }
    else
    {
        ap_config.ap.authmode = WIFI_AUTH_WPA_PSK; //WIFI_AUTH_OPEN;
    }
    memcpy(g_ap, ap_config.ap.ssid, strlen(ap_config.ap.ssid));
    memcpy(g_psw, ap_config.ap.password, strlen(ap_config.ap.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    printf("AP %s %s \n", ap_config.ap.ssid, ap_config.ap.password);

    ESP_ERROR_CHECK(esp_wifi_start());

    sleep(1);
}

void asw_ap_start()
{
    int provisioned = 0;
    scan_ap_ini();
    start_new_server();
    sleep(2);
}
