#include "meter_md_process.h"

static const char *TAG = "meter_md_processs.c";
uint16_t meter_pro[][8] = {
    /**
     * 0 --- start addr
     * 1 --- reg num
     * 2 --- pac
     * 3 --- e_in
     * 4 --- e_out
     * 5 --- qac
     * 6 --- sac
     * 7 --- pf
     * */
    /** 0,   1,  2, 3,  4,  5, 6, 7*/
    // {0x0034, 70, 0, 20, 22, 4, 2, 6},           // 0 estron 630 ct
    // {0x0034, 70, 0, 20, 22, 4, 2, 6},           // 1 estron 630 dc
    // {0x000C, 70, 0, 60, 62, 4, 2, 6},           // 2 estron 230
    // {0x000C, 70, 0, 60, 62, 4, 2, 6},           // 3 estron 220
    // {0x000C, 70, 0, 60, 62, 4, 2, 6},           // 4 estron 120
    // {0x0001, 20, 4, 10, 12, 0x00FF, 0x00FF, 6}, // 5 hu tai
    // {0x2000, 10, 2, 6, 8, 3, 4, 5}              // 6 ACTEL

    {0x0034, 70, 0, 20, 22, 8, 4, 10},          // 0 estron 630 ct
    {0x0034, 70, 0, 20, 22, 8, 4, 10},          // 1 estron 630 dc
    {0x000C, 70, 0, 60, 62, 12, 6, 18},         // 2 estron 230 ----------- confirmed
    {0x000C, 70, 0, 60, 62, 12, 6, 18},         // 3 estron 220
    {0x000C, 70, 0, 60, 62, 12, 6, 18},         // 4 estron 120
    {0x0001, 20, 4, 10, 12, 0x00FF, 0x00FF, 6}, // 5 hu tai
    {0x2000, 10, 2, 6, 8, 3, 4, 5},              // 6 ACTEL

#if TRIPHASE_ARM_SUPPORT
                                   //增加一个类型，第七行，如果行数不对，前后的内容中的下标都要同步更改
    {0x1921, 17, 0, 9, 11, 4, 2, 6}, // 11 AISWEI-INV-METER
#endif
};
int total_rate_power = 0;
int total_curt_power = 0;
int16_t meter_real_factory = 0;

uint32_t m_fast_read_pac = 0;

static int g_start_addr = 0x0;  /** 解析电表数据的modbus起始地址，是协议文档里的绝对地址*/
meter_data_t m_inv_meter = {0}; //[tgl mark]为了项目全局访问，在没有特殊的作用下，去掉了static修饰。 同事 inv_meter -> g_inv_meter

/** 每个寄存器4字节的，例如艾瑞达，不符合标准Modbus的*/
uint32_t get_u32_from_res_frame_4B_reg(int addr, uint8_t *buf)
{
    uint32_t var = 0;
    int idx = (addr - g_start_addr) * 4 + 3;
    var = (buf[idx] << 24) + (buf[idx + 1] << 16) + (buf[idx + 2] << 8) + (buf[idx + 3]);
    return var;
}
/** 每个寄存器4字节的，例如艾瑞达，不符合标准Modbus的*/
int32_t get_i32_from_res_frame_4B_reg(int addr, uint8_t *buf)
{
    uint32_t var = 0;
    int idx = (addr - g_start_addr) * 4 + 3;
    var = (buf[idx] << 24) + (buf[idx + 1] << 16) + (buf[idx + 2] << 8) + (buf[idx + 3]);
    return (int32_t)var;
}
/** 从响应帧提取数据值，地址是文档绝对地址，标准Modbus*/
uint32_t get_u32_from_res_frame(int addr, uint8_t *buf)
{
    uint32_t var = 0;
    int idx = (addr - g_start_addr) * 2 + 3;
    var = (buf[idx] << 24) + (buf[idx + 1] << 16) + (buf[idx + 2] << 8) + (buf[idx + 3]);
    return var;
}
/** 从响应帧提取数据值，地址是文档绝对地址，标准Modbus*/
uint16_t get_u16_from_res_frame(int addr, uint8_t *buf)
{
    uint16_t var = 0;
    int idx = (addr - g_start_addr) * 2 + 3;
    var = (buf[idx] << 8) + (buf[idx + 1]);
    return var;
}
/** 从响应帧提取数据值，地址是文档绝对地址，标准Modbus*/
int16_t get_i16_from_res_frame(int addr, uint8_t *buf)
{
    uint16_t var = 0;
    int idx = (addr - g_start_addr) * 2 + 3;
    var = (buf[idx] << 8) + (buf[idx + 1]);
    return (int16_t)var;
}

//--- Eng.Stg.Mch-lanstick 20220908 + ----//

float get_f32_from_u16(uint16_t *buf, int idx)
{
    float f = 0;
    uint32_t u = (buf[idx] << 16) + buf[idx + 1];
    f = *(float *)&u;
    return f;
}

#if TRIPHASE_ARM_SUPPORT

uint32_t get_u32_from_u8arry(uint8_t *buf)
{
    uint32_t u = 0;
    u = ((((uint32_t)*buf) << 24) & 0xFF000000) + ((((uint32_t) * (buf + 1) << 16)) & 0x00FF0000) + ((((uint32_t) * (buf + 2)) << 8) & 0x0000FF00) + (((uint32_t) * (buf + 3)) & 0x000000FF);
    return u;
}
//增加新函数
uint32_t get_u16_from_u8arry(uint8_t *buf)
{
    uint32_t u = 0;
    u = ((((uint32_t)*buf) << 8) & 0x0000FF00) + (((uint32_t) * (buf + 1)) & 0x000000FF);
    return u;
}

#endif

//=====================================//

float u32_to_float(uint32_t stored_data)
{
    float f;
    uint32_t u = stored_data;
    f = *(float *)&u;
    return f;
}
//---------------------------------//
int is_time_valid(void)
{
    int year = get_current_year();
    if (year > 2020 && year < 2100)
    {
        return 1;
    }

    else
    {
        return 0;
    }
}

//-------------------------------------//
int parse_sync_time(void)
{
    return 0; //[tgl mark] why?
}
//---------------------------------//

int8_t pack_SDM_meter_fun(int type, uint8_t *buf, uint16_t len)
{
    /** modbus长度错误检查 */
    int reg_num = meter_pro[type][1];
    uint32_t tmp = 0;
    // float f_tmp = 0.0;
    uint16_t input_reg[128] = {0};
    uint8_t j;

    /* Eng.Stg.Mch-lanstick 20220908 - */
    // if (len != 5 + 2 * reg_num || buf[2] != 2 * reg_num)
    // {
    //     return ASW_FAIL;
    // }

    for (j = 0; j < reg_num; j++)
    {
        input_reg[j] = (buf[3 + 2 * j] << 8) + buf[4 + 2 * j];
    }

    uint16_t pac_idx = meter_pro[type][2];    // index of the ac power
    uint16_t etl_in_idx = meter_pro[type][3]; // index of the total energy input
    uint16_t etl_ou_idx = meter_pro[type][4]; // index of the total energy output
    uint16_t qac_idx = meter_pro[type][5];    // index of the reactive power
    uint16_t sac_idx = meter_pro[type][6];    // index of the apparent power
    uint16_t pf_idx = meter_pro[type][7];     // index of the power factor

    /* Eng.Stg.Mch-lanstick 20220908 +- */
    // if (type != 6)
    if (type < 6)
    {
        m_inv_meter.invdata.pac = get_f32_from_u16(input_reg, pac_idx);                  // W for cloud
        m_fast_read_pac = get_f32_from_u16(input_reg, pac_idx);                          // W for regulate
        m_inv_meter.invdata.e_in_total = get_f32_from_u16(input_reg, etl_in_idx) * 100;  // 0.01kwh
        m_inv_meter.invdata.e_out_total = get_f32_from_u16(input_reg, etl_ou_idx) * 100; // 0.01kwh
        m_inv_meter.invdata.qac = get_f32_from_u16(input_reg, qac_idx);
        m_inv_meter.invdata.sac = get_f32_from_u16(input_reg, sac_idx);
        m_inv_meter.invdata.cosphi = get_f32_from_u16(input_reg, pf_idx) * 100; // 0.01

        if (type >= 2 && type <= 4)
            m_inv_meter.invdata.fac = get_f32_from_u16(input_reg, 58) * 100; // 0.01 Hz
        else if (type >= 0 && type <= 1)
        {
            m_inv_meter.invdata.fac = get_f32_from_u16(input_reg, 18) * 100; // 0.01 Hz
        }

        meter_real_factory = get_f32_from_u16(input_reg, pf_idx) * 10000; // 0.0001,pf
                                                                          // m_inv_meter.invdata.fac = get_f32_from_u16(input_reg, 58) * 100;        // 0.01 Hz
                                                                          // meter_real_factory = get_f32_from_u16(input_reg, pf_idx) * 10000;       // 0.0001,pf
#if DEBUG_PRINT_ENABLE
        printf("\n-----[tgl debug print]--------\n  -2- fast read pac:%d\n", m_fast_read_pac);
#endif
    }

    if (type == 6)
    {
        /** Total system power */
        m_inv_meter.invdata.pac = (int16_t)input_reg[pac_idx]; // W
        tmp = input_reg[etl_in_idx] + input_reg[etl_in_idx + 1];
        m_inv_meter.invdata.e_in_total = (uint32_t)tmp;
        tmp = input_reg[etl_ou_idx] + input_reg[etl_ou_idx + 1];
        m_inv_meter.invdata.e_out_total = (uint32_t)tmp;
        m_inv_meter.invdata.qac = (int16_t)input_reg[qac_idx];
        m_inv_meter.invdata.sac = (int16_t)input_reg[sac_idx];
        m_inv_meter.invdata.cosphi = (int16_t)input_reg[pf_idx];
    }
    // meter_real_factory = (int16_t)m_inv_meter.invdata.cosphi; // 0.0001, pf -------------- regulate

    return ASW_OK;
}
//-----------------------------------//
void pack_irdIM1281B_meter_fun(uint8_t *buf, uint16_t len)
{
    if (len == 45)
    {
        g_start_addr = 0x48;
        //[tgl mark] 获取的数据的位数和符号类型不匹配
        m_inv_meter.invdata.vac[0] = get_u32_from_res_frame_4B_reg(0x48, buf) / 1000;       // 0.1V
        m_inv_meter.invdata.iac[0] = get_i32_from_res_frame_4B_reg(0x49, buf) / 100;        // 0.01A
        m_inv_meter.invdata.pac = get_i32_from_res_frame_4B_reg(0x4A, buf) / 10000;         // W ----------------- regulate
        m_inv_meter.invdata.e_total = get_u32_from_res_frame_4B_reg(0x4B, buf) / 10000;     // kWh
        m_inv_meter.invdata.cosphi = get_i32_from_res_frame_4B_reg(0x4C, buf) / 10;         // 0.01 --cosphi
        m_inv_meter.invdata.fac = get_u32_from_res_frame_4B_reg(0x4F, buf);                 // 0.01Hz
        m_inv_meter.invdata.e_in_total = get_u32_from_res_frame_4B_reg(0x50, buf) / 10000;  // positive kWh
        m_inv_meter.invdata.e_out_total = get_u32_from_res_frame_4B_reg(0x51, buf) / 10000; // negative kWh

        ESP_LOGI(TAG, "volt %f [V]\n", (float)(m_inv_meter.invdata.vac[0] * 0.1));
        ESP_LOGI(TAG, "curr %f [A]\n", (float)((int)m_inv_meter.invdata.iac[0] * 0.01));
        ESP_LOGI(TAG, "pac %d [W]\n", (int)m_inv_meter.invdata.pac);
        ESP_LOGI(TAG, "e %u [kWh]\n", m_inv_meter.invdata.e_total);
        ESP_LOGI(TAG, "pf %f\n", (float)((int)m_inv_meter.invdata.cosphi * 0.01));
        ESP_LOGI(TAG, "freq %f [Hz]\n", (float)(m_inv_meter.invdata.fac * 0.01));
        ESP_LOGI(TAG, "e+ %u [kWh]\n", m_inv_meter.invdata.e_in_total);
        ESP_LOGI(TAG, "e- %u [kWh]\n", m_inv_meter.invdata.e_out_total);

        meter_real_factory = get_i32_from_res_frame_4B_reg(0x4C, buf) * 10; // 0.0001, pf -------------- regulate
    }
}
//-------------------------------//
void pack_acrelADL200_meter_fun(uint8_t *buf, uint16_t len, uint8_t frame_order)
{
    /** 第一帧*/
    if (frame_order == 1)
    {
        if (len == 19)
        {
            //[tgl mark] 获取的数据的位数和符号类型不匹配
            g_start_addr = 0x0B;
            m_inv_meter.invdata.vac[0] = get_u16_from_res_frame(0x0B, buf);             // 0.1V
            m_inv_meter.invdata.iac[0] = get_u16_from_res_frame(0x0C, buf);             // 0.01A
            m_inv_meter.invdata.pac = (int)get_i16_from_res_frame(0x0D, buf);           // W ----------------- regulate
            m_inv_meter.invdata.qac = (int)get_i16_from_res_frame(0x0E, buf);           // Var
            m_inv_meter.invdata.sac = get_u16_from_res_frame(0x0F, buf);                // VA
            m_inv_meter.invdata.cosphi = (int)(get_i16_from_res_frame(0x10, buf) / 10); // 0.01
            m_inv_meter.invdata.fac = get_u16_from_res_frame(0x11, buf);                // 0.01Hz

            ESP_LOGI(TAG, "volt %f [V]\n", (float)(m_inv_meter.invdata.vac[0] * 0.1));
            ESP_LOGI(TAG, "curr %f [A]\n", (float)(m_inv_meter.invdata.iac[0] * 0.01));
            ESP_LOGI(TAG, "pac %d [W]\n", (int)m_inv_meter.invdata.pac);
            ESP_LOGI(TAG, "qac %d [Var]\n", (int)m_inv_meter.invdata.qac);
            ESP_LOGI(TAG, "sac %u [VA]\n", m_inv_meter.invdata.sac);

            ESP_LOGI(TAG, "pf %f\n", (float)((int)m_inv_meter.invdata.cosphi * 0.01));
            ESP_LOGI(TAG, "freq %f [Hz]\n", (float)(m_inv_meter.invdata.fac * 0.01));
            ESP_LOGI(TAG, "\n");

            meter_real_factory = get_i16_from_res_frame(0x10, buf) * 10; // 0.0001, pf -------------- regulate

            ESP_LOGI(TAG, "pf: %d [0.0001]\n", (int)meter_real_factory);
        }
    }
    /** 第二帧*/
    else if (frame_order == 2)
    {
        if (len == 29)
        {
            g_start_addr = 0x68;
            m_inv_meter.invdata.e_in_total = get_u32_from_res_frame(0x68, buf);  /** 0.01kWh*/
            m_inv_meter.invdata.e_out_total = get_u32_from_res_frame(0x72, buf); /** 0.01kWh*/

            ESP_LOGI(TAG, "e+ %u [kWh] %u [0.01kWh]\n", m_inv_meter.invdata.e_in_total / 100, m_inv_meter.invdata.e_in_total);
            ESP_LOGI(TAG, "e- %u [kWh] %u [0.01kWh]\n", m_inv_meter.invdata.e_out_total / 100, m_inv_meter.invdata.e_out_total);
        }
    }
}

//--------------------------------//
int8_t md_decode_meter_pack(char is_fast, uint8_t type, uint8_t *buf, uint16_t len, uint8_t frame_order)
{
    /** modbus crc16错误检查*/
    uint16_t crc16 = crc16_calc(buf, len);
    if (crc16 != 0)
        return ASW_FAIL;

    int8_t res = ASW_FAIL;

    static int meter_data_time = -600; /** -600 sec 保证第一次发送*/
    static uint8_t last_day = 0;
    meter_gendata gendata = {0};

    if (is_fast == 0)
    {
        if (type <= 6)
        {
            res = pack_SDM_meter_fun(type, buf, len);
            if (res != ASW_OK)
                return res;
        }
        /** 艾瑞达（单帧）*/
        else if (type == METER_IREADER_IM1281B) // ireader
        {
            pack_irdIM1281B_meter_fun(buf, len);
        }
        /** 安科瑞*/
        else if (type == METER_ACREL_ADL200)
        {
            pack_acrelADL200_meter_fun(buf, len, frame_order);
        }

#if TRIPHASE_ARM_SUPPORT
        else if (type == 11) //增加类型11
        {                    //增加类型11
            // printf("decodeing AISWEI-INV-METER");                                                //增加类型11
            /** modbus长度错误检查*/ //增加类型11
            // int reg_num = meter_pro[7][1];                                                       //增加类型11
            //增加类型11
            uint16_t pac_idx = 3 + 2 * meter_pro[7][2];                                   // index of the ac power                       //增加类型11
            uint16_t etl_in_idx = 3 + 2 * meter_pro[7][3];                                // index of the total energy input             //增加类型11
            uint16_t etl_ou_idx = 3 + 2 * meter_pro[7][4];                                // index of the total energy output            //增加类型11
            uint16_t qac_idx = 3 + 2 * meter_pro[7][5];                                   // index of the reactive power                 //增加类型11
            uint16_t sac_idx = 3 + 2 * meter_pro[7][6];                                   // index of the apparent power                 //增加类型11
            uint16_t pf_idx = 3 + 2 * meter_pro[7][7];                                    // index of the power factor                   //增加类型11
                                                                                         //增加类型11
            m_inv_meter.invdata.pac = get_u32_from_u8arry(&buf[pac_idx]);                  // W for cloud              //增加类型11
            m_fast_read_pac = get_u32_from_u8arry(&buf[pac_idx]);                        // W for regulate           //增加类型11
            m_inv_meter.invdata.e_in_total = get_u32_from_u8arry(&buf[etl_in_idx]) * 100;  // 0.01kwh  //增加类型11
            m_inv_meter.invdata.e_out_total = get_u32_from_u8arry(&buf[etl_ou_idx]) * 100; // 0.01kwh //增加类型11
            m_inv_meter.invdata.qac = get_u32_from_u8arry(&buf[qac_idx]);                  //增加类型11
            m_inv_meter.invdata.sac = get_u32_from_u8arry(&buf[sac_idx]);                  //增加类型11
            m_inv_meter.invdata.cosphi = get_u16_from_u8arry(&buf[pf_idx]) * 100;          // 0.01             //增加类型11
            m_inv_meter.invdata.fac = get_u16_from_u8arry(&buf[19]) * 100;                 // 0.01 Hz          //增加类型11
            meter_real_factory = get_u16_from_u8arry(&buf[pf_idx]) * 10000;              // 0.0001,pf            //增加类型11
        }
        

#endif
    }
    /** 快速读取：只读功率*/
    else
    {
        // Eng.Stg.Mch-lanstick 20220908 +
#if TRIPHASE_ARM_SUPPORT
        if (type <= 6 || type == 11) //增加类型11

#else
        if (type <= 6)

#endif
        {
            int reg_num = 2; // only pac
            uint16_t input_reg[4] = {0};
            for (int j = 0; j < reg_num; j++)
            {
                input_reg[j] = (buf[3 + 2 * j] << 8) + buf[4 + 2 * j];
            }

            uint8_t pac_idx = 0; // index of the ac power

            if (type != 6)
            {
                /** pac */
                // inv_meter.invdata.pac = get_f32_from_u16(input_reg, pac_idx); //W
                m_fast_read_pac = get_f32_from_u16(input_reg, pac_idx); // W
                m_inv_meter.invdata.pac = m_fast_read_pac;
#if DEBUG_PRINT_ENABLE
                printf("\n-----[tgl debug print]--------\n  -1- fast read pac:%d\n", m_fast_read_pac);
#endif
            }
            else if (type == 6)
            {
                /** Total system power */
                m_inv_meter.invdata.pac = (int16_t)input_reg[pac_idx]; // W ----- regulate
                m_fast_read_pac = m_inv_meter.invdata.pac;
            }
        }
        if (type == METER_IREADER_IM1281B)
        {
            g_start_addr = 0x4A;
            m_inv_meter.invdata.pac = get_i32_from_res_frame_4B_reg(0x4A, buf) / 10000; // W ----------------- regulate
        }
        if (type == METER_ACREL_ADL200)
        {
            g_start_addr = 0x0D;
            m_inv_meter.invdata.pac = (int)get_i16_from_res_frame(0x0D, buf); // W ----------------- regulate
        }
        ESP_LOGI(TAG, "pac: %d [W]\n", (int)m_inv_meter.invdata.pac);
    }

    /************************* 计算当日买卖电量 ****************************************/

    write_global_var(GLOBAL_METER_DATA, &m_inv_meter);
    // Eng.Stg.Mch-lanstick 20220908 +-
    //  if (!(frame_order == 2 && type == METER_ACREL_ADL200))
    if (is_fast == 1) /** 如果没有读到总买卖电量*/
    {
        return ASW_OK;
    }
    if (is_time_valid() == 0) /** 系统时间不对，不计算当日发电量*/
    {
        return ASW_OK;
    }

    general_query(NVS_METER_GEN, (void *)&gendata);

    if (!last_day) /** 第一次，仅一次*/
    {
        if (get_current_days() != gendata.day) /** day发生变化*/
        {
            memset(&gendata, 0, sizeof(meter_gendata));
            gendata.e_in_total = m_inv_meter.invdata.e_in_total;
            gendata.e_out_total = m_inv_meter.invdata.e_out_total;
            gendata.day = get_current_days();
            general_add(NVS_METER_GEN, (void *)&gendata);
            m_inv_meter.e_in_day_begin = m_inv_meter.invdata.e_in_total;
            m_inv_meter.e_out_day_begin = m_inv_meter.invdata.e_out_total;
        }
        else /** day未发生变化*/
        {
            m_inv_meter.e_in_day_begin = gendata.e_in_total;
            m_inv_meter.e_out_day_begin = gendata.e_out_total;
        }
        last_day = get_current_days();
    }
    else if (get_current_days() != gendata.day)
    {
        // save_days();
        // save_meter_etotal(inv_meter.invdata.e_total);
        // save_meter_htotal(m_inv_meter.invdata.h_total);
        memset(&gendata, 0, sizeof(meter_gendata));
        gendata.e_in_total = m_inv_meter.invdata.e_in_total;
        gendata.e_out_total = m_inv_meter.invdata.e_out_total;
        gendata.day = get_current_days();
        general_add(NVS_METER_GEN, (void *)&gendata);

        m_inv_meter.e_in_day_begin = m_inv_meter.invdata.e_in_total;
        m_inv_meter.e_out_day_begin = m_inv_meter.invdata.e_out_total;
        last_day = get_current_days();
    }

    m_inv_meter.invdata.e_in_today = (m_inv_meter.invdata.e_in_total - m_inv_meter.e_in_day_begin); // 0.01 kwh
    m_inv_meter.invdata.e_out_today = (m_inv_meter.invdata.e_out_total - m_inv_meter.e_out_day_begin);

    if ((int)m_inv_meter.invdata.e_in_today < 0 || (int)m_inv_meter.invdata.e_out_today < 0)
    {
        m_inv_meter.invdata.e_in_today = 0;
        m_inv_meter.invdata.e_out_today = 0;

        memset(&gendata, 0, sizeof(meter_gendata));
        gendata.e_in_total = m_inv_meter.invdata.e_in_total;
        gendata.e_out_total = m_inv_meter.invdata.e_out_total;
        gendata.day = get_current_days();
        general_add(NVS_METER_GEN, (void *)&gendata);

        m_inv_meter.e_in_day_begin = m_inv_meter.invdata.e_in_total;
        m_inv_meter.e_out_day_begin = m_inv_meter.invdata.e_out_total;
        last_day = get_current_days();
    }

    ESP_LOGI(TAG, "e_in_today: %u [0.01kwh]\n", m_inv_meter.invdata.e_in_today);
    ESP_LOGI(TAG, "e_out_today: %u [0.01kwh]\n", m_inv_meter.invdata.e_out_today);
    ESP_LOGI(TAG, "e_in_day_begin: %u [0.01kwh]\n", m_inv_meter.e_in_day_begin);
    ESP_LOGI(TAG, "e_out_day_begin: %u [0.01kwh]\n", m_inv_meter.e_out_day_begin);

    write_global_var(GLOBAL_METER_DATA, &m_inv_meter);

    if (get_second_sys_time() - meter_data_time >= 300)
    {
        meter_data_time = get_second_sys_time();
        if (mq2 != NULL)
        {
            xQueueSend(mq2, (void *)&m_inv_meter, (TickType_t)0);
            ESP_LOGI(TAG, "meter data send cld ok\n");
        }
        // msgsnd(meter_msg_id, &trans_meter_cloud, sizeof(trans_meter_cloud.data), IPC_NOWAIT);
        return ASW_OK;
    }

    else
        return ASW_OK;
}

#if 0
    // /** Total system power */
        // tmp = (input_reg[pac_idx] << 16) + input_reg[pac_idx + 1];
        // f_tmp = u32_to_float(tmp);
        // // W --------------------------------- regulate
        // m_inv_meter.invdata.pac = (uint32_t)f_tmp; // [tgl mark] 此处应该是有符号还是无符号？？？
        // // printf("--------meter power:%04X, %d\r\n",tmp,inv_ptr->data.pac);

        // /** Total Input Energy */
        // tmp = (input_reg[etl_in_idx] << 16) + input_reg[etl_in_idx + 1];
        // f_tmp = u32_to_float(tmp);
        // m_inv_meter.invdata.e_in_total = (uint32_t)(f_tmp * 10);
        // // printf("--------meter energy input:%04X, %d\r\n",tmp,inv_ptr->data.pdc);

        // /** Total Output Energy */
        // tmp = (input_reg[etl_ou_idx] << 16) + input_reg[etl_ou_idx + 1];
        // f_tmp = u32_to_float(tmp);
        // m_inv_meter.invdata.e_out_total = (uint32_t)(f_tmp * 10);
        // // printf("--------meter energy output:%04X, %d\r\n",tmp,inv_ptr->data.sac);

        // /** reactive power */
        // tmp = (input_reg[qac_idx] << 16) + input_reg[qac_idx + 1];
        // f_tmp = u32_to_float(tmp);
        // m_inv_meter.invdata.qac = (uint32_t)(f_tmp); // [tgl mark] 此处应该是有符号还是无符号？？？
        // // printf("--------meter reactive power:%04X, %d\r\n",tmp,inv_ptr->data.e_total);

        // /** apparent power*/
        // tmp = (input_reg[sac_idx] << 16) + input_reg[sac_idx + 1];
        // f_tmp = u32_to_float(tmp);
        // m_inv_meter.invdata.sac = (uint32_t)(f_tmp); // [tgl mark] 此处应该是有符号还是无符号？？？
        // // printf("--------meter apparent power:%04X, %d\r\n",tmp,inv_ptr->data.h_total);

        // /** power factor*/
        // tmp = (input_reg[pf_idx] << 16) + input_reg[pf_idx + 1];
        // f_tmp = u32_to_float(tmp);
        // m_inv_meter.invdata.cosphi = (uint32_t)(f_tmp); // [tgl mark] 此处应该是有符号还是无符号？？？
        // // printf("--------meter power factor:%04X, %d\r\n",tmp,inv_ptr->data.e_today);

#endif