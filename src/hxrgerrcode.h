#ifndef HXRGERRCODE_H
#define HXRGERRCODE_H

typedef enum hxrgCMD_ERR_CODE
{
    hxrgCMD_ERR_UNKNOWN_KEYW = 1,
    hxrgCMD_ERR_VALUE,
    hxrgCMD_ERR_PARAM_FORMAT,
    hxrgCMD_ERR_PARAM_VALUE,
    hxrgCMD_ERR_NOT_SUPPORTED,
    hxrgCMD_ERR_PARAM_UNKNOWN,
    hxrgCMD_ERR_NO_FILENAME
} hxrgCMD_ERR_CODE;

#endif // HXRGERRCODE_H
