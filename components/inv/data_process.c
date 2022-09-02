#include "data_process.h"


// ScheduleBat g_schedule_bat = {0};
int g_drm_value = 0;

uint32_t task_cld_msg = 0;
uint32_t task_inv_msg = 0;
uint32_t task_other_msg = 0;
// uint32_t event_group_0 = 0;

char g_p2p_mode = 0; //p2p 


//kaco-lanstick 20220811 +
int g_kaco_set_addr = 3;
uint16_t event_group_0 = 0;

//MonitorPara g_monitor_para = {0}; //  move->asw_mux.c

// Batt_data g_bat_data = {0};
// meter_data_t g_inv_meter = {0};


void get_current_date(DATE_STRUCT *date)
{
    struct tm currtime = {0};
    time_t t = time(NULL);
    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;

    date->YEAR = currtime.tm_year;
    date->MONTH = currtime.tm_mon;
    date->DAY = currtime.tm_mday;
    date->HOUR = currtime.tm_hour;
    date->MINUTE = currtime.tm_min;
    date->SECOND = currtime.tm_sec;
    date->WDAY = currtime.tm_wday;
    date->YDAY = currtime.tm_yday;
}

void set_current_date(DATE_STRUCT *date)
{
    struct timeval tv;
    struct tm ptime = {0};
    ptime.tm_year = date->YEAR - 1900;
    ptime.tm_mon = date->MONTH - 1;
    ptime.tm_mday = date->DAY;
    ptime.tm_hour = date->HOUR;
    ptime.tm_min = date->MINUTE;
    ptime.tm_sec = date->SECOND;

    tv.tv_sec = mktime(&ptime);
    settimeofday(&tv, NULL);
}
