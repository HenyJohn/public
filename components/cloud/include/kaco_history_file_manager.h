/**
 * @file kaco_history_file_manager.h
 * @author TGL (you@domain.com)
 * @brief
 * @version 1.0.1
 * @date 2022-08-27
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _KACO_HISTORY_FILE_MANAGER_H
#define _KACO_HISTORY_FILE_MANAGER_H
#include "Asw_global.h"


void asw_printf_filelist();
void asw_printf_file_csv(char *pFileName);



int asw_kaco_get_filename(char *pFileName, char *dir);
int asw_kao_delete_fileName_from_path(char *fileName, char *dir);


#endif

#if 0
//从历史文件列表中删除文件名 fileNamel，返回值 0--成功，非0--失败
int asw_delete_fileName_from_historyfiles(char *fileName);

//从历史文件列表中添加文件名 fileNamel，返回值 0--成功，-1--失败, 1--已存在？？ 
int asw_add_fileName_to_historyfiles(char *fileName);

//从历史数据文件列表中获取
int asw_get_fileName_from_historyfiles(char *fileName);

int asw_has_history_data();

int asw_get_history_info(char *pChrFileName);


#endif