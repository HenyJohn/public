/**
 * @file asw_protocol_ftp.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-08-18
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _ASW_PROTOCOL_FTP_H
#define _ASW_PROTOCOL_FTP_H

#include "Asw_global.h"
#include "FtpClient.h"

void asw_ftp_init();

// void asw_ftp_create_file(char *psn, char *content);
// int asw_ftp_upload();
// int get_ftp_filelist_size();

/* asw-kaco */

int asw_ftp_connect();
void asw_ftp_close();
int asw_ftp_upload(char *psn, char *content, int len);


//上传文件名fileName的csv文件到ftp 服务器
int asw_ftp_upload_file_byName(char *localFileName,char *ftpFileName);

#endif