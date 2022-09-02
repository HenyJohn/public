#include "asw_modbus.h"
/*****************************************/
static const char *TAG = "asw_modbus.c";

Inv_arr_t inv_dev_info = {0};
//----------------------------------//
int8_t md_write_inv_time(Inverter *inv_ptr)
{
    //[32][10][03][E8][00][06][0C][07][E4][00][07][00][0F][00][0A][00][27][00][03][B9][2A]
    uint16_t i = 0;
    struct tm currtime = {0};
    time_t t = time(NULL);
    uint16_t crc = 0;
    int8_t res;

    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;
    ESP_LOGI(TAG, "----data-------- %d %d %d %d %d %d \n", currtime.tm_year, currtime.tm_mon,
             currtime.tm_mday, currtime.tm_hour, currtime.tm_min, currtime.tm_sec);

    unsigned char set_time[21] = {0x32, 0x10, 0x03, 0xE8, 0x00, 0x06, 0x0C, 0x07, 0xE4, 0x00, 0x07, 0x00,
                                  0x0F, 0x00, 0x0A, 0x00, 0x27, 0x00, 0x03, 0xB9, 0x2A};
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

    crc = crc16_calc(set_time, 19);
    set_time[19] = crc & 0xFF;
    set_time[20] = (crc & 0xFF00) >> 8;

    // ESP_LOGI(TAG, "send set time >>>>>>>>>>>>>\n");

    for (i = 0; i < 21; i++)
        printf("<%02X> ", set_time[i]);

    uart_write_bytes(UART_NUM_1, (const char *)set_time, 21);
    memset(set_time, 0, 21);
    // i = 8; //[  mark] 在新版本的里面去掉了，有影响吗？？
    ASW_LOGW("recv data write inv time ---,size:%d", i);
    res = recv_bytes_frame_waitting(UART_NUM_1, set_time, &i);

    ASW_LOGW("recv data result,res: %d,len:%d", res, i);

    // len = modbus_write_registers(ctx, 1000, TIME_REG_NUM, inv_time_buf);
    if (i <= 0 || res != ASW_OK)
    {
        ESP_LOGE(TAG, "setting time error \n");
        return ASW_FAIL;
    }
    else
    {
        return ASW_OK;
    }
}

//----------------------------------------//

int8_t md_write_data(Inverter *inv_ptr, Inv_cmd cmd)
{
    int8_t ret = ASW_FAIL;
    sleep(1);
    switch (cmd)
    {
    case CMD_MD_WRITE_INV_TIME:

        ret = md_write_inv_time(inv_ptr);
        break;
    /*for upgrade inverter firmware*/
    default:
        break;
    }
    return ret;
}
//-------------kaco-lanstick 20220811 + ----------//

int mb_write_recv_bytes_frame_waitting(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    uint8_t buf[500] = {0};
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
        // 20 / portTICK_RATE_MS
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

        // 20 / portTICK_RATE_MS
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
//-----------------------------------------//
int8_t recv_bytes_frame_waitting(int fd, uint8_t *res_buf, uint16_t *res_len)
{
    uint8_t buf[500] = {0};
    // uint8_t buf[300] = {0}; //[  mark] 减少大小
    int len = 0;
    int nread = 0;
    int timeout_cnt = 0;
    uint16_t crc;
    int i = 0;
    uint16_t max_len = *res_len;
    *res_len = 0;
    ESP_LOGI(TAG, "maxlen------%d------\n", max_len);

    /** 第一次读到数据*/
    while (1)
    {
        usleep(50000); //
        timeout_cnt++;
        // if (timeout_cnt >= 60)
        if (timeout_cnt >= 300) // kaco-lanstick 20220811 +-
        {
            // ASW_LOGE(" ---  recv inv data time out ");
            return ASW_FAIL;
        }
        // 20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);

        if (nread > 0)
        {
            len += nread;
            // for (; i < len; i++) //kaco-lanstick 20220811+-
            for (; i < 10; i++)
                printf("*%02X", buf[i]);

            printf("\n");

            break;
        }
    }
    ASW_LOGE("\nfirst recv data maxlen:%d,len:%d", max_len, len);

    if (len > max_len + 5)
    {
        printf("recv len is erro:A maxlen--------%d  ----%d \n", max_len, len);
        return ASW_FAIL;
    }
    /** 判断长度是否足够，直至超时*/
    while (1)
    {
        usleep(50000); // [  mark]
        timeout_cnt++;
        // if (timeout_cnt >= 60)  //kaco-lanstick 20220811 +-
        if (timeout_cnt >= 30)
            break;

        // 20 / portTICK_RATE_MS
        nread = uart_read_bytes(fd, buf + len, sizeof(buf) - len, 0);
        if (nread > 0)
        {
            len += nread;
            // for (; i < len; i++)
            for (; i < 10; i++)
                printf("*%02X", buf[i]);
            printf("\n");
            break;
            // if (len >= max_len)
            // goto AAA;
        }
    }

    // AAA: [  update] delete
    if (len > max_len + 5)
    {
        printf("recv len is erro:B maxlen------%d---%d---\n", max_len, len);
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
            printf("\n");
        }
        else
        {
            // ASW_LOGW("--------> print, md read data is finished .... ");
            if (len > max_len + 5)
            {
                printf("recv len is erro: C maxlen----%d---%d---\n", max_len, len);
                return ASW_FAIL;
            }
            memcpy(res_buf, buf, len);
            *res_len = len;
            break;
            // return 0;
        }
    }

    crc = crc16_calc(res_buf, *res_len);

    ESP_LOGI(TAG, "\nrecvv ok %x  %x %d \n", crc, ((res_buf[*res_len - 2] << 8) | res_buf[*res_len - 1]), len);
    if (len > 1 && 0 == crc)
    {
        //     memcpy(res_buf, buf, len);
        //     *res_len = len;
        return ASW_OK;
    }
    else
        return ASW_FAIL;
}
//--------------------------------------------------//

int8_t asw_read_registers(mb_req_t *mb_req, mb_res_t *mb_res, uint8_t funcode)
{
    uint8_t req_frame[256] = {0};
    uint16_t crc;
    uint8_t i = 0;
    req_frame[0] = mb_req->slave_id;
    req_frame[1] = funcode; // 0x04;
    req_frame[2] = (mb_req->start_addr & 0xFF00) >> 8;
    req_frame[3] = mb_req->start_addr & 0xFF;
    req_frame[4] = (mb_req->reg_num & 0xFF00) >> 8;
    req_frame[5] = mb_req->reg_num & 0xFF;
    crc = crc16_calc(req_frame, 6);
    req_frame[6] = crc & 0xFF;
    req_frame[7] = (crc & 0xFF00) >> 8;

    ESP_LOGI(TAG, "send data ");

    // for (i = 0; i < 8; i++)  //kaco-lanstick 20220811 +-
    for (i = 0; i < 2; i++)
    {
        printf("<%02X> ", req_frame[i]);
    }

    printf("\n");
    ESP_LOGI(TAG, "-------%d \n", mb_res->len);

    uart_write_bytes(mb_req->fd, (const char *)req_frame, 8);

    return recv_bytes_frame_waitting(mb_req->fd, mb_res->frame, &mb_res->len);
}

//-----------------------------------------------//
/* kaco-lanstick 20220802 + */
uint8_t get_data_5time(char *time)
{
    char buf[8] = {0};
    memcpy(buf, time + strlen(time) - 5, 2);
    return (uint8_t)atoi(buf);
}
//---------------------------------------------//
/* kaco-lanstick 20220802 + */
int parse_data_5time(char *time)
{
    char buf[8] = {0};
    memcpy(buf, time, 4);
    int year = atoi(buf);

    if (year < 2020 || year > 2038)
        return -2;

    memset(buf, 0, 8);
    memcpy(buf, time + strlen(time) - 5, 2);
    uint8_t min = atoi(buf);

    if (min % 5 == 0 || min == 0)
        return 0;
    return -1;
}
/* kaco-lanstick 20220802 + */
int get_data_time(char *addr, struct tm *rtc_date)
{
    char year[8] = {0};
    char mon[8] = {0};
    char day[8] = {0};
    char hour[8] = {0};
    char min[8] = {0};
    char sec[8] = {0};

    strncpy(year, addr, 4);
    addr = addr + 5;

    strncpy(mon, addr, 2);
    addr = addr + 3;

    strncpy(day, addr, 2);
    addr = addr + 3;

    strncpy(hour, addr, 2);
    addr = addr + 3;

    strncpy(min, addr, 2);
    addr = addr + 3;

    strncpy(sec, addr, 2);

    rtc_date->tm_year = atoi(year);
    rtc_date->tm_mon = atoi(mon);
    rtc_date->tm_mday = atoi(day);
    rtc_date->tm_hour = atoi(hour);
    rtc_date->tm_min = atoi(min);
    rtc_date->tm_sec = atoi(sec);

    return 0;
}
//--------------------------------------------------//
uint8_t last_save_tmin[INV_NUM] = {255, 255, 255, 255, 255, 255};
/* kaco-lanstick 20220802 + - */
int save_inv_data(Inv_data p_data, unsigned int *time, uint16_t *error_code, uint8_t inv_no)
{

    // Inv_data _data = p_data;

    kaco_inv_data_t _kacoInvData;
    _kacoInvData.crt_inv_data = p_data;
    _kacoInvData.crt_inv_index = inv_no;


    struct timeval tv;

    static uint8_t last_save_min = 255; // kaco-lanstick 20220802 +

    printf("\n ========== last save min:%d-%d\n", last_save_tmin[inv_no], get_data_5time(_kacoInvData.crt_inv_data.time));

    /*  kaco-lanstick 20220802 +  */
    if (last_save_tmin[inv_no] != get_data_5time(_kacoInvData.crt_inv_data.time) && parse_data_5time(_kacoInvData.crt_inv_data.time) == 0)
    {
        last_save_tmin[inv_no] = get_data_5time(_kacoInvData.crt_inv_data .time);
        printf("save date--------------------- %s\r\n", _kacoInvData.crt_inv_data.time);
    }
    else
    {
        return 0;
    }

    if ((((_kacoInvData.crt_inv_data.time[0] - 0x30) * 10 + (_kacoInvData.crt_inv_data.time[1] - 0x30)) * 100 +
         (_kacoInvData.crt_inv_data.time[2] - 0x30) * 10 + (_kacoInvData.crt_inv_data.time[3] - 0x30)) < 2020)
        return 0;

    ESP_LOGI(TAG, "read** %s %s %d %d %d\n", _kacoInvData.crt_inv_data.psn, _kacoInvData.crt_inv_data.time,
             (_kacoInvData.crt_inv_data.time[0] - 0x30) * 10 + (_kacoInvData.crt_inv_data.time[1] - 0x30), (_kacoInvData.crt_inv_data.time[2] - 0x30) * 10 + (_kacoInvData.crt_inv_data.time[3] - 0x30),
             (((_kacoInvData.crt_inv_data.time[0] - 0x30) * 10 + (_kacoInvData.crt_inv_data.time[1] - 0x30)) * 100 + (_kacoInvData.crt_inv_data.time[2] - 0x30) * 10 + (_kacoInvData.crt_inv_data.time[3] - 0x30)));

    if (mq0 != NULL && g_kaco_run_mode == 1) // kaco-lanstick 20220802 +
    {
        xQueueSend(mq0, (void *)&_kacoInvData, (TickType_t)0);

        ESP_LOGW(TAG, "----->>>>>mq0 send ok<<<<<-------\n");
    }
    return 0;
}

// kaco-lanstick 20220802 +  TGL MARK
int get_inv_phase_type(uint8_t inv_index)
{
    if (cgi_inv_arr[inv_index].regInfo.type == 0x31 || cgi_inv_arr[inv_index].regInfo.type == 0x33)
    {
        return cgi_inv_arr[inv_index].regInfo.type;
    }
    else
    {
        return 0x31;
    }
}
//--------------------------------------------------//
/*! \brief  This function check the comm receive packet whether is right.*/
int md_query_inv_data(Inverter *inv_ptr, unsigned int *time, uint16_t *error_code)
{
    mb_req_t mb_req = {0};
    mb_res_t mb_res = {0};
    mb_req.fd = UART_NUM_1;
    mb_req.start_addr = 1300;
    mb_req.reg_num = DATA_REG_NUM;
    Inv_data inv_data = {0};
    char now_time[32] = {0};
    uint8_t k = 0;

    mb_req.slave_id = inv_ptr->regInfo.modbus_id;
    mb_res.len = 88 * 2; // mb_res.len = 88 * 2 + 5; [  update]change

    // ASW_LOGW("-----TGL DEBUG, md-read-data --->md_query_inv_data,cmd-adr:  %d", mb_req.start_addr);
    int8_t res = asw_read_registers(&mb_req, &mb_res, 0x04);
    // ASW_LOGW("-----TGL DEBUG, md-read-data --->md_query_inv_data,return code %d", res);
    if (res != ASW_OK)
    {
        uint8_t i = 0;

        for (i = 0; i < 170; i++)
            if (mb_res.frame[i])
                return -1;

        if (i >= 168)
            return -2;
    }

    md_decode_inv_data_content(&mb_res.frame[3], &inv_data);

    total_curt_power = inv_data.pac;

    inv_data.rtc_time = get_time(now_time, sizeof(now_time)) - 8 * 3600;
    strncpy(inv_data.time, now_time, sizeof(inv_data.time));

    memcpy(inv_data.psn, inv_ptr->regInfo.sn, sizeof(inv_ptr->regInfo.sn));

    for (k = 0; k < INV_NUM; k++)
    {
        if (inv_arr[k].regInfo.modbus_id == inv_ptr->regInfo.modbus_id)
        {
            memcpy(&cgi_inv_arr[k].invdata, &inv_data, sizeof(Inv_data));
            break;
        }
    }
    ESP_LOGI(TAG, "read data sn %s %s %d %d\n", inv_data.psn, inv_data.time,
             (inv_data.time[0] - 0x30) * 10 + (inv_data.time[1] - 0x30),
             (inv_data.time[2] - 0x30) * 10 + (inv_data.time[3] - 0x30));
    if (strlen(inv_data.psn) > 10                                    // get_work_mode() == 0
        && get_second_sys_time() > 60 && get_device_sn() == ASW_OK) //检测序列号为空，则不上传云端 [  add 20220622]//180
    {
        save_inv_data(inv_data, time, error_code, k); // kaco-lanstick 20220818 +-
        ASW_LOGW("############# SAVE INV DATA update to cloud");
    }

    return res;
}

//-------------------------------------------------//

int8_t md_query_inv_info(Inverter *inv_ptr, unsigned int *inv_index)
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

    mb_res.len = 75 * 2; // mb_res.len = 75 * 2 + 5; [  update]change
    int8_t res = asw_read_registers(&mb_req, &mb_res, 0x04);

    if (res != ASW_OK)
        return ASW_FAIL;
    md_decode_inv_Info(&mb_res.frame[3], p_device);

    total_rate_power += p_device->rated_pwr;
    ESP_LOGI(TAG, "total_rate_power %d \n", total_rate_power);

    memcpy(&(inv_arr[*inv_index - 1].regInfo.mode_name), p_device->mode_name, sizeof(p_device->mode_name));
    memcpy(&(inv_arr[*inv_index - 1].regInfo.sn), p_device->sn, sizeof(p_device->sn));

    //[  update] change
    // memcpy(&(inv_arr[*inv_index - 1].regInfo), p_device, sizeof(InvRegister));
    memcpy(&(cgi_inv_arr[*inv_index - 1].regInfo), p_device, sizeof(InvRegister));
    memcpy(&(inv_dev_info[*inv_index - 1].regInfo), p_device, sizeof(InvRegister));

    return ASW_OK;
}

//--------------------------------------------------//
/*! \brief  This function query rtc time from inverter.
 *  \param  md_addr        [in],modbus address of inverter
 *  \param  buff           [out],rtc time from invertr
 *  \return bool            The result of query;True:Success,False:faile
 *  \see  从逆变器获取时间信息后，更新到stick系统时间。
 *  \note
 */

int8_t md_read_inv_time(Inverter *inv_ptr)
{
    time_t nowt = time(NULL);
    struct timeval stime;
    mb_req_t mb_req = {0};
    mb_res_t mb_res = {0};
    mb_req.fd = UART_NUM_1;
    mb_req.start_addr = 1000;
    mb_req.reg_num = TIME_REG_NUM;
    // Inv_data inv_data = {0};
    // char now_time[32] = {0};

    mb_req.slave_id = inv_ptr->regInfo.modbus_id;
    mb_res.len = 6 * 2; // mb_res.len = 6 * 2 + 5; [  update]change
    int8_t res = asw_read_registers(&mb_req, &mb_res, 0x03);

    if (res != ASW_OK)
    {
        ESP_LOGW(TAG, "read inv time fun return is not OK!!!\n");
        return ASW_FAIL;
    }

    ESP_LOGI(TAG, "read inv time stat \n");
    uint8_t len = 0;
    // for (len = 0; len < 16; len++)
    for (len = 0; len < 2; len++) // kaco-lanstick 20220811
        printf(TAG, "%d ", mb_res.frame[len]);
    ESP_LOGI(TAG, "time end\n ");

    if (mb_res.frame[2] != 12)
        return ASW_FAIL;
    int iyeartemp = (((uint8_t)mb_res.frame[3] << 8) | ((uint8_t)mb_res.frame[4]));
    if ((iyeartemp >= 2020) && (iyeartemp < 2050))
    {
        struct tm time = {0};
        time.tm_year = iyeartemp - 1900;
        time.tm_mon = (((uint8_t)mb_res.frame[5] << 8) | ((uint8_t)mb_res.frame[6])) - 1;
        time.tm_mday = (((uint8_t)mb_res.frame[7] << 8) | ((uint8_t)mb_res.frame[8]));
        time.tm_hour = (((uint8_t)mb_res.frame[9] << 8) | ((uint8_t)mb_res.frame[10]));
        time.tm_min = (((uint8_t)mb_res.frame[11] << 8) | ((uint8_t)mb_res.frame[12]));
        time.tm_sec = (((uint8_t)mb_res.frame[13] << 8) | ((uint8_t)mb_res.frame[14]));

        ESP_LOGI(TAG, "read local time %ld \n", nowt);

        stime.tv_sec = mktime(&time);
        ESP_LOGI(TAG, "##data-------- %d %d %d %d %d %d \n", time.tm_year, time.tm_mon,
                 time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        ESP_LOGI(TAG, "read local time----- %ld  %ld \n", nowt, stime.tv_sec);

        if (g_monitor_state == 0)
        {
            if (stime.tv_sec > nowt)
                settimeofday(&stime, NULL); //更新时间信息到系统时间变量   mark
        }
        return ASW_OK;
    }
    else
    {
        ESP_LOGI(TAG, "get time year %d err from inv\r\n", iyeartemp);
        return ASW_FAIL;
    }
}
//-------------------------------------------------//

int8_t md_read_data(Inverter *inv_ptr, Inv_cmd cmd, unsigned int *time, uint16_t *error)
{
    int8_t ret = ASW_FAIL;
    /** some type inverter need sleep, like storm*/
    // sleep(1); // return md_query_inv_data(inv_ptr); kaco-lanstick 20220811 -

    switch (cmd)
    {
    case CMD_READ_INV_DATA:
        ret = md_query_inv_data(inv_ptr, time, error);
        // if (ret == 0)  [  update]delete
        // {
        //     read_bat_if_has_estore();
        // }

        break;
    case CMD_MD_READ_INV_INFO:
        ret = md_query_inv_info(inv_ptr, time);
        break;
    case CMD_MD_READ_INV_TIME:
        ret = md_read_inv_time(inv_ptr);
        break;
    default:
        ret = ASW_FAIL;
        break;
    }
    return ret;
}

//-----------------------------------------//
static int8_t modbus_write(int fd, uint8_t slave_id, uint16_t start_addr, uint16_t reg_num, uint16_t *data)
{
    uint8_t buf[257] = {0};
    buf[0] = slave_id;
    uint16_t len = 0;
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
    ESP_LOGI(TAG, "E-MODBUS WRITE:\n");
    // hex_print(buf, len);

    memset(buf, 0, sizeof(buf));
    len = 8;
    ASW_LOGW("recv data modbus write AAAA.....");

    // kaco-lanstick 20220811 +-
    res = mb_write_recv_bytes_frame_waitting(fd, buf, &len);
    // res = recv_bytes_frame_waitting(fd, buf, &len);
    if (res == 0)
    {
        if (reg_num == 1)
        {
            if (len == 8)
            {
                if (buf[1] == 6)
                {
                    return ASW_OK;
                }
            }
        }
        else
        {
            if (len == 8)
            {
                if (buf[1] == 16)
                {
                    return ASW_OK;
                }
            }
        }
    }

    return ASW_FAIL;
}
//---------------------------------------------------//
/** *********************
 * fc: 0x03 or 0x04
 * modbus function code
 * read input or holding registers
 */
static int8_t modbus_read(int fd, uint8_t slave_id, uint8_t fc, uint16_t start_addr, uint16_t reg_num, uint16_t *res_data)
{
    uint8_t buf[257] = {0};
    uint16_t len = 0;
    int res = 0;
    uint16_t crc = 0;

    buf[0] = slave_id;
    buf[1] = fc;
    buf[2] = (start_addr >> 8) & 0xFF;
    buf[3] = start_addr & 0xFF;
    buf[4] = (reg_num >> 8) & 0xFF;
    buf[5] = reg_num & 0xFF;
    crc = crc16_calc(buf, 6);
    buf[6] = crc & 0xFF;
    buf[7] = (crc >> 8) & 0xFF;

    uart_write_bytes(fd, (const char *)buf, 8);
    printf("E-MODBUS READ:\n");
    // hex_print(buf, 8);
    memset(buf, 0, sizeof(buf));
    len = reg_num * 2 + 5;

    ASW_LOGW("recv data modbus read BBBB.....");
    res = recv_bytes_frame_waitting(fd, buf, &len);
    if (res == ASW_OK)
    {
        if (buf[0] == slave_id && buf[1] == fc)
        {
            if (len == 2 + 1 + 2 * reg_num + 2 && buf[2] == 2 * reg_num)
            {
                for (int j = 0; j < reg_num; j++)
                {
                    res_data[j] = (buf[3 + 2 * j] << 8) + buf[4 + 2 * j];
                }
                return ASW_OK;
            }
        }
    }

    return ASW_FAIL;
}
//------------------------------------------------------------------//
int8_t modbus_write_inv(uint8_t slave_id, uint16_t start_addr, uint16_t reg_num, uint16_t *data)
{
    return modbus_write(UART_NUM_1, slave_id, start_addr, reg_num, data);
}

//-----------------------------------------------------------------//
int8_t modbus_holding_read_inv(uint8_t slave_id, uint16_t start_addr, uint16_t reg_num, uint16_t *data)
{
    return modbus_read(UART_NUM_1, slave_id, 0x03, start_addr, reg_num, data);
}
