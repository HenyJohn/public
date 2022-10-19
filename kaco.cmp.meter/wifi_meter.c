#include "data_process.h"
#include "device_data.h"
#include <stdio.h>
#include <string.h>
#include "asw_store.h"
#include "meter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "mqueue.h"

extern Inv_arr_t inv_arr;
extern int inv_real_num;
//extern Inv_arr_t cgi_inv_arr;
extern Inv_arr_t inv_dev_info;

int total_rate_power = 0;
int total_curt_power = 0;
int sync_meter_config = 0;
unsigned short pwr_curr_per = 10000;
short meter_real_factory = 0;

meter_setdata mtconfig = {0};
Inverter inv_meter = {0};
static float iee754_to_float(uint_32 stored_data);
int writeRegulatePower_fast(short adjust_num);
int writeReative_factory(short fac);
int lost_meter = 0;

int write_meter_file(meter_setdata *mtconfigx)
{
    //printf("enb:%d, mod:%d, regulate:%d, target:%d, date:%d \n", mtconfig.enb, mtconfig.mod, mtconfig.reg, mtconfig.target, mtconfig.date);

    general_add(NVS_METER_SET, (void *)mtconfigx);
    return 0;
}

int read_md_meter_pack(uint_8 salveaddr, uint_8 funcode, uint_16 regaddr, uint_16 count, uint_8 *packet)
{
    uint16_t check_crc = 0xffff;
    packet[0] = salveaddr,
    packet[1] = funcode;
    packet[2] = regaddr >> 8;
    packet[3] = regaddr & 0x00ff;
    packet[4] = count >> 8;
    packet[5] = count & 0x00ff;
    uint_8 len = 6;
    check_crc = crc16_calc(packet, len);
    packet[len++] = (uint8_t)(check_crc & 0xff);        // check sum high
    packet[len++] = (uint8_t)((check_crc >> 8) & 0xff); // check sum low
    return len;
}

int md_decode_meter_pack_abs(uint_8 metmod, uint_8 salveaddr, uint_8 funcode, uint_8 len, uint_8 *buffer, char type, uint32_t *pac)
{
    static unsigned int meter_data_time = 0;

    if (buffer[0] != salveaddr)
    {
        printf("get meter data addr err, address=%d\r\n", buffer[0]);
        return -1;
    }
    if (buffer[1] != funcode)
    {
        printf("get meter data function code err, func=%d\r\n", buffer[1]);
        return -1;
    }

    if (crc16_calc(buffer, len) != 0)
    {
        printf("get meter data crc err\r\n");
        return -1;
    }
    uint_16 dest[8] = {0};

    if (1 == type && len == 17)
    {
        dest[0] = (buffer[3] << 8) | buffer[4]; //p1
        dest[1] = (buffer[5] << 8) | buffer[6];

        dest[2] = (buffer[7] << 8) | buffer[8]; //p2
        dest[3] = (buffer[9] << 8) | buffer[10];

        dest[4] = (buffer[11] << 8) | buffer[12]; //p3
        dest[5] = (buffer[13] << 8) | buffer[14];
    }

    uint_16 start_addr = 0x0014;
    sint_32 tmp = 0;
    float f_tmp[3] = {0.0};
    float ph_min = 0.0;

    for (int i = 0, j = 0; i < 6;)
    {
        f_tmp[j++] = iee754_to_float(dest[i] << 16 | dest[i + 1]);
        printf("pac %d-------------PW %f\r\n", j, f_tmp[j - 1]);
        i += 2;
    }

    if (f_tmp[0] > f_tmp[1])
        ph_min = f_tmp[1];
    else
        ph_min = f_tmp[0];

    if (ph_min > f_tmp[2])
        ph_min = f_tmp[2];

    printf("pac min-------------PWL %f\r\n", ph_min);
    //inv_meter.invdata.pac = (sint_32)(ph_min*3.0);
    *pac = (sint_32)(ph_min * 3.0);
    printf("pac%f-dest%d- \r\n", ph_min * 3.0, *pac);
    return 0;
}

int md_decode_meter_pack(uint_8 metmod, uint_8 salveaddr, uint_8 funcode, uint_8 len, uint_8 *buffer, char type)
{
    static unsigned int meter_data_time = 0;
    struct timeval tv;

    if (buffer[0] != salveaddr)
    {
        printf("get meter data addr err, address=%d\r\n", buffer[0]);
        return -1;
    }
    if (buffer[1] != funcode)
    {
        printf("get meter data function code err, func=%d\r\n", buffer[1]);
        return -1;
    }
    if (!type && buffer[2] != 140)
    {
        printf("get meter data data length err, data length=%d\r\n", buffer[2]);
        return -1;
    }
    if (!type && len != 145)
    {
        printf("get meter data data length err, data length=%d\r\n", len);
        return -1;
    }
    if (crc16_calc(buffer, len) != 0)
    {
        printf("get meter data crc err\r\n");
        return -1;
    }
    uint_16 dest[70] = {0};
    uint_16 temp, i;
    /*format two uint8 bytes to one uint16 byte*/
    //printf("\r\n");
    if (!type && len > 0)
    {
        for (i = 0; i < 70; i++)
        {
            /* shift reg hi_byte to temp */
            temp = buffer[3 + i * 2] << 8;
            /* OR with lo_byte           */
            temp = temp | buffer[4 + i * 2];

            dest[i] = temp;
            //printf(" %02x",dest[i]);
        }
    }
    else if (1 == type && len == 9)
    {
        dest[0] = (buffer[3] << 8) | buffer[4];
        dest[1] = (buffer[5] << 8) | buffer[6];
    }

    uint_16 start_addr = 0x0014;
    sint_32 tmp = 0;
    float f_tmp = 0.0;
    static uint_8 last_day = 0;

    /* Total system power:2 registers */
    tmp = (dest[0] << 16) + dest[1];
    //printf("dest[0]:%02x,dest[1]:%02x,tmp:%02x\r\n",dest[0],dest[1],tmp);
    f_tmp = iee754_to_float(tmp);
    printf("pac-------------%f\r\n", f_tmp);
    inv_meter.invdata.pac = (sint_32)f_tmp;

    if (type)
    {
        printf("pac--###%d###--- \r\n", inv_meter.invdata.pac);
        return 0;
    }

    switch (metmod)
    {
    case 0: //Eastron 630 DC
    case 1: //Eastron 630 CT
    {
        /* Total system Apparent power */
        tmp = (dest[4] << 16) + dest[5];
        f_tmp = iee754_to_float(tmp);
        inv_meter.invdata.sac = (sint_32)f_tmp;

        /* Total system Reactive power  */
        tmp = (dest[8] << 16) + dest[9];
        f_tmp = iee754_to_float(tmp);
        inv_meter.invdata.qac = (sint_32)f_tmp;

        /* Total system Power factor    */
        tmp = (dest[10] << 16) + dest[11];
        //printf("factor:%08X\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        printf("factord %d\r\n", (sint_32)(f_tmp * 100), (short)(sint_32)(f_tmp * 10000));
        meter_real_factory = (short)(sint_32)(f_tmp * 10000);
        inv_meter.invdata.cosphi = (sint_8)(sint_32)(f_tmp * 100);

        /* Total system Power Phase angle*/
        tmp = (dest[14] << 16) + dest[15];
        // tmp= dest[14] ;
        //                printf("Phase angle:%08X\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        //                printf("Phase angle2:%d\r\n", (sint_16)(sint_32)(f_tmp*1000));
        inv_meter.invdata.bus_voltg = (sint_16)(sint_32)(f_tmp * 1000);

        start_addr = 0x0012;
        /*FAC*/
        tmp = (dest[start_addr] << 16) + dest[start_addr + 1];
        f_tmp = iee754_to_float(tmp);
        inv_meter.invdata.fac = (f_tmp * 100);
        printf("freq:%f\r\n", f_tmp);

        /* Total Input Energy */
        tmp = (dest[start_addr + 2] << 16) + dest[start_addr + 3];
        f_tmp = iee754_to_float(tmp);
        //meter_data->e_total_in = (sint_32)(f_tmp*10);
        inv_meter.invdata.e_total = (sint_32)(f_tmp * 10); //meter e_total(in)

        /* Total Output Energy */
        tmp = (dest[start_addr + 4] << 16) + dest[start_addr + 5];
        f_tmp = iee754_to_float(tmp);
        //meter_data->e_total_out = (sint_32)(f_tmp*10);
        inv_meter.invdata.h_total = (sint_32)(f_tmp * 10); ////meter E-Total( out) (old msg)
    }
    break;
    case 2: //Eastron 230
    case 3: //Eastron 220
    case 4: //Eastron 120
    {
        /* Total system Apparent power */
        tmp = (dest[6] << 16) + dest[7];
        f_tmp = iee754_to_float(tmp);
        inv_meter.invdata.sac = (sint_32)f_tmp;

        /* Total system Reactive power  */
        tmp = (dest[12] << 16) + dest[13];
        f_tmp = iee754_to_float(tmp);
        inv_meter.invdata.qac = (sint_32)f_tmp;

        /* Total system Power factor    */
        tmp = (dest[18] << 16) + dest[19];
        //  tmp=  dest[10] ;
        //                printf("factor:%d\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        //                printf("factor21123:%d\r\n",(sint_32)(f_tmp*100));
        printf("factor%d %d\r\n", (sint_32)(f_tmp * 100), (short)(sint_32)(f_tmp * 10000));
        meter_real_factory = (short)(sint_32)(f_tmp * 10000);
        inv_meter.invdata.cosphi = (sint_8)(sint_32)(f_tmp * 100);

        /* Total system Power Phase angle*/
        tmp = (dest[24] << 16) + dest[25];
        // tmp= dest[14] ;
        //                printf("Phase angle:%d\r\n",tmp);
        f_tmp = iee754_to_float(tmp);
        //                printf("Phase angle2:%d\r\n", (sint_16)(sint_32)(f_tmp*1000));
        inv_meter.invdata.bus_voltg = (sint_16)(sint_32)(f_tmp * 1000);

        start_addr = 0x003A;
        //FAC
        tmp = (dest[start_addr] << 16) + dest[start_addr + 1];
        f_tmp = iee754_to_float(tmp);
        inv_meter.invdata.fac = (f_tmp * 100);

        /* Total Input Energy */
        tmp = (dest[start_addr + 2] << 16) + dest[start_addr + 3];
        f_tmp = iee754_to_float(tmp);
        //meter_data->e_total_in = (sint_32)(f_tmp*10);
        inv_meter.invdata.e_total = (sint_32)(f_tmp * 10); //meter e_total(in)

        /* Total Output Energy */
        tmp = (dest[start_addr + 4] << 16) + dest[start_addr + 5];
        f_tmp = iee754_to_float(tmp);
        //meter_data->e_total_out = (sint_32)(f_tmp*10);
        inv_meter.invdata.h_total = (sint_32)(f_tmp * 10); //meter E-Total( out) (old msg)
    }
    break;
    default:
        break;
    }
    //FAC
    tmp = (dest[start_addr] << 16) + dest[start_addr + 1];
    f_tmp = iee754_to_float(tmp);
    inv_meter.invdata.fac = (f_tmp * 100);

    /* Total Input Energy */
    tmp = (dest[start_addr + 2] << 16) + dest[start_addr + 3];
    f_tmp = iee754_to_float(tmp);
    //meter_data->e_total_in = (sint_32)(f_tmp*10);
    inv_meter.invdata.e_total = (sint_32)(f_tmp * 10); //meter e_total(in)

    /* Total Output Energy */
    tmp = (dest[start_addr + 4] << 16) + dest[start_addr + 5];
    f_tmp = iee754_to_float(tmp);
    //meter_data->e_total_out = (sint_32)(f_tmp*10);
    inv_meter.invdata.h_total = (sint_32)(f_tmp * 10); //meter E-Total( out) (old msg)

    printf("powerout:%d \n", inv_meter.invdata.pac);
    printf("factory :%d \n", inv_meter.invdata.fac);
    printf("cosphi  :%d \n", inv_meter.invdata.cosphi);

    //       /* calc E_today I*/
    //rtc_time_s_t curr_date;
    //GetTime_s(&curr_date);

    meter_gendata gendata = {0};
    general_query(NVS_METER_GEN, (void *)&gendata);
    if (!last_day)
    {

        if (get_current_days() != gendata.day)
        {
            // save_days();
            // save_meter_etotal(inv_meter.invdata.e_total);
            // save_meter_htotal(inv_meter.invdata.h_total);
            memset(&gendata, 0, sizeof(meter_gendata));
            gendata.e_total = inv_meter.invdata.e_total;
            gendata.h_total = inv_meter.invdata.h_total;
            gendata.day = get_current_days();
            general_add(NVS_METER_GEN, (void *)&gendata);
            inv_meter.fir_etotal = inv_meter.invdata.e_total;
            inv_meter.fir_etotal_out = inv_meter.invdata.h_total;
        }
        else if (get_current_days() == gendata.day)
        {
            inv_meter.fir_etotal = gendata.e_total;
            inv_meter.fir_etotal_out = gendata.h_total;
        }
        last_day = get_current_days();
    }
    else
    {
        if (get_current_days() != gendata.day)
        {
            // save_days();
            // save_meter_etotal(inv_meter.invdata.e_total);
            // save_meter_htotal(inv_meter.invdata.h_total);
            memset(&gendata, 0, sizeof(meter_gendata));
            gendata.e_total = inv_meter.invdata.e_total;
            gendata.h_total = inv_meter.invdata.h_total;
            gendata.day = get_current_days();
            general_add(NVS_METER_GEN, (void *)&gendata);

            inv_meter.fir_etotal = inv_meter.invdata.e_total;
            inv_meter.fir_etotal_out = inv_meter.invdata.h_total;
            last_day = get_current_days();
        }
    }

    inv_meter.invdata.con_stu = (inv_meter.invdata.e_total - inv_meter.fir_etotal) * 10;
    inv_meter.invdata.e_today = (inv_meter.invdata.h_total - inv_meter.fir_etotal_out) * 10;

    gettimeofday(&tv, NULL);

    if (get_second_sys_time() > 180 && (tv.tv_sec - meter_data_time >= 300 || tv.tv_sec - meter_data_time < 0))
    {
        //if (period >= SAVE_TIMING || period < 0)
        //if(strlen(inv_data.psn)>10 && get_work_mode()==0 && get_second_sys_time() > 180)
        printf("save MMMM--------------------- %d\r\n", tv.tv_sec - meter_data_time);
        meter_data_time = tv.tv_sec; //get_second_sys_time();
    }
    else
        return 0;

    //printf("meter period--------------- %d %d \n", period, meter_msg_id);
    //send msg
    meter_cloud_data trans_meter_cloud = {0};
    trans_meter_cloud.type = 1;
    memcpy(&trans_meter_cloud.data, &inv_meter.invdata, sizeof(Inv_data));

    if (mq2 != NULL)
    {
        xQueueSend(mq2, (void *)&trans_meter_cloud, (TickType_t)0);
        printf("mqueue send ok\n");
    }
    //msgsnd(meter_msg_id, &trans_meter_cloud, sizeof(trans_meter_cloud.data), IPC_NOWAIT);
    return 0;
}

/*----------------------------------------------------------------------------*/
/*! \brief   This function format iee754 data to float.
 *  \param   uint32_t stored_data : iee754 data
 *  \return  float,the result
 *  \note
 */
/*----------------------------------------------------------------------------*/
static float iee754_to_float(uint_32 stored_data)
{
    float f;
    char *pbinary = (char *)&stored_data;
    char *pf = (char *)&f;

    memcpy(pf, pbinary, sizeof(float));
    return f;
}

int md_query_meter_data(unsigned char meter_mode, unsigned char slaveadd, char type, char abs, uint32_t *pac_3ph)
{

    unsigned char funcode = 0x04;
    char buffer[200] = {0};
    unsigned short send_len = 0;
    unsigned short recv_len = 0;
    printf("===========%d------------%d>>>\n", abs, type);
    switch (meter_mode)
    {
    case 0: //EASTRON630
    case 1: //EASTRON630DC
    {
        if (type)
        {
            if (abs)
                send_len = read_md_meter_pack(slaveadd, funcode, 12, 6, buffer);
            else
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
        if (abs == 0)
            recv_len = 2 * 2;
        if (abs == 1)
            recv_len = 2 * 6;
    }
    else
        recv_len = 70 * 2;

    int res = recv_bytes_frame_meter(UART_NUM_1, buffer, &recv_len);

    if (res >= 0 && recv_len > 0)
    {
        if (type == 1 && abs == 1)
            if (0 == md_decode_meter_pack_abs(meter_mode, slaveadd, funcode, recv_len, buffer, 1, pac_3ph))
                return 0;

        if (0 == md_decode_meter_pack(meter_mode, slaveadd, funcode, recv_len, buffer, type))
            return 0;
        else
            printf("read meter error \n");
    }
    return -1;
}

char get_meter_info(char mod, char *meterbran, char *metername)
{
    switch (mod)
    {
    case 0:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON"));
        strncpy(metername, "SDM630CT", strlen("SDM630CT"));
    }
    break;
    case 1:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON"));
        strncpy(metername, "SDM630DC", strlen("SDM630DC"));
    }
    break;
    case 2:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON"));
        strncpy(metername, "SDM230", strlen("SDM230"));
    }
    break;
    case 3:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON"));
        strncpy(metername, "SDM220", strlen("SDM220"));
    }
    break;
    case 4:
    {
        strncpy(meterbran, "EASTRON", strlen("EASTRON"));
        strncpy(metername, "SDM120", strlen("SDM120"));
    }
    break;
    case 5:
    {
        strncpy(meterbran, "NORK", strlen("NORK"));
        strncpy(metername, "NORK", strlen("NORK"));
    }
    break;
    default:
        break;
    }
    return 0;
}

int clear_meter_setting(int msg)
{
    if (msg == 6 || msg == 60)
    {
        printf("meter sett clear\n");
        memset(&mtconfig, 0, sizeof(meter_setdata));
        sync_meter_config = 0;
    }
    return 0;
}

int query_meter_proc(void)
{
    int ret = -1;
    static int read_time = 0;
    static int pf_time = 0;
    uint32_t pwr_abs_val = 0;
    static int pwr_abs_offset = 0;

    if (!mtconfig.date)
    {
        // general_query(NVS_METER_SET, (void *)&mtconfig);
        // printf("read ---->enb:%d, mod:%d, regulate:%d, target:%d, date:%d \n", mtconfig.enb, mtconfig.mod, mtconfig.reg, mtconfig.target, mtconfig.date);
        if (0 == parse_sync_time() && sync_meter_config == 0)
        {
            general_query(NVS_METER_SET, (void *)&mtconfig);
            printf("read ---->enb:%d, mod:%d, regulate:%d, target:%d, date:%d pf:%d tpf:%d \n", mtconfig.enb, mtconfig.mod, mtconfig.reg, mtconfig.target, mtconfig.date, mtconfig.enb_pf, mtconfig.target_pf);
            pwr_abs_offset = 0;
            if (mtconfig.reg > 0x100 && mtconfig.target == 0)
                pwr_abs_offset = mtconfig.reg - 0x100;
            //  if(get_current_days() != mtconfig.date)
            //     mtconfig.reg=0;

            sync_meter_config = 1;
        }
    }

    if (mtconfig.enb == 1)
    {
        if (0x100 == (mtconfig.reg & 0XF00))
        {
            printf("(100 == mtconfig.reg) %d %d \n", mtconfig.reg, pwr_abs_offset);
            if (read_time >= 10)
            {
                ret = md_query_meter_data(mtconfig.mod, 1, 0, pwr_abs_offset > 0, &pwr_abs_val);
                // if(10 == mtconfig.enb_pf)
                //     writeReative_factory((short)mtconfig.target_pf);//inv_meter.invdata.cosphi)
                read_time = 0;
                pf_time = 15;
            }
            else
                ret = md_query_meter_data(mtconfig.mod, 1, 1, pwr_abs_offset > 0, &pwr_abs_val);

            if ((0 == ret) && (0x100 == (mtconfig.reg & 0XF00)))
            {
                // if(lost_meter == 1)
                // {
                //     lost_meter = 0;
                //     pwr_curr_per = 100;
                //     writeRegulatePower();
                //     usleep(500000);
                // }
                energy_meter_control(mtconfig.target, pwr_abs_offset, pwr_abs_val, 0);
            }

            if (ret < 0 || ret == 255)
            {
                printf("AS REGULATE--%d sfty--%d \n", total_rate_power, inv_arr[0].regInfo.safety_type);
                if (total_rate_power > 0 && (inv_arr[0].regInfo.safety_type > 73 && inv_arr[0].regInfo.safety_type < 78)) //74--77
                {
                    // pwr_curr_per = mtconfig.target*100.00 / total_rate_power;;
                    // if (pwr_curr_per > 100)
                    //     pwr_curr_per = 100;
                    // else if (pwr_curr_per <= 0)
                    //     pwr_curr_per = 0;

                    // writeRegulatePower();
                    // lost_meter = 1;
                    energy_meter_control(mtconfig.target, pwr_abs_offset, pwr_abs_val, 1);
                    sleep(10);
                }
            }
            // if(ret < 0)
            //     flush_serial_port();

            //read_time++;
        }
        else
        {
            if (read_time >= 10)
            {
                ret = md_query_meter_data(mtconfig.mod, 1, 0, 0, &pwr_abs_val);
                // if(10 == mtconfig.enb_pf)
                //     writeReative_factory((short)mtconfig.target_pf);
                read_time = 0;
                pf_time = 15;
            }
        }

        read_time++;

        pf_time++;
        if (pf_time >= 10)
        {
            if (10 == mtconfig.enb_pf)
            {
                printf("meter cosphi%d %d pftarget %d \n", inv_meter.invdata.cosphi, meter_real_factory, (short)mtconfig.target_pf);
                if (abs((short)mtconfig.target_pf + meter_real_factory) > 1 && (meter_real_factory < 50000))
                {
                    if ((short)mtconfig.target_pf > 0 && meter_real_factory > 0)
                        writeReative_factory((short)mtconfig.target_pf); //meter_real_factory);
                    if ((short)mtconfig.target_pf < 0 && meter_real_factory < 0)
                        writeReative_factory((short)mtconfig.target_pf); //meter_real_factory);

                    if (((short)mtconfig.target_pf + meter_real_factory) > 0)
                    {
                        printf("meter cosphi min\n");
                        if ((short)mtconfig.target_pf > 0)
                        {
                            if (((short)mtconfig.target_pf + meter_real_factory) > 99)
                                writeReative_factory(abs(meter_real_factory) + ((short)mtconfig.target_pf + meter_real_factory) / 2);
                            else if (((short)mtconfig.target_pf + meter_real_factory) < 100)
                                writeReative_factory(abs(meter_real_factory) + 10);
                        }
                        else
                        {
                            if (((short)mtconfig.target_pf + meter_real_factory) > 99)
                                writeReative_factory(0 - meter_real_factory + ((short)mtconfig.target_pf + meter_real_factory) / 2);
                            else if (((short)mtconfig.target_pf + meter_real_factory) < 20)
                                writeReative_factory(0 - meter_real_factory + 10);
                        }
                    }
                    else //if(inv_meter.invdata.cosphi >(short)mtconfig.target_pf/100)
                    {
                        printf("meter cosphi over\n");
                        if ((short)mtconfig.target_pf > 0)
                        {
                            if (((short)mtconfig.target_pf + meter_real_factory) < -99)
                                writeReative_factory(abs(meter_real_factory) + ((short)mtconfig.target_pf + meter_real_factory) / 2);
                            else if (((short)mtconfig.target_pf + meter_real_factory) > -100)
                                writeReative_factory(abs(meter_real_factory) - 10);
                        }
                        else
                        {
                            if (((short)mtconfig.target_pf + meter_real_factory) < -99)
                                writeReative_factory(0 - meter_real_factory + ((short)mtconfig.target_pf + meter_real_factory) / 2);
                            else if (((short)mtconfig.target_pf + meter_real_factory) > -100)
                                writeReative_factory(0 - meter_real_factory - 10);
                        }
                    }
                    meter_real_factory = 60000;
                }
            }
            pf_time = 0;
        }

        if (ret < 0)
            flush_serial_port();
    }

    // pf_time++;
    // if(pf_time >=10 )
    // {
    //     if(10 == mtconfig.enb_pf)
    //         {
    //             inv_meter.invdata.cosphi
    //             writeReative_factory((short)mtconfig.target_pf);
    //         }
    //     pf_time=0;
    // }

    return 0;
}

int energy_meter_control(int set_power, int offset, uint32_t pwr_val, int lost_meter)
{
    static int adj_step = 0;
    static int steady_cnt = 0;
    static int change_cnt = 0;
    static int first_point = 0;
    static int last_point = 0;
    static int prev_point = 0;
    static int last_diff = 0;

    static int nowing_fast = 0;
    static int adjust_cnt = 0;
    static int normal_mode = 0;
    static int last_percent = 0;
    static int last_fast = 0;

    static int last_meter = 0; //PV

    int curr_point = 100;
    int set_point = 100;
    int curr_diff = 0;
    int curr_add = 0;
    int tmp_diff = 0;
    float pwr_offset = 0.002;

    char b_need_adjust = 0;
    char inv_com_protocol[16] = {0};
    if (0 == total_rate_power)
        return -1; //total INV power 6KW+15KW+....
    Inverter *inv_ptr = &inv_arr[0];

    memcpy(inv_com_protocol, &(inv_dev_info[0].regInfo.protocol_ver), 13);
    printf("inv protocol ver %s \n", inv_com_protocol); //"cmv": "V2.1.0V2.0.0",

    curr_point = inv_meter.invdata.pac;
    set_point = set_power; //来自网络APP,cgi，服务器等调节值2000W

    /* calculate the current diff */
    curr_add = (set_point + curr_point) * 100 / total_rate_power;

    //printf("curr_add:%d,set_point:%d,curr_point:%d,inv_total_rated_pac:%d powerinv %d \n", curr_add, set_point, curr_point, total_rate_power , total_curt_power);
    if (offset > 0 && offset <= 100)
    {
        pwr_offset = offset / 1000.0;
        curr_point = pwr_val;
    }
    if (lost_meter)
        curr_point = 0 - get_current_inv_total_power();

    if (strncmp(inv_com_protocol, "V2.1.1", sizeof("V2.1.1")) >= 0)
    {
        float stop_adjust = (set_point + curr_point) * 1.000 / total_rate_power;
        printf("stop---------------------------#########%f####%d##############%d %f \n", stop_adjust, set_point, curr_point, pwr_offset);

        float adjust_per = (set_power + curr_point) * 10000.0 / (total_rate_power * 1.0000);
        short fast_pwr_curr_per = (unsigned short)(adjust_per); //(set_power+total_curt_power+curr_point)*100.0/total_rate_power;

        printf("regulate---------------------> %d %d %d %f\n", fast_pwr_curr_per, curr_add, (set_power + curr_point), adjust_per);

        if (stop_adjust <= pwr_offset && stop_adjust >= 0)
        {
            printf("stop adjust \n");
            return 0;
        }

        printf("NNNN regulate********** %d \n", fast_pwr_curr_per);
        if (fast_pwr_curr_per > 10000)
            fast_pwr_curr_per = 10000;

        if (fast_pwr_curr_per < -10000)
            fast_pwr_curr_per = -10000;

        return writeRegulatePower_fast(fast_pwr_curr_per);
    }

    if ((curr_add >= 0) && (curr_add < 2))
    {
        printf("Steady state ready\r\n");
        steady_cnt++;
        if (steady_cnt >= 2) //No change, it's Steady state
        {
            adj_step = 0;
            change_cnt = 0;
            steady_cnt = 0;
        }
        return 0;
    }
    /* Begin to adjust, forcast initial value */
    printf("adj_step:%d\r\n", adj_step);
    if (0 == adj_step)
    {
        first_point = curr_point + total_rate_power;
        last_point = curr_point;
        b_need_adjust = 1;
        printf("first_point:%d curr_point:%d inv_total_rated_pac:%d\r\n", first_point, curr_point, total_rate_power);
        //task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
        printf("Forcast curr:%d pwr_limit:%d\r\n", pwr_curr_per, pwr_curr_per + curr_add);
    }
    else
    {
        if (steady_cnt >= 4) //No any change, timeout
        {
            //task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
            b_need_adjust = 1;
            printf("No change,timeout b_need_adjust :%d\r\n", b_need_adjust);
        }
        else
        {
            //printf("satet:%d first_meter:%d curr_meter:%d\r\n", adj_phase, first_meter, meter_real_data.power);
            curr_diff = ((last_point - curr_point) * 100 / total_rate_power);
            tmp_diff = ((prev_point - last_point) * 100 / total_rate_power);
            printf("curr_diff-----------tmp_diff------------%d %d \n", curr_diff, tmp_diff);
            if (curr_diff == 0) //No changed state
            {
                steady_cnt++;
                if (steady_cnt >= 1 && adj_step == 2)
                {
                    //task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
                    b_need_adjust = 1;
                    printf("No change state\r\n");
                }
            }
            else //Changed state
            {
                printf("Change state\r\n");
                if ((curr_diff * tmp_diff) < 0)
                    change_cnt++;
                /* changed more than 6 times,but can't reach set point. */
                if (change_cnt >= 4)
                {
                    //task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
                    b_need_adjust = 1;
                    printf(
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

        printf("Fir_point:%d curr_point:%d curr_diff:%d last_diff:%d curr_add:%d\r\n",
               first_point,
               curr_point, curr_diff, last_diff, curr_add);
        if (curr_diff == 0 && last_diff == 0)
        {
            pwr_curr_per = total_curt_power / total_rate_power;
            printf("re init curr_per:%d\r\n", pwr_curr_per);
        }

        printf("pwr_curr_per 1111---------------> %d %d\r\n", pwr_curr_per, curr_add);

        if (abs(curr_add) > pwr_curr_per)
        {
            printf("pwr_curr_per 33---------------> %d %d\r\n", pwr_curr_per, curr_add);
            pwr_curr_per = (unsigned short)abs(pwr_curr_per + curr_add);
        }
        else
        {
            printf("pwr_curr_per 55---------------> %d %d\r\n", pwr_curr_per, curr_add);
            pwr_curr_per = pwr_curr_per + curr_add;
        }
        printf("pwr_curr_per 222 ----------->%d\r\n", pwr_curr_per);

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
            printf("pwr_curr_per 6666 ----------->%d\r\n", pwr_curr_per);
        }
        else if (pwr_curr_per <= 0)
        {
            pwr_curr_per = 1;
        }

        //task_inv_msg |= MSG_PWR_ACTIVE_INDEX;
        writeRegulatePower();
    }

    //rtc_to_date_buffer(rtc_value, NULL);
}

static rst_code md_write_SIG_command(const Inverter *inv_ptr, const void *data_ptr, uint16_t len)
{
    sint_8 buffer[256] = {0};
    uint16_t data_len = 0;
    uint16_t crc_val = 0xFFFF;
    TMD_RTU_WR_MTP_REQ_Packet *spack = NULL;
    spack = (TMD_RTU_WR_MTP_REQ_Packet *)data_ptr;
    memset(buffer, 0x00, 256);
    if (NULL == inv_ptr)
    {
        buffer[0] = 0X00; //broadcast address
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
    for (int i = 0; i < 8; i++)
        printf("<%02x >", buffer[i]);

    data_len = 8;
    memset(buffer, 0, 256);
    int res = recv_bytes_frame_meter(UART_NUM_1, buffer, &data_len);

    for (int i = 0; i < 8; i++)
        printf("<%02x >", buffer[i]);

    if (0 != crc16_calc(buffer, data_len))
    {
        printf("set fail,timeout,buffer length = %d\r\n", data_len);
        return ERR_CRC;
    }

    if ((uint8_t)buffer[1] != 0x06)
    {
        printf("rcv write single fun code error \r\n");
        return ERR_FUN_CODE;
    }
    //printf("reg add:2:%02X-%02X; %d-%d\r\n",buffer[2], buffer[3],((buffer[2] << 8) | buffer[3] ), spack->reg_addr);
    if ((((uint8_t)buffer[2] << 8) | (uint8_t)buffer[3]) != spack->reg_addr)
    {
        printf("rcv write single reg add error \r\n");
        return ERR_REG_ADDR;
    }
    if ((((uint8_t)buffer[4] == spack->data[0]) && (uint8_t)buffer[5] != spack->data[1]))
    {
        printf("rcv write single data error \r\n");
        return ERR_DATA;
    }

    return RST_OK;
}

int pwr = 1;
int md_write_active_pwr(const Inverter *inv_ptr)
{
    TMD_RTU_WR_MTP_REQ_Packet spack = {0};
    int pwr_temp = pwr_curr_per * 100;
    memset(&spack, 0x00, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
    //uint16_t pwr = act_pwr
    spack.reg_addr = 0x151A; //45402 有功功率控制设置值
    spack.reg_num = 1;
    spack.len = 2;
    spack.data[0] = (pwr_temp >> 8) & 0XFF;
    spack.data[1] = pwr_temp & 0XFF;
    printf("%d times adjust************ %d \n", pwr++, pwr_temp);
    return md_write_SIG_command(inv_ptr, &spack, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
}

int md_write_active_pwr_fast(const Inverter *inv_ptr, short adjust)
{
    TMD_RTU_WR_MTP_REQ_Packet spack = {0};
    //int pwr_temp = pwr_curr_per * 100;
    memset(&spack, 0x00, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
    //uint16_t pwr = act_pwr
    spack.reg_addr = 0x154A; //45402 有功功率控制设置值,154A
    spack.reg_num = 1;
    spack.len = 2;
    spack.data[0] = (adjust >> 8) & 0XFF;
    spack.data[1] = adjust & 0XFF;
    printf("%d times adjust fastttt************ %d  \n", pwr++, adjust);
    return md_write_SIG_command(inv_ptr, &spack, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
}

int md_write_reactive_pwr_factory(const Inverter *inv_ptr, short factory)
{
    TMD_RTU_WR_MTP_REQ_Packet spack = {0};
    memset(&spack, 0x00, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
    spack.reg_addr = 0x157E; //45402 有功功率控制设置值,154A
    spack.reg_num = 1;
    spack.len = 2;
    spack.data[0] = (factory >> 8) & 0XFF;
    spack.data[1] = factory & 0XFF;
    printf("reactive_pwr_factory %d  \n", factory);
    return md_write_SIG_command(inv_ptr, &spack, sizeof(TMD_RTU_WR_MTP_REQ_Packet));
}

int writeRegulatePower(void)
{
    if (inv_real_num == 1)
    {
        Inverter *inv_ptr = &inv_arr[0];
        md_write_active_pwr(inv_ptr);
    }
    else
    {
        md_write_active_pwr(NULL);
    }
    return 0;
}

int writeRegulatePower_fast(short adjust_num)
{
    if (inv_real_num == 1)
    {
        Inverter *inv_ptr = &inv_arr[0];
        md_write_active_pwr_fast(inv_ptr, adjust_num);
    }
    else
    {
        md_write_active_pwr_fast(NULL, adjust_num);
    }
    return 0;
}

int writeReative_factory(short fac)
{
    short facx = fac;
    if (facx > 10000)
        facx = 10000;
    if (facx < -10000)
        facx = -10000;

    if (inv_real_num == 1)
    {
        Inverter *inv_ptr = &inv_arr[0];
        md_write_reactive_pwr_factory(inv_ptr, facx);
    }
    else
    {
        //md_write_active_pwr(NULL);
    }
    return 0;
}
