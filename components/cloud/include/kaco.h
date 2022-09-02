/**
 * @file kaco.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-08-02
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _KACO_H
#define _KACO_H

#include "time.h"
#include "string.h"
#include "data_process.h"
#include "Asw_global.h"



int pub_invs_data(Inv_data data, Inv_data meterdata, int flg);
int set_time_from_string(const char *chGetTiem, int tmz);

int asw_kaco_format_csv_header(char *fpBuff, bool isFoundMeter, Stu_KacoCsvHeader *pStuKacoCsv);
// int asw_kaco_format_csv_data(char *fpDataBuf, bool isFoundMeter, Inv_data *invData, Inv_data *meterData);
int asw_kaco_format_csv_data(char *fpDataBuf, bool isFoundMeter, Inv_data *invData, Inv_data *meterData, uint8_t inv_index);
int asw_pub_invs_data(kaco_inv_data_t data, Inv_data meterdata, int flg);

//上传本地文件到ftp服务器，上传成功返回0，失败返回-1
int asw_upload_history_to_ftp(char *localFile);

#endif
