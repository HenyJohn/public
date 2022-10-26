#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include <stdlib.h>
#include "mqueue.h"
#include "data_process.h"

extern int netework_state;
extern int8_t monitor_state;
// extern int mqtt_connect_state;
// extern int ap_enable;
extern int inv_com_reboot_flg;

#define DELETE_NUM 5
#define OLD_DB_PATH "/inv/lost.db"
#define NEW_DB_PATH "/inv/new.db"

int check_file_exist(char *path)
{
    if (access(path, 0) != 0)
    {
        ESP_LOGE(path, "not existed \n");
        return -1;
    }
    return 0;
}

int find_new_days(FILE *fp, char *day)
{
    if (fp == NULL)
    {
        ESP_LOGE("read_lost_data", "find days error \n");
        return -1;
    }
    char buff[300] = {0};
    int i = 0;

    while (!feof(fp))
    {
        fread(buff, sizeof(char), sizeof(Inv_data) + 2, fp);
        ESP_LOGE("read index", "all read*%d#%s#--- ", i, buff);
        i++;

        if (0 != crc16_calc(buff, sizeof(Inv_data) + 2))
        {
            printf("read lost data crc error \n");
            continue;
        }

        memcpy(day, &buff[50], 10);
        break;
    }

    ESP_LOGE("read days", "current available day %s line is %d \n", day, i);
    if (i)
        return i;
    return -1;
}

int find_next_day(FILE *fp, char *day, int lines)
{
    if (fp == NULL)
    {
        ESP_LOGE("read_lost_data", "find next days error \n");
        return -1;
    }
    char buff[300] = {0};
    int i = lines;

    while (!feof(fp))
    {
        fread(buff, sizeof(char), sizeof(Inv_data) + 2, fp);
        ESP_LOGE("read next days", "all read*%d#%s#--- ", i, buff);
        i++;

        if (0 != crc16_calc(buff, sizeof(Inv_data) + 2))
        {
            printf("read lost data crc error \n");
            continue;
        }

        ESP_LOGE("find available next days", "it's %s \n", &buff[50]);
        if (strncmp((const char *)day, &buff[50], 10) != 0)
        {
            ESP_LOGE("read days", "next change day %s line is %d \n", day, i);
            return i;
        }
    }

    return -1;
}

int transfer_another_file(void)
{
    FILE *new_fp = NULL;
    int nread = 0;
    int blob_size = 0;
    char buf[5900] = {0};
    int read_index = 0;
    char days[10] = {0};
    int line = 0;

    FILE *fp = fopen(OLD_DB_PATH, "rb");
    if (fp == NULL)
    {
        ESP_LOGE("read_lost_data", "Failed to open file for lostdata");
        return -1;
    }

    blob_size = sizeof(Inv_data) + 2;

    line = find_new_days(fp, days);
    if (line > 0)
    {
        fseek(fp, blob_size * line, SEEK_SET);
        line = find_next_day(fp, days, line);
        if (line < 2)
        {
            fclose(fp);
            remove(OLD_DB_PATH);
            remove("/inv/index.db");
            ESP_LOGE("move data", "all data repeat delet lost.db \n");
            return -1;
        }
    }
    else
    {
        fclose(fp);
        remove(OLD_DB_PATH);
        remove("/inv/index.db");
        ESP_LOGE("move data", "not found available day delet lost.db\n");
        return -1;
    }
    // ESP_LOGE("transfer_another_file ", "reseek line  %d \n",  line);
    if (fp != NULL)
    {
        fseek(fp, blob_size * line, SEEK_SET);
        new_fp = fopen(NEW_DB_PATH, "wb");
        while (!feof(fp)) // to read file
        {
            nread = fread(buf, 1, blob_size * 20, fp);
            ESP_LOGE("move data", "read bytes %d \n", nread);

            if (0 == (nread % blob_size))
            {
                fwrite(buf, 1, nread, new_fp);
            }
            usleep(10000);
        }
        fclose(fp);
        fclose(new_fp);
        remove(OLD_DB_PATH);
        rename(NEW_DB_PATH, OLD_DB_PATH);
        read_index = read_lost_index();
        // ESP_LOGE("transfer_another_file ", "redefine index %d %d \n", read_index, line);
        if (read_index > line)
        {
            write_lost_index(read_index - line);
        }
        else
        {
            remove("/inv/index.db");
        }
    }
    return 0;
}

int check_inv_pv(Inv_data *inv_data)
{
    int i = 0;
    for (i = 0; i < 10;)
        if (inv_data->PV_cur_voltg[i].iVol > 800 && inv_data->PV_cur_voltg[i].iVol < 65535)
            break; //j++;//printf("PV > 80  %d\n", inv_data_ptr->PV_cur_voltg[i].iVol);
        else
            i++;

    if (i > 9)
    {
        printf("PV TO LOW return \n");
        return -1;
    }
    printf("PV > 80  %d\n", inv_data->PV_cur_voltg[i].iVol);
    return 0;
}

int write_lost_data(Inv_data *inv_data)
{
    if (check_inv_pv(inv_data) < 0)
        return -1;

    FILE *fp = fopen("/inv/lost.db", "ab+"); //"wb"
    if (fp == NULL)
    {
        ESP_LOGE("ff", "Failed to open file for writing");
        return -2;
    }

    fseek(fp, 0, SEEK_END);
    ESP_LOGE("TAG", "file len  %d \n", ftell(fp));

    if (ftell(fp) > 1827000) //5 invs 7day 1827000
    {
        ESP_LOGE("write lostdata", "file too large move it");
        fclose(fp);
        transfer_another_file();

        FILE *fp = fopen("/inv/lost.db", "ab+"); //"wb"
        if (fp == NULL)
        {
            ESP_LOGE("ff", "Failed to open file for writing");
            return -3;
        }
    }

    fseek(fp, 0, SEEK_SET);

    char buff[300] = {0};
    memcpy(buff, (char *)inv_data, sizeof(Inv_data));

    uint16_t crc = crc16_calc(buff, sizeof(Inv_data));
    buff[sizeof(Inv_data)] = crc & 0xFF;
    buff[sizeof(Inv_data) + 1] = (crc >> 8) & 0xFF;
    int i = 0;

    // printf("out put invdata %d \n", sizeof(Inv_data)+2);
    // for(;i<sizeof(Inv_data)+2; i++)
    //     printf("%02x ", buff[i]);
    // printf("output end\n");

    fwrite(buff, sizeof(char), sizeof(Inv_data) + 2, fp);

    fclose(fp);
    return 0;
    //ESP_LOGI("ff", "File written");

    // Open file for reading
    /*
    ESP_LOGI("TAG", "Reading file");
    fp = fopen("/inv/lost.db", "rb");
    if (fp == NULL) {
        ESP_LOGE("TAG", "Failed to open file for reading");
        return;
    }
    char buff[512]={0};
    fseek(fp, 0, SEEK_END);
    ESP_LOGE("TAG", "file len  %d \n", ftell(fp));    
    fseek(fp, 0, SEEK_SET);        
    while (!feof(fp) )
    {
        fread(buff, sizeof(char), 512, fp);
        ESP_LOGE("TAG", "all read   %s \n", buff);
    }
    */
    //memset(buff, 0, sizeof(buff));
    //if(!feof(fp))
    // fread(buff, sizeof(char), 512, fp);
    // ESP_LOGE("TAG", "all read   %s \n", buff);
    //rewind(fp);
    // char line[128];
    // int ln=0;
    // while (!feof(fp) )
    // {
    //     fgets(line, sizeof(line), fp);
    //     // strip newline
    //     char *pos = strchr(line, '\n');
    //     if (pos) {
    //         *pos = '\0';
    //     }
    //     ln++;
    //     ESP_LOGI("TAG", "Read %d from file: '%s'", ln, line);
    //      ESP_LOGE("TAG", "read %d file '%s'", ln, line);
    // }

    //fclose(fp);
}

int write_lost_index(int index)
{
    if ((-1 == check_file_exist("/inv/lost.db")) || (index < 1))
        return -1;

    if (access("/inv/index.db", 0) != 0)
    {
        ESP_LOGE("write_lost_index", "index.db not existed \n");
        if (index > 1)
            return -1;
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

int read_lost_index(void)
{
    int tmp_index = 0;

    if (access("/inv/index.db", 0) != 0)
    {
        ESP_LOGE("read_index_data", "file don't existed \n");
        return -1;
    }

    FILE *fp = fopen("/inv/index.db", "rb");
    if (fp == NULL)
    {
        ESP_LOGE("read_lost_index", "Failed to open file for readindex");
        return 0;
    }

    char buff[32] = {0};
    char i = 0;
    i = fread(buff, sizeof(char), 32, fp);
    ESP_LOGE("read index", "all read*%d#%s#--- ", i, buff);

    tmp_index = atoi(buff);
    ESP_LOGE("read index", "read index is %d \n", tmp_index);
    if (tmp_index < 8000)
        return tmp_index;
    else
        return 0;
}

int check_lost_data(void)
{
    if (access("/inv/lost.db", 0) != 0)
    {
        ESP_LOGE("check_lost_data", "file don't existed \n");
        return -1;
    }

    FILE *fp = fopen("/inv/lost.db", "rb");
    if (fp == NULL)
    {
        ESP_LOGE("check_lost_data", "Failed to open file for reading");
        return -2;
    }

    char buff[300] = {0};
    fread(buff, sizeof(char), sizeof(Inv_data) + 2, fp);
    fclose(fp);

    if (0 != crc16_calc(buff, sizeof(Inv_data) + 2))
    {
        printf("check lost data crc error \n");
        return -3;
    }

    return 0;
}

int read_lost_data(Inv_data *inv_data, int *line)
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
        //remove("/inv/lost.db");
        return -1;
    }
    char buff[300] = {0};
    int tmp_line = 0;
    tmp_line = *line;

    fseek(fp, (*line) * 290, SEEK_SET);

    //while (!feof(fp) )
    {
        tmp_line++;
        fread(buff, sizeof(char), sizeof(Inv_data) + 2, fp);
        ESP_LOGE("TAG", "all read--%d--%d--  %s ", tmp_line, *line, buff);

        if (tmp_line > *line)
        {
            ESP_LOGE("TAG", "find new line --%d--%d--  %s ", tmp_line, *line, buff);
            //break;
        }
    }
    // if(tmp_line == *line)
    //     {
    //         ESP_LOGE("TAG", "file read to end--%d--%d--  %s ", tmp_line, *line, buff);
    //         break;
    //     }
    if (feof(fp))
    {
        ESP_LOGE("read lost data", "read file end delete file------------>>>>>>\n");
        remove("/inv/lost.db");
        remove("/inv/index.db");
        //remove("/inv/*");
    }
    fclose(fp);

    int i = 0;
    printf("read put invdata %d crc=%d \n", sizeof(Inv_data) + 2, crc16_calc(buff, sizeof(Inv_data) + 2));

    if (0 != crc16_calc(buff, sizeof(Inv_data) + 2))
    {
        printf("read lost data crc error \n");
        *line = tmp_line;
        return -2;
    }

    for (; i < sizeof(Inv_data) + 2; i++)
        printf("%02x ", buff[i]);
    printf("readput end\n");

    memcpy(inv_data, buff, sizeof(Inv_data));
    *line = tmp_line;
    ESP_LOGE("TAG", "read %d 'st line  invdata time %s %s\n", *line, inv_data->time, inv_data->psn);
    return 0;
}

int recive_invdata(int *flag, Inv_data *inv_data, int *lost_flag)
{
    Inv_data invdata = {0};
    static int lost_line = 0;
    static Inv_data tmp_invdata = {0};
    static int read_lost_time = 0;
    int res = 0;
    static int last_time_log = -5; //-5 means ini

    check_external_reboot();

    /** 进行存储*/
    // printf("AAAE: lost_flag = %d ------------2\n", *lost_flag);
    if (3 == *lost_flag)
    {
        if (0 == time_legal_parse())
        {
            // printf("AAAE: lost_flag = %d ------------3\n", *lost_flag);
            // if (strlen(inv_data->psn) == 0 || strlen(inv_data->time) == 0)
            //     printf("AAAE: invdata is null, ------------4\n");
            if (write_lost_data(inv_data) < -1)
                force_flash_reformart("invdata");
        }
        *lost_flag = 0;
    }

    if (mq0 != NULL)
        if (xQueueReceive(mq0, &invdata, (TickType_t)10) == pdPASS)
        {
            // printf("AAAE: GHHHHHHHHHHHH------------2\n");
            /** 读取队列的数据*/
            printf("mqtt status: %d\n", cat1_get_mqtt_status(0) == 1);
            printf("netework_state: %d\n", !netework_state);
            printf("monitor_state: %d\n", monitor_state && INV_SYNC_PMU_STATE);
            if ((cat1_get_mqtt_status(0) == 1) && (!netework_state || (monitor_state && INV_SYNC_PMU_STATE))) //net ok
            {
                // printf("AAAE: FHHHHHHHHHHHH------------2\n");
                *flag = 1;
                *lost_flag = 0;
                *inv_data = invdata;
                //printf("recvg  ##########----- invdata %d \n", sizeof(Inv_data)+2);
                //return 0;
                /*               char buff[300]={0};
    memcpy(buff, (char *)&invdata, sizeof(Inv_data));

    int i=0;
    printf("read ************ invdata %d crc=%d \n", sizeof(Inv_data)+2, crc16_calc(buff, sizeof(Inv_data)+2));
    for(;i<sizeof(Inv_data)+2; i++)
        printf("%02x ", buff[i]);
    printf("readput end\n");

memcpy(buff, (char *)inv_data, sizeof(Inv_data));
 
    printf("read ----- invdata %d crc=%d \n", sizeof(Inv_data)+2, crc16_calc(buff, sizeof(Inv_data)+2));
    for(i=0;i<sizeof(Inv_data)+2; i++)
        printf("%02x ", buff[i]);
    printf("readput end\n");*/
                if ((last_time_log != 0 || last_time_log == -5))
                {
                    network_log(",[ok] MQTT CONNECT");
                    last_time_log = 0;
                }
                return 777;
            }
            else /** 网络异常，直接存储*/
            {
                if (0 == time_legal_parse())
                {
                    if (write_lost_data(&invdata) < -1)
                        force_flash_reformart("invdata");
                    else
                    {
                        if (check_lost_data() < -1)
                            if (check_inv_pv(&invdata) == 0)
                                force_flash_reformart("invdata");
                    }
                }

                if ((last_time_log == 0 || last_time_log == -5))
                {
                    network_log(",[err] MQTT CONNECT");
                    last_time_log = -10;
                }
            }
            return 0; /** 队列有数据，返回*/
        }
        else
        {
            /** 队列无数据*/
            *flag = 0;
        }

    /** 队列无数据*/
    if ((cat1_get_mqtt_status(0) == 1) && (!netework_state || (monitor_state && INV_SYNC_PMU_STATE)) && get_second_sys_time() > 300) //query data
    {
        *flag = 0;
        if ((get_second_sys_time() - read_lost_time) < 1)
            return -1;
        read_lost_time = get_second_sys_time();

        if (0 == *lost_flag)
            lost_line = read_lost_index();

        if (*lost_flag != 1)
        {
            write_lost_index(lost_line);
            printf("read next line %d \n", *lost_flag);
            if ((res = read_lost_data(&invdata, &lost_line)) >= 0)
            {
                printf("read %d st line \n", lost_line);
                *flag = 1;
                *lost_flag = 1; /** 读取下一条缓存，成功*/
                *inv_data = invdata;
                tmp_invdata = invdata;
            }
            if (res == -2)
            {
                printf("read lost data crc error , contiue loop\n");
                *flag = 0;
                *lost_flag = 2; /** 读缓存数据失败*/
                //write_lost_index(lost_line);
                memset(inv_data, 0, sizeof(Inv_data));
                return -2;
            }
            if (res == -1)
            {
                printf("don't found lost data , contiue loop\n");
                *flag = 0;
                *lost_flag = 0; /** 无缓存数据*/
                //write_lost_index(lost_line);
                memset(inv_data, 0, sizeof(Inv_data));
                return -1;
            }
        }
        else
        {
            printf("copy preline %d line \n", lost_line);
            *inv_data = tmp_invdata;
            *flag = 1;
            *lost_flag = 1; /** 缓存发送失败，继续那一条*/
        }
        // if (check_sn_format(inv_data->psn) < 15 || ((inv_data->time[0] - 0x30) * 10 + (inv_data->time[0] - 0x30) < 20))
        // if (check_sn_format(inv_data->psn) < 15)
        // {
        //     printf("read inv flag: %s %s\n", inv_data->psn, inv_data->time);
        //     *flag = 0;
        //     *lost_flag = 0;
        // }
        // printf("read time %d \n", (inv_data->time[0] - 0x30) * 10 + (inv_data->time[0] - 0x30));
    }
    return 0;
}

int check_external_reboot(void)
{
    if (inv_com_reboot_flg == 1)
    {
        sleep(2);
        esp_restart();
    }
}

#include "asw_store.h"
#include "device_data.h"
#include "aiot_mqtt_sign.h"
extern char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1];
int _3rd_pmu_info_publish(int index, char *pub_topic)
{
    int res = -1;
    char payload[1000];
    Setting_Para ini_para = {0};
    general_query(NVS_ATE, &ini_para);
    sprintf(payload, "{\"Action\":\"SyncPmuInfo\","
                     "\"sn\":\"%s\","
                     "\"typ\":\"%d\","
                     "\"nam\":\"%s\","
                     "\"sw\":\"%s\","
                     "\"hw\":\"%s\","
                     "\"sys\":\"%s\","
                     "\"md1\":\"%s\","
                     "\"md2\":\"%s\","
                     "\"mod\":\"%s\","
                     "\"muf\":\" %s\","
                     "\"brd\":\"%s\","
                     "\"cmd\":\"%s\"}",
            DEMO_DEVICE_NAME,
            ini_para.typ,
            ini_para.nam,
            FIRMWARE_REVISION,
            ini_para.hw,
            "freertos",
            ini_para.md1,
            CGI_VERSION, //ini_para.md2,//////////////////////////////V1.0
            ini_para.mod,
            ini_para.mfr,
            ini_para.brw,
            ini_para.cmd);
    res = cat1_mqtt_pub(index, g_cst_pub_topic, payload);
    if (res == 0)
        return 0;
    else
    {
        printf("[ err ] pub 3rd pmu info\n");
        return -1;
    }
}

extern Inv_arr_t inv_dev_info;
extern Inv_arr_t inv_arr;
extern int inv_real_num;
int _3rd_inv_info_publish(int index, char *pub_topic)
{
    int res = -1;
    char payload[1000] = {0};
    int inv_sync_num = 0;
    int success_num = 0;
    InvRegister devicedata = {0};

    while ((inv_sync_num < inv_real_num))
    {
        usleep(50000);
        if (inv_arr[inv_sync_num].regInfo.modbus_id > 0 && strlen(inv_arr[inv_sync_num].regInfo.sn) > 0)
        {
            memcpy(&devicedata, &(inv_dev_info[inv_sync_num].regInfo), sizeof(InvRegister));
            if (check_sn_format(devicedata.sn) > 0)
            {
                sprintf(payload, "{\"Action\":\"SyncInvInfo\","
                                 "\"psn\":\"%s\","
                                 "\"typ\":\"%d\","
                                 "\"sn\":\"%s\","
                                 "\"mod\":\"%s\","
                                 "\"sft\":\"%d\","
                                 "\"rtp\":\"%d\","
                                 "\"mst\":\"%s\","
                                 "\"slv\":\"%s\","
                                 "\"sfv\":\"%s\","
                                 "\"cmv\":\"%s\","
                                 "\"muf\":\" %s\","
                                 "\"brd\":\"%s\","
                                 "\"mty\":\"%d\","
                                 "\"MPT\":\"%d\","
                                 "\"str\":\"%d\","
                                 //"\"phs\":\"%d\","
                                 "\"Imc\":\"%d\"}",
                        DEMO_DEVICE_NAME,
                        devicedata.type,
                        devicedata.sn,
                        devicedata.mode_name,
                        devicedata.safety_type,
                        devicedata.rated_pwr,
                        devicedata.msw_ver,
                        devicedata.ssw_ver,
                        devicedata.tmc_ver,
                        devicedata.protocol_ver,
                        devicedata.mfr,
                        devicedata.brand,
                        devicedata.mach_type,
                        devicedata.pv_num,
                        devicedata.str_num,
                        //"machine type",
                        devicedata.modbus_id);
                // printf("PUB  %s inv info**************\n", payload);
                res = cat1_mqtt_pub(index, g_cst_pub_topic, payload);
                if (res == 0)
                {
                    printf("[ 3rd_publish ] %d inv info success\n", inv_sync_num);
                    success_num++;
                }
                else
                {
                    printf("[ 3rd_publish ] %d inv info fail\n", inv_sync_num);
                }
            }
        }
        inv_sync_num++;
    }

    if (success_num > 0)
        return 0;
    else
    {
        return -1;
    }
}

int get_invdata_payload(char *payload, Inv_data invdata)
{
    int datlen = sprintf(payload, "{\"Action\":\"UpdateInvData\",\"psn\":\"%s\",\"sn\":\"%s\","
                                  "\"smp\":5,\"tim\":\"%s\","
                                  "\"fac\":%d,\"pac\":%d,\"er\":%d,"
                                  "\"pf\":%d,\"cf\":%d,\"eto\":%d,\"etd\":%d,\"hto\":%d,",
                         DEMO_DEVICE_NAME, invdata.psn,
                         invdata.time,
                         invdata.fac, invdata.pac, invdata.error,
                         invdata.cosphi, invdata.col_temp, invdata.e_total, invdata.e_today, invdata.h_total);
    int i;
    for (i = 0; i < 10; i++)
    {

        if ((invdata.PV_cur_voltg[i].iVol != 0xFFFF) && (invdata.PV_cur_voltg[i].iCur != 0xFFFF))
        {
            datlen += sprintf(payload + datlen, "\"v%d\":%d,\"i%d\":%d,",
                              i + 1, invdata.PV_cur_voltg[i].iVol,
                              i + 1, invdata.PV_cur_voltg[i].iCur);
        }
    }
    /* check 1~3 ac parameters*/
    for (i = 0; i < 3; i++)
    {
        if ((invdata.AC_vol_cur[i].iVol != 0xFFFF) && (invdata.AC_vol_cur[i].iCur != 0xFFFF))
        {
            datlen += sprintf(payload + datlen, "\"va%d\":%d,\"ia%d\":%d,",
                              i + 1, invdata.AC_vol_cur[i].iVol,
                              i + 1, invdata.AC_vol_cur[i].iCur);
        }
    }
    /* check 1~20 string current parameters*/
    for (i = 0; i < 20; i++)
    {
        if ((invdata.istr[i] != 0xFFFF))
        {

            datlen += sprintf(payload + datlen, "\"s%d\":%d,", i + 1, invdata.istr[i]);
        }
    }
    //Check_upload_unit16
    Check_upload_unit32("\"prc\":%u,", invdata.qac, payload, &datlen);
    Check_upload_unit32("\"sac\":%u,", invdata.sac, payload, &datlen);
    Check_upload_sint16("\"tu\":%d,", invdata.U_temp, payload, &datlen);
    Check_upload_sint16("\"tv\":%d,", invdata.V_temp, payload, &datlen);
    Check_upload_sint16("\"tw\":%d,", invdata.W_temp, payload, &datlen);
    Check_upload_sint16("\"cb\":%d,", invdata.contrl_temp, payload, &datlen);
    Check_upload_sint16("\"bv\":%d,", invdata.bus_voltg, payload, &datlen);
    //printf("[ payload 5 ]\n%s\n", payload);
    /*check 1~10 warn*/
    for (i = 0; i < 10; i++)
    {
        if (invdata.warn[i] != 0xFF)
        {
            datlen += sprintf(payload + datlen, "\"wn%d\":%d,", i + 1, invdata.warn[i]);
        }
    }
    /* itv field as the end of the body*/
    datlen += sprintf(payload + datlen, "\"itv\":%d}", GetMinute1((invdata.time[11] - 0x30) * 10 + invdata.time[12] - 0x30, (invdata.time[14] - 0x30) * 10 + invdata.time[15] - 0x30, 0));
    // printf("payload   %s\n", payload);
    return datlen;
}

void recive_invdata_cst(int *flag, Inv_data *inv_data, int *lost_flag)
{
    // Inv_data invdata = {0};
    // int res = -1;

    // /** send queue data, if exists */
    // if (_3rd_mq != NULL)
    // {
    //     if (xQueueReceive(_3rd_mq, &invdata, (TickType_t)10) == pdPASS)
    //     {
    //         // char payload[1200] = {0};
    //         // get_invdata_payload(payload, invdata);
    //         // cat1_mqtt_pub(index, pub_topic, payload);
    //         //publish; if fail, save
    //         return;
    //     }
    // }
    Inv_data invdata = {0};
    static int cst_lost_line = 0;
    static Inv_data cst_tmp_invdata = {0};
    static int cst_read_lost_time = 0;
    int res = 0;
    static int cst_last_time_log = -5; //-5 means ini

    if (3 == *lost_flag)
    {
        if (0 == time_legal_parse())
        {
            if (write_lost_data_cst(inv_data) < -1)
                force_flash_reformart("invdata");
        }
        *lost_flag = 0;
    }

    if (_3rd_mq != NULL)
        if (xQueueReceive(_3rd_mq, &invdata, (TickType_t)10) == pdPASS)
        {
            if ((cat1_get_mqtt_status(1) == 1) && (!netework_state || (monitor_state && INV_SYNC_PMU_STATE)))
            {

                *flag = 1;
                *inv_data = invdata;
                if ((cst_last_time_log != 0 || cst_last_time_log == -5))
                {
                    network_log(",[ok] MQTT CONNECT");
                    cst_last_time_log = 0;
                }
            }
            else
            {
                if (0 == time_legal_parse())
                {
                    if (write_lost_data_cst(&invdata) < -1)
                        force_flash_reformart("invdata");
                    /*else
                    {
                        if(check_lost_data() < -1)
                            if(check_inv_pv(&invdata) == 0)
                                force_flash_reformart("invdata");
                    }*/
                }

                if ((cst_last_time_log == 0 || cst_last_time_log == -5))
                {
                    network_log(",[err] MQTT CONNECT");
                    cst_last_time_log = -10;
                }
            }
            return 0;
        }
        else
        {
            *flag = 0;
        }

    if ((cat1_get_mqtt_status(1) == 1) && (!netework_state || (monitor_state && INV_SYNC_PMU_STATE)) && get_second_sys_time() > 300) //query data
    {
        *flag = 0;
        if ((get_second_sys_time() - cst_read_lost_time) < 1)
            return -1;
        cst_read_lost_time = get_second_sys_time();

        if (0 == *lost_flag)
            cst_lost_line = read_lost_index_cst();
        printf("lost_linexxxxx------------------------->%d  %d \n", cst_lost_line, *lost_flag);
        if (*lost_flag != 1)
        {
            printf("lost_lineppppp------------------------->%d  %d \n", cst_lost_line, *lost_flag);
            write_lost_index_cst(cst_lost_line);
            printf("read next line %d \n", *lost_flag);
            if ((res = read_lost_data_cst(&invdata, &cst_lost_line)) >= 0)
            {
                printf("read %d st line \n", cst_lost_line);
                *flag = 1;
                *lost_flag = 1;
                *inv_data = invdata;
                cst_tmp_invdata = invdata;
            }
            if (res == -2)
            {
                printf("cstread lost data crc error , contiue loop\n");
                *flag = 0;
                *lost_flag = 2;

                memset(inv_data, 0, sizeof(Inv_data));
                return -2;
            }
            if (res == -1)
            {
                printf("cstdon't found lost data , contiue loop\n");
                *flag = 0;
                *lost_flag = 0;

                memset(inv_data, 0, sizeof(Inv_data));
                return -1;
            }
        }
        else
        {
            printf("lost_linekkkkkkk------------------------->%d  %d \n", cst_lost_line, *lost_flag);
            printf("cst copy preline %d line \n", cst_lost_line);
            *inv_data = cst_tmp_invdata;
            *flag = 1;
            *lost_flag = 1;
        }
    }
    return 0;
}
