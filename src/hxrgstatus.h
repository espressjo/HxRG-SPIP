#ifndef HXRGEXPSTATUS_H
#define HXRGEXPSTATUS_H

enum hxrgStatus {
                hxrgEXP_NONE = 0,
                hxrgEXP_INACTIVE = 1,
                hxrgEXP_PENDING = 2,
                hxrgEXP_INTEGRATING = 4,
                hxrgEXP_READING = 16,
                hxrgEXP_PROCESSING = 32,
                hxrgEXP_TRANSFERRING = 64,
                hxrgEXP_COMPL_SUCCESS = 128,
                hxrgEXP_COMPL_FAILURE = 256,
                hxrgEXP_COMPL_ABORTED = 512
                };
enum hxrgFITSMTD {
                hxrgFITSMTD_NONE = 0,
                hxrgFITSMTD_COMPRESSED = 1,
                hxrgFITSMTD_UNCOMPRESSED = 2,
                hxrgFITSMTD_BOTH = 3
                };
#endif // HXRGEXPSTATUS_H
