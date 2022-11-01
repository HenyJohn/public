#include "asw_invtask_fun.h"
#include "asw_mutex.h"
#include "inv_com.h"

#define DAY_CHARG_SCH_NUM 6 /// mark +

#define INV_BROADCAST_DELAYMS 500 * 1000 // 200ms
#define INV_BROADCAST_NUM 3              //广播次数

// static DayDetailSch dayDetailSch[DAY_CHARG_SCH_NUM]; // current day schedule of charge/discharge power into/from battery

static DayDetailSch Bat_DaySchdle_arr_t[DAY_CHARG_SCH_NUM];

static int16_t cur_week_day = 0; // current day

static uint16_t last_mdata[7] = {0}; // for meter status frame change detect

//*************************************//
static const char *TAG = "asw_invtask_fun.c";

// int8_t inv_set_para(uint32_t msg_index);
int8_t inv_set_para(int inv_index, uint32_t msg_index);

void handleMsg_setChangefun(void *p);
//-----------------------------------------------//
// Eng.Stg.Mch-Lanstick 20220907 +
#define REG_41153_IDLE 0
#define REG_41153_METER_PAC 1
#define REG_41153_PWR_LIMIT 2

#define BATTERY_CHARGING_STOP 1 // stop charging/discharging
#define BATTERY_CHARGING 2      // charging
#define BATTERY_DISCHARGING 3   // discharging

static uint8_t g_is_in_schedule[INV_NUM] = {0};
static uint8_t g_41153_state[INV_NUM] = {0};
//----------------------------------------/
void handleMsg_getSafty_fun()
{
    DATE_STRUCT curr_date;
    uint8_t i;
    get_current_date(&curr_date);
    ESP_LOGI(TAG, "------current data:%04d-%02d-%02d %02d:%02d:%02d\n",
             curr_date.YEAR, curr_date.MONTH, curr_date.DAY, curr_date.HOUR, curr_date.MINUTE, curr_date.SECOND);
    /*update the charging/discharging/stop schedule*/
    uint16_t uHour = 0;
    uint16_t uMinutes = 0;
    cur_week_day = curr_date.WDAY;  /** 产生数据，给全局变量*/
    ScheduleBat schedule_bat = {0}; /** 产生数据，给全局变量*/
    // Bat_Schdle_arr_t schedule_bat = {0}; /** 产生数据，给全局变量*/

    read_global_var(PARA_SCHED_BAT, &schedule_bat);
    ESP_LOGI(TAG, "curr: weekday var1 var2 %d %u %u", cur_week_day, schedule_bat.daySchedule[cur_week_day].time_sch[0],
             schedule_bat.daySchedule[cur_week_day].time_sch[1]);
    for (i = 0; i < 6; i++)
    {
        uHour = (schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0xFF000000) >> 24;
        uMinutes = (schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0x00FF0000) >> 16;
        Bat_DaySchdle_arr_t[i].minutes_inday = uHour * 60 + uMinutes;
        Bat_DaySchdle_arr_t[i].duration = ((schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0x0000FF00) >> 8);
        Bat_DaySchdle_arr_t[i].charge_flag = schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0x000000FF;
        ESP_LOGI(TAG, "------dayDetailSch:%d, minutes_inday=%d, duration=%d, charge_flag=%d\n", i, Bat_DaySchdle_arr_t[i].minutes_inday,
                 Bat_DaySchdle_arr_t[i].duration, Bat_DaySchdle_arr_t[i].charge_flag);
    }
}
//-----------------------------------------------------//
//---Eng.Stg.Mch-lanstick 20220907 +-
void handleMsg_setChange_fun(int mIndex, MonitorBat_info *p_monitor_para)
{
    uint16_t data[2] = {0};

    uint16_t data_mode = 0;
    static uint16_t sche_last_data_mode[INV_NUM] = {0};

    // int ret = 0;

    data[0] = p_monitor_para->batmonitor.freq_mode;

    //-------------------------//
    ScheduleBat schedule_bat = {0}; /** 产生数据，给全局变量*/
    // Bat_Schdle_arr_t schedule_bat = {0}; /** 产生数据，给全局变量*/

    read_global_var(PARA_SCHED_BAT, &schedule_bat);
    //-------------------------//

    static uint16_t sche_last_data[INV_NUM][2] = {0};

    ESP_LOGI(TAG, "charge mode: %d\n", p_monitor_para->batmonitor.freq_mode);
    switch (p_monitor_para->batmonitor.freq_mode)
    {
    case 1: // stop

        data_mode = 2; /** 自发自用模式*/

        break;
    case 2: // charging -
        // data[1] = (p_monitor_para->batmonitor.fq_t1 * -1);
        data[1] = (schedule_bat.fq_t1 * -1);

        data_mode = 4; /** 自定义模式*/

        break;
    case 3: // discharging +
        data[1] = schedule_bat.ov_t1;

        data_mode = 4; /** 自定义模式*/

        break;
    default:
        break;
    }

    /////////////////////////////////////////////

    Bat_arr_t mBattery_arr_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &mBattery_arr_data);
    ///////////////////////////////////////////////////////////////////////////
    printf("\n===================== handleMsg_setChange_fun [%d]=================\n", p_monitor_para->batmonitor.freq_mode);
    for (uint8_t i = 0; i < 2; i++)
    {

        printf("* %d:%04X  *", data[i], data[i]);
    }
    printf("\n===================== handleMsg_setChange_fun =================\n");
    //////////////////////////////////////////////////////////////////////

    if (data_mode > 0 && data_mode != sche_last_data_mode[mIndex])
    {

        if (modbus_write_inv(mBattery_arr_data[mIndex].modbus_id, 0x044F, 1, &data_mode) == ASW_OK) /** 储能机运行模式：41104*/
            sche_last_data_mode[mIndex] = data_mode;
        else
        {
            ESP_LOGE("-- Write inv cmd Error--", "modebus failed write data to 41104.");
        }

        printf("\n run mode set ----   send data[%d] to inv %d reg addr:%d\n", data_mode, mBattery_arr_data[mIndex].modbus_id, 0x044F);
    }

    if (p_monitor_para->batmonitor.freq_mode == 1) //停止
    {
        if (data[0] > 0 && data[0] != sche_last_data[mIndex][0]) //
        {
            if (modbus_write_inv(mBattery_arr_data[mIndex].modbus_id, INV_REG_ADDR_CHARGE, 1, data) == ASW_OK) /** 储能机充放电标志：41152*/
                sche_last_data[mIndex][0] = data[0];
            else
            {
                ESP_LOGE("-- Write inv cmd Error--", "modebus failed write data to 41152.");
            }

            printf("\n stop charge set ----   send data[%d] to inv %d reg addr:%d\n", data[0], mBattery_arr_data[mIndex].modbus_id, INV_REG_ADDR_CHARGE);
        }
    }
    else // 冲放电
    {
        if ((data[0] > 0 && memcmp(data, sche_last_data[mIndex], sizeof(data)) != 0) || g_41153_state[mIndex] != REG_41153_PWR_LIMIT)
        {
            /** 储能机充放电标志：41152*/
            if (modbus_write_inv(mBattery_arr_data[mIndex].modbus_id, INV_REG_ADDR_CHARGE, 2, data) == ASW_OK)
            {
                memcpy(sche_last_data[mIndex], data, sizeof(data));
                g_41153_state[mIndex] = REG_41153_PWR_LIMIT;
            }
            else
            {
                ESP_LOGE("-- Write inv cmd Error-- ", "  inv index:%d modebus failed write data to 41152+2.", mIndex);
            }

            printf("\n charge set ----   send data[%d,%d] to inv %d reg addr:%d\n", data[0], data[1], mBattery_arr_data[mIndex].modbus_id, INV_REG_ADDR_CHARGE);
        }
    }
}

//------------------------------------------------------//

void handleMsg_pwrActive_fun(MonitorPara *p_monitor_para, meter_data_t *p_inv_meter)
{
    Bat_Monitor_arr_t bat_monitor_para = {0};

    read_global_var(PARA_CONFIG, &bat_monitor_para);

    static uint8_t need_send = 1; /** 开机后, 发一次*/
    uint16_t m16data[7] = {0};    // data form active power
    // MonitorPara *p_monitor_para = (MonitorPara)p1;
    // meter_data_t *p_inv_meter = (meter_data_t)p2;
    int ret;
    uint8_t m_invModbus_Id = 0;

    m16data[0] = 0x000A; // status of the meter
    m16data[1] = 0x000A; // flag of the function

    m16data[2] = ((p_monitor_para->adv.meter_target & 0xFFFF0000) >> 16);
    m16data[3] = ((p_monitor_para->adv.meter_target & 0x0000FFFF));
    m16data[4] = ((p_inv_meter->invdata.pac & 0xFFFF0000) >> 16);
    m16data[5] = (p_inv_meter->invdata.pac & 0x0000FFFF);
    m16data[6] = p_monitor_para->adv.meter_regulate;
    /** 某些设置如果变化, 发一次*/
    if (memcmp(m16data, last_mdata, 2 * sizeof(uint16_t)) != 0 || memcmp(&m16data[6], &last_mdata[6], sizeof(uint16_t)) != 0)
    {
        memcpy(last_mdata, m16data, sizeof(m16data));
        need_send = 1;
    }

    if (event_group_0 & ESTORE_BECOME_ONLINE_MASK) /** 发现逆变器重启，也发一次*/
    {
        event_group_0 &= ~ESTORE_BECOME_ONLINE_MASK;
        need_send = 1;
    }

    if (need_send == 1)
    {
        // ret = modbus_write_inv(3, INV_REG_ADDR_METER, 7, m16data); /** 电表防逆流: 41108-41114*/
        ////////////////////////////////////////////////////////
        // [MAKR MultiInv]

        Bat_arr_t m_battery_data = {0};
        read_global_var(GLOBAL_BATTERY_DATA, &m_battery_data);

        for (uint8_t m = 0; m < g_num_real_inv; m++)
        {
            m_invModbus_Id = m_battery_data[m].modbus_id;
            if (m_invModbus_Id >= 3 && m_invModbus_Id < 255)
            {
                ret = modbus_write_inv(m_invModbus_Id, INV_REG_ADDR_METER, 7, m16data); /** 电表防逆流: 41108-41114*/
                if (ret == 0)
                    need_send = 0;
                ESP_LOGI(TAG, "Send meter status to invid:%d  + regulate: 000A000A %d\n", m_invModbus_Id, p_monitor_para->adv.meter_regulate);
            }
            else
            {
                ESP_LOGE("--LanStick Multi Eng.Stg Erro--", "handleMsg_pwrActive_funA: faied to get the inv modebus ID");
            }
        }
    }

    memset(m16data, 0, sizeof(m16data));

    /** **********************************/
    /** 非自定义模式下，发电表功率*/
    Bat_arr_t m_batterys_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_batterys_data);

    for (uint8_t m = 0; m < g_num_real_inv; m++)
    {
        ////////////////////////////////////////////////////////
        ESP_LOGI(TAG, "inv%d,sn:%s,run-mode store-type: %d %d,g_is_in_schedule[%d]\n", m, bat_monitor_para[m].sn, bat_monitor_para[m].batmonitor.uu1,
                 bat_monitor_para[m].batmonitor.dc_per, g_is_in_schedule[m]);

        // Eng.Stg.Mch-Lanstick 20220907 +-
        /** 非自定义模式下，发电表功率*/
        if (bat_monitor_para[m].batmonitor.uu1 != 4 || (bat_monitor_para[m].batmonitor.uu1 == 4 && g_is_in_schedule[m] == 0)) /** 非自定义模式*/
        {
            m16data[0] = (int16_t)((int)p_inv_meter->invdata.pac);
            event_group_0 |= LOW_TIMEOUT_FOR_MODBUS;
            ////////////////////////////////////////////////////////
            m_invModbus_Id = m_batterys_data[m].modbus_id;
            if (m_invModbus_Id >= 3 && m_invModbus_Id < 255)
            {
                ret = modbus_write_inv(m_invModbus_Id, 0x0480, 1, m16data); /** 41153: 电表实际功率*/
                ESP_LOGI(TAG, "send meter to inv:%d pac: %d\n", m_invModbus_Id, (int16_t)m16data[0]);
            }
            else
            {
                ESP_LOGE("--LanStick Multi Eng.Stg Erro--", "handleMsg_pwrActive_funB: faied to get the inv modebus ID");
            }
            g_41153_state[m] = REG_41153_METER_PAC;
        }
    }
}

//------------------------------------------------------------//
void handleMsg_setAdv_fun()
{
    // int ret;
    uint16_t data[6] = {0};
    data[0] = 0x0005;
    data[1] = 0x0005;
    // modbus_write_inv(3, INV_REG_ADDR_METER, 2, data); /** 电表状态: 41108*/

    ///////////////////////////////////////////////////////
    Bat_arr_t m_batts_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_batts_data);
    uint8_t m_invModbus_Id;
    for (uint8_t m = 0; m < g_num_real_inv; m++)
    {
        // m_invModbus_Id = inv_arr[m].regInfo.modbus_id;
        m_invModbus_Id = m_batts_data[m].modbus_id;

        if (m_invModbus_Id >= 3 && m_invModbus_Id < 255)
        {
            modbus_write_inv(m_invModbus_Id, INV_REG_ADDR_METER, 2, data); /** 电表状态: 41108*/
            ESP_LOGI(TAG, "Send meter to inv:%d status command:%04X %04X \r\n", m_invModbus_Id, data[0], data[1]);
        }
        else
        {
            ESP_LOGE("--LanStick Multi Eng.Stg Erro--", "handleMsg_setAdv_fun: faied to get the inv modebus ID");
        }
    }

    ///////////////////////////////////////////////////////

    memcpy(last_mdata, data, 2 * sizeof(uint16_t));
    // ESP_LOGI(TAG, "Send meter status command:%04X %04X \r\n", data[0], data[1]);
}
//------------------------------------------------------------//

//-----------------------------------------------------//
// void handleMsg_setBattery_fun(int mIndex, MonitorPara *p_monitor_para)
void handleMsg_setBattery_fun(int mIndex, MonitorBat_info *p_monitor_para)
{
    int ret;
    uint16_t data[6] = {0};

    Bat_arr_t m_bats_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_bats_data);

    data[0] = p_monitor_para->batmonitor.dc_per;                                          // estore mach type
    data[1] = p_monitor_para->batmonitor.uu1;                                             // running model
    data[2] = p_monitor_para->batmonitor.up1;                                             // battery factory
    data[3] = p_monitor_para->batmonitor.uu2;                                             // battery model
    data[4] = p_monitor_para->batmonitor.up2;                                             // battery quantity
    ret = modbus_write_inv(m_bats_data[mIndex].modbus_id, INV_REG_ADDR_BATTERY, 5, data); /** 储能机种类别选择*/
    if (ret == ASW_OK)
    {
        ESP_LOGI(TAG, "**********com task->set battery index:data[0]=%d,data[1]=%d,"
                      "data[2]=%d,data[3]=%d,data[4]=%d\r\n",
                 data[0], data[1], data[2], data[3], data[4]);
    }

    memset(data, 0, sizeof(data));
    data[0] = p_monitor_para->batmonitor.chg_max * 100; // estore mach type
    data[1] = p_monitor_para->batmonitor.dchg_max * 100;
    ret = modbus_write_inv(m_bats_data[mIndex].modbus_id, INV_REG_ADDR_BATTERY_CHAGE_CONFIG, 2, data); /** 储能机种类别选择*/
    if (ret == ASW_OK)
    {
        ESP_LOGI(TAG, "**********com task->set battery dischage/chage info index:data[0]=%d,data[1]=%d", data[0], data[1]);
    }
}
///////////////////////////////////////////////////////
void handleMsg_broadCast_setBattery_fun(MonitorBat_info *p_monitor_para)
{
    int ret;
    uint16_t data[6] = {0};
    // MonitorPara *p_monitor_para = (MonitorPara)p1;
    data[0] = p_monitor_para->batmonitor.dc_per; // estore mach type
    data[1] = p_monitor_para->batmonitor.uu1;    // running model
    data[2] = p_monitor_para->batmonitor.up1;    // battery factory
    data[3] = p_monitor_para->batmonitor.uu2;    // battery model
    data[4] = p_monitor_para->batmonitor.up2;    // battery quantity
#if !PARALLEL_HOST_SET_WITH_SN

    MonitorPara monitor_para = {0};
    read_global_var(METER_CONTROL_CONFIG, &monitor_para);

    printf("\n===========handleMsg_broadCast_setBattery_fun[%d]=== \n", monitor_para.is_parallel);

    if (monitor_para.is_parallel == 0)

#else

    if (g_parallel_enable == 0)

#endif
    {

        for (uint8_t i = 0; i < INV_BROADCAST_NUM; i++)
        {
            modbus_write_inv(0, INV_REG_ADDR_BATTERY, 5, data); /** 储能机种类别选择*/
            usleep(INV_BROADCAST_DELAYMS);                      // 260ms
        }

        ESP_LOGI(TAG, "*****broadcast *****com task->set battery index:data[0]=%d,data[1]=%d,"
                      "data[2]=%d,data[3]=%d,data[4]=%d\r\n",
                 data[0], data[1], data[2], data[3], data[4]);

        memset(data, 0, sizeof(data));
        data[0] = p_monitor_para->batmonitor.chg_max * 100; // estore mach type
        data[1] = p_monitor_para->batmonitor.dchg_max * 100;

        for (uint8_t i = 0; i < INV_BROADCAST_NUM; i++)
        {
            modbus_write_inv(0, INV_REG_ADDR_BATTERY_CHAGE_CONFIG, 2, data); /** 储能机种类别选择*/

            usleep(INV_BROADCAST_DELAYMS); // 260ms
        }

        ESP_LOGI(TAG, "*********broad cast *com task->set battery dischage/chage info index:data[0]=%d,data[1]=%d", data[0], data[1]);
    }
    else
    {

#if PARALLEL_HOST_SET_WITH_SN
        modbus_write_inv(g_host_modbus_id, INV_REG_ADDR_BATTERY, 5, data); /** 储能机种类别选择*/
        usleep(200);
        modbus_write_inv(g_host_modbus_id, INV_REG_ADDR_BATTERY_CHAGE_CONFIG, 2, data); /** 储能机种类别选择*/

        ESP_LOGI(TAG, "********* cast inv host modbus_id:%d *com task->set battery dischage/chage info index:data[0]=%d,data[1]=%d",
                 g_host_modbus_id, data[0], data[1]);

#else
        modbus_write_inv(monitor_para.host_adr, INV_REG_ADDR_BATTERY, 5, data); /** 储能机种类别选择*/
        usleep(200);
        modbus_write_inv(monitor_para.host_adr, INV_REG_ADDR_BATTERY_CHAGE_CONFIG, 2, data); /** 储能机种类别选择*/

        ESP_LOGI(TAG, "********* cast inv host modbus_id:%d *com task->set battery dischage/chage info index:data[0]=%d,data[1]=%d",
                 monitor_para.host_adr, data[0], data[1]);
#endif
    }
}

//------------------------------------------------------//
void handleMsg_broadCast_setRunMode_fun(MonitorBat_info *p_monitor_para)
{
    int ret;
    uint16_t data[6] = {0};
    // MonitorPara *p_monitor_para = (MonitorPara)p1;

    data[0] = p_monitor_para->batmonitor.dc_per; // beast type
    data[1] = p_monitor_para->batmonitor.uu1;    // running model
#if !PARALLEL_HOST_SET_WITH_SN

    MonitorPara monitor_para = {0};
    read_global_var(METER_CONTROL_CONFIG, &monitor_para);

    if (monitor_para.is_parallel == 0)

#else
    if (g_parallel_enable == 0)
#endif
    {

        for (uint8_t i = 0; i < INV_BROADCAST_NUM; i++)
        {
            modbus_write_inv(0, INV_REG_ADDR_BATTERY, 2, data); /** 储能机种类别选择*/

            usleep(INV_BROADCAST_DELAYMS); // 260ms
        }

        ESP_LOGI(TAG, "***broad cast com task->set run mode index:data[0]=%d,data[1]=%d\r\n",
                 data[0], data[1]);
    }
    else
    {
#if PARALLEL_HOST_SET_WITH_SN
        modbus_write_inv(g_host_modbus_id, INV_REG_ADDR_BATTERY, 2, data); /** 储能机种类别选择*/
        ESP_LOGI(TAG, "*** cast inv host modebus_id:%d com task->set run mode index:data[0]=%d,data[1]=%d\r\n",
                 g_host_modbus_id, data[0], data[1]);

#else
        modbus_write_inv(monitor_para.host_adr, INV_REG_ADDR_BATTERY, 2, data); /** 储能机种类别选择*/

        modbus_write_inv(monitor_para.host_adr, INV_REG_ADDR_BATTERY, 2, data); /** 储能机种类别选择*/
        ESP_LOGI(TAG, "*** cast inv host modebus_id:%d com task->set run mode index:data[0]=%d,data[1]=%d\r\n",
                 monitor_para.host_adr, data[0], data[1]);
#endif
    }
}
//------------------------------------------------------//
void handleMsg_broadCast_setPower_fun(MonitorBat_info *p_monitor_para)
{
    int ret;
    // MonitorPara *p_monitor_para = (MonitorPara)p1;
    uint16_t midata = p_monitor_para->batmonitor.uu4;
#if !PARALLEL_HOST_SET_WITH_SN
    MonitorPara monitor_para = {0};
    read_global_var(METER_CONTROL_CONFIG, &monitor_para);
    if (monitor_para.is_parallel == 0)
#else
    if (g_parallel_enable == 0)

#endif
    {
        // if (is_inv_has_estore() == 1) /** 储能机*/
        for (uint8_t i = 0; i < INV_BROADCAST_NUM; i++)
        {
            modbus_write_inv(0, INV_REG_ADDR_POWER_ON, 1, &midata); /** 储能机开关: 41102*/
            usleep(INV_BROADCAST_DELAYMS);                          // 260ms
        }

        if (p_monitor_para->batmonitor.uu4 == 2)
            midata = 1;
        else
            midata = 0;

        for (uint8_t i = 0; i < INV_BROADCAST_NUM; i++)
        {
            modbus_write_inv(0, INV_REG_DRM_0N_OFF, 1, &midata); /** 启停： 40201*/
            usleep(INV_BROADCAST_DELAYMS);
        }

        ESP_LOGI(TAG, "****** Broadcast ****com task->set power on/off index:address=%d,data=%d\r\n",
                 INV_REG_ADDR_POWER_ON, p_monitor_para->batmonitor.uu4);
    }
    else
    {
#if PARALLEL_HOST_SET_WITH_SN
        // modbus_write_inv(g_host_modbus_id, INV_REG_ADDR_BATTERY, 2, data); /** 储能机种类别选择*/
        modbus_write_inv(g_host_modbus_id, INV_REG_ADDR_POWER_ON, 1, &midata); /** 储能机开关: 41102*/
        usleep(200);
        modbus_write_inv(g_host_modbus_id, INV_REG_DRM_0N_OFF, 1, &midata); /** 启停： 40201*/
        ESP_LOGI(TAG, "****** inv host modebus_id:%d ****com task->set power on/off index:address=%d,data=%d\r\n",
                 g_host_modbus_id, INV_REG_ADDR_POWER_ON, p_monitor_para->batmonitor.uu4);

#else
        modbus_write_inv(monitor_para.host_adr, INV_REG_ADDR_POWER_ON, 1, &midata); /** 储能机开关: 41102*/
        usleep(200);
        modbus_write_inv(monitor_para.host_adr, INV_REG_DRM_0N_OFF, 1, &midata); /** 启停： 40201*/
        ESP_LOGI(TAG, "****** inv host modebus_id:%d ****com task->set power on/off index:address=%d,data=%d\r\n",
                 monitor_para.host_adr, INV_REG_ADDR_POWER_ON, p_monitor_para->batmonitor.uu4);
#endif
    }
}
//---------------------------------------------------//

// 判断逻辑： 储能电池是玉泽（电池厂商代码是6）&& 逆变器通讯协议版本是2.1.2
void handleMsg_wrt_staInfo2Inv_fun()
{
    int ret;
    uint8_t buf[64] = {0};
    // uint8_t psswd_data[32] = {0};
    // MonitorPara *p_monitor_para = (MonitorPara)p1;

    wifi_sta_para_t sta_para = {0};
    general_query(NVS_STA_PARA, &sta_para);
    if (strlen((char *)sta_para.ssid) == 0 || strlen((char *)sta_para.password) == 0)
    {
        ESP_LOGW("-- Wrt SSID & PSW  WARN --", "the SSID or  PASSWORD is NULL.");
        xSemaphoreGive(g_semar_wrt_sync_reboot);
        return;
    }
    Bat_Monitor_arr_t monitor_para = {0};
    read_global_var(PARA_CONFIG, &monitor_para);

    char m_com_protocol_inv[13];

    memcpy(buf, sta_para.ssid, 32);
    memcpy(buf + 32, sta_para.password, 32);

    for (uint8_t i = 0; i < g_num_real_inv; i++)
    {
#if DEBUG_PRINT_ENABLE
        printf("\n-----------------------------------\n");
        printf("---- test sn ----,the monitor[%d] sn:%s, the inv[%d] sn:%s",
               i, monitor_para[i].sn, i, inv_arr[i].regInfo.sn);
        printf("\n-----------------------------------\n");
#endif
        memcpy(m_com_protocol_inv, &(inv_arr[i].regInfo.protocol_ver), 13);

        ESP_LOGI(TAG, "inv  %d protocol ver %s , bat_type:%d \n",
                 i, m_com_protocol_inv, monitor_para[i].batmonitor.dc_per);
        int8_t res = 0;
        if (monitor_para[i].batmonitor.dc_per == 6 && strncmp(m_com_protocol_inv, "V2.1.2", sizeof("V2.1.2")) == 0)
        {
            res = modbus_write_inv_bit8(inv_arr[i].regInfo.modbus_id, INV_REG_ADDR_SSID_PSSWD, 32, buf);
        }

        printf("-- write sta info to inv res:%d\n", res);

        usleep(INV_BROADCAST_DELAYMS);
    }

    ESP_LOGI(TAG, "****** ***com write inv ssid:%s  & password:%s \r\n",
             sta_para.ssid, sta_para.password);
    xSemaphoreGive(g_semar_wrt_sync_reboot);
}

//--------------------------------------------------------//
//写并联模式到逆变器
void handleMsg_wrt_parallelInfo2Inv_fun()
{

    // char m_com_protocol_inv[13];

    MonitorPara monitor_para = {0};
    read_global_var(METER_CONTROL_CONFIG, &monitor_para);

    /////////////  Lanstick-MutltivInv //////////////////

    // uint16_t data[2] = {0};
    // data[0] = 0;
    // data[1] = monitor_para.is_parallel;

    uint16_t data = monitor_para.is_parallel;

    for (uint8_t i = 0; i < INV_BROADCAST_NUM; i++)
    {
        modbus_write_inv(0, INV_REG_ADDR_SET_PARALLEL, 1, &data); /** 并联模式: 41118*/
        usleep(INV_BROADCAST_DELAYMS);                            // 260ms
        usleep(INV_BROADCAST_DELAYMS);                            // 260ms
    }

    // if (monitor_para.is_parallel)
    int res = modbus_write_inv(monitor_para.host_adr, INV_REG_ADDR_SET_HOST, 1, &data); /** 主从模式: 41117*/

    printf("\n====== handleMsg_wrt_parallelInfo2Inv_fun res[%d]=====\n ", res);
}

//----------------------------------------//
//----------------------------------------//
void handleMsg_broadCast_handleMsg_dspZvCld_fun()
{
    uint16_t data = 0;
    // int ret=0;

    wifi_sta_para_t sta_para = {0};
    //////////////////////
    Bat_arr_t m_arrbats_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_arrbats_data);

#if !PARALLEL_HOST_SET_WITH_SN

    MonitorPara monitor_para = {0};
    read_global_var(METER_CONTROL_CONFIG, &monitor_para);
#endif

    general_query(NVS_STA_PARA, &sta_para);
    if (strlen((char *)sta_para.ssid) == 0 && strlen((char *)sta_para.password) == 0)
    {
        data = 0x00AF;
    }
    else
    {
        if (g_state_mqtt_connect == 0) // Eng.Stg.Mch-Lanstick 20220907 +-
        {
            data = 0x000A;
        }
        else
        {
            data = 0x0005;
        }
    }
#if !PARALLEL_HOST_SET_WITH_SN
    if (monitor_para.is_parallel == 0) //普通模式下广播发送
#else
    if (g_parallel_enable == 0)                                                //普通模式下广播发送

#endif
    {
        for (uint8_t i = 0; i < INV_BROADCAST_NUM; i++)
        {
            modbus_write_inv(0, INV_REG_CLD_STATUS, 1, &data); /** 告知逆变器MQTT状态*/
            usleep(INV_BROADCAST_DELAYMS);                     // 260ms
        }

        ESP_LOGI(TAG, "******broadcast ****com task->set cloud status index:data=0x%04X\r\n", data);
    }
    else //并机模式下 主机发送
    {
#if PARALLEL_HOST_SET_WITH_SN

        modbus_write_inv(g_host_modbus_id, INV_REG_CLD_STATUS, 1, &data); /** 告知逆变器MQTT状态*/
        ESP_LOGI(TAG, "*******host inv***com %d,task->set cloud status index:data=0x%04X\r\n", g_host_modbus_id, data);

#else
        modbus_write_inv(monitor_para.host_adr, INV_REG_CLD_STATUS, 1, &data); /** 告知逆变器MQTT状态*/
        ESP_LOGI(TAG, "*******host inv***com task->set cloud status index:data=0x%04X\r\n", monitor_para.host_adr, data);
#endif
    }
}
//------------------------------------------------------//
void handleMsg_setRunMode_fun(int mIndex, MonitorBat_info *p_monitor_para)
{
    int ret;
    uint16_t data[6] = {0};
    Bat_arr_t m_bat_datas = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_bat_datas);
    // MonitorPara *p_monitor_para = (MonitorPara)p1;

    data[0] = p_monitor_para->batmonitor.dc_per; // beast type
    data[1] = p_monitor_para->batmonitor.uu1;    // running model

    ret = modbus_write_inv(m_bat_datas[mIndex].modbus_id, INV_REG_ADDR_BATTERY, 2, data); /** 储能机种类别选择*/
    if (ret == ASW_OK)
    {
        ESP_LOGI(TAG, "***com task->set run mode index:data[0]=%d,data[1]=%d\r\n",
                 data[0], data[1]);
    }
}
//------------------------------------------------------//
void handleMsg_setFirstRun_fun(int mIndex, MonitorBat_info *p_monitor_para)
{
    int ret;
    // uint16_t data[6] = {0};
    // MonitorPara *p_monitor_para = (MonitorPara)p1;

    Bat_arr_t m_bat_datas = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_bat_datas);

    uint16_t midata = p_monitor_para->batmonitor.up4;
    ret = modbus_write_inv(m_bat_datas[mIndex].modbus_id, INV_REG_ADDR_FIRST_ON, 1, &midata); /** 首次开机状态：41101*/
    if (ret == ASW_OK)
    {
        int inv_idx = get_estore_inv_arr_first();
        if (inv_idx >= 0)
        {
            inv_arr[inv_idx].status_first_running = p_monitor_para->batmonitor.up4;
            ESP_LOGI(TAG, "**********com task->set first power on/off index:address=%d,data=%d\r\n",
                     INV_REG_ADDR_FIRST_ON, p_monitor_para->batmonitor.up4);
        }
    }
}
//--------------------------------------------------------//
void handleMsg_setPower_fun(int mIndex, MonitorBat_info *p_monitor_para)
{
    Bat_arr_t m_arrbats_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_arrbats_data);
    int ret;
    // uint16_t data[6] = {0};
    // MonitorPara *p_monitor_para = (MonitorPara)p1;
    uint16_t midata = p_monitor_para->batmonitor.uu4;
    // if (is_inv_has_estore() == 1) /** 储能机*/
    if (inv_arr[mIndex].status && inv_arr[mIndex].regInfo.mach_type > 10) // estore && online
    {
        ret = modbus_write_inv(m_arrbats_data[mIndex].modbus_id, INV_REG_ADDR_POWER_ON, 1, &midata); /** 储能机开关: 41102*/
    }

    if (p_monitor_para->batmonitor.uu4 == 2)
        midata = 1;
    else
    {
        midata = 0;
    }
    ret = modbus_write_inv(m_arrbats_data[mIndex].modbus_id, INV_REG_DRM_0N_OFF, 1, &midata); /** 启停： 40201*/
    if (ret == ASW_OK)
    {
        ESP_LOGI(TAG, "**********com task->set power on/off index:address=%d,data=%d\r\n",
                 INV_REG_ADDR_POWER_ON, p_monitor_para->batmonitor.uu4);
    }
}
//------------------------------------------------------//
void handleMsg_loadSpeed_fun(int mIndex, MonitorBat_info *p_monitor_para)
{
    int ret;
    Bat_arr_t m_arrbats_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_arrbats_data);
    uint16_t data[6] = {0};
    // MonitorPara *p_monitor_para = (MonitorPara)p1;

    ret = modbus_holding_read_inv(m_arrbats_data[mIndex].modbus_id, INV_REG_ADDR_Q_PHASE, 1, data);
    if (ret == ASW_OK)
    {
        p_monitor_para->batmonitor.fq_ld_sp = data[0]; //[tgl mark] 测试下是否修改成功 传入的monitorpara
    }
}
//------------------------------------------------------//
void handleMsg_invTime_fun(int mIndex)
{
    int ret;
    DATE_STRUCT curr_date = {0};
    get_current_date(&curr_date);
    uint16_t u_time[6] = {0};
    u_time[0] = curr_date.YEAR;
    u_time[1] = curr_date.MONTH;
    u_time[2] = curr_date.DAY;
    u_time[3] = curr_date.HOUR;
    u_time[4] = curr_date.MINUTE;
    u_time[5] = curr_date.SECOND;
    //////////////////////
    Bat_arr_t m_arrbats_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_arrbats_data);

    ret = modbus_write_inv(m_arrbats_data[mIndex].modbus_id, INV_REG_ADDR_TIME, 6, u_time); /** 设置逆变器RTC时间*/
    if (ret == ASW_OK)
    {
        ESP_LOGI(TAG, "-------set time to inverter ok!! time: %d-%d-%d %d:%d:%d\n",
                 u_time[0], u_time[1], u_time[2], u_time[3], u_time[4], u_time[5]);
    }
    else
    {
        ESP_LOGI(TAG, "-------set time to inverter fail!! time: %d-%d-%d %d:%d:%d\n",
                 u_time[0], u_time[1], u_time[2], u_time[3], u_time[4], u_time[5]);
    }
}
//----------------------------------------------//
void handleMsg_comBoxTime_fun(int mIndex)
{
    uint16_t data[8] = {0};
    int ret;
    //////////////////////
    Bat_arr_t m_arrbats_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_arrbats_data);

    ret = modbus_holding_read_inv(m_arrbats_data[mIndex].modbus_id, INV_REG_ADDR_TIME, 6, data); /** 从逆变器对时*/
    if (ret == ASW_OK)
    {
        DATE_STRUCT date_time = {0};
        date_time.YEAR = data[0];
        date_time.MONTH = data[1];
        date_time.DAY = data[2];
        date_time.HOUR = data[3];
        date_time.MINUTE = data[4];
        date_time.SECOND = data[5];
        if (date_time.YEAR > 2020)
            set_current_date(&date_time);
    }
}

//-------------------------------------------------//
void handleMsg_dspZvCld_fun(int mIndex)
{
    uint16_t data = 0;
    // int ret=0;

    wifi_sta_para_t sta_para = {0};
    //////////////////////
    Bat_arr_t m_arrbats_data = {0};
    read_global_var(GLOBAL_BATTERY_DATA, &m_arrbats_data);

    general_query(NVS_STA_PARA, &sta_para);
    if (strlen((char *)sta_para.ssid) == 0 && strlen((char *)sta_para.password) == 0)
    {
        data = 0x00AF;
    }
    else
    {

        // if (get_mqtt_status() == 0)
        if (g_state_mqtt_connect == 0) // Eng.Stg.Mch-Lanstick 20220907 +-
        {
            data = 0x000A;
        }
        else
        {
            data = 0x0005;
        }
    }

    modbus_write_inv(m_arrbats_data[mIndex].modbus_id, INV_REG_CLD_STATUS, 1, &data); /** 告知逆变器MQTT状态*/
    ESP_LOGI(TAG, "**********com task->set cloud status index:data=%d\r\n", data);
}

//////////////////// Lanstick-MultilInv +/////////////////////

//------------------------------------//
int8_t asw_meter2inv_para_set(uint32_t msg_index)
{

    MonitorPara monitor_para = {0};
    meter_data_t inv_meter = {0};

    read_global_var(METER_CONTROL_CONFIG, &monitor_para);
    read_global_var(GLOBAL_METER_DATA, &inv_meter);
    switch (msg_index)
    {
    case MSG_PWR_ACTIVE_INDEX:
        handleMsg_pwrActive_fun(&monitor_para, &inv_meter);
        break;

        // zero export: send the meter status, while meter is offline.
    case MSG_INV_SET_ADV_INDEX:
        handleMsg_setAdv_fun();
        break;

    default:
        break;
    }
    task_inv_meter_msg &= (~msg_index);
    return 0;
}

//////////////////////////////////////////////////////////
///////////////////  broadcast ////////////////////////
int8_t inv_broadcast_set_para(uint32_t msg_index)
{
    Bat_Monitor_arr_t monitor_para_arr = {0};
    read_global_var(PARA_CONFIG, &monitor_para_arr);
    MonitorBat_info monitor_para = monitor_para_arr[0];

    switch (msg_index)
    {
        // battery: set battery information
    case MSG_BRDCST_SET_BATTERY_INDEX:
        handleMsg_broadCast_setBattery_fun(&monitor_para);
        break;

    case MSG_BRDCST_SET_RUN_MODE_INDEX:
        handleMsg_broadCast_setRunMode_fun(&monitor_para);
        break;

    case MSG_BRDCST_SET_POWER_INDEX:
        handleMsg_broadCast_setPower_fun(&monitor_para);
        break;

        //同步电池调度信息
    case MSG_BRDCST_GET_SAFETY_INDEX:
        handleMsg_getSafty_fun();
        break;

        //写sta联网信息到逆变器
    case MSG_WRT_STA_INFO_INDEX:
        handleMsg_wrt_staInfo2Inv_fun();
        break;

        //写并联模式到逆变器
    case MSG_WRT_SET_HOST_INDEX:
        handleMsg_wrt_parallelInfo2Inv_fun();
        break;
        //同步云端通讯状态到逆变器
    case MSG_BRDCST_DSP_ZV_CLD_INDEX:
        handleMsg_broadCast_handleMsg_dspZvCld_fun();
        break;

    default:
        break;
    }
    g_task_inv_broadcast_msg &= (~msg_index);

    return 0;
}

/////////////////////////////////////////////////////////////
int8_t inv_set_para(int inv_index, uint32_t msg_index)
{
    // MonitorPara monitor_para = {0};
    // meter_data_t inv_meter = {0};
    Bat_Monitor_arr_t monitor_para_arr = {0};

    // meter_data_t inv_meter = {0};

    read_global_var(PARA_CONFIG, &monitor_para_arr);
    // read_global_var(GLOBAL_METER_DATA, &inv_meter);

    MonitorBat_info monitor_para = monitor_para_arr[inv_index];

    switch (msg_index)
    {
        // battery: set battery information
    case MSG_SET_BATTERY_INDEX:
        handleMsg_setBattery_fun(inv_index, &monitor_para);
        break;

    // battery: set running mode
    case MSG_SET_RUN_MODE_INDEX:
        handleMsg_setRunMode_fun(inv_index, &monitor_para);
        break;

    // battery: set inverter charging / discharging
    case MSG_SET_CHARGE_INDEX:
        handleMsg_setChange_fun(inv_index, &monitor_para);
        break;

        // battery: set inverter power on/off
    case MSG_SET_POWER_INDEX:
        handleMsg_setPower_fun(inv_index, &monitor_para);
        break;

    case MSG_DSP_ZV_CLD_INDEX:
        handleMsg_dspZvCld_fun(inv_index);
        break;

    /** fresh setdefine of current day*/
    case MSG_GET_SAFETY_INDEX:
        handleMsg_getSafty_fun();
        break;

    default:
        break;
    }
    task_inv_msg_arr[inv_index] &= (~msg_index);
    return 0;
}

////////////////////////////////
int8_t setting_event_handler(void)
{
    static int8_t last_check_time = -1;
    static int drm_ld_sp[INV_NUM] = {0};
    int period = 0;
    int now_sec = get_second_sys_time(); // now_sec
    DATE_STRUCT curr_date;
    int8_t curr_check_time;
    uint16_t cur_minutes;
    bool in_schedule;
    uint8_t i;
    uint16_t uHour = 0;
    uint16_t uMinutes = 0;

    static int mIndex = 0; // Lanstick-MultilInv +
    /** 无逆变器在线*/

    if (mIndex++ > g_num_real_inv)
    {
        mIndex = 1;
    }

    if (inv_arr[mIndex - 1].status == 0) // estore && online
    {
        return 0;
    }

    /** 储能机处理*/
    if (task_inv_msg_arr[mIndex - 1] & MSG_INV_INDEX_GROUP)
    {
        printf("\n----------------------setting_event_handler -----------------\n  ");
        printf(" task inv msg value:%08x", task_inv_msg_arr[mIndex] & MSG_INV_INDEX_GROUP);
        printf("\n----------------------setting_event_handler -----------------\n  ");

        // battery information setting--1
        if (task_inv_msg_arr[mIndex - 1] & MSG_SET_BATTERY_INDEX)
            return inv_set_para(mIndex - 1, MSG_SET_BATTERY_INDEX);

        // battery running mode---2
        else if (task_inv_msg_arr[mIndex - 1] & MSG_SET_RUN_MODE_INDEX)
            return inv_set_para(mIndex - 1, MSG_SET_RUN_MODE_INDEX);

        // battery charging/discharging/stop --3
        else if (task_inv_msg_arr[mIndex - 1] & MSG_SET_CHARGE_INDEX)
            return inv_set_para(mIndex - 1, MSG_SET_CHARGE_INDEX);

        // set first running status after finished configuration --4
        else if (task_inv_msg_arr[mIndex - 1] & MSG_SET_FIRST_RUN_INDEX)
            return inv_set_para(mIndex - 1, MSG_SET_FIRST_RUN_INDEX);

        // set power on/off  --5
        else if (task_inv_msg_arr[mIndex - 1] & MSG_SET_POWER_INDEX)
            return inv_set_para(mIndex - 1, MSG_SET_POWER_INDEX);

        // set cloud status to inverter --9
        else if (task_inv_msg_arr[mIndex - 1] & MSG_DSP_ZV_CLD_INDEX)
            return inv_set_para(mIndex - 1, MSG_DSP_ZV_CLD_INDEX);

        // update charging/discharging immediately --10
        else if (task_inv_msg_arr[mIndex - 1] & MSG_GET_SAFETY_INDEX)
            return inv_set_para(mIndex - 1, MSG_GET_SAFETY_INDEX);
    }
    ////////////////////////////  add for broadcast ///////////////////////
    else if (g_task_inv_broadcast_msg != 0)
    {
        // setting? device=4,action=setbattery
        if (g_task_inv_broadcast_msg & MSG_BRDCST_SET_BATTERY_INDEX)
        {
            return inv_broadcast_set_para(MSG_BRDCST_SET_BATTERY_INDEX);
        }

        else if (g_task_inv_broadcast_msg & MSG_BRDCST_SET_RUN_MODE_INDEX)
        {
            return inv_broadcast_set_para(MSG_BRDCST_SET_RUN_MODE_INDEX); // only set running mode)
        }

        else if (g_task_inv_broadcast_msg & MSG_BRDCST_SET_POWER_INDEX)
        {
            return inv_broadcast_set_para(MSG_BRDCST_SET_POWER_INDEX);
        }

        else if (g_task_inv_broadcast_msg & MSG_BRDCST_GET_SAFETY_INDEX)
        {
            return inv_broadcast_set_para(MSG_BRDCST_GET_SAFETY_INDEX);
        }

        else if (g_task_inv_broadcast_msg & MSG_WRT_STA_INFO_INDEX)
        {
            return inv_broadcast_set_para(MSG_WRT_STA_INFO_INDEX);
        }

        else if (g_task_inv_broadcast_msg & MSG_WRT_SET_HOST_INDEX)
        {
            return inv_broadcast_set_para(MSG_WRT_SET_HOST_INDEX);
        }
        else if (g_task_inv_broadcast_msg & MSG_BRDCST_DSP_ZV_CLD_INDEX)
        {
            return inv_broadcast_set_para(MSG_BRDCST_DSP_ZV_CLD_INDEX);
        }
    }

    /* 广播消息 */
    /////////////////////////////////

    //////////////////////////////////
    /** 自定义充放电计划：调度*/

    Bat_Monitor_arr_t monitor_para = {0};
    // Bat_Schdle_arr_t schedule_bat = {0};
    ScheduleBat schedule_bat = {0};

    read_global_var(PARA_CONFIG, &monitor_para);
    read_global_var(PARA_SCHED_BAT, &schedule_bat);
    get_current_date(&curr_date);

    curr_check_time = check_time_correct();

    if (last_check_time == -1 && curr_check_time == 0)
    {
        g_task_inv_broadcast_msg |= MSG_BRDCST_GET_SAFETY_INDEX;
    }
    last_check_time = curr_check_time;

    if (monitor_para[mIndex - 1].batmonitor.dc_per == 1                               // Energy stored type
        && monitor_para[mIndex - 1].batmonitor.uu1 == 4 && check_time_correct() == 0) //  user define mode && date
    {
        cur_minutes = curr_date.HOUR * 60 + curr_date.MINUTE; /** 当前每天分钟数*/

        in_schedule = false; // not in schedule
        period = now_sec - drm_ld_sp[mIndex - 1];
        // if (period >= 30) // charging/discharging every 30 seconds
        if (period >= 2) // Eng.Stg.Mch change
        {
            ESP_LOGI(TAG, "******timestamp:%d start to check the schedule\n", now_sec);

            // Eng.Stg.Mch change
            drm_ld_sp[mIndex - 1] = now_sec;

            /*1. judge the current day time whether in schedule: > start time and < start time+duration*/
            for (i = 0; i < DAY_CHARG_SCH_NUM; i++)
            {
                /*current time as minutes - schedule time as minutes*/
                period = cur_minutes - Bat_DaySchdle_arr_t[i].minutes_inday; /** 当前时刻-计划开始时刻*/
                if (period >= 0 && period < Bat_DaySchdle_arr_t[i].duration)
                {
                    g_is_in_schedule[mIndex - 1] = 1; /** mem */

                    in_schedule = true; // in schedule
                    {
                        monitor_para[mIndex - 1].batmonitor.freq_mode = Bat_DaySchdle_arr_t[i].charge_flag; /** 充放电标志*/

                        ESP_LOGI(TAG, "----schedule cur_minutes:%d,minutes_inday:%d,period:%d, duration:%d  freq_mode:%d\n",
                                 cur_minutes, Bat_DaySchdle_arr_t[i].minutes_inday, period,
                                 Bat_DaySchdle_arr_t[i].duration, monitor_para[mIndex - 1].batmonitor.freq_mode);

                        write_global_var(PARA_CONFIG, &monitor_para);
                        task_inv_msg_arr[mIndex - 1] |= MSG_SET_CHARGE_INDEX;
                        return 0; /** 每30秒进来一次，发送最终充放电命令，跳出*/
                    }
                }
            }
            ESP_LOGI(TAG, "  inv:%d ,monitor_para.adv.freq_mode:%d\n", mIndex - 1, monitor_para[mIndex - 1].batmonitor.freq_mode);
            /*if out of the schedule, stop the current operation(charging/discharging)*/
            if (false == in_schedule) //以前版本的判断条件里还包括这个&& monitor_para.adv.freq_mode != BATTERY_CHARGING_STOP
            {
                g_is_in_schedule[mIndex - 1] = 0; /** mem */

                ESP_LOGI(TAG, "set monitor_para.adv.freq_mode = BATTERY_CHARGING_STOP\n");
                monitor_para[mIndex - 1].batmonitor.freq_mode = BATTERY_CHARGING_STOP;
                write_global_var(PARA_CONFIG, &monitor_para);
                task_inv_msg_arr[mIndex - 1] |= MSG_SET_CHARGE_INDEX;
                return 0;
            }
        }
    }
    //--- Eng.Stg.Mch-Lanstick 20220907 +
    else
    {
        g_is_in_schedule[mIndex - 1] = 0; /** mem */
    }

    /* Clear the INV SN INVALID FLAG */
    /* one day passed, change the charging/discharging schedule */
    /** 日发生变化，刷新*/
    if ((cur_week_day != curr_date.WDAY) && check_time_correct() == ASW_OK)
    {
        ESP_LOGI(TAG, "week day change, old new %d %d\n", cur_week_day, curr_date.WDAY);
        // monitor_state &=(~INV_SN_INVALID_INDEX);
        cur_week_day = curr_date.WDAY;

        // MSG_CONSUME(monitor.app_id[1],INV_SN_INVALID_INDEX);
        /*calculate the dayly detail schedule after synchronize the time from cloud or inverter*/
        /** 每天，最多6段充放电计划*/
        for (int i = 0; i < DAY_CHARG_SCH_NUM; i++)
        {
            uHour = (schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0xFF000000) >> 24;
            uMinutes = (schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0x00FF0000) >> 16;
            Bat_DaySchdle_arr_t[i].minutes_inday = uHour * 60 + uMinutes;
            Bat_DaySchdle_arr_t[i].duration = ((schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0x0000FF00) >> 8);
            Bat_DaySchdle_arr_t[i].charge_flag = schedule_bat.daySchedule[cur_week_day].time_sch[i] & 0x000000FF;
        }
        // write_global_var(PARA_SCHED_BAT, &schedule_bat);
    }

    /** Check active & inactive power adjust */
    /** 2. get the device id information*/
    /** 3. get inverter data every 10 seconds*/
    /** 4. set smater meter status*/
    if (task_inv_meter_msg & MSG_PWR_ACTIVE_INDEX)
    {
        // ESP_LOGI(TAG,"****MSG_PWR_ACTIVE_INDEX\n");
        return asw_meter2inv_para_set(MSG_PWR_ACTIVE_INDEX); /** 电表在线,有功功率调节*/
    }
    else if (task_inv_meter_msg & MSG_INV_SET_ADV_INDEX)
    {
        // ESP_LOGI(TAG,"****MSG_INV_SET_ADV_INDEX\n");
        return asw_meter2inv_para_set(MSG_INV_SET_ADV_INDEX); /** 电表离线*/
    }
    /*DRM priority is the last one*/
    // dred_fun_ctrl(0);

    return 0;
}

//--------------------------------------//
