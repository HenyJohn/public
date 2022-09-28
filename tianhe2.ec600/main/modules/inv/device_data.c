#include "string.h"
#include "device_data.h"
#include "data_process.h"
#include "time.h"
#include "stdio.h"
#include "inv_com.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "datadecode.h"
#include "mqueue.h"
#include "esp_log.h"

cloud_inv_msg trans_data;
Inv_arr_t inv_dev_info = {0};
// extern modbus_t *ctx;
extern int netework_state;
extern Inv_arr_t cgi_inv_arr;
extern int total_rate_power;
extern int total_curt_power;
uint32_t get_time(char *p_date, int len);
int save_inv_data(Inv_data p_data, unsigned int *time, unsigned short *error_code);

void get_pmu_ver(char *ver)
{
    int len = strlen(FIRMWARE_REVISION);
    if (len > 10)
        len = 10;

    memcpy(ver, FIRMWARE_REVISION, len);
}

void short2char(uint8_t *arr1, uint16_t *arr2, int len)
{
    int x = 0;
    int y = 0;
    for (x; x < len; x++)
    {
        arr1[x++] = arr2[y] >> 8;
        if (2 * y + 1 >= len)
            continue;

        arr1[x] = (arr2[y++]) & 0xff;
    }
}
/* below is for ATE */
// int md_byte_query_inv_data(char *buf, int type, int *size)
// {
//     int len;
//     int reg_num;
//     int start_addr;
//     uint16_t reg_buf[125] = {0}; //每帧最多125个寄存器，官方规定
//     memset(reg_buf, 0, sizeof(reg_buf));
//     if (type == 0) //inv data
//     {
//         reg_num = DATA_REG_NUM;
//         start_addr = 1300;
//     }
//     else if (type == 1) //inv info
//     {
//         reg_num = INFO_REG_NUM;
//         start_addr = 1000;
//     }

//     /*1. read  inv data */
//     len = modbus_read_input_registers(ctx, start_addr, reg_num, reg_buf); //1300 88
//     int index = 0;
//     if (len == reg_num)
//     {
//         short2char(buf, reg_buf, 2 * reg_num);
//         *size = reg_num * 2;
//         return 0;
//     }
//     else
//     {
//         return -1;
//     }
// }

// int direct_to_inv_req_handler(uint8_t *req_msg, int *req_len, uint8_t *res_msg, int *res_len)
// {
//     uint8_t buf[6000] = {0};

//     int fd = serialOpen("/dev/ttyHSL0", 9600);
//     serialFlush(fd);
//     printf("send data %02x %02x %02x %02x \n", req_msg[0], req_msg[1], req_msg[2], req_msg[3]);
//     write(fd, req_msg, *req_len);
//     recv_bytes_frame_waitting(fd, res_msg, res_len);
//     printf("transform %02x %02x %02x %02x \n", res_msg[0], res_msg[1], res_msg[2], res_msg[3]);
//     serialClose(fd);
//     return 0;
// }
/*----------------------------------------------------------------------------*/
/*! \brief  This function check the comm receive packet whether is right.*/
uint16_t g_pwr_rate = 0;
uint16_t g_gnd_pe_en = 0;
uint16_t g_grid_switch = 0;
int md_query_inv_data(Inverter *inv_ptr, unsigned int *time, unsigned short *error_code)
{
    mb_req_t mb_req = {0};
    mb_res_t mb_res = {0};
    uint16_t var = 0;
    int rs = -1;
    static int last_sec = -300; //使得开机时读一次
    int this_sec = 0;

    /** 除了invdata，其他是5分钟读一次。以免部分寄存器反应慢阻塞透传*/
    this_sec = get_second_sys_time();
    if (this_sec - last_sec > 300 || event_group_0 & CMD_INV_ONOFF)
    {
        last_sec = this_sec;
        /** read pwr rate*/
        memset(&mb_req, 0, sizeof(mb_req));
        memset(&mb_res, 0, sizeof(mb_res));
        mb_req.fd = UART_NUM_1;
        mb_req.start_addr = 0x151A;
        mb_req.reg_num = 1;
        mb_req.slave_id = inv_ptr->regInfo.modbus_id;
        mb_res.len = 2;
        rs = asw_read_input_registers(&mb_req, &mb_res, 0x03);
        if (mb_res.len == 7 && mb_res.frame[0] == 3 && mb_res.frame[1] == 03 && rs == 0)
        {
            hex_print_frame(mb_res.frame, mb_res.len);
            var = 0;
            var = (mb_res.frame[3] << 8) + mb_res.frame[4];
            if (var >= 0 && var <= 20000)
            {
                g_pwr_rate = var;
                printf("read inv ok: g_pwr_rate=%d\n", (int)g_pwr_rate);
            }
        }

        /** read gnd state*/
        memset(&mb_req, 0, sizeof(mb_req));
        memset(&mb_res, 0, sizeof(mb_res));
        mb_req.fd = UART_NUM_1;
        mb_req.start_addr = 0x0FAB;
        mb_req.reg_num = 1;
        mb_req.slave_id = inv_ptr->regInfo.modbus_id;
        mb_res.len = 2;
        rs = asw_read_input_registers(&mb_req, &mb_res, 0x03);
        if (mb_res.len == 7 && mb_res.frame[0] == 3 && mb_res.frame[1] == 03 && rs == 0)
        {
            hex_print_frame(mb_res.frame, mb_res.len);
            var = 0;
            var = (mb_res.frame[3] << 8) + mb_res.frame[4];
            if (var == 1 || var == 0)
                g_gnd_pe_en = var;
            else
            {
                g_gnd_pe_en = 1;
            }

            printf("read inv ok: g_gnd_pe_en=%d\n", (int)g_gnd_pe_en);
        }

        /** read grid switch */
        memset(&mb_req, 0, sizeof(mb_req));
        memset(&mb_res, 0, sizeof(mb_res));
        mb_req.fd = UART_NUM_1;
        mb_req.start_addr = 0x00C8;
        mb_req.reg_num = 1;
        mb_req.slave_id = inv_ptr->regInfo.modbus_id;
        mb_res.len = 2;
        rs = asw_read_input_registers(&mb_req, &mb_res, 0x03);
        if (mb_res.len == 7 && mb_res.frame[0] == 3 && mb_res.frame[1] == 03 && rs == 0)
        {
            hex_print_frame(mb_res.frame, mb_res.len);
            var = 0;
            var = mb_res.frame[4];
            if (var == 1 || var == 0)
            {
                g_grid_switch = var;
            }
            else
            {
                g_grid_switch = 1;
            }

            printf("read inv ok: g_grid_switch=%d\n", (int)g_grid_switch);

            char tianhe_cmd_onoff = get_tianhe_onoff();
            if (g_grid_switch != tianhe_cmd_onoff && tianhe_cmd_onoff == 0)
            {
                uint_16 data = tianhe_cmd_onoff;
                modbus_write_inv(inv_ptr->regInfo.modbus_id, 0x00C8, 1, &data); /** 电表状态: 40201*/
            }
        }
    }

    /** read inv data*/
    memset(&mb_req, 0, sizeof(mb_req));
    memset(&mb_res, 0, sizeof(mb_res));
    mb_req.fd = UART_NUM_1;
    mb_req.start_addr = 1300;
    mb_req.reg_num = DATA_REG_NUM;
    Inv_data inv_data = {0};
    char now_time[32] = {0};
    mb_req.slave_id = inv_ptr->regInfo.modbus_id;
    mb_res.len = 88 * 2;
    int res = asw_read_input_registers(&mb_req, &mb_res, 0x04);

    if (res < 0)
    {
        int i = 0;
        for (i = 0; i < 170; i++)
            if (mb_res.frame[i])
                return -1;

        if (i >= 168)
            return -2;
    }
    int xres = md_decode_inv_data_content(&mb_res.frame[3], &inv_data);
    if (xres != 0)
    {
        return -3;
    }
    total_curt_power = inv_data.pac;

    inv_data.rtc_time = get_time(now_time, sizeof(now_time)) - 8 * 3600;
    memcpy(inv_data.time, now_time, strlen(now_time));

    memcpy(inv_data.psn, inv_ptr->regInfo.sn, sizeof(inv_ptr->regInfo.sn));
    int k = 0;
    for (k = 0; k < INV_NUM; k++)
    {
        if (inv_arr[k].regInfo.modbus_id == inv_ptr->regInfo.modbus_id)
        {
            //memcpy(&inv_arr[k].invdata, &inv_data, sizeof(Inv_data));
            memcpy(&cgi_inv_arr[k].invdata, &inv_data, sizeof(Inv_data));
            break;
        }
    }
    printf("read data sn %s %s\n", inv_data.psn, inv_data.time);
    if ((strlen(inv_data.psn) > 10 && get_second_sys_time() > 180) || event_group_0 & CMD_INV_ONOFF)
    {
        save_inv_data(inv_data, time, error_code);
    }

    return res;
}

int md_query_inv_info(Inverter *inv_ptr, int *inv_index)
{
    //[31][04][03][E8][00][4B][35][BD]
    mb_req_t mb_req = {0};
    mb_res_t mb_res = {0};
    mb_req.fd = UART_NUM_1;
    mb_req.start_addr = 1000;
    mb_req.reg_num = INFO_REG_NUM;
    InvRegister inv_device = {0};
    InvRegister_ptr p_device = &inv_device;
    mb_req.slave_id = inv_ptr->regInfo.modbus_id;
    mb_res.len = 75 * 2;
    int res = asw_read_input_registers(&mb_req, &mb_res, 0x04);

    if (mb_res.len < 75 * 2)
        return -1;

    if (res < 0)
        return -1;
    md_decode_inv_Info(&mb_res.frame[3], p_device);

    total_rate_power = p_device->rated_pwr;
    printf("total_rate_power %d \n", total_rate_power);

    memcpy(&(inv_arr[*inv_index - 1].regInfo.mode_name), p_device->mode_name, sizeof(p_device->mode_name));
    memcpy(&(inv_arr[*inv_index - 1].regInfo.msw_ver), p_device->msw_ver, sizeof(p_device->msw_ver));
    memcpy(&(inv_arr[*inv_index - 1].regInfo.sn), p_device->sn, sizeof(p_device->sn));
    inv_arr[*inv_index - 1].regInfo.mach_type = p_device->mach_type;

    memcpy(&(cgi_inv_arr[*inv_index - 1].regInfo), p_device, sizeof(InvRegister));
    memcpy(&(inv_dev_info[*inv_index - 1].regInfo), p_device, sizeof(InvRegister));

    // int j = 0;
    // Inv_reg_arr_t inv_info_arr = {0};
    // for (j = 0; j < INV_NUM; j++)
    // {
    //     memcpy(&inv_info_arr[j], &(inv_arr[j].regInfo), sizeof(InvRegister));
    // }
    //save <inv_info_arr> as blob
    return 0;
}
/*----------------------------------------------------------------------------*/
/*! \brief  This function query rtc time from inverter.
  *  \param  md_addr        [in],modbus address of inverter
  *  \param  buff           [out],rtc time from invertr
  *  \return bool            The result of query;True:Success,False:faile
  *  \see
  *  \note
  */
/*----------------------------------------------------------------------------*/
extern int8_t monitor_state;
int md_read_inv_time(Inverter *inv_ptr)
{
    time_t nowt = time(NULL);
    struct timeval stime;
    mb_req_t mb_req = {0};
    mb_res_t mb_res = {0};
    mb_req.fd = UART_NUM_1;
    mb_req.start_addr = 1000;
    mb_req.reg_num = TIME_REG_NUM;
    Inv_data inv_data = {0};
    char now_time[32] = {0};

    mb_req.slave_id = inv_ptr->regInfo.modbus_id;
    mb_res.len = 6 * 2;
    int res = asw_read_input_registers(&mb_req, &mb_res, 0x03);

    if (res < 0)
        return -1;
    //md_decode_inv_data_content(&mb_res.frame[3], &inv_data);
    //get_time(now_time, sizeof(now_time));

    printf("read inv time stat \n");
    char len = 0;
    for (len = 0; len < 16; len++)
        printf("%d ", mb_res.frame[len]);
    printf("time end\n ");

    if (mb_res.frame[2] != 12)
        return -1;
    int iyeartemp = (((uint8_t)mb_res.frame[3] << 8) | ((uint8_t)mb_res.frame[4]));
    if ((iyeartemp >= 2020) && (iyeartemp < 2050))
    {
        if (!monitor_state)
        {
            struct tm time;
            time.tm_year = iyeartemp - 1900;
            time.tm_mon = (((uint8_t)mb_res.frame[5] << 8) | ((uint8_t)mb_res.frame[6])) - 1;
            time.tm_mday = (((uint8_t)mb_res.frame[7] << 8) | ((uint8_t)mb_res.frame[8]));
            time.tm_hour = (((uint8_t)mb_res.frame[9] << 8) | ((uint8_t)mb_res.frame[10]));
            time.tm_min = (((uint8_t)mb_res.frame[11] << 8) | ((uint8_t)mb_res.frame[12]));
            time.tm_sec = (((uint8_t)mb_res.frame[13] << 8) | ((uint8_t)mb_res.frame[14]));

            // time_t nowt = time(0);
            // time_t t = time(NULL);
            printf("read local time %ld \n", nowt);
            //if(iyeartemp*365*24*3600 + )

            stime.tv_sec = mktime(&time);
            printf("############data-------- %d %d %d %d %d %d \n", time.tm_year, time.tm_mon,
                   time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
            printf("read local time----- %ld  %ld \n", nowt, stime.tv_sec);
            if (stime.tv_sec > nowt)
                settimeofday(&stime, NULL);
        }
        return 0;
    }
    else
    {
        printf("get time year %d err from inv\r\n", iyeartemp);
        return -1;
    }
}

// int md_read_inv_time_xxx(Inverter *inv_ptr)
// {

//     int len;
//     struct timeval stime;
//     struct tm time;
//     uint16_t inv_time_buf[125] = {0}; //每帧最多125个寄存器，官方规定
//     uint16_t iyeartemp = 0;

//     modbus_set_slave(ctx, inv_ptr->regInfo.modbus_id);
//     modbus_set_response_timeout(ctx, 0, 500000); //sec，usec，超时设置

//     /*1. read inv time */
//     len = modbus_read_registers(ctx, 1000, 6, inv_time_buf); //3x999,115
//     if (len < 0)
//     {
//         printf("[error] read 4x registers: (%d)\n", len);
//         return -1;
//     }
//     printf("read inv time stat \n");
//     for (len = 0; len < 16; len++)
//         printf("%d ", inv_time_buf[len]);
//     printf("time end\n ");
//     iyeartemp = inv_time_buf[0]; //(((uint8_t)inv_time_buf[0] << 8) | ((uint8_t)inv_time_buf[1]));
//     if ((iyeartemp >= 2019) && (iyeartemp < 2047))
//     {
//         time.tm_year = iyeartemp - 1900;
//         time.tm_mon = inv_time_buf[1] - 1;
//         time.tm_mday = inv_time_buf[2];
//         time.tm_hour = inv_time_buf[3];
//         time.tm_min = inv_time_buf[4];
//         time.tm_sec = inv_time_buf[5];

//         stime.tv_sec = mktime(&time);
//         settimeofday(&stime, NULL);
//     }
//     else
//     {
//         printf("get time year %d err from inv\r\n", iyeartemp);
//         return -1;
//     }

//     return 0;
// }
// int md_read_inv_time_old(Inverter *inv_ptr)
// {

//     int len;
//     struct timeval stime;
//     struct tm time;
//     uint16_t inv_time_buf[125] = {0}; //每帧最多125个寄存器，官方规定
//     uint16_t iyeartemp = 0;

//     modbus_set_slave(ctx, inv_ptr->regInfo.modbus_id);
//     modbus_set_response_timeout(ctx, 0, 500000); //sec，usec，超时设置

//     /*1. read inv time */
//     len = modbus_read_registers(ctx, 1000, 6, inv_time_buf); //3x999,115
//     if (len < 0)
//     {
//         printf("[error] read 4x registers: (%d)\n", len);
//         return -1;
//     }
//     iyeartemp = (((uint8_t)inv_time_buf[0] << 8) | ((uint8_t)inv_time_buf[1]));
//     if ((iyeartemp >= 2019) && (iyeartemp < 2047))
//     {
//         time.tm_year = iyeartemp - 1900;
//         time.tm_mon = inv_time_buf[3] - 1;
//         time.tm_mday = inv_time_buf[5];
//         time.tm_hour = inv_time_buf[7];
//         time.tm_min = inv_time_buf[9];
//         time.tm_sec = inv_time_buf[11];

//         stime.tv_sec = mktime(&time);
//         settimeofday(&stime, NULL);
//     }
//     else
//     {
//         printf("get time year %d err from inv\r\n", iyeartemp);
//         return -1;
//     }

//     return 0;
// }

int md_write_inv_time(Inverter *inv_ptr)
{
    //[32][10][03][E8][00][06][0C][07][E4][00][07][00][0F][00][0A][00][27][00][03][B9][2A]
    int i = 0;
    struct tm currtime;
    time_t t = time(NULL);

    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;
    printf("----data-------- %d %d %d %d %d %d \n", currtime.tm_year, currtime.tm_mon,
           currtime.tm_mday, currtime.tm_hour, currtime.tm_min, currtime.tm_sec);

    unsigned char set_time[21] = {0x32, 0x10, 0x03, 0xE8, 0x00, 0x06, 0x0C, 0x07, 0xE4, 0x00, 0x07, 0x00, 0x0F, 0x00, 0x0A, 0x00, 0x27, 0x00, 0x03, 0xB9, 0x2A};
    set_time[0] = inv_ptr->regInfo.modbus_id;
    set_time[7] = (currtime.tm_year >> 8) & 0XFF;
    set_time[8] = (currtime.tm_year) & 0XFF;

    set_time[9] = (currtime.tm_mon >> 8) & 0XFF;
    set_time[10] = (currtime.tm_mon) & 0XFF;

    set_time[11] = (currtime.tm_mday >> 8) & 0XFF;
    set_time[12] = (currtime.tm_mday) & 0XFF;

    set_time[13] = (currtime.tm_hour >> 8) & 0XFF;
    set_time[14] = (currtime.tm_hour) & 0XFF;

    set_time[15] = (currtime.tm_min >> 8) & 0XFF;
    set_time[16] = (currtime.tm_min) & 0XFF;

    set_time[17] = (currtime.tm_sec >> 8) & 0XFF;
    set_time[18] = (currtime.tm_sec) & 0XFF;

    uint16_t crc = crc16_calc(set_time, 19);
    set_time[19] = crc & 0xFF;
    set_time[20] = (crc & 0xFF00) >> 8;

    printf("send set time >>>>>>>>>>>>>\n");

    for (; i < 21; i++)
        printf(" <%02X> ", set_time[i]);

    uart_write_bytes(UART_NUM_1, (const char *)set_time, 21);
    memset(set_time, 0, 21);
    int res = recv_bytes_frame_waitting(UART_NUM_1, set_time, &i);

    //len = modbus_write_registers(ctx, 1000, TIME_REG_NUM, inv_time_buf);
    if (i < 0)
    {
        printf("setting time error \n");
        return -1;
    }
    else
    {
        return 0;
    }
}

int md_read_data(Inverter *inv_ptr, Inv_cmd cmd, unsigned int *time, unsigned short *error)
{

    int ret = 0;
    sleep(1);
    //return md_query_inv_data(inv_ptr);
    switch (cmd)
    {
    case CMD_READ_INV_DATA:
        ret = md_query_inv_data(inv_ptr, time, error);
        break;
    case CMD_MD_READ_INV_INFO:
        ret = md_query_inv_info(inv_ptr, time);
        break;
    case CMD_MD_READ_INV_TIME:
        ret = md_read_inv_time(inv_ptr);
        break;
    default:
        ret = 1;
        break;
    }
    return ret;
}

int md_write_data(Inverter *inv_ptr, Inv_cmd cmd)
{
    int ret;
    sleep(1);
    switch (cmd)
    {
    case CMD_MD_WRITE_INV_TIME:
        //ret = md_write_MTP_command(inv_ptr, data_ptr, len);
        ret = md_write_inv_time(inv_ptr);
        break;
    /*for upgrade inverter firmware*/
    default:
        break;
    }
    return ret;
}

// unsigned long get_file_size(const char *path)
// {
//     unsigned long filesize = -1;
//     struct stat statbuff;
//     if (stat(path, &statbuff) < 0)
//     {
//         return filesize;
//     }
//     else
//     {
//         filesize = statbuff.st_size;
//     }
//     return filesize;
// }
//write log to logfile. logfile <= 5M

void get_time_old(char *p_date)
{
    time_t t = time(NULL);
    struct tm currtime;
    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;

    sprintf(p_date, "%d-%02d-%02d %02d:%02d:%02d",
            currtime.tm_year,
            currtime.tm_mon,
            currtime.tm_mday,
            currtime.tm_hour,
            currtime.tm_min,
            currtime.tm_sec);
}

uint32_t get_time(char *p_date, int len)
{
    time_t t = time(0);
    struct tm currtime = {0};
    strftime(p_date, len, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &currtime));
    return (uint32_t)t;
}

int64_t get_time_stamp_ms(void)
{
    time_t t = time(0);
    int64_t tms = (t - 8 * 3600) * 1000;
    return tms;
}

void get_time_00sec(char *p_date, int len)
{
    time_t t = time(0);
    strftime(p_date, len, "%Y-%m-%d %H:%M:00", localtime(&t));
}

void get_mobile_time(char *p_date, int len)
{
    time_t t = time(0);
    strftime(p_date, len, "%H:%M %d/%m/%Y", localtime(&t));
}

void get_time_compact(char *p_date, int len)
{
    time_t t = time(0);
    strftime(p_date, len, "%Y%m%d%H%M%S", localtime(&t));
}

int save_inv_data(Inv_data p_data, unsigned int *time, unsigned short *error_code)
{

    static uint16_t last_grid_switch = 8;
    Inv_data _data = p_data;
    //save data every five minute
    struct timeval tv;
    //static uint32_t save_data_time = 0; // the time for sampling inverter data
    int period = 0;

    uint32_t var = get_tianhe_reg_state();
    if (var != 0xAABBCCDD)
    {
        // printf("tianhe not reg, not save\n");
        return 0;
    }
    else
    {
        // printf("tianhe reg done, do save\n");
    }

    if ((*error_code != p_data.error) && (p_data.error != 0xFFFF))
    {
        *error_code = p_data.error;
        printf("find error----->>>> %d %d \n", *error_code, p_data.error);
        if (p_data.error != 0)
        {
            if (event_group_0 & CMD_INV_ONOFF)
            {
                event_group_0 &= ~CMD_INV_ONOFF;
            }
            goto sent_error;
        }
    }
    /*save data every 1 minte*/
    gettimeofday(&tv, NULL);

    period = tv.tv_sec - *time;

    if (period >= SAVE_TIMING || period < 0)
    {
        //printf("save period--------------------- %d\r\n", period);
        //save_data_time = tv.tv_sec;
        *time = tv.tv_sec;
    }
    else if (g_grid_switch != last_grid_switch)
    {
        last_grid_switch = g_grid_switch;
        *time = tv.tv_sec;
    }
    else if (event_group_0 & CMD_INV_ONOFF)
    {
        event_group_0 &= ~CMD_INV_ONOFF;
        *time = tv.tv_sec;
    }
    else
    {
        return 0;
    }

    // if (period >= SAVE_TIMING || period < 0)
    // {
    //     *time = tv.tv_sec - tv.tv_sec % SAVE_TIMING;
    // }
    // else
    // {
    //     return 0;
    // }

    if (strncmp(_data.time, "2020", 4) < 0 || strncmp(_data.time, "2070", 4) > 0)
        return 0;

    /* pass to Message Queue */
    // Inv_data_msg_t inv_data_msg = {0};
    // inv_data_msg.type = 1;
    // memcpy(&inv_data_msg.data, &_data, sizeof(Inv_data));

sent_error:
    // int flag = (inv_data_add(&_data));
    printf("read********************** %s %s %d %d %d\n", _data.psn, _data.time, (_data.time[0] - 0x30) * 10 + (_data.time[1] - 0x30), (_data.time[2] - 0x30) * 10 + (_data.time[3] - 0x30), (((_data.time[0] - 0x30) * 10 + (_data.time[1] - 0x30)) * 100 + (_data.time[2] - 0x30) * 10 + (_data.time[3] - 0x30)));
    //if (0 == netework_state)
    {
        //printf("save #############--------------------- %d\r\n", period);
        if (mq0 != NULL)
        {
            if (is_in_cloud_main_loop == 1 && cat1_get_tcp_status() == 1 && cat1_get_data_call_status() == 1)
            {
                xQueueSend(mq0, (void *)&_data, (TickType_t)0);
                printf("mqueue send ok\n");
            }
            else
            {
                if (xSemaphoreTake(invdata_store_sema, 10000 / portTICK_RATE_MS) == pdTRUE)
                {
                    if (write_lost_data(&_data) < -1)
                        force_flash_reformart("invdata");
                    else
                    {
                        if (check_lost_data() < -1)
                            if (check_inv_pv(&_data) == 0)
                                force_flash_reformart("invdata");
                    }
                    xSemaphoreGive(invdata_store_sema);
                }
            }
        }
        if (_3rd_mq != NULL)
        {
            xQueueSend(_3rd_mq, (void *)&_data, (TickType_t)0);
            printf("_3rd_mq send ok\n");
        }
    }
    // else if (1 == netework_state)
    // {
    //     printf("save ***********--------------------- %d\r\n", period);
    //     inv_data_add(&_data);
    // }
    return 0;
}

int get_current_days(void)
{
    struct tm currtime;
    time_t t = time(NULL);

    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;
    //printf("date-------------------------------------%d \n", currtime.tm_mday);
    return currtime.tm_mday;
    //printf("----data-------- %d %d %d %d %d %d \n", currtime.tm_year,currtime.tm_mon,
    //currtime.tm_mday, currtime.tm_hour, currtime.tm_min, currtime.tm_sec);
}

int parse_sync_time(void)
{
    return 0;
    // struct tm currtime;
    // time_t t = time(NULL);

    // localtime_r(&t, &currtime);
    // currtime.tm_year += 1900;
    // currtime.tm_mon += 1;
    // //printf("year-------------------------------------%d \n", currtime.tm_year);
    // if (currtime.tm_year > 2019)
    //     return 0;
    // return -1;
}

void set_time_cgi(char *str)
{
    struct tm tm = {0};
    struct timeval tv = {0};
    char buf[20] = {0};

    get_time(buf, 20);
    printf("before set: %s\n", buf); ////recv time: 2020 10 22 14 08 13
    // strptime(str, "%+4Y%+2m%+2d%+2H%+2M%+2S", &tm);// sscanf( dtm, "%s %s %d  %d", weekday, month, &day, &year );
    // ESP_LOGE("parse set time", "%04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    sscanf(str, "%4d", &tm.tm_year);
    sscanf(str + 4, "%2d", &tm.tm_mon);
    sscanf(str + 6, "%2d", &tm.tm_mday);
    sscanf(str + 8, "%2d", &tm.tm_hour);
    sscanf(str + 10, "%2d", &tm.tm_min);
    sscanf(str + 12, "%2d", &tm.tm_sec);
    if (tm.tm_year > 2019 && tm.tm_year < 2050)
    {
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        ESP_LOGE("parse set time", "%04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        if (tm.tm_year > 119)
        {
            tv.tv_sec = mktime(&tm);
            settimeofday(&tv, NULL);
            memset(buf, 0, 20);
            get_time(buf, 20);
            printf("after set: %s\n", buf);
        }
    }
}
// if (mq0 != NULL)
// {
//     xQueueSend(mq0, (void *)&inv_data, (TickType_t)0);
//     printf("mqueue send ok\n");
// }
// else
// {
//     printf("mqueue is NULL\n");
// }

// void write_to_log(char *str)
// {
//     int ret = 0;
//     char log[300] = {0};
//     time_t t = time(NULL);
//     struct tm currtime;
//     localtime_r(&t, &currtime);
//     currtime.tm_year += 1900;
//     currtime.tm_mon += 1;

//     snprintf(log, 299, "echo %d-%02d-%02d %02d:%02d:%02d %s >> /data/asw/log",
//              currtime.tm_year, currtime.tm_mon,
//              currtime.tm_mday, currtime.tm_hour,
//              currtime.tm_min, currtime.tm_sec, str);
//     system(log);
//     //10M

//     if (get_file_size("/data/asw/log") >= 10 * 1024 * 1024)
//     {
//         memset(log, 0, 300);
//         snprintf(log, 299, "sed -i '%d,%dd' /data/asw/log", 1, 100);
//         system(log);
//     }

//     return;
// }

// void txt_log(char *describe, char *text)
// {
//     char log[300] = {0};

//     snprintf(log, 299, "%s %s", describe, text);
//     write_to_log(log);
// }

// void num_log(char *describe, int64_t num)
// {
//     char log[300] = {0};

//     snprintf(log, 299, "%s %lld", describe, num);
//     write_to_log(log);
// }
