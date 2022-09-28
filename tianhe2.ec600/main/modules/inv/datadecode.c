
#include "datadecode.h"
#include <stdio.h>
#include <string.h>
#include "data_process.h"
#include "device_data.h"

int write_log_enable = 0;
extern int total_rate_power;
extern int total_curt_power;
/*----------------------------------------------------------------------------*/
/*! \brief      This function transform the hex string to hex char
 *  \param      char *hex_str   [in],  The hex string.
 *  \param      char *data_buff [in],  The pointer points to hex char.
 *  \param      int len         [in],  the length of hex string.
 *  \return     rst_code                The result in the execution.
 *  \see
 *  \note
 */
/*----------------------------------------------------------------------------*/
int hexstr_to_charbuff(char *hex_str, char *data_buff, int len)
{
    int i = 0;
    unsigned char tmp_h = 0, tmp_l = 0;

    for (i = 0; i < len; i += 2)
    {
        tmp_h = hex_str[i];
        if (tmp_h >= 'a' && tmp_h <= 'f')
            tmp_h = tmp_h - 'a' + 10;
        else if (tmp_h >= 'A' && tmp_h <= 'F')
            tmp_h = tmp_h - 'A' + 10;
        else if (tmp_h >= '0' && tmp_h <= '9')
            tmp_h = tmp_h - '0';
        tmp_h = (tmp_h << 4);

        tmp_l = hex_str[i + 1];
        if (tmp_l >= 'a' && tmp_l <= 'f')
            tmp_l = tmp_l - 'a' + 10;
        else if (tmp_l >= 'A' && tmp_l <= 'F')
            tmp_l = tmp_l - 'A' + 10;
        if (tmp_l >= '0' && tmp_l <= '9')
            tmp_l = tmp_l - '0';
        data_buff[i / 2] = tmp_h | tmp_l;
    }
    return i / 2;
}

int trim_space(char *str, int size)
{
    char *head = NULL;
    char *tail = NULL;
    char *new_str = calloc(size, 1);

    //first non space char
    for (int j = 0; j < size; j++)
    {
        if (str[j] != ' ')
        {
            head = &str[j];
            break;
        }
    }

    //last non space char
    for (int j = size - 1; j >= 0; j--)
    {
        if (str[j] != ' ' && str[j] != '\0')
        {
            tail = &str[j];
            break;
        }
    }

    if (head == NULL || tail == NULL)
        return -1;
    int new_size = tail + 1 - head;
    if (new_str == NULL)
        return -1;
    memcpy(new_str, head, new_size);
    memset(str, 0, size);
    memcpy(str, new_str, new_size);
    free(new_str);
    return 0;
}

/*----------------------------------------------------------------------------*/
/*! \brief  This function decode the received buffer for register read ID_Info command.
 *  \param  packet_buffer   [in],the inverter's SN pointer.
 *  \param  len             [in],the buffer's length.
 *  \return int32_t         the decode result.
 *  \see
 *  \note   here must adding the inverter SN, because of there is no SN ,just has modbus address in default
 */
/*----------------------------------------------------------------------------*/
extern InvRegister g_inv_info;
int32_t md_decode_inv_Info(const uint8_t *data, InvRegister *inv_ptr)
{
    printf("start to decode the inverter info!!\r\n");

    //tyep (Byte 00~01)
    inv_ptr->type = (uint_8)data[1];
    if (inv_ptr->type != 0x31 && inv_ptr->type != 0x33)
    {
        inv_ptr->type = 0x33;
    }

    //modbus address (Byte 02~03)
    inv_ptr->modbus_id = (uint_8)data[3];

    //Serial Number (Byte 4~35)
    memset(inv_ptr->sn, 0, sizeof(inv_ptr->sn));
    memcpy(inv_ptr->sn, data + 4, 32);
    inv_ptr->sn[32] = '\0';
    trim_space(inv_ptr->sn, sizeof(inv_ptr->sn));
    //printf("****$$$$get inv sn:%d--%s\r\n",strlen(inv_ptr->device.sn),inv_ptr->device.sn);
    //mode name (Byte 36 ~ 51)
    memcpy(inv_ptr->mode_name, data + 36, 16);
    inv_ptr->mode_name[16] = '\0';
    printf("type:%02X,add:%d,sn:%s,mode_name:%s\r\n", inv_ptr->type, inv_ptr->modbus_id, inv_ptr->sn, inv_ptr->mode_name);

    //safety type(Byte 52 ~ 53)
    inv_ptr->safety_type = data[53];

    //VA rating (Byte 54~57)
    inv_ptr->rated_pwr = (uint_32)((data[54] << 24) | (data[55] << 16) | (data[56] << 8) | (data[57]));
    printf("type:%d, add:%d, sn:%s, mode_name:%s, safety_type:%d, rated_pwr:%d; \r\n",
           inv_ptr->type, inv_ptr->modbus_id, inv_ptr->sn, inv_ptr->mode_name,
           inv_ptr->safety_type, inv_ptr->rated_pwr);

    //inverter master Firmware Ver (Byte 58~71)
    memset(inv_ptr->msw_ver, 0, sizeof(inv_ptr->msw_ver));
    memcpy(inv_ptr->msw_ver, data + 58, 14);
    inv_ptr->msw_ver[14] = '\0';
    trim_space(inv_ptr->msw_ver, sizeof(inv_ptr->msw_ver));

    //inverter slave Firmware ver (Byte 72~85)
    memcpy(inv_ptr->ssw_ver, data + 72, 14);
    inv_ptr->ssw_ver[14] = '\0';

    //inverter safety Firmware Ver (Byte 86~99)
    memset(inv_ptr->tmc_ver, 0, sizeof(inv_ptr->tmc_ver));
    memcpy(inv_ptr->tmc_ver, data + 86, 14);
    inv_ptr->tmc_ver[14] = '\0';
    trim_space(inv_ptr->tmc_ver, sizeof(inv_ptr->tmc_ver));

    //protocol version             (Byte 100~111)
    memcpy(inv_ptr->protocol_ver, data + 100, 12);
    inv_ptr->protocol_ver[12] = '\0';

    //Manufacturer (Byte 112~127)
    memcpy(inv_ptr->mfr, data + 112, 16);
    inv_ptr->mfr[16] = '\0';
    //Manufacturer (Byte 128~143)
    memcpy(inv_ptr->brand, data + 128, 16);
    inv_ptr->brand[16] = '\0';

    printf("msw_ver:%s, ssw_ver:%s, tmc_ver:%s, protocol_ver:%s, mfr:%s, brand:%s; \r\n",
           inv_ptr->msw_ver, inv_ptr->ssw_ver, inv_ptr->tmc_ver, inv_ptr->protocol_ver,
           inv_ptr->mfr, inv_ptr->brand);

    //the inverter type 1-单相机1-3kw   2-单相机3-6kw (Byte 144~145)
    inv_ptr->mach_type = data[145];

    //the inverter pv number (Byte 146~147)
    inv_ptr->pv_num = data[147];

    //the inverter string number (Byte 148~149)
    inv_ptr->str_num = data[149];

    /* ---------repetition---------------*/
    //        inv_ptr->status |= INV_SYNC_DEV_INDEX;
    //        inv_ptr->status |= INV_ONLINE_STATUS_INDEX;
    printf("mach_type:%d, pv_num:%d, str_NUM:%d\r\n", inv_ptr->mach_type, inv_ptr->pv_num, inv_ptr->str_num);
    memset(&g_inv_info, 0, sizeof(InvRegister));
    memcpy(&g_inv_info, inv_ptr, sizeof(InvRegister));

    memset(g_inv_ma_ver, 0, sizeof(g_inv_ma_ver));
    memset(g_inv_ma_ver, 0, sizeof(g_inv_sf_ver));
    shorten_ver(g_inv_info.msw_ver, g_inv_ma_ver, "MA");
    shorten_ver(g_inv_info.tmc_ver, g_inv_sf_ver, "SF");

    return 1;
}

/*----------------------------------------------------------------------------*/
/*! \brief  This function decode the chanel data.
 *  \param  packet         [in], The received comm paket.
 *  \param  channel_table  [in],The real data decode table.
 *  \param  inv_data_ptr   [out],The real data decode table.
 *  \return none.
 *  \see
 *  \note
 */
/*----------------------------------------------------------------------------*/
//#define PRT_DATA
extern uint16_t g_pwr_rate;
int md_decode_inv_data_content(uint8_t *packet, Inv_data *inv_data_ptr)
{
    inv_data_ptr->grid_voltg = ((uint8_t)packet[0] << 8) | (uint8_t)packet[1];                                                          /// the grid voltage
    inv_data_ptr->e_today = ((uint8_t)packet[4] << 24) | (uint8_t)packet[5] << 16 | ((uint8_t)packet[6] << 8) | (uint8_t)packet[7];     ///< The accumulated kWh of that day
    inv_data_ptr->e_total = ((uint8_t)packet[8] << 24) | (uint8_t)packet[9] << 16 | ((uint8_t)packet[10] << 8) | (uint8_t)packet[11];   ///< Total energy to grid
    inv_data_ptr->h_total = ((uint8_t)packet[12] << 24) | (uint8_t)packet[13] << 16 | ((uint8_t)packet[14] << 8) | (uint8_t)packet[15]; ///< Total operation hours
    inv_data_ptr->status = (uint8_t)packet[17];                                                                                         ///< Inverter operation status
    inv_data_ptr->pac_out_percent = g_pwr_rate;                                                                                         //((uint8_t)packet[18] << 8) | (uint8_t)packet[19];                                                           ///< the check time of inverter
    inv_data_ptr->col_temp = (packet[20] << 8) | packet[21];                                                                            ///< the cooling fin temperature
    inv_data_ptr->U_temp = (sint_16)(((uint8_t)packet[22] << 8) | (uint8_t)packet[23]);                                                 ///< the inverter U phase temperature
    inv_data_ptr->V_temp = (sint_16)(((uint8_t)packet[24] << 8) | (uint8_t)packet[25]);                                                 ///< the inverter V phase temperature
    inv_data_ptr->W_temp = (sint_16)(((uint8_t)packet[26] << 8) | (uint8_t)packet[27]);                                                 ///< the inverter W phase temperature
    inv_data_ptr->contrl_temp = (sint_16)(((uint8_t)packet[28] << 8) | (uint8_t)packet[29]);                                            ///< the inverter boost temperature
    inv_data_ptr->rsv_temp = (sint_16)(((uint8_t)packet[30] << 8) | (uint8_t)packet[31]);                                               ///< the reserve temperature
    inv_data_ptr->bus_voltg = ((uint8_t)packet[32] << 8) | (uint8_t)packet[33];                                                         ///< bus voltage
    inv_data_ptr->con_stu = ((uint8_t)packet[34] << 8) | (uint8_t)packet[35];                                                           ///< conntect status
#ifdef PRT_DATA
    printf("\r\ngrid_voltg:%d,e_today:%d,e_total:%d,\n"
           "h_total:%d,status:%d,pac_out_percent:%d,\n"
           "col_temp:%d,U_temp:%d,V_temp:%d,\n"
           "W_temp:%d,contrl_temp:%d,rsv_temp:%d,\n"
           "bus_voltg:%d,con_stu:%d\r\n",
           inv_data_ptr->grid_voltg, inv_data_ptr->e_today, inv_data_ptr->e_total,
           inv_data_ptr->h_total, inv_data_ptr->status, inv_data_ptr->pac_out_percent,
           inv_data_ptr->col_temp, inv_data_ptr->U_temp, inv_data_ptr->V_temp,
           inv_data_ptr->W_temp, inv_data_ptr->contrl_temp, inv_data_ptr->rsv_temp,
           inv_data_ptr->bus_voltg, inv_data_ptr->con_stu);
#endif
    //printf();
    /*10 mppt voltage and current*/
    write_log_enable = 0;
    for (int i = 0; i < 10; i++)
    {
        inv_data_ptr->PV_cur_voltg[i].iVol = ((uint8_t)packet[36 + 4 * i] << 8) | (uint8_t)packet[36 + 1 + 4 * i];
        inv_data_ptr->PV_cur_voltg[i].iCur = ((uint8_t)packet[36 + 2 + 4 * i] << 8) | (uint8_t)packet[36 + 3 + 4 * i];

        if (inv_data_ptr->PV_cur_voltg[i].iVol > 80 && inv_data_ptr->PV_cur_voltg[i].iVol < 65535)
            write_log_enable = 1;

#ifdef PRT_DATA
        printf("PV_cur_voltg[%d].iVol:%d,PV_cur_voltg[%d].iCur:%d\r\n",
               i, inv_data_ptr->PV_cur_voltg[i].iVol, i, inv_data_ptr->PV_cur_voltg[i].iCur);
#endif
    }
    /*20 smart string currents*/
    for (int j = 0; j < 20; j++)
    {
#ifdef PRT_DATA
        if ((j % 4) == 0)
        {
            printf("\r\n");
        }
#endif
        inv_data_ptr->istr[j] = ((uint8_t)packet[76 + 2 * j] << 8) | (uint8_t)packet[76 + 1 + 2 * j]; ///<string current 1~20
        //printf("string[%d]:%d ,", j, inv_data_ptr->istr[j]);
    }
#ifdef PRT_DATA
    printf("\r\n");
#endif
    /* AC voltage and current*/
    for (int k = 0; k < 3; k++)
    {
        inv_data_ptr->AC_vol_cur[k].iVol = ((uint8_t)packet[116 + 4 * k] << 8) | (uint8_t)packet[116 + 1 + 4 * k];
        inv_data_ptr->AC_vol_cur[k].iCur = ((uint8_t)packet[116 + 2 + 4 * k] << 8) | (uint8_t)packet[116 + 3 + 4 * k];
#ifdef PRT_DATA
        printf("AC_vol_cur[%d].iVol:%d ,AC_vol_cur[%d].iCur:%d ,",
               k, inv_data_ptr->AC_vol_cur[k].iVol, k, inv_data_ptr->AC_vol_cur[k].iCur);
#endif
    }
    /*相电压*/
    inv_data_ptr->rs_voltg = ((uint8_t)packet[128] << 8) | (uint8_t)packet[129];
    inv_data_ptr->rt_voltg = ((uint8_t)packet[130] << 8) | (uint8_t)packet[131];
    inv_data_ptr->st_voltg = ((uint8_t)packet[132] << 8) | (uint8_t)packet[133];

    inv_data_ptr->fac = ((uint8_t)packet[134] << 8) | (uint8_t)packet[135];                                                             ///< AC frequence
    inv_data_ptr->sac = ((uint8_t)packet[136] << 24) | (uint8_t)packet[137] << 16 | ((uint8_t)packet[138] << 8) | (uint8_t)packet[139]; ///< Total power to grid(3Phase)
    inv_data_ptr->pac = ((uint8_t)packet[140] << 24) | (uint8_t)packet[141] << 16 | ((uint8_t)packet[142] << 8) | (uint8_t)packet[143]; ///< Total power to grid(3Phase)
    inv_data_ptr->qac = ((uint8_t)packet[144] << 24) | (uint8_t)packet[145] << 16 | ((uint8_t)packet[146] << 8) | (uint8_t)packet[147]; ///< Total power to grid(3Phase)
    inv_data_ptr->cosphi = (sint_8)packet[149];                                                                                         ///< the AC phase
    inv_data_ptr->pac_stu = (uint8_t)packet[151];                                                                                       ///< limit power status
    inv_data_ptr->err_stu = (uint8_t)packet[153];                                                                                       ///< error status
    uint16_t var = 0;
    var = ((uint8_t)packet[154] << 8) | (uint8_t)packet[155];
    if (var == 0xFFFF)
        var = 0;
    inv_data_ptr->error = var;
#ifdef PRT_DATA
    printf("\nrs_voltg:%d,rt_voltg:%d,st_voltg:%d,\n"
           "fac:%d,sac:%d,pac:%d,qac:%d,cosphi:%d,\n"
           "pac_stu:%d,err_stus:%d,error:%d\n",
           inv_data_ptr->rs_voltg, inv_data_ptr->rt_voltg, inv_data_ptr->st_voltg,
           inv_data_ptr->fac, inv_data_ptr->sac, inv_data_ptr->pac, inv_data_ptr->qac, inv_data_ptr->cosphi,
           inv_data_ptr->pac_stu, inv_data_ptr->err_stu, inv_data_ptr->error);
#endif

    /*10 warns*/
    for (int h = 0; h < 10; h++)
    {
        var = (uint8_t)packet[156 + 1 + 2 * h];
        if (var == 0xFF)
            var = 0;
        inv_data_ptr->warn[h] = var; ///< Failure description for status 'failure'
#ifdef PRT_DATA
        printf("warn[%d]:%d ", h, inv_data_ptr->warn[h]);
#endif
    }

    if (inv_data_ptr->fac > 30000 || inv_data_ptr->e_today > 1000 * 24 * 10) //1000kw*24h*10
    {
        return -1;
    }
    return 0;
}

void Check_upload_sint16(const char *field, sint_16 value, char *body, uint_16 *datalen)
{
    uint_16 len = (*datalen);
    if (value != 0xFFFF)
    {
        (*datalen) = len + sprintf(body + len, field, value);
    }
}
void Check_upload_unit32(const char *field, uint_8 value, char *body, uint_16 *datalen)
{
    uint_16 len = (*datalen);
    if (value != 0xFFFFFFFF)
    {
        (*datalen) = len + sprintf(body + len, field, value);
    }
}

// int main()
// {

//     uint8_t *infobuf = "0033000343413030323030303132303430303331000000000000000000000000000000002020202041535732304B2D4C54202020002300004E20563631302D30333033332D303020563631302D36303030382D303020563631302D31303030302D3030200000000000000000000000004149535745492020202020202020202041495357454920202020202020202020000400020004";
//     uint8_t infodata[300] = {0};

//     uint8_t *databuf = "089813880000006D000018B40000005D0001000001E901C501C701C601C27FFF18C5000012E20034132E0033FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0018001C001A0018FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF08B9004809130048092B00490F780F990F8013880000134C0000134C000000000064000000000000FF00000000000000000000000000000000000000";
//     uint8_t datadata[300] = {0};

//     hexstr_to_charbuff(infobuf, infodata, strlen(infobuf));

//     Inverter inv = {0};

//     md_decode_inv_Info(infodata, &inv);

//     printf("\r\n-------------------------------------\r\n");

//     hexstr_to_charbuff(databuf, datadata, strlen(databuf));

//     md_decode_inv_data_content(datadata, &inv.data);

//     char invdata[500] = {0};

//     char invinfo[500] = {0};

//     int i = 0;

//     int infolen = sprintf(invinfo, "{\"Action\":\"SyncInvInfo\",\"psn\":\"%s\",\"typ\":\"%d\",\"sn\":\"%s\","
//                                    "\"mod\":\"%s\",\"sft\":\"%d\",\"rtp\":\"%d\","
//                                    "\"mst\":\"%s\",\"slv\":\"%s\",\"sfv\":\"%s\","
//                                    "\"cmv\":\"%s\",\"muf\":\" %s\",\"brd\":\"%s\","
//                                    "\"mty\":\"%d\",\"MPT\":\"%d\",\"str\":\"%d\",\"Imc\":\"%d\"}",
//                           "B300019C0111", inv.device.type, inv.device.sn,
//                           inv.device.mode_name, inv.device.safety_type, inv.device.rated_pwr,
//                           inv.device.msw_ver, inv.device.ssw_ver, inv.device.tmc_ver,
//                           inv.device.protocol_ver, inv.device.mfr, inv.device.brand,
//                           inv.device.mach_type, inv.device.pv_num, inv.device.str_num, inv.device.md_add);

//     uint_16 datlen = sprintf(invdata, "{\"Action\":\"UpdateInvData\",\"psn\":\"%s\",\"sn\":\"%s\","
//                                       "\"smp\":5,\"tim\":\"%04d-%02d-%02d %02d:%02d:%02d\","
//                                       "\"fac\":%d,\"pac\":%d,\"er\":%d,"
//                                       "\"cos\":%d,\"tem\":%d,\"eto\":%d,\"etd\":%d,\"hto\":%d,",
//                              "B300019C0111", inv.device.sn,
//                              2020, 05, 26, 16, 01, 25,
//                              inv.data.fac, inv.data.pac, 0,
//                              inv.data.cosphi, inv.data.col_temp, inv.data.e_total, inv.data.e_today, inv.data.h_total);
//     /* check 1~10 MPPT Parameters*/
//     for (i = 0; i < 10; i++)
//     {
//         if ((inv.data.PV_cur_voltg[i].iVol != 0xFFFF) && (inv.data.PV_cur_voltg[i].iCur != 0xFFFF))
//         {
//             datlen += sprintf(invdata + datlen, "\"v%d\":%d,\"i%d\":%d,",
//                               i + 1, inv.data.PV_cur_voltg[i].iVol,
//                               i + 1, inv.data.PV_cur_voltg[i].iCur);
//         }
//     }
//     /* check 1~3 ac parameters*/
//     for (i = 0; i < 3; i++)
//     {
//         if ((inv.data.AC_vol_cur[i].iVol != 0xFFFF) && (inv.data.AC_vol_cur[i].iCur != 0xFFFF))
//         {
//             datlen += sprintf(invdata + datlen, "\"va%d\":%d,\"ia%d\":%d,",
//                               i + 1, inv.data.AC_vol_cur[i].iVol,
//                               i + 1, inv.data.AC_vol_cur[i].iCur);
//         }
//     }
//     /* check 1~20 string current parameters*/
//     for (i = 0; i < 20; i++)
//     {
//         if ((inv.data.istr[i] != 0xFFFF))
//         {

//             datlen += sprintf(invdata + datlen, "\"s%d\":%d,", i + 1, inv.data.istr[i]);
//         }
//     }
//     //Check_upload_unit16
//     Check_upload_unit32("\"prc\":%d,", inv.data.qac, invdata, &datlen);
//     Check_upload_unit32("\"sac\":%d,", inv.data.sac, invdata, &datlen);
//     Check_upload_sint16("\"tu\":%d,", inv.data.U_temp, invdata, &datlen);
//     Check_upload_sint16("\"tv\":%d,", inv.data.V_temp, invdata, &datlen);
//     Check_upload_sint16("\"tw\":%d,", inv.data.W_temp, invdata, &datlen);
//     Check_upload_sint16("\"cb\":%d,", inv.data.contrl_temp, invdata, &datlen);
//     Check_upload_sint16("\"bv\":%d,", inv.data.bus_voltg, invdata, &datlen);
//     /*check 1~10 warn*/
//     for (i = 0; i < 10; i++)
//     {
//         if (inv.data.warn[i] != 0xFF)
//         {
//             datlen += sprintf(invdata + datlen, "\"wn%d\":%d,", i + 1, inv.data.warn[i]);
//         }
//     }
//     /* itv field as the end of the body*/
//     datlen += sprintf(invdata + datlen, "\"itv\":%d}", 456);

//     printf("\r\ninvdata:%s\r\n", invdata);

//     printf("\r\ninvinfo:%s\r\n", invinfo);

//     return 0;
// }