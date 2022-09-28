#include "inv_com.h"
#include <time.h>
#include "stdint.h"
#include "string.h"
// #include "modbus.h"
#include <errno.h>
#include "stdio.h"
#include <pthread.h>
#include "device_data.h"
// #include "asw_sqlite.h"
#include "data_process.h"
#include <sys/time.h>
#include <unistd.h>
// #include <sys/msg.h>

#include "string.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "mqueue.h"
#include "asw_store.h"
#include "data_process.h"
#include "log.h"

sys_log inv_log = {0};
sys_log com_log = {0};
sys_log last_comm = {0};

uint8_t single_multi_mode = MODE_SINGE;
int8_t clear_register_tab_evt_flag = 0;
int8_t monitor_state = 0;
int8_t inv_state = 0;
int inv_index = 0;
int64_t request_data_time = 0, read_meter_time = 0;
extern int scan_stop;

// modbus_t *ctx;
// modbus_t *global_ctx;
extern cloud_inv_msg trans_data;

/* register begin *******************************************/
const uint8_t clear_register_frame[] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x10, 0x04, 0x00, 0x01, 0x14};
const uint8_t check_sn_frame[] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x10, 0x07, 0x00, 0x01, 0x17};
const uint8_t set_addr_header[] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x10, 0x08, 0x21};
Inv_arr_t inv_arr = {0};
Inv_arr_t ipc_inv_arr = {0};
Inv_arr_t cgi_inv_arr = {0};

int inv_com_reboot_flg = 0;
int now_read_inv_index = 0;
int inv_real_num = 0;
int inv_comm_error = 0;
#define msleep(n) usleep(n * 1000)
pthread_mutex_t inv_arr_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int srcLen);
extern void inv_update(char *inv_fw_path);

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#define MAIN_UART_TXD (GPIO_NUM_26)
#define MAIN_UART_RXD (GPIO_NUM_27)

#define CAT1_UART_TXD (GPIO_NUM_18)
#define CAT1_UART_RXD (GPIO_NUM_19)

#define ALL_UART_RTS (UART_PIN_NO_CHANGE)
#define ALL_UART_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

int serialport_init(int type)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    int baud_rate;
    int uart_port;
    int uart_tx;
    int uart_rx;

    switch (type)
    {
    case MAIN_UART:
    {
        baud_rate = 9600;
        uart_port = UART_NUM_1;
        uart_tx = MAIN_UART_TXD;
        uart_rx = MAIN_UART_RXD;
    }
    break;
    case CAT1_UART:
    {
        baud_rate = 115200;
        uart_port = UART_NUM_2;
        uart_tx = CAT1_UART_TXD;
        uart_rx = CAT1_UART_RXD;
        // uart_tx = CAT1_UART_RXD;
        // uart_rx = CAT1_UART_TXD;
    }
    break;
    default:
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
    if (type == CAT1_UART)
        uart_driver_install(uart_port, BUF_SIZE * 4, 0, 0, NULL, 0);
    else
    {
        uart_driver_install(uart_port, BUF_SIZE * 2, 0, 0, NULL, 0);
    }
    return 0;
}

int recv_bytes_frame_waitting_wr(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    char buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    int i = 0;
    uint16_t max_len = *res_len;
    *res_len = 0;
    printf("maxlen---------------------------------%d------\n", max_len);
    /** 第一次读到数据*/
    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
        {
            return -1;
        }
        //20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            if (len >= max_len)
                goto AAA;
            break;
        }
    }

    if (len > max_len)
    {
        printf("maxlen---------------------------------%d---%d---\n", max_len, len);
        return -1;
    }

    /** 判断长度是否足够，直至超时*/
    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
            break;

        //20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            if (len >= max_len)
                goto AAA;
        }
    }

AAA:
    if (len != max_len)
    {
        printf("maxlen---------------------------------%d---%d---\n", max_len, len);
        return -1;
    }
    uint16_t crc = crc16_calc(buf, max_len);

    printf("\nrecvv ok %x  %x %d \n", crc, ((buf[max_len - 2] << 8) | buf[max_len - 1]), len);
    if (len > 1 && 0 == crc)
    {
        memcpy(res_buf, buf, len);
        *res_len = len;
        return 0;
    }
    else
        return -1;
}

int recv_bytes_frame_waitting(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    char buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    int i = 0;
    uint16_t max_len = *res_len;
    printf("maxlen---------------------------------%d------\n", max_len);
    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
        {
            return -1;
        }
        //20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            break;
        }
    }

    if (len > max_len + 5)
    {
        printf("maxlen---------------------------------%d---%d---\n", max_len, len);
        return -1;
    }

    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
            break;

        //20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            break;
        }
    }

    if (len > max_len + 5)
    {
        printf("maxlen---------------------------------%d---%d---\n", max_len, len);
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
                printf("maxlen---------------------------------%d---%d---\n", max_len, len);
                return -1;
            }
            memcpy(res_buf, buf, len);
            *res_len = len;
            break;
            //return 0;
        }
    }

    uint16_t crc = crc16_calc(res_buf, *res_len);

    printf("recvv ok %x  %x %d \n", crc, ((res_buf[*res_len - 2] << 8) | res_buf[*res_len - 1]), len);
    if (len > 1 && 0 == crc) //((res_buf[*res_len-2]<<8)|res_buf[*res_len-1]) == crc)
        return 0;
    else
        return -1;
}

int recv_bytes_frame_waitting_fast(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    char buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    int i = 0;
    uint16_t max_len = *res_len;
    printf("maxlen---------------------------------%d------\n", max_len);
    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
        {
            return -1;
        }
        //20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            break;
        }
    }

    if (len > max_len + 5)
    {
        printf("maxlen---------------------------------%d---%d---\n", max_len, len);
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
                printf("maxlen---------------------------------%d---%d---\n", max_len, len);
                return -1;
            }
            memcpy(res_buf, buf, len);
            *res_len = len;
            break;
            //return 0;
        }
    }

    uint16_t crc = crc16_calc(res_buf, *res_len);

    printf("recvv ok %x  %x %d \n", crc, ((res_buf[*res_len - 2] << 8) | res_buf[*res_len - 1]), len);
    if (len > 1 && 0 == crc) //((res_buf[*res_len-2]<<8)|res_buf[*res_len-1]) == crc)
        return 0;
    else
        return -1;
}

int recv_bytes_frame_waitting_nomd(int fd, uint8_t *res_buf, int *res_len)
{
    char buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    int i = 0;

    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
        {
            return -1;
        }
        //20 / portTICK_RATE_MS
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

    while (1)
    {
        usleep(50000);
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        //printf("recvvv ****************************%d \n", nread);
        if (nread > 0)
        {
            len += nread;
            ;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            printf("\r\n");
        }
        else
        {
            printf("recvvv ****************************888 \n");
            memcpy(res_buf, buf, len);
            *res_len = len;
            //break;
            return 0;
        }
    }
    printf("recvvv -1 \n");
    *res_len = 0;
    return -1;
}

int recv_bytes_frame_transform(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    char buf[500] = {0};
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    int i = 0;
    uint16_t max_len = *res_len;
    uint16_t crc = 0;
    printf("maxlen---------------------------------%d------\n", max_len);
    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
        {
            return -1;
        }
        //20 / portTICK_RATE_MS
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
        crc = crc16_calc(buf, len);
        printf("ffff crc %d \n", crc);
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
        printf("XXX crc %d \n", crc);
        if (crc == 0)
        {
            memcpy(res_buf, buf, len);
            *res_len = len;
            goto end_read;
        }
    }

    if (len > max_len + 5)
    {
        printf("maxlen---------------------------------%d---%d---\n", max_len, len);
        return -1;
    }

    while (1)
    {
        usleep(50000);
        timeout_cnt++;
        if (timeout_cnt >= 60)
            break;

        //20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            for (; i < len; i++)
                printf("*%02X", buf[i]);
            break;
        }
    }

    if (len > max_len + 5)
    {
        printf("maxlen---------------------------------%d---%d---\n", max_len, len);
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
                printf("maxlen---------------------------------%d---%d---\n", max_len, len);
                return -1;
            }
            //end_read:
            memcpy(res_buf, buf, len);
            *res_len = len;
            break;
            //return 0;
        }
    }
    crc = crc16_calc(res_buf, *res_len);

end_read:
    printf("recvv ok %x  %x %d \n", crc, ((res_buf[*res_len - 2] << 8) | res_buf[*res_len - 1]), len);
    if (len > 1 && 0 == crc) //((res_buf[*res_len-2]<<8)|res_buf[*res_len-1]) == crc)
        return 0;
    else
        return -1;
}

int asw_read_input_registers(mb_req_t *mb_req, mb_res_t *mb_res, unsigned char funcode)
{
    uint8_t req_frame[256] = {0};
    char i = 0;
    req_frame[0] = mb_req->slave_id;
    req_frame[1] = funcode; //0x04;
    req_frame[2] = (mb_req->start_addr & 0xFF00) >> 8;
    req_frame[3] = mb_req->start_addr & 0xFF;
    req_frame[4] = (mb_req->reg_num & 0xFF00) >> 8;
    req_frame[5] = mb_req->reg_num & 0xFF;
    uint16_t crc = crc16_calc(req_frame, 6);
    req_frame[6] = crc & 0xFF;
    req_frame[7] = (crc & 0xFF00) >> 8;

    printf("send data ");
    // char *xxx = "12345678";
    // uart_write_bytes(mb_req->fd, (const char *)xxx, 8);
    for (; i < 8; i++)
        printf(" <%02X> ", req_frame[i]);
    printf("-------%d \n", mb_res->len);
    printf(" \n");
    uart_write_bytes(mb_req->fd, (const char *)req_frame, 8);
    //usleep(50000);
    int res = recv_bytes_frame_waitting(mb_req->fd, mb_res->frame, &mb_res->len);
    return res;
}

void flush_serial_port()
{
    uart_wait_tx_done(UART_NUM_1, 0);
    uart_tx_flush(0);
    uart_flush_input(UART_NUM_1);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%--

// int set_modbus_newadd(unsigned char add, unsigned char stime, unsigned int utime)
// {
//     modbus_set_slave(ctx, add);
//     modbus_set_response_timeout(ctx, stime, utime); //sec，usec，超时设置
//     return 0;
// }
int clear_inv_list(void)
{
    memset(&inv_arr, 0, sizeof(Inv_arr_t));
    //memset(&cgi_inv_arr, 0, sizeof(Inv_arr_t));
    clear_cloud_invinfo();
    inv_real_num = 0;
    inv_index = 0;
    memset(&inv_log, 0, sizeof(sys_log));
    memset(&com_log, 0, sizeof(sys_log));
    memset(&last_comm, 0, sizeof(sys_log));
    request_data_time = get_second_sys_time();
    read_meter_time = get_msecond_sys_time();
}
/*----------------------------------------------------------------------------*/
/*! \brief  This function create the com comunication task.
 *  \param  initial_data      [in], The task's data.
 *  \return none.
 *  \see
 *  \note
 */
/*----------------------------------------------------------------------------*/
int query_data_proc()
{
    //static int inv_index = 0;

    //Inverter curr_inv = {0};
    //Inverter *curr_inv_ptr = &curr_inv;
    //Inv_data inv_data = {0};
    //InvDevice inv_info = {0};
    struct tm currtime;
    time_t t = time(NULL);
    unsigned int save_time = 0;
    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;
    //printf("-------------------->  %d\r\n",  inv_arr[0].last_save_time);

    /*read inv data*/
    //memset(&curr_inv, 0, sizeof(curr_inv));

    //inverter_query(curr_inv_ptr);

    //read inv data inv_arr
    //memset(inv_arr, 0, sizeof(Inv_arr_t));
    //printf("data md add %d \n",  inv_arr[0].regInfo.modbus_id);
    //md_read_data(curr_inv_ptr, CMD_READ_INV_DATA);
    if (!inv_real_num)
    {
        inv_real_num = load_reg_info();
        if (inv_real_num > 0)
            internal_inv_log(inv_real_num);
    }

    if (inv_real_num < 1)
    {
        printf(" only one inverter default add 3 \n");
        inv_arr[0].regInfo.modbus_id = 0x03;
        inv_real_num = 1;
        internal_inv_log(0);
    }

    if (inv_index >= inv_real_num)
        inv_index = 0;
    Inverter *curr_inv_ptr = NULL;
    curr_inv_ptr = &inv_arr[inv_index++];

    flush_serial_port();

    if (monitor_state & SYNC_TIME_FROM_CLOUD_INDEX)
        if (curr_inv_ptr->regInfo.syn_time == 0)
            if (md_write_data(curr_inv_ptr, CMD_MD_WRITE_INV_TIME) == RST_OK)
            {
                inv_arr[inv_index - 1].regInfo.syn_time = 1;
                // uptime_print("CMD_MD_WRITE_INV_TIME:----------------------\n");
            }
    printf("read time >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  %d %d \n", monitor_state, curr_inv_ptr->status);
    if (!monitor_state)
    {
        printf("read time ############################# %d %d \n", monitor_state, curr_inv_ptr->status);
        if (curr_inv_ptr->status)
            if (RST_OK == (md_read_data(curr_inv_ptr, CMD_MD_READ_INV_TIME, &save_time, (unsigned short *)&save_time)))
            {
                monitor_state |= SYNC_TIME_FROM_INV_INDEX;
                // uptime_print("CMD_MD_READ_INV_TIME:----------------------\n");
            }
    }
    //inv_arr[inv_index-1].regInfo.syn_time = 2;

    /*4. read data from inverter*/ //&& monitor_state == SYNC_TIME_END
    printf("1111111111 read inv data start... %d  %d %s\n", curr_inv_ptr->regInfo.modbus_id, inv_arr[inv_index - 1].last_save_time, inv_arr[inv_index - 1].regInfo.sn);
    //save_time = inv_arr[inv_index-1].last_save_time;
    int res = md_read_data(curr_inv_ptr, CMD_READ_INV_DATA, &inv_arr[inv_index - 1].last_save_time, &inv_arr[inv_index - 1].last_error);
    if (RST_OK == res)
    {
        // uptime_print("CMD_READ_INV_DATA:----------------------\n");
        //(RST_OK == md_read_data(curr_inv_ptr, CMD_READ_INV_DATA,  &inv_arr[inv_index-1].last_save_time, &inv_arr[inv_index-1].last_error))
        /* Notice:This is very important,if you want change the inv's real data */

        //如果读数据成功，则再次读出逆变器信息并标记“上线”和“同步”标志
        /*check the status of inverter online or offline*/
        inv_arr[inv_index - 1].status = 1; //curr_inv_ptr->status=1;
        //inv_arr[inv_index-1].last_save_time = save_time;
        save_time = inv_arr[inv_index - 1].regInfo.syn_time;
        printf("msw_ver %s \n", inv_arr[inv_index - 1].regInfo.msw_ver);
        if (strlen(inv_arr[inv_index - 1].regInfo.msw_ver) <= 0)
            if (RST_OK == (md_read_data(curr_inv_ptr, CMD_MD_READ_INV_INFO, &inv_index, (unsigned short *)&save_time)))
            {
                // uptime_print("CMD_MD_READ_INV_INFO:----------------------\n");
                printf("INV INFO READ >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                //inv_state |= INV_SYNC_DEV_INDEX;
                //inv_state |= INV_CHECK_ONLINE_INDEX;
            }
            else
            {
                printf("current inverter point is nULL or current inverter's serial number is Unknow!!\r\n!!");
            }
        inv_arr[inv_index - 1].regInfo.syn_time = save_time;
        last_com_data(inv_index);
    }
    else
    {
        inv_com_log(inv_index, res);
        inv_comm_error++;
        inv_arr[inv_index - 1].status = 0; //curr_inv_ptr->status=0;
        printf("read inv data is error\r\n");
    }
    // char i = 0;
    // for (i; i < inv_real_num;)
    //     if (inv_arr[i].status == 1)
    //         i++;
    //     else
    //         break;

    // if (i >= inv_real_num)
    //     ;
    // // led_green_on();
    // else if (0 == i)
    //     ;
    // // led_green_off();
    // else if (0 < i < inv_real_num)
    //     ;
    // // green_fast_blink();

    return TASK_IDLE;
}

int chk_msg(cloud_inv_msg *trans_data)
{
    cloud_inv_msg buf = {0};
    // if (mq1 != NULL)
    // {
    if (xQueueReceive(mq1, &buf, 0) == pdFALSE)
    {
        if (to_inv_fdbg_mq != NULL)
        {
            if (xQueueReceive(to_inv_fdbg_mq, &buf, 0) == pdFALSE)
                return -1;
            //printf("recv msg cgi %d %d \n", buf.data[0], buf.type);
        }
    }
    //ret = msgrcv(id, &buf, sizeof(buf.data), 0, MSG_NOERROR | IPC_NOWAIT); //2
    //printf("recv msg @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

    if (2 == buf.type)
    {
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        /*if(1 == buf.data[buf.len])
            trans_data->ws = buf.data[buf.len+1];
        else if(2 == buf.data[buf.len])
            trans_data->ws = (buf.data[buf.len+1]<<8) | buf.data[buf.len+2];
        */
        char ws[512] = {0};
        memcpy(ws, &trans_data->data[trans_data->len], strlen(trans_data->data)); //-trans_data->len);
        printf("recvmsg %d %d  %d  %d %s\n", sizeof(buf), trans_data->len, strlen(trans_data->data), trans_data->len, ws);

        return 2;
    }
    else if (3 == buf.type)
    {
        printf("recv msg---%s--%d--\n", buf.data, buf.len);
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        trans_data->data[buf.len] = '\0';
        printf("mstyp %d --%s-- %c-- %d --%s--\n", buf.type, buf.data, buf.ws, trans_data->len, trans_data->data);
        trans_data->ws = buf.len;
        return 3;
    }
    else if (5 == buf.type)
    {
        printf("mstyp %d %d %d %d %d\n", buf.type, buf.data[0], buf.data[1], buf.data[2], buf.ws);
        trans_data->ws = buf.data[0];
        return 5;
    }
    else if (6 == buf.type)
    {
        printf("mstyp clear meter %d %s \n", buf.type, buf.data);
        //trans_data->ws = buf.data[0];
        return 6;
    }
    else if (99 == buf.type)
    {
        printf("reboot monitor or inverter %d %d %s \n", buf.type, buf.data[0], &buf.data[1]);
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        //trans_data->ws = buf.data[0];
        return 99;
    }
    else if (30 == buf.type)
    {
        printf("recv cgi msg update---%s--%d--\n", buf.data, buf.len);
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        trans_data->data[buf.len] = '\0';
        printf("mstyp %d --%s-- %c-- %d --%s--\n", buf.type, buf.data, buf.ws, trans_data->len, trans_data->data);
        trans_data->ws = buf.len;
        return 30;
    }
    else if (50 == buf.type)
    {
        printf("recv cgi msg scan---%s--%d--\n", buf.data, buf.len);
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;
        trans_data->data[buf.len] = '\0';
        printf("mstyp %d --%s-- %c-- %d --%s--\n", buf.type, buf.data, buf.ws, trans_data->len, trans_data->data);
        trans_data->ws = buf.len;
        return 50;
    }
    else if (20 == buf.type)
    {
        // uptime_print("KKK:fdbg: com recved cmd 20------------------\n");
        memcpy(trans_data->data, buf.data, sizeof(buf.data));
        trans_data->len = buf.len;

        char ws[512] = {0};
        memcpy(ws, &trans_data->data[trans_data->len], strlen(trans_data->data)); //-trans_data->len);
        printf("cgi trans recvmsg %d %d  %d  %d %s\n", sizeof(buf), trans_data->len, strlen(trans_data->data), trans_data->len, ws);

        return 20;
    }
    else if (60 == buf.type)
    {
        printf("mstyp clear meter %d %s \n", buf.type, buf.data);
        //trans_data->ws = buf.data[0];
        return 60;
    }
    return -1;
}

int8_t inv_task_schedule(char *func, cloud_inv_msg *data)
{
    //static int64_t request_data_time = 0, read_meter_time = 0; // the time for sampling inverter data
    int64_t period = 0;
    int res;

    /*query data from inverter every 10 seconds*/
    //period = (esp_timer_get_time() - request_data_time);
    period = (get_second_sys_time() - request_data_time);

    res = chk_msg(data);
    if (2 == res)
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
    else if (99 == res) //reboot
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

    //printf("*************************query interval %ld %ld \n", get_second_sys_time(), request_data_time);
    if (period >= 10) //period >= QUERY_TIMING * 1000000 || period < 0)
    {
        //printf("query interval %d\r\n", period);
        request_data_time = get_second_sys_time(); //esp_timer_get_time();
        printf("read inv data time sec: %lld\n", request_data_time);
        return TASK_QUERY_DATA;
    }

    if (get_msecond_sys_time() - read_meter_time > 390)
    {
        // printf("read meter interval: %d-----\n", (int)(get_msecond_sys_time() - read_meter_time));
        // printf("query meter %lld %llu \r\n", read_meter_time, read_meter_time);
        read_meter_time = get_msecond_sys_time();
        return TASK_QUERY_METER;
    }
    return TASK_IDLE;
}

extern int g_fdbg_mqtt_index;
int8_t upinv_transdata_proc(int func, cloud_inv_msg tans_data)
{
    int fd, i = 0, j = 0, nread, data_len = 0;
    unsigned char read_data[300] = {0}, buff_data[300] = {0}, ws[65] = {0};

    //printf("rec uuu ws----> %d %d  %d  %d %s\n", func, tans_data.len, strlen(tans_data.data), tans_data.len, &tans_data.data[8]);

    for (i = 0; i < 32; i++)
        printf("org---->[%d] =%d \n", i, tans_data.data[i]);

    if (strlen(tans_data.data) - tans_data.len > 64)
        memcpy(ws, &tans_data.data[tans_data.len], 64);
    else
        memcpy(ws, &tans_data.data[tans_data.len], strlen(tans_data.data) - tans_data.len);
    //printf("rec uuu ws>>>>>>>>> %d %d  %d  %d %s\n", func, tans_data.len, strlen(tans_data.data), tans_data.len, ws);

    uart_wait_tx_done(UART_NUM_1, 0);
    uart_tx_flush(0);
    uart_flush_input(UART_NUM_1);

    if (2 == func)
    {
        if (g_fdbg_mqtt_index == 0) //tianhe
        {
            // int byteLen = StrToHex(read_data, tans_data.data, tans_data.len);
            memcpy(read_data, tans_data.data, tans_data.len);
            int byteLen = tans_data.len;

            printf("transdata ");
            for (i = 0; i < byteLen; i++)
                printf(" %02X ", read_data[i]);
            printf("\n ");

            //write(fd, read_data, byteLen);
            uart_write_bytes(UART_NUM_1, (const char *)read_data, byteLen);
            // sleep(1);
        }
        else if (g_fdbg_mqtt_index == 1) //aliyun
        {
            int byteLen = StrToHex(read_data, tans_data.data, tans_data.len);

            printf("transdata ");
            for (i = 0; i < byteLen; i++)
                printf(" %02X ", read_data[i]);
            printf("\n ");

            //write(fd, read_data, byteLen);
            uart_write_bytes(UART_NUM_1, (const char *)read_data, byteLen);
        }
    }

    if (3 == func)
    {
        printf("update inverter fw ---%s--- \n ", tans_data.data);
        inv_update(tans_data.data);
    }

    if (99 == func)
    {
        printf("restart machine data ---%d--- \n ", tans_data.data[0]);
        sleep(2);
        if (tans_data.data[0] == 0)
            //esp_restart();
            inv_com_reboot_flg = 1;
        if (tans_data.data[0] == 1)
            //esp_restart();          //reboot inverter
            inv_com_reboot_flg = 1;
    }

    if (30 == func)
    {
        printf("cgi update inverter fw ---%s--- \n ", tans_data.data);
        inv_update(tans_data.data);
    }

    if (20 == func)
    {
        int byteLen = StrToHex(read_data, tans_data.data, tans_data.len);

        printf("cgi transdata ");
        for (i = 0; i < byteLen; i++)
            printf(" %02X ", read_data[i]);
        printf("\n ");

        uart_write_bytes(UART_NUM_1, (const char *)read_data, byteLen);
        //sleep(1);
    }

    //memset(read_data, 0, sizeof(read_data));
    data_len = read_data[5] * 2 + 5; //read_data[5]*2+5;//290;
    memset(read_data, 0, sizeof(read_data));
    //usleep(500 * 1000);                                                     //100ms
    int res = recv_bytes_frame_transform(UART_NUM_1, buff_data, &data_len); //recv_bytes_frame_waitting(UART_NUM_1, buff_data, &data_len);

    printf("recvied respons data OK OK %d \n ", data_len);

    memset(trans_data.data, 0, sizeof(trans_data.data));
    memcpy(trans_data.data, buff_data, data_len);
    trans_data.len = data_len;

    if (2 == func)
        trans_resrrpc_pub(&trans_data, ws, data_len);

    if (20 == func)
    {
        fdbg_msg_t fdbg_msg = {0};
        fdbg_msg.data = calloc(data_len, sizeof(char));
        fdbg_msg.len = data_len;
        memcpy(fdbg_msg.data, buff_data, data_len);

        if (to_cgi_fdbg_mq != NULL)
        {
            printf("cgidata %d \n ", data_len);
            for (i = 0; i < data_len; i++)
                printf(" %02X ", fdbg_msg.data[i]);
            printf("\n ");

            fdbg_msg.type = MSG_FDBG;
            if (xQueueSend(to_cgi_fdbg_mq, (void *)&fdbg_msg, 10 / portTICK_RATE_MS) == pdPASS)
                printf("transform to cgi \n");
        }
    }

    printf("recivedata ");
    for (i = 0; i < data_len; i++)
        printf(" %02X ", buff_data[i]);
    printf("\n ");

    printf("transdata OK OK return 485 ");

    return TASK_QUERY_DATA;
}

void reg_info_log(void)
{
    int i;
    printf("scan result:\n");
    for (i = 0; i < INV_NUM; i++)
    {
        if (inv_arr[i].regInfo.modbus_id != 0)
        {
            printf("modbus id: %02x, sn: %s\n", inv_arr[i].regInfo.modbus_id, inv_arr[i].regInfo.sn);
        }
    }
}

int check_inv_states()
{
    char i = 0, j = 0;
    for (i; i < inv_real_num; i++)
        if (inv_arr[i].status == 1)
            j++;

    //printf("--------------------> 55 %d %d %d %d \n", i,inv_real_num, inv_arr[inv_index-1].status, inv_arr[inv_index-1].regInfo.syn_time);
    //printf("--------------------> led %d %d   \n", j,inv_real_num);
    if (j >= inv_real_num)
        return 1; //led_green_on();

    if (j < 1)
        return -1; //led_green_off();

    if (j < inv_real_num && j > 0)
        return 0; // green_fast_blink();
    return j;
}

int get_next_modbus_id(uint8_t *modbus_id)
{
    int i;
    int index;
    for (i = 0; i < INV_NUM; i++)
    {
        index = (now_read_inv_index + 1 + i) % INV_NUM;
        if (inv_arr[index].regInfo.modbus_id != 0)
        {
            *modbus_id = inv_arr[index].regInfo.modbus_id;
            return 0;
        }
    }
    return -1;
}

int load_reg_info(void)
{
    Inv_reg_arr_t reg_info_arr = {0}; //nvs_invert_db
    int i;
    int ret;
    printf("load modbus id---------------------\n");
    ret = general_query(NVS_INVTER_DB, (void *)reg_info_arr);
    printf("load modbus id:********************\n");

    if (-1 == ret)
        return 0;

    for (i = 0; i < INV_NUM; i++)
    {
        if (reg_info_arr[i].modbus_id < 3) //START_ADD)
            break;
        memcpy(&inv_arr[i].regInfo, &reg_info_arr[i], sizeof(InvRegister));
        printf("load modbus id: %02x, sn: %s\n", inv_arr[i].regInfo.modbus_id, inv_arr[i].regInfo.sn);
        // if (inv_arr[i].regInfo.modbus_id < START_ADD)
        //     break;
    }
    printf("inv num %d \n", i);
    return i;
}

int save_reg_info(void)
{
    Inv_reg_arr_t info_arr;
    int i;
    for (i = 0; i < INV_NUM; i++)
    {
        if (inv_arr[i].regInfo.modbus_id != 0)
            printf("save modbus id: %02x, sn: %s\n", inv_arr[i].regInfo.modbus_id, inv_arr[i].regInfo.sn);

        memcpy(&info_arr[i], &inv_arr[i].regInfo, sizeof(InvRegister));
    }

    general_add(NVS_INVTER_DB, (void *)info_arr);
    return 0;
}

// int save_inverter_info(void)
// {
//     sqlite3 *inverterinfo_db;
//     Inv_reg_arr_t info_arr;
//     int i;
//     for (i = 0; i < INV_NUM; i++)
//     {
//         if (inv_arr[i].regInfo.modbus_id != 0)
//         printf("save modbus id: %02x, sn: %s\n", inv_arr[i].regInfo.modbus_id, inv_arr[i].regInfo.sn);

//         memcpy(&info_arr[i], &inv_arr[i].regInfo, sizeof(InvRegister));
//     }

//     sqlite3_open(INVERTER_INFO_PATH, &inverterinfo_db); //SQLITE_OK

//     inverterinfo_create_all_tables(&inverterinfo_db);
//     inverterinfo_add(info_arr, &inverterinfo_db);
//     sqlite3_close(inverterinfo_db);
//     return 0;
// }

int get_sn_by_id(char *sn, uint8_t id)
{
    int i;
    if (id < 3 || id > 247)
        return -1;
    for (i = 0; i < INV_NUM; i++)
    {
        if (inv_arr[i].regInfo.modbus_id == id)
            memcpy(sn, inv_arr[i].regInfo.sn, strlen(inv_arr[i].regInfo.sn));
        return 0;
    }
    return -1;
}

int get_modbus_id(uint8_t *id)
{
    int scan_id;
    int i;

    if (!inv_arr[0].regInfo.modbus_id)
    {
        *id = START_ADD;
        return 0;
    }

    for (i = 0; i < INV_NUM; i++)
        if (inv_arr[i].regInfo.modbus_id == 0 && inv_arr[i - 1].regInfo.modbus_id >= START_ADD)
            break;

    if (inv_arr[i - 1].regInfo.modbus_id < 246)
    {
        *id = inv_arr[i - 1].regInfo.modbus_id + 1;
        printf("next id %d \n", *id);
        return 0;
    }
    else
        return -1;
}

int get_sum(uint8_t *buf, int len, uint8_t *high_byte, uint8_t *low_byte)
{
    uint16_t sum = 0;
    int i = 0;
    for (i = 0; i < len; i++)
        sum += buf[i];
    *high_byte = (sum >> 8) & 0xFF;
    *low_byte = sum & 0xFF;
    printf("high low: %02X %02X\n", *high_byte, *low_byte);
}

int check_sum(uint8_t *buf, int len)
{
    uint8_t h_byte = 0;
    uint8_t l_byte = 0;
    get_sum(buf, len - 2, &h_byte, &l_byte);

    if (h_byte == buf[len - 2] && l_byte == buf[len - 1])
        return 0;
    else
    {
        return -1;
    }
}

int parse_sn(uint8_t *buf, int len, char *psn)
{
    if (check_sum(buf, len) != 0 || len != 45)
    {
        printf("check sum error\n");
        return -1;
    }
    memcpy(psn, buf + 9, 32);
    return 0;
}

int parse_modbus_addr(uint8_t *buf, int len, uint8_t *addr)
{
    if (check_sum(buf, len) != 0 || len != 12)
        return -1;
    memcpy(addr, buf + 3, 1);
    *addr = buf[3];
    return 0;
}

// int recv_frame_old(int fd, uint8_t *buf, int buf_size)
// {
//     int len = 0;
//     int nread = 0;
//     usleep(200 * 1000);
//     while (serialDataAvail(fd) > 0 && buf_size - len > 0)
//     {
//         nread = read(fd, buf + len, buf_size - len);
//         len += nread;
//         usleep(200 * 1000);
//     }
//     return len;
// }

// int recv_frame(int fd, uint8_t *_buf, int buf_size)
// {
//     char buf[500] = {0};
//     int len = 0;
//     int nread = 0;
//     int timeout_cnt = 0;

//     while (1)
//     {
//         usleep(50000);
//         timeout_cnt++;
//         if (timeout_cnt >= 6)
//         {
//             return 0;
//         }
//         if (serialDataAvail(fd) > 0)
//         {
//             nread = read(fd, buf + len, sizeof(buf));
//             len += nread;
//             break;
//         }
//     }

//     while (1)
//     {
//         usleep(50000);
//         if (serialDataAvail(fd) > 0)
//         {
//             nread = read(fd, buf + len, sizeof(buf));
//             len += nread;
//         }
//         else
//         {
//             memcpy(_buf, buf, len);
//             return len;
//         }
//     }
// }

extern Inv_arr_t _inv_arr;
int get_inv_total_num(void)
{
    int i = 0;
    int cnt = 0;
    for (i = 0; i < INV_NUM; i++)
    {
        if (strlen(_inv_arr[i].regInfo.sn) != 0)
            cnt++;
    }
    return cnt;
}

int legal_check(char *text)
{
    if (strlen(text) > 0)
        return 0;
    return -1;
    // int i;
    // for (i = 0; i < strlen(text); i++)
    // {
    //     if ((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'A' && text[i] <= 'Z') || (text[i] >= 'a' && text[i] <= 'z'))
    //     {
    //     }
    //     else
    //     {
    //         return -1;
    //     }
    // }
    // return 0;
}

int scan_register(void)
{
    int i = 0;
    int inv_fd = -1;
    int len = 0;
    uint8_t read_frame[500] = {0};
    uint8_t buf[500] = {0};
    char psn[33] = {0};
    uint8_t high_byte = 0;
    uint8_t low_byte = 0;
    uint8_t modbus_id = 0;
    uint8_t modbus_id_ack = 0;
    int set_addr_frame_len = 0;
    int reg_num = 0;
    int time = 0;
    int abort_cnt = 0;
    int64_t scan_start_time_sec = esp_timer_get_time();
    printf("scann######################################################################\n ");
    flush_serial_port();
    memset(inv_arr, 0, sizeof(inv_arr)); //clear register
    for (i = 0; i < 3; i++)
    {
        uart_write_bytes(UART_NUM_1, (const char *)clear_register_frame, sizeof(clear_register_frame));
        sleep(1);
    }
    printf("end 111111 \n");
    memset(&cgi_inv_arr, 0, sizeof(Inv_arr_t));
    while (1)
    {
        flush_serial_port();
        uart_write_bytes(UART_NUM_1, check_sn_frame, sizeof(check_sn_frame));
        printf("read sn frame send\r\n");
        //usleep(20000);
        memset(read_frame, 0, sizeof(read_frame));
        int res = recv_bytes_frame_waitting_nomd(UART_NUM_1, read_frame, &len);
        if (read_frame[0] != 0XAA && read_frame[1] != 0X55)
            len = 0;
        if (len == 0)
            abort_cnt++;
        else
        {
            abort_cnt = 0;
        }
        printf("end 111111 %d %d %d \n", esp_timer_get_time(), scan_start_time_sec, abort_cnt);
        if (abort_cnt > 15 || 1 == scan_stop) //|| ((esp_timer_get_time() - scan_start_time_sec) > 300))// || 1 == scan_stop) //5min timeout
        {
            // uptime_print("KKK:recv scan stop cmd, invcom.c------------------\n");
            goto FINISH_SCAN;
        }

        /*if(1==scan_stop)
            while (1)
            {
                if(0==scan_stop) break;
                if (abort_cnt > 20 || esp_timer_get_time() - scan_start_time_sec > 300)//5min timeout
                    goto FINISH_SCAN;
                msleep(10);
            }*/

        printf("recv frame\n");
        for (i = 0; i < len; i++)
        {
            printf("%02X ", read_frame[i]);
        }
        printf("\n");
        if (len > 0)
        {
            parse_sn(read_frame, len, psn);
            if (legal_check(psn) != 0)
                continue;
            printf("psn: %s\n", psn);
            memset(buf, 0, sizeof(buf));
            set_addr_frame_len = 0;

            memcpy(buf, set_addr_header, sizeof(set_addr_header));
            set_addr_frame_len += sizeof(set_addr_header);

            memcpy(buf + set_addr_frame_len, psn, strlen(psn));
            set_addr_frame_len += sizeof(psn);

            if (get_modbus_id(&modbus_id) != 0)
                continue;

            //modbus_id = 0x7A;
            memcpy(buf + set_addr_frame_len - 1, &modbus_id, 1);
            //set_addr_frame_len += 1;

            get_sum(buf, set_addr_frame_len, &high_byte, &low_byte);
            memcpy(buf + set_addr_frame_len, &high_byte, 1);
            set_addr_frame_len += 1;
            memcpy(buf + set_addr_frame_len, &low_byte, 1);
            set_addr_frame_len += 1;
            printf("send addr frame\n");
            for (i = 0; i < set_addr_frame_len; i++)
            {
                printf("%02X ", buf[i]);
            }
            flush_serial_port();
            uart_write_bytes(UART_NUM_1, buf, set_addr_frame_len);
            //usleep(20000);
            memset(read_frame, 0, sizeof(read_frame));
            int res = recv_bytes_frame_waitting_nomd(UART_NUM_1, read_frame, &len);
            if (read_frame[0] != 0XAA && read_frame[0] != 0X55)
                len = 0;
            printf("recv add frame\n");
            for (i = 0; i < len; i++)
            {
                printf("%02X ", read_frame[i]);
            }
            printf("\n");
            if (len > 0)
            {
                modbus_id_ack = 0;
                parse_modbus_addr(read_frame, len, &modbus_id_ack);
                printf("id idack: %02X %02X\n", modbus_id, modbus_id_ack);
                if (modbus_id != modbus_id_ack)
                    continue;
                //register one inv ok, please save modbus_id, psn
                for (i = 0; i < INV_NUM; i++)
                {
                    printf("total add: %02X \n", inv_arr[i].regInfo.modbus_id);

                    if (inv_arr[i].regInfo.modbus_id == 0)
                    {
                        inv_arr[i].regInfo.modbus_id = modbus_id;

                        memset(inv_arr[i].regInfo.sn, 0, sizeof(inv_arr[i].regInfo.sn));
                        memcpy(inv_arr[i].regInfo.sn, psn, strlen(psn));
                        memcpy(cgi_inv_arr[i].regInfo.sn, psn, strlen(psn));
                        cgi_inv_arr[i].regInfo.modbus_id = modbus_id;
                        //printf("CGI SN: %s \n", cgi_inv_arr[i].regInfo.sn);
                        reg_num++;
                        break;
                    }
                }
                if (i > 0)
                    break;
            }
        }
        else
        {
            if (reg_num > 0)
                break;
        }
    }

FINISH_SCAN:
    printf("end scan \n");

    if (reg_num)
    {
        // uptime_print("KKK:11111111111111------------------\n");
        save_reg_info();
        printf("total register num: %d\n", reg_num);
        reg_info_log();
        printf("end ************************************\n");
        // uptime_print("KKK:2222222222222------------------\n");
        //esp_restart();
        clear_inv_list();
        // uptime_print("KKK:333333333------------------\n");
        // sleep(1);
    }
    scan_stop = 0;
    return TASK_IDLE;
}

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
                buf[j + 1] = inv_arr[i].regInfo.modbus_id;
                buf[j + 1 + 1] = '\n';
                j += 3;
                printf("buf%d %s \n", j, buf);
            }
        }
        else
        {
            //printf("only one inverter modbus_id %d \n", inv_arr[0].regInfo.modbus_id);
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

    printf("inv_log_enable------------------------ %s \n", inv_log.tag);
    printf("inv_log_enable==================>>>>>>>>>> %s \n", inv_log.log);
}

int inv_com_log(int index, int err)
{
    char buf[300] = {0};
    char sts[16] = {0};
    int j = 0;

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
        //printf("move%d length%d  %d", com_log.index+ strlen(buf) - 330, 330-(com_log.index+ strlen(buf) - 330), com_log.index-(com_log.index+ strlen(buf) - 330));
        char tmp[160] = {0};
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
        //printf("========================new index---- %d \n", com_log.index);
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
}

int last_com_data(int index)
{
    Inv_data invdata = {0};
    if (strlen(last_comm.tag) == 0)
        memcpy(last_comm.tag, "\r\nLast comm\r\n", strlen("\r\nLast comm\r\n"));

    char payload[330] = {0};

    memset(last_comm.log, 0, sizeof(last_comm.log));
    memcpy(&invdata, &cgi_inv_arr[index - 1].invdata, sizeof(Inv_data));

    int datlen = sprintf(payload, "%s %s,%d,%d hz,%d w,%d err,%d pf,%d cf,",
                         invdata.time, invdata.psn, cgi_inv_arr[index - 1].regInfo.modbus_id,
                         invdata.fac, invdata.pac, invdata.error,
                         invdata.cosphi, invdata.col_temp);
    int i;
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
    //printf("lost commm#################### %s \n", last_comm.tag);
    //printf("lost commm#################### %s \n", last_comm.log);
}

int get_inv_log(char *buf, int typ)
{
    if (typ == 1)
    {
        memcpy(buf, inv_log.tag, strlen(inv_log.tag));
        memcpy(&buf[strlen(inv_log.tag)], inv_log.log, inv_log.index);
        //printf("rrrrrrrrrrrrrrinvlog %s %s\n", inv_log.tag, inv_log.log);
    }

    if (typ == 2)
    {
        memcpy(buf, com_log.tag, strlen(com_log.tag));
        memcpy(&buf[strlen(com_log.tag)], com_log.log, com_log.index);
        //printf("rrrrrrrrrrrrrrcom_log %s \n", com_log.log);
    }

    if (typ == 3)
    {
        memcpy(buf, last_comm.tag, strlen(last_comm.tag));
        memcpy(&buf[strlen(last_comm.tag)], last_comm.log, strlen(last_comm.log));
        //printf("rrrrrrrrrrrrrrlast_comm %s \n", last_comm.log);
    }
}

int get_cld_mach_type(void)
{
    for (int j = 0; j < INV_NUM; j++)
    {
        int mach_type = inv_arr[j].regInfo.mach_type;
        int status = inv_arr[j].status;
        if (status == 1)
        {
            return mach_type;
        }
    }

    return 0;
}

/** *********************************************************************************************
 * Modbus rgister read write API
 * **********************************************************************************************
*/

void hex_print(char *buf, int len)
{
    if (len > 0)
    {
        printf("\n");
        for (int i = 0; i < len; i++)
            printf("<%02X> ", buf[i]);
        printf("\n");
    }
}

static int modbus_write(int fd, uint8_t slave_id, uint16_t start_addr, uint16_t reg_num, uint16_t *data)
{
    uint8_t buf[257] = {0};
    buf[0] = slave_id;
    int len = 0;
    int res = 0;
    uint16_t crc = 0;
    if (reg_num == 1)
    {
        buf[1] = 6;
        buf[2] = (start_addr >> 8) & 0xFF;
        buf[3] = start_addr & 0xFF;
        buf[4] = (data[0] >> 8) & 0xFF;
        buf[5] = data[0] & 0xFF;
        crc = crc16_calc(buf, 6);
        buf[6] = crc & 0xFF;
        buf[7] = (crc >> 8) & 0xFF;
        len = 8;
    }
    else
    {
        buf[1] = 16;
        buf[2] = (start_addr >> 8) & 0xFF;
        buf[3] = start_addr & 0xFF;
        buf[4] = (reg_num >> 8) & 0xFF;
        buf[5] = reg_num & 0xFF;
        buf[6] = (uint8_t)(reg_num * 2);
        for (int j = 0; j < reg_num; j++)
        {
            buf[7 + 2 * j] = (data[j] >> 8) & 0xFF;
            buf[8 + 2 * j] = data[j] & 0xFF;
        }
        len = 7 + 2 * reg_num;
        crc = crc16_calc(buf, len);
        buf[len] = crc & 0xFF;
        buf[len + 1] = (crc >> 8) & 0xFF;
        len += 2;
    }
    uart_write_bytes(fd, (const char *)buf, len);
    printf("E-MODBUS WRITE:\n");
    hex_print(buf, len);

    memset(buf, 0, sizeof(buf));
    len = 8;
    res = recv_bytes_frame_waitting_wr(fd, buf, &len);
    if (res == 0)
    {
        if (reg_num == 1)
        {
            if (len == 8)
            {
                if (buf[1] == 6)
                {
                    return 0;
                }
            }
        }
        else
        {
            if (len == 8)
            {
                if (buf[1] == 16)
                {
                    return 0;
                }
            }
        }
    }

    return -1;
}

int modbus_write_inv(uint8_t slave_id, uint16_t start_addr, uint16_t reg_num, uint16_t *data)
{
    return modbus_write(UART_NUM_1, slave_id, start_addr, reg_num, data);
}
