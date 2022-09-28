#include "utility.h"


int HexToStr(unsigned char *pbDest, unsigned char *pbSrc, int srcLen)
{
    int i;
    for (i = 0; i < srcLen; i++)
    {
        char tmp[2] = {0};
        sprintf(tmp, "%02x", pbSrc[i]);
        pbDest[i * 2] = tmp[0];
        pbDest[i * 2 + 1] = tmp[1];
    }
    return srcLen * 2;
}