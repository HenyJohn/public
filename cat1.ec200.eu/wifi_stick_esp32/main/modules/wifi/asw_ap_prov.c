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

#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN

#define DEFAULT_SCAN_LIST_SIZE 80
#define CONFIG_AP_SSID "AISWEI-9999"

#define EXAMPLE_AP_RECONN_ATTEMPTS CONFIG_EXAMPLE_AP_RECONN_ATTEMPTS

/* Set the SSID and Password via project configuration, or can set directly here */
#define CONFIG_EXAMPLE_WIFI_SSID "SS1289"
#define CONFIG_EXAMPLE_WIFI_PASSWORD "123456789"

#define DEFAULT_SSID CONFIG_EXAMPLE_WIFI_SSID
#define DEFAULT_PWD CONFIG_EXAMPLE_WIFI_PASSWORD

#if CONFIG_EXAMPLE_WIFI_ALL_CHANNEL_SCAN
#define DEFAULT_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#elif CONFIG_EXAMPLE_WIFI_FAST_SCAN
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#else
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#endif /*CONFIG_EXAMPLE_SCAN_METHOD*/

#if CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SECURITY
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#else
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#endif /*CONFIG_EXAMPLE_SORT_METHOD*/

#if CONFIG_EXAMPLE_FAST_SCAN_THRESHOLD
#define DEFAULT_RSSI CONFIG_EXAMPLE_FAST_SCAN_MINIMUM_SIGNAL
#if CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_OPEN
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#elif CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_WEP
#define DEFAULT_AUTHMODE WIFI_AUTH_WEP
#elif CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_WPA
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA_PSK
#elif CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_WPA2
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA2_PSK
#else
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#endif
#else
#define DEFAULT_RSSI -127
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#endif /*CONFIG_EXAMPLE_FAST_SCAN_THRESHOLD*/

static wifi_scan_config_t scanres = {
    .ssid = NULL,
    .bssid = NULL,
    .channel = 0,
    .show_hidden = 1};

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

void get_ap_psw(char *ap, char *psw)
{
    wifi_ap_para_t ap_para = {0};
    general_query(NVS_AP_PARA, &ap_para);

    if (strlen(ap_para.ssid) > 0)
    {
        memcpy(ap, ap_para.ssid, strlen(ap_para.ssid));
        memcpy(psw, ap_para.password, strlen(ap_para.password));
    }
}

/* Initialize Wi-Fi as sta and set scan method */
void scan_ini(void)
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

    char new_ssid[64] = {0};
    char new_pass[64] = {0};

    memcpy(new_ssid, "ssd123456", strlen("ssd123456"));
    memcpy(new_pass, "123456789", strlen("123456789"));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = new_ssid,     //DEFAULT_SSID,
            .password = new_pass, //DEFAULT_PWD,
            .scan_method = DEFAULT_SCAN_METHOD,
            .sort_method = DEFAULT_SORT_METHOD,
            .threshold.rssi = DEFAULT_RSSI,
            .threshold.authmode = DEFAULT_AUTHMODE,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    //printf("STA %s %s \n", wifi_config.sta.ssid, wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_start());
    sleep(1);
}

static int lookup_channel(void)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t *ap_info = calloc(DEFAULT_SCAN_LIST_SIZE, sizeof(wifi_ap_record_t));
    // wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE] = {0};
    uint16_t ap_count = 0;
    int cnt[14] = {0};
    int j = 0;
    int channel = 0;

    memset(ap_info, 0, sizeof(ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    if (ap_count > DEFAULT_SCAN_LIST_SIZE)
    {
        ap_count = DEFAULT_SCAN_LIST_SIZE;
    }

    if (ap_count <= 0)
    {
        free(ap_info);
        return 11;
    }

    printf("total: %d\n", ap_count);
    /** count ap channel 1-13 usage*/
    for (j = 0; j < ap_count; j++)
    {
        int chan = ap_info[j].primary;
        if (chan >= 1 && chan <= 13)
        {
            cnt[chan]++;
        }
    }

    /** print channel usage:*/
    for (j = 1; j <= 13; j++)
    {
        printf("channel: %d usage: %d\n", j, cnt[j]);
    }

    /** find less used channel*/
    int this_chan = 1;
    int this_count = cnt[1];
    for (j = 2; j <= 13; j++)
    {
        if (cnt[j] < this_count)
        {
            this_chan = j;
            this_count = cnt[j];
        }
    }
    free(ap_info);
    return this_chan;
}

int scan_best_channel(void)
{
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scanres, true));
    ESP_LOGE(TAG, "scan channel res---> ok");

    int chan = lookup_channel();
    printf("best channel: %d\n", chan);
    return chan;
    // esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE);
}

/* Initialize Wi-Fi as sta and set scan method */
void scan_ap_ini(void)
{
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    // {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);
    // tcpip_adapter_init();
    // ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));
    // // ESP_ERROR_CHECK(esp_event_loop_create_default());

    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // ESP_ERROR_CHECK(esp_wifi_start());

    scan_ini();

    int chan = scan_best_channel();
    ESP_ERROR_CHECK(esp_wifi_stop());

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_ap_para_t ap_para = {0};
    general_query(NVS_AP_PARA, &ap_para);

    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);

    wifi_config_t ap_config = {
        .ap = {
            .password = "12345678",
            .ssid = CONFIG_AP_SSID,
            .ssid_len = 0,
            .channel = chan,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_PSK}};

    if (strlen(ap_para.ssid) > 0 && strlen(setting.psn) > 0)
    {
        memset(ap_config.ap.ssid, 0, sizeof(ap_config.ap.ssid));
        memset(ap_config.ap.password, 0, sizeof(ap_config.ap.password));
        memcpy(ap_config.ap.ssid, ap_para.ssid, strlen(ap_para.ssid));
        memcpy(ap_config.ap.password, ap_para.password, strlen(ap_para.password));

        ap_config.ap.authmode = WIFI_AUTH_WPA_PSK; //WIFI_AUTH_OPEN;
    }
    else
    {
        ap_config.ap.authmode = WIFI_AUTH_WPA_PSK; //WIFI_AUTH_OPEN;
    }

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
