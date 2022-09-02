#include "meter.h"

static const char *TAG = "meter.c";
//----------------------------------//

int sync_meter_config = 0;

uint16_t pwr_curr_per = 10000;

static int pwr = 1;
Inverter g_inv_meter = {0};

//------------------------------//
EXT_RAM_ATTR meter_setdata g_merter_config = {0}; //[  udpate] add for global sharewith two folder

int total_rate_power = 0;
int total_curt_power = 0;
int16_t meter_real_factory = 0;

// EXT_RAM_ATTR meter_data_t g_meter_inv_meter = {0}; //[  mark]为了项目全局访问，在没有特殊的作用下，去掉了static修饰。 同事 g_inv_meter -> g_inv_meter
// static int g_start_addr = 0x0;        /** 解析电表数据的modbus起始地址，是协议文档里的绝对地址*/
// //-----------------------//

int write_meter_file(meter_setdata *mtconfigx)
{
    general_add(NVS_METER_SET, (void *)mtconfigx);
    return 0;
}

//-------------------------------------//
int parse_sync_time(void)
{
    return 0; //
}

//--------------------------------//
// kaco-lanstick 20220802 +
int get_meter_mem_data(Inv_data *data)
{
    if (g_inv_meter.invdata.status == 0)
        memcpy(data, &g_inv_meter.invdata, sizeof(Inv_data));
    else
    {
        memset(data, 0, sizeof(Inv_data));
    }
    return 0;
}

// kaco-lanstick 20220802 +
int get_meter_modelname(int mod, char *metername)
{
    switch (mod)
    {
    case 0:
        strncpy(metername, "SDM630CT", strlen("SDM630CT")+1);
        break;

    case 1:
        strncpy(metername, "SDM630DC", strlen("SDM630DC")+1);
        break;

    case 2:
        strncpy(metername, "SDM230", strlen("SDM230")+1);
        break;

    case 3:
        strncpy(metername, "SDM220", strlen("SDM220")+1);
        break;

    case 4:
        strncpy(metername, "SDM120", strlen("SDM120")+1);
        break;

    case 5:
        strncpy(metername, "NORK", strlen("NORK")+1);
        break;

    default:
        break;
    }
    return 0;
}

//---------------------------------//
static float iee754_to_float(uint32_t stored_data)
{
    float f;
    char *pbinary = (char *)&stored_data;
    char *pf = (char *)&f;

    memcpy(pf, pbinary, sizeof(float));
    return f;
}
//-------------------------------//

//------------------------------//

int md_decode_meter_pack(uint8_t metmod, uint8_t salveaddr, uint8_t funcode, uint8_t len, uint8_t *buffer, char type)
{
    // static unsigned int meter_data_time = 0;
    // struct timeval tv;

    if (buffer[0] != salveaddr)
    {
        ESP_LOGE(TAG, "get meter data addr err, address=%d\r\n", buffer[0]);
        return -1;
    }
    if (buffer[1] != funcode)
    {
        ESP_LOGE(TAG, "get meter data function code err, func=%d\r\n", buffer[1]);
        return -1;
    }
    if (!type && buffer[2] != 140)
    {
        ESP_LOGE(TAG, "get meter data data length err, data length=%d\r\n", buffer[2]);
        return -1;
    }
    if (!type && len != 145)
    {
        ESP_LOGE(TAG, "get meter data data length err, data length=%d\r\n", len);
        return -1;
    }
    if (crc16_calc(buffer, len) != 0)
    {
        ESP_LOGE(TAG, "get meter data crc err\r\n");
        return -1;
    }
    uint16_t dest[70] = {0};
    uint16_t temp, i;
    /*format two uint8 bytes to one uint16 byte*/
    // printf("\r\n");
    if (!type && len > 0)
    {
        for (i = 0; i < 70; i++)
        {
            /* shift reg hi_byte to temp */
            temp = buffer[3 + i * 2] << 8;
            /* OR with lo_byte           */
            temp = temp | buffer[4 + i * 2];

            dest[i] = temp;
            // printf(" %02x",dest[i]);
        }
    }
    else if (1 == type && len == 9)
    {
        dest[0] = (buffer[3] << 8) | buffer[4];
        dest[1] = (buffer[5] << 8) | buffer[6];
    }

    uint16_t start_addr = 0x0014;
    int tmp = 0;
    float f_tmp = 0.0;
    static uint8_t last_day = 0;

    /* Total system power:2 registers */
    tmp = (dest[0] << 16) + dest[1];
    // ESP_LOGI(TAG,"dest[0]:%02x,dest[1]:%02x,tmp:%02x\r\n",dest[0],dest[1],tmp);
    f_tmp = iee754_to_float(tmp);
    ESP_LOGI(TAG, "pac-------------%f\r\n", f_tmp);
    g_inv_meter.invdata.pac = (int)f_tmp;

    if (type)
    {
        ESP_LOGI(TAG, "pac--###%d###--- \r\n", g_inv_meter.invdata.pac);
        return 0;
    }

    switch (metmod)
    {
    case 0: // Eastron 630 DC
    case 1: // Eastron 630 CT
    {
        /* Total system Apparent power */
        tmp = (dest[4] << 16) + dest[5];
        f_tmp = iee754_to_float(tmp);
        g_inv_meter.invdata.sac = (int)f_tmp;

        /* Total system Reactive power  */
        tmp = (dest[8] << 16) + dest[9];
        f_tmp = iee754_to_float(tmp);
        g_inv_meter.invdata.qac = (int)f_tmp;

        /* Total system Power factor    */
        tmp = (dest[10] << 16) + dest[11];
        // ESP_LOGI(TAG,"factor:%08X\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        // ESP_LOGI(TAG, "factord %d\r\n", (int)(f_tmp * 100), (short)(int)(f_tmp * 10000));
        meter_real_factory = (short)(int)(f_tmp * 10000);
        g_inv_meter.invdata.cosphi = (int8_t)(int)(f_tmp * 100); //功率因数 float->int8 *100

        /* Total system Power Phase angle*/
        tmp = (dest[14] << 16) + dest[15];
        // tmp = dest[14] ;
        // ESP_LOGI(TAG,"Phase angle:%08X\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        // ESP_LOGI(TAG,"Phase angle2:%d\r\n", (sint_16)(int)(f_tmp*1000));
        g_inv_meter.invdata.bus_voltg = (int)(f_tmp * 1000);

        start_addr = 0x0012; // 0x34+0x12=0x46 频率   mark
        /*FAC*/
        tmp = (dest[start_addr] << 16) + dest[start_addr + 1];
        f_tmp = iee754_to_float(tmp);
        g_inv_meter.invdata.fac = (f_tmp * 100);
        ESP_LOGI(TAG, "freq:%f\r\n", f_tmp);

        /* Total Input Energy */
        tmp = (dest[start_addr + 2] << 16) + dest[start_addr + 3];
        f_tmp = iee754_to_float(tmp);
        // meter_data->e_total_in = (int)(f_tmp*10);
        g_inv_meter.invdata.e_total = (int)(f_tmp * 10); // meter e_total(in)

        /* Total Output Energy */
        tmp = (dest[start_addr + 4] << 16) + dest[start_addr + 5];
        f_tmp = iee754_to_float(tmp);
        // meter_data->e_total_out = (int)(f_tmp*10);
        g_inv_meter.invdata.h_total = (int)(f_tmp * 10); ////meter E-Total( out) (old msg)
    }
    break;
    case 2: // Eastron 230
    case 3: // Eastron 220
    case 4: // Eastron 120
    {
        /* Total system Apparent power */
        tmp = (dest[6] << 16) + dest[7];
        f_tmp = iee754_to_float(tmp);
        g_inv_meter.invdata.sac = (int)f_tmp;

        /* Total system Reactive power  */
        tmp = (dest[12] << 16) + dest[13];
        f_tmp = iee754_to_float(tmp);
        g_inv_meter.invdata.qac = (int)f_tmp;

        /* Total system Power factor    */
        tmp = (dest[18] << 16) + dest[19];
        //  tmp=  dest[10] ;
        //                ESP_LOGI(TAG,"factor:%d\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        //                ESP_LOGI(TAG,"factor21123:%d\r\n",(int)(f_tmp*100));
        ESP_LOGI(TAG, "factor%d %d\r\n", (int)(f_tmp * 100), (short)(int)(f_tmp * 10000));
        meter_real_factory = (short)(int)(f_tmp * 10000);
        g_inv_meter.invdata.cosphi = (int)(f_tmp * 100);

        /* Total system Power Phase angle*/
        tmp = (dest[24] << 16) + dest[25];
        // tmp= dest[14] ;
        //                ESP_LOGI(TAG,"Phase angle:%d\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        //                ESP_LOGI(TAG,"Phase angle2:%d\r\n", (sint_16)(int)(f_tmp*1000));
        g_inv_meter.invdata.bus_voltg = (int16_t)(f_tmp * 1000);

        start_addr = 0x003A;
        // FAC
        tmp = (dest[start_addr] << 16) + dest[start_addr + 1];
        f_tmp = iee754_to_float(tmp);
        g_inv_meter.invdata.fac = (f_tmp * 100);

        /* Total Input Energy */
        tmp = (dest[start_addr + 2] << 16) + dest[start_addr + 3];
        f_tmp = iee754_to_float(tmp);
        // meter_data->e_total_in = (int)(f_tmp*10);
        g_inv_meter.invdata.e_total = (int)(f_tmp * 10); // meter e_total(in)

        /* Total Output Energy */
        tmp = (dest[start_addr + 4] << 16) + dest[start_addr + 5];
        f_tmp = iee754_to_float(tmp);
        // meter_data->e_total_out = (int)(f_tmp*10);
        g_inv_meter.invdata.h_total = (int)(f_tmp * 10); // meter E-Total( out) (old msg)
    }
    break;
    default:
        break;
    }
    // FAC 频率
    tmp = (dest[start_addr] << 16) + dest[start_addr + 1];
    f_tmp = iee754_to_float(tmp);
    g_inv_meter.invdata.fac = (f_tmp * 100);

    /* Total Input Energy   正向有功电量 */
    tmp = (dest[start_addr + 2] << 16) + dest[start_addr + 3];
    f_tmp = iee754_to_float(tmp);
    // meter_data->e_total_in = (int)(f_tmp*10);
    g_inv_meter.invdata.e_total = (int)(f_tmp * 10); // meter e_total(in)

    /* Total Output Energy  反向有功电量 */
    tmp = (dest[start_addr + 4] << 16) + dest[start_addr + 5];
    f_tmp = iee754_to_float(tmp);
    // meter_data->e_total_out = (int)(f_tmp*10);
    g_inv_meter.invdata.h_total = (int)(f_tmp * 10); // meter E-Total( out) (old msg)

    ESP_LOGI(TAG, "powerout:%d \n", g_inv_meter.invdata.pac);
    ESP_LOGI(TAG, "factory :%d \n", g_inv_meter.invdata.fac);
    ESP_LOGI(TAG, "cosphi  :%d \n", g_inv_meter.invdata.cosphi);

    //       /* calc E_today I*/
    // rtc_time_s_t curr_date;
    // GetTime_s(&curr_date);

    meter_gendata gendata = {0};
    general_query(NVS_METER_GEN, (void *)&gendata);
    if (!last_day)
    {

        if (get_current_days() != gendata.day)
        {
            // save_days();
            // save_meter_etotal(g_inv_meter.invdata.e_total);
            // save_meter_htotal(g_inv_meter.invdata.h_total);
            memset(&gendata, 0, sizeof(meter_gendata));
            gendata.e_total = g_inv_meter.invdata.e_total;
            gendata.h_total = g_inv_meter.invdata.h_total;
            gendata.day = get_current_days();
            general_add(NVS_METER_GEN, (void *)&gendata);
            g_inv_meter.fir_etotal = g_inv_meter.invdata.e_total;
            g_inv_meter.fir_etotal_out = g_inv_meter.invdata.h_total;
        }
        else if (get_current_days() == gendata.day)
        {
            g_inv_meter.fir_etotal = gendata.e_total;
            g_inv_meter.fir_etotal_out = gendata.h_total;
        }
        last_day = get_current_days();
    }
    else
    {
        if (get_current_days() != gendata.day)
        {
            // save_days();
            // save_meter_etotal(g_inv_meter.invdata.e_total);
            // save_meter_htotal(g_inv_meter.invdata.h_total);
            memset(&gendata, 0, sizeof(meter_gendata));
            gendata.e_total = g_inv_meter.invdata.e_total;
            gendata.h_total = g_inv_meter.invdata.h_total;
            gendata.day = get_current_days();
            general_add(NVS_METER_GEN, (void *)&gendata);

            g_inv_meter.fir_etotal = g_inv_meter.invdata.e_total;
            g_inv_meter.fir_etotal_out = g_inv_meter.invdata.h_total;
            last_day = get_current_days();
        }
    }

    g_inv_meter.invdata.con_stu = (g_inv_meter.invdata.e_total - g_inv_meter.fir_etotal) * 10;
    g_inv_meter.invdata.e_today = (g_inv_meter.invdata.h_total - g_inv_meter.fir_etotal_out) * 10;

    // gettimeofday(&tv, NULL);
    // period = (tv.tv_sec - meter_data_time);
    //逆变器和电表数据同步 上传
    /* get_second_sys_time() - meter_data_time >= 300 && */

    /*
    if (get_second_sys_time() > 180 && (tv.tv_sec - meter_data_time >= 300 || tv.tv_sec - meter_data_time < 0))
    {
        printf("save MMMM--------------------- %ld\r\n", tv.tv_sec - meter_data_time);
        meter_data_time = tv.tv_sec;
    }
    */
    if (g_meter_inv_synupload_flag && get_second_sys_time() - g_uint_meter_data_time < 10)
    {
        // meter_data_time = get_second_sys_time();
        g_meter_inv_synupload_flag = 0;
    }
    else
        return 0;

    // ESP_LOGI(TAG,"meter period--------------- %d %d \n", period, meter_msg_id);
    // send msg
    /* kaco-lanstick 20220811 - */
    // meter_cloud_data trans_meter_cloud = {0};
    // trans_meter_cloud.type = 1;
    // memcpy(&trans_meter_cloud.data, &g_inv_meter.invdata, sizeof(Inv_data));

    // if (mq2 != NULL)
    // {
    //     xQueueSend(mq2, (void *)&trans_meter_cloud, (TickType_t)0);

    //     ESP_LOGI(TAG, "mqueue send ok\n");
    // }

    // msgsnd(meter_msg_id, &trans_meter_cloud, sizeof(trans_meter_cloud.data), IPC_NOWAIT);
    return 0;
}
//-------------------------------------------------//

// kaco-lanstick 20220811 -
#if 0
//[  update] add
int md_decode_meter_pack_abs(uint8_t metmod, uint8_t salveaddr, uint8_t funcode, uint8_t len, uint8_t *buffer, char type, uint32_t *pac)
{
    // static unsigned int meter_data_time = 0;

    if (buffer[0] != salveaddr)
    {
        ESP_LOGE(TAG, "get meter data addr err, address=%d\r\n", buffer[0]);
        return -1;
    }
    if (buffer[1] != funcode)
    {
        ESP_LOGE(TAG, "get meter data function code err, func=%d\r\n", buffer[1]);
        return -1;
    }

    if (crc16_calc(buffer, len) != 0)
    {
        ESP_LOGE(TAG, "get meter data crc err\r\n");
        return -1;
    }
    uint16_t dest[8] = {0};

    if (1 == type && len == 17)
    {
        dest[0] = (buffer[3] << 8) | buffer[4]; // p1
        dest[1] = (buffer[5] << 8) | buffer[6];

        dest[2] = (buffer[7] << 8) | buffer[8]; // p2
        dest[3] = (buffer[9] << 8) | buffer[10];

        dest[4] = (buffer[11] << 8) | buffer[12]; // p3
        dest[5] = (buffer[13] << 8) | buffer[14];
    }

    // uint16_t start_addr = 0x0014;
    // int tmp = 0;
    float f_tmp[3] = {0.0};
    float ph_min = 0.0;

    for (int i = 0, j = 0; i < 6;)
    {
        f_tmp[j++] = iee754_to_float(dest[i] << 16 | dest[i + 1]);
        ESP_LOGI(TAG, "pac %d-------------PW %f\r\n", j, f_tmp[j - 1]);
        i += 2;
    }

    if (f_tmp[0] > f_tmp[1])
        ph_min = f_tmp[1];
    else
        ph_min = f_tmp[0];

    if (ph_min > f_tmp[2])
        ph_min = f_tmp[2];

    ESP_LOGI(TAG, "pac min-------------PWL %f\r\n", ph_min);
    // g_inv_meter.invdata.pac = (int)(ph_min*3.0);
    *pac = (int)(ph_min * 3.0);
    ESP_LOGI(TAG, "pac%f-dest%d- \r\n", ph_min * 3.0, *pac);
    return 0;
}

#endif
//--------------------------------//
//[  update] add
int read_md_meter_pack(uint8_t salveaddr, uint8_t funcode, uint16_t regaddr, uint16_t count, uint8_t *packet)
{
    uint16_t check_crc = 0xffff;
    packet[0] = salveaddr,
    packet[1] = funcode;
    packet[2] = regaddr >> 8;
    packet[3] = regaddr & 0x00ff;
    packet[4] = count >> 8;
    packet[5] = count & 0x00ff;
    uint8_t len = 6;
    check_crc = crc16_calc(packet, len);
    packet[len++] = (uint8_t)(check_crc & 0xff);        // check sum high
    packet[len++] = (uint8_t)((check_crc >> 8) & 0xff); // check sum low
    return len;
}
//-----------------

//------------------------------------------//
SERIAL_STATE clear_meter_setting(char msg)
{
    uint8_t imsg = (uint8_t)msg;

    if (imsg == 6 || imsg == 60)
    {
        ESP_LOGI(TAG, "meter sett clear\n");
        memset(&g_merter_config, 0, sizeof(meter_setdata));
        sync_meter_config = 0;
    }
    return TASK_IDLE;
}

// kaco-lanstick 20220802 +
int get_meter_index(void)
{
    if (g_merter_config.enb == 1)
        return (g_merter_config.mod);
    return -1;
}

//----------------------------------------------//
/* kaco-lanstick 20220811 +- */
char md_query_meter_data(unsigned char meter_mode, unsigned char slaveadd, char type) //, char abs, uint32_t *pac_3ph)
{
    unsigned char funcode = 0x04;
    uint8_t buffer[200] = {0};
    unsigned short send_len = 0;
    unsigned short recv_len = 0;
    // ESP_LOGI(TAG, "===========%d------------%d>>>\n", abs, type);
    switch (meter_mode)
    {
    case 0: // EASTRON630
    case 1: // EASTRON630DC
    {
        if (type)
        {
            // if (abs)
            //     send_len = read_md_meter_pack(slaveadd, funcode, 12, 6, buffer);
            // else
                send_len = read_md_meter_pack(slaveadd, funcode, 0x0034, 02, buffer);
        }
        else
            send_len = read_md_meter_pack(slaveadd, funcode, 0x0034, 70, buffer);
    }
    break;
    case 2:
    case 3:
    case 4:
    case 5:
    {
        if (type)
            send_len = read_md_meter_pack(slaveadd, funcode, 0x000C, 02, buffer);
        else
            send_len = read_md_meter_pack(slaveadd, funcode, 0x000C, 70, buffer);
    }
    break;
    default:
        break;
    }
    uart_write_bytes(UART_NUM_1, (const char *)buffer, send_len);
    memset(buffer, 0, 200);

    if (type)
    {
        // if (abs == 0)
            recv_len = 2 * 2;
        // if (abs == 1)
        //     recv_len = 2 * 6;
    }
    else
        recv_len = 70 * 2;

    int res = recv_bytes_frame_meter(UART_NUM_1, buffer, &recv_len);

    if (res >= 0 && recv_len > 0)
    {
    //     if (type == 1 && abs == 1)
    //         if (0 == md_decode_meter_pack_abs(meter_mode, slaveadd, funcode, recv_len, buffer, 1, pac_3ph))
    //             return 0;

        if (0 == md_decode_meter_pack(meter_mode, slaveadd, funcode, recv_len, buffer, type))
            return 0;
        else
            ESP_LOGE(TAG, "read meter error \n");
    }
    return -1;
}
//===================================================//

/** 电表状态变化时发一次，若逆变器离线，则记下来等到在线时发送 *
 * 触发储能机并网*/
void send_meter_status(int8_t ret)
{
    static uint8_t timeout_cnt = 0;
    static uint8_t last_meter_status = 0;
    uint8_t curr_meter_status = 0;

    if (ret == ASW_OK)
    {
        curr_meter_status = 1; // 1: online 0:offline

        timeout_cnt = 0;
        task_inv_msg |= MSG_PWR_ACTIVE_INDEX; /** 只要读到电表，就会触发 */
    }
    else if (timeout_cnt++ > METER_RECON_TIMES)
    {
        timeout_cnt = 0;
        task_inv_msg |= MSG_INV_SET_ADV_INDEX;
    }
    /** 电表从在线到离线*/
    if (last_meter_status == 1 && curr_meter_status == 0)
    {
        task_inv_msg |= MSG_INV_SET_ADV_INDEX;
    }

    last_meter_status = curr_meter_status;
}

//--------------------------------//
// SIG:UART_NUM_1

static rst_code md_write_SIG_command(const Inverter *inv_ptr, const void *data_ptr /*, uint16_t len*/) //[  mark] len无效
{
    // sint_8 buffer[256] = {0};  //  change [warnning]
    uint8_t buffer[256] = {0};
    uint8_t i;
    uint16_t data_len = 0;
    uint16_t crc_val = 0xFFFF;
    TMD_RTU_WR_MTP_REQ_Packet *spack = NULL;
    spack = (TMD_RTU_WR_MTP_REQ_Packet *)data_ptr;
    memset(buffer, 0x00, 256);
    if (NULL == inv_ptr)
    {
        buffer[0] = 0X00; // broadcast address
        buffer[1] = 0x06;
        buffer[2] = (uint8_t)((spack->reg_addr >> 8) & 0xff);
        buffer[3] = (uint8_t)(spack->reg_addr & 0xff);
        buffer[4] = (uint8_t)(spack->data[0]);
        buffer[5] = (uint8_t)(spack->data[1]);
        crc_val = crc16_calc(buffer, 6);
        buffer[6] = (uint8_t)(crc_val & 0xff);
        buffer[7] = (uint8_t)((crc_val >> 8) & 0xff);
        uart_write_bytes(UART_NUM_1, (const char *)buffer, 8);
    }
    else
    {
        buffer[0] = inv_ptr->regInfo.modbus_id; //,addr;
        buffer[1] = 0x06;
        buffer[2] = (uint8_t)((spack->reg_addr >> 8) & 0xff);
        buffer[3] = (uint8_t)(spack->reg_addr & 0xff);
        buffer[4] = (char)(spack->data[0]);
        buffer[5] = (char)(spack->data[1]);
        crc_val = crc16_calc(buffer, 6);
        buffer[6] = (uint8_t)(crc_val & 0xff);
        buffer[7] = (uint8_t)((crc_val >> 8) & 0xff);
        uart_write_bytes(UART_NUM_1, (const char *)buffer, 8);
    }
    for (i = 0; i < 8; i++)
        ESP_LOGI(TAG, "<%02X> ", buffer[i]);

    data_len = 8;
    memset(buffer, 0, 256);

    // int res =    delete
    ASW_LOGW("recv data wirte SIG cmd write EEEE.....");
    recv_bytes_frame_waitting(UART_NUM_1, buffer, &data_len);

    if (0 != crc16_calc(buffer, data_len))
    {
        ESP_LOGE(TAG, "set fail,timeout,buffer length = %d\r\n", data_len);
        return ERR_CRC;
    }

    if (buffer[1] != 0x06)
    {
        ESP_LOGE(TAG, "rcv write single fun code error \r\n");
        return ERR_FUN_CODE;
    }
    // printf("reg add:2:%02X-%02X; %d-%d\r\n",buffer[2], buffer[3],((buffer[2] << 8) | buffer[3] ), spack->reg_addr);
    if (((buffer[2] << 8) | buffer[3]) != spack->reg_addr)
    {
        ESP_LOGE(TAG, "rcv write single reg add error \r\n");
        return ERR_REG_ADDR;
    }
    if (((buffer[4] == spack->data[0]) && buffer[5] != spack->data[1]))
    {
        ESP_LOGE(TAG, "rcv write single data error \r\n");
        return ERR_DATA;
    }

    return RST_YES;
}
//--------------------------------//
int md_write_active_pwr_fast(const Inverter *inv_ptr, int16_t adjust)
{
    TMD_RTU_WR_MTP_REQ_Packet spack = {0};

    memset(&spack, 0x00, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
    // uint16_t pwr = act_pwr
    spack.reg_addr = 0x154A; // 45402 有功功率控制设置值,154A
    spack.reg_num = 1;
    spack.len = 2;
    spack.data[0] = (adjust >> 8) & 0XFF;
    spack.data[1] = adjust & 0XFF;
    ESP_LOGI(TAG, "%d times adjust fastttt************ %d  \n", pwr++, adjust);
    return md_write_SIG_command(inv_ptr, &spack);
}
//---------------------------------//

int md_write_active_pwr(const Inverter *inv_ptr)
{
    TMD_RTU_WR_MTP_REQ_Packet spack = {0};
    int pwr_temp = pwr_curr_per * 100;
    memset(&spack, 0x00, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
    // uint16_t pwr = act_pwr
    spack.reg_addr = 0x151A; // 45402 有功功率控制设置值
    spack.reg_num = 1;
    spack.len = 2;
    spack.data[0] = (pwr_temp >> 8) & 0XFF;
    spack.data[1] = pwr_temp & 0XFF;
    printf("%d times adjust************ %d \n", pwr++, pwr_temp);
    return md_write_SIG_command(inv_ptr, &spack);
}

//--------------------------------//
int writeRegulatePower_fast(int16_t adjust_num)
{
    if (g_num_real_inv == 1)
    {
        Inverter *inv_ptr = &inv_arr[0];
        return md_write_active_pwr_fast(inv_ptr, adjust_num);
    }
    else
    {
        md_write_active_pwr_fast(NULL, adjust_num);
    }
    return 0;
}
//---------------------------------//
int writeRegulatePower(void)
{
    if (g_num_real_inv == 1)
    {
        Inverter *inv_ptr = &inv_arr[0];
        return md_write_active_pwr(inv_ptr);
    }
    else
    {
        md_write_active_pwr(NULL);
    }
    return 0;
}

/**
 * @brief  此函数仅支持V2.1.1
 *           mark
 *
 * @param set_power
 * @param offset
 * @param pwr_val
 * @return int8_t
 */
//-------------------------------//
/* kaco-lanstick 20220811 +- */
int8_t energy_meter_control(int set_power)  //, int offset, uint32_t pwr_val)
{
    static int adj_step = 0;
    static int steady_cnt = 0;
    static int change_cnt = 0;
    static int first_point = 0;
    static int last_point = 0;
    static int prev_point = 0;
    static int last_diff = 0;

    int curr_point = 100;
    int set_point = 100;
    int curr_diff = 0;
    int curr_add = 0;
    int tmp_diff = 0;

    // float pwr_offset = 0.002;

    uint8_t b_need_adjust = 0;
    char inv_com_protocol[16] = {0};
    if (0 == total_rate_power)
        return ASW_FAIL; // total INV power 6KW+15KW+....
    // Inverter *inv_ptr = &inv_arr[0]; 

    memcpy(inv_com_protocol, &(inv_dev_info[0].regInfo.protocol_ver), 13);
    ESP_LOGI(TAG, "inv protocol ver %s \n", inv_com_protocol); //"cmv": "V2.1.0V2.0.0",

    curr_point = (int)g_inv_meter.invdata.pac;
    set_point = set_power; //来自网络APP,cgi，服务器等调节值2000W

    /* calculate the current diff */
    curr_add = (set_point + curr_point) * 100 / total_rate_power;

    //kaco-lanstick 20220811 -
    // if (offset > 0 && offset <= 100)
    // {
    //     pwr_offset = offset / 1000.0;
    //     curr_point = pwr_val;
    // }

    ESP_LOGI(TAG, "curr_add:%d,set_point:%d,curr_point:%d,inv_total_rated_pac:%d  \n", curr_add, set_point, curr_point, total_rate_power);
    if (strncmp(inv_com_protocol, "V2.1.1", sizeof("V2.1.1")) >= 0)
    {
        float stop_adjust = (set_point + curr_point) * 1.000 / total_rate_power;
        ESP_LOGI(TAG, "stop---------------------------###########################%f  \n", stop_adjust);

        float adjust_per = (set_power + curr_point) * 10000.0 / (total_rate_power * 1.0000);
        int16_t fast_pwr_curr_per = (int16_t)(adjust_per); //(set_power+total_curt_power+curr_point)*100.0/total_rate_power;

        ESP_LOGI(TAG, "regulate---------------------> %d %d %d %f\n", fast_pwr_curr_per, curr_add, (set_power + curr_point), adjust_per);

        if (stop_adjust <= 0.02 && stop_adjust >= 0)
        {
            ESP_LOGI(TAG, "stop adjust \n");
            return ASW_OK;
        }

        ESP_LOGI(TAG, "NNNN regulate********** %d \n", fast_pwr_curr_per);
        if (fast_pwr_curr_per > 10000)
            fast_pwr_curr_per = 10000;

        if (fast_pwr_curr_per < -10000)
            fast_pwr_curr_per = -10000;

        return writeRegulatePower_fast(fast_pwr_curr_per);
    }

    if ((curr_add >= 0) && (curr_add < 2))
    {
        ESP_LOGI(TAG, "Steady state ready\r\n");
        steady_cnt++;
        if (steady_cnt >= 2) // No change, it's Steady state
        {
            adj_step = 0;
            change_cnt = 0;
            steady_cnt = 0;
        }
        return ASW_OK;
    }
    /* Begin to adjust, forcast initial value */
    ESP_LOGI(TAG, "adj_step:%d\r\n", adj_step);
    if (0 == adj_step)
    {
        first_point = curr_point + total_rate_power;
        last_point = curr_point;
        b_need_adjust = 1;
        ESP_LOGI(TAG, "first_point:%d curr_point:%d inv_total_rated_pac:%d\r\n", first_point, curr_point, total_rate_power);
        // task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
        ESP_LOGI(TAG, "Forcast curr:%d pwr_limit:%d\r\n", pwr_curr_per, pwr_curr_per + curr_add);
    }
    else
    {
        if (steady_cnt >= 4) // No any change, timeout
        {
            // task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
            b_need_adjust = 1;
            ESP_LOGI(TAG, "No change,timeout b_need_adjust :%d\r\n", b_need_adjust);
        }
        else
        {

            curr_diff = ((last_point - curr_point) * 100 / total_rate_power);
            tmp_diff = ((prev_point - last_point) * 100 / total_rate_power);
            ESP_LOGI(TAG, "curr_diff-----------tmp_diff------------%d %d \n", curr_diff, tmp_diff);
            if (curr_diff == 0) // No changed state
            {
                steady_cnt++;
                if (steady_cnt >= 1 && adj_step == 2)
                {
                    // task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
                    b_need_adjust = 1;
                    ESP_LOGI(TAG, "No change state\r\n");
                }
            }
            else // Changed state
            {
                ESP_LOGI(TAG, "Change state\r\n");
                if ((curr_diff * tmp_diff) < 0)
                    change_cnt++;
                /* changed more than 6 times,but can't reach set point. */
                if (change_cnt >= 4)
                {
                    // task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
                    b_need_adjust = 1;
                    ESP_LOGI(TAG,
                             "Change more than 6 times, but can't reach set point,so adjust it\r\n");
                }
                adj_step = 2;
                steady_cnt = 0;
                last_point = curr_point;
            }
        }
    }
    if (1 == b_need_adjust)
    {
        curr_diff = ((first_point - curr_point) * 100 / total_rate_power);

        ESP_LOGI(TAG, "Fir_point:%d curr_point:%d curr_diff:%d last_diff:%d curr_add:%d\r\n",
                 first_point,
                 curr_point, curr_diff, last_diff, curr_add);
        if (curr_diff == 0 && last_diff == 0)
        {
            pwr_curr_per = total_curt_power / total_rate_power;
            ESP_LOGI(TAG, "re init curr_per:%d\r\n", pwr_curr_per);
        }

        ESP_LOGI(TAG, "pwr_curr_per 1111---------------> %d %d\r\n", pwr_curr_per, curr_add);

        if (abs(curr_add) > pwr_curr_per)
        {
            ESP_LOGI(TAG, "pwr_curr_per 33---------------> %d %d\r\n", pwr_curr_per, curr_add);
            pwr_curr_per = (unsigned short)abs(pwr_curr_per + curr_add);
        }
        else
        {
            ESP_LOGI(TAG, "pwr_curr_per 55---------------> %d %d\r\n", pwr_curr_per, curr_add);
            pwr_curr_per = pwr_curr_per + curr_add;
        }
        ESP_LOGI(TAG, "pwr_curr_per 222 ----------->%d\r\n", pwr_curr_per);

        last_diff = curr_diff;

        prev_point = last_point;
        first_point = curr_point;
        last_point = curr_point;
        steady_cnt = 0;
        change_cnt = 0;
        adj_step = 1;
        if (pwr_curr_per > 100)
        {
            pwr_curr_per = 100;
            ESP_LOGI(TAG, "pwr_curr_per 6666 ----------->%d\r\n", pwr_curr_per);
        }
        else if (pwr_curr_per <= 0)
        {
            pwr_curr_per = 1;
        }
        // task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
        writeRegulatePower();
    }
    return ASW_OK;
    // rtc_to_date_buffer(rtc_value, NULL);
}
//--------------------------------------//
int md_write_reactive_pwr_factory(const Inverter *inv_ptr, short factory)
{
    TMD_RTU_WR_MTP_REQ_Packet spack = {0};
    memset(&spack, 0x00, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
    spack.reg_addr = 0x157E; // 45402 有功功率控制设置值,154A
    spack.reg_num = 1;
    spack.len = 2;
    spack.data[0] = (factory >> 8) & 0XFF;
    spack.data[1] = factory & 0XFF;
    ESP_LOGI(TAG, "reactive_pwr_factory %d  \n", factory);
    return md_write_SIG_command(inv_ptr, &spack);
}

//-------------------------------//
void writeReative_factory(short fac)
{
    short facx = fac;
    if (facx > 10000)
        facx = 10000;
    if (facx < -10000)
        facx = -10000;

    if (g_num_real_inv == 1)
    {
        Inverter *inv_ptr = &inv_arr[0];
        md_write_reactive_pwr_factory(inv_ptr, facx);
    }
}

//-----------------------------------------------//
//-----------------------------------------------//
/* kaco-lanstick 20220811 +- */
SERIAL_STATE query_meter_proc(void)
{
    int ret = -1;
    static int read_time = 0;
    static int pf_time = 0;

    static int meter_read_fail_sec = 0;

    // uint32_t pwr_abs_val = 0;
    // static int pwr_abs_offset = 0;
    //----------------------
    // struct tm currtime = {0};
    // time_t t = time(NULL);

    // localtime_r(&t, &currtime);
    // currtime.tm_year += 1900;
    // currtime.tm_mon += 1;
    //--------------------------------------
    if (!g_merter_config.date)
    {

        if (0 == parse_sync_time() && sync_meter_config == 0)
        {
            general_query(NVS_METER_SET, (void *)&g_merter_config);
            ESP_LOGI(TAG, "read ---->enb:%d, mod:%d, regulate:%d, target:%d, date:%d pf:%d tpf:%d \n",
                     g_merter_config.enb, g_merter_config.mod, g_merter_config.reg, g_merter_config.target, g_merter_config.date, g_merter_config.enb_pf, g_merter_config.target_pf);
            // pwr_abs_offset = 0;
            // if (g_merter_config.reg > 0x100 && g_merter_config.target == 0)
            //     pwr_abs_offset = g_merter_config.reg - 0x100;
            sync_meter_config = 1;

            if (g_merter_config.enb == 1 && strlen(g_inv_meter.invdata.psn) == 0)
                get_meter_modelname(g_merter_config.mod, g_inv_meter.invdata.psn);
        }
    }

    if (g_merter_config.enb == 1)
    {
        // printf("\n------------------------------------\n");
        // ASW_LOGW("***********   debug print:  enable meter %d",g_merter_config.reg);
        // if (0x100 == (g_merter_config.reg & 0XF00)) // single
        if (10 == g_merter_config.reg)
        {
            printf("(10 == g_merter_config.reg) %d \n", g_merter_config.reg);
            // ESP_LOGI(TAG, "(100 == g_merter_config.reg) %d %d \n", g_merter_config.reg, pwr_abs_offset);
            if (read_time >= 10)
            {
                ret = md_query_meter_data(g_merter_config.mod, 1, 0); //, pwr_abs_offset > 0, &pwr_abs_val);
                read_time = 0;
                pf_time = 15;
            }
            else
                ret = md_query_meter_data(g_merter_config.mod, 1, 1); //, pwr_abs_offset > 0, &pwr_abs_val);

            // if ((0 == ret) && (0x100 == (g_merter_config.reg & 0XF00)))
            if ((0 == ret) && (10 == g_merter_config.reg))
            {
                energy_meter_control(g_merter_config.target); //, pwr_abs_offset, pwr_abs_val); //   mark 与 逆变器数据采集同步 TODO 20220607
            }

            /* kaco-lanstick 20220811 + */
              int now_sec = get_second_sys_time();
            if (ret < 0 || ret == 255)
            {
                printf("read meter fail-aaa\n");
                flush_serial_port(UART_NUM_1);

                if (meter_read_fail_sec != 0)
                {
                    if (now_sec - meter_read_fail_sec > 8 * 60)
                    {
                        g_inv_meter.invdata.status = 1;
                    }
                }
                else
                {
                    g_inv_meter.invdata.status = 1;
                }
            }
            else
            {
                meter_read_fail_sec = now_sec;
                g_inv_meter.invdata.status = 0;
            }


        }
        else // total  配合app待确定 此处是 single 模式还是 total模式
        {
            if (read_time >= 10)
            {
                ret = md_query_meter_data(g_merter_config.mod, 1, 0); //, 0, &pwr_abs_val);
                read_time = 0;
                pf_time = 15;
            }
        }

        read_time++;

        pf_time++;
        if (pf_time >= 10)
        {
            if (10 == g_merter_config.enb_pf)
            {
                ESP_LOGI(TAG, "meter cosphi%d %d pftarget %d \n", g_inv_meter.invdata.cosphi, meter_real_factory, (short)g_merter_config.target_pf);
                if (abs((short)g_merter_config.target_pf + meter_real_factory) > 1 && (meter_real_factory < 32766)) //[  mark]
                {
                    if ((short)g_merter_config.target_pf > 0 && meter_real_factory > 0)
                        writeReative_factory((short)g_merter_config.target_pf); // meter_real_factory);
                    if ((short)g_merter_config.target_pf < 0 && meter_real_factory < 0)
                        writeReative_factory((short)g_merter_config.target_pf); // meter_real_factory);

                    if (((short)g_merter_config.target_pf + meter_real_factory) > 0)
                    {
                        ESP_LOGI(TAG, "meter cosphi min\n");
                        if ((short)g_merter_config.target_pf > 0)
                        {
                            if (((short)g_merter_config.target_pf + meter_real_factory) > 99)
                                writeReative_factory(abs(meter_real_factory) + ((short)g_merter_config.target_pf + meter_real_factory) / 2);
                            else if (((short)g_merter_config.target_pf + meter_real_factory) < 100)
                                writeReative_factory(abs(meter_real_factory) + 10);
                        }
                        else
                        {
                            if (((short)g_merter_config.target_pf + meter_real_factory) > 99)
                                writeReative_factory(0 - meter_real_factory + ((short)g_merter_config.target_pf + meter_real_factory) / 2);
                            else if (((short)g_merter_config.target_pf + meter_real_factory) < 20)
                                writeReative_factory(0 - meter_real_factory + 10);
                        }
                    }
                    else // if(g_inv_meter.invdata.cosphi >(short)g_merter_config.target_pf/100)
                    {
                        ESP_LOGI(TAG, "meter cosphi over\n");
                        if ((short)g_merter_config.target_pf > 0)
                        {
                            if (((short)g_merter_config.target_pf + meter_real_factory) < -99)
                                writeReative_factory(abs(meter_real_factory) + ((short)g_merter_config.target_pf + meter_real_factory) / 2);
                            else if (((short)g_merter_config.target_pf + meter_real_factory) > -100)
                                writeReative_factory(abs(meter_real_factory) - 10);
                        }
                        else
                        {
                            if (((short)g_merter_config.target_pf + meter_real_factory) < -99)
                                writeReative_factory(0 - meter_real_factory + ((short)g_merter_config.target_pf + meter_real_factory) / 2);
                            else if (((short)g_merter_config.target_pf + meter_real_factory) > -100)
                                writeReative_factory(0 - meter_real_factory - 10);
                        }
                    }
                    meter_real_factory = 60000;
                }
            }
            pf_time = 0;
        }
        if (ret < 0)
            flush_serial_port(UART_NUM_1); //[  mark] add UART_NUM_1
    }
    return TASK_IDLE;
}

//----------------------------------------------//
void get_meter_info(char mod, char *meterbran, char *metername)
{
    switch (mod)
    {
    case 0:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON") + 1);
        strncpy(metername, "SDM630CT", strlen("SDM630CT") + 1);
    }
    break;
    case 1:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON") + 1);
        strncpy(metername, "SDM630DC", strlen("SDM630DC") + 1);
    }
    break;
    case 2:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON") + 1);
        strncpy(metername, "SDM230", strlen("SDM230") + 1);
    }
    break;
    case 3:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON") + 1);
        strncpy(metername, "SDM220", strlen("SDM220") + 1);
    }

    break;
    case 4:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON") + 1);
        strncpy(metername, "SDM120", strlen("SDM120") + 1);
    }
    break;
    case 5:
    {
        strncpy(meterbran, "NORK", strlen("NORK") + 1);
        strncpy(metername, "NORK", strlen("NORK") + 1);
    }
    break;
    default:
    {
        strncpy(meterbran, "AiMonitor", strlen("AiMonitor") + 1);
        strncpy(metername, "AiMonitor", strlen("AiMonitor") + 1);
    }
    break;
    }
    return;
}
