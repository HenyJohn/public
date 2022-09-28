#include <stdio.h>
#include <sys/time.h>
#include "stdint.h"
#include "data_process.h"
#include "tianhe.h"
#include <string.h>
#include <time.h>

#define RAW "\033[0m"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"

extern InvRegister g_inv_info;
static base_para_t g_base_para = {0};

char g_tianhe_host[50] = {0};
int g_tianhe_port = 0;

void hex_print_frame(uint8_t *buf, int len)
{
    if (len <= 0)
        return;
    int i = 0;
    printf("\n<HEX>\n");
    for (int i = 0; i < len; i++)
        printf("%02X ", (uint8_t)buf[i]);
    printf("\n</HEX>\n");
}

void base_para_ini(base_para_t base_para)
{
    g_base_para = base_para;
}

int tianhe_get_filter_head(uint8_t *buf)
{
    int len = 0;
    buf[0] = 0x54;
    buf[1] = 0x53;
    len += 2;
    memcpy(&buf[len], g_base_para.inv_sn, strlen(g_base_para.inv_sn));
    len += 25;
    memcpy(&buf[len], g_base_para.pmu_sn, strlen(g_base_para.pmu_sn));
    len += 25;
    int i = 0;
    printf("tianhe_get_filter_head: %d\n", len);
    for (int i = 0; i < len; i++)
        printf("%02X ", buf[i]);
    printf("\n\n");
    return len;
}

static int create_send_frame(uint8_t *msg, uint16_t cmd, uint8_t *data, int data_len)
{
    uint8_t buf[1400] = {0};
    int len = 0;

    buf[0] = 0x54;
    buf[1] = 0x53;
    len += 2;
    memcpy(&buf[len], g_base_para.inv_sn, strlen(g_base_para.inv_sn));
    len += 25;
    memcpy(&buf[len], g_base_para.pmu_sn, strlen(g_base_para.pmu_sn));
    len += 25;

    buf[len] = (cmd >> 8) & 0xFF;
    buf[len + 1] = cmd & 0xFF;
    len += 2;

    buf[len] = (TIANHE_VER >> 8) & 0xFF;
    buf[len + 1] = TIANHE_VER & 0xFF;
    len += 2;

    buf[len] = (data_len >> 8) & 0xFF;
    buf[len + 1] = data_len & 0xFF;
    len += 2;

    /** data block: */
    if (data != NULL)
    {
        memcpy(buf + len, data, data_len);
        len += data_len;
    }

    uint16_t crc = crc16_calc(buf, len);
    buf[len] = (crc >> 8) & 0xFF;
    buf[len + 1] = crc & 0xFF;
    len += 2;

    printf("create_send_frame: %d\n", len);
    for (int i = 0; i < len; i++)
        printf("%02X ", buf[i]);
    printf("\n\n");

    if (msg != NULL)
        memcpy(msg, buf, len);
    return len;
}

void get_tianhe_fmt_time(uint8_t *buf)
{
    struct tm *info;
    time_t t = time(0);
    info = localtime(&t);
    int YY = info->tm_year + 1900;
    int MM = info->tm_mon + 1;
    int DD = info->tm_mday;
    int hh = info->tm_hour;
    int mm = info->tm_min;
    int ss = info->tm_sec;

    printf("time:%d-%02d-%02d %02d:%02d:%02d\n", YY, MM, DD, hh, mm, ss);
    char time[7] = {0};

    int var;
    var = YY / 100 % 10 + YY / 1000 % 10 * 16;
    time[0] = var;
    var = YY % 10 + YY / 10 % 10 * 16;
    time[1] = var;
    var = MM % 10 + MM / 10 % 10 * 16;
    time[2] = var;
    var = DD % 10 + DD / 10 % 10 * 16;
    time[3] = var;
    var = hh % 10 + hh / 10 % 10 * 16;
    time[4] = var;
    var = mm % 10 + mm / 10 % 10 * 16;
    time[5] = var;
    var = ss % 10 + ss / 10 % 10 * 16;
    time[6] = var;
    printf("KKKK------------------------------: ss\n");

    memcpy(buf, time, 7);
}

void get_tianhe_time_from_std(uint8_t *out, uint8_t *in)
{
    int YY;
    int MM;
    int DD;
    int hh;
    int mm;
    int ss;
    int parsed_num = 0;
    parsed_num = sscanf(in, "%d-%d-%d %d:%d:%d", &YY, &MM, &DD, &hh, &mm, &ss);
    if (parsed_num != 6)
        return;
    printf("parse hist time:%d-%02d-%02d %02d:%02d:%02d\n", YY, MM, DD, hh, mm, ss);
    char time[7] = {0};
    int var;
    var = YY / 100 % 10 + YY / 1000 % 10 * 16;
    time[0] = var;
    var = YY % 10 + YY / 10 % 10 * 16;
    time[1] = var;
    var = MM % 10 + MM / 10 % 10 * 16;
    time[2] = var;
    var = DD % 10 + DD / 10 % 10 * 16;
    time[3] = var;
    var = hh % 10 + hh / 10 % 10 * 16;
    time[4] = var;
    var = mm % 10 + mm / 10 % 10 * 16;
    time[5] = var;
    var = ss % 10 + ss / 10 % 10 * 16;
    time[6] = var;
    printf("KKKK------------------------------: ss\n");

    memcpy(out, time, 7);
}

int get_login_frame(uint8_t *msg, login_para_t para)
{
    char *sp;
    int len = 0;
    uint8_t data[1400] = {0};
    memcpy(data + len, para.mfr, 2);
    len += 2;
    memcpy(data + len, para.type, 13);
    len += 13;
    memcpy(data + len, para.iccid, 20);
    len += 20;
    printf("pra=======>>>>%s  %s \n", data, para.type);
    char time[7] = {0};
    get_tianhe_fmt_time(time);
    memcpy(data + len, time, 7);
    len += 7;
    /** PMU VER*/
    get_pmu_ver(data + len);
    len += 15;

    /** ECU VER*/
    len += 15;

    /** INV VER --DSP*/
    sp = strstr(g_inv_info.msw_ver, "V610");
    if (sp != NULL)
        memcpy(data + len, sp, 13);
    len += 15;

    /** INV VER --ARM*/
    sp = strstr(g_inv_info.tmc_ver, "V610");
    if (sp != NULL)
        memcpy(data + len, sp, 13);
    len += 15;

    /** INV VER --COMM CHIP*/
    len += 15;

    /** INV VER --LCD*/
    len += 15;

    data[len] = 0x01;
    len += 1;

    int res = 0;
    if (msg != NULL)
    {
        printf("get_login_frame:\n");
        res = create_send_frame(msg, CMD_LOGIN, data, len);
    }
    return res;
}

// extern uint16_t g_pwr_rate;
extern uint16_t g_gnd_pe_en;
extern uint16_t g_grid_switch;
int get_invdata_frame(Inv_data invdata, uint8_t *msg, int is_hist)
{
    int len = 0;
    int i;
    uint16_t var = 0;
    uint8_t data[1400] = {0};

    uint16_t temp = 0;
    temp = invdata.col_temp + 1000; //T=(t+100)*10=t*10+1000
    data[0] = (temp >> 8) & 0xFF;
    data[1] = temp & 0xFF;
    len = 2;

    /** 6 MPPT*/
    for (i = 0; i < 6; i++)
    {
        if ((invdata.PV_cur_voltg[i].iVol != 0xFFFF) && (invdata.PV_cur_voltg[i].iCur != 0xFFFF))
        {
            data[2 + 2 * i] = (invdata.PV_cur_voltg[i].iVol >> 8) & 0xFF;
            data[3 + 2 * i] = invdata.PV_cur_voltg[i].iVol & 0xFF;

            var = invdata.PV_cur_voltg[i].iCur / 10;
            data[62 + 2 * i] = (var >> 8) & 0xFF; //2+2*30
            data[63 + 2 * i] = var & 0xFF;
        }
    }
    len = 122; //2+4*30
    for (i = 0; i < 3; i++)
    {
        if (invdata.AC_vol_cur[i].iVol != 65535 && invdata.AC_vol_cur[i].iCur != 65535)
        {

            data[122 + 2 * i] = (invdata.AC_vol_cur[i].iVol >> 8) & 0xFF;
            data[123 + 2 * i] = invdata.AC_vol_cur[i].iVol & 0xFF;

            data[130 + 2 * i] = (invdata.AC_vol_cur[i].iCur >> 8) & 0xFF;
            data[131 + 2 * i] = invdata.AC_vol_cur[i].iCur & 0xFF;
        }
    }
    len = 136;
    len = 138;

    int ac_num = 1;
    if (g_inv_info.type == 0x33)
        ac_num = 3;
    else
        ac_num = 1;
    for (i = 0; i < ac_num; i++)
    {
        data[138 + 2 * i] = (invdata.fac >> 8) & 0xFF;
        data[139 + 2 * i] = invdata.fac & 0xFF;
    }
    len = 144;
    len = 146;
    uint32_t power = invdata.pac;
    data[146] = (power >> 16) & 0xFF;
    data[147] = (power >> 8) & 0xFF;
    data[148] = power & 0xFF;
    len = 149;

    uint32_t e_today = invdata.e_today * 10;
    data[149] = (e_today >> 16) & 0xFF;
    data[150] = (e_today >> 8) & 0xFF;
    data[151] = e_today & 0xFF;
    len = 152;

    uint32_t e_total = invdata.e_total;
    data[152] = (e_total >> 24) & 0xFF;
    data[153] = (e_total >> 16) & 0xFF;
    data[154] = (e_total >> 8) & 0xFF;
    data[155] = (e_total >> 0) & 0xFF;
    len = 156;
    uint32_t h_total = invdata.h_total;
    data[156] = (h_total >> 24) & 0xFF;
    data[157] = (h_total >> 16) & 0xFF;
    data[158] = (h_total >> 8) & 0xFF;
    data[159] = (h_total >> 0) & 0xFF;
    len = 160;
    len = 162;
    uint8_t grid_switch_state = 0xBE; //0xBE:ON, 0xDE:OFF
    if (g_grid_switch == 1)
        grid_switch_state = 0xBE;
    else
    {
        grid_switch_state = 0xDE;
    }

    data[162] = grid_switch_state;
    len = 163;
    uint8_t pwr_curve_no = 0x06;
    data[163] = pwr_curve_no;
    len = 164;
    len = 166;

    uint16_t warning = 0;
    for (int k = 0; k < 10; k++)
    {
        if (invdata.warn[k] != 0 && invdata.warn[k] != 0xFF)
        {
            warning = invdata.warn[k];
            break;
        }
    }
    if (g_grid_switch == 0)
    {
        warning = 60000; /** report on off*/
    }
    data[166] = (warning >> 8) & 0xFF;
    data[167] = (warning >> 0) & 0xFF;

    uint16_t error = 0;
    if (invdata.error != 0 && invdata.error != 0xFFFF)
    {
        error = invdata.error;
    }

    if (error == 0)
    {
        for (int k = 0; k < 10; k++)
        {
            if (invdata.warn[k] == 50)
            {
                error = 1050;
                break;
            }
        }
    }

    data[168] = (error >> 8) & 0xFF;
    data[169] = (error >> 0) & 0xFF;
    len = 170;

    uint8_t tianhe_time[7] = {0};
    if (is_hist == 0)
    {
        get_tianhe_fmt_time(tianhe_time);
    }
    else
    {
        //2021-01-26 12:51:21
        get_tianhe_time_from_std(tianhe_time, invdata.time);
        /* code */
    }

    memcpy(data + 170, tianhe_time, 7);
    len = 177;

    uint16_t pwr_rate = 0x1027;
    data[177] = (invdata.pac_out_percent >> 8) & 0xFF;
    data[178] = invdata.pac_out_percent & 0xFF;
    len = 179;

    uint32_t power_p = invdata.pac;
    data[179] = (power_p >> 16) & 0xFF;
    data[180] = (power_p >> 8) & 0xFF;
    data[181] = (power_p >> 0) & 0xFF;
    len = 182;
    uint32_t power_q = invdata.qac;
    data[182] = (power_q >> 16) & 0xFF;
    data[183] = (power_q >> 8) & 0xFF;
    data[184] = (power_q >> 0) & 0xFF;
    len = 185;
    uint32_t power_s = invdata.sac;
    data[185] = (power_s >> 16) & 0xFF;
    data[186] = (power_s >> 8) & 0xFF;
    data[187] = (power_s >> 0) & 0xFF;
    len = 188;
    uint16_t pf = invdata.cosphi * 10;
    data[188] = (pf >> 8) & 0xFF;
    data[189] = pf & 0xFF;
    len = 190;
    len = 192;
    uint8_t lcd_ver = 0xFF;
    data[192] = lcd_ver;
    len = 193;

    data[193] = g_gnd_pe_en;
    len = 194;

    int res = 0;
    if (msg != NULL)
    {
        printf("get_login_frame:\n");
        if (is_hist == 0)
            res = create_send_frame(msg, CMD_SEND_INV_DATA, data, len);
        else
        {
            res = create_send_frame(msg, CMD_SEND_HIST_INV_DATA, data, len);
        }
    }
    return res;
}

int get_keepalive_frame(uint8_t *msg)
{
    int len = 0;
    uint8_t *data = NULL;
    int res = 0;
    if (msg != NULL)
        res = create_send_frame(msg, CMD_KEEP_ALIVE, data, len);
    return res;
}

int get_server_time_frame(uint8_t *msg)
{
    int len = 0;
    uint8_t *data = NULL;
    int res = 0;
    if (msg != NULL)
        res = create_send_frame(msg, CMD_GET_TIME, data, len);
    return res;
}

int set_tianhe_fmt_time(uint8_t *buf)
{
    struct tm tm = {0};
    struct timeval tv = {0};
    tm.tm_year = (buf[0] / 16 * 10 + buf[0] % 16) * 100 + buf[1] / 16 * 10 + buf[1] % 16 - 1900;
    tm.tm_mon = buf[2] / 16 * 10 + buf[2] % 16 - 1;
    tm.tm_mday = buf[3] / 16 * 10 + buf[3] % 16;
    tm.tm_hour = buf[4] / 16 * 10 + buf[4] % 16;
    tm.tm_min = buf[5] / 16 * 10 + buf[5] % 16;
    tm.tm_sec = buf[6] / 16 * 10 + buf[6] % 16;

    if (tm.tm_year >= 2021 - 1900)
    {
        set_print_color(YELLOW);
        printf("set local time ok:\n");
        printf("%d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        set_print_color(BLUE);
        tv.tv_sec = mktime(&tm);
        settimeofday(&tv, NULL);
        return 0;
    }
    else
    {
        printf("year error: %d\n", tm.tm_year);
        return -1;
    }
}

/** application ===============================================================================================*/
int tianhe_login(login_para_t para)
{
    uint8_t buf[1400] = {0};
    int len = get_login_frame(buf, para);
    cat1_tcp_send(buf, len);
    memset(buf, 0, sizeof(buf));
    printf("KKKK------------------------------1\n");
    int res = cat1_tcp_recv(buf, &len);
    printf("KKKK------------------------------2\n");
    if (len > 0 && res == 0)
    {
        set_print_color(YELLOW);
        printf("tcp recv:%d\n\n%.*s\n", len, len, buf);
        printf("<RECV TIANHE DATA==============>\n");
        for (int i = 0; i < len; i++)
            printf("%02X ", (uint8_t)buf[i]);
        printf("\n</RECV TIANHE DATA==============>\n");
        set_print_color(BLUE);
        if (len == 7) /** 7字节是时间*/
        {
            printf("tianhe_login: successful\n");
            return 0;
        }
    }
    printf("tianhe_login: 333\n");
    set_print_color(RED);
    printf("tianhe_login error\n");
    printf("login response len: %d\n", len);
    hex_print_frame(buf, len);
    set_print_color(BLUE);
    return -1;
}

int tianhe_send_invdata(Inv_data invdata)
{
    uint8_t buf[1400] = {0};
    printf("KKKK------------------------------=1\n");
    int len = get_invdata_frame(invdata, buf, 0);
    printf("KKKK------------------------------=2\n");
    cat1_tcp_send(buf, len);
    memset(buf, 0, sizeof(buf));
    printf("KKKK------------------------------=3\n");
    int res = cat1_tcp_recv(buf, &len);
    printf("KKKK------------------------------=4\n");
    if (res == 0)
    {
        set_print_color(YELLOW);
        printf("tianhe send_invdata ok\n");
        set_print_color(BLUE);
        return 0;
    }

    set_print_color(RED);
    printf("tianhe send_invdata error\n");
    set_print_color(BLUE);

    return -1;
}

int tianhe_send_hist_invdata(Inv_data invdata)
{
    uint8_t buf[1400] = {0};
    int len = get_invdata_frame(invdata, buf, 1);
    cat1_tcp_send(buf, len);
    memset(buf, 0, sizeof(buf));

    int res = cat1_tcp_recv(buf, &len);

    if (res == 0)
    {
        set_print_color(YELLOW);
        printf("tianhe_send_hist_invdata: ok\n");
        set_print_color(BLUE);
        return 0;
    }

    set_print_color(RED);
    printf("tianhe_send_hist_invdata: error\n");
    set_print_color(BLUE);

    return -1;
}

int tianhe_keepalive(void)
{
    uint8_t buf[1400] = {0};
    int len = get_keepalive_frame(buf);
    cat1_tcp_send(buf, len);
    memset(buf, 0, sizeof(buf));

    int res = cat1_tcp_recv(buf, &len);
    if (res == 0)
    {
        set_print_color(YELLOW);
        printf("tianhe_keepalive res ok\n");
        set_print_color(BLUE);
        return 0;
    }

    set_print_color(RED);
    printf("tianhe_keepalive error\n");
    set_print_color(BLUE);

    return -1;
}

int tianhe_keepalive_sec(void)
{
    static int last_keepalive_sec = 0;
    int res = -1;

    if (cat1_get_tcp_status() == 0)
        return -1;

    int now_sec = get_second_sys_time();
    if (now_sec - last_keepalive_sec > 90)
    {

        res = tianhe_keepalive();
        if (res == 0)
        {
            last_keepalive_sec = now_sec;
            cat1_set_tcp_ok();
        }
        else
        {
            cat1_set_tcp_error();
        }

        return res;
    }
    return 0;
}

int tianhe_get_server_time(void)
{
    uint8_t buf[1400] = {0};
    int len = get_server_time_frame(buf);
    cat1_tcp_send(buf, len);
    memset(buf, 0, sizeof(buf));
    int res = cat1_tcp_recv(buf, &len);

    if (len == 7 && res == 0)
    {
        res = set_tianhe_fmt_time(buf);
        if (res == 0)
        {
            printf("tianhe_get_server_time: ok\n");
            return 0;
        }
    }

    set_print_color(RED);
    printf("tianhe_get_server_time error\n");
    hex_print_frame(buf, len);
    set_print_color(BLUE);

    return -1;
}

/** recv handler =========================================================================*/

int create_response_frame(uint8_t *req, uint16_t req_len, uint8_t *data, uint16_t data_len, uint8_t *res)
{
    int len;
    memcpy(res, req, 56);
    uint16_t cmd = (req[52] << 8) + req[53];
    cmd = cmd + 0x0800;
    res[52] = (cmd >> 8) & 0xFF;
    res[53] = cmd & 0xFF;

    res[56] = (data_len >> 8) & 0xFF;
    res[57] = data_len & 0xFF;

    if (data != NULL)
        memcpy(res + 58, data, data_len);
    uint16_t crc = crc16_calc(res, 58 + data_len);
    len = 58 + data_len;
    res[len] = (crc >> 8) & 0xFF;
    res[len + 1] = crc & 0xFF;
    len += 2;
    set_print_color(YELLOW);
    printf("<CMD RESPONSE==============>\n");
    for (int i = 0; i < len; i++)
        printf("%02X ", (uint8_t)res[i]);
    printf("</CMD RESPONSE==============>\n");
    set_print_color(BLUE);

    return len;
}

extern char g_ccid[22];
extern char g_oper[30];
extern char g_imei[20];
extern char g_imsi[20];
extern int g_4g_mode;
extern char g_lac[4];
extern char g_ci[4];
void set_server_handler(uint8_t *buf, int len)
{
    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);

    /** tcp reconnect, save*/
    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, NULL, 0, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("response ok: set_server_handler\n");
    set_print_color(BLUE);

    if (len == 58 + 52 + 2)
    {
        // memcpy(g_tianhe_host, buf + 58, 50);
        // g_tianhe_port = (buf[58 + 50] << 8) + buf[58 + 50 + 1];
        memset(setting.host, 0, sizeof(setting.host));
        memcpy(setting.host, buf + 58, 50);
        setting.port = (buf[58 + 50] << 8) + buf[58 + 50 + 1];
        if (strlen(setting.host) > 0)
        {
            printf("new host:%s\n", setting.host);
            printf("new port:%d\n", setting.port);
            general_add(NVS_ATE, &setting);
            printf("host change ok\n");
            sleep(2);
            esp_restart();
        }
        else
        {
            printf("host setting is null\n");
        }
    }
}

#include "update.h"
void upgrade_handler(uint8_t *buf, int len)
{
    if (len == 58 + 114 + 2)
    {
        char frame[1300] = {0};
        int send_len = create_response_frame(buf, len, NULL, 0, frame);
        cat1_tcp_send(frame, send_len);
        set_print_color(YELLOW);
        printf("upgrade_hander: response ok\n");
        set_print_color(BLUE);

        update_url para = {0};
        uint8_t url_type = 0;
        uint8_t fw_type = 0;
        // uint8_t url[101] = {0};
        uint8_t user_name[7] = {0};
        uint8_t psw[7] = {0};

        url_type = buf[58];
        fw_type = buf[59];
        memcpy(para.down_url, &buf[60], 100);
        memcpy(user_name, &buf[160], 6);
        memcpy(psw, &buf[166], 6);

        printf("upgrade url:%s\n", para.down_url);

        if (url_type == 0)
        {
            if (fw_type == 0)
            {
                para.update_type = 1;
            }
            else if (fw_type == 1)
            {
                para.update_type = 2;
            }
            download_task(&para);
        }
    }
}

void off_handler(uint8_t *buf, int len)
{
    //turn off ac side
    //40201:0
    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, NULL, 0, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("off_handler: response ok\n");
    set_print_color(BLUE);

    uint8_t id = 0x03;
    uint8_t fc = 0x06; //single 4x
    uint16_t addr = 40201 - 40001;
    uint16_t var = 0;
    uint16_t crc = 0;

    uint8_t msg[20] = {0};
    int msg_len = 0;
    msg[0] = id;
    msg[1] = fc;
    msg[2] = (addr >> 8) & 0xFF;
    msg[3] = (addr >> 0) & 0xFF;
    msg[4] = (var >> 8) & 0xFF;
    msg[5] = (var >> 0) & 0xFF;
    crc = crc16_calc(msg, 6);
    msg[6] = (crc >> 0) & 0xFF;
    msg[7] = (crc >> 8) & 0xFF;
    msg_len = 8;

    tianhe_fdbg_send(msg, msg_len);
    memset(msg, 0, sizeof(msg));
    tianhe_fdbg_recv(msg, &msg_len);
    set_tianhe_onoff(0);
    event_group_0 |= CMD_INV_ONOFF;
}

void on_handler(uint8_t *buf, int len)
{
    //turn on ac side
    //40201:0
    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, NULL, 0, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("on_handler: response ok\n");
    set_print_color(BLUE);

    uint8_t id = 0x03;
    uint8_t fc = 0x06; //single 4x
    uint16_t addr = 40201 - 40001;
    uint16_t var = 1;
    uint16_t crc = 0;

    uint8_t msg[20] = {0};
    int msg_len = 0;
    msg[0] = id;
    msg[1] = fc;
    msg[2] = (addr >> 8) & 0xFF;
    msg[3] = (addr >> 0) & 0xFF;
    msg[4] = (var >> 8) & 0xFF;
    msg[5] = (var >> 0) & 0xFF;
    crc = crc16_calc(msg, 6);
    msg[6] = (crc >> 0) & 0xFF;
    msg[7] = (crc >> 8) & 0xFF;
    msg_len = 8;

    tianhe_fdbg_send(msg, msg_len);
    memset(msg, 0, sizeof(msg));
    tianhe_fdbg_recv(msg, &msg_len);
    set_tianhe_onoff(1);
    event_group_0 |= CMD_INV_ONOFF;
}

void pmu_restart_handler(uint8_t *buf, int len)
{
    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, NULL, 0, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("upgrade_hander: response ok\n");
    set_print_color(BLUE);

    sleep(3);
    esp_restart();
}

extern char g_pmu_ver[20];
void get_tianhe_dev_type(uint8_t *type)
{
    uint8_t data[13] = {0};
    memcpy(data, "TSSN", 4);
    int kw = g_inv_info.rated_pwr / 1000;
    char buf[10] = {0};
    sprintf(buf, "%02d", kw);
    memcpy(data + 4, buf, 2);
    data[6] = 'K';
    data[7] = 'T';
    data[8] = 'L';
    if (g_inv_info.type == 0x33)
    {
        data[9] = '3';
    }
    else
    {
        data[9] = '1';
    }
    data[10] = 'E';
    data[11] = 'S';
    if (g_inv_info.pv_num >= 0)
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%1d", g_inv_info.pv_num);
        memcpy(&data[12], buf, 1);
    }

    memcpy(type, data, 13);
    printf("type==== %s ===%s\n", type, data);
}

void dev_info_handler(uint8_t *buf, int len)
{
    uint8_t data[53] = {0};
    char *mfr = "TE";
    // char *type = "type";
    // char *iccid = "89861119208035797428";
    char time[7] = {0};
    // char *pmu_ver = "1.0.0";
    char GB = 0x01;
    memcpy(data, mfr, strlen(mfr));
    // memcpy(data + 2, type, strlen(type));
    get_tianhe_dev_type(&data[2]);
    memcpy(data + 15, g_ccid, strlen(g_ccid));
    get_tianhe_fmt_time(time);
    memcpy(data + 35, time, 7);

    memcpy(data + 42, g_pmu_ver, strlen(g_pmu_ver));
    data[52] = 0x01;

    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, data, 53, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("response ok: send dev info\n");
    set_print_color(BLUE);
}

//45212:U16 0.1   --- max
//45221:U16 0.1   --- min
void set_safe_volt_handler(uint8_t *buf, int len)
{
    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, NULL, 0, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("set_safe_volt_handler: response ok\n");
    set_print_color(BLUE);

    uint16_t max_volt = 0;
    uint16_t min_volt = 0;
    uint16_t var = 0;

    uint8_t safe_code = 0;
    safe_code = buf[58];
    printf("get safe code: %d\n", (int)safe_code);
    if (safe_code == 0)
    {
        max_volt = 242;
        min_volt = 198;
    }
    else if (safe_code == 1)
    {
        max_volt = 253;
        min_volt = 195;
    }
    else if (safe_code == 2)
    {
        max_volt = 285;
        min_volt = 160;
    }
    else
    {
        return;
    }
    max_volt *= 10;
    min_volt *= 10;

    uint8_t id;
    uint8_t fc;
    uint16_t addr;
    uint16_t crc;
    uint8_t msg[20] = {0};
    int msg_len;

    /** set max volt:*/
    id = 0x03;
    fc = 0x06; //0x06-single 0x10-multiple
    addr = 45212 - 40001;
    var = max_volt;

    memset(msg, 0, sizeof(msg));
    msg[0] = id;
    msg[1] = fc;
    msg[2] = (addr >> 8) & 0xFF;
    msg[3] = (addr >> 0) & 0xFF;
    msg[4] = (var >> 8) & 0xFF;
    msg[5] = (var >> 0) & 0xFF;
    crc = crc16_calc(msg, 6);
    msg[6] = (crc >> 0) & 0xFF;
    msg[7] = (crc >> 8) & 0xFF;
    msg_len = 8;

    tianhe_fdbg_send(msg, msg_len);
    memset(msg, 0, sizeof(msg));
    tianhe_fdbg_recv(msg, &msg_len);

    /** set min volt:*/
    id = 0x03;
    fc = 0x06; //0x06-single 0x10-multiple
    addr = 45221 - 40001;
    var = min_volt;

    memset(msg, 0, sizeof(msg));
    msg[0] = id;
    msg[1] = fc;
    msg[2] = (addr >> 8) & 0xFF;
    msg[3] = (addr >> 0) & 0xFF;
    msg[4] = (var >> 8) & 0xFF;
    msg[5] = (var >> 0) & 0xFF;
    crc = crc16_calc(msg, 6);
    msg[6] = (crc >> 0) & 0xFF;
    msg[7] = (crc >> 8) & 0xFF;
    msg_len = 8;

    tianhe_fdbg_send(msg, msg_len);
    memset(msg, 0, sizeof(msg));
    tianhe_fdbg_recv(msg, &msg_len);
}

void recv_fdbg_handler(uint8_t *buf, int len)
{
    uint8_t msg[300] = {0};
    int msg_len = len - 58 - 2;
    if (msg_len <= 0)
        return;

    memcpy(msg, buf + 58, msg_len);
    tianhe_fdbg_send(msg, msg_len);

    memset(msg, 0, sizeof(msg));
    tianhe_fdbg_recv(msg, &msg_len);

    if (msg_len > 0 && msg_len < 255)
    {
        char frame[1300] = {0};
        int send_len = create_response_frame(buf, len, msg, msg_len, frame);
        cat1_tcp_send(frame, send_len);
        set_print_color(YELLOW);
        printf("response ok: recv_fdbg_handler\n");
        set_print_color(BLUE);
    }
}

void set_pwr_rate_handler(uint8_t *buf, int len)
{

    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, NULL, 0, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("upgrade_hander: response ok\n");
    set_print_color(BLUE);

    //45403:0.01 %
    uint8_t pwr_per = buf[58];
    if (pwr_per < 0 || pwr_per > 100)
        return;
    printf("set_pwr_rate_handler: %d percent\n", pwr_per);

    uint8_t id = 0x03;
    uint8_t fc = 0x06; //single 4x
    uint16_t addr = 45403 - 40001;
    uint16_t var = pwr_per * 100;
    uint16_t crc = 0;

    uint8_t msg[20] = {0};
    int msg_len = 0;
    msg[0] = id;
    msg[1] = fc;
    msg[2] = (addr >> 8) & 0xFF;
    msg[3] = (addr >> 0) & 0xFF;
    msg[4] = (var >> 8) & 0xFF;
    msg[5] = (var >> 0) & 0xFF;
    crc = crc16_calc(msg, 6);
    msg[6] = (crc >> 0) & 0xFF;
    msg[7] = (crc >> 8) & 0xFF;
    msg_len = 8;

    tianhe_fdbg_send(msg, msg_len);
    memset(msg, 0, sizeof(msg));
    tianhe_fdbg_recv(msg, &msg_len);
}

extern char g_inv_ma_ver[20];
extern char g_inv_sf_ver[20];
void get_inv_sw_ver_handler(uint8_t *buf, int len)
{
    uint8_t data[12] = {0};
    data[0] = 0x02; //ver num

    // master
    data[1] = 0x01; //ver type
    memcpy(&data[2], g_inv_ma_ver, 10);
    //len=12

    // safety
    data[12] = 0x11; //ver type
    memcpy(&data[13], g_inv_sf_ver, 10);

    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, data, 23, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("response ok: get_inv_sw_ver_handler\n");
    set_print_color(BLUE);
}

//45212:U16 0.1   --- max
//45221:U16 0.1   --- min
void get_safe_volt_handler(uint8_t *buf, int len)
{
    uint16_t max_volt = 0;
    uint16_t min_volt = 0;

    uint8_t msg[50];
    int msg_len;
    uint8_t id;
    uint8_t fc;
    uint16_t addr;
    uint16_t crc;

    /** read max volt from inverter*/
    memset(msg, 0, sizeof(msg));
    id = 0x03;
    fc = 0x03; //4x-0x03, 3x-0x04
    addr = 45212 - 40001;
    msg[0] = id;
    msg[1] = fc;
    msg[2] = (addr >> 8) & 0xFF;
    msg[3] = (addr >> 0) & 0xFF;
    msg[4] = 0;
    msg[5] = 10;
    crc = crc16_calc(msg, 6);
    msg[6] = (crc >> 0) & 0xFF;
    msg[7] = (crc >> 8) & 0xFF;
    msg_len = 8;
    tianhe_fdbg_send(msg, msg_len);
    memset(msg, 0, sizeof(msg));
    tianhe_fdbg_recv(msg, &msg_len);

    if (msg[0] == 03 && msg[1] == 03 && crc16_calc(msg, msg_len) == 0)
    {
        max_volt = ((msg[3] << 8) + msg[4]) / 10;
        min_volt = ((msg[21] << 8) + msg[22]) / 10;
    }
    else
    {
        hex_print_frame(msg, msg_len);
        printf("safe volt get: crc error --1\n");
        return;
    }

    /** response tinahe server*/
    uint8_t data[4] = {0};
    // max_volt = 2900;
    // min_volt = 1500;
    data[0] = (max_volt >> 8) & 0xFF;
    data[1] = max_volt & 0xFF;
    data[2] = (min_volt >> 8) & 0xFF;
    data[3] = min_volt & 0xFF;

    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, data, 4, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("response ok: get_safe_volt_handler\n");
    set_print_color(BLUE);
}

extern int g_lac_len;
extern int g_ci_len;
void get_sim_info_handler(uint8_t *buf, int len)
{
    uint8_t data[12] = {0};
    int i = 0;

    /** 4字节右对齐*/
    for (i = 0; i < g_lac_len; i++)
    {
        if (3 - i >= 0 && g_lac_len - 1 - i >= 0)
            data[3 - i] = g_lac[g_lac_len - 1 - i];
    }

    for (i = 0; i < g_ci_len; i++)
    {
        if (7 - i >= 0 && g_ci_len - 1 - i >= 0)
            data[7 - i] = g_ci[g_ci_len - 1 - i];
    }

    uint32_t csq = abs(cat1_get_csq());
    data[8] = (csq >> 24) & 0xFF;
    data[9] = (csq >> 16) & 0xFF;
    data[10] = (csq >> 8) & 0xFF;
    data[11] = (csq >> 0) & 0xFF;

    printf("\nsim imfo: lac:\n");
    hex_print_frame(&data[0], 4);
    printf("\nsim imfo: ci:\n");
    hex_print_frame(&data[4], 4);
    printf("\nsim imfo: csq:\n");
    hex_print_frame(&data[8], 4);

    char frame[1300] = {0};
    int send_len = create_response_frame(buf, len, data, 12, frame);
    cat1_tcp_send(frame, send_len);
    set_print_color(YELLOW);
    printf("response ok: get_sim_info_handler\n");
    set_print_color(BLUE);
}

recv_cmd_tab_t recv_cmd_tab[] =
    {
        {CMD_SET_SERVER, set_server_handler},
        {CMD_UPGRADE, upgrade_handler},
        {CMD_OFF, off_handler}, //40201:0
        {CMD_ON, on_handler},   //40201:1
        {CMD_PMU_RESTART, pmu_restart_handler},
        {CMD_DEV_INFO, dev_info_handler},
        {CMD_SET_SAFE_VOLT, set_safe_volt_handler}, //fdbg--
        {CMD_RECV_FDBG, recv_fdbg_handler},         //fdbg
        {CMD_SET_PWR_RATE, set_pwr_rate_handler},   //45403:0.01 %
        {CMD_GET_SAFE_VOLT, get_safe_volt_handler}, //fdbg--
        {CMD_GET_INV_SW_VER, get_inv_sw_ver_handler},
        {CMD_GET_SIM_INFO, get_sim_info_handler},
        {CMD_RSD_ENABLE, NULL},
        {CMD_COMPONENT_RESTART, NULL},
        {CMD_COMPONENT_UPGRAGE, NULL},
        {CMD_GET_ECU_INV_CMPNT_MAP, NULL}};

uint16_t send_cmd_tab[] = {CMD_LOGIN, CMD_KEEP_ALIVE, CMD_GET_TIME, CMD_SEND_FDBG, CMD_SEND_INV_DATA, CMD_SEND_HIST_INV_DATA, CMD_SEND_COMPONENT_DATA, CMD_SEND_HIST_COMPONENT_DATA};

int is_send_cmd(uint16_t cmd)
{
    int max = sizeof(send_cmd_tab) / sizeof(uint16_t);
    for (int j = 0; j < max; j++)
    {
        if (cmd == send_cmd_tab[j])
        {
            return 1;
        }
    }
    return 0;
}

int get_recv_cmd_tab_num(void)
{
    return sizeof(recv_cmd_tab) / sizeof(recv_cmd_tab_t);
}

int find_recv_index(uint16_t cmd)
{
    int num = get_recv_cmd_tab_num();
    int i = 0;
    for (i = 0; i < num; i++)
    {
        if (recv_cmd_tab[i].cmd == cmd)
            return i;
    }
    return -1;
}

void recv_hook_by_index(int index, uint8_t *buf, int len)
{
    if (recv_cmd_tab[index].hook != NULL)
        recv_cmd_tab[index].hook(buf, len);
}

#include <string.h>
int tianhe_tcp_filter(char *msg, int len, char *frame, int *frame_len)
{
    /** process tianhe command*/
    char head[52] = {0};
    uint8_t *p1 = msg;
    uint8_t *p0 = NULL;
    int head_len = tianhe_get_filter_head(head);

    int num = get_recv_cmd_tab_num();
    int i = 0;
    int res = -1;
    printf("KKKK------------------------------7\n");

    printf("<XXX FRAME==============>\n");
    for (i = 0; i < len; i++)
        printf("%02X ", (uint8_t)p1[i]);
    printf("</XXX FRAME==============>\n");

    while (1)
    {
        int left_len = (uint8_t *)msg + len - p1;
        printf("KKKK------------------------------8\n");
        p0 = memmem(p1, (size_t)left_len, head, (size_t)head_len);
        p1 = p0;
        printf("left_len, head_len:%d, %d\n", left_len, head_len);
        printf("KKKK------------------------------9\n");
        if (p1 != NULL)
        {
            printf("KKKK------------------------------9-1 %d %d\n", (int)*(p1 + 52), (int)*(p1 + 53));
            uint16_t cmd = 0;
            // cmd = (p1[52] << 8) | (p1[53]);
            cmd = ((*(p1 + 52)) << 8) | (*(p1 + 53));

            printf("KKKK------------------------------9-2\n");
            uint8_t *data = p1 + 58;
            uint8_t *total_data = p1;

            int data_len = (*(p1 + 56) << 8) + *(p1 + 57);
            int total_len = data_len + 60;
            printf("data_len: %d\n", data_len);
            int this_len = 58 + data_len + 2; /** frame*/

            printf("KKKK------------------------------10\n");
            uint16_t crc = crc16_calc(p1, this_len - 2);

            if (crc != (*(p1 + this_len - 2) << 8) + *(p1 + this_len - 1))
            {
                goto NEXT;
            }
            printf("KKKK------------------------------11\n");
            int index = find_recv_index(cmd);
            if (index >= 0)
            {
                printf("KKKK------------------------------12\n");
                recv_hook_by_index(index, total_data, total_len);
                printf("KKKK------------------------------13\n");
                goto NEXT;
            }
            else
            {
                if (is_send_cmd(cmd) == 0)
                {
                    // printf("found server resp\n");
                    // printf("KKKK------------------------------14\n");
                    memcpy(frame, data, data_len); /** 拷贝服务端的响应数据，不含外围格式 */
                    // printf("KKKK------------------------------15\n");
                    *frame_len = data_len;
                    res = 0;
                }
                // else
                // {
                //     printf("found cmd, but not resp\n");
                // }
                goto NEXT;
            }
        }
        else
        {
            return res;
        }
    NEXT:
        p1 = p1 + head_len;
        if (p1 < msg + len)
        {
            continue;
        }
        else
        {
            return res;
        }
    }
}

/** demo test*/
extern char g_pmu_sn[25];
extern char g_inv_sn[25];
void tianhe_ini(void)
{
    char *inv_sn = "SHIZHEN123456INV";
    char *pmu_sn = "B9101234567K";

    base_para_t base_para = {0};

    memcpy(base_para.inv_sn, g_inv_sn, strlen(g_inv_sn));
    memcpy(base_para.pmu_sn, g_pmu_sn, strlen(pmu_sn));

    base_para_ini(base_para);

    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);

    if (strlen(setting.host) > 0)
    // if (0) /** fortest*/
    {
        memset(g_tianhe_host, 0, sizeof(g_tianhe_host));
        memcpy(g_tianhe_host, setting.host, strlen(setting.host));
        g_tianhe_port = setting.port;
    }
    else
    {
        /** standard server*/
        char *host = "psinf.trinasolar.com";
        int port = 3002;

        memset(g_tianhe_host, 0, strlen(g_tianhe_host));
        memcpy(g_tianhe_host, host, strlen(host));
        g_tianhe_port = port;
        /** test server */
        // char *host = "47.102.127.54";
        // int port = 8080;

        // memset(g_tianhe_host, 0, strlen(g_tianhe_host));
        // memcpy(g_tianhe_host, host, strlen(host));
        // g_tianhe_port = port;
    }
}
