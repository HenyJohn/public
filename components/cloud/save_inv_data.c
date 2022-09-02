#include "save_inv_data.h"
#include "kaco_history_file_manager.h"

static const char *TAG = "save_inv_data";

int net_com_reboot_flg = 0;

int lostdb_size = 0;

uint8_t g_num_failed_write_csv = 0;
/********************************/

// void set_resetnet_reboot(void)
// {
//     net_com_reboot_flg =1;
// }

// int get_mqtt_status(void)  //[ mark]去掉 改为全局
// {
//     return g_state_mqtt_connect;
// }
//-------------------------------//
void check_resetnet_reboot(void)
{
    if (net_com_reboot_flg == 1)
    {
        ESP_LOGW("ESP_RESTART", " will restart by [check reset net] ");
        sleep(2);
        esp_restart();
    }
}
//--------------------------------//
int write_change_apconfig(void)
{
    if (access("/inv/configap", 0) != 0)
    {
        ESP_LOGE("write_lost_index", "configap not existed \n");

        FILE *fp = fopen("/inv/configap", "w");
        if (fp == NULL)
        {
            ESP_LOGE("ff", "Failed to open /inv/configap file");
            return -1;
        }
        fclose(fp);
    }
    return 0;
}

//--------------------------------
void check_external_reboot(void)
{
    if (inv_com_reboot_flg == 1)
    {
        ESP_LOGW("ESP_RESTART", " will restart by [check external] ");
        sleep(2);
        esp_restart();
    }
}
//-------------------------------//
int write_lost_indexs(int index)
{
    if ((-1 == check_file_exist("/inv/lost.db")) || (index < 1))
    {
        remove("/inv/index.db");
        return -1;
    }

    if (access("/inv/index.db", 0) != 0)
    {
        ESP_LOGE("write_lost_index", "index.db not existed create it\n");
        // if(index > 1)
        //     return -1;
    }

    FILE *fp = fopen("/inv/index.db", "wb"); //"wb"
    if (fp == NULL)
    {
        ESP_LOGE("ff", "Failed to open file for writing");
        return -1;
    }
    char buff[32] = {0};

    sprintf(buff, "%d", index);
    ESP_LOGE("write index", "all write*%d#%s#--- ", index, buff);
    fwrite(buff, sizeof(char), sizeof(buff), fp);
    fclose(fp);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
/*1.mq0有数据
    ---1.1 网络正常，发送实时数据
        ----1.1.1 发送成功，正常返回，实时数据标志位置位（1）
        ----1.1.2 发送失败，存储到数据到csv文件中，实时数据标志位复位（0）
    ---1.2 网络异常，存储数据到csv文件中，实时数据标志位复位（0或者2？？)

  2. mq0没数据（上传csv文件）
*/
////////////////////////////////////////////////////////////////////////////////////
/* kaco-lanstick 20220802 + -  */
int recive_invdata(int *flag, kaco_inv_data_t *inv_data, Inv_data *meter_data, int *lost_flag)
{

    kaco_inv_data_t kacoInvData={0};
    // Inv_data invdata = {0};
    Inv_data metdata = {0};

    kaco_inv_data_t last_invdata = {0};
    Inv_data last_metdata = {0};

    uint32_t mTimeOffline = 0;

    static int last_time_log = -5; //-5 means ini

    check_external_reboot();

       // kaco-lanstick 20220802 +
    if (g_kaco_run_mode != 1)
        return 0;
    /* kaco-lanstick 20220802 + */
    if (g_publish_fail > 5)
    {
        sleep(2);
        ESP_LOGE("--Kaco Restart--", " post data to server failed more than five!! ");
        esp_restart();
    }

    if (mq0 != NULL)
    {

        if (xQueueReceive(mq0, &kacoInvData, (TickType_t)10) == pdPASS)
        {
            /* 网络正常，发送实时数据  */
            if ((!netework_state && (g_monitor_state & SYNC_TIME_FROM_CLOUD_INDEX))) // net ok
            {
                get_meter_mem_data(&metdata); // kaco-lanstick 20220802 +

                *inv_data = kacoInvData;
                *meter_data = metdata;              

                ESP_LOGW(TAG, " - mq0 received and to be send to cloud !!\n");
                ///上次实时数据未发送，保存到历史数据中
                if (*lost_flag == 1)
                {
                    if (0 == time_legal_parse())
                    {

                        if (asw_kaco_write_lost_data(&last_invdata, &last_metdata) != 0)
                        {
                            ESP_LOGE("--- Kaco Write Lost Data ERRO --", " failed write lost data to csv file!! ");
                            g_num_failed_write_csv++;
                        }
                        else
                        {
                            *lost_flag = 0;
                        }
                    }
                    else
                    {
                        ESP_LOGE("-- Kaco Write ERRO --", " time is not right. ");
                    }
                }
                //保存上次数据
                last_invdata = kacoInvData;
                last_metdata = metdata;

                *flag = 1;
                *lost_flag = 1;
            }
            else /* no net ,save 网络异常,存储数据到csv文件中*/
            {
                printf("recv offline msg===========>>>>%s \n", kacoInvData.crt_inv_data.psn);
                if (0 == time_legal_parse())
                {
                    get_meter_mem_data(&metdata); // kaco-lanstick 20220802 +
                    *flag = 0;
                    if (asw_kaco_write_lost_data(&kacoInvData, &metdata) != 0)
                    {
                        ESP_LOGE("--- Kaco Write Lost Data ERRO --", " failed write lost data to csv file!! ");
                        g_num_failed_write_csv++;
                        *flag = 2; //历史数据写失败
                    }
                }
                else
                {
                    ESP_LOGE("--- Kaco Write Lost Data ERRO --", "time is not set !!! ");
                    *flag = 2; //历史数据写失败
                }
            }
        }
        else if (get_eth_connect_status() == 0 && get_second_sys_time() > 30) // net ok and 3min after
        /* 有网络，没实时数据，发送历史数据文件 */
        {
            *flag = 3; //消息队列没有数据，发送历史数据
        }

        return 0;
    }
    ESP_LOGE("--Kaco mq0 ERRO--", "mq0 is NULL.");
    return -1;
}

// kaco-lanstick 20220802 +
int read_lost_data_multi(Inv_data *inv_data, int *line, int *er_line, int *filetail, int *met_data)
{
    if (access("/inv/lost.db", 0) != 0)
    {
        ESP_LOGE("read_lost_data", "file don't existed \n");
        return -1;
    }

    if (*line < 0)
        *line = 0;

    FILE *fp = fopen("/inv/lost.db", "rb");
    if (fp == NULL)
    {
        ESP_LOGE("TAG", "Failed to open file for reading");
        // remove("/inv/lost.db");
        return -1;
    }
    uint8_t buff[300] = {0};
    int tmp_line = 0;
    tmp_line = *line;
    uint8_t j = 0;
    int error_line = 0;
    int rst = 0;
    int reads = 0;
    int org_line = *line;
    int met_flg = 0;

    fseek(fp, 0, SEEK_END);
    ESP_LOGE("TAG", "startline %d readlost file len  %ld \n", org_line, ftell(fp));
    lostdb_size = ftell(fp);

    fseek(fp, (*line) * 290, SEEK_SET);
    // tmp_line++;
    while (!feof(fp) && j < 10 && reads < 11)
    {
        if (j)
            fseek(fp, (tmp_line)*290, SEEK_SET);
        rst = fread(buff, sizeof(char), sizeof(Inv_data) + 2, fp);
        if (rst == 290)
        {
            ESP_LOGE("TAG", "all read--%d--%d--  %s--%d ", tmp_line, *line, buff, rst);

            if (tmp_line > *line)
            {
                ESP_LOGE("TAG", "find new line --%d--%d--  %s ", tmp_line, *line, buff);
                // break;
            }
            //}
            // int i = 0;
            printf("read put invdata %d crc=%d \n", sizeof(Inv_data) + 2, crc16_calc(buff, sizeof(Inv_data) + 2));

            if (0 != crc16_calc(buff, sizeof(Inv_data) + 2))
            {
                printf("read lost data crc error \n");
                //*line=tmp_line;
                error_line++;
                *er_line = error_line;

                ESP_LOGE("read lost data", "lost.db error delete file------------>>>>>>\n");
                fclose(fp);
                remove("/inv/lost.db");
                remove("/inv/index.db");
                return -2;
            }

            memcpy(&inv_data[j++], buff, sizeof(Inv_data));
            tmp_line++;
        }
        reads++;
        if (j > 0 && (0 == strncmp(inv_data[j - 1].psn, "SDM", 3)))
            met_flg++;

        ESP_LOGE("TAG", "read %d 'st line  invdata time %s %s %d %d\n", tmp_line, inv_data[j - 1].time, inv_data[j - 1].psn, feof(fp), (tmp_line)*290);
    }
    ESP_LOGE("TAG_END", "read %d 'st line  invdata time %s %s %d %d\n", tmp_line, inv_data[j - 1].time, inv_data[j - 1].psn, feof(fp), (tmp_line)*290);

    if (feof(fp))
    {
        *filetail = 1;
        ESP_LOGE("read lost data", "read file end watting delet \n");

        if (j == 0)
        {
            ESP_LOGE("don't found data", "read file has end delete file------------>>>>>>\n");
            remove("/inv/lost.db");
            remove("/inv/index.db");
        }
    }
    fclose(fp);
    *line = (tmp_line - org_line);
    *met_data = met_flg;
    return j;
}

// kaco-lanstick 20220802 +
int find_local_data(Inv_data *invdata, int *line, int *fileof, int *meter_flg)
{
    int lost_line = 0;
    int errorline = 0;

    int res = -10;

    // if (1) //get_second_sys_time() > 300)  //query data
    // {
    lost_line = read_lost_index();

    if ((res = read_lost_data_multi(invdata, &lost_line, &errorline, fileof, meter_flg)) >= 0)
    {

        *line = lost_line;
        printf("toal read %d %d %d st line \n", lost_line, *line, res);
        return res;
    }
    if (res == -2)
    {
        printf("read lost data crc error , delet lost.db\n");

        lostdb_size = 0;
        return -2;
    }
    if (res == -1)
    {
        printf("don't found lost data , contiue loop\n");
        lostdb_size = 0;

        return -1;
    }

    return 0;
    // }
}

int delet_local_data(int line, int file_tail)
{
    if (file_tail)
    {
        ESP_LOGE("read lost data end", "read file end pub ok delete file------------>>>>>>\n");
        remove("/inv/lost.db");
        remove("/inv/index.db");
        return 0;
    }
    else
    {
        int lostflgline = read_lost_index();
        if (lostflgline < 1)
            lostflgline = 0;
        if ((line + lostflgline) * 290 >= lostdb_size)
        {
            ESP_LOGE("read lostdb completed", "read file end pub ok delete file------------>>>>>>\n");
            remove("/inv/lost.db");
            remove("/inv/index.db");
            return 1;
        }
        write_lost_indexs(line + lostflgline);
        return 2;
    }
}

// kaco-lanstick 20220802 +
void get_time_from_tmstp(char *p_date, int len, uint32_t tmstp)
{
    time_t t = tmstp;
    struct tm currtime = {0};
    strftime(p_date, len, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &currtime));
}
// kaco-lanstick 20220802 +
int parse_dst(char *str, uint32_t time_sec)
{
    // struct tm tm = {0};
    // struct timeval tv = {0};
    char curr_date[20] = {0};
    char date1[20] = {0};
    char date2[20] = {0};

    get_time_from_tmstp(curr_date, sizeof(curr_date), time_sec);
    char *p = strstr(str, ",");
    memcpy(date1, str, 19);
    memcpy(date2, p + 1, 19);

    printf("curr date:%s\n", curr_date);
    printf("date1:%s\n", date1);
    printf("date2:%s\n", date2);

    // kaco-lanstick 20220811 +-
    if (strcmp(date2, date1) > 0)
    {
        if (strcmp(curr_date, date1) > 0 && strcmp(curr_date, date2) < 0)
        {
            return 0;
        }
    }

    // if (strcmp(curr_date, date1) > 0 && strcmp(curr_date, date2) < 0)
    // {
    //     return 0;
    // }
    return -1;
}

// kaco-lanstick 20220802 +

uint32_t read_timezone(uint32_t utc_sec)
{
    int i = -1;
    // FILE *file;
    int r_size = 0;
    char buf[512] = {0};
    int tm_off = 0;
    char time[100] = {0};
    uint32_t time_sec = utc_sec;

    FILE *fp = fopen("/inv/timezone", "rb");
    if (fp == NULL)
    {
        ESP_LOGE("TAG", "Failed to open timezone for reading");
        // remove("/inv/lost.db");
        return time_sec;
    }

    r_size = fread(buf, sizeof(char), sizeof(buf), fp);
    // if(FR_OK == f_read(&file, buf, sizeof(buf), (uint_32*)&r_size))
    printf("read %d timezone:%s \n", r_size, buf);
    if (r_size > 0)
    {
        char *p1 = strstr(buf, "\r\n"); // time offset
        if (p1)
        {
            memcpy(time, buf, p1 - buf);
            printf("read first %d %d line %s\n", strlen(time), p1 - buf, time);
            tm_off = atoi(time);
            memset(time, 0, sizeof(time));
            p1 += 2;
            printf("tm_off %d \n", tm_off);
        }
        time_sec = utc_sec + tm_off * 60;
        printf("utc+offset %d \n", time_sec);

        while (i++ < 10)
        {
            char *p2 = strstr(p1, "\r\n");
            if (p2 != NULL)
            {
                memcpy(time, p1, p2 - p1);
                printf("read %d line %d %d %s\n", i, strlen(time), p2 - p1, time);
                if (parse_dst(time, time_sec) == 0)
                {
                    if (tm_off == 630) //+10:30 = Loard Howe Island
                    {
                        printf("Lord Howe \n");
                        time_sec += 1800;
                    }
                    else
                    {
                        time_sec += 3600;
                    }

                    break;
                }
                p1 = p2 + 2;
            }
            else
            {
                printf("newline not found\n");
                break;
            }
        }
    }
    else
        printf("read timezone error\n");

    fclose(fp);
    printf("finally timesec %d \n", time_sec);
    return time_sec;
}
