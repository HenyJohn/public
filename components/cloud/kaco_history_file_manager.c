
#include <dirent.h>
#include "data_process.h"
#include "asw_fat.h"
#include "kaco_history_file_manager.h"

#define HISTORY_FILELIST_NAME "/inv/hstfiles.csv"
#define HISTORY_TMP_NAME "/inv/tmp.csv"



//"/inv"
int asw_kaco_get_filename(char *pFileName, char *dir)
{
    assert(pFileName != NULL);
    assert(dir != NULL);
    DIR *theFolder = opendir((const char *)dir);
    struct dirent *next_file;
    char filepath[300];


    ESP_LOGI("--Kaco File Manager --"," get %s-%s historyfile is begin...",dir,pFileName);

    while ((next_file = readdir(theFolder)) != NULL)
    {
        // build the path for each file in the folder
        if (strcmp(next_file->d_name, ".") == 0 || strcmp(next_file->d_name, "..") == 0)
            continue;
        sprintf(filepath, "%s/%s", dir, next_file->d_name);
        printf("%s\n", filepath);

        if (strcmp(filepath, pFileName) == 0)
        {
            ESP_LOGI("--KACO GetHistory File --", "Get the history file about %s", pFileName);
            closedir(theFolder);

            return 1;
        }
    }
    closedir(theFolder);

    ESP_LOGW("-- Kaco get Filename --", " No found the filename is:%s", pFileName);

    return 0;
}

int asw_kao_delete_fileName_from_path(char *fileName, char *dir)
{

    assert(fileName != NULL);
    // assert(dir != NULL);
    // char filePathName[100] = {0};
    // sprintf(filePathName, "%s/%s", dir, fileName);

    if (remove(fileName) == 0)
    {
        ESP_LOGI("--Kaco Del file --", "%s be deleted.", fileName);
    }

    else
    {
        ESP_LOGW("--Kaco Del file --", "%s be deleted. ERRO", fileName);
    }

    return 0;
}
/////////////////////////////////////////////////////////////////////////////

void asw_printf_filelist()
{
    FILE *fp = NULL;

    int res = 0;

    fp = fopen(HISTORY_FILELIST_NAME, "r+"); //"wb"

    if (fp == NULL)
    {
        ESP_LOGE("-- Kaco OPEN FileName  ERRO -- ", "Failed to open hisotry filelist.");
        return;
    }
    ///////////////////////////////////////
    char fileName1[12][70];
    int i = 0;

    while (!feof(fp))
    {

        if (fgets(fileName1[i], sizeof(fileName1[i]), fp) != NULL)
            printf("%s", fileName1[i]);
        else
            break;
        i++;
    }
    fclose(fp);

    for (uint8_t j = 0; j < i; j++)

        asw_printf_file_csv(fileName1[j]);

    //////////////////////////////////////
}

void asw_printf_file_csv(char *pFileName)
{
    FILE *fp = NULL;

    int res = 0;

    fp = fopen(pFileName, "r+"); //"wb"

    if (fp == NULL)
    {
        ESP_LOGE("-- Kaco OPEN CSV FileName  ERRO -- ", "Failed to open CSV %s file.", pFileName);
        return;
    }
    ///////////////////////////////////////
    char fileName_content[200];
    printf("\n======== printf %s file content================\n", pFileName);

    while (!feof(fp))
    {
        if (fgets(fileName_content, sizeof(fileName_content), fp) != NULL)
            printf("%s", fileName_content);
        else
            break;
    }
    printf("\n======== printf %s file content================\n\n", pFileName);
    fclose(fp);
    //////////////////////////////////////
}

#if 0

int asw_get_history_info(char *pChrFileName)
{
    FILE *fp = NULL;
    int res = 0;

    assert(pChrFileName != NULL);

    fp = fopen(HISTORY_FILELIST_NAME, "r+"); //"wb"

    if (fp == NULL)
    {
        ESP_LOGE("-- Kaco OPEN FileName  ERRO -- ", "Failed to open hisotry filelist.");
        return 0;
    }

    struct stat statbuf;
    stat(pChrFileName, &statbuf);
    int size = statbuf.st_size;

    if (size < 7)
    {
        remove(HISTORY_FILELIST_NAME);
        return 0;
    }
    ///////////////////////////////////////
    char fileName[70];
    while (!feof(fp))
    {

        if (fgets(fileName, sizeof(fileName), fp) != NULL)
        {
            // if (strcmp(pChrFileName, fileName) == 0)
            if (strstr(pChrFileName, fileName) == NULL)
            {
                ESP_LOGI("--KACO GetHistory File --", "Get the history file about %s", fileName);
                fclose(fp);

                return 1;
            }
        }
        else
            break;
    }

    fclose(fp);

    return 0;
}

int asw_has_history_data()
{
    if (access(HISTORY_FILELIST_NAME, 0) != 0)
    {
        ESP_LOGW("-- Kaco Read History FileList Info -- ", " hisotry filelist not exited. ");
        return 0;
    }
    else
    {
        ESP_LOGW("-- Kaco Read History FileList Info -- ", " hisotry filelist  exited. ");
        return 1;
    }
}
/**
 * @brief  从历史文件列表中删除文件名 fileName
 *
 * @param fileName
 * @return int  返回值 >=0剩余行数 -1,失败
 */
int asw_delete_fileName_from_historyfiles(char *fileName)
{
    char fileNameGet[70];
    bool bGetFileName = 0;
    int mLineNum = 0;

    if (access(HISTORY_FILELIST_NAME, 0) != 0)
    {
        ESP_LOGW("-- Kaco Del FileName  WARN -- ", " hisotry filelist not exited. ");
        return 0;
    }

    FILE *fp = fopen(HISTORY_FILELIST_NAME, "r+"); //"wb"
    FILE *tfp = fopen(HISTORY_TMP_NAME, "w+");
    if (fp == NULL || tfp == NULL)
    {
        ESP_LOGE("-- Kaco Del FileName  ERRO -- ", "Failed to open hisotry filelist.");
        return -1;
    }
    /* 获取文件大小处理 */
    fseek(fp, 0, SEEK_END);
    int file_len = ftell(fp);

    printf("\n================ file:%s size :%d ========", HISTORY_FILELIST_NAME, file_len);
    // --- 0 删除文件，并返回 0
    if (file_len <= 7)
    {
        close(fp);
        remove(HISTORY_FILELIST_NAME);
        ESP_LOGW("-- Kaco Dele Hisoty FileList Warn --", " FileList is empty,remove the file.");
        return 0;
    }
    fseek(fp, 0, SEEK_SET);
    bGetFileName = 0;
    // --- 非0，读取文件，获取文件列表，找到fileName,删除，在写入文件
    while (!feof(fp))
    {
        if (fgets(fileNameGet, sizeof(fileNameGet), fp) == NULL)
            break;

        printf("-----DDDD--- %s", fileNameGet);
        // if (strcmp(fileNameGet, fileName) != 0)
        if (strstr(fileNameGet, fileName) != NULL)
        {
            // fwrite(fileNameGet, sizeof(fileNameGet), 1, tfp);
            // int mSize = fprintf(tfp, fileNameGet); //TODO

            fputs(fileNameGet, tfp);
            mLineNum++;
        }
        else
        {
            // rewind(fp);
            remove(fileName); //删除本地文件
            bGetFileName = 1;
            ESP_LOGI("--Kaco Del  FileName form History FileList info--", "Sucess Got the FileName:%s.", fileName);
        }
    }

    fclose(fp);
    fclose(tfp);

    if (!bGetFileName)
    {

        remove(HISTORY_TMP_NAME);
        ESP_LOGW("-- Kaco Del FileName form History FileList Warn --", "Failed found the fileName:%s", fileName);
        return 0;
    }

    int res = remove(HISTORY_FILELIST_NAME);
    int res1 = rename(HISTORY_TMP_NAME, HISTORY_FILELIST_NAME);

    printf("\n---------- KACO DELETE FILENAME DEBUG INFO---------------\n");
    printf("remove return value:%d,rename return value:%d.[0--right,-1--error]", res, res1);
    printf("\n---------- KACO DELETE FILENAME DEBUG INFO---------------\n");

    return mLineNum;
}



/**
 * @brief 从历史文件列表中添加文件名 fileName
 *
 * @param fileName
 * @return int  返回值 0--成功,>0文件大小，-1--失败, 1--已存在
 */
int asw_add_fileName_to_historyfiles(char *fileName)
{
    FILE *fp = NULL;

    int res = 0;

    fp = fopen(HISTORY_FILELIST_NAME, "a+"); //"wb"

    if (fp == NULL)
    {
        ESP_LOGE("-- Kaco Add FileName  ERRO -- ", "Failed to open hisotry filelist.");
        return -1;
    }

    //写入文件，成功返回0，失败返回-1 TODO
    // int mSize = fwrite(fileName, sizeof(fileName), 1, fp);

    fputs(fileName, fp);
    fputs("\r\n", fp);

    fclose(fp);
    return 0;
}



/**
 * @brief 从历史数据文件列表中获取第index(1)个文件名
 *
 * @param index
 * @return int 0:success
 */
int asw_get_fileName_from_historyfiles(char *fileName)
{

    assert(fileName != NULL);
    if (access(HISTORY_FILELIST_NAME, 0) != 0)
    {
        ESP_LOGE("-- Kaco Read History FileList ERRO -- ", " hisotry filelist not exited. ");
        return -1;
    }

    FILE *fp = fopen(HISTORY_FILELIST_NAME, "r+"); //"wb"
    if (fp == NULL)
    {
        ESP_LOGE("-- Kaco Read History FileList ERRO -- ", "Failed to open hisotry filelist.");
        perror("--- Failed to open hisotry filelist. ----");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    int file_len = ftell(fp);

    // --- 0 删除文件，并返回NULL
    if (file_len <= 7)
    {
        close(fp);
        remove(HISTORY_FILELIST_NAME);
        ESP_LOGW("-- Kaco Read Hisoty FileList Warn --", " FileList is empty,remove the file.");
        return -1;
    }
    // --- 非0，读取文件，获取文件列表，返回第1个文件名内容
    fseek(fp, 0, SEEK_SET);

    if (fgets(fileName, 70, fp) != NULL)
    {
        fclose(fp);
        printf("\n-- Success,get fileName:%s from history filelists.\n", fileName);
        return 0;
    }
    else
    {
        fclose(fp);
        ESP_LOGE("-- Kaco Read Hisoty FileList ERRO --", " Failed get filelist filename.");
        return -1;
    }
}

#endif