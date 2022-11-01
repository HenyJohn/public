#include "meter.h"
#include "inv_com.h"

static const char *TAG = "meter.c";
//----------------------------------//

// Eng.Stg.Mch-Lanstick ---+_
// static meter_data_t m_m_inv_meter = {0};

int sync_meter_config = 0;

uint16_t pwr_curr_per = 10000;

static int pwr = 1;

extern meter_data_t m_inv_meter; // meter_md_process defined

static uint8_t meter_frame_order = 1; /** 某些电表需要发送多帧，排序，从1开始*/

const char ireader_meter_send[] = {0x01, 0x03, 0x00, 0x48, 0x00, 0x0A, 0x45, 0xDB}; // all in one
const char ireader_read_pac_send[] = {0x01, 0x03, 0x00, 0x4A, 0x00, 0x01, 0xA5, 0xDC};
const char acrel_meter_send1[] = {0x01, 0x03, 0x00, 0x0B, 0x00, 0x07, 0x75, 0xCA};   // read U I PAC FAC ...
const char acrel_meter_send2[] = {0x01, 0x03, 0x00, 0x68, 0x00, 0x0C, 0xC4, 0x13};   // read energy
const char acrel_read_pac_send[] = {0x01, 0x03, 0x00, 0x0D, 0x00, 0x01, 0x15, 0xC9}; // fast
//--------------------------------------//
//------------------------------------------//
SERIAL_STATE clear_meter_setting(char msg)
{
    uint8_t imsg = (uint8_t)msg;

    if (imsg == 6 || imsg == 60)
    {
        ESP_LOGI(TAG, "meter sett clear\n");
        MonitorPara monitor_para = {0};
        read_global_var(METER_CONTROL_CONFIG, &monitor_para);
        monitor_para.adv.meter_enb = 0;
        // monitor_para.adv.meter_mod = 100;

#if TRIPHASE_ARM_SUPPORT
        monitor_para.adv.meter_mod = 1; // Eng.Stg.Mch-lanstick 20220908 +-
#else
        monitor_para.adv.meter_mod = 2; // Eng.Stg.Mch-lanstick 20220908 +-
#endif

        monitor_para.adv.meter_regulate = 0;
        monitor_para.adv.meter_target = 0;
        monitor_para.adv.meter_day = 0;
        monitor_para.adv.meter_enb_PF = 0;
        monitor_para.adv.meter_target_PF = 0;
        write_global_var_to_nvs(METER_CONTROL_CONFIG, &monitor_para);
        sync_meter_config = 0;
    }
    return TASK_IDLE;
}
//------------------------------------------//

/* ireader : UART_NUM_2 */
//[tgl mark] iread_read_meter 与 acrel_read_meter
// 有共用的函数表达式，建议封装为一个函数，提供不同的参数即可
//---------------------------------------//
int8_t ireader_read_meter(char is_fast)
{
    uint8_t buffer[257] = {0};
    uint16_t recv_len = 0;
    int8_t res = 0;
    int meter_type;
    /** no fast*/
    uart_write_bytes(UART_NUM_2, (const char *)ireader_meter_send, 8);
    hex_print(ireader_meter_send, 8);
    if (is_fast == 0)
    {
        recv_len = 45;
    }
    else
    {
        recv_len = 5 + 4;
    }

    res = recv_bytes_frame_waitting(UART_NUM_2, buffer, &recv_len);

    if (res == ASW_OK && recv_len > 0)
    {
        ESP_LOGI(TAG, "meter res ok\n");
        meter_type = METER_IREADER_IM1281B;
        if (ASW_OK == md_decode_meter_pack(is_fast, meter_type, buffer, recv_len, meter_frame_order))
        {
            ESP_LOGI(TAG, "read meter ok \n");
            return ASW_OK;
        }
        else
        {
            ESP_LOGE(TAG, "decode  meter  data error \n");
            return ASW_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "read meter fail\n");
        return ASW_FAIL;
    }
}

//---------------------------------------------------//

#if 0 // tgl mark
int8_t acrel_read_meter_1(char is_fast)
{
    uint8_t buffer[257] = {0};
    uint16_t recv_len = 0;
    int res = 0;

    meter_frame_order = 1;

    if (is_fast == 0)
    {
        uart_write_bytes(UART_NUM_1, (const char *)acrel_meter_send1, 8);
        hex_print(acrel_meter_send1, 8);
        recv_len = 19; // 5 + 2 * 7
    }
    else
    {
        uart_write_bytes(UART_NUM_1, (const char *)acrel_read_pac_send, 8);
        hex_print(acrel_read_pac_send, 8);
        recv_len = 7; // 5 + 2
    }
    res = recv_bytes_frame_waitting(UART_NUM_1, buffer, &recv_len);

    if ((res == ASW_OK) && (recv_len > 0))
    {
        ESP_LOGI(TAG, "meter res ok\n");
        aimeter_types meter_type = METER_ACREL_ADL200; // tgl debug
        if (ASW_OK == md_decode_meter_pack(is_fast, meter_type, buffer, recv_len, meter_frame_order))
        {
            ESP_LOGI(TAG, "read meter ok \n");
            return ASW_OK;
            ;
        }
        else
        {
            ESP_LOGE(TAG, "decode meter error \n");
            return ASW_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "read meter fail\n");
        return ASW_FAIL;
    }
}

//----------acrel:UART_NUM_2-------------------------------//
// read energy, no fast
int8_t acrel_read_meter_2(char is_fast)
{
    uint8_t buffer[257] = {0};
    uint16_t recv_len = 0;
    int res = 0;
    int meter_type;

    if (is_fast == 1)
    {
        ESP_LOGW(TAG, " acrel fast mode dont read energy!!");
        return ASW_OK;
    }

    // mark]采用这种方式传递参数，比较不建议，应该写到函数的接口参数里，减少耦合性。
    meter_frame_order = 2; //此变量会在md_decode_meter_pack 中调用

    uart_write_bytes(UART_NUM_1, acrel_meter_send2, 8);
    hex_print(acrel_meter_send2, 8);
    recv_len = 29; // 5 + 2 * 12
    res = recv_bytes_frame_waitting(UART_NUM_2, buffer, &recv_len);

    if (res == ASW_OK && recv_len > 0)
    {
        ESP_LOGI(TAG, "meter res ok\n");
        meter_type = METER_ACREL_ADL200;
        if (0 == md_decode_meter_pack(is_fast, meter_type, buffer, recv_len, meter_frame_order))
        {
            ESP_LOGI(TAG, "read meter ok \n");
            return ASW_OK;
        }
        else
        {
            ESP_LOGE(TAG, "decode meter error \n");
            return ASW_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "read meter fail\n");
        return ASW_FAIL;
    }
}
#endif
//----------------------------------------------//

int8_t md_query_meter_data(int type, unsigned char modbus_id, char is_fast)
{
    ESP_LOGI(TAG, "xxxx type modbus-id is_fast %d %d %d\n", (int)type, (int)modbus_id, (int)is_fast);
    int8_t res = ASW_FAIL;
    if (is_safety_96_97() == 1)
    {
        event_group_0 |= SAFE_96_97_READ_METER;
    }
    if (type <= 6) /** 原有电表类型*/
    {

        mb_req_t mb_req = {0};
        mb_res_t mb_res = {0};

        mb_req.fd = UART_NUM_1;
        mb_req.slave_id = modbus_id;
        mb_req.start_addr = meter_pro[type][0];
        if (is_fast == 0) /** 慢速功率调节*/
        {
            mb_req.reg_num = meter_pro[type][1];
            mb_res.len = meter_pro[type][1] * 2 + 5;
        }
        else /** 快速读取功率*/
        {
            mb_req.reg_num = 2;
            mb_res.len = mb_req.reg_num * 2 + 5;
        }

        /** special */
        if (type == 6)
        {
            mb_req.slave_id = 77;
        }
        else
        {
            mb_req.slave_id = 1;
        }

        res = asw_read_registers(&mb_req, &mb_res, 0x04);
        if (res != ASW_OK)
            return ASW_FAIL;
        else
        {
            /////////////////////////////// MAKR --- 20220916 ///////////////////////////////////////
            md_decode_meter_pack(is_fast, type, mb_res.frame, mb_res.len, meter_frame_order);
            return ASW_OK;
        }
    }
    else if (type == 7) // ireader
    {
        return ireader_read_meter(is_fast);
    }

#if TRIPHASE_ARM_SUPPORT
    else if (type == 11)
    {
        mb_req_t mb_req = {0};
        mb_res_t mb_res = {0};

        mb_req.fd = UART_NUM_1;
        mb_req.slave_id = 3;     // TODO MARK  这个地方需要后期修改适配电表类型     
                                          //增加类型11
        if (is_fast == 0)
        {
            mb_req.start_addr = meter_pro[7][0];
            mb_req.reg_num = meter_pro[7][1];
        }
        else
        {
            // mb_req.start_addr = meter_pro[7][0] + meter_pro[7][2]; //START_ADDR + PAC_INDEX //增加类型11
            mb_req.start_addr = meter_pro[7][0];
            mb_req.reg_num = 2;
        }
        mb_res.len = mb_req.reg_num * 2 + 5;
        int res = asw_read_registers(&mb_req, &mb_res, 0x03);
        if (res < 0)
            return -1;
        else
        {
            md_decode_meter_pack(is_fast, type, mb_res.frame, mb_res.len,meter_frame_order);
            return 0;
        }
    }

#endif

#if 0 // tgl mark -
    else if (type == METER_ACREL_ADL200) // acrel
    {
        // if (acrel_read_meter_1(is_fast) == ASW_OK && acrel_read_meter_2(is_fast) == ASW_OK)
        //         return ASW_OK;
        // else
        //     return ASW_FAIL;
        acrel_read_meter_2(is_fast);
        return acrel_read_meter_1(is_fast);
    }
#endif

    return ASW_FAIL;
}
//===================================================//

//--------------------------------//
// SIG:UART_NUM_1

static rst_code md_write_SIG_command(const Inverter *inv_ptr, const void *data_ptr /*, uint16_t len*/) //[tgl mark] len无效
{
    // sint_8 buffer[256] = {0};  //tgl change [warnning]
    uint8_t buffer[256] = {0};
    uint8_t i;
    uint16_t data_len = 0;
    uint16_t crc_val = 0xFFFF;
    TMD_RTU_WR_MTP_REQ_Packet *spack = NULL;
    spack = (TMD_RTU_WR_MTP_REQ_Packet *)data_ptr;
    memset(buffer, 0x00, 256);
    if (NULL == inv_ptr)
    {
        //并机模式下只发送到主机
        if (g_parallel_enable)
        {
            buffer[0] = g_host_modbus_id & 0xff; // broadcast address
        }
        // 普通模式广播发送
        else
        {
            buffer[0] = 0X00; // broadcast address
        }
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

#if DEBUG_PRINT_ENABLE
    printf("\nsend modbus buf:\n");
    for (i = 0; i < 8; i++)
    {
        printf("<%02X> ", buffer[i]);
    }
    printf("\n");
#endif

    //////////////////////
    if (NULL == inv_ptr && !g_parallel_enable)
    {
        usleep(200 * 1000); // 200ms
        printf("\n==== broad cast to inv control meter power finished. \n");
        return RST_YES;
    }

    /////////////////
    data_len = 8;
    memset(buffer, 0, 256);

    // int res =  tgl delete
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
        // md_write_active_pwr(NULL);
        return md_write_active_pwr_fast(NULL, adjust_num); // add for broadcast
        // TODO Eng.Stg.Mch-lanstick 20220908
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
        return md_write_active_pwr(NULL); // add for broadcast

        // TODO Eng.Stg.Mch-lanstick 20220908
    }
    return 0;
}

//-------------------------------//
extern uint32_t m_fast_read_pac;
int8_t energy_meter_control(int set_power)
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

    uint8_t b_need_adjust = 0;
    char inv_com_protocol[16] = {0};
    if (0 == total_rate_power)
        return ASW_FAIL; // total INV power 6KW+15KW+....

    // Inverter *inv_ptr = &inv_arr[0];

    memcpy(inv_com_protocol, &(inv_arr[0].regInfo.protocol_ver), 13);
    ESP_LOGI(TAG, "inv protocol ver %s \n", inv_com_protocol); //"cmv": "V2.1.0V2.0.0",

    // Eng.Stg.Mch-lanstick +
    if (strncmp(inv_com_protocol, "V2.1.1", sizeof("V2.1.1")) >= 0)
        curr_point = m_fast_read_pac;
    else
        curr_point = m_inv_meter.invdata.pac;

    if (set_power == 0)
        set_power = -400;
    set_point = set_power; //来自网络APP,cgi，服务器等调节值2000W

    /* calculate the current diff */
    curr_add = (set_point + curr_point) * 100 / total_rate_power;

    ESP_LOGI(TAG, "curr_add:%d,set_point:%d,curr_point:%d,inv_total_rated_pac:%d  \n",
             curr_add, set_point, curr_point, total_rate_power);

    if (strncmp(inv_com_protocol, "V2.1.1", sizeof("V2.1.1")) >= 0)
    {
        float stop_adjust = (set_point + curr_point) * 1.000 / total_rate_power;
        ESP_LOGI(TAG, "stop---------------------------###########################%f  \n", stop_adjust);

        float adjust_per = (set_power + curr_point) * 10000.0 / (total_rate_power * 1.0000);
        int16_t fast_pwr_curr_per = (int16_t)(adjust_per); //(set_power+total_curt_power+curr_point)*100.0/total_rate_power;

        ESP_LOGI(TAG, "regulate---------------------> %d %d %d %f\n", fast_pwr_curr_per, curr_add, (set_power + curr_point), adjust_per);

        if (stop_adjust <= 0.002 && stop_adjust >= 0)
        {
            ESP_LOGI(TAG, "stop adjust \n");
            return ASW_OK;
        }

        ESP_LOGI(TAG, "NNNN regulate********** %d \n", fast_pwr_curr_per);
        if (fast_pwr_curr_per > 10000)
            fast_pwr_curr_per = 10000;

        if (fast_pwr_curr_per < -10000)
            fast_pwr_curr_per = -10000;
        event_group_0 |= LOW_TIMEOUT_FOR_MODBUS;

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
        ESP_LOGI(TAG, "Forcast curr:%d pwr_limit:%d\r\n", pwr_curr_per, pwr_curr_per + curr_add);
    }
    else
    {
        if (steady_cnt >= 4) // No any change, timeout
        {
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

            pwr_curr_per = (unsigned short)abs(pwr_curr_per + curr_add);
        }
        else
        {
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
int writeReative_factory(short fac)
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
    else
    {
        // TODO multil inv [tgl Mark]
    }

    return 0;
}
//-----------------------------------//

//-----------------------------------------------//
#if 0
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
        task_inv_msg |= MSG_PWR_ACTIVE_INDEX; /** 只要读到电表，就会触发*/
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
#endif
/** 电表状态变化时发一次，若逆变器离线，则记下来等到在线时发送 *
 * 触发储能机并网*/
void send_meter_status(int ret)
{
    static int8_t last_meter_status = -1; /** -1 is init */
    char curr_meter_status = 0;
    static int offline_sec = 0;
    int now_sec = get_second_sys_time();

    if (ret == 0)
    {
        curr_meter_status = 1; // 1: online 0:offline

        /** online 只要读到电表，就会触发*/
        task_inv_meter_msg |= MSG_PWR_ACTIVE_INDEX;

        // printf("\n------------   AAAAAAA@@@@@@@@@@11111111111111111------------\n");
    }
    else
    {
        if (last_meter_status == 1 || last_meter_status == -1)
        {
            // if (is_safety_96_97() == 1)
            // {
            //     uint16_t data[2] = {0};
            //     data[0] = 0x0005;
            //     data[1] = 0x0005;
            //     modbus_write_inv(3, INV_REG_ADDR_METER, 2, data); /** 电表状态: 41108*/
            //     // task_inv_msg |= MSG_INV_SET_ADV_INDEX;

            //     task_inv_meter_msg |= MSG_INV_SET_ADV_INDEX;
            // }
            /////////////////////////////////////////////
            for (uint8_t j = 0; j < g_num_real_inv; j++)
            {
                int mach_type = inv_arr[j].regInfo.mach_type;
                int status = inv_arr[j].status;
                if (mach_type > 10 && status == 1) // estore && online
                {
                    uint8_t msafe = inv_arr[j].regInfo.safety_type;

                    if (msafe == 96 || msafe == 97 || msafe == 80)
                    {
                        uint16_t data[2] = {0};
                        data[0] = 0x0005;
                        data[1] = 0x0005;
                        modbus_write_inv(inv_arr[j].regInfo.modbus_id, INV_REG_ADDR_METER, 2, data); /** 电表状态: 41108*/
                        if ((task_inv_meter_msg & MSG_INV_SET_ADV_INDEX) == 0)
                            task_inv_meter_msg |= MSG_INV_SET_ADV_INDEX;
                    }
                }
            }

            /////////////////////////////////////////////////
            offline_sec = now_sec;
        }
        else if (last_meter_status == 0)
        {
            if (now_sec - offline_sec >= 30)
            {
                offline_sec = now_sec;
                // task_inv_msg |= MSG_INV_SET_ADV_INDEX; /** offline, every 30 sec*/
                task_inv_meter_msg |= MSG_INV_SET_ADV_INDEX;
            }
        }
    }

    last_meter_status = curr_meter_status;
}
//-------------------------------//
void meter_offline_handler(void)
{
    m_inv_meter.invdata.pac = 0;
    m_inv_meter.invdata.qac = 0;
    m_inv_meter.invdata.sac = 0;
    m_inv_meter.invdata.cosphi = 0;
    m_inv_meter.invdata.fac = 0;
    write_global_var(GLOBAL_METER_DATA, &m_inv_meter);
}

/* LanStick-Eng.Stg.Mch */
//-----------------------------------------------//
SERIAL_STATE query_meter_proc(int is_for_cloud)
{
    int ret = -1;
    static int read_time = 0;
    static int pf_time = 0;
    MonitorPara monitor_para = {0};

    static int64_t m_last_ms = 0; // for test

    // ESP_LOGI("--Meter--", "query_meter_proc--->");

    // write_global_var(GLOBAL_METER_DATA, &m_inv_meter); /** in loop */  ///// TGL MAKR delete
    read_global_var(METER_CONTROL_CONFIG, &monitor_para);
#if !TRIPHASE_ARM_SUPPORT
    // if (monitor_para.adv.meter_mod != 2)
    if (monitor_para.adv.meter_mod < 0 || monitor_para.adv.meter_mod > 4)
    {
        monitor_para.adv.meter_mod = METER_ESTRON_230; /** default: sdm 230 support only */
    }
#endif
    if ((event_group_0 & METER_CONFIG_MASK) == 1 && 0 == parse_sync_time())
    {

        ESP_LOGI(TAG, "enb:%d\n", monitor_para.adv.meter_enb);
        ESP_LOGI(TAG, "mod:%d\n", monitor_para.adv.meter_mod);
        ESP_LOGI(TAG, "regulate:%d\n", monitor_para.adv.meter_regulate);
        ESP_LOGI(TAG, "target:%d\n", monitor_para.adv.meter_target);
        ESP_LOGI(TAG, "day:%d\n", monitor_para.adv.meter_day);
        ESP_LOGI(TAG, "enb_pf:%d\n", monitor_para.adv.meter_enb_PF);
        ESP_LOGI(TAG, "target_pf:%d\n", monitor_para.adv.meter_target_PF);
        event_group_0 &= ~METER_CONFIG_MASK;
    }
    // ESP_LOGI("--Meter--", "NNNNNNNNNNNNNNNNN meter_enb:%d--->", monitor_para.adv.meter_enb);

    if (monitor_para.adv.meter_enb != 1)
        return TASK_IDLE;

    if (is_for_cloud == 1)
    {
        ret = md_query_meter_data(monitor_para.adv.meter_mod, 1, 0);
        if (ret != 0)
        {
            meter_offline_handler();
        }
        send_meter_status(ret);

        return 0;
    }

    if (10 == monitor_para.adv.meter_regulate)
    {
        ESP_LOGI(TAG, "(10 == meter reg) %d \n", monitor_para.adv.meter_regulate);
#if !TRIPHASE_ARM_SUPPORT
        /** 防逆流已使能，快速读取为主*/
        ret = md_query_meter_data(monitor_para.adv.meter_mod, 1, 1);
        if (ret != 0)
        {
            meter_offline_handler();
        }

        send_meter_status(ret);

        if ((ASW_OK == ret) && (10 == monitor_para.adv.meter_regulate))
        {
            ////////////////////////////////////////
            printf("\n----------------query_meter_proc enable power control[%lldms]-------------\n",
                   get_msecond_sys_time() - m_last_ms);
            m_last_ms = get_msecond_sys_time();
            ////////////////////////////////////
            read_global_var(METER_CONTROL_CONFIG, &monitor_para); /** keep target power newest*/
            energy_meter_control(monitor_para.adv.meter_target);
        }

#endif
    }
    else
    {
        /** 防逆流未使能，慢速读取*/
        if (is_safety_96_97() == 1)
        {
            ret = md_query_meter_data(monitor_para.adv.meter_mod, 1, 0);
            if (ret != 0)
            {
                meter_offline_handler();
            }
            send_meter_status(ret);

            read_time = 0;
        }
        else
        {
            if (read_time >= 3) // 10
            {
                ret = md_query_meter_data(monitor_para.adv.meter_mod, 1, 0);
                if (ret != 0)
                {
                    meter_offline_handler();
                }
                send_meter_status(ret);

                read_time = 0;
            }
        }
    }
#if !TRIPHASE_ARM_SUPPORT

    if (read_time++ > 10000)
        read_time = 10000;

    pf_time++;

    if (pf_time >= 10)
    {
        if (10 == monitor_para.adv.meter_enb_PF)
        {
            int target_pf = monitor_para.adv.meter_target_PF;
            ESP_LOGI(TAG, "meter cosphi%d %d pftarget %d \n", m_inv_meter.invdata.cosphi, meter_real_factory, (short)target_pf);
            if (abs((short)target_pf + meter_real_factory) > 1 && (meter_real_factory < 32767 /*50000*/)) //[tgl mark] 50000超出了上限 2^15
            {
                if ((short)target_pf > 0 && meter_real_factory > 0)
                    writeReative_factory((short)target_pf); // meter_real_factory);
                if ((short)target_pf < 0 && meter_real_factory < 0)
                    writeReative_factory((short)target_pf); // meter_real_factory);

                if (((short)target_pf + meter_real_factory) > 0)
                {
                    ESP_LOGI(TAG, "meter cosphi min\n");
                    if ((short)target_pf > 0)
                    {
                        if (((short)target_pf + meter_real_factory) > 99)
                            writeReative_factory(abs(meter_real_factory) + ((short)target_pf + meter_real_factory) / 2);
                        else if (((short)target_pf + meter_real_factory) < 100)
                            writeReative_factory(abs(meter_real_factory) + 10);
                    }
                    else
                    {
                        if (((short)target_pf + meter_real_factory) > 99)
                            writeReative_factory(0 - meter_real_factory + ((short)target_pf + meter_real_factory) / 2);
                        else if (((short)target_pf + meter_real_factory) < 20)
                            writeReative_factory(0 - meter_real_factory + 10);
                    }
                }
                else // if(m_inv_meter.invdata.cosphi >(short)target_pf/100)
                {
                    ESP_LOGI(TAG, "meter cosphi over\n");
                    if ((short)target_pf > 0)
                    {
                        if (((short)target_pf + meter_real_factory) < -99)
                            writeReative_factory(abs(meter_real_factory) + ((short)target_pf + meter_real_factory) / 2);
                        else if (((short)target_pf + meter_real_factory) > -100)
                            writeReative_factory(abs(meter_real_factory) - 10);
                    }
                    else
                    {
                        if (((short)target_pf + meter_real_factory) < -99)
                            writeReative_factory(0 - meter_real_factory + ((short)target_pf + meter_real_factory) / 2);
                        else if (((short)target_pf + meter_real_factory) > -100)
                            writeReative_factory(0 - meter_real_factory - 10);
                    }
                }
                meter_real_factory = 60000;
            }
        }
        pf_time = 0;
    }

    /**
     * @brief [tgl mark] 此处应该刷新哪个串口？？？ UART_NUM_1
     *       旧电表为UART_NUM_1(type<=6) iread&acrel的为UART_NUM_2
     */
    if (ret < 0)
    {

        flush_serial_port(UART_NUM_1);
    }

#endif

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

#if TRIPHASE_ARM_SUPPORT
    case 11: //增加一个类型，名字是我自己编的，看情况改
    {
        strncpy(meterbran, "ASWEI", strlen("ASWEI")+1);
        strncpy(metername, "ASWEI", strlen("ASWEI")+1);
    }
    break;
#endif
    default:
    {
        strncpy(meterbran, "AiMonitor", strlen("AiMonitor") + 1);
        strncpy(metername, "AiMonitor", strlen("AiMonitor") + 1);
    }
    break;
    }
    return;
}

#if TRIPHASE_ARM_SUPPORT

uint8_t asw_magic_header[] = {0x66, 0x99, 0x0D, 0x0A, 0x66, 0x99, 0x0D, 0x0A};
SERIAL_STATE energy_meter_control_combox()
{
    int len = 0;
    uint8_t buffer[200] = {0};
    static char last_set[28 + 4] = {0};
    uint8_t res_buf[256] = {0};
    uint16_t res_len = 0;
    int res = 0;
    uint16_t crc = 0;
    MonitorPara monitor_para;

    uint8_t index = 0;

    // read_global_var(PARA_CONFIG, &monitor_para);
    read_global_var(METER_CONTROL_CONFIG, &monitor_para);

    if (monitor_para.adv.meter_mod < 0 || monitor_para.adv.meter_mod > 4)
    {
        return -1; /** default: sdm 230 support only */
    }

    for (uint8_t i = 0; i < sizeof(asw_magic_header); i++)
    {
        buffer[index++] = asw_magic_header[i];
    }

    // buffer[index++] = 0x66;
    // buffer[index++] = 0x99;
    // buffer[index++] = 0x0D;
    // buffer[index++] = 0x0A;
    // buffer[index++] = 0x66;
    // buffer[index++] = 0x99;
    // buffer[index++] = 0x0D;
    // buffer[index++] = 0x0A;
    /** fc */
    buffer[index++] = 0x00;
    buffer[index++] = 0x04;
    /** length */
    buffer[10] = 0x00;
    buffer[11] = 0x00;
    buffer[12] = 0x00;
    buffer[13] = 0x30; // 8+2+4+4+32+2=48
    /** data*/
    /**     enb */
    buffer[14] = 0; // monitor_para.adv.meter_enb;
    buffer[15] = 0;
    buffer[16] = 0;
    buffer[17] = (monitor_para.adv.meter_enb >> 0) & 0x0001;
    /**     mod*/
    buffer[18] = (monitor_para.adv.meter_mod >> 24) & 0x00ff;
    buffer[19] = (monitor_para.adv.meter_mod >> 16) & 0x00ff;
    buffer[20] = (monitor_para.adv.meter_mod >> 8) & 0x00ff;
    buffer[21] = (monitor_para.adv.meter_mod >> 0) & 0x00ff;
    /**     regulate */
    buffer[22] = 0; //(monitor_para.adv.meter_target >> 8) & 0x00ff;
    buffer[23] = 0;
    buffer[24] = 0;
    buffer[25] = (monitor_para.adv.meter_regulate) & 0x000f;
    /**     target */
    buffer[26] = (monitor_para.adv.meter_target >> 24) & 0x00ff;
    buffer[27] = (monitor_para.adv.meter_target >> 16) & 0x00ff;
    buffer[28] = (monitor_para.adv.meter_target >> 8) & 0x00ff;
    buffer[29] = (monitor_para.adv.meter_target >> 0) & 0x00ff;
    /**     enb_PF */
    buffer[30] = 0;
    buffer[31] = 0;
    buffer[32] = 0;
    buffer[33] = (monitor_para.adv.meter_enb_PF) & 0x000f;
    /**     target_PF */
    buffer[34] = (monitor_para.adv.meter_target_PF >> 24) & 0x00ff;
    buffer[35] = (monitor_para.adv.meter_target_PF >> 16) & 0x00ff;
    buffer[36] = (monitor_para.adv.meter_target_PF >> 8) & 0x00ff;
    buffer[37] = (monitor_para.adv.meter_target_PF >> 0) & 0x00ff;
    /**     abs_offset */
    buffer[38] = (monitor_para.adv.regulate_offset >> 24) & 0x00ff;
    buffer[39] = (monitor_para.adv.regulate_offset >> 16) & 0x00ff;
    buffer[40] = (monitor_para.adv.regulate_offset >> 8) & 0x00ff;
    buffer[41] = (monitor_para.adv.regulate_offset >> 0) & 0x00ff;

    /** total power */
    buffer[42] = (total_rate_power >> 24) & 0x00ff;
    buffer[43] = (total_rate_power >> 16) & 0x00ff;
    buffer[44] = (total_rate_power >> 8) & 0x00ff;
    buffer[45] = (total_rate_power >> 0) & 0x00ff;

    if (memcmp(last_set, &buffer[14], 32) == 0)
    {
        printf("no change with last setting\n");
        return 0;
    }

    /**     crc */
    len = 42 + 4;
    crc = crc16_calc(buffer, len);
    buffer[len] = crc & 0xFF;
    len++;
    buffer[len] = (crc >> 8) & 0xFF;
    len++;

    printf("start sending............\n");
    for (int i = 0; i < len; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");

    for (int j = 0; j < 10; j++)
    {
        uart_write_bytes((UART_NUM_1), &buffer, len);
        recv_bytes_frame_waitting_nomd(UART_NUM_1, res_buf, &res_len);
        res = -1;
        if (res_len == 12 && crc16_calc(res_buf, res_len) == 0)
        {
            if (memcmp(res_buf, buffer, 8) == 0)
            {
                if (res_buf[9] == 0x04)
                {
                    printf("meter control ack ok\n");
                    res = 0;
                }
            }
        }

        sleep(1);

        printf("send meter control frame for the ----------------- %dth time\n", j);
        if (res == 0)
            break;
    }
    if (res != 0)
    {
        printf("send meter control times fail\n");
        return 0;
    }
    memcpy(last_set, &buffer[14], 28 + 4);

    return TASK_IDLE;
}

#endif
