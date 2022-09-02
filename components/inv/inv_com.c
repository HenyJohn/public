#include "inv_com.h"
#include "mqueue.h"
#include "kaco_tcp_server.h"
#include "asw_protocol_ftp.h"

#define INV_UART_TXD (GPIO_NUM_32) // (GPIO_NUM_2)//
#define INV_UART_RXD (GPIO_NUM_33) //[(GPIO_NUM_15)  mark] [32-tx 33-rx[调试临时用]]

// #define ATE_UART_TXD (GPIO_NUM_18)
// #define ATE_UART_RXD (GPIO_NUM_19)
// #define ATE_UART_TXD (GPIO_NUM_32) //[  mark]
// #define ATE_UART_RXD (GPIO_NUM_33) //

#define ALL_UART_RTS (UART_PIN_NO_CHANGE)
#define ALL_UART_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)
static const char *TAG = "inv_com.c";

Inv_arr_t inv_arr = {0};
static int64_t request_data_time = 0, read_meter_time = 0;

uint8_t g_num_real_inv = 0; //[  mark] g_g_num_real_inv 建议改成
int inv_comm_error = 0;
unsigned int inv_index = 0;
uint8_t g_monitor_state = 0; //[  mark]

sys_log inv_log = {0};
sys_log com_log = {0};
sys_log last_comm = {0};

int inv_com_reboot_flg = 0;
int now_read_inv_index = 0;
cloud_inv_msg trans_data;

// Inv_arr_t ipc_inv_arr = {0};

Inv_arr_t cgi_inv_arr = {0};
Inv_arr_t cld_inv_arr = {0};

/* register begin */
const uint8_t clear_register_frame[] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x10, 0x04, 0x00, 0x01, 0x14};
const uint8_t check_sn_frame[] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x10, 0x07, 0x00, 0x01, 0x17};
const uint8_t set_addr_header[] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x10, 0x08, 0x21};
/***********************************************/

//--------------------------------------//
int check_inv_states()
{
    uint8_t i = 0, j = 0;
    for (i = 0; i < g_num_real_inv; i++)
    {
        if (inv_arr[i].status == 1)
            j++;
    }

    if (j >= g_num_real_inv)
        return 1; // led_green_on();

    if (j < 1)
        return -1; // led_green_off();

    if (j < g_num_real_inv && j > 0)
        return 0; // green_fast_blink();
    return j;
}

//---------------------------------------------//

// int16_t get_cur_week_key()
// {
//     return cur_week_day;
// }

// void set_cur_week_key(int16_t day)
// {
//     cur_week_day=day;
// }

//----------------------------------------------//
int8_t serialport_init(UART_TYPE type)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    int baud_rate;
    int uart_port;
    int uart_tx;
    int uart_rx;
    int res = -1;

    switch (type)
    {
    case INV_UART:
    {
        baud_rate = 9600;
        uart_port = UART_NUM_1;

        uart_tx = INV_UART_TXD; // [  mark] 引脚号需要根据最新的硬件修改
                                // （UART0 的引脚应该是固定的不能修改）
        uart_rx = INV_UART_RXD;
    }
    break;
    // case ATE_UART:
    // {
    //     baud_rate = 115200; // 9600; [  update]change
    //     uart_port = UART_NUM_2;
    //     uart_tx = ATE_UART_RXD; // 17  [  mark] 引脚号需要根据最新的硬件修改

    //     uart_rx = ATE_UART_TXD; // 16
    // }
    // break;
    default:
    {
        ESP_LOGE(TAG, " the type is error ");
        // printf("\n-------TGL DEBUG PRINT ERROR: the type is error \n");

        return ASW_FAIL;
    }
    break;
    }

    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(uart_port, &uart_config);
    uart_set_pin(uart_port, uart_tx, uart_rx, ALL_UART_RTS, ALL_UART_CTS);

    /* 消息 [  mark] */
    res = uart_driver_install(uart_port, BUF_SIZE * 2, 0, 0, NULL, 0);
    if (res == ESP_OK)
        return ASW_OK;
    return ASW_FAIL;
}
//----------------------------------------------------//

//----------------------------------------------------//
uint8_t load_reg_info(void)
{
    Inv_reg_arr_t reg_info_arr = {0}; // nvs_invert_db
    int i;
    int ret;

    ret = general_query(NVS_INVTER_DB, (void *)reg_info_arr);
    // printf("load modbus id:********************\n");

    if (ASW_FAIL == ret)
        return 0;

    for (i = 0; i < INV_NUM; i++)
    {
        if (reg_info_arr[i].modbus_id < 3) // START_ADD
            break;
        memcpy(&inv_arr[i].regInfo, &reg_info_arr[i], sizeof(InvRegister));
        ESP_LOGI(TAG, "load modbus id: %02x, sn: %s\n",
                 inv_arr[i].regInfo.modbus_id, inv_arr[i].regInfo.sn);
    }
    ESP_LOGI(TAG, "inv num %d \n", i);
    return i;
}
//-__---------------------------------------------------//
int8_t get_modbus_id(uint8_t *id)
{
    // int scan_id;
    uint8_t i;

    if (!inv_arr[0].regInfo.modbus_id)
    {
        *id = START_ADD;
        return ASW_OK;
    }

    for (i = 0; i < INV_NUM; i++)
        if (inv_arr[i].regInfo.modbus_id == 0 && inv_arr[i - 1].regInfo.modbus_id >= START_ADD)
            break;

    if (inv_arr[i - 1].regInfo.modbus_id < 246)
    {
        *id = inv_arr[i - 1].regInfo.modbus_id + 1;
        ESP_LOGI(TAG, "next id %d \n", *id);
        return ASW_OK;
    }
    else
        return ASW_FAIL;
}

//--------------------------------------------------//
int8_t get_sum(uint8_t *buf, int len, uint8_t *high_byte, uint8_t *low_byte)
{
    uint16_t sum = 0;
    uint8_t i = 0;
    for (i = 0; i < len; i++)
        sum += buf[i];
    *high_byte = (sum >> 8) & 0xFF;
    *low_byte = sum & 0xFF;
    ESP_LOGI(TAG, "high low: %02X %02X\n", *high_byte, *low_byte);
    return ASW_OK;
}
//--------------------------------------------------//
int8_t check_sum(uint8_t *buf, int len)
{
    uint8_t h_byte = 0;
    uint8_t l_byte = 0;
    get_sum(buf, len - 2, &h_byte, &l_byte);

    if (h_byte == buf[len - 2] && l_byte == buf[len - 1])
        return ASW_OK;
    else
    {
        return ASW_FAIL;
    }
}
//------------------------------------------//
int8_t parse_sn(uint8_t *buf, int len, char *psn)
{
    if (check_sum(buf, len) != 0 || len != 45)
    {
        ESP_LOGE(TAG, "check sum error\n");
        return ASW_FAIL;
    }
    memcpy(psn, buf + 9, 32);
    return ASW_OK;
}
//---------------------------------------------------//
int chk_msg(cloud_inv_msg *trans_data)
{
    cloud_inv_msg buf = {0};
    char ws[512] = {0};

    if (mq1 == NULL)
    {
        ESP_LOGE(TAG, "ERrro: mq1 is null");
        return -1;
    }

    if (xQueueReceive(mq1, &buf, 0) == pdFALSE)
    {
        // printf("\n============= inv update debug D1 =============\n");
        if (to_inv_fdbg_mq != NULL)
        {
            // printf("\n============= inv update debug D2 =============\n");
            if (xQueueReceive(to_inv_fdbg_mq, &buf, 0) == pdFALSE) // cgi和inv之间，通过2个消息队列实现双向通信*/
            {
                return -1;
            }
        }
    }
    ASW_LOGW("DEBUG_INFO: chk_msg recv buf have data type:%d", buf.type);

    // kaco-lanstick 20220801 +
    if (MODBUS_TCP_MSG_TYPE == buf.type)
    {
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        return MODBUS_TCP_MSG_TYPE;
    }
    else if (2 == buf.type)
    {
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;

        memcpy(ws, &trans_data->data[trans_data->len], strlen(trans_data->data)); //[  mark] ???

        ESP_LOGI(TAG, "recvmsg %d %d  %d  %d %s\n", sizeof(buf), trans_data->len,
                 strlen(trans_data->data), trans_data->len, ws);
        return 2;
    }
    else if (3 == buf.type)
    {

        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        trans_data->ws = buf.len;

        ESP_LOGI(TAG, "recv msg---%s--%d--\n", buf.data, buf.len);
        ESP_LOGI(TAG, "mstyp %d --%s-- %c-- %d --%s--\n", buf.type,
                 buf.data, buf.ws, trans_data->len, trans_data->data);

        return 3;
    }
    else if (5 == buf.type)
    {
        trans_data->ws = buf.data[0];
        ESP_LOGI(TAG, "mstyp %d %d %d %d %d\n", buf.type, buf.data[0], buf.data[1], buf.data[2], buf.ws);
        return 5;
    }
    else if (6 == buf.type)
    {
        ESP_LOGI(TAG, "mstyp clear meter %d %s \n", buf.type, buf.data);

        return 6;
    }
    else if (99 == buf.type)
    {

        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        ESP_LOGI(TAG, "reboot monitor or inverter %d %d %s \n", buf.type, buf.data[0], &buf.data[1]);
        return 99;
    }
    else if (30 == buf.type)
    {

        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        trans_data->ws = buf.len;
        ESP_LOGI(TAG, "recv cgi msg update---%s--%d--\n", buf.data, buf.len);
        ESP_LOGI(TAG, "mstyp %d --%s-- %c-- %d --%s--\n", buf.type, buf.data,
                 buf.ws, trans_data->len, trans_data->data);

        return 30;
    }
    else if (50 == buf.type)
    {

        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        trans_data->ws = buf.len;
        ESP_LOGI(TAG, "recv cgi msg scan---%s--%d--\n", buf.data, buf.len); //[  mark] data读取不到值
        ESP_LOGI(TAG, "mstyp %d --%s-- %c-- %d --%s--\n", buf.type, buf.data,
                 buf.ws, trans_data->len, trans_data->data);

        return 50;
    }
    else if (20 == buf.type)
    {
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        memcpy(ws, &trans_data->data[trans_data->len], strlen(trans_data->data)); //-trans_data->len);

        ESP_LOGI(TAG, "cgi trans recvmsg %d %d  %d  %d %s\n", sizeof(buf),
                 trans_data->len, strlen(trans_data->data), trans_data->len, ws);

        return 20;
    }
    else if (60 == buf.type)
    {
        ESP_LOGI(TAG, "mstyp clear meter %d %s \n", buf.type, buf.data);

        return 60;
    }
    return -1;
}

//-----------------------------------------------------//
void reg_info_log(void)
{
    uint8_t i;
    ESP_LOGI(TAG, "scan result:\n");
    for (i = 0; i < INV_NUM; i++)
    {
        if (inv_arr[i].regInfo.modbus_id != 0)
        {
            ESP_LOGI(TAG, "modbus id: %02x, sn: %s\n",
                     inv_arr[i].regInfo.modbus_id, inv_arr[i].regInfo.sn);
        }
    }
}

//---------------------//
// [  update] add
int recv_bytes_frame_meter(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    char buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    int i = 0;
    uint16_t max_len = *res_len;
    uint16_t crc = 0;
    ESP_LOGI(TAG, "maxlen---------------------------------%d------\n", max_len);
    while (1)
    {

        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
        {
            return -1;
        }
        // 20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);

        if (nread > 0)
        {
            len += nread;

            for (i = 0; i < len; i++)
                printf("*%02X", buf[i]);
            break;
        }
    }

    if (len > 2)
    {
        crc = crc16_calc((uint8_t *)buf, len);
        ESP_LOGI(TAG, "mmm crc %d \n", crc);
        if (crc == 0)
        {
            memcpy(res_buf, buf, len);
            *res_len = len;
            goto end_read;
        }
    }

    if (len == max_len)
    {
        crc = crc16_calc(res_buf, *res_len);
        ESP_LOGI(TAG, "MMM crc %d \n", crc);
        if (crc == 0)
        {
            memcpy(res_buf, buf, len);
            *res_len = len;
            goto end_read;
        }
    }

    if (len > max_len + 5)
    {
        ESP_LOGI(TAG, "maxlen-----------%d---%d---\n", max_len, len);
        return -1;
    }

    while (1)
    {
        usleep(50000);
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            ;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
        }
        else
        {
            if (len > max_len + 5)
            {
                ESP_LOGI(TAG, "maxlen-----------%d---%d---\n", max_len, len);
                return -1;
            }
            // end_read:
            memcpy(res_buf, buf, len);
            *res_len = len;
            break;
            // return 0;
        }
    }
    crc = crc16_calc(res_buf, *res_len);

end_read:
    ESP_LOGI(TAG, "recvv ok %x  %x %d \n", crc, ((res_buf[*res_len - 2] << 8) | res_buf[*res_len - 1]), len);
    if (len > 1 && 0 == crc) //((res_buf[*res_len-2]<<8)|res_buf[*res_len-1]) == crc)
        return 0;
    else
        return -1;
}
//----------------------------------------------------//
int8_t recv_bytes_frame_waitting_nomd(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    char buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    uint8_t i = 0;

    while (1)
    {
        usleep(30000);
        timeout_cnt++;
        if (timeout_cnt >= 50)
        {
            return ASW_FAIL;
        }
        // 20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]); //   mark

            break;
        }
    }

    while (1)
    {
        usleep(50000);
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        printf("recvvv ****************************%d \n", nread);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]); //
        }
        else
        {
            ESP_LOGI(TAG, "recvvv ***** \n");
            memcpy(res_buf, buf, len);
            *res_len = len;
            return ASW_OK;
        }
    }
    ESP_LOGI(TAG, "recvvv -1 \n");
    *res_len = 0;
    return ASW_FAIL;
}

//----------kaco-lanstick 20220801 + -------//
int8_t recv_bytes_frame_waitting_nomd_tcp(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    char buf[500] = {0};
    uint16_t len = 0;
    uint16_t nread = 0;
    uint16_t timeout_cnt = 0;
    uint16_t i = 0;
    uint16_t expect_len = *res_len;

    if (expect_len <= 0)
        goto END;

    while (1)
    {
        usleep(5000);
        timeout_cnt++;
        if (timeout_cnt >= 300)
        {
            goto END;
        }
        // 20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            printf("\r\n");
            break;
        }
    }
    timeout_cnt = 0;
    while (1)
    {
        usleep(10000);
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        // printf("recvvv ****************************%d \n", nread);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            printf("\r\n");
        }
        else if (len < expect_len)
        {
            timeout_cnt++;
            if (timeout_cnt >= 200)
            {
                goto END;
            }
            else if ((buf[1] & 0x80) && len == 5) // modbus func 0x80 is error
            {
                memcpy(res_buf, buf, len);
                *res_len = len;
                return ASW_OK;
            }
            else
            {
                continue;
            }
        }
        else if (len == expect_len)
        {
            // printf("recvvv ****************************888 \n");
            memcpy(res_buf, buf, len);
            *res_len = len;
            return ASW_OK;
        }
        else
        {
            goto END;
        }
    }
END:
    *res_len = 0;
    return ASW_FAIL;
}

//------------------------------------------------//
int8_t legal_check(char *text)
{

    if (strlen(text) > 6)
        return ASW_OK;
    else
        return ASW_FAIL;
}

//--------------------------------------------------//
SERIAL_STATE inv_task_schedule(char *func, cloud_inv_msg *data)
{
    int64_t period = 0;
    int res;

    /*query data from inverter every 10 seconds*/

    /* kaco-lanstick 20220811 + */
    if (event_group_0 & KACO_SET_ADDR)
    {
        event_group_0 &= ~KACO_SET_ADDR;
        return TASK_KACO_SET_ADDR;
    }
    period = (get_second_sys_time() - request_data_time);

    /**
     * @brief 6&60\2&20\3&30 一样的内容 chk_msg返回的data也是一样的
     */

    res = chk_msg(data);

    // 根据不同的索引,返回不同的值。
    /* kaco-lanstick 20220801 + */
    if (MODBUS_TCP_MSG_TYPE == res) /** modbus tcp fdbg */
    {
        *func = MODBUS_TCP_MSG_TYPE;
        return TASK_TRANS_UP_INV;
    }
    else if (2 == res)
    {
        *func = 2;
        return TASK_TRANS_UP_INV;
    }
    else if (3 == res)
    {
        *func = 3;
        return TASK_TRANS_UP_INV;
    }
    else if (5 == res)
    {
        *func = 5;
        return TASK_TRANS_SCAN_INV;
    }
    else if (6 == res)
    {
        *func = 6;
        return CLEAR_METER_SET;
    }
    else if (99 == res) // reboot
    {
        *func = 99;
        return RESTART;
    }
    else if (30 == res)
    {
        *func = 30;
        return TASK_TRANS_UP_INV;
    }
    else if (50 == res)
    {
        *func = 50;
        return TASK_TRANS_SCAN_INV;
    }
    else if (20 == res)
    {
        *func = 20;
        return TASK_TRANS_UP_INV;
    }
    else if (60 == res)
    {
        *func = 60;
        return CLEAR_METER_SET;
    }

    /**
     * @brief [  mark]
     * 每10s,  调用TASK_QUERY_DATA
     * 每390ms,调用TASK_QUERY_METER  //没有来自云端的数据信息会到这里
     */
    // ASW_LOGW("********  debug print,inv_task_scedule AA %lld,funCode %d ,peroid: %lld,meter %lld",get_second_sys_time(),res,period,get_msecond_sys_time() - read_meter_time);
    if (period >= 10)
    {
        request_data_time = get_second_sys_time(); // esp_timer_get_time();
        ESP_LOGI(TAG, "read inv data time sec: %lld\n", request_data_time);
        return TASK_QUERY_DATA;
    }

    if (get_msecond_sys_time() - read_meter_time > 390) // 390 ms
    {
        read_meter_time = get_msecond_sys_time();
        return TASK_QUERY_METER;
    }
    return TASK_IDLE;
}

//----------------------------------------------------------------//
int inv_com_log(int index, int err)
{
    char buf[300] = {0};
    char sts[16] = {0};
    char tmp[160] = {0};
    // uint8_t j = 0;

    if (strlen(com_log.tag) == 0)
        memcpy(com_log.tag, "\r\nComm error\r\n", strlen("\r\nComm error\r\n"));

    if (err == -1)
        sprintf(sts, "%s", "err");

    if (err == -2)
        sprintf(sts, "%s", "out");

    if (strlen(inv_arr[index - 1].regInfo.sn) < 1)
        sprintf(buf, "%s,%s\r\n", "onyle one inverter", sts);
    else
        sprintf(buf, "%s,%s\r\n", inv_arr[index - 1].regInfo.sn, sts);

    if (com_log.index + strlen(buf) > 329)
    {

        if (com_log.index > 260)
            memcpy(tmp, &com_log.log[100], 160);
        else if (com_log.index > 100 && com_log.index < 261)
            memcpy(tmp, &com_log.log[100], com_log.index - 100);

        memset(com_log.log, 0, sizeof(com_log.log));
        com_log.index = 0;

        memcpy(com_log.log, tmp, strlen(tmp));
        com_log.log[160] = '\n';
        memcpy(&com_log.log[161], buf, strlen(buf));

        com_log.index += (161 + strlen(buf));
        // printf("========================new index---- %d \n", com_log.index);
    }
    else
    {
        memcpy(&com_log.log[com_log.index], buf, strlen(buf));
        com_log.index += strlen(buf);
    }

    if (com_log.index > 330)
    {
        memset(com_log.log, 0, sizeof(com_log.log));
        com_log.index = 0;
    }
    return 0;
}

//--------------------------------------------------------------------//
int internal_inv_log(int typ)
{
    char buf[300] = {0};
    int j = 0;

    if (strlen(inv_log.tag) == 0)
    {
        memcpy(inv_log.tag, "\r\nList invs\r\n", strlen("\r\nList invs\r\n"));
        if (typ > 0)
        {
            for (int i = 0; i < typ && j < 200; i++)
            {
                memcpy(&buf[j], inv_arr[i].regInfo.sn, strlen(inv_arr[i].regInfo.sn));
                j += strlen(inv_arr[i].regInfo.sn);
                buf[j] = ',';
                // buf[j + 1] = inv_arr[i].regInfo.modbus_id + 0x30; //[  udpate]
                buf[j + 1] = inv_arr[i].regInfo.modbus_id; //[  udpate]  kaco-lanstick 20220811 +-
                buf[j + 1 + 1] = '\n';
                j += 3;
                printf("buf%d %s \n", j, buf);
            }
        }
        else
        {
            if (inv_arr[0].regInfo.modbus_id == 0x03)
                memcpy(buf, "only one inverter modbus_id 3\n", strlen("only one inverter modbus_id 3\n"));
            else
            {
                memcpy(buf, "only one inverter modbus_id err", strlen("only one inverter modbus_id err"));
                buf[strlen("only one inverter modbus_id err")] = inv_arr[0].regInfo.modbus_id;
                buf[strlen("only one inverter modbus_id err") + 1] = '\n';
            }
        }
        inv_log.index = strlen(buf);
        memcpy(inv_log.log, buf, strlen(buf));
    }

    ESP_LOGI(TAG, "inv_log_enable-- %s \n", inv_log.tag);
    ESP_LOGI(TAG, "inv_log_enable   %s \n", inv_log.log);
    return 0;
}
//--------------------------------------------------------------//
int last_com_data(unsigned int index)
{
    int datlen;
    uint8_t i;
    Inv_data invdata = {0};
    if (strlen(last_comm.tag) == 0)
        memcpy(last_comm.tag, "\r\nLast comm\r\n", strlen("\r\nLast comm\r\n"));

    char payload[330] = {0};

    memset(last_comm.log, 0, sizeof(last_comm.log));
    memcpy(&invdata, &cgi_inv_arr[index - 1].invdata, sizeof(Inv_data)); //[  update]

    datlen = sprintf(payload, "%s %s,%d,%d hz,%d w,%d err,%d pf,%d cf,",
                     invdata.time, invdata.psn, cgi_inv_arr[index - 1].regInfo.modbus_id,
                     invdata.fac, invdata.pac, invdata.error,
                     invdata.cosphi, invdata.col_temp);

    for (i = 0; i < 10; i++)
    {
        if ((invdata.PV_cur_voltg[i].iVol != 0xFFFF) && (invdata.PV_cur_voltg[i].iCur != 0xFFFF))
        {
            datlen += sprintf(payload + datlen, "v%d:%d,i%d:%d;",
                              i + 1, invdata.PV_cur_voltg[i].iVol,
                              i + 1, invdata.PV_cur_voltg[i].iCur);
        }
    }
    last_comm.index = strlen(payload);
    memcpy(last_comm.log, payload, strlen(payload));

    return 0;
}

/*----------------------------------------------------------------------------*/
/*! \brief  This function create the com comunication task.
 *  \param  initial_data      [in], The task's data.
 *  \return none.
 *  \see
 *  \note
----------------------------------------------------------------------------*/
SERIAL_STATE query_data_proc()
{

    struct tm currtime = {0};
    time_t t = time(NULL);
    unsigned int save_time = 0;

    int res = -1;

    Inverter *curr_inv_ptr = NULL;

    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;

    static int last_num_real_inv = 0;

    if (!g_num_real_inv)
    {
        g_num_real_inv = load_reg_info();


///---------------------kaco start------------------------------///
        // if (g_num_real_inv != last_num_real_inv)
        // {
        //     // kaco-lanstick 集中上传csv文件时，当逆变器数量发生变化时，初始化ftp本地文件。
        //     ESP_LOGW(TAG, " inv num is changed %d to %d.", last_num_real_inv, g_num_real_inv);
        //     last_num_real_inv = g_num_real_inv;

        //     asw_ftp_upload();
        // }
///---------------------kaco end------------------------------///


        if (g_num_real_inv > 0)
            internal_inv_log(g_num_real_inv);
    }

    if (g_num_real_inv < 1)
    {
        ESP_LOGI(TAG, " only one inverter default add 3 \n");
        inv_arr[0].regInfo.modbus_id = 0x03;
        g_num_real_inv = 1;
        internal_inv_log(0);
    }

    if (inv_index >= g_num_real_inv)
        inv_index = 0;

    curr_inv_ptr = &inv_arr[inv_index++];

    flush_serial_port(UART_NUM_1);

    /****************************************************************
     * 1. @brief 从云端时间的同步完成后，g_monitor_state 的
     *        SYNC_TIME_FROM_CLOUD_INDEX置位，
     *        同时检测regInfo.syn_time状态是否为0，如两个条件满足，
     *        开始发送时间到inv *
     * @param
     ****************************************************************/
    if (g_monitor_state & SYNC_TIME_FROM_CLOUD_INDEX)
    {
        if (curr_inv_ptr->regInfo.syn_time == 0)
        {
            //[  mark] udpate
            ASW_LOGW("DEBUG: Sync time from cloud");

            if (md_write_data(curr_inv_ptr, CMD_MD_WRITE_INV_TIME) == ASW_OK)
            {
                inv_arr[inv_index - 1].regInfo.syn_time = 1;
                ASW_LOGW("DEBUG: Sync time to inv OK!!");
            }
        }
    }

    /****************************************************************
     * 2. @brief  g_monitor_state==0时调用（上电后第一次调用）
     *          从逆变器读取时间 *  stick与逆变器时间同步（上电会执行）
     * @param
     ****************************************************************/
    ESP_LOGI(TAG, "read time A %d %d \n", g_monitor_state, curr_inv_ptr->status);
    if (!g_monitor_state)
    {
        ESP_LOGI(TAG, "read time B  %d %d \n", g_monitor_state, curr_inv_ptr->status);
        if (curr_inv_ptr->status)
        {
            // if (ASW_OK == (md_read_data(curr_inv_ptr, CMD_MD_READ_INV_TIME, &save_time,  /
            //                             (uint16_t *)&save_time))) // erro 只有在CMD_READ_INV_DATA才有效
            if (ASW_OK == (md_read_data(curr_inv_ptr, CMD_MD_READ_INV_TIME, NULL, NULL))) //
            {
                g_monitor_state |= SYNC_TIME_FROM_INV_INDEX;
            }
        }
    }
    /* read data from inverter*/ //&& g_monitor_state == SYNC_TIME_END
    ESP_LOGI(TAG, "read inv data start... %d  %d %s\n",
             curr_inv_ptr->regInfo.modbus_id,
             inv_arr[inv_index - 1].last_save_time,
             inv_arr[inv_index - 1].regInfo.sn);

    //[  mark] udpate
    // ESP_LOGW(TAG, "---TGL PRINT TODO update component...");

    /****************************************************************
     * 3. @brief  周期性调用（10s），获取逆变器数据
     *
     * @param
     ****************************************************************/
    res = md_read_data(curr_inv_ptr, CMD_READ_INV_DATA,
                       &inv_arr[inv_index - 1].last_save_time,
                       &inv_arr[inv_index - 1].last_error);
    if (ASW_OK == res)
    {
        /* Notice:This is very important,if you want change the inv's real data */

        //如果读数据成功，则再次读出逆变器信息并标记“上线”和“同步”标志
        /*check the status of inverter online or offline*/
        inv_arr[inv_index - 1].status = 1; // curr_inv_ptr->status=1;

        save_time = inv_arr[inv_index - 1].regInfo.syn_time;

        ESP_LOGI(TAG, "mode_name %s \n", inv_arr[inv_index - 1].regInfo.mode_name);

        /****************************************************************
         * @brief 信息错误时，重新读取info
         *
         ****************************************************************/
        if (strlen(inv_arr[inv_index - 1].regInfo.mode_name) < 10)
        {

            if (ASW_OK == (md_read_data(curr_inv_ptr, CMD_MD_READ_INV_INFO, &inv_index, (uint16_t *)&save_time)))
            {
                ESP_LOGI(TAG, "INV INFO READ \n");
            }
            else
            {
                ESP_LOGE(TAG, "current inverter point is nULL or current inverter's serial number is Unknow!!\r\n!!");
            }
        }

        inv_arr[inv_index - 1].regInfo.syn_time = save_time;
        last_com_data(inv_index);
    }
    else
    {
        inv_com_log(inv_index, res);
        inv_comm_error++;
        inv_arr[inv_index - 1].status = 0; // curr_inv_ptr->status=0;
        // ESP_LOGE(TAG, "read inv data is error\r\n");
    }

    return TASK_IDLE;
}
//---------------------------------------------------//
int8_t recv_bytes_frame_transform(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    uint8_t buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    int i = 0;
    uint16_t max_len = *res_len;
    uint16_t crc = 0;
    ESP_LOGI(TAG, "maxlen--%d---\n", max_len);

    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 30)
        {
            return ASW_FAIL;
        }
        // 20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);

        if (nread > 0)
        {
            len += nread;

            for (i = 0; i < len; i++)
                ESP_LOGI(TAG, "*%02X", buf[i]);
            break;
        }
    }

    if (len > 2)
    {
        crc = crc16_calc(buf, len);
        ESP_LOGI(TAG, "ffff crc %d \n", crc);
        if (crc == 0)
        {
            memcpy(res_buf, buf, len);
            *res_len = len;
            goto end_read;
        }
    }

    if (len == max_len)
    {
        crc = crc16_calc(res_buf, *res_len);
        ESP_LOGI(TAG, "XXX crc %d \n", crc);
        if (crc == 0)
        {
            memcpy(res_buf, buf, len);
            *res_len = len;
            goto end_read;
        }
    }

    if (len > max_len + 5)
    {
        ESP_LOGI(TAG, "maxlen------%d---%d---\n", max_len, len);
        return ASW_FAIL;
    }
    timeout_cnt = 0; // kaco-lanstick 20220811 +
    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        // if (timeout_cnt >= 60)
        if (timeout_cnt >= 30)
            break;

        // 20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            break;
        }
    }

    if (len > max_len + 5)
    {
        ESP_LOGI(TAG, "maxlen-----%d---%d---\n", max_len, len);
        return ASW_FAIL;
    }

    while (1)
    {
        usleep(50000);
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;

            for (; i < len; i++)
                printf("*%02X", buf[i]);
        }
        else
        {
            if (len > max_len + 5)
            {
                ESP_LOGI(TAG, "maxlen----%d---%d---\n", max_len, len);
                return ASW_FAIL;
            }
            // end_read:
            memcpy(res_buf, buf, len);
            *res_len = len;
            break;
        }
    }
    crc = crc16_calc(res_buf, *res_len);

end_read:
    ESP_LOGI(TAG, "recvv ok %x  %x %d ",
             crc, ((res_buf[*res_len - 2] << 8) | res_buf[*res_len - 1]), len);
    if (len > 1 && 0 == crc)
        return ASW_OK;
    else
        return ASW_FAIL;
}

//-----------------------------------------------------------------//
void flush_serial_port(uint8_t UART_NO)
{
    // [  mark]
    uart_wait_tx_done(UART_NO, 0);
    uart_flush(UART_NO); // uart_tx_flush(0); [  mark]
    uart_flush_input(UART_NO);
}

//-------------------------------------------------//
int8_t clear_inv_list(void)
{
    memset(&inv_arr, 0, sizeof(Inv_arr_t));
    clear_cloud_invinfo(); //

    g_num_real_inv = 0; // will reload register later
    inv_index = 0;
    memset(&inv_log, 0, sizeof(sys_log));
    memset(&com_log, 0, sizeof(sys_log));
    memset(&last_comm, 0, sizeof(sys_log));
    request_data_time = get_second_sys_time();
    read_meter_time = get_msecond_sys_time();
    return ASW_OK;
}

//------------------------------------------------------------------//
int8_t parse_modbus_addr(uint8_t *buf, int len, uint8_t *addr)
{
    if (check_sum(buf, len) != ASW_OK || len != 12)
        return ASW_FAIL;
    memcpy(addr, buf + 3, 1);
    *addr = buf[3];
    return ASW_OK;
}
//---------------------------------------------------------//

// kaco-lanstick 20220801 +

//?
void wait_recv_flush(int interval_ms)
{
    char buf[500] = {0};
    int nread = 0;
    while (1)
    {
        usleep(interval_ms * 1000);
        nread = uart_read_bytes(UART_NUM_1, buf, sizeof(buf), 0);
        if (nread > 0)
        {
            continue;
        }
        else
        {
            return;
        }
    }
}

// void wait_recv_flush_exit(void)
// {
//     wait_recv_flush(80);
// }
//-----------------------------------------------------------------//

SERIAL_STATE upinv_transdata_proc(char func, cloud_inv_msg tans_data)
{
    /** 是否透传电表*/
    // int8_t is_fdbg_meter = 0;
    int i = 0;
    uint16_t data_len = 0;
    unsigned char read_data[300] = {0}, buff_data[300] = {0}, ws[65] = {0};
    int byteLen;
    fdbg_msg_t fdbg_msg;

    for (i = 0; i < 32; i++)
        printf("org---->[%d] =%d \n", i, tans_data.data[i]);

    if (strlen(tans_data.data) - tans_data.len > 64)
        memcpy(ws, &tans_data.data[tans_data.len], 64);
    else
        memcpy(ws, &tans_data.data[tans_data.len], strlen(tans_data.data) - tans_data.len);

    ESP_LOGI(TAG, "rec uuu ws>  %d %d  %d  %d %s\n", func, tans_data.len, strlen(tans_data.data), tans_data.len, ws);

    uart_wait_tx_done(UART_NUM_1, 0);
    // uart_tx_flush(0); [  mark]  没有这个函数 替换为uart_flush(0)
    uart_flush(UART_NUM_1);
    uart_flush_input(UART_NUM_1);

    /* kaco-lanstick 20220801 + */
    if (MODBUS_TCP_MSG_TYPE == func)
    {
        uint8_t rtu[256] = {0};
        int rtu_len = 0;
        char *pdu = tans_data.data;
        int pdu_len = tans_data.len;

        rtu[0] = inv_arr[0].regInfo.modbus_id;
        memcpy(&rtu[1], pdu, pdu_len);
        uint16_t crc = crc16_calc(rtu, pdu_len + 1);
        rtu[pdu_len + 1] = crc & 0xFF;
        rtu[pdu_len + 2] = (crc >> 8) & 0xFF;
        rtu_len = pdu_len + 3;
        printf("rtu send:\n");
        for (i = 0; i < rtu_len; i++)
            printf(" %02X ", rtu[i]);
        printf("\n ");
        is_modbus_tcp_in_use = 0;
        wait_recv_flush(10);
        uart_write_bytes(UART_NUM_1, (const char *)rtu, rtu_len);
        int obj_num = 0;
        switch (rtu[1])
        {
        case 1:
        case 2:
            obj_num = (rtu[4] << 8) + rtu[5];
            int q_num = obj_num / 8;
            int r_num = obj_num % 8;
            if (r_num > 0)
                q_num += 1;
            data_len = q_num + 5;
            break;
        case 3:
        case 4:
            obj_num = (rtu[4] << 8) + rtu[5];
            data_len = obj_num * 2 + 5;
            break;
        case 5:
        case 6:
        case 15:
        case 16:
            data_len = 8;
            break;

        default:
            data_len = 0;
            break;
        }
    }

    if (2 == func || 20 == func)
    {
        byteLen = StrToHex(read_data, tans_data.data, (int)tans_data.len);
        // read_data=strtol( tans_data.data, NULL, 16);[  mark] 可以测试下这个函数
        uart_write_bytes(UART_NUM_1, (const char *)read_data, byteLen);
        ESP_LOGW(TAG, "transdata ");
        for (i = 0; i < byteLen; i++)
            printf("<%02X> ", read_data[i]);
    }

    if (3 == func || 30 == func)
    {
        ESP_LOGW(TAG, "update inverter fw ---%s--- \n ", tans_data.data);
        inv_update(tans_data.data); //[  mark]
        // ESP_LOGW(TAG, "---TGL DEBUG PRINTF inv_updata in update/update.c ");
    }

    if (99 == func)
    {
        ESP_LOGW(TAG, "restart machine data ---%d--- \n ", tans_data.data[0]);
        sleep(2);
        if (tans_data.data[0] == 0 || tans_data.data[0] == 1)
        {
            inv_com_reboot_flg = 1; // reboot inverter
        }
    }

    /** 计算预期返回长度*/

    /* kaco-lanstick 20220801 + */
    if (MODBUS_TCP_MSG_TYPE != func)
        data_len = read_data[5] * 2 + 5; //[  mark ] ？？？如果 func不是2/20 的条件下，read_data=0;

    memset(read_data, 0, sizeof(read_data));

    int8_t res;
    if (MODBUS_TCP_MSG_TYPE != func)
        res = recv_bytes_frame_transform(UART_NUM_1, buff_data, &data_len); // recv_bytes_frame_waitting(UART_NUM_1, buff_data, &data_len);
    else
    {
        res = recv_bytes_frame_waitting_nomd_tcp(UART_NUM_1, buff_data, &data_len); /* kaco-lanstick 20220801 + */
    }
    //------------------------

    ESP_LOGI(TAG, "recvied respons data OK OK%s  %d \n ", buff_data, data_len);
    memset(trans_data.data, 0, sizeof(trans_data.data));
    //------------------------

    memcpy(&trans_data.data, buff_data, data_len);
    trans_data.len = data_len;

    /* kaco-lanstick 20220801 + */
    if (MODBUS_TCP_MSG_TYPE == func)
    {
        tcp_fdbg_msg_t fdbg_msg = {0};

        if (data_len > 3 && res == 0)
        {
            unsigned char *pdu = buff_data + 1;
            int pdu_len = data_len - 3;
            fdbg_msg.len = pdu_len;
            memcpy(fdbg_msg.data, pdu, pdu_len);
        }
        else
        {
            fdbg_msg.len = 0;
        }

        if (to_tcp_task_mq != NULL)
        {
            fdbg_msg.type = MODBUS_TCP_MSG_TYPE;
            if (xQueueSend(to_tcp_task_mq, (void *)&fdbg_msg, 100 / portTICK_RATE_MS) != pdPASS)
            {
                printf("snd to tcp mq fail\n");
            }
        }
    }

    //   printf("\n=================tasns_data.len:%d,data_len:%d,data:%s=======\n", trans_data.len,data_len,trans_data.data);
    if (2 == func)
        trans_resrrpc_pub(&trans_data, ws, data_len); // [  mark] ws没用到
    ESP_LOGW(TAG, "--- DEBUG MARK waiting clound component app_mqtt.c---");

    if (20 == func)
    {
        // fdbg_msg = {0 }; [  mark]
        fdbg_msg.data = calloc(data_len, sizeof(char)); //[  mark]注意free的调用
        fdbg_msg.len = data_len;
        memcpy(fdbg_msg.data, buff_data, data_len);

        if (to_cgi_fdbg_mq != NULL)
        {
            ESP_LOGI(TAG, "cgidata %d \n ", data_len);
            for (i = 0; i < data_len; i++)
                printf("%02X ", fdbg_msg.data[i]);

            fdbg_msg.type = MSG_FDBG;
            if (to_cgi_fdbg_mq != NULL)
            {
                if (xQueueSend(to_cgi_fdbg_mq, (void *)&fdbg_msg, 10 / portTICK_RATE_MS) == pdPASS) //[  mark]注意free的调用
                    ESP_LOGI(TAG, "transform to cgi \n");
            }
            else
            {
                ESP_LOGE(TAG, "ERROR to_cgi_fdbg_mq is null");
            }
        }
    }

    ESP_LOGI(TAG, "recivedata ");
    for (i = 0; i < data_len; i++)
        printf("%02X ", buff_data[i]);

    ESP_LOGI(TAG, "transdata OK OK return 485 ");

    return TASK_QUERY_DATA;
}
//--------------------------------------------//
int8_t save_reg_info(void)
{
    Inv_reg_arr_t info_arr = {0};
    uint8_t i;
    for (i = 0; i < INV_NUM; i++)
    {
        if (inv_arr[i].regInfo.modbus_id != 0)
            ESP_LOGI(TAG, "save modbus id: %02x, sn: %s\n",
                     inv_arr[i].regInfo.modbus_id, inv_arr[i].regInfo.sn);

        memcpy(&info_arr[i], &inv_arr[i].regInfo, sizeof(InvRegister));
    }

    general_add(NVS_INVTER_DB, (void *)info_arr);
    return ASW_OK;
}
//--------------------------------------------//

SERIAL_STATE scan_register(void)
{

    char psn[33] = {0};
    uint8_t i = 0;
    // static uint8_t mCount = 0;
    // int8_t res;

    uint8_t read_frame[500] = {0};
    uint8_t buf[500] = {0};
    uint8_t high_byte = 0;
    uint8_t low_byte = 0;
    uint8_t modbus_id = 0;
    uint8_t modbus_id_ack = 0;

    int set_addr_frame_len = 0;
    int reg_num = 0;
    // int time = 0;
    int abort_cnt = 0;

    // int inv_fd = -1;
    uint16_t len = 0;
    int64_t scan_start_time_sec = esp_timer_get_time(); // 返回为0 ???? [  mark]

    ASW_LOGW("********  debug printf: scan_start_time_sec %lld", scan_start_time_sec);

    { //这是一个串口发送指令流程 A
        ESP_LOGI(TAG, "sacn# ");

        flush_serial_port(UART_NUM_1);
        memset(inv_arr, 0, sizeof(inv_arr)); // clear register
        for (i = 0; i < 3; i++)
        {

            uart_write_bytes(UART_NUM_1, (const char *)clear_register_frame, sizeof(clear_register_frame));

            sleep(1);
        }
    } //这是一个串口发送指令流程 A end

    /**
     * @brief
          //TODO
     *
     */

    memset(&cgi_inv_arr, 0, sizeof(Inv_arr_t));
    while (1)
    {

        { //这是一个串口发送指令流程 B start
            flush_serial_port(UART_NUM_1);
            uart_write_bytes(UART_NUM_1, check_sn_frame, sizeof(check_sn_frame));
            ESP_LOGI(TAG, "read sn frame send\r\n");
            usleep(1000); // kaco-lanstick 20220811 +
            memset(read_frame, 0, sizeof(read_frame));

            recv_bytes_frame_waitting_nomd(UART_NUM_1, read_frame, &len);

            if (read_frame[0] != 0XAA && read_frame[1] != 0X55)
                len = 0;
            if (len == 0)
                abort_cnt++;
            else
                abort_cnt = 0;

            ESP_LOGI(TAG, "end  %lld %lld %d \n", esp_timer_get_time(), scan_start_time_sec, abort_cnt);
            if (abort_cnt > 15 || 1 == g_scan_stop)
            {
                goto FINISH_SCAN;
            }

            ESP_LOGI(TAG, "recv frame\n");
            for (i = 0; i < len; i++)
            {
                printf("%02X ", read_frame[i]);
            }
        } //这是一个串口发送指令流程 B end

        {

            if (len > 0)
            {
                { //这是B串口数据解析流程
                    parse_sn(read_frame, len, psn);
                    if (legal_check(psn) != 0)
                        continue;
                    ESP_LOGI(TAG, "psn: %s\n", psn);
                    memset(buf, 0, sizeof(buf));
                    set_addr_frame_len = 0;

                    memcpy(buf, set_addr_header, sizeof(set_addr_header));
                    set_addr_frame_len += sizeof(set_addr_header);

                    memcpy(buf + set_addr_frame_len, psn, strlen(psn));
                    set_addr_frame_len += sizeof(psn);

                    if (get_modbus_id(&modbus_id) != 0)
                        continue;

                    memcpy(buf + set_addr_frame_len - 1, &modbus_id, 1);

                    get_sum(buf, set_addr_frame_len, &high_byte, &low_byte);
                    memcpy(buf + set_addr_frame_len, &high_byte, 1);
                    set_addr_frame_len += 1;
                    memcpy(buf + set_addr_frame_len, &low_byte, 1);
                    set_addr_frame_len += 1;
                    ESP_LOGI(TAG, "send modbus assign addr frame\n");
                    for (i = 0; i < set_addr_frame_len; i++)
                    {
                        printf("%02X ", buf[i]);
                    }
                } //这是B串口数据解析流程 end

                { //这是串口数据发送流程 C
                    flush_serial_port(UART_NUM_1);
                    uart_write_bytes(UART_NUM_1, buf, set_addr_frame_len);
                    // usleep(20000);
                    memset(read_frame, 0, sizeof(read_frame));
                    recv_bytes_frame_waitting_nomd(UART_NUM_1, read_frame, &len);
                    if (read_frame[0] != 0XAA && read_frame[0] != 0X55)
                        len = 0;
                    ESP_LOGI(TAG, "recv add frame\n");
                    for (i = 0; i < len; i++)
                    {
                        printf("%02X ", read_frame[i]);
                    }
                } //这是串口数据发送流程 C end

                { //这是串口数据解析流程 C
                    if (len > 0)
                    {
                        modbus_id_ack = 0;
                        parse_modbus_addr(read_frame, len, &modbus_id_ack);
                        ESP_LOGI(TAG, "id idack: %02X %02X\n", modbus_id, modbus_id_ack);
                        if (modbus_id != modbus_id_ack)
                            continue;
                        // register one inv ok, please save modbus_id, psn
                        for (i = 0; i < INV_NUM; i++)
                        {
                            ESP_LOGI(TAG, "total add: %02X \n", inv_arr[i].regInfo.modbus_id);

                            if (inv_arr[i].regInfo.modbus_id == 0)
                            {
                                inv_arr[i].regInfo.modbus_id = modbus_id; //如果有多个inv设备，mobus id 是同一个值??? [  mark]

                                memset(inv_arr[i].regInfo.sn, 0, sizeof(inv_arr[i].regInfo.sn));
                                memcpy(inv_arr[i].regInfo.sn, psn, strlen(psn));
                                memcpy(cgi_inv_arr[i].regInfo.sn, psn, strlen(psn));
                                cgi_inv_arr[i].regInfo.modbus_id = modbus_id;

                                // inv_arr_memcpy(&ipc_inv_arr, &inv_arr);
                                // printf("CGI SN: %s \n", inv_arr[i].regInfo.sn);
                                reg_num++;
                                break;
                            }
                        }
                    }
                } //这是串口数据解析流程 C end
            }
            else
            {
                // if (reg_num > 9) //kaco-lanstick 20220811
                if (reg_num > 5)
                    break;
            }
        }
    }

FINISH_SCAN:
    ESP_LOGI(TAG, "end scan \n");
    if (reg_num) //[  upate ] change >=0
    {
        save_reg_info(); /** inv_arr存到另一个地方*/
        ESP_LOGI(TAG, "total register num: %d\n", reg_num);
        reg_info_log();
        ESP_LOGI(TAG, "end **\n");
        usleep(100000);
        // esp_restart();
        clear_inv_list(); /** 清空inv_arr, 重新加载; 此刻不需要清空ipc_inv_arr,从而保证cgi访问连续 */
        g_scan_stop = 0;
    }

    return TASK_IDLE;
}
//-----------------------------------------//
int get_inv_log(char *buf, int typ)
{
    if (typ == 1)
    {
        memcpy(buf, inv_log.tag, strlen(inv_log.tag));
        memcpy(&buf[strlen(inv_log.tag)], inv_log.log, inv_log.index);
        // printf("rrrrrrrrrrrrrrinvlog %s %s\n", inv_log.tag, inv_log.log);
    }

    if (typ == 2)
    {
        memcpy(buf, com_log.tag, strlen(com_log.tag));
        memcpy(&buf[strlen(com_log.tag)], com_log.log, com_log.index);
    }

    if (typ == 3)
    {
        memcpy(buf, last_comm.tag, strlen(last_comm.tag));
        memcpy(&buf[strlen(last_comm.tag)], last_comm.log, strlen(last_comm.log));
    }
    return 0;
}

//----------------------------//

void kaco_change_modbus_addr_then(void)
{
    inv_arr[0].regInfo.modbus_id = g_kaco_set_addr;
    cgi_inv_arr[0].regInfo.modbus_id = g_kaco_set_addr;
    // ipc_inv_arr[0].regInfo.modbus_id = g_kaco_set_addr;  //[mark no used so commit]
}

//-----------------------------//
/* kaco-lanstick 20220811 +  */

SERIAL_STATE md_kaco_set_addr(void)
{
    if (g_num_real_inv == 1 && inv_arr[0].status == 1)
    {
        uint16_t data = g_kaco_set_addr;
        uint8_t addr = inv_arr[0].regInfo.modbus_id;
        int res = modbus_write_inv(addr, 0x0835, 1, &data); // 42102 modbus addr set
        if (res == 0)
        {
            event_group_0 |= KACO_SET_ADDR_DONE;
        }
    }
    return TASK_IDLE;
}