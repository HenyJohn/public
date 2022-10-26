/* MQTT over SSL Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
// #include "mqtt_client.h"
// #include "esp_tls.h"

#include "aiot_mqtt_sign.h"

#include "app_mqtt.h"
#include "cJSON.h"
#include "data_process.h"
#include "mqueue.h"
#include "update.h"
#include "meter.h"

char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = "a1tqX123itk";
char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = "B80052030075";
char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = "VrRCgge0PTkZesAVyGguPJrMIA4Tr4ME"; //"JDuE0bUeFjcfQucPtvnJEk5vcVcD5E2L";//"VrRCgge0PTkZesAVyGguPJrMIA4Tr4ME";

static const char *TAG = "MQTTS_EXAMPLE";

char my_uri[100] = "mqtts://a1xAam8NEec.iot-as-mqtt.cn-shanghai.aliyuncs.com:1883";
uint32_t port = 1883;

char client_id[150] = "G123456789EF&a1xAam8NEec|timestamp=123456,securemode=2,signmethod=hmacsha1,lan=C|";
char username[64] = "G123456789EF&a1xAam8NEec";
char password[65] = "6D09E41CCE71B739557F11BA6C2F26987FCF043BCED9A41CBDCC4107390360C8";

char pub_topic[100] = "/a1tqX123itk/B80052030075/user/Action";
//char sub_topic[100] = "/a1tqX123itk/B80052030075/user/get";
char sub_topic[100] = "/sys/a1tqX123itk/B80052030075/rrpc/request/+";
// int mqtt_connect_state = -1;
char rrpc_res_topic[256] = {0};

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipse_org_pem_start[] = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipse_org_pem_start[] asm("_binary_mqtt_eclipse_org_pem_start");
#endif
extern const uint8_t mqtt_eclipse_org_pem_end[] asm("_binary_mqtt_eclipse_org_pem_end");

extern int netework_state;
extern int inv_comm_error;

void download_task(void *pvParameters);

#include "asw_store.h"
int mqtt_app_start(void)
{
    int res = -1;
    ESP_LOGI(TAG, "[mqtt_app_start] Free memory: %d bytes client_id:%s", esp_get_free_heap_size(), client_id);

    Setting_Para para = {0};
    general_query(NVS_ATE, &para);

    if (strlen(para.host) > 0)
        res = cat1_mqtt_conn(0, para.host, 1883, client_id, username, password);
    else
        res = cat1_mqtt_conn(0, "cn-shanghai", 1883, client_id, username, password);

    if (res != 0)
        return -1;

    res = cat1_mqtt_sub(0, sub_topic);
    if (res != 0)
        return -1;
    cat1_set_mqtt_ok(0);
    return 0;
}

void set_mqtt_server(char *pdk, char *server, int port)
{
    snprintf(my_uri, sizeof(my_uri), "mqtts://%s.iot-as-mqtt.%s.aliyuncs.com:%d", pdk, server, port);
    printf("mqtt server %s \n", my_uri);
}

void set_product_key(char *product_key)
{
    memset(DEMO_PRODUCT_KEY, 0, sizeof(DEMO_PRODUCT_KEY));
    strncpy(DEMO_PRODUCT_KEY, product_key, sizeof(DEMO_PRODUCT_KEY));
}

void set_device_name(char *device_name)
{
    memset(DEMO_DEVICE_NAME, 0, sizeof(DEMO_DEVICE_NAME));
    strncpy(DEMO_DEVICE_NAME, device_name, sizeof(DEMO_DEVICE_NAME));
}

/*
*	setting device secrect
*
*/
void set_device_secret(char *device_secret)
{
    memset(DEMO_DEVICE_SECRET, 0, sizeof(DEMO_DEVICE_SECRET));
    strncpy(DEMO_DEVICE_SECRET, device_secret, sizeof(DEMO_DEVICE_SECRET));
}

int asw_mqtt_setup(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    aiotMqttSign(DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME, DEMO_DEVICE_SECRET, client_id, username, password);
    ESP_LOGI(TAG, "[client_id] %s\n", client_id);
    ESP_LOGI(TAG, "[username] %s\n", username);
    ESP_LOGI(TAG, "[password] %s\n", password);

    return 0;
}

//@return: -1 means failure
int asw_mqtt_publish(const char *topic, const char *data, int data_len, int qos)
{
    cat1_mqtt_pub(0, topic, data);
    // ESP_LOGI(TAG, "published, msg_id=%d ************", msg_id);
    return 0;
}

int asw_publish(void *cpayload)
{
    int res = 0;
    char *payload = (char *)cpayload;

    // asw_mqtt_publish(topic, payload, strlen(payload), ini_para.mqtt_qos);
    res = cat1_mqtt_pub(0, pub_topic, payload);

    if (res < 0)
    {
        printf("publish failed, res = %d", res);
        return -1;
    }
    return 0;
}

int StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int srcLen)
{
    char h1, h2;
    unsigned char s1, s2;
    int i;

    for (i = 0; i < srcLen / 2; i++)
    {
        h1 = pbSrc[2 * i];
        h2 = pbSrc[2 * i + 1];

        s1 = toupper(h1) - 0x30;
        if (s1 > 9)
            s1 -= 7;

        s2 = toupper(h2) - 0x30;
        if (s2 > 9)
            s2 -= 7;
        pbDest[i] = s1 * 16 + s2;
    }
    return srcLen / 2;
}

int HexToStr(unsigned char *pbDest, unsigned char *pbSrc, int srcLen)
{
    int i;
    for (i = 0; i < srcLen; i++)
    {
        char tmp[2] = {0};
        sprintf(tmp, "%02x", pbSrc[i]);
        pbDest[i * 2] = tmp[0];
        pbDest[i * 2 + 1] = tmp[1];
    }
    return srcLen * 2;
}

void send_msg(int type, unsigned char *buff, int lenthg, unsigned char *ws)
{
    cloud_inv_msg buf;
    int ret;
    //int id = msgget(0x6789, 0);

    memset(&buf, 0, sizeof(buf));
    if (lenthg)
        memcpy(buf.data, buff, lenthg);
    if (ws)
        memcpy(&buf.data[lenthg], ws, strlen(ws));
    buf.type = type;
    buf.len = lenthg;

    printf("send msg %s %d \n", buf.data, lenthg);
    //ret = msgsnd(id, &buf, sizeof(buf.data), 0);
    if (mq1 != NULL)
    {
        xQueueSend(mq1, (void *)&buf, (TickType_t)0);
        printf("send cloud msg ok\n");
    }

    //if(-1 == ret)
    //    perror("msgsnd");
}

int parse_mqtt_msg(char *payload)
{
    cJSON *json;
    json = cJSON_Parse(payload);
    cJSON *cmd_ = cJSON_GetObjectItem(json, "Action");
    if (cmd_ == NULL || cmd_->type == cJSON_NULL)
    {
        printf("mqtt msg can't found Action \n");
        return -1;
    }
    char *function = cJSON_GetObjectItem(json, "Action")->valuestring;
    printf("ACTION STR %s \n", function);
    if (function != NULL)
    {
        //{"Action":"setscan","value":{"setscan":15}}
        if (!(strcmp(function, "Scan")))
        {
            //char ws=0;
            uint8_t u8msg[128] = {0};
            unsigned int cnt = 0;
            cJSON *item = cJSON_GetObjectItem(json, "value");

            printf("cnt int %d \n", cJSON_GetObjectItem(item, "setscan")->valueint);

            cnt = cJSON_GetObjectItem(item, "setscan")->valueint;
            int byteLen = 1;
            u8msg[0] = cnt;
            printf("cnt int %d  %s\n", cnt, u8msg);
            send_msg(5, u8msg, byteLen, NULL);
        }
        else if (!(strcmp(function, "Full-Trans")))
        {
            char ws[65] = {0};
            char msg[300] = {0};
            cJSON *item = cJSON_GetObjectItem(json, "ws");
            strncpy(ws, item->valuestring, 64);
            item = cJSON_GetObjectItem(json, "message");
            strncpy(msg, item->valuestring, 300);

            printf("ms %s ws %s \n", msg, ws);
            send_msg(2, msg, strlen(msg), ws);
        }
        else if (!(strcmp(function, "reboot"))) //0, esp32;1,inv
        {
            char ws[65] = {0};
            char msg[300] = {0};
            //cJSON *item = cJSON_GetObjectItem(json, "ws");
            //strncpy(ws, item->valuestring, 64);
            //item = cJSON_GetObjectItem(json, "message");
            //strncpy(msg, item->valuestring, 300);
            memset(ws, 'T', 64);
            msg[0] = cJSON_GetObjectItem(json, "reboot")->valueint;
            printf("ms %d ws %s \n", msg[0], ws);
            send_msg(99, msg, 1, ws);
        }
        else if (!(strcmp(function, "setmeter")))
        {
            cJSON *meter = cJSON_GetObjectItem(json, "value");
            //cJSON_AddNumberToObject(meter, "date", get_current_days());
            //printf("meter set %s \n", cJSON_PrintUnformatted(cJSON_GetObjectItem(json, "value")));
            printf("meter set %s \n", cJSON_PrintUnformatted(meter));
            meter_setdata mt = {0};
            mt.enb = cJSON_GetObjectItem(meter, "enb")->valueint;
            mt.mod = cJSON_GetObjectItem(meter, "mod")->valueint;
            mt.reg = cJSON_GetObjectItem(meter, "regulate")->valueint;
            mt.target = cJSON_GetObjectItem(meter, "target")->valueint;

            int abs0 = 0;
            int offs = 0;
            getJsonNum(&abs0, "abs", meter);
            printf("abs0: %d\n", abs0);
            getJsonNum(&offs, "offset", meter);
            printf("offs: %d\n", offs);

            if (mt.reg == 10)
            {
                mt.reg = 0X100;
                if (abs0 == 1 && mt.target == 0)
                {
                    if (offs <= 0 || offs > 100)
                        offs = 5;

                    mt.reg |= offs;
                }
            }

            mt.date = get_current_days();
            write_meter_file(&mt);
            sleep(1);
            send_msg(6, "clearmeter", 9, NULL);
            //esp_restart();
            //write_meter_file(cJSON_PrintUnformatted(meter));

        } //{"Action":"upgrade","value":{"type":1, "path":{"directory":"http://zevercloud-update.oss-cn-hangzhou.aliyuncs.com/GPRS/20200515-1", "files":["master.bin","slave.bin","safty.bin"]}}}
        else if (!(strcmp(function, "upgrade")))
        {
            cJSON *item = cJSON_GetObjectItem(json, "value");
            if (cJSON_HasObjectItem(item, "p2p") == 1)
            {
                g_p2p_mode = cJSON_GetObjectItem(item, "p2p")->valueint;
                printf("p2p %d \n", g_p2p_mode);
            }

            char up_type = cJSON_GetObjectItem(item, "type")->valueint;
            printf("update type %d \n", up_type);

            cJSON *item1 = cJSON_GetObjectItem(item, "path");
            char *url0 = cJSON_GetObjectItem(item1, "directory")->valuestring;
            printf("url %s \n", url0);

            cJSON *item2 = cJSON_GetObjectItem(item1, "files");
            printf("file type %d %d \n", item2->type, cJSON_GetArraySize(item2));
            printf("meter str %s \n", cJSON_GetArrayItem(item2, 0)->valuestring);
            char *p[cJSON_GetArraySize(item2)];

            update_url download_url = {0};

            if (cJSON_GetArraySize(item2) == 1)
            {
                p[0] = cJSON_GetArrayItem(item2, 0)->valuestring;

                if (up_type == 1) //item2->type == 1)
                {
                    download_url.update_type = 1;
                    sprintf(download_url.down_url, "%s/%s", url0, p[0]);
                }
                else
                {
                    download_url.update_type = 2;
                    sprintf(download_url.down_url, "%s/%s", url0, p[0]);
                }
            }

            printf("dowan_loadurl %s  %d %d\n", download_url.down_url, download_url.update_type, strlen(download_url.down_url));

            TaskHandle_t download_task_handle = NULL;
            BaseType_t xReturned;

            download_task((void *)&download_url);
            // xReturned = xTaskCreate(
            //     download_task,          /* Function that implements the task. */
            //     "download task",        /* Text name for the task. */
            //     10240,                  /* Stack size in words, not bytes. 50K = 25600 */
            //     (void *)&download_url,  /* Parameter passed into the task. */
            //     6,                      /* Priority at which the task is created. */
            //     &download_task_handle); /* Used to pass out the created task's handle. */
            // if (xReturned != pdPASS)
            // {
            //     printf("create cgi task failed.\n");
            // }
        }
    }
}

int sent_newmsg(void)
{
    char ws[65] = {0};
    char msg[32] = {0};
    memset(ws, 'U', 64);
    msg[0] = 0;
    printf("rebootms %d ws %s \n", msg[0], ws);
    send_msg(99, msg, 1, ws);
    return 0;
}

int mqtt_client_destroy_free(void)
{
    int res = -1;

    printf("mqtt_client_destroy_free \n");
    res = cat1_mqtt_release(0);
    // mqtt_connect_state = -1;
    netework_state = 1;
}

int replace_string(char *str, char *outstr, char *oldstr, char *newstr)
{
    char bstr[strlen(str)];
    memset(bstr, 0, sizeof(bstr));
    int i = 0;
    for (i = 0; i < strlen(str); i++)
    {
        if (!strncmp(str + i, oldstr, strlen(oldstr)))
        {
            strcat(bstr, newstr);
            i += strlen(oldstr) - 1;
        }
        else
        {
            strncat(bstr, str + i, 1);
        }
    }

    strcpy(outstr, bstr);
    return 0;
}

int get_rrpc_restopic(char *rpc_topic, int rpc_len, char *response_topic)
{
    int res = 0;
    //char *payload = (char *)cpayload;
    //char *payload = "inverter scan has completed";
    char resp_topicx[256] = {0};
    char resp_topic[256] = {0};
    if (rpc_len > 256)
        return -1;

    memcpy(resp_topicx, rpc_topic, rpc_len);
    replace_string(resp_topicx, resp_topic, "request", "response");
    printf("response publish %s\n", resp_topic);
    // res=asw_mqtt_publish(resp_topic, (uint8_t *)payload, strlen(payload), 0);
    memcpy(response_topic, resp_topic, strlen(resp_topic));
    return 0;
}

#include "asw_store.h"
void setting_new_server(cJSON *json)
{
    Setting_Para setting = {0};
    if (json == NULL)
        return;
    int res = general_query(NVS_ATE, &setting);
    if (res == 0)
    {
        memset(&setting.host, 0, sizeof(setting.host));
        memset(&setting.product_key, 0, sizeof(setting.product_key));
        memset(&setting.device_secret, 0, sizeof(setting.device_secret));

        getJsonStr(setting.host, "host", sizeof(setting.host), json);
        getJsonStr(setting.product_key, "pdk", sizeof(setting.product_key), json);
        getJsonStr(setting.device_secret, "sec", sizeof(setting.device_secret), json);

        general_add(NVS_ATE, &setting);
    }
}

int parse_mqtt_msg_rrpc(char *rpc_topic, int rpc_len, void *payload, int data_len)
{
    char *recv_ok = "{\"dat\":\"ok\"}";
    char *recv_er = "{\"dat\":\"err\"}";
    if (strlen(payload) < 3)
        return -1;
    cJSON *json;
    json = cJSON_Parse(payload);

    char resp_topic[256] = {0};
    if (get_rrpc_restopic(rpc_topic, rpc_len, resp_topic) < 0)
    {
        printf("mqtt rrpc topic too long \n");
        asw_mqtt_publish(resp_topic, (uint8_t *)recv_er, strlen(recv_er), 0);
        cJSON_Delete(json);
        return -1;
    }
    printf("RRPC ACTION STR \n"); // item = cJSON_GetObjectItem(root, "semantic");
    //char *function = cJSON_GetObjectItem(json, "setscan")->valueint;

    if (cJSON_HasObjectItem(json, "setserver") == 1)
    {
        cJSON *new_server = cJSON_GetObjectItem(json, "setserver");

        setting_new_server(new_server);
        asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
        sleep(5);
        esp_restart();
    }
    else if (cJSON_HasObjectItem(json, "setscan") == 1 && cJSON_GetObjectItem(json, "setscan")->valueint > 0)
    {
        //char ws=0;
        uint8_t u8msg[128] = {0};
        unsigned int cnt = 10;
        //cJSON *item = cJSON_GetObjectItem(json, "value");

        //printf("cnt int %d \n", cJSON_GetObjectItem(item, "setscan")->valueint);

        //cnt = cJSON_GetObjectItem(item, "setscan")->valueint;
        int byteLen = 1;
        u8msg[0] = cnt;
        printf("cnt int %d  %s\n", cnt, u8msg);
        send_msg(5, u8msg, byteLen, NULL);
        asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0); //res scan
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "fdbg") == 1 && strlen(cJSON_GetObjectItem(json, "fdbg")->valuestring) > 5)
    {
        char ws[65] = {0};
        char msg[300] = {0};
        //cJSON *item = cJSON_GetObjectItem(json, "ws");
        //strncpy(ws, item->valuestring, 64);
        //item = cJSON_GetObjectItem(json, "message");
        //strncpy(msg, item->valuestring, 300);
        memset(ws, 'R', 64);
        strncpy(msg, cJSON_GetObjectItem(json, "fdbg")->valuestring, 300);

        printf("ms %s ws %s \n", msg, ws);
        memcpy(rrpc_res_topic, resp_topic, strlen(resp_topic));
        send_msg(2, msg, strlen(msg), ws);
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "reboot") == 1)
    {
        //int reboot_num=cJSON_GetObjectItem(json, "reboot")->valueint;
        char ws[65] = {0};
        char msg[300] = {0};
        //cJSON *item = cJSON_GetObjectItem(json, "ws");
        //strncpy(ws, item->valuestring, 64);
        //item = cJSON_GetObjectItem(json, "message");
        //strncpy(msg, item->valuestring, 300);
        memset(ws, 'U', 64);
        //strncpy(msg, cJSON_GetObjectItem(json, "fdbg")->valuestring, 300);
        msg[0] = cJSON_GetObjectItem(json, "reboot")->valueint;
        printf("ms %d ws %s \n", msg[0], ws);
        //memcpy(rrpc_res_topic, resp_topic, strlen(resp_topic));
        send_msg(99, msg, 1, ws);
        asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "setmeter") == 1 && cJSON_GetObjectItem(cJSON_GetObjectItem(json, "setmeter"), "enb")->valueint > -1)
    {
        cJSON *meter = cJSON_GetObjectItem(json, "setmeter");
        //cJSON_AddNumberToObject(meter, "date", get_current_days());
        //printf("meter set %s \n", cJSON_PrintUnformatted(cJSON_GetObjectItem(json, "value")));
        printf("meter set %s \n", cJSON_PrintUnformatted(meter));
        meter_setdata mt = {0};
        general_query(NVS_METER_SET, &mt);
        getJsonNum(&mt.enb, "enb", meter);
        getJsonNum(&mt.mod, "mod", meter);
        getJsonNum(&mt.reg, "regulate", meter);

        int abs0 = 0;
        int offs = 0;
        getJsonNum(&abs0, "abs", meter);
        printf("abs0: %d\n", abs0);
        getJsonNum(&offs, "offset", meter);
        printf("offs: %d\n", offs);

        if (mt.reg == 10)
        {
            mt.reg = 0X100;
            if (abs0 == 1 && mt.target == 0)
            {
                if (offs <= 0 || offs > 100)
                    offs = 5;

                mt.reg |= offs;
            }
        }

        mt.date = get_current_days();
        write_meter_file(&mt);
        sleep(1);
        send_msg(6, "clearmeter", 9, NULL);
        asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
        return 0;
        //esp_restart();
        //write_meter_file(cJSON_PrintUnformatted(meter));

    }                                                                                                                                      //{"Action":"upgrade","value":{"type":1, "path":{"directory":"http://zevercloud-update.oss-cn-hangzhou.aliyuncs.com/GPRS/20200515-1", "files":["master.bin","slave.bin","safty.bin"]}}}
    else if (cJSON_HasObjectItem(json, "upgrade") == 1 && cJSON_GetObjectItem(cJSON_GetObjectItem(json, "upgrade"), "type")->valueint > 0) //(!(strcmp(function, "upgrade")))
    {

        cJSON *item = cJSON_GetObjectItem(json, "upgrade");

        if (cJSON_HasObjectItem(item, "p2p") == 1)
        {
            g_p2p_mode = cJSON_GetObjectItem(item, "p2p")->valueint;
            printf("p2p %d \n", g_p2p_mode);
        }

        char up_type = cJSON_GetObjectItem(item, "type")->valueint;
        printf("update type %d \n", up_type);

        cJSON *item1 = cJSON_GetObjectItem(item, "path");
        char *url0 = cJSON_GetObjectItem(item1, "directory")->valuestring;
        printf("url %s \n", url0);

        cJSON *item2 = cJSON_GetObjectItem(item1, "files");
        printf("file type %d %d \n", item2->type, cJSON_GetArraySize(item2));
        printf("meter str %s \n", cJSON_GetArrayItem(item2, 0)->valuestring);
        char *p[cJSON_GetArraySize(item2)];

        update_url download_url = {0};

        if (cJSON_GetArraySize(item2) == 1)
        {
            p[0] = cJSON_GetArrayItem(item2, 0)->valuestring;

            if (up_type == 1) //item2->type == 1)
            {
                download_url.update_type = 1;
                sprintf(download_url.down_url, "%s/%s", url0, p[0]);
            }
            else
            {
                download_url.update_type = 2;
                sprintf(download_url.down_url, "%s/%s", url0, p[0]);
            }
        }
        asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0); // res update
        printf("dowan_loadurl %s  %d %d\n", download_url.down_url, download_url.update_type, strlen(download_url.down_url));
        printf("RRPCRESP %s  %s \n", resp_topic, recv_ok);

        TaskHandle_t download_task_handle = NULL;
        BaseType_t xReturned;

        download_task((void *)&download_url);

        // xReturned = xTaskCreate(
        //     download_task,          /* Function that implements the task. */
        //     "download task",        /* Text name for the task. */
        //     10240,                  /* Stack size in words, not bytes. 50K = 25600 */
        //     (void *)&download_url,  /* Parameter passed into the task. */
        //     6,                      /* Priority at which the task is created. */
        //     &download_task_handle); /* Used to pass out the created task's handle. */
        // if (xReturned != pdPASS)
        // {
        //     printf("create cgi task failed.\n");
        // }
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "factory_reset") == 1)
    {
        asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
        factory_initial();
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_systime") == 1)
    {
        char now_timex[30] = {0};
        char ipx[30] = {0};
        char bufx[100] = {0};

        get_time(now_timex, sizeof(now_timex));
        // get_sta_ip(ipx);
        sprintf(bufx, "%s ip:%s ", now_timex, ipx);

        asw_mqtt_publish(resp_topic, (uint8_t *)bufx, strlen(bufx), 0);
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_invsts") == 1)
    {
        char bufx[60] = {0};
        sprintf(bufx, "inverter comm error %d ", inv_comm_error);

        asw_mqtt_publish(resp_topic, (uint8_t *)bufx, strlen(bufx), 0);
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_syslog") == 1)
    {
        char buff[1024] = {0};
        //get_sys_log(bufx);
        get_all_log(buff);
        printf("rrpcsyslog %d--%s \n", strlen(buff), buff);

        if (strlen(buff))
            asw_mqtt_publish(resp_topic, (uint8_t *)buff, strlen(buff), 0);
        else
            asw_mqtt_publish(resp_topic, "no log", strlen("no log"), 0);
        cJSON_Delete(json);
        return 0;
    }
    asw_mqtt_publish(resp_topic, (uint8_t *)recv_er, strlen(recv_er), 0);
    cJSON_Delete(json);
    return -1;
}

int get_systimestr()
{
}