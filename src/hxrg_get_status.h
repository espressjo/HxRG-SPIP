#ifndef HXRG_GET_STATUS_H
#define HEXRG_GET_STATUS_H

#include "uics_base64.h"
#include <string>
#include "insthandle.h"
#include "uics_cmds.h"
#include "hxrgstatus.h"
#include "hxrgerrcode.h"

typedef struct hxrgSTATUS
{
    // *** Revision number
    int64_t    revNum;

    // ***  Is initialized
    int64_t    isInitialized;

    // *** Exposure status
    int64_t     expStatus;
    int64_t     countDown; //temps restant en seconde (rampe)
    int64_t     expTime; //temps d'integration reçu ramp*read*5.57ss
    int64_t     dataReady;//0-> no data ready, 1-> data ready
    int64_t     isBusy;//is in an acquisition
    // *** Setup
    double      dit;
    int64_t     nDit;
    int64_t     ndSamples;
    char frameType[32];
    int64_t gain;
    int64_t warmMode;//0->cold, 1->warm

    // *** Exposure file
    int64_t fileFormat;
    char fileName[128];

    // *** Exposure meter
    int64_t expoMeterCount;
    double expoMeter[2];
    char expoMeterMask[2][32];

} hxrgSTATUS;



void send_status(instHandle *handle);
void send_data(instHandle *handle);
#endif // HXRG_GET_STATUS_H
