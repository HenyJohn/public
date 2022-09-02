#include "kaco.h"
#include "save_inv_data.h"
#include "asw_modbus.h"
#include "https_client.h"
#include "meter.h"
#include "asw_protocol_ftp.h"
#include "asw_mqtt.h"

#include "math.h" //kaco-lanstick

char metermodesn[7][10] = {
    "SDM630CT",
    "SDM630DC",
    "SDM230",
    "SDM220",
    "SDM120",
    "NORK"};

//////////////////////////////////////////////////////////
#define DEMO_DEVICE_NAME_TEST "B3278A260749"
//--------------------kaco -test
const char *kaco_dev_psn[] =
    {
        "30.0NX312004373",
        "3.7NX12002145",
        "SP001000221C0007",
        "5.5.0NX12000033",  
        "RG500001119C0001"};

const char *kaco_conv_psn[] =
    {
        "30.0NX312004373",
        "5.0NX12000010",
        "5.0NX12000087",
        "5.0NX312001410",
        "10.0NX312001413"};

uint8_t g_kaco_ftp_start = 0;

//////////////////////////////////////////////////////////

///-------------kaco lanstick --------------///
///////////////////////////////////////////////////////////////////////
// typedef struct KACO_CSV_HEADER
// {
//     char dataloger[32]; // 1
//     char invsn[64];     // 2
//     char metersn[64];   // 3
// } Stu_KacoCsvHeader;

#define KACO_CSV_HEADER_NUM 6
#define KACO_CSV_UNIPHASE_COLUMN_NUM 13       // 12+1
#define KACO_CSV_UNIPHASE_COLUMN_NUM_METER 19 // 12+1+6

#define KACO_CSV_TRIPHASE_COLUMN_NUM 17       // 16+1
#define KACO_CSV_TRIPHASE_COLUMN_NUM_METER 23 // 16+1+6

#define KACO_CSV_HEADER_DATALOGGER "datalogger,%s\r\n"

char strKacoHeaders[KACO_CSV_HEADER_NUM][16] = {
    "type",
    "name",
    "serial",
    "uid",
    "unit",
    "abbreviation"};

///-- UniPhase no meter
char strKaco_Uni_Uint[KACO_CSV_UNIPHASE_COLUMN_NUM][8] = {
    /*1--2----3-----4----5----6----7----8----9----10-----11---12----13- */
    "W", "", "Hz", "V", "A", "V", "V", "A", "A", "kWh", "kWh", "", "°C"};
char strKaco_Uni_Abr[KACO_CSV_UNIPHASE_COLUMN_NUM][16] = {
    /*1---------2-------3-------4--------5---------6-------7-------8--------9--------10--------11--------12-----13-*/
    "P_AC", "COS_PHI", "F_AC", "U_AC", "I_AC", "U_DC1", "U_DC2", "I_DC1", "I_DC2", "E_DAY", "E_TOTAL", "ERROR", "T"};

///-- UniPhase with Meter
char strKaco_UniMeter_Uint[KACO_CSV_UNIPHASE_COLUMN_NUM_METER][8] = {
    /*1--2----3-----4----5----6----7----8----9----10-----11---12----13----14-----15----16----17----18----19--*/
    "W", "", "Hz", "V", "A", "V", "V", "A", "A", "kWh", "kWh", "", "°C", "kWh", "kWh", "Hz", "W", "VAr", "VA"};
char strKaco_UniMeter_Abr[KACO_CSV_UNIPHASE_COLUMN_NUM_METER][16] = {
    /*1---------2---------3-------4-------5-------6----------7-------8-------9--------10--*/
    "P_AC", "COS_PHI", "F_AC", "U_AC", "I_AC", "U_DC1", "U_DC2", "I_DC1", "I_DC2", "E_DAY",
    "E_TOTAL", "ERROR", "T", "M_AC_E_EXP", "M_AC_E_IMP", "M_AC_F", "M_AC_P", "M_AC_Q", "M_AC_S"};
/*------11-------12------13--------14------------15----------16--------17-------18--------19--*/

///-- TriPhase no meter
char strKaco_Tri_Uint[KACO_CSV_TRIPHASE_COLUMN_NUM][8] = {
    /*1--2----3-----4----5----6----7----8----9---10---11---12---13----14-----15----16---17--*/
    "W", "", "Hz", "V", "V", "V", "A", "A", "A", "V", "V", "A", "A", "kWh", "kWh", "", "°C"};
char strKaco_Tri_Abr[KACO_CSV_TRIPHASE_COLUMN_NUM][8] = {
    /*1---------2---------3-------4-------5---------6-------7---------8-------9--------10-------11-------12------13--------14-------15---------16------17-*/
    "P_AC", "COS_PHI", "F_AC", "U_AC1", "U_AC2", "U_AC3", "I_AC1", "I_AC2", "I_AC3", "U_DC1", "U_DC2", "I_DC1", "I_DC2", "E_DAY", "E_TOTAL", "ERROR", "T"};

///-- TriPhase with Meter
char strKaco_TriMeter_Uint[KACO_CSV_TRIPHASE_COLUMN_NUM_METER][8] = {
    /*1--2----3-----4----5----6---7----8-----9---10---11---12---13----14-----15----16---17----18-----19-----20---21----22----23-*/
    "W", "", "Hz", "V", "V", "V", "A", "A", "A", "V", "V", "A", "A", "kWh", "kWh", "", "°C", "kWh", "kWh", "Hz", "W", "VAr", "VA"};
char strKaco_TriMeter_Abr[KACO_CSV_TRIPHASE_COLUMN_NUM_METER][16] = {
    /*1--------2--------3-----4-------5-------6-------7--------8-------9------10------11------12------13------14------15----*/
    "P_AC", "COS_PHI", "F_AC", "U_AC1", "U_AC2", "U_AC3", "I_AC1", "I_AC2", "I_AC3", "U_DC1", "U_DC2", "I_DC1", "I_DC2", "E_DAY", "E_TOTAL",
    "ERROR", "T", "M_AC_E_EXP", "M_AC_E_IMP", "M_AC_F", "M_AC_P", "M_AC_Q", "M_AC_S"};
/*----16------17------18-------------19----------20---------21-------22-------23---*/

////-----------------------///
////----- FUNC DEFINE -----///
////-----------------------///

int asw_int_format_strfloat(int xNum, uint8_t dot, char *strFloat)
{
    uint8_t mdot = dot;
    char tmpbuf[20] = {0};
    int len = 0;
    uint32_t multDot = 1;
    if (mdot == 0)
    {
        len = sprintf(strFloat, ",%d", xNum);
    }
    else
    {
        for (uint8_t i = 0; i < mdot; i++)
            multDot *= 10;

        assert(multDot != 0);
        float fnum = (float)xNum / multDot;

        sprintf(tmpbuf, ",%%.%df", mdot); /// TODO mark

        len = sprintf(strFloat, tmpbuf, fnum);

        // len = sprintf(strFloat, "%.2f", fnum);
    }
    return len;
}

/**
 * @brief kaco-lanstick v1.0.0
 *        无电表kaco-csv-header
 *
 * @param fpBuff
 * @param pIndex
 * @param phaseType
 * @param invSn
 * @return int
 */
int asw_kaco_csv_header_noMeter(char *fpBuff, uint16_t pIndex, int phaseType, char *invSn)
{
    assert(invSn != NULL);
    assert(fpBuff != NULL);

    assert(strlen(invSn) > 0);

    int mPhaseType = phaseType;

    uint16_t mKacoCsvColumNum = 0;
    uint16_t mIndex = pIndex;

    if (mPhaseType == 1) // uniphase
    {
        mKacoCsvColumNum = KACO_CSV_UNIPHASE_COLUMN_NUM;
    }

    else if (mPhaseType == 3) // triphase
    {
        mKacoCsvColumNum = KACO_CSV_TRIPHASE_COLUMN_NUM;
    }

    for (uint8_t i = 0; i < KACO_CSV_HEADER_NUM; i++)
    {
        mIndex += sprintf(fpBuff + mIndex, "%s", strKacoHeaders[i]);
        for (uint8_t j = 0; j < mKacoCsvColumNum; j++)
        {
            switch (i)
            {
            case 0: // 0-"type"
            {
                if (j < mKacoCsvColumNum - 1)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", "inverter");
                else
                    mIndex += sprintf(fpBuff + mIndex, ",%s", "meteo");
            }
            break;

            case 1: // 1-"name"
            case 2: // 2-"serial"
            case 3: // 3-"uid"
                mIndex += sprintf(fpBuff + mIndex, ",%s", invSn);
                break;

            case 4: // 4-"unit"

                if (mPhaseType == 1) // uniphase
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_Uni_Uint[j]);
                else if (mPhaseType == 3)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_Tri_Uint[j]);

                break;

            case 5: // 5-"abbreviation"
                if (mPhaseType == 1)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_Uni_Abr[j]);
                else if (mPhaseType == 3)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_Tri_Abr[j]);

                break;

            default:
                break;
            }
        }
        mIndex += sprintf(fpBuff + mIndex, "\r\n");
    }

    return mIndex;
}

/**
 * @brief kaco-lanstick v1.0.0
 *        有电表kaco-csv-header
 *
 * @param fpBuff
 * @param pIndex
 * @param invSn
 * @param meterSn
 * @return int
 */
int asw_kaco_csv_header_withMeter(char *fpBuff, uint16_t pIndex, int phaseType, char *invSn, char *meterSn)
{
    assert(invSn != NULL);
    assert(meterSn != NULL);
    assert(fpBuff != NULL);

    assert(strlen(invSn) > 0);
    assert(strlen(meterSn) > 0);

    int mPhaseType = phaseType;

    uint16_t mKacoCsvColumNum = 0;
    uint16_t mIndex = pIndex;

    if (mPhaseType == 1) // uniphase
    {
        mKacoCsvColumNum = KACO_CSV_UNIPHASE_COLUMN_NUM_METER; // KACO_CSV_UNIPHASE_COLUMN_NUM;
    }

    else if (mPhaseType == 3) // triphase
    {
        mKacoCsvColumNum = KACO_CSV_TRIPHASE_COLUMN_NUM_METER; // KACO_CSV_TRIPHASE_COLUMN_NUM;
    }

    for (uint8_t i = 0; i < KACO_CSV_HEADER_NUM; i++)
    {
        mIndex += sprintf(fpBuff + mIndex, "%s", strKacoHeaders[i]);
        for (uint8_t j = 0; j < mKacoCsvColumNum; j++)
        {
            switch (i)
            {
            case 0: // 0-"type"
            {
                if (j < mKacoCsvColumNum - 7)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", "inverter");
                else if (mKacoCsvColumNum - 7 == j)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", "meteo");
                else
                    mIndex += sprintf(fpBuff + mIndex, ",%s", "meter");
            }
            break;

            case 1: // 1-"name"
            case 2: // 2-"serial"
            case 3: // 3-"uid"
            {
                if (j < mKacoCsvColumNum - 6)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", invSn);
                else
                    mIndex += sprintf(fpBuff + mIndex, ",%s", meterSn);
            }
            break;

            case 4: // 4-"unit"
                if (mPhaseType == 1)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_UniMeter_Uint[j]);
                else if (mPhaseType == 3)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_TriMeter_Uint[j]);

                break;

            case 5: // 5-"abbreviation"
                if (mPhaseType == 1)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_UniMeter_Abr[j]);
                else if (mPhaseType == 3)
                    mIndex += sprintf(fpBuff + mIndex, ",%s", strKaco_TriMeter_Abr[j]);
                break;

            default:
                break;
            }
        }
        mIndex += sprintf(fpBuff + mIndex, "\r\n");
    }

    return mIndex;
}

/**
 * @brief  asw_kaco_csv_header
 *         kaco-lanstick v1.0.0
 *
 * @param fpBuff
 * @param isFoundMeter
 * @param phaseType
 * @param pStuKacoCsv
 * @return int
 */
int asw_kaco_csv_header(char *fpBuff, bool isFoundMeter, int phaseType, Stu_KacoCsvHeader *pStuKacoCsv)
{
    assert(fpBuff != NULL);
    uint16_t mIndex;

    uint16_t mDataLen;

    int mPhaseType = phaseType;

    assert(mPhaseType == 1 || mPhaseType == 3);

    assert(strlen(pStuKacoCsv->dataloger) > 0);
    assert(strlen(pStuKacoCsv->invsn) > 0);

    mIndex = sprintf(fpBuff, KACO_CSV_HEADER_DATALOGGER, pStuKacoCsv->dataloger);

    assert(mIndex > 0);

    if (isFoundMeter)
    {
        assert(strlen(pStuKacoCsv->metersn) > 0);
        mDataLen = asw_kaco_csv_header_withMeter(fpBuff, mIndex, mPhaseType, pStuKacoCsv->invsn, pStuKacoCsv->metersn);
    }
    else
    {
        mDataLen = asw_kaco_csv_header_noMeter(fpBuff, mIndex, mPhaseType, pStuKacoCsv->invsn);
    }
    return mDataLen;
}

/**
 * @brief 生成带电表数据
 *        kaco-lanstick v1.0.0
 *
 * @param fpBuff
 * @param phaseType
 * @param invData
 * @param meterData
 * @return int
 */
int asw_kaco_csv_data_withMeter(char *fpBuff, int phaseType, Inv_data *invData, Inv_data *meterData)
{
    assert(fpBuff != NULL);
    int mPhaseType = phaseType;
    assert(mPhaseType == 1 || mPhaseType == 3);
    char time[128] = {0};
    struct tm curr_date = {0};
    int mpBufIndex = 0;

    // char buf[1024] = {0}; // Tsize is ok ? MAKR TODO TEST

    //------- get time -------//
    get_data_time(invData->time, &curr_date);
    if (curr_date.tm_min % 10 > 0 && curr_date.tm_min % 10 < 5) // 1234 5 67890
    {
        char tmp_m = curr_date.tm_min / 10;
        curr_date.tm_min = tmp_m * 10 + 0;
    }
    else if (curr_date.tm_min % 10 > 5 && curr_date.tm_min % 10 < 10)
    {
        char tmp_m = curr_date.tm_min / 10;
        curr_date.tm_min = tmp_m * 10 + 5;
    }

    // short datlen =
    mpBufIndex += sprintf(fpBuff, "%04d-%02d-%02d %02d:%02d:%02d",
                          curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

    // char des[18][16] = {0};
    // char mtd[6][16] = {0};
    // uint8_t mIndex = 0;

    mpBufIndex += asw_int_format_strfloat(invData->pac, 0, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->cosphi, 2, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->fac, 2, fpBuff + mpBufIndex);

    if (mPhaseType == 1)
    {

        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iCur, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iVol, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iCur, 2, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iCur, 2, fpBuff + mpBufIndex);
    }
    else if (mPhaseType == 3)
    {
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[1].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[2].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iCur, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[1].iCur, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[2].iCur, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iVol, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iCur, 2, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iCur, 2, fpBuff + mpBufIndex);
    }

    mpBufIndex += asw_int_format_strfloat(invData->e_today, 1, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->e_total, 1, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->error, 0, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->col_temp, 1, fpBuff + mpBufIndex);

    // meter-data
    mpBufIndex += asw_int_format_strfloat(meterData->h_total, 1, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(meterData->e_total, 1, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(meterData->fac, 2, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(meterData->pac, 0, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(meterData->qac, 0, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(meterData->sac, 0, fpBuff + mpBufIndex);

    mpBufIndex += sprintf(fpBuff + mpBufIndex, "\r\n");

    return mpBufIndex;
}

/**
 * @brief 生成不带电表数据
 *        kaco-lanstick v1.0.0
 *
 * @param fpBuff
 * @param phaseType
 * @param invData
 * @return int
 */
int asw_kaco_csv_data_noMeter(char *fpBuff, int phaseType, Inv_data *invData)
{
    assert(fpBuff != NULL);
    int mPhaseType = phaseType;
    assert(mPhaseType == 1 || mPhaseType == 3);
    char time[128] = {0};
    struct tm curr_date = {0};
    int mpBufIndex = 0;

    // char buf[1024] = {0}; // Tsize is ok ? MAKR TODO TEST

    //------- get time -------//
    get_data_time(invData->time, &curr_date);
    if (curr_date.tm_min % 10 > 0 && curr_date.tm_min % 10 < 5) // 1234 5 67890
    {
        char tmp_m = curr_date.tm_min / 10;
        curr_date.tm_min = tmp_m * 10 + 0;
    }
    else if (curr_date.tm_min % 10 > 5 && curr_date.tm_min % 10 < 10)
    {
        char tmp_m = curr_date.tm_min / 10;
        curr_date.tm_min = tmp_m * 10 + 5;
    }

    mpBufIndex += sprintf(fpBuff, "%04d-%02d-%02d %02d:%02d:%02d",
                          curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec);

    mpBufIndex += asw_int_format_strfloat(invData->pac, 0, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->cosphi, 2, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->fac, 2, fpBuff + mpBufIndex);
    if (mPhaseType == 1)
    {

        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iCur, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iVol, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iCur, 2, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iCur, 2, fpBuff + mpBufIndex);
    }
    else if (mPhaseType == 3)
    {
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[1].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[2].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[0].iCur, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[1].iCur, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->AC_vol_cur[2].iCur, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iVol, 1, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iVol, 1, fpBuff + mpBufIndex);

        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[0].iCur, 2, fpBuff + mpBufIndex);
        mpBufIndex += asw_int_format_strfloat(invData->PV_cur_voltg[1].iCur, 2, fpBuff + mpBufIndex);
    }

    mpBufIndex += asw_int_format_strfloat(invData->e_today, 1, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->e_total, 1, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->error, 0, fpBuff + mpBufIndex);
    mpBufIndex += asw_int_format_strfloat(invData->col_temp, 1, fpBuff + mpBufIndex);

    mpBufIndex += sprintf(fpBuff + mpBufIndex, "\r\n");

    return mpBufIndex;
}

/**
 * @brief 生成csv内容格式
 *   kaco-lanstick-v1.0.0
 * @param buf
 * @param invData
 * @param metData
 * @return int
 */
int asw_kaco_format_csv_header(char *fpBuff, bool isFoundMeter, Stu_KacoCsvHeader *pStuKacoCsv)
{
    int mInvPhaseType = 0;
    assert(fpBuff != NULL);
    assert(pStuKacoCsv != NULL);

    mInvPhaseType = get_inv_phase_type(pStuKacoCsv->crt_index) == 0x31 ? 1 : 3;

    printf("\n---DEBUG-INFO*---[inv:%d]---[phase:%d]\n", pStuKacoCsv->crt_index, mInvPhaseType);

    int mlenCsvHeader = asw_kaco_csv_header(fpBuff, isFoundMeter, mInvPhaseType, pStuKacoCsv);

    // printf("\n----------- kaco csv header start -----------\n");
    // printf("%s", fpBuff);
    // printf("\n----------- kaco csv header  end  -----------\n");

    return mlenCsvHeader;
}
/**
 * @brief 生成csv 数据
 *   kaco-lanstick-v1.0.0
 *
 * @param fpDataBuf
 * @param isFoundMeter
 * @param invData
 * @param meterData
 * @return int
 */
int asw_kaco_format_csv_data(char *fpDataBuf, bool isFoundMeter, Inv_data *invData, Inv_data *meterData, uint8_t inv_index)
{
    int mInvPhaseType = 0;
    int mlenCsvData = 0;
    assert(fpDataBuf != NULL);
    assert(invData != NULL);

    mInvPhaseType = get_inv_phase_type(inv_index) == 0x31 ? 1 : 3;

    assert(mInvPhaseType == 1 || mInvPhaseType == 3);

    if (isFoundMeter)
    {
        assert(meterData != NULL);
        mlenCsvData = asw_kaco_csv_data_withMeter(fpDataBuf, mInvPhaseType, invData, meterData);
    }
    else
    {
        mlenCsvData = asw_kaco_csv_data_noMeter(fpDataBuf, mInvPhaseType, invData);
    }

    printf("\n----------- kaco csv data start -----------\n");
    printf("%s", fpDataBuf);
    printf("\n----------- kaco csv data  end  -----------\n");

    return mlenCsvData;
}

//

int asw_pub_invs_data(kaco_inv_data_t kacoInvdata, Inv_data meterdata, int flg)
{
    char payload[4096] = {0};
    kaco_inv_data_t invdata = {0};
    int i = 0, rst = -1;
    int lines = 0;
    int file_end = 0;
    int meter_found = 0;
    int meter_save = 0;

    int64_t msTimeStart, msTimeEnd;

    char *pc_meter_mode_sn = NULL;
    if (g_kaco_ftp_start == 0)
    {
        if (asw_ftp_connect() != 0)
        {
            // TODO  retry 1-3
            ESP_LOGW("Kaco.c", "ftp connect is err0!");

            return -1;
        }
        g_kaco_ftp_start = 1;
    }
    invdata = kacoInvdata;

    ESP_LOGI("---Kaco Inv Data ---", " new data will upload.");
    // TODO
    if (flg == 1)
        printf("find new inv data ====>>>>>%s %d \n", invdata.crt_inv_data.psn, invdata.crt_inv_data.PV_cur_voltg[0].iVol);

    msTimeStart = esp_timer_get_time() / 1000;
    if (0 == strncmp(meterdata.psn, "SDM", 3))
    {

        meter_found = 1;
        int meter_mode_index = get_meter_index();
        if (meter_mode_index >= 0 && meter_mode_index <= 5)
            pc_meter_mode_sn = metermodesn[meter_mode_index];
    }

    printf("\n----------- DEBUG INFO: ----------[meter_found:%d]\n", meter_found);

    ///////////////////////////////////////////////////

    //----------------kaco test------------------//
    for (uint8_t i = 0; i < 5; i++)
    {
        if (strcmp(invdata.crt_inv_data.psn, kaco_dev_psn[i]) == 0)
        {
            memset(invdata.crt_inv_data.psn, 0, sizeof(invdata.crt_inv_data.psn));

            memcpy(invdata.crt_inv_data.psn, kaco_conv_psn[i], strlen(kaco_conv_psn[i]) + 2);
        }
    }

    ////////////////////////////////////////////////////

    static Stu_KacoCsvHeader mStu_KacoCSVHeader;

    memcpy(mStu_KacoCSVHeader.dataloger, DEMO_DEVICE_NAME_TEST, sizeof(DEMO_DEVICE_NAME_TEST)); // MARK for test  DEMO_DEVICE_NAME
    memcpy(mStu_KacoCSVHeader.invsn, invdata.crt_inv_data.psn, sizeof(invdata.crt_inv_data.psn));
    mStu_KacoCSVHeader.crt_index = invdata.crt_inv_index;

    if (meter_found)
    {
        if (pc_meter_mode_sn != NULL)
            memcpy(mStu_KacoCSVHeader.metersn, pc_meter_mode_sn, strlen(pc_meter_mode_sn) + 1);
        else
        {
            ESP_LOGE("-- Kaco.c  ERRO--", " Failed to get meter psn.");
            return -1;
        }
    }

    int mHeaderLen = asw_kaco_format_csv_header(payload, meter_found, &mStu_KacoCSVHeader);

    // format_data_muilts(payload, &invdata[meter_save], meterdata, meter_found, i - meter_save);
    if (strlen(payload) < 500)
    {
        ESP_LOGW("-- Kaco upload scv ERRO---", "payload size if less than 500!");
        return -1;
    }

    char mpFileDataBuf[1024] = {0}; // mark size if OK ??? TO TEST

    int mDataLen = asw_kaco_format_csv_data(mpFileDataBuf, meter_found, &invdata.crt_inv_data, &meterdata, invdata.crt_inv_index);
    if (strlen(mpFileDataBuf) <= 0)
    {

        ESP_LOGW("-- Kaco upload scv ERRO---", "data  size if less than  or equal 0!");
        return -1;
    }

    strcat(payload, mpFileDataBuf);

    printf("\n============ csv total content ======== \n");
    printf("payload size-%d-\n%s", strlen(payload), payload);
    printf("\n============ csv total content ======== \n");

    msTimeEnd = esp_timer_get_time() / 1000;

    ESP_LOGW("====  Kaco ====", " payload len:%d,format data use:%lld ms \n", strlen(payload) + 1, msTimeEnd - msTimeStart);

    msTimeStart = esp_timer_get_time() / 1000;
    //----------------------------------------------------------//
    rst = asw_ftp_upload(invdata.crt_inv_data.psn, payload, strlen(payload) + 1);

    msTimeEnd = esp_timer_get_time() / 1000;
    ESP_LOGW("=== Kaco ===", " ftp update data use:%lld ms \n", msTimeEnd - msTimeStart);

    if (rst == 0)
    {
        return 0;
    }
    else
    {
        ESP_LOGE("---Kaco Upload File To Ftp ERRO--", "Faied to upload %s real inv data to ftp file." ,invdata.crt_inv_data.psn);
        return -1;
    }

    //----------------------------------------------------------//
}

/**
 * @brief  上传本地文件到ftp服务器
 *
 * @param localFile  /inv/psn_meterx.csv
 * @return int  上传成功返回0，失败返回-1
 */
int asw_upload_history_to_ftp(char *localFileName)
{
    /*--1.生成远程服务器ftp文件名 */
    assert(localFileName != NULL);
    char srcFileName[80] = {0};

    char cFileName[36] = {0};
    char cFileNameTime[40] = {0};
    // fileName [psn]_meterx
    /*根据fileName 重新生成上传到ftp服务器的文件名 psn_xxxx_xxxx_xx.csv */

    int res = sscanf(localFileName, "/inv/%[^_meter]", cFileName);
    if (res <= 0)
    {
        ESP_LOGE("-- Kaco Upload File Erro --", "Failed get right fileName.");
        return -1;
    }

    //时间处理【min需要0或者5】

    struct tm curr_time;
    time_t t = time(NULL);
    localtime_r(&t, &curr_time);
#if 1                                                           /// DEBUG
    if (curr_time.tm_min % 10 > 0 && curr_time.tm_min % 10 < 5) // 1234 5 67890
    {
        char tmp_m = curr_time.tm_min / 10;
        curr_time.tm_min = tmp_m * 10;
    }
    else if (curr_time.tm_min % 10 > 5 && curr_time.tm_min % 10 < 10)
    {
        char tmp_m = curr_time.tm_min / 10;
        curr_time.tm_min = tmp_m * 10 + 5;
    }
#endif
    sprintf(cFileNameTime, "%04d_%02d%02d_%02d%02d", curr_time.tm_year + 1900, curr_time.tm_mon + 1,
            curr_time.tm_mday, curr_time.tm_hour, curr_time.tm_min);

    // sprintf(srcFileDtTm, "%04d_%02d%02d_%02d%02d", curr_time.tm_year + 1900, curr_time.tm_mon + 1,
    // curr_time.tm_mday, curr_time.tm_hour, curr_time.tm_min);

    sprintf(srcFileName, "%s_%s.csv", cFileName, cFileNameTime);

    printf("\n=== debug info: upload ftp filename:%s", srcFileName);

    // return 0;//FOR TEST
    /* 调用函数 上传ftp服务器 */
    if (g_kaco_ftp_start == 0)
    {
        if (asw_ftp_connect() != 0)
        {
            // TODO  retry 1-3
            ESP_LOGW("Kaco.c", "ftp connect is err0!");

            return -1;
        }
        g_kaco_ftp_start = 1;
    }

    int res1 = asw_ftp_upload_file_byName(localFileName, srcFileName);
    printf("\n========DEBUG-INFO: localFile:%s,ftp file:%s ============\n", localFileName, srcFileName);

    return res1;
}
////////////////////////////////////////////////////////////////
///-------------kaco lanstick --------------///

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
#if 0
int pub_invs_data(Inv_data data, Inv_data meterdata, int flg)
{
    char payload[4096] = {0};
    Inv_data invdata[16] = {0};
    int i = 0, rst = -1;
    int lines = 0;
    int file_end = 0;
    int meter_found = 0;
    int meter_save = 0;

    int64_t msTimeStart, msTimeEnd;

    if (g_kaco_ftp_start == 0)
    {
        if (asw_ftp_connect() != 0)
        {
            // TODO  retry 1-3
            ESP_LOGW("Kaco.c", "ftp connect is err0!");

            return -1;
        }
        g_kaco_ftp_start = 1;
    }

    i = find_local_data(invdata, &lines, &file_end, &meter_found);
    if (i != lines)
        i = 0;

    if (flg == 1)
    {
        invdata[i] = data;
        i++;
        printf("find new inv data %d====>>>>>%s %d \n", i, data.psn, data.PV_cur_voltg[0].iVol);
    }
    printf("found %d invs data ====>>>>>%s %d \n", i, data.psn, data.PV_cur_voltg[0].iVol);

    if (i > 0 && i <= 12)
    {

        msTimeStart = esp_timer_get_time() / 1000;
        if (0 == strncmp(meterdata.psn, "SDM", 3))
            meter_found = 1;

        meter_save = find_only_meter(invdata, i);
        printf("find only meter data %d %d mt %d dtsn %s\n", meter_save, i, meter_found, invdata[meter_save].psn);
        if (meter_save == i || strlen(invdata[meter_save].psn) < 6)
            goto exit_pub;

        format_data_muilts(payload, &invdata[meter_save], meterdata, meter_found, i - meter_save);
        if (strlen(payload) < 500)
            goto exit_pub;

        // create csv file to upload with ftp.
        // asw_ftp_create_file(invdata[meter_save].psn, payload);

        msTimeEnd = esp_timer_get_time() / 1000;

        ESP_LOGW("====  Kaco ===", " payload len:%d,format data use:%lld ms \n", strlen(payload) + 1, msTimeEnd - msTimeStart);

        msTimeStart = esp_timer_get_time() / 1000;
        //----------------------------------------------------------//
        rst = asw_ftp_upload(invdata[meter_save].psn, payload, strlen(payload) + 1);

        msTimeEnd = esp_timer_get_time() / 1000;
        ESP_LOGW("=== Kaco ===", " ftp update data use:%lld ms \n", msTimeEnd - msTimeStart);
        if (rst == 0)
        {

        exit_pub:
            if (lines)
            {
                printf("delet file %d lines\n", lines);
                delet_local_data(lines, file_end);
            }
            return 0;
        }
        else
            return -1;

        //----------------------------------------------------------//
    }

    else
        return -2;
}

#endif

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
            printf("data to second %ld\n", tv.tv_sec);
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
    printf("GetTiem : %s \n", chGetTiem); // 2021-03-22 08:25:00
    char chGetTemp[5];

    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[0], 4);
    if (atoi(chGetTemp) < 2019 || atoi(chGetTemp) > 2050)
    {
        return -1;
    }
    rtime.tm_year = atoi(chGetTemp);

    // month
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[5], 2);
    rtime.tm_mon = atoi(chGetTemp);

    // day
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[8], 2); // 2021-03-22 08:35:36
    rtime.tm_mday = atoi(chGetTemp);
    // hour
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[11], 2);
    rtime.tm_hour = atoi(chGetTemp);
    // minute
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[14], 2);
    rtime.tm_min = atoi(chGetTemp);
    // second
    memset(chGetTemp, 0x00, 5);
    memcpy(chGetTemp, &chGetTiem[17], 2);
    rtime.tm_sec = atoi(chGetTemp);

    printf("Cloud time:%s  %02d-%02d-%02d %02d:%02d:%02d\n ", chGetTiem,
           rtime.tm_year, rtime.tm_mon, rtime.tm_mday, rtime.tm_hour, rtime.tm_min, rtime.tm_sec);

    int rmt_tmp = RTC_ConvertDatetimeToSeconds(&rtime);
    printf("cloud time seconds %d %d\r\n", rmt_tmp, rmt_tmp + tmz * 60);
    // rmt_tm_sec += tmz*60;
    // parse dst
    uint32_t rmt_tm_sec = read_timezone(rmt_tmp);
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

    if ((rtime.tm_year != curr_time.tm_year) || (rtime.tm_mon != curr_time.tm_mon) || (rtime.tm_mday != curr_time.tm_mday) || (rtime.tm_hour != curr_time.tm_hour) || (abs(rtime.tm_min - curr_time.tm_min) >= 1))
    {
        printf("Cloud current time > 1  minutes and set it \n");

        struct timeval stime = {0};
        stime.tv_sec = mktime(&rtime);

        settimeofday(&stime, NULL);
    }
    else
    {
        printf("Cloud current time < 1 minute, not set it\n");
    }
    return 0;
}
