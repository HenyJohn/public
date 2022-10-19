#include "time.h"
#include "string.h"
#include "data_process.h"

#define POST_DATA_HEADER POST_DATA_HEADER_3PAHSE
//17 seg
#define POST_DATA_HEADER_3PAHSE                                                                                                                                      \
    "datalogger,%s\r\n"                                                                                                                                              \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                    \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                  \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                     \
    "unit,W,,Hz,V,V,V,A,A,A,V,V,A,A,kWh,kWh,,°C\r\n"                                                                                                                \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC1,U_AC2,U_AC3,I_AC1,I_AC2,I_AC3,U_DC1,U_DC2,I_DC1,I_DC2,E_DAY,E_TOTAL,ERROR,T\r\n"

#define POST_DATA_HEADER_3PAHSE_DC1                                                                                                                \
    "datalogger,%s\r\n"                                                                                                                            \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                        \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                      \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                         \
    "unit,W,,Hz,V,V,V,A,A,A,V,A,kWh,kWh,,°C\r\n"                                                                                                  \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC1,U_AC2,U_AC3,I_AC1,I_AC2,I_AC3,U_DC1,I_DC1,E_DAY,E_TOTAL,ERROR,T\r\n"

#define POST_DATA_HEADER_3PAHSE_DC2                                                                                                                \
    "datalogger,%s\r\n"                                                                                                                            \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                        \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                      \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                         \
    "unit,W,,Hz,V,V,V,A,A,A,V,A,kWh,kWh,,°C\r\n"                                                                                                  \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC1,U_AC2,U_AC3,I_AC1,I_AC2,I_AC3,U_DC2,I_DC2,E_DAY,E_TOTAL,ERROR,T\r\n"

//13 seg
#define POST_DATA_HEADER_1PAHSE                                                                                                  \
    "datalogger,%s\r\n"                                                                                                          \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                            \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                          \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                             \
    "unit,W,,Hz,V,A,V,V,A,A,kWh,kWh,,°C\r\n"                                                                                    \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC,I_AC,U_DC1,U_DC2,I_DC1,I_DC2,E_DAY,E_TOTAL,ERROR,T\r\n"
//11 seg
#define POST_DATA_HEADER_1PAHSE_DC1                                                                            \
    "datalogger,%s\r\n"                                                                                        \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                              \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                 \
    "unit,W,,Hz,V,A,V,A,kWh,kWh,,°C\r\n"                                                                      \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC,I_AC,U_DC1,I_DC1,E_DAY,E_TOTAL,ERROR,T\r\n"
//11 seg
#define POST_DATA_HEADER_1PAHSE_DC2                                                                            \
    "datalogger,%s\r\n"                                                                                        \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                              \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                 \
    "unit,W,,Hz,V,A,V,A,kWh,kWh,,°C\r\n"                                                                      \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC,I_AC,U_DC2,I_DC2,E_DAY,E_TOTAL,ERROR,T\r\n"

//comp meter
#define POST_DATA_HEADER_3PAHSE_MET                                                                                                                                                                      \
    "datalogger,%s\r\n"                                                                                                                                                                                  \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                                      \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                                    \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                                       \
    "unit,W,,Hz,V,V,V,A,A,A,V,V,A,A,kWh,kWh,,°C,kWh,kWh,Hz,W,VAr,VA\r\n"                                                                                                                                \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC1,U_AC2,U_AC3,I_AC1,I_AC2,I_AC3,U_DC1,U_DC2,I_DC1,I_DC2,E_DAY,E_TOTAL,ERROR,T,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"

#define POST_DATA_HEADER_3PAHSE_DC1_MET                                                                                                                                                \
    "datalogger,%s\r\n"                                                                                                                                                                \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                          \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                        \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                           \
    "unit,W,,Hz,V,V,V,A,A,A,V,A,kWh,kWh,,°C,kWh,kWh,Hz,W,VAr,VA\r\n"                                                                                                                  \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC1,U_AC2,U_AC3,I_AC1,I_AC2,I_AC3,U_DC1,I_DC1,E_DAY,E_TOTAL,ERROR,T,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"

#define POST_DATA_HEADER_3PAHSE_DC2_MET                                                                                                                                                \
    "datalogger,%s\r\n"                                                                                                                                                                \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                          \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                        \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                                           \
    "unit,W,,Hz,V,V,V,A,A,A,V,A,kWh,kWh,,°C,kWh,kWh,Hz,W,VAr,VA\r\n"                                                                                                                  \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC1,U_AC2,U_AC3,I_AC1,I_AC2,I_AC3,U_DC2,I_DC2,E_DAY,E_TOTAL,ERROR,T,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"

//13 seg
#define POST_DATA_HEADER_1PAHSE_MET                                                                                                                                  \
    "datalogger,%s\r\n"                                                                                                                                              \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                              \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                            \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                               \
    "unit,W,,Hz,V,A,V,V,A,A,kWh,kWh,,°C,kWh,kWh,Hz,W,VAr,VA\r\n"                                                                                                    \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC,I_AC,U_DC1,U_DC2,I_DC1,I_DC2,E_DAY,E_TOTAL,ERROR,T,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"
//11 seg
#define POST_DATA_HEADER_1PAHSE_DC1_MET                                                                                                            \
    "datalogger,%s\r\n"                                                                                                                            \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                  \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                   \
    "unit,W,,Hz,V,A,V,A,kWh,kWh,,°C,kWh,kWh,Hz,W,VAr,VA\r\n"                                                                                      \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC,I_AC,U_DC1,I_DC1,E_DAY,E_TOTAL,ERROR,T,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"
//11 seg
#define POST_DATA_HEADER_1PAHSE_DC2_MET                                                                                                            \
    "datalogger,%s\r\n"                                                                                                                            \
    "type,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,inverter,meteo,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                  \
    "serial,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                \
    "uid,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n"                                                                                   \
    "unit,W,,Hz,V,A,V,A,kWh,kWh,,°C,kWh,kWh,Hz,W,VAr,VA\r\n"                                                                                      \
    "abbreviation,P_AC,COS_PHI,F_AC,U_AC,I_AC,U_DC2,I_DC2,E_DAY,E_TOTAL,ERROR,T,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"
//comp meter
#define POST_DATA_HEADER_METER                     \
    "datalogger,%s\r\n"                            \
    "type,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s\r\n"                   \
    "serial,%s,%s,%s,%s,%s,%s\r\n"                 \
    "uid,%s,%s,%s,%s,%s,%s\r\n"                    \
    "unit,kWh,kWh,Hz,W,VAr,VA\r\n"                 \
    "abbreviation,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"

#define POST_DATA_HEADER_INV_METER                 \
    "\r\ndatalogger,%s\r\n"                        \
    "type,meter,meter,meter,meter,meter,meter\r\n" \
    "name,%s,%s,%s,%s,%s,%s\r\n"                   \
    "serial,%s,%s,%s,%s,%s,%s\r\n"                 \
    "uid,%s,%s,%s,%s,%s,%s\r\n"                    \
    "unit,kWh,kWh,Hz,W,VAr,VA\r\n"                 \
    "abbreviation,M_AC_E_EXP,M_AC_E_IMP,M_AC_F,M_AC_P,M_AC_Q,M_AC_S\r\n"

#define POST_DATA_CONTENT "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC1 "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC2 "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"

#define POST_DATA_CONTENT_MET "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC1_MET "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC2_MET "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"

//ADD BY WANGDAN
#define POST_DATA_CONTENT_3PHASE "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC1_3PHASE "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC2_3PHASE "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"

#define POST_DATA_CONTENT_MET_3PHASE "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC1_MET_3PHASE "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
#define POST_DATA_CONTDC2_MET_3PHASE "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"

#define POST_REQ_HEADER                 \
    "POST /v2 HTTP/1.1\r\n"             \
    "Host: mii-csv.meteocontrol.de\r\n" \
    "content-type: text/csv\r\n"        \
    "API-Key: isejgjsnl4j05bc\r\n"      \
    "Content-Length: %d\r\n"            \
    "Connection: close\r\n"             \
    "\r\n"

/*
 * format string
 */
char *formatstr(int x, char p, char *dest)
{
    char strx[255] = {0};
    char str1[32] = {0};
    char str2[32] = {0};

    char *res = NULL;
    sprintf(strx, "%d", x);

    if (!p)
    {
        res = strx;
        memcpy(dest, strx, strlen(strx));
        return res;
    }
    if (strlen(strx) > p)
    {
        memcpy(str1, strx + (strlen(strx) - p), p);
        memcpy(str2, strx, strlen(strx) - p);
        sprintf(strx, "%s.%s", str2, str1);
        memcpy(dest, strx, strlen(strx));
        res = strx;
        return res;
    }
    else
    {
        char tmp[255] = {0};
        if (strlen(strx) == p)
            sprintf(tmp, "0%s", strx);

        if (p - strlen(strx) == 1)
            sprintf(tmp, "00%s", strx);

        if (p - strlen(strx) == 2)
            sprintf(tmp, "000%s", strx);

        if (p - strlen(strx) == 3)
            sprintf(tmp, "0000%s", strx);

        if (p - strlen(strx) == 4)
            sprintf(tmp, "00000%s", strx);

        if (p - strlen(strx) == 5)
            sprintf(tmp, "00000%s", strx);

        memset(strx, 0, sizeof(strx));
        memcpy(strx, tmp, strlen(tmp));
        memcpy(str1, strx + (strlen(strx) - p), p);
        memcpy(str2, strx, strlen(strx) - p);
        sprintf(strx, "%s.%s", str2, str1);
        memcpy(dest, strx, strlen(strx));
        res = strx;
        return res;
    }
}

/*
 * format inv data inv_data_ptr->PV_cur_voltg[i].iVol
 */
int format_data_muilts_old(char *buf, Inv_data *inv, Inv_data met, int meter_flg, char num)
{
//printf("PVk %d %d %s\n", inv[0].PV_cur_voltg[0].iVol, inv[0].PV_cur_voltg[1].iVol, inv[0].psn);
#if 1
    //printf("psnname,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n",
    //inv[0].psn, inv[0].psn, inv[0].psn,inv[0].psn, inv[0].psn, inv[0].psn,inv[0].psn, inv[0].psn, inv[0].psn,inv[0].psn, inv[0].psn, inv[0].psn,inv[0].psn);
    char data_type = 0;
    if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol > 300)
    {
        data_type = 1;
        sprintf(buf, POST_DATA_HEADER_1PAHSE, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
    }

    if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol < 300)
    {
        data_type = 2;
        sprintf(buf, POST_DATA_HEADER_1PAHSE_DC1, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
    }

    if (inv[0].PV_cur_voltg[0].iVol < 300 && inv[0].PV_cur_voltg[1].iVol > 300)
    {
        data_type = 3;
        sprintf(buf, POST_DATA_HEADER_1PAHSE_DC2, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
    }

    //printf("post_header %s \n", buf);
    char tmp[4096] = {0};
    char time[128] = {0};
    char tmp_data[4096] = {0};
    struct tm curr_date = {0};
    char i = 0;

    for (; i < num; i++)
    {
        get_data_time(inv[i].time, &curr_date);
        //printf("%d%d%d%d%d%d\n", curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

        if (curr_date.tm_min % 10 > 0 && curr_date.tm_min % 10 < 5) //1234 5 67890
        {
            char tmp_m = curr_date.tm_min / 10;
            curr_date.tm_min = tmp_m * 10 + 0;
        }
        else if (curr_date.tm_min % 10 > 5 && curr_date.tm_min % 10 < 10)
        {
            char tmp_m = curr_date.tm_min / 10;
            curr_date.tm_min = tmp_m * 10 + 5;
        }

        short datlen = sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d",
                               curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

        char des[18][16] = {0};
        formatstr(inv[i].pac, 0, des[0]);
        formatstr(inv[i].cosphi, 2, des[1]);
        formatstr(inv[i].fac, 2, des[2]);

        formatstr(inv[i].AC_vol_cur[0].iVol, 1, des[3]);
        formatstr(inv[i].AC_vol_cur[0].iCur, 1, des[4]);

        formatstr(inv[i].PV_cur_voltg[0].iVol, 1, des[5]);
        formatstr(inv[i].PV_cur_voltg[1].iVol, 1, des[6]);

        formatstr(inv[i].PV_cur_voltg[0].iCur, 2, des[7]);
        formatstr(inv[i].PV_cur_voltg[1].iCur, 2, des[8]);

        formatstr(inv[i].e_today, 1, des[9]);
        formatstr(inv[i].e_total, 1, des[10]);
        formatstr(inv[i].error, 0, des[11]);
        formatstr(inv[i].col_temp, 1, des[12]);

        if (data_type == 1) //POST_DATA_HEADER_1PAHSE
            sprintf(tmp, POST_DATA_CONTENT, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12]);
        if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
            sprintf(tmp, POST_DATA_CONTDC1, time, des[0], des[1], des[2], des[3], des[4], des[5], des[7], des[9], des[10], des[11], des[12]);
        if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
            sprintf(tmp, POST_DATA_CONTDC2, time, des[0], des[1], des[2], des[3], des[4], des[6], des[8], des[9], des[10], des[11], des[12]);

        //printf("tmp %s %d \n", tmp, strlen(tmp));
        printf("tmp %s \n", tmp);
        if (strlen(tmp) > 0)
        {
            if (i + 1 < num)
            {
                strcat(tmp, "\r\n");
                strcat(tmp_data, tmp);
            }
            else
                strcat(tmp_data, tmp);
        }
        memset(tmp, 0, sizeof(tmp));
    }

    strcat(buf, tmp_data);
// memset(tmp, 0, sizeof(tmp));

// memset(tmp, 0, sizeof(tmp));
// sprintf(tmp, POST_REQ_HEADER, strlen(buf));
// strcat(tmp, buf);
// memset(buf, 0, sizeof(buf));
// memcpy(buf, tmp, strlen(tmp));
#endif
}

/*
 * format inv data inv_data_ptr->PV_cur_voltg[i].iVol
 */
int format_data_muilts(char *buf, Inv_data *inv, Inv_data met, int meter_flg, char num)
{
    if (get_inv_phase_type() == 0x31)
    {
        if (meter_flg == 0)
        {
            char data_type = 0;
            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 1;
                sprintf(buf, POST_DATA_HEADER_1PAHSE, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
            }

            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol < 300)
            {
                data_type = 2;
                sprintf(buf, POST_DATA_HEADER_1PAHSE_DC1, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
            }

            if (inv[0].PV_cur_voltg[0].iVol < 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 3;
                sprintf(buf, POST_DATA_HEADER_1PAHSE_DC2, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
            }

            //printf("post_header %s \n", buf);
            char tmp[4096] = {0};
            char time[128] = {0};
            char tmp_data[4096] = {0};
            struct tm curr_date = {0};
            char i = 0;

            for (; i < num; i++)
            {
                get_data_time(inv[i].time, &curr_date);
                //printf("%d%d%d%d%d%d\n", curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

                if (curr_date.tm_min % 10 > 0 && curr_date.tm_min % 10 < 5) //1234 5 67890
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 0;
                }
                else if (curr_date.tm_min % 10 > 5 && curr_date.tm_min % 10 < 10)
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 5;
                }

                short datlen = sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d",
                                       curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

                char des[18][16] = {0};
                formatstr(inv[i].pac, 0, des[0]);
                formatstr(inv[i].cosphi, 2, des[1]);
                formatstr(inv[i].fac, 2, des[2]);

                formatstr(inv[i].AC_vol_cur[0].iVol, 1, des[3]);
                formatstr(inv[i].AC_vol_cur[0].iCur, 1, des[4]);

                formatstr(inv[i].PV_cur_voltg[0].iVol, 1, des[5]);
                formatstr(inv[i].PV_cur_voltg[1].iVol, 1, des[6]);

                formatstr(inv[i].PV_cur_voltg[0].iCur, 2, des[7]);
                formatstr(inv[i].PV_cur_voltg[1].iCur, 2, des[8]);

                formatstr(inv[i].e_today, 1, des[9]);
                formatstr(inv[i].e_total, 1, des[10]);
                formatstr(inv[i].error, 0, des[11]);
                formatstr(inv[i].col_temp, 1, des[12]);

                if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                    sprintf(tmp, POST_DATA_CONTENT, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12]);
                if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                    sprintf(tmp, POST_DATA_CONTDC1, time, des[0], des[1], des[2], des[3], des[4], des[5], des[7], des[9], des[10], des[11], des[12]);
                if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                    sprintf(tmp, POST_DATA_CONTDC2, time, des[0], des[1], des[2], des[3], des[4], des[6], des[8], des[9], des[10], des[11], des[12]);

                //printf("tmp %s %d \n", tmp, strlen(tmp));
                printf("tmp %s \n", tmp);
                if (strlen(tmp) > 0)
                {
                    if (i + 1 < num)
                    {
                        strcat(tmp, "\r\n");
                        strcat(tmp_data, tmp);
                    }
                    else
                        strcat(tmp_data, tmp);
                }

                memset(tmp, 0, sizeof(tmp));
            }

            strcat(buf, tmp_data);
        }
        else
        {
            char metermodesn[7][10] = {
                "SDM630CT",
                "SDM630DC",
                "SDM230",
                "SDM220",
                "SDM120",
                "NORK"};
            char data_type = 0;
            char *meter_mode_sn = "UNKNOWN";
            int meter_mode_index = get_meter_index();
            if (meter_mode_index >= 0 && meter_mode_index <= 5)
                meter_mode_sn = metermodesn[meter_mode_index];

            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 1;
                sprintf(buf, POST_DATA_HEADER_1PAHSE_MET, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn);
            }

            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol < 300)
            {
                data_type = 2;
                sprintf(buf, POST_DATA_HEADER_1PAHSE_DC1_MET, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn);
            }

            if (inv[0].PV_cur_voltg[0].iVol < 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 3;
                sprintf(buf, POST_DATA_HEADER_1PAHSE_DC2_MET, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn);
            }

            printf("post_header %s \n", buf);
            char tmp[4096] = {0}; //4096
            char time[128] = {0};
            char tmp_data[4096] = {0}; //4096
            struct tm curr_date = {0};
            char i = 0;

            for (; i < num; i++)
            {
                get_data_time(inv[i].time, &curr_date);
                if (curr_date.tm_min % 10 > 0 && curr_date.tm_min % 10 < 5) //1234 5 67890
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 0;
                }
                else if (curr_date.tm_min % 10 > 5 && curr_date.tm_min % 10 < 10)
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 5;
                }

                short datlen = sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d",
                                       curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

                char des[18][16] = {0};
                formatstr(inv[i].pac, 0, des[0]);
                formatstr(inv[i].cosphi, 2, des[1]);
                formatstr(inv[i].fac, 2, des[2]);

                formatstr(inv[i].AC_vol_cur[0].iVol, 1, des[3]);
                formatstr(inv[i].AC_vol_cur[0].iCur, 1, des[4]);

                formatstr(inv[i].PV_cur_voltg[0].iVol, 1, des[5]);
                formatstr(inv[i].PV_cur_voltg[1].iVol, 1, des[6]);

                formatstr(inv[i].PV_cur_voltg[0].iCur, 2, des[7]);
                formatstr(inv[i].PV_cur_voltg[1].iCur, 2, des[8]);

                formatstr(inv[i].e_today, 1, des[9]);
                formatstr(inv[i].e_total, 1, des[10]);
                formatstr(inv[i].error, 0, des[11]);
                formatstr(inv[i].col_temp, 1, des[12]);

                if (0 == strncmp(inv[i + 1].psn, "SDM", 3) || 0 == strncmp(met.psn, "SDM", 3))
                {
                    if (0 == strncmp(inv[i + 1].psn, "SDM", 3))
                    {
                        char mtd[6][16] = {0};
                        formatstr(inv[i + 1].h_total, 1, mtd[0]);
                        formatstr(inv[i + 1].e_total, 1, mtd[1]);
                        formatstr(inv[i + 1].fac, 2, mtd[2]);
                        formatstr(inv[i + 1].pac, 0, mtd[3]);
                        formatstr(inv[i + 1].qac, 0, mtd[4]);
                        formatstr(inv[i + 1].sac, 0, mtd[5]);

                        if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                            sprintf(tmp, POST_DATA_CONTENT_MET, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                            sprintf(tmp, POST_DATA_CONTDC1_MET, time, des[0], des[1], des[2], des[3], des[4], des[5], des[7], des[9], des[10], des[11], des[12], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                            sprintf(tmp, POST_DATA_CONTDC2_MET, time, des[0], des[1], des[2], des[3], des[4], des[6], des[8], des[9], des[10], des[11], des[12], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        i++;
                    }
                    if (i == num - 1 && 0 == strncmp(met.psn, "SDM", 3))
                    {
                        char mtd[6][16] = {0};
                        formatstr(met.h_total, 1, mtd[0]);
                        formatstr(met.e_total, 1, mtd[1]);
                        formatstr(met.fac, 2, mtd[2]);
                        formatstr(met.pac, 0, mtd[3]);
                        formatstr(met.qac, 0, mtd[4]);
                        formatstr(met.sac, 0, mtd[5]);

                        if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                            sprintf(tmp, POST_DATA_CONTENT_MET, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                            sprintf(tmp, POST_DATA_CONTDC1_MET, time, des[0], des[1], des[2], des[3], des[4], des[5], des[7], des[9], des[10], des[11], des[12], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                            sprintf(tmp, POST_DATA_CONTDC2_MET, time, des[0], des[1], des[2], des[3], des[4], des[6], des[8], des[9], des[10], des[11], des[12], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        memset(&met.psn, 0, 10);
                    }
                }
                else
                {
                    char emt[6][16] = {0};
                    if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                        sprintf(tmp, POST_DATA_CONTENT_MET, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12], emt[0], emt[1], emt[2], emt[3], emt[4], emt[5]);
                    if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                        sprintf(tmp, POST_DATA_CONTDC1_MET, time, des[0], des[1], des[2], des[3], des[4], des[5], des[7], des[9], des[10], des[11], des[12], emt[0], emt[1], emt[2], emt[3], emt[4], emt[5]);
                    if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                        sprintf(tmp, POST_DATA_CONTDC2_MET, time, des[0], des[1], des[2], des[3], des[4], des[6], des[8], des[9], des[10], des[11], des[12], emt[0], emt[1], emt[2], emt[3], emt[4], emt[5]);
                }
                printf("tmp %s %d \n", tmp, strlen(tmp));
                if (strlen(tmp) > 0)
                {
                    if (i + 1 < num)
                    {
                        strcat(tmp, "\r\n");
                        strcat(tmp_data, tmp);
                    }
                    else
                        strcat(tmp_data, tmp);
                }
                memset(tmp, 0, sizeof(tmp));
            }

            strcat(buf, tmp_data);
            // memset(tmp, 0, sizeof(tmp));

            // memset(tmp, 0, sizeof(tmp));
            // sprintf(tmp, POST_REQ_HEADER, strlen(buf));
            // strcat(tmp, buf);
            // memset(buf, 0, sizeof(buf));
            // memcpy(buf, tmp, strlen(tmp));
        }
    }
    else if (get_inv_phase_type() == 0x33)
    {
        for (int z = 0; z < num; z++)
        {
            printf("<psn>idx %d psn %s meter_flg %d v1 %d v2 %d</psn>\n", z, inv[z].psn, meter_flg, inv[z].PV_cur_voltg[0].iVol, inv[z].PV_cur_voltg[1].iVol);
        }

        if (meter_flg == 0)
        {
            char data_type = 0;
            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 1;
                sprintf(buf, POST_DATA_HEADER_3PAHSE, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
            }

            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol < 300)
            {
                data_type = 2;
                sprintf(buf, POST_DATA_HEADER_3PAHSE_DC1, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
            }

            if (inv[0].PV_cur_voltg[0].iVol < 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 3;
                sprintf(buf, POST_DATA_HEADER_3PAHSE_DC2, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn);
            }

            //printf("post_header %s \n", buf);
            char tmp[4096] = {0};
            char time[128] = {0};
            char tmp_data[4096] = {0};
            struct tm curr_date = {0};
            char i = 0;

            for (; i < num; i++)
            {
                get_data_time(inv[i].time, &curr_date);
                //printf("%d%d%d%d%d%d\n", curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

                if (curr_date.tm_min % 10 > 0 && curr_date.tm_min % 10 < 5) //1234 5 67890
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 0;
                }
                else if (curr_date.tm_min % 10 > 5 && curr_date.tm_min % 10 < 10)
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 5;
                }

                short datlen = sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d",
                                       curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

                char des[18][16] = {0};
                formatstr(inv[i].pac, 0, des[0]);
                formatstr(inv[i].cosphi, 2, des[1]);
                formatstr(inv[i].fac, 2, des[2]);

                formatstr(inv[i].AC_vol_cur[0].iVol, 1, des[3]);
                formatstr(inv[i].AC_vol_cur[1].iVol, 1, des[4]);
                formatstr(inv[i].AC_vol_cur[2].iVol, 1, des[5]);
                formatstr(inv[i].AC_vol_cur[0].iCur, 1, des[6]);
                formatstr(inv[i].AC_vol_cur[1].iCur, 1, des[7]);
                formatstr(inv[i].AC_vol_cur[2].iCur, 1, des[8]);

                formatstr(inv[i].PV_cur_voltg[0].iVol, 1, des[9]);
                formatstr(inv[i].PV_cur_voltg[1].iVol, 1, des[10]);

                formatstr(inv[i].PV_cur_voltg[0].iCur, 2, des[11]);
                formatstr(inv[i].PV_cur_voltg[1].iCur, 2, des[12]);

                formatstr(inv[i].e_today, 1, des[13]);
                formatstr(inv[i].e_total, 1, des[14]);
                formatstr(inv[i].error, 0, des[15]);
                formatstr(inv[i].col_temp, 1, des[16]);

                if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                    sprintf(tmp, POST_DATA_CONTENT_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12], des[13], des[14], des[15], des[16]);
                if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                    sprintf(tmp, POST_DATA_CONTDC1_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[11], des[13], des[14], des[15], des[16]);
                if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                    sprintf(tmp, POST_DATA_CONTDC2_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[10], des[12], des[13], des[14], des[15], des[16]);

                //printf("tmp %s %d \n", tmp, strlen(tmp));
                printf("tmp %s \n", tmp);
                if (strlen(tmp) > 0)
                {
                    if (i + 1 < num)
                    {
                        strcat(tmp, "\r\n");
                        strcat(tmp_data, tmp);
                    }
                    else
                        strcat(tmp_data, tmp);
                }
                memset(tmp, 0, sizeof(tmp));
            }

            strcat(buf, tmp_data);
        }
        else
        {
            char metermodesn[7][10] = {
                "SDM630CT",
                "SDM630DC",
                "SDM230",
                "SDM220",
                "SDM120",
                "NORK"};
            char data_type = 0;
            char *meter_mode_sn = "UNKNOWN";
            int meter_mode_index = get_meter_index();
            if (meter_mode_index >= 0 && meter_mode_index <= 5)
                meter_mode_sn = metermodesn[meter_mode_index];

            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 1;
                sprintf(buf, POST_DATA_HEADER_3PAHSE_MET, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn);
            }

            if (inv[0].PV_cur_voltg[0].iVol > 300 && inv[0].PV_cur_voltg[1].iVol < 300)
            {
                data_type = 2;
                sprintf(buf, POST_DATA_HEADER_3PAHSE_DC1_MET, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn);
            }

            if (inv[0].PV_cur_voltg[0].iVol < 300 && inv[0].PV_cur_voltg[1].iVol > 300)
            {
                data_type = 3;
                sprintf(buf, POST_DATA_HEADER_3PAHSE_DC2_MET, inv[0].psn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn,
                        inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, inv[0].psn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn, meter_mode_sn);
            }

            printf("post_header %s \n", buf);
            char tmp[4096] = {0}; //4096
            char time[128] = {0};
            char tmp_data[4096] = {0}; //4096
            struct tm curr_date = {0};
            char i = 0;

            for (; i < num; i++)
            {
                get_data_time(inv[i].time, &curr_date);
                if (curr_date.tm_min % 10 > 0 && curr_date.tm_min % 10 < 5) //1234 5 67890
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 0;
                }
                else if (curr_date.tm_min % 10 > 5 && curr_date.tm_min % 10 < 10)
                {
                    char tmp_m = curr_date.tm_min / 10;
                    curr_date.tm_min = tmp_m * 10 + 5;
                }

                short datlen = sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d",
                                       curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

                char des[18][16] = {0};
                formatstr(inv[i].pac, 0, des[0]);
                formatstr(inv[i].cosphi, 2, des[1]);
                formatstr(inv[i].fac, 2, des[2]);

                formatstr(inv[i].AC_vol_cur[0].iVol, 1, des[3]);
                formatstr(inv[i].AC_vol_cur[1].iVol, 1, des[4]);
                formatstr(inv[i].AC_vol_cur[2].iVol, 1, des[5]);
                formatstr(inv[i].AC_vol_cur[0].iCur, 1, des[6]);
                formatstr(inv[i].AC_vol_cur[1].iCur, 1, des[7]);
                formatstr(inv[i].AC_vol_cur[2].iCur, 1, des[8]);

                formatstr(inv[i].PV_cur_voltg[0].iVol, 1, des[9]);
                formatstr(inv[i].PV_cur_voltg[1].iVol, 1, des[10]);

                formatstr(inv[i].PV_cur_voltg[0].iCur, 2, des[11]);
                formatstr(inv[i].PV_cur_voltg[1].iCur, 2, des[12]);

                formatstr(inv[i].e_today, 1, des[13]);
                formatstr(inv[i].e_total, 1, des[14]);
                formatstr(inv[i].error, 0, des[15]);
                formatstr(inv[i].col_temp, 1, des[16]);

                if (0 == strncmp(inv[i + 1].psn, "SDM", 3) || 0 == strncmp(met.psn, "SDM", 3))
                {
                    if (0 == strncmp(inv[i + 1].psn, "SDM", 3))
                    {
                        printf("SDM I+1 %d \n", i);
                        char mtd[6][16] = {0};
                        formatstr(inv[i + 1].h_total, 1, mtd[0]);
                        formatstr(inv[i + 1].e_total, 1, mtd[1]);
                        formatstr(inv[i + 1].fac, 2, mtd[2]);
                        formatstr(inv[i + 1].pac, 0, mtd[3]);
                        formatstr(inv[i + 1].qac, 0, mtd[4]);
                        formatstr(inv[i + 1].sac, 0, mtd[5]);

                        if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                            sprintf(tmp, POST_DATA_CONTENT_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12], des[13], des[14], des[15], des[16], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                            sprintf(tmp, POST_DATA_CONTDC1_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[11], des[13], des[14], des[15], des[16], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                            sprintf(tmp, POST_DATA_CONTDC2_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[10], des[12], des[13], des[14], des[15], des[16], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        i++;
                    }
                    if (i == num - 1 && 0 == strncmp(met.psn, "SDM", 3))
                    {
                        printf("METPSN %d \n", i);
                        char mtd[6][16] = {0};
                        formatstr(met.h_total, 1, mtd[0]);
                        formatstr(met.e_total, 1, mtd[1]);
                        formatstr(met.fac, 2, mtd[2]);
                        formatstr(met.pac, 0, mtd[3]);
                        formatstr(met.qac, 0, mtd[4]);
                        formatstr(met.sac, 0, mtd[5]);

                        if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                            sprintf(tmp, POST_DATA_CONTENT_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12], des[13], des[14], des[15], des[16], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                            sprintf(tmp, POST_DATA_CONTDC1_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[11], des[13], des[14], des[15], des[16], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                            sprintf(tmp, POST_DATA_CONTDC2_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[10], des[12], des[13], des[14], des[15], des[16], mtd[0], mtd[1], mtd[2], mtd[3], mtd[4], mtd[5]);
                        memset(&met.psn, 0, 10);
                    }
                }
                else
                {
                    char emt[6][16] = {0};
                    if (data_type == 1) //POST_DATA_HEADER_1PAHSE
                        sprintf(tmp, POST_DATA_CONTENT_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[10], des[11], des[12], des[13], des[14], des[15], des[16], emt[0], emt[1], emt[2], emt[3], emt[4], emt[5]);
                    if (data_type == 2) //POST_DATA_HEADER_1PAHSE_DC1
                        sprintf(tmp, POST_DATA_CONTDC1_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[9], des[11], des[13], des[14], des[15], des[16], emt[0], emt[1], emt[2], emt[3], emt[4], emt[5]);
                    if (data_type == 3) //POST_DATA_HEADER_1PAHSE_DC2
                        sprintf(tmp, POST_DATA_CONTDC2_MET_3PHASE, time, des[0], des[1], des[2], des[3], des[4], des[5], des[6], des[7], des[8], des[10], des[12], des[13], des[14], des[15], des[16], emt[0], emt[1], emt[2], emt[3], emt[4], emt[5]);
                }
                printf("tmp %s %d \n", tmp, strlen(tmp));
                if (strlen(tmp) > 0)
                {
                    if (i + 1 < num)
                    {
                        strcat(tmp, "\r\n");
                        strcat(tmp_data, tmp);
                    }
                    else
                        strcat(tmp_data, tmp);
                }
                memset(tmp, 0, sizeof(tmp));
            }

            strcat(buf, tmp_data);
            // memset(tmp, 0, sizeof(tmp));

            // memset(tmp, 0, sizeof(tmp));
            // sprintf(tmp, POST_REQ_HEADER, strlen(buf));
            // strcat(tmp, buf);
            // memset(buf, 0, sizeof(buf));
            // memcpy(buf, tmp, strlen(tmp));
        }
    }

    printf("pack %s \n", buf);
    return 0;
}

int find_only_meter(Inv_data *tmp, int num)
{
    int i = 0, j = 0;

    for (; i < num; i++)
        if (0 == strncmp(tmp[i].psn, "SDM", 3))
            j++;
        else
            break;

    return j;
}
/*
 *package post data and sent data
 */
int pub_invs_data(Inv_data data, Inv_data meterdata, int flg)
{
    char payload[4096] = {0};
    Inv_data invdata[16] = {0};
    int i = 0, rst = -1;
    int lines = 0;
    int file_end = 0;
    int meter_found = 0;
    int meter_save = 0;

    i = find_loacl_data(invdata, &lines, &file_end, &meter_found);
    if (i != lines)
        i = 0;
    //memcmp(&invdata[i], &data, sizeof(Inv_data));
    if (flg == 1)
    {
        invdata[i] = data;
        i++;
        printf("find new inv data %d====>>>>>%s %d \n", i, data.psn, data.PV_cur_voltg[0].iVol);
    }
    printf("found %d invs data ====>>>>>%s %d \n", i, data.psn, data.PV_cur_voltg[0].iVol);
    //printf("PV %d %d %s\n", invdata[0].PV_cur_voltg[0].iVol, invdata[0].PV_cur_voltg[1].iVol, invdata[0].psn);

    if (i > 0 && i <= 12)
    {
        if (0 == strncmp(meterdata.psn, "SDM", 3))
            meter_found = 1;

        meter_save = find_only_meter(invdata, i);
        printf("find only meter data %d %d mt %d dtsn %s\n", meter_save, i, meter_found, invdata[meter_save].psn);
        if (meter_save == i || strlen(invdata[meter_save].psn) < 6)
            goto exit_pub;
        format_data_muilts(payload, &invdata[meter_save], meterdata, meter_found, i - meter_save);
        if (strlen(payload) < 500)
            goto exit_pub;
        //printf("package %d data %s \n", strlen(payload), payload);
        int http_code = 0;
        https_postdata(payload, &http_code, &rst);
        printf("post data res %d %d %d \n", http_code, rst, strlen(payload));
        if (http_code == 202 && rst == 1)
        {
        exit_pub:
            if (lines)
            {
                printf("delet file %d lines\n", lines);
                delet_loacl_data(lines, file_end);
            }
            return 0;
        }
        else
            return -1;
    }
    return -2;
}

uint32_t RTC_ConvertDatetimeToSeconds(struct tm *datetime)
{
    struct timeval tv = {0};

    if (datetime->tm_year > 2019 && datetime->tm_year < 2050)
    {
        datetime->tm_year -= 1900;
        datetime->tm_mon -= 1;

        if (datetime->tm_year > 119)
        {
            tv.tv_sec = mktime(datetime);
            printf("data to second %d\n", tv.tv_sec);
            return (tv.tv_sec);
        }
    }
    return -1;
}

int RTC_ConvertSecondsToDatetime(uint32_t seconds, struct tm *datetime)
{
    struct tm *info;

    info = localtime(&seconds);
    memcpy(datetime, info, sizeof(struct tm));
    return 0;
}
/*
 *setting time from string
 */
int set_time_from_string(const char *chGetTiem, int tmz)
{
    struct tm rtime;
    printf("GetTiem : %s \n", chGetTiem); //2021-03-22 08:25:00
    char chGetTemp[5];

    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[0], 4);
    if (atoi(chGetTemp) < 2019 || atoi(chGetTemp) > 2050)
    {
        return -1;
    }
    rtime.tm_year = atoi(chGetTemp);

    //month
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[5], 2);
    rtime.tm_mon = atoi(chGetTemp);

    //day
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[8], 2); //2021-03-22 08:35:36
    rtime.tm_mday = atoi(chGetTemp);
    //hour
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[11], 2);
    rtime.tm_hour = atoi(chGetTemp);
    //minute
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[14], 2);
    rtime.tm_min = atoi(chGetTemp);
    //second
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[17], 2);
    rtime.tm_sec = atoi(chGetTemp);

    printf("Cloud time:%s  %02d-%02d-%02d %02d:%02d:%02d\n ", chGetTiem,
           rtime.tm_year, rtime.tm_mon, rtime.tm_mday, rtime.tm_hour, rtime.tm_min, rtime.tm_sec);

    int rmt_tmp = RTC_ConvertDatetimeToSeconds(&rtime);
    printf("cloud time seconds %d %d\r\n", rmt_tmp, rmt_tmp + tmz * 60);
    //rmt_tm_sec += tmz*60;
    //parse dst
    int rmt_tm_sec = read_timezone(rmt_tmp);
    printf("adjust time seconds %d \r\n", rmt_tm_sec);
    //
    struct tm changed_tm = {0};
    RTC_ConvertSecondsToDatetime(rmt_tm_sec, &changed_tm);
    printf("changed local time:%d  %02d-%02d-%02d %02d:%02d:%02d\n ", rmt_tm_sec,
           changed_tm.tm_year + 1900, changed_tm.tm_mon + 1, changed_tm.tm_mday, changed_tm.tm_hour, changed_tm.tm_min, changed_tm.tm_sec);
    rtime = changed_tm;

    //-----------------------------------------
    struct tm curr_time;
    time_t t = time(NULL);

    localtime_r(&t, &curr_time);
    // curr_time.tm_year += 1900;

    printf("get local time: %04d-%02d-%02d %02d:%02d:%02d \n",
           curr_time.tm_year + 1900, curr_time.tm_mon + 1, curr_time.tm_mday, curr_time.tm_hour, curr_time.tm_min, curr_time.tm_sec);

    if ((rtime.tm_year != curr_time.tm_year) || (rtime.tm_mon != curr_time.tm_mon) || (rtime.tm_mday != curr_time.tm_mday) || (rtime.tm_hour != curr_time.tm_hour) || (abs(rtime.tm_min - curr_time.tm_min) >= 2))
    {
        printf("Cloud current time > 2  minutes and set it \n");

        struct timeval stime = {0};
        stime.tv_sec = mktime(&rtime);

        settimeofday(&stime, NULL);
    }
    else
    {
        printf("Cloud current time < 2 minute, not set it\n");
    }
    return 0;
}
