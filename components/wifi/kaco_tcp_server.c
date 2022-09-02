/*  kaco-lanstick 20220801 +  */
#include "kaco_tcp_server.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "mqueue.h"
#include "asw_job_http.h"

#define RECV_BUF_SIZE 1024 * 4
#define CLIENT_TIMEOUT_SECONDS 60 * 2
#define PORT 502

static const char *TAG = "kaco-tcp-server";
// kaco-lanstick 20220801 +
static SemaphoreHandle_t tcp_mutex = NULL;
static TaskHandle_t tcp_server_task_handle = NULL;
static char is_tcp_task_exit = 0;
static int last_keep_alive_time = 0;

uint8_t g_tcp_conn_state = 0;

//========================================//

uint8_t is_modbus_tcp_in_use = 0;

//--------------------------------------//
bool is_would_block(void)
{
    int err = errno;
    return err == EINPROGRESS || err == EWOULDBLOCK || err == EAGAIN || err == EINTR;
}
//--------------------------------------//

/** Modbus-TCP请求帧中提取PDU，转为RTU，执行后返回PDU，再次组成Modbus-TCP响应报文
 *
 *                                                          |<--     PDU      -->|
 * RTU:                                       SLAVE_ID[1] + FUNC_CODE[1] + DATA[?] + CRC[2]
 * TCP:TRANS_ID[2] + PROTO_ID[2] + LENGTH[2] + UNIT_ID[1] + FUNC_CODE[1] + DATA[?]
 *
 **/

int modbus_request_pdu_legal_check(int func, int pdu_len, int w_byte_num)
{
    switch (func)
    {
    case 1:
    case 2:
    case 5:
    case 3:
    case 4:
    case 6:
        if (pdu_len == 5)
            return 0;
        break;

    case 15:
    case 16:
        if (pdu_len == 6 + w_byte_num)
            return 0;
        break;
    default:
        break;
    }
    return -1;
}

//-------------------------------------//

int modbus_tcp_fdbg(uint8_t *frame, int frame_len)
{
    tcp_fdbg_msg_t fdbg_msg = {0};
    uint8_t head[7] = {0};
    // int left_len = (frame[4] << 8) + frame[5];  // +-
    int left_len = ((frame[4] << 8) & 0xff00) | (frame[5] & 0x00ff);

    // if (memcmp(frame, "hello", 5) == 0)
    // {
    //     is_modbus_tcp_in_use = 0;
    //     return frame_len;
    // }

    if (left_len == frame_len - 6 && frame_len > 7)                                      /** length check */
        if (frame[2] == 0 && frame[3] == 0)                                              /** protocol id */
            if (modbus_request_pdu_legal_check(frame[7], frame_len - 7, frame[12]) == 0) /** func & pdu size check */
            {
                memcpy(head, frame, 7);
                xQueueReset(to_tcp_task_mq);
                if (send_cgi_msg(MODBUS_TCP_MSG_TYPE, (char *)(frame + 7), frame_len - 7, NULL) == 0) /** pdu send to inv task */
                {
                    is_modbus_tcp_in_use = 1;
                    memset(&fdbg_msg, 0, sizeof(fdbg_msg));
                    if (to_tcp_task_mq != NULL)
                    {
                        if (xQueueReceive(to_tcp_task_mq,
                                          &fdbg_msg,
                                          5000 / portTICK_RATE_MS) == pdPASS)
                        {
                            if (fdbg_msg.type == MODBUS_TCP_MSG_TYPE && fdbg_msg.len > 0)
                            {
                                int pdu_len = fdbg_msg.len;
                                char *pdu = fdbg_msg.data;
                                // printf("rx tcp len %d\n", pdu_len);
                                memcpy(frame, head, 7);
                                memcpy(frame + 7, pdu, pdu_len);
                                int field_len = pdu_len + 1;
                                frame[4] = (field_len >> 8) & 0xFF;
                                frame[5] = field_len & 0xFF;
                                is_modbus_tcp_in_use = 0;
                                return (7 + pdu_len);
                            }
                        }
                    }
                }
            }
    is_modbus_tcp_in_use = 0;
    return 0;
}
//==============kaco for test ==============//
typedef struct _md_tcp_header
{
    union
    {
        uint16_t transactionId;
        uint8_t transactionId_Hi;
        uint8_t transactionId_Li;
    };
    union
    {
        uint16_t protocalId;
        uint8_t protocalId_Hi;
        uint8_t protocalId_Li;
    };

    union
    {
        uint16_t dataLen;
        uint8_t dataLen_Hi;
        uint8_t dataLen_Li;
    };

} md_tcp_header_t;

typedef struct _md_rtu_message
{
    uint8_t slaveId;
    uint8_t funCode;

    union
    {
        uint16_t regAdr;
        uint8_t regAdr_Hi;
        uint8_t regAdr_Li;
    };

    union
    {
        uint16_t regNum;
        uint8_t regNum_Hi;
        uint8_t regNum_Li;
    };

} md_rtu_msg_t;

typedef struct _md_tcp_pack
{
    md_tcp_header_t mstr_header;
    md_rtu_msg_t mstr_payload;

} md_tcp_pack_t;

typedef struct _md_rtu_pack
{
    md_rtu_msg_t mstr_rtu_header;
    uint16_t mCrc;
} md_rtu_send_msg_t;

typedef enum
{
    MD_TCP_PARSE_STATE_UINIT = 0,
    MD_TCP_PARSE_STATE_IDLE,
    MD_TCP_PARSE_STATE_START,
    MD_TCP_PARSE_STATE_STANSID,
    MD_TCP_PARSE_STATE_PROTOLID,
    MD_TCP_PARSE_STATE_GOTLEN,
    MD_TCP_PARSE_STATE_GETDATA
} md_parse_state_t;

typedef enum
{
    MD_RTU_PARSE_STATE_UINIT = 0,
    MD_RTU_PARSE_STATE_IDLE,
    MD_RTU_PARSE_STATE_START,
    MD_RTU_PARSE_STATE_FUCCODE,
    MD_RTU_PARSE_STATE_STARTREG,
    MD_RTU_PARSE_STATE_REGNUM,
    MD_RTU_PARSE_STATE_DATALEN,
    MD_RTU_PARSE_STATE_GETDATA,
    MD_RTU_PARSE_STATE_CRC
} md_parse_rtu_state_t;
//-------------------------------------//
void modbus_parse_tcp_fdbg(uint8_t *frame, int frame_len)
{

    static md_parse_state_t em_recv_state = MD_TCP_PARSE_STATE_UINIT;
    md_tcp_pack_t md_tcp_packed_data;
    md_rtu_msg_t md_tcp_rtu_payload;

    md_rtu_send_msg_t md_rtu_send_msg[10];

    static uint16_t num_rtu_data = 0;
    uint8_t index = 0;

    uint8_t buff_tcp_payload[256];
    // static

    while (index >= frame_len)
    {
        switch (em_recv_state)
        {
        case MD_TCP_PARSE_STATE_UINIT:

        case MD_TCP_PARSE_STATE_IDLE:
        case MD_TCP_PARSE_STATE_START:

            memset(buff_tcp_payload, 0, sizeof(buff_tcp_payload) / sizeof(buff_tcp_payload[0]));

            md_tcp_packed_data.mstr_header.transactionId_Hi = frame[index++];
            md_tcp_packed_data.mstr_header.transactionId_Li = frame[index++];

            em_recv_state = MD_TCP_PARSE_STATE_STANSID;
            break;

        case MD_TCP_PARSE_STATE_STANSID:
            md_tcp_packed_data.mstr_header.protocalId_Hi = frame[index++];
            md_tcp_packed_data.mstr_header.protocalId_Li = frame[index++];

            if (md_tcp_packed_data.mstr_header.protocalId == 0x0000)
            {
                em_recv_state = MD_TCP_PARSE_STATE_PROTOLID;
            }
            else
            {
                em_recv_state = MD_TCP_PARSE_STATE_IDLE; // TODO
            }
            break;

        case MD_TCP_PARSE_STATE_PROTOLID:
            md_tcp_packed_data.mstr_header.dataLen_Hi = frame[index++];
            md_tcp_packed_data.mstr_header.dataLen_Li = frame[index++];

            if (md_tcp_packed_data.mstr_header.dataLen > 0 && md_tcp_packed_data.mstr_header.dataLen < 256)
            {
                em_recv_state = MD_TCP_PARSE_STATE_GOTLEN;
            }
            else
            {
                em_recv_state = MD_TCP_PARSE_STATE_IDLE; // TODO
            }
            break;

        case MD_TCP_PARSE_STATE_GOTLEN:
            for (uint8_t j = 0; j < md_tcp_packed_data.mstr_header.dataLen; j++)
            {
                buff_tcp_payload[j] = frame[index++];

                printf("0x%0x ", buff_tcp_payload[j]);
            }
            em_recv_state = MD_TCP_PARSE_STATE_GETDATA;
            break;

        case MD_TCP_PARSE_STATE_GETDATA:

            if (send_cgi_msg(MODBUS_TCP_MSG_TYPE, (char *)(buff_tcp_payload), md_tcp_packed_data.mstr_header.dataLen, NULL) == 0) /** pdu send to inv task */
            {
                em_recv_state = MD_TCP_PARSE_STATE_START;
                num_rtu_data++;
                printf("\n ==========   send msg to inv ok %d \n", num_rtu_data);
            }
            else
            {
                em_recv_state = MD_TCP_PARSE_STATE_IDLE;
                printf("\n ==========   send msg to inv erro \n");
            }

            break;

        default:
            break;
        }
    }
}

//-------------------------------------//
// for test
int tcp_frame_recv(int sock, uint8_t *buf, int max_size)
{
    int nread;
    int len = 0;
    while (1)
    {
        // usleep(10000);
        // if (get_second_sys_time() - last_keep_alive_time > CLIENT_TIMEOUT_SECONDS)
        // {
        //     last_keep_alive_time = get_second_sys_time();
        //     return -1;
        // }
        nread = recv(sock, buf + len, max_size - len, MSG_DONTWAIT);
        if (nread < 0)
        {
            if (is_would_block() == 1)
            {
                if (len > 0 && len <= 512)
                {
                    return len; /** 读取间断，有数据则返回数据 **/
                }
                else if (len > 512)
                {
                    len = 0; /** 数据黏包，长度过长*/
                    continue;
                }
                else
                {
                    continue; /** 读取间断，无数据则继续读取 **/
                }
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            return -1;
        }
        else
        {

            printf("\n=============   recv data ================\n");
            for (uint8_t j = 0; j < nread; j++)
                printf("0x%0x ", (uint8_t) * (buf + len + j));

            printf("\n=============   recv data ================\n");
            last_keep_alive_time = get_second_sys_time();
            len += nread;
            if (len >= max_size)
                len = 0;
            continue;
        }
    }
    printf("\n=============   recv data end ===>>>>=============\n");
    printf("\n=============   recv data end ===<<<<=============\n");
}

#if 0 // for test
int tcp_frame_recv(int sock, uint8_t *buf, int max_size)
{
    int nread;
    int len = 0;
    while (1)
    {
        usleep(10000);
        if (get_second_sys_time() - last_keep_alive_time > CLIENT_TIMEOUT_SECONDS)
        {
            last_keep_alive_time = get_second_sys_time();
            return -1;
        }
        nread = recv(sock, buf + len, max_size - len, MSG_DONTWAIT);
        if (nread < 0)
        {
            if (is_would_block() == 1)
            {
                if (len > 0 && len <= 512)
                {
                    return len; /** 读取间断，有数据则返回数据 **/
                }
                else if (len > 512)
                {
                    len = 0; /** 数据黏包，长度过长*/
                    continue;
                }
                else
                {
                    continue; /** 读取间断，无数据则继续读取 **/
                }
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            return -1;
        }
        else
        {
            last_keep_alive_time = get_second_sys_time();
            len += nread;
            if (len >= max_size)
                len = 0;
            continue;
        }
    }
}

#endif

//----------------------------------------------//
static void tcp_server_task(void *pvParameters)
{
    uint8_t *buf = calloc(RECV_BUF_SIZE, 1);

    char addr_str[128] = {0};
    int addr_family;
    int ip_protocol;
    int listen_sock;
    struct sockaddr_in dest_addr;

    last_keep_alive_time = get_second_sys_time();

#if 1 // IPV4
    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_TCP; // IPPROTO_IP;
    memset(addr_str, 0, sizeof(addr_str));
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

#else // IPV6
    struct sockaddr_in6 dest_addr;
    bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);

    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        sleep(1);
        goto TASK_EXIT;
    }

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_sock);
        sleep(1);
        goto TASK_EXIT;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);

    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_sock);

        sleep(1);
        goto TASK_EXIT;
    }
    ESP_LOGI(TAG, "Socket listening");

    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    uint addr_len = sizeof(source_addr);

    int sock = 0;
    int len = 0;

RE_ACCEPT:
    g_tcp_conn_state = 0;
    sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
        close(listen_sock);
        sleep(1);
        goto RE_ACCEPT;
    }
    g_tcp_conn_state = 1;
    last_keep_alive_time = get_second_sys_time();
    ESP_LOGI(TAG, "Socket accepted");

    while (1)
    {
        usleep(10000);
        memset(buf, 0, RECV_BUF_SIZE);
        len = tcp_frame_recv(sock, buf, RECV_BUF_SIZE);
        // Error occurred during receiving
        if (len < 0)
        {
            goto CLOSE;
        }
        else if (len > 0)
        {
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
            /** hex print rx buf */
            len = modbus_tcp_fdbg(buf, len);
            if (len > 0)
            {
                err = send(sock, buf, len, MSG_DONTWAIT);
                if (err == 0)
                {
                    ESP_LOGE(TAG, "tcp closed when send: errno %d", errno);
                    goto CLOSE;
                }
                else if (err < 0)
                {
                    if (is_would_block() == 1)
                    {
                        continue;
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Connection closed");
                        goto CLOSE;
                    }
                }
            }
        }
    }

CLOSE:
    if (sock != -1)
    {
        ESP_LOGE(TAG, "Shutting down socket and re-accept...");
        shutdown(sock, SHUT_RDWR);
        close(sock);
        sock = -1;
        sleep(3);
    }
    goto RE_ACCEPT;

TASK_EXIT:
    free(buf);
    is_tcp_task_exit = 1;
    vTaskDelete(NULL);
    return;
}

//----------------------------------------//

void start_kaco_tcp_server(void)
{
    if (g_kaco_run_mode != 2)
        return;
    static uint8_t is_tcp_server_started = 0;

    tcp_mutex = xSemaphoreCreateMutex();

    if (is_tcp_server_started == 0)
    {
        is_tcp_server_started = 1;
        if (xSemaphoreTake(tcp_mutex, (TickType_t)10000 / portTICK_RATE_MS) == pdTRUE)
        {
            if (tcp_server_task_handle != NULL)
            {
                vTaskDelete(tcp_server_task_handle);

                tcp_server_task_handle = NULL;
                sleep(2);
            }
            is_tcp_task_exit = 0; //[  add for debug]
            xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, &tcp_server_task_handle);

            xSemaphoreGive(tcp_mutex);
        }
    }
}

void tcp_damon(uint32_t *discnt_time)
{

    if (is_tcp_task_exit == 1 && get_second_sys_time() - *discnt_time > 1800)
    {
        discnt_time = get_second_sys_time();
        sent_newmsg();
    }
}