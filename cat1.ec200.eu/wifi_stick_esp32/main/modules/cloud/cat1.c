#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "stdint.h"
#include "inv_com.h"
#include "cat1.h"
#include "data_process.h"
#include "asw_store.h"

#define FAST_LOG_EN 1
#define MQTT_CA_FILE "-----BEGIN CERTIFICATE-----\n"                                      \
                     "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n" \
                     "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n" \
                     "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n" \
                     "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n" \
                     "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n" \
                     "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n" \
                     "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n" \
                     "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n" \
                     "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n" \
                     "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n" \
                     "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n" \
                     "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n" \
                     "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n" \
                     "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n" \
                     "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n" \
                     "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n" \
                     "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n" \
                     "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n" \
                     "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"                             \
                     "-----END CERTIFICATE-----"

/** data call commands *******************************************************************************/
#define at "AT"
#define at_disable_echo "ATE0"
#define at_reboot "AT+CFUN=1,1"

#define at_cereg "AT+CEREG?"
#define at_cgreg "AT+CGREG?"

#define at_csq "AT+CSQ"
#define at_net_info "AT+COPS?" //CMCC, 4G/2G

#define at_set_apn "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\""
#define at_data_call "AT+QIACT=1"
#define at_data_call_clear "AT+QIDEACT=1"
#define at_check_data_call "AT+QIACT?"

#define at_qccid "AT+QCCID"
#define at_imei "AT+GSN"
#define at_imsi "AT+CIMI"

/** mqtt commands *******************************************************************************/
//if upload fail, that you have uploaded yet.
#define at_upload_ca_cert "AT+QFUPL=\"UFS:%s\",%d,5" //cacert.pem
#define at_del_ca_cert "AT+QFDEL=\"UFS:%s\""         //cacert.pem
#define at_read_ca_cert "AT+QFDWL=\"UFS:%s\""        //cacert.pem
#define at_ls "AT+QFLST"

static char *ssl_name_tab[2] = {"cacert.pem", "cacert1.pem"};

#define at_set_recv_mode "AT+QMTCFG=\"recv/mode\",%d,1,1"                         //idx=1
#define at_set_ssl "AT+QMTCFG=\"SSL\",%d,%d,%d"                                   //idx=1 ssl_idx=2
#define at_set_ca_cert "AT+QSSLCFG=\"cacert\",%d,\"UFS:%s\""                      //ssl_idx=2 cacert.pem
#define at_set_ssl_level "AT+QSSLCFG=\"seclevel\",%d,1"                           //0:no, 1:server, 2:server&client                                   //ssl_idx=2
#define at_ssl_ignore_valid "AT+QSSLCFG=\"ignoreinvalidcertsign\",%d,1"           //ssl idx
#define at_set_ssl_version "AT+QSSLCFG=\"sslversion\",%d,4"                       //ssl_idx=2
#define at_set_ciphersuite "AT+QSSLCFG=\"ciphersuite\",%d,0xFFFF"                 //ssl_idx=2
#define at_ssl_ignore_ltime "AT+QSSLCFG=\"ignorelocaltime\",%d,1"                 //ssl_idx=2
#define at_set_keepalive "AT+QMTCFG=\"keepalive\",%d,180"                         //idx
#define at_mqtt_open_aiswei "AT+QMTOPEN=%d,\"%s.iot-as-mqtt.%s.aliyuncs.com\",%d" //idx
#define at_mqtt_open "AT+QMTOPEN=%d,\"%s\",%d"                                    //idx url
#define at_mqtt_connect "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\""                      //idx
#define at_mqtt_sub "AT+QMTSUB=%d,123,\"%s\",0"                                   //idx
#define at_mqtt_pub "AT+QMTPUBEX=%d,%d,1,0,\"%s\",%d"                             //idx
#define at_mqtt_disc "AT+QMTDISC=%d"                                              //idx

#define at_mqtt_recv_check "AT+QMTRECV?"
#define at_mqtt_recv_read "AT+QMTRECV=%d,%d" //0~4 //idx=fst

/** http commands ************************************************************************************/
#define at_http_response_head_off "AT+QHTTPCFG=\"responseheader\",0"
#define at_http_response_head_on "AT+QHTTPCFG=\"responseheader\",1"
#define at_http_request_head_off "AT+QHTTPCFG=\"requestheader\",0"
#define at_http_request_head_on "AT+QHTTPCFG=\"requestheader\",1"

#define at_http_id "AT+QHTTPCFG=\"contextid\",1"
#define at_http_url "AT+QHTTPURL=%d,5" //len
#define at_http_get_action "AT+QHTTPGET=20"
#define at_http_user_get "AT+QHTTPGET=60,%d" //request head size
#define at_http_read "AT+QHTTPREAD=5"

#define at_http_release "AT+QHTTPSTOP"

#define at_simple_http_set_res "AT+QHTTPCFG=\"responseheader\",0"
#define at_simple_http_set_req "AT+QHTTPCFG=\"requestheader\",0"
#define at_simple_http_get_action "AT+QHTTPGET=30"

//segment read file
#define at_fdel_getfile "AT+QFDEL=\"getfile\""   //getfile
#define at_fopen_getfile "AT+QFOPEN=\"getfile\"" //return index
#define at_fread_getfile "AT+QFREAD=%d,%d"       // index, length
#define at_fclose_getfile "AT+QFCLOSE=%d"
#define RANGE_HEAD_FMT "GET %s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n"
#define KB_N 5
/** gps commands ************************************************************************************/
#define at_gps_open "AT+QGPS=1"
#define at_gps_read "AT+QGPSLOC=0"
#define at_gps_close "AT+QGPSEND"

#define at_low_power "AT+CFUN=0"
/** *************************************************************************************************/

//file sys
#define at_fs_mem "AT+QFLDS=\"UFS\""
#define at_fs_ls "AT+QFLST"

#define CAT1_MSG_TYPE 1

#define RAW "\033[0m"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"

enum Module_Type
{
    QUECTEL_EC600,
    QUECTEL_EC200,
};

char log2cld_gps = 0;

static int g_data_call_ok = 0;
static int g_mqtt_ok[2] = {0};
static int g_cereg_ok = 0;
static int g_cgreg_ok = 0;
static int g_net_mode = -1;
static int g_csq = -1;

static int is_downloading = 0;
char g_mqtt_pdk[32] = {0};
char g_module_type = 0;

/** 用于LED灯、重新拨号、MQTT重连*/
int cat1_get_csq(void)
{
    return g_csq;
}

int cat1_get_cereg_status(void)
{
    return g_cereg_ok;
}

int cat1_get_cgreg_status(void)
{
    return g_cgreg_ok;
}

int cat1_get_net_mode_value(void)
{
    return g_net_mode;
}

void cat1_set_data_call_ok(void)
{
    g_data_call_ok = 1;
}

void cat1_set_data_call_error(void)
{
    g_data_call_ok = 0;
}

void cat1_set_mqtt_ok(int index)
{
    g_mqtt_ok[index] = 1;
}

void cat1_set_mqtt_error(int index)
{
    g_mqtt_ok[index] = 0;
}

int cat1_get_data_call_status(void)
{
    return g_data_call_ok;
}

int cat1_get_mqtt_status(int index)
{
    return g_mqtt_ok[index];
}

void set_print_color(char *color)
{
    printf("%s", color);
}

void set_print_blue(void)
{
    set_print_color(BLUE);
}

void red_log(char *msg)
{
    set_print_color(RED);
    printf("%s\n", msg);
    set_print_color(BLUE);
}

void yellow_log(char *msg)
{
    set_print_color(YELLOW);
    printf("%s\n", msg);
    set_print_color(BLUE);
}

/** uart send and recv log*/
static int frame_recv(uint8_t *buf, int buf_size)
{
    // char buf[1536] = {0};
    int len = 0;
    int nread = 0;
    int timeout_count = 0;
    int nleft = buf_size;

    while (1)
    {
        usleep(50000);
        timeout_count++;
        if (timeout_count > 20) /** 20 = 1 second*/
        {
            return 0;
        }

        nleft = buf_size - len - 1;
        nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
        if (nread > 0)
        {
            len += nread;
            break;
        }
    }

    while (1)
    {
        usleep(50000);
        nleft = buf_size - len - 1;
        if (nleft > 0)
        {
            nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
            if (nread > 0)
            {
                len += nread;
                continue;
            }
        }

        // memcpy(_buf, buf, len);
        return len;
    }
}

static int frame_recv_1ms(uint8_t *buf, int buf_size)
{
    // char buf[1536] = {0};
    int len = 0;
    int nread = 0;
    int timeout_count = 0;
    int nleft = buf_size;

    while (1)
    {
        usleep(50000);
        timeout_count++;
        if (timeout_count > 60) /** 20 = 1 second*/
        {
            return 0;
        }

        nleft = buf_size - len - 1;
        nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
        if (nread > 0)
        {
            len += nread;
            break;
        }
    }

    while (1)
    {
        usleep(1000);
        nleft = buf_size - len - 1;
        if (nleft > 0)
        {
            nread = uart_read_bytes(CAT1_UART, buf + len, nleft, 0);
            if (nread > 0)
            {
                len += nread;
                continue;
            }
        }

        // memcpy(_buf, buf, len);
        return len;
    }
}

/** 接收一帧，并过滤检查通知类信息*/
static int frame_filter(char *buf, int buf_size)
{
    // char buf[1536] = {0};
    // at_response_msg_t msg = {0};
    char *p1;
    char *p2;
    int recv_len = frame_recv(buf, buf_size);
    if (recv_len > 0)
    {
        if (is_downloading == 0)
        {
            if (FAST_LOG_EN == 1)
            {
                if (strncmp(buf, "\r\n+QMTRECV: 0,", strlen("\r\n+QMTRECV: 0,")) != 0)
                    printf("RECV FRAME: %d <RECV FRAME>%.*s</RECV FRAME>\n", recv_len, recv_len, buf);
            }

            /** MQTT通知*/
            char *mqtt_tag = "\r\n+QMTSTAT:";
            p1 = memmem(buf, recv_len, mqtt_tag, strlen(mqtt_tag));
            if (p1 != NULL)
            {
                int mqtt_urc = 0;
                int mqtt_idx = 0;
                printf("FRAME FILTER: found +QMTSTAT *******************\n");
                printf("%s\n", p1);
                int n_parsed = sscanf(p1, "\r\n+QMTSTAT: %d,%d\r\n", &mqtt_idx, &mqtt_urc);
                if (n_parsed == 2 && mqtt_urc > 0)
                {
                    cat1_set_mqtt_error(mqtt_idx);
                }
            }

            char *start_tag = "\r\nRDY\r\n";
            p1 = memmem(buf, recv_len, start_tag, strlen(start_tag));
            if (p1 != NULL)
            {
                printf("FRAME FILTER: found +RDY *******************\n");
                sleep(1);
                int res = cat1_echo_disable();
                printf("\nRDY, disable AT echo ok, res=%d**************************************+++\n", res);
            }
        }

        // /** 驻网信息，不用管*/
        // if (strstr(buf, "\r\n+CGREG:") != NULL)
        // {
        //     ;
        // }

        // memcpy(msg, buf, recv_len);
        return recv_len;
    }
    else
    {
        return 0;
    }
}

static int at_recv(char *buf, int buf_size)
{
    return frame_filter(buf, buf_size);
}

static void at_send(char *cmd)
{
    char buf[1536] = {0};
    int len = strlen(cmd);
    memcpy(buf, cmd, len);
    memcpy(buf + len, "\r", 1);
    len = len + 1;

    uart_flush(CAT1_UART);
    uart_write_bytes(CAT1_UART, buf, len);
    if (FAST_LOG_EN == 1)
    {
        if (strncmp(buf, "AT+QMTRECV?", strlen("AT+QMTRECV?")) != 0)
            printf("SERIAL AT SEND: %s\n", buf);
    }
}

static void raw_send(char *cmd, int len)
{
    uart_flush(CAT1_UART);
    uart_write_bytes(CAT1_UART, cmd, len);
    printf("SERIAL SEND: %s\n", cmd);
}

/** 接收AT指令回复，匹配关键字，10秒超时*/
//-0: ok
//-1: error
//-2: timeout
static int multi_keyword_check(int argc, char **argv)
{
    char buf[1536] = {0};
    int size = 0;
    static int index = 0;
    int i = 0;

    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec <= 3)
    {
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            for (i = 0; i < argc; i++)
            {
                if (strstr(buf, argv[i]) != NULL)
                {
                    int res = 0;
                    if (i == 0)
                    {
                        res = 0;
                    }
                    else
                    {
                        res = -1;
                    }
                    set_print_color(YELLOW);
                    printf("keyword check ok: %s --------------------(%d)\n", argv[i], index++);
                    set_print_color(BLUE);
                    return res;
                }
            }
        }
    }
    set_print_color(RED);
    printf("keyword check timeout: ");
    for (i = 0; i < argc; i++)
    {
        printf("%s, ", argv[i]);
        printf("\n");
    }
    set_print_color(BLUE);
    // exit(0);
    return -2;
}

static int multi_keyword_sec_check(int argc, char **argv, int sec)
{
    char buf[1536] = {0};
    int size = 0;
    static int index = 0;
    int i = 0;

    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec <= sec)
    {
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            for (i = 0; i < argc; i++)
            {
                if (strstr(buf, argv[i]) != NULL)
                {
                    int res = 0;
                    if (i == 0)
                    {
                        res = 0;
                    }
                    else
                    {
                        res = -1;
                    }
                    set_print_color(YELLOW);
                    printf("keyword check ok: %s --------------------(%d)\n", argv[i], index++);
                    set_print_color(BLUE);

                    if (log2cld_gps == 1)
                    {
                        log2cld_gps = 0;
                        if (cat1_get_mqtt_status(0) == true)
                            asw_publish(buf);
                    }
                    return res;
                }
            }
        }
    }
    set_print_color(RED);
    printf("keyword check timeout: ");
    for (i = 0; i < argc; i++)
    {
        printf("%s, ", argv[i]);
        printf("\n");
    }
    set_print_color(BLUE);
    log2cld_gps == 0;
    // exit(0);
    return -2;
}

static int keyword_check(char *keyword)
{
    char *s1[] = {keyword, "ERROR"};
    return multi_keyword_check(2, s1);
}

static int keyword_sec_check(char *keyword, int sec)
{
    char *s1[] = {keyword, "ERROR"};
    return multi_keyword_sec_check(2, s1, sec);
}

char g_ip[17] = {0};
static int get_data_call_ip(char *ip)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QIACT:";
    //3 = 3 sec
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        printf("get_data_call_ip: count=%d size=%d %s\n", count, size, buf);
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int state = -1;
                char my_ip[17] = {0};
                int num = sscanf(p1, "+QIACT: %*d,%d,%*d,\"%[^\"]", &state, my_ip);
                printf("22222222222222222222222sss\n");
                if (num == 2)
                {
                    if (state == 1)
                    {
                        if (ip != NULL)
                        {
                            memcpy(ip, my_ip, strlen(my_ip));
                        }
                        memcpy(g_ip, my_ip, strlen(my_ip));

                        set_print_color(YELLOW);
                        printf("data call already ok, ip: %s\n", my_ip);
                        set_print_color(BLUE);
                        return 0;
                    }
                    else
                    {
                        printf("data call loss\n");
                        at_send(at_data_call_clear);
                        keyword_check("OK");
                        return -1;
                    }
                }
                else
                {
                    printf("3333333333333333333333333ssss\n");
                    return -1;
                }
            }
            else
            {
                printf("11111111111111111111111111111sss\n");
                p1 = strstr(buf, "OK");
                if (p1 != NULL)
                    return -1;
            }
        }
    }

    set_print_color(RED);
    printf("data call state check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int revision_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int nread = 0;
    int count = 0;
    char *keyword = "Revision:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        nread = at_recv(buf + size, sizeof(buf) - size);
        if (nread > 0)
        {
            size += nread;
            char *p1 = strstr(buf, keyword);
            char *p2 = strstr(buf, "OK");
            if (p1 != NULL && p2 != NULL)
            {
                if (strstr(buf, "EC200") != NULL)
                {
                    g_module_type = QUECTEL_EC200;
                    yellow_log("EC200 found");
                }
                else
                {
                    g_module_type = QUECTEL_EC600;
                    yellow_log("EC600 found");
                }

                return 0;
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("revision_check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

int cat1_low_power(void)
{
    at_send(at_low_power);
    return keyword_sec_check("OK", 5);
}

int cat1_gps_open(void)
{
    at_send(at_gps_open);
    return keyword_sec_check("OK", 5);
}

int cat1_gps_close(void)
{
    at_send(at_gps_close);
    return keyword_sec_check("OK", 40);
}

int cat1_gps_read(void)
{
    at_send(at_gps_read);
    log2cld_gps = 1;
    return keyword_sec_check("OK", 5);
}

void gps_test(void)
{
    // cat1_low_power();
    cat1_gps_open();
    sleep(5);
    cat1_gps_read();
    cat1_gps_close();
}

static int csq_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+CSQ:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int csq = -1;
                int num = sscanf(p1, "\r\n+CSQ: %d,", &csq);
                if (num == 1 && csq >= 0 && csq <= 31)
                {
                    set_print_color(YELLOW);
                    printf("CSQ = %d **************\n", csq);
                    set_print_color(BLUE);
                    return csq;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("csq check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int cereg_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+CEREG:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int reg = -1;
                int num = sscanf(p1, "\r\n+CEREG: %*d,%d", &reg);
                if (num == 1 && (reg == 1 || reg == 5))
                {
                    set_print_color(YELLOW);
                    printf("CEREG OK: stat=%d **************\n", reg);
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("cereg check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int cgreg_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+CGREG:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int reg = -1;
                int num = sscanf(p1, "\r\n+CGREG: %*d,%d", &reg);
                if (num == 1 && (reg == 1 || reg == 5))
                {
                    set_print_color(YELLOW);
                    printf("CGREG OK: stat=%d **************\n", reg);
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("cgreg check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int net_info_check(char *oper, int *mode)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+COPS:";

    /** 1 = 1 sec*/
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int my_mode = -1;
                char my_oper[30] = {0}; //
                int num = sscanf(p1, "%*[^\"]\"%[^\",]\",%d", my_oper, &my_mode);
                if (num == 2)
                {
                    if (oper != NULL)
                        memcpy(oper, my_oper, strlen(my_oper));
                    *mode = my_mode;
                    set_print_color(YELLOW);
                    printf("OPER: %s ***\n", my_oper);
                    printf("NET_MODE: %d ***\n", my_mode);
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("net_info_check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int mqtt_sub_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "+QMTSUB: ";

    //1 = 1 sec
    while (count < 5)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int result = -1;
                int num = sscanf(p1, "+QMTSUB: %*d,%*d,%d", &result);
                if (num == 1 && result == 0)
                {
                    set_print_color(YELLOW);
                    printf("mqtt sub check ok\n");
                    set_print_color(BLUE);
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("mqtt sub check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int mqtt_pub_check(uint16_t *_msg_id)
{
    char buf[1536] = {0};
    int size = 0;
    char *keyword = "+QMTPUBEX: ";

    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec <= 15) //5sec * 3times = 15sec
    {
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = NULL;
            p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                uint16_t msg_id = 0;
                int pub_res = -1;

                int num = sscanf(p1, "+QMTPUBEX: %*d,%hu,%d", &msg_id, &pub_res);
                if (num == 2)
                {
                    if (pub_res == 0 || pub_res == 1)
                    {
                        set_print_color(YELLOW);
                        printf("mqtt pub check ok: msg_id=%hu, return=%d\n", msg_id, pub_res);
                        *_msg_id = msg_id;
                        set_print_color(BLUE);
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("mqtt pub check timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

static int get_mqtt_recv_index(int *index)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    int row = 0;
    char *keyword = "\r\n+QMTRECV:";

    //1 = 1 sec
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = buf;
            while (1)
            {
                p1 = strstr(p1, keyword);
                if (p1 != NULL)
                {
                    int arr[5] = {0};
                    int num = sscanf(p1, "\r\n+QMTRECV: %d,%d,%d,%d,%d,%d", index, &arr[0], &arr[1], &arr[2], &arr[3], &arr[4]);
                    if (num == 6)
                    {
                        int i = 0;
                        for (i = 0; i < 5; i++)
                        {
                            if (arr[i] == 1)
                                return i;
                            if (i == 4) // 1st row no msg, search 2nd row
                            {
                                // printf("mqtt recv check, row=%d, no msg ***************\n", row);
                                row++;
                                p1 = p1 + strlen(keyword);
                                continue;
                            }
                        }
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("mqtt_recv_check_index timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int mqtt_recv_parse(int index)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QMTRECV:";

    //1 = 1 sec
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                char topic[200] = {0};
                char payload[1536] = {0};
                int parsed_num = 0;

                parsed_num = sscanf(buf, "%*[^\"]\"%[^\",]\",%*d,\"%s\"", topic, payload);
                if (parsed_num == 2)
                {
                    set_print_color(BLUE);
                    printf("sub recv sub topic: %s\n", topic);
                    memset(payload + strlen(payload) - 1, 0, 1); //remove last \"
                    printf("sub recv payload: %s\n", payload);
                    set_print_color(BLUE);

                    /** 根据topic类型分别处理*/
                    if (index == 0 || index == 1)
                    {
                        if (strstr(topic, "rrpc") != NULL)
                        {
                            parse_mqtt_msg_rrpc(topic, strlen(topic), payload, strlen(payload));
                            if (strstr(payload, "\"fdbg\"") != NULL)
                                return 222;
                            return 0;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
    }
    set_print_color(RED);
    printf("mqtt_recv_parse timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int http_action_head_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QHTTPGET: ";

    //1 = 1 sec
    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec < 15)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int body_len = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, "\r\n+QHTTPGET: 0,200,%d", &body_len);
                if (parsed_num == 1)
                {
                    set_print_color(YELLOW);
                    printf("http_action_head_check ok, content lenth = %d\n", body_len);
                    set_print_color(BLUE);
                    return body_len;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("http_action_head_check timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int fopen_getfile_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QFOPEN:";

    //1 = 1 sec
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int fd = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, "\r\n+QFOPEN: %d", &fd);
                if (parsed_num == 1)
                {
                    set_print_color(YELLOW);
                    printf("fopen_getfile_check ok, file fd = %d\n", fd);
                    set_print_color(BLUE);
                    return fd;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("fopen_getfile_check timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int http_resp2file_check(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QHTTPREADFILE:";

    //1 = 1 sec
    // cat1_fs_ls();
    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec < 33)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, "\r\n+QHTTPREADFILE: %d", &res);
                if (parsed_num == 1 && res == 0)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("http_save2file_check timeout\n");
    set_print_color(BLUE);
    cat1_fs_ls();
    return -1;
}

int cat1_mqtt_recv(void)
{
    if (cat1_get_mqtt_status(0) == 0 && cat1_get_mqtt_status(0) == 0)
        return -1;
    at_send(at_mqtt_recv_check);
    int mqtt_index = -1;
    int index = get_mqtt_recv_index(&mqtt_index);
    if (index < 0 || index > 4 || mqtt_index > 1 || mqtt_index < 0)
        return -1;
    if (g_module_type == QUECTEL_EC200)
        index = 4 - index; /** 2022.5.19 */
    char cmd[100] = {0};
    sprintf(cmd, at_mqtt_recv_read, mqtt_index, index);
    at_send(cmd);
    return mqtt_recv_parse(mqtt_index);
}

static int http_clear(void)
{
    int res = -1;
    at_send(at_http_release);
    res = keyword_check("OK");
    if (res == 0 || res == -1)
        return 0;
    else
        return -1;
}

static int ccid_check(char *ccid)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QCCID:";

    //1 = 1 sec
    while (count < 2)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, "\r\n+QCCID: %s", ccid);
                if (parsed_num == 1)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("ccid_check timeout\n");
    set_print_color(BLUE);
    return -1;
}

static int imei_or_imsi_check(char *data)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n";

    //1 = 1 sec
    while (count < 2)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int res = 0;
                int parsed_num = 0;

                parsed_num = sscanf(p1, "\r\n%s", data);
                if (parsed_num == 1)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("imei_or_imsi_check timeout\n");
    set_print_color(BLUE);
    return -1;
}

int cat1_get_ccid(char *ccid)
{
    int res = -1;
    at_send(at_qccid);
    res = ccid_check(ccid);
    return res;
}

int cat1_get_imei(char *imei)
{
    int res = -1;
    at_send(at_imei);
    res = imei_or_imsi_check(imei);
    return res;
}

int cat1_get_imsi(char *imsi)
{
    int res = -1;
    at_send(at_imsi);
    res = imei_or_imsi_check(imsi);
    return res;
}

// void hex_print_frame(uint8_t *buf, int len)
// {
//     if (len <= 0)
//         return;
//     int i = 0;
//     printf("\n<HEX>\n");
//     for (int i = 0; i < len; i++)
//         printf("%02X ", (uint8_t)buf[i]);
//     printf("\n</HEX>\n");
// }

// void file_content_check(FILE *fp, int offset, int len, uint8_t *pp)
// {
//     int res = 0;
//     int j = 0;
//     uint8_t buf[5121] = {0};
//     fseek(fp, (long)offset, SEEK_SET);
//     res = fread(buf, 1, len, fp);
//     if (res != len)
//     {
//         fclose(fp);
//         red_log("fread error");
//         sleep(3600);
//     }

//     for (j = 0; j < len; j++)
//     {
//         if (*(buf + j) != *(pp + j))
//         {
//             red_log("[ERR] download content:");
//             printf("offset+j: %d\n", offset + j);
//             int this_len = len - j > 100 ? 100 : len - j;
//             set_print_color(GREEN);
//             printf("%.*s\n", this_len, buf + j);
//             hex_print_frame(buf + j, this_len);
//             set_print_color(RED);

//             hex_print_frame(pp + j, this_len);
//             printf("%.*s\n", this_len, pp + j);
//             set_print_color(BLUE);
//             fclose(fp);
//             sleep(3600);
//         }
//     }
// }
/** 读取一次GET响应，并解析有效数据*/
static int http_read_file_check(int *next_begin_addr, int *next_end_addr, char *file_name, int urc_body_size)
{
    char buf[8192] = {0};
    int size = 0;
    // int count = 0;
    static int index = 0;
    int body_len = 0;
    int body_begin_count = 0;
    int body_end_count = 0;
    int file_total_len = 0;
    static int current_len;
    static FILE *fp = NULL;
    int urc_len = 0;
    int nwrite = 0;
    int add_size = 0;
    int double_newline_has_found = 0; //flag
    char *pp1 = NULL;

    //200 = 10 sec
    int start_sec = get_second_sys_time();
    memset(buf, 0, sizeof(buf));
    size = 0;
    /** 完整接收响应，不遗漏*/
    while (1)
    {
        if (get_second_sys_time() - start_sec > 10)
            return -1;
        usleep(50000);
        add_size = at_recv(buf + size, sizeof(buf) - size - 1);
        if (add_size > 0)
        {
            size += add_size;
            printf("add size: %d ****\n", add_size);
            printf("<RECV>%.*s</RECV>\n", size, buf);
            if (double_newline_has_found == 0)
            {
                pp1 = memmem(buf, size, "\r\n\r\n", 4);
                if (pp1 != NULL)
                {
                    pp1 += 4;
                    double_newline_has_found = 1;
                }
            }
            if (double_newline_has_found == 1)
            {
                if (buf + size - pp1 >= urc_body_size + 23) //+23 tail
                {
                    printf("<RECV>%.*s</RECV>\n", size, buf);
                    break;
                }
            }
        }
    }

    /** 处理接收内容--------------------------------------------------------------*/
    if (strstr(buf, "HTTP/1.1 206 Partial Content") != NULL)
    {
        set_print_color(YELLOW);
        printf("http recv file check ok --------------------(%d)\n", index++);
        set_print_color(BLUE);

        /** 提取body长度*/
        char *p1 = strstr(buf, "Content-Length: ");
        if (p1 != NULL)
        {
            red_log("+++++++++++++++++++++++++++++++++++++ content-length");
            sscanf(p1, "Content-Length: %d", &body_len);
            p1 = strstr(buf, "Content-Range: ");
            if (p1 != NULL)
            {
                red_log("+++++++++++++++++++++++++++++++++++++ multi para before");
                sscanf(p1, "Content-Range: bytes %d-%d/%d", &body_begin_count, &body_end_count, &file_total_len);
                red_log("+++++++++++++++++++++++++++++++++++++ multi para after");
                /** 提取body内容*/
                p1 = strstr(buf, "\r\n\r\n");
                if (p1 != NULL)
                {
                    red_log("+++++++++++++++++++++++++++++++++++++ double newline");
                    p1 += 4;
                    if (body_begin_count == 0)
                    {
                        if (fp != NULL)
                        {
                            fclose(fp);
                            fp = NULL;
                        }

                        fp = fopen(file_name, "wb+");
                        if (fp == NULL)
                        {
                            force_flash_reformart("storage");
                            printf("http_read_file_check: fopen fail\n");
                            return -1;
                        }
                        current_len = 0;
                        red_log("+++++++++++++++++++++++++++++++++++++ fopen");
                    }
                    if (fp != NULL)
                    {
                        nwrite = fwrite(p1, 1, body_len, fp);
                        if (nwrite != body_len)
                        {
                            fclose(fp);
                            fp = NULL;
                            force_flash_reformart("storage");
                            printf("http_read_file_check: fwrite fail\n");
                            return -1;
                        }
                        set_print_color(RED);
                        printf("\n<fwrite>%.*s</fwrite>\n", body_len, p1);
                        set_print_color(BLUE);
                        current_len += body_len;
                        printf("current file len: %d\n", current_len);

                        if (body_end_count >= file_total_len - 1)
                        {
                            yellow_log("GGG-11111111111111");
                            *next_begin_addr = 0;
                            *next_end_addr = 0;
                            fclose(fp);
                            fp = NULL;
                            set_print_color(YELLOW);
                            printf("finish file len: %d\n", current_len);
                            set_print_color(BLUE);
                            return 1;
                        }
                        else
                        {
                            yellow_log("GGG-2222222222222");
                            *next_begin_addr = body_end_count + 1;
                            *next_end_addr = body_end_count + 1024 * KB_N; //2048
                            return 0;
                        }
                        printf("--77777\n");
                    }
                    printf("--6666\n");
                }
                printf("--444\n");
            }
            printf("--333\n");
        }
        else
            printf("--88888\n");
    }
    else
        printf("--2222\n");

    printf("--11111111\n");

    if (fp != NULL)
    {
        index = 0;
        current_len = 0;
        fclose(fp);
        fp = NULL;
    }

    printf("http read check timeout.\n");
    // exit(0);
    return -1;
}

static int http_read_check(char *response)
{
    int count = 0;
    char buf[1536] = {0};
    int size;

    /** 1 = 1sec */
    while (count < 3)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            char *p1 = strstr(buf, "CONNECT\r\n");
            if (p1 != NULL)
            {
                p1 = p1 + strlen("CONNECT\r\n");
                char *p2 = strstr(p1, "\r\nOK\r\n");
                if (p2 != NULL)
                {
                    memcpy(response, p1, p2 - p1);
                    set_print_color(YELLOW);
                    printf("<http get>%.*s</http get>\n", (int)(p2 - p1), p1);
                    set_print_color(BLUE);
                    return 0;
                }
            }
        }
    }
    return -1;
}

#include <unistd.h>
#include <time.h>
int cat1_http_get_time(char *url, struct tm *p_tm)
{
    int res;
    char buf[1536] = {0};

    res = http_clear();
    if (res != 0)
        return -1;

    at_send(at_simple_http_set_res);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    at_send(at_simple_http_set_req);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    sprintf(buf, at_http_url, (int)strlen(url));
    at_send(buf);
    res = keyword_check("CONNECT");
    if (res != 0)
        return -1;

    raw_send(url, strlen(url));
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    at_send(at_simple_http_get_action);
    res = keyword_sec_check("+QHTTPGET: 0,200,", 33); //33 sec
    if (res != 0)
        return -1;

    //{"time":"20201125132110"}
    memset(buf, 0, sizeof(buf));
    at_send(at_http_read);
    res = http_read_check(buf);
    if (res != 0)
        return -1;
    char tm_str[20] = {0};
    res = sscanf(buf, "{\"time\":\"%[^\"]", tm_str);
    if (res != 1)
        return -1;
    res = sscanf(tm_str, "%4d%2d%2d%2d%2d%2d", &p_tm->tm_year, &p_tm->tm_mon, &p_tm->tm_mday, &p_tm->tm_hour, &p_tm->tm_min, &p_tm->tm_sec);
    if (res != 6)
        return -1;

    res = http_clear();
    if (res != 0)
        return -1;

    return 0;
}

int http_range_head_create(char *head, char *url, int begin, int end)
{
    char host[100] = {0};
    char path[200] = {0};

    char *p1 = NULL;
    char *p2 = NULL;
    p1 = strstr(url, "//");
    if (p1 != NULL)
    {
        p1 += 2;
        p2 = strstr(p1, "/");
        if (p2 != NULL)
        {
            memcpy(host, p1, p2 - p1);
            memcpy(path, p2, strlen(p2));
            sprintf(head, RANGE_HEAD_FMT, path, host, begin, end);
            yellow_log(head);
            return 0;
        }
    }
    red_log("http_range_head_create err");
    return -1;
}

static int http_res_206_len(void)
{
    char buf[1536] = {0};
    int size = 0;
    int count = 0;
    char *keyword = "\r\n+QHTTPGET: 0,206,";
    char *fmt = "\r\n+QHTTPGET: 0,206,%d";

    int start_sec = get_second_sys_time();
    while (get_second_sys_time() - start_sec < 65)
    {
        count++;
        memset(buf, 0, sizeof(buf));
        size = at_recv(buf, sizeof(buf));
        if (size > 0)
        {
            // printf("<>%s</>\n", buf);
            char *p1 = strstr(buf, keyword);
            if (p1 != NULL)
            {
                int http_body_len = -1;
                int num = sscanf(p1, fmt, &http_body_len);
                if (num == 1)
                {
                    set_print_color(YELLOW);
                    printf("http_body_len = %d **************\n", http_body_len);
                    set_print_color(BLUE);
                    return http_body_len;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                p1 = strstr(buf, "ERROR");
                if (p1 != NULL)
                {
                    return -1;
                }
            }
        }
    }
    set_print_color(RED);
    printf("http_res_206_len timeout: %s\n", keyword);
    set_print_color(BLUE);
    // exit(0);
    return -1;
}

int http_get_file(char *url, char *file_name, int *real_body_len)
{
    char buf[500] = {0};
    char head_buf[300] = {0};
    int size;
    int begin = 0;
    int end = 1024 * KB_N - 1; //2047
    int res = 0;
    int fail_cnt = 0;
    int response_body_size = 0;

    res = -1;
    // timeout_cnt = 0;
    /** 单次GET请求响应20sec超时*/
    int start_sec = get_second_sys_time();
    is_downloading = 1;
    cat1_mqtt_release(0);
    sleep(3);
    // suspend_all();

    while (1)
    {
        usleep(50000);

        if (fail_cnt >= 6)
        {
            yellow_log("err, http range get fail more than 6 times\n");
            is_downloading = 0;
            goto ERR;
        }

        http_clear();

        at_send(at_http_id);
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;

        at_send(at_http_response_head_on);
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;
        at_send(at_http_request_head_on);
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;

        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_http_url, strlen(url));
        at_send(buf);
        res = keyword_check("CONNECT");
        if (res != 0)
            goto ERR;
        raw_send(url, strlen(url));
        res = keyword_check("OK");
        if (res != 0)
            goto ERR;

        memset(head_buf, 0, sizeof(head_buf));
        res = http_range_head_create(head_buf, url, begin, end);
        if (res != 0)
            goto ERR;
        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_http_user_get, strlen(head_buf));
        at_send(buf);
        res = keyword_check("CONNECT");
        if (res != 0)
            goto ERR;
        raw_send(head_buf, strlen(head_buf));
        response_body_size = http_res_206_len();
        if (response_body_size <= 0)
            goto ERR;

        at_send(at_http_read);
        res = http_read_file_check(&begin, &end, file_name, response_body_size);

        if (res == 0)
        {
            fail_cnt = 0;
            start_sec = get_second_sys_time();
            yellow_log("http range get once success.");
            continue;
        }
        else if (res == 1)
        {
            fail_cnt = 0;
            is_downloading = 0;
            // resume_all();
            return 0;
        }
        else
        {
            fail_cnt++;
            set_print_color(RED);
            printf("http get file once failed.try again ...\n");
            set_print_color(BLUE);
            continue;
        }
    }

ERR:
    cat1_fast_reboot();
    esp_restart();
    return -1;
}

int get_csq(void)
{
    at_send(at_csq);
    int res = csq_check();
    g_csq = res;
    return res;
}

int check_data_call_status(void)
{
    int res = -1;
    at_send(at_check_data_call);
    res = get_data_call_ip(NULL);
    if (res == 0)
    {
        g_data_call_ok = 1;
        return 0;
    }
    else
    {
        g_data_call_ok = 0;
        return -1;
    }
}

int data_call_clear(void)
{
    int res = -1;
    at_send(at_data_call_clear);
    res = keyword_check("OK");
    if (res != 0)
        return -1;
}

void cat1_get_oper_and_mode(char *oper, int *net_mode)
{
    at_send(at_net_info);
    net_info_check(oper, net_mode);
}

int data_call(void)
{
    char buf[1536] = {0};
    int res = -1;
    static uint8_t take_turn = 0;
    cat1_echo_disable();

    if (get_csq() < 12)
        return -1;

    at_send(at_cereg);
    int res1 = cereg_check();
    if (res1 == 0)
        g_cereg_ok = 1;
    else
    {
        g_cereg_ok = 0;
    }

    at_send(at_cgreg);
    res = cgreg_check();
    if (res == 0)
        g_cgreg_ok = 1;
    else
    {
        g_cgreg_ok = 0;
    }
    if (res1 != 0 && res != 0) //2G 4G both fail
        return -1;

    int net_mode = -1;
    at_send(at_net_info);
    res = net_info_check(NULL, &net_mode);
    if (res == 0)
        g_net_mode = net_mode;
    else
    {
        g_net_mode = -1;
    }

    if (res != 0 || net_mode < 0 || net_mode > 8) //4G: net_mode = 7
        return -1;

    at_send(at_check_data_call);
    res = get_data_call_ip(NULL);
    if (res == 0)
        return 0;

    /** clear data call*/
    at_send(at_data_call_clear);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    apn_para_t apn_para = {0};
    general_query(NVS_APN_PARA, &apn_para);
    /** APN设置，拨号*/
    memset(buf, 0, sizeof(buf));
    if (strlen(apn_para.apn) <= 0 || take_turn == 1)
        sprintf(buf, at_set_apn, "", "", "");
    else
    {
        sprintf(buf, at_set_apn, apn_para.apn, apn_para.username, apn_para.password);
    }
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    at_send(at_data_call);
    res = keyword_check("OK");
    if (res != 0)
    {
        take_turn = !take_turn;
        return -1;
    }

    return 0;
}

int cat1_mqtt_release(int index)
{
    char buf[50] = {0};
    sprintf(buf, at_mqtt_disc, index);
    at_send(buf);
    return keyword_check("+QMTDISC");
}

int cat1_mqtt_conn(int index, char *host, int port, char *client_id, char *username, char *password)
{
    static int fail_cnt = 0;
    char buf[1536] = {0};
    int res = -1;
    int _3rd_ssl_enable = 0;

    cat1_echo_disable();
    res = cat1_mqtt_release(index);
    if (res != 0 && res != -1)
        return -1;

    if (index == 1)
    {
        _3rd_mqtt_para_t setting = {0};
        general_query(NVS_3RD_MQTT_PARA, &setting);
        _3rd_ssl_enable = setting.ssl_enable;
    }

    if (index == 0)
    {
        res = cat1_cacaert_set(index, MQTT_CA_FILE);
        if (res != 0)
        {
            red_log("ca file set error: aswei");
            return -1;
        }
    }
    else if (_3rd_ssl_enable == 1)
    {
        // ssl_file_t f = {0};
        // general_quey(NVS_3RD_CA_CERT, f);
        res = cat1_cacaert_set(index, MQTT_CA_FILE);
        if (res != 0)
        {
            red_log("ca file set error: aswei");
            return -1;
        }
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_recv_mode, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
    {
        // netlog_write(999);//fortest
        res = cat1_hard_reboot();
        if (res == 0 && get_second_sys_time() > 1800)
        {
            printf("cat1_hard_reboot restart \n");
            esp_restart();
            sleep(10);
        }
        return -1;
    }

    //*******************************************************
    memset(buf, 0, sizeof(buf));
    if (index == 0)
        sprintf(buf, at_set_ssl, index, 1, index); //ssl idx also use mqtt idx: 0~5
    else
    {
        if (_3rd_ssl_enable == 1)
        {
            sprintf(buf, at_set_ssl, index, 1, index);
        }
        else
        {
            sprintf(buf, at_set_ssl, index, 0, index);
        }
    }

    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    if (index == 0 || _3rd_ssl_enable == 1)
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_set_ca_cert, index, ssl_name_tab[index]);
        at_send(buf);
        res = keyword_check("OK");
        if (res != 0)
            return -1;

        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_set_ssl_level, index);
        at_send(buf);
        res = keyword_check("OK");
        if (res != 0)
            return -1;

        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_ssl_ignore_valid, index);
        at_send(buf);
        res = keyword_check("OK");

        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_set_ssl_version, index);
        at_send(buf);
        res = keyword_check("OK");
        if (res != 0)
            return -1;

        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_set_ciphersuite, index);
        at_send(buf);
        res = keyword_check("OK");
        if (res != 0)
            return -1;

        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_ssl_ignore_ltime, index);
        at_send(buf);
        res = keyword_check("OK");
        if (res != 0)
            return -1;
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_set_keepalive, index);
    at_send(buf);
    res = keyword_check("OK");
    if (res != 0)
        return -1;

    memset(buf, 0, sizeof(buf));
    if (index == 0)
    {

        sprintf(buf, at_mqtt_open_aiswei, index, g_mqtt_pdk, host, port);
    }
    else
    {
        sprintf(buf, at_mqtt_open, index, host, port);
    }

    printf("MQTT OPEN: %s\n", buf);
    at_send(buf);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "+QMTOPEN: %d,0", index);
    sleep(2);
    res = keyword_sec_check(buf, 10);
    if (res != 0)
        goto ERR;

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_mqtt_connect, index, client_id, username, password);
    printf("MQTT CONN: %s\n", buf);
    at_send(buf);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "+QMTCONN: %d,0,0", index);
    sleep(2);
    res = keyword_sec_check(buf, 5);
    if (res != 0)
        goto ERR;
    cat1_set_mqtt_ok(index);
    fail_cnt = 0;
    return 0;

ERR:
    fail_cnt++;
    if (fail_cnt > 3)
    {
        cat1_set_data_call_error();
        fail_cnt = 0;
    }
    cat1_set_mqtt_error(index);
    cat1_mqtt_release(index);
    return -1;
}

int cat1_mqtt_pub(int index, char *topic, char *payload)
{
    int res;
    static uint16_t msg_id = 0;
    static uint16_t res_msg_id = 0;

    // printf("[ pub ]%s,%s\n", topic, payload);
    char *buf = calloc(1, 256);
    sprintf(buf, at_mqtt_pub, index, ++msg_id, topic, (int)strlen(payload));
    at_send(buf);
    free(buf);
    res = keyword_check("> ");
    if (res != 0)
        return -1;

    at_send(payload);
    res = mqtt_pub_check(&res_msg_id);
    if (res == 0 && res_msg_id == msg_id)
    {
        set_print_color(YELLOW);
        printf("publish success: index=%d msgid=%hu\r\n", index, msg_id);
        set_print_color(BLUE);
        cat1_set_mqtt_ok(index);
        return 0;
    }
    else
    {
        cat1_set_mqtt_error(index);
        return -1;
    }
}

int cat1_mqtt_sub(int index, char *topic)
{
    int res;

    char *buf = calloc(1, 1536);
    sprintf(buf, at_mqtt_sub, index, topic);
    at_send(buf);
    free(buf);
    res = mqtt_sub_check();
    if (res != 0)
    {
        cat1_set_mqtt_error(index);
        return -1;
    }
    cat1_set_mqtt_ok(index);
    return 0;
}

static void cat1_file_sum(char *str_sum, char *input, int input_len)
{
    int i = 0;
    uint8_t sum[2] = {0};
    if (input_len % 2 != 0)
    {
        sum[0] = input[input_len - 1];
    }

    for (i = 0; i < input_len / 2; i++)
    {
        sum[0] ^= (uint8_t)input[i * 2];
        sum[1] ^= (uint8_t)input[i * 2 + 1];
    }
    sprintf(str_sum, "%02x%02x", sum[0], sum[1]);
}

int cat1_cacaert_set(int index, char *content)
{
    char buf[1536] = {0};
    int res;

    sprintf(buf, at_read_ca_cert, ssl_name_tab[index]);
    at_send(buf);

    char sum[50] = {0};
    char tag[50] = {0};
    cat1_file_sum(sum, content, strlen(content));
    sprintf(tag, "+QFDWL: %d,%s", (int)strlen(content), sum);
    char *s1[] = {tag, "+CME ERROR", "+QFDWL"};

    res = multi_keyword_check(3, s1);
    if (res == 0)
    {
        set_print_color(YELLOW);
        printf("cacert already exists:  cert_name **********************\n", ssl_name_tab[index]);
        set_print_color(BLUE);
        return 0;
    }
    else
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, at_del_ca_cert, ssl_name_tab[index]);
        at_send(buf);
        char *s2[] = {"OK", "+CME ERROR"};
        multi_keyword_check(2, s2);
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, at_upload_ca_cert, ssl_name_tab[index], (int)strlen(content));
    at_send(buf);
    res = keyword_check("CONNECT");
    if (res != 0)
        return -1;

    at_send(content);
    char upl_tag[50] = {0};
    sprintf(upl_tag, "+QFUPL: %d,%s", (int)strlen(content), sum);
    char *s3[] = {upl_tag, "+CME ERROR", "+QFUPL"};
    sleep(3); //response need some time
    res = multi_keyword_check(3, s3);
    if (res != 0)
        return -1;

    set_print_color(YELLOW);
    printf("cacert set ok: %s **********************\n", ssl_name_tab[index]);
    set_print_color(BLUE);
    return 0;
}

int cat1_echo_disable(void)
{
    at_send(at_disable_echo);
    return keyword_check("OK");
}

int cat1_reboot(void)
{
    at_send(at_reboot);
    sleep(15);
    return keyword_check("\r\nRDY\r\n");
}

void cat1_fast_reboot(void)
{
    at_send(at_reboot);
    sleep(5);
}

void some_refresh(void)
{
    static int start_sec = 0;
    int now_sec = get_second_sys_time();
    if (now_sec - start_sec > 30)
    {
        // cat1_gps_read();
        get_csq();
        start_sec = now_sec;
    }
}

void cat1_fs_ls(void)
{
    at_send("ATI");
    revision_check();
    at_send(at_fs_mem);
    keyword_check("OK");
    at_send(at_fs_ls);
    keyword_check("OK");
    cat1_gps_open();
}

// int main_demo(void)
// {
//     int res;
//     // serial_init();

//     red_log("+++++++++++++++++++++++++++++++++++++ we start ...");

//     at_send(at_disable_echo);
//     keyword_check("OK");

//     // data_call("iot.dcp.orange.com");
//     data_call("cmnet");

//     /** mqtt conn pub sub test*/
//     char *host = "cn-shanghai";
//     int port = 1883;
//     char *cilent_id = "B30005000001&a1tqX123itk|timestamp=2524608000000,securemode=2,signmethod=hmacsha256,lan=C|";
//     char *username = "B30005000001&a1tqX123itk";
//     char *password = "EB873F7D9570A5DFE24CC8299646859E775E5C92EAD24262DE9C0D105204C30A";

//     res = cat1_mqtt_conn(host, port, cilent_id, username, password);
//     if (res != 0)
//         return -1;
//     char *sub_topic = "/a1tqX123itk/B30005000001/user/get";
//     res = cat1_mqtt_sub(sub_topic);
//     if (res != 0)
//         return -1;

//     char *pub_topic = "/a1tqX123itk/B30005000001/user/Action";
//     char *pub_payload = "hello from device";
//     res = cat1_mqtt_pub(pub_topic, pub_payload);
//     // if (res != 0)
//     //     return -1;

//     usleep(50000);
//     cat1_mqtt_recv();

//     cat1_mqtt_release();
//     printf("finish mqtt test success-------------------------------------------------\n");

//     /** http get file test*/
//     char *url = "http://upload.aisweicloud.com/res/esp_app.bin";
//     int file_len;
//     res = http_get_file(url, "file.txt", &file_len);
//     if (res == 0)
//         printf("finish http test success-------------------------------------------------\n");

//     /** http get time test*/
//     char buf[1536] = {0};
//     char *url2 = "http://mqtt.aisweicloud.com/api/v1/updatetime?psno=B300019A0095&sign=MLOOONFNOOFJOOFJ";
//     res = cat1_http_get_time(url2, buf);
//     if (res == 0)
//         red_log(buf);

//     exit(0);
//     return 0;
// }
