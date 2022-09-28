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
#include "asw_store.h"

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

extern char g_tianhe_host[50];
extern int g_tianhe_port;
extern char g_ccid[22];
int mqtt_app_start(void)
{
    int res = -1;
    ESP_LOGI(TAG, "[mqtt_app_start] Free memory: %d bytes", esp_get_free_heap_size());

    printf("mqtt_app_start: host port: %s %d\n", g_tianhe_host, g_tianhe_port);

    // res = cat1_tcp_conn(g_tianhe_host, g_tianhe_port);
    // res = cat1_tcp_conn("139.224.244.87", 3003);
    // res = cat1_tcp_conn("psinf.trinasolar.com", 3003);
    res = cat1_tcp_conn("iotgw.trinablue.com", 3003);
    // res = cat1_tcp_conn("139.224.244.87", 3013);
    if (res != 0)
    {
        return -1;
    }

    login_para_t login_para = {0};
    char *mfr = "TE";
    //char *type = "type";
    char *iccid = g_ccid;
    //char *pmu_ver = "1.0.0";
    char GB = 0x01;
    memcpy(login_para.mfr, mfr, strlen(mfr));
    //memcpy(login_para.type, type, strlen(type));
    get_tianhe_dev_type(login_para.type);
    memcpy(login_para.iccid, iccid, strlen(iccid));
    //memcpy(login_para.pmu_ver, pmu_ver, strlen(pmu_ver));
    login_para.GB = 0x01;
    res = tianhe_login(login_para);
    if (res != 0)
        return -1;
    else
    {
        uint32_t rd_var = 0;
        rd_var = get_tianhe_reg_state();
        if (rd_var != 0xAABBCCDD)
        {
            uint32_t wr_var = 0xAABBCCDD;
            set_tianhe_reg_state(&wr_var);
            printf("write 0xAABBCCDD\n");
        }
        else
        {
            printf("already 0xAABBCCDD\n");
        }
    }

    cat1_set_tcp_ok();
    res = tianhe_get_server_time();
    if (res != 0)
        return -1;
    else
    {
        goto_set_inv_time();
    }

    res = tianhe_keepalive();
    if (res != 0)
        return -1;

    return 0;
}

// void setting_new_server(cJSON *json)
// {
//     Setting_Para setting = {0};
//     if (json == NULL)
//         return;
//     int res = general_query(NVS_ATE, &setting);
//     if (res == 0)
//     {
//         memset(&setting.host, 0, sizeof(setting.host));
//         memset(&setting.product_key, 0, sizeof(setting.product_key));
//         memset(&setting.device_secret, 0, sizeof(setting.device_secret));

//         getJsonStr(setting.host, "host", sizeof(setting.host), json);
//         getJsonStr(setting.product_key, "pdk", sizeof(setting.product_key), json);
//         getJsonStr(setting.device_secret, "sec", sizeof(setting.device_secret), json);

//         general_add(NVS_ATE, &setting);
//     }
// }

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
// int asw_mqtt_publish(const char *topic, const char *data, int data_len, int qos)
// {
//     cat1_tcp_send(data, data_len);
//     // ESP_LOGI(TAG, "published, msg_id=%d ************", msg_id);
//     return 0;
// }

// int asw_publish(void *cpayload)
// {
//     int res = 0;
//     char *payload = (char *)cpayload;

//     // asw_mqtt_publish(topic, payload, strlen(payload), ini_para.mqtt_qos);
//     res = cat1_tcp_send(0, pub_topic, payload);

//     if (res < 0)
//     {
//         printf("publish failed, res = %d", res);
//         return -1;
//     }
//     return 0;
// }

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

int parse_mqtt_msg_aliyun(char *rpc_topic, int rpc_len, void *payload, int data_len)
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
        aliyun_publish(resp_topic, (uint8_t *)recv_er, strlen(recv_er), 0);
        cJSON_Delete(json);
        return -1;
    }
    printf("RRPC ACTION STR \n"); // item = cJSON_GetObjectItem(root, "semantic");
    //char *function = cJSON_GetObjectItem(json, "setscan")->valueint;

    if (cJSON_HasObjectItem(json, "setscan") == 1 && cJSON_GetObjectItem(json, "setscan")->valueint > 0)
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
        aliyun_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0); //res scan
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "fdbg") == 1 && strlen(cJSON_GetObjectItem(json, "fdbg")->valuestring) > 5)
    {
        char ws[65] = {0};
        char msg[300] = {0};
        g_fdbg_mqtt_index = 1;
        //cJSON *item = cJSON_GetObjectItem(json, "ws");
        //strncpy(ws, item->valuestring, 64);
        //item = cJSON_GetObjectItem(json, "message");
        //strncpy(msg, item->valuestring, 300);

        memset(ws, 'R', 64);
        strncpy(msg, cJSON_GetObjectItem(json, "fdbg")->valuestring, 300);

        event_group_0 |= ALIYUN_FDBG_MASK;
        if (get_tianhe_onoff() == 0)
        {
            if (is_found_onoff_cmd(msg, strlen(msg)))
            {
                char *rmsg = "{\"dat\":\"err\",\"msg\":\"no response\"}";
                aliyun_publish(resp_topic, (uint8_t *)rmsg, strlen(rmsg), 0);
                cJSON_Delete(json);
                event_group_0 &= ~ALIYUN_FDBG_MASK;
                return 0;
            }
        }

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
        aliyun_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
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
        mt.enb = cJSON_GetObjectItem(meter, "enb")->valueint;
        mt.mod = cJSON_GetObjectItem(meter, "mod")->valueint;
        mt.reg = cJSON_GetObjectItem(meter, "regulate")->valueint;
        mt.target = cJSON_GetObjectItem(meter, "target")->valueint;
        mt.date = get_current_days();
        write_meter_file(&mt);
        sleep(1);
        send_msg(6, "clearmeter", 9, NULL);
        aliyun_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
        cJSON_Delete(json);
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
        else
        {
            g_p2p_mode = 0;
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
        aliyun_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0); // res update
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
        aliyun_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
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

        aliyun_publish(resp_topic, (uint8_t *)bufx, strlen(bufx), 0);
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_invsts") == 1)
    {
        char bufx[60] = {0};
        sprintf(bufx, "inverter comm error %d ", inv_comm_error);

        aliyun_publish(resp_topic, (uint8_t *)bufx, strlen(bufx), 0);
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
            aliyun_publish(resp_topic, (uint8_t *)buff, strlen(buff), 0);
        else
            aliyun_publish(resp_topic, "no log", strlen("no log"), 0);
        cJSON_Delete(json);
        return 0;
    }
    // else if (cJSON_HasObjectItem(json, "setserver") == 1)
    // {
    //     cJSON *new_server = cJSON_GetObjectItem(json, "setserver");

    //     setting_new_server(new_server);
    //     aliyun_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
    //     sleep(5);
    //     esp_restart();
    // }

    aliyun_publish(resp_topic, (uint8_t *)recv_er, strlen(recv_er), 0);
    cJSON_Delete(json);
    return -1;
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

void tianhe_fdbg_send(char *msg, int len)
{
    char ws[65] = {0};
    g_fdbg_mqtt_index = 0;
    memset(ws, 'R', 64);
    printf("ms %s ws %s \n", msg, ws);
    send_msg(2, msg, len, ws);
    return 0;
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
        // asw_mqtt_publish(resp_topic, (uint8_t *)recv_er, strlen(recv_er), 0);
        cJSON_Delete(json);
        return -1;
    }
    printf("RRPC ACTION STR \n"); // item = cJSON_GetObjectItem(root, "semantic");
    //char *function = cJSON_GetObjectItem(json, "setscan")->valueint;

    if (cJSON_HasObjectItem(json, "setscan") == 1 && cJSON_GetObjectItem(json, "setscan")->valueint > 0)
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
        // asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0); //res scan
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
        // asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
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
        mt.enb = cJSON_GetObjectItem(meter, "enb")->valueint;
        mt.mod = cJSON_GetObjectItem(meter, "mod")->valueint;
        mt.reg = cJSON_GetObjectItem(meter, "regulate")->valueint;
        mt.target = cJSON_GetObjectItem(meter, "target")->valueint;
        mt.date = get_current_days();
        write_meter_file(&mt);
        sleep(1);
        send_msg(6, "clearmeter", 9, NULL);
        // asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
        cJSON_Delete(json);
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
        else
        {
            g_p2p_mode = 0;
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
        // asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0); // res update
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
        // asw_mqtt_publish(resp_topic, (uint8_t *)recv_ok, strlen(recv_ok), 0);
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

        // asw_mqtt_publish(resp_topic, (uint8_t *)bufx, strlen(bufx), 0);
        cJSON_Delete(json);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_invsts") == 1)
    {
        char bufx[60] = {0};
        sprintf(bufx, "inverter comm error %d ", inv_comm_error);

        // asw_mqtt_publish(resp_topic, (uint8_t *)bufx, strlen(bufx), 0);
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
            ;
        // asw_mqtt_publish(resp_topic, (uint8_t *)buff, strlen(buff), 0);
        else
            // asw_mqtt_publish(resp_topic, "no log", strlen("no log"), 0);
            ;
        cJSON_Delete(json);
        return 0;
    }
    // asw_mqtt_publish(resp_topic, (uint8_t *)recv_er, strlen(recv_er), 0);
    cJSON_Delete(json);
    return -1;
}

int aliyun_publish(const char *topic, const char *data, int data_len, int qos)
{
    cat1_mqtt_pub_qos_0(1, topic, data);
    // ESP_LOGI(TAG, "published, msg_id=%d ************", msg_id);
    return 0;
}

/** add aliyun para check, if not exist, get from net **********************************/

void http_get_aliyun_para(void)
{
    char *url_fmt = "http://mqtt.aisweicloud.com/api/v1/getODMStickInfo?deviceName=%s&sign=%s&productKeyType=1";
    int res;
    char buf[200] = {0};
    char url[200] = {0};
    char sign[17] = {0};
    Setting_Para setting = {0};
    cJSON *json = NULL;

    general_query(NVS_ATE, &setting);
    if (strlen(setting.psn) <= 0)
        return;
    sn_to_reg(setting.psn, sign);
    sprintf(url, url_fmt, setting.psn, sign);

    res = cat1_http_get(url, buf);
    if (res != 0)
    {
        return;
    }
    //parse json
    json = cJSON_Parse(buf);
    if (json != NULL)
    {
        if (cJSON_HasObjectItem(json, "deviceSecret") == 1)
        {
            char *pdk = "a1OfkFJlLQc";
            memcpy(setting.product_key, pdk, strlen(pdk));
            getJsonStr(setting.device_secret, "deviceSecret", sizeof(setting.device_secret), json);
            general_add(NVS_ATE, &setting);
        }
    }

    if (json != NULL)
        cJSON_Delete(json);
}

int aliyun_para_exist_check(void)
{
    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);
    if (strlen(setting.device_secret) > 0 && strlen(setting.product_key) > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

void aliyun_para_check(void)
{
    if (aliyun_para_exist_check() == 0) /** yes, exist*/
        return;
    else
    {
        /** get aliyun para by http request */
        http_get_aliyun_para();
    }
}
/** **********************************************************************************************/