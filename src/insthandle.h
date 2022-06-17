#ifndef INSTHANDLE_H
#define INSTHANDLE_H

#include "states.h"
#include "macie.h"
#include <stdint.h>
#include <string>
#include "hxrgstatus.h"

typedef struct{
    unsigned short numCards;
    MACIE_CardInfo *pCard;
    unsigned long handle;
    MACIE_Connection connection;
    unsigned char avaiMACIEs;
    unsigned char slctMACIEs;
    unsigned char avaiASICs;
    unsigned char slctASIC;
    unsigned int val;
    MACIE_STATUS bRet;
    int haltTriggered;
    int asBeenInit; //0-> never initialized,
                    //1-> MACIE_INIT() has been called, this is true after the 1st few seconds of init
                    //2-> the whole init sequence is done and successfull.
    int isReconfiguring;//0-> not reconfiguring
                        //1-> is currently reconfiguring
}cards;
typedef struct{
    bool toponly;
    bool oddeven;
    bool bias;
    bool nl;
    bool nl_status;
    bool bias_status;
    std::string bias_uid;
    std::string nl_uid;

}fits2rampconf;
typedef struct {
    unsigned short *pData;
    uint gain;
    uint HxRG;
    float readoutTime;//in ms
    float effExpTime;
    int frameX;
    int frameY;
    uint nbOutput;
    uint preampInputScheme;//(1) Use InPCommon as V1,V2,V4 against InP on V3 (h4502 and h45c2 for averaged channels, Use input configuration based on bit <0> of this register
    uint preampInputVal;
    uint idle;
}parameterDet;
typedef struct {
    uint ramp;
    uint read;
    uint reset;
    uint drop;
    uint group;
    //uint idleReset;
    std::string savePath;
    std::string calpath;
    uint winStartX;
    uint winStartY;
    uint winStopY;
    uint winStopX;
    int isWindow;
    int isOngoing;
    std::string posemeter_mask;
    std::string posemeter_mask_optional;
    double flux;
    double flux_optional;
    int simulator;//mode 0-> normal acquisition, mode 1-> simulator
}paramterAcq;
typedef struct{
  std::string pathPython;
  std::string pathMCD;
  std::string pathData;
  std::string pathLog;
  std::string pathConf;
  std::string pathRacine;
  std::string pathReg;
  std::string serie_path;
}Path;
typedef struct{
    std::string mcdASIC;
    std::string mcdOpt;
    std::string mrf;
}MCD;

typedef struct {
std::string OBJET;
std::string ACQDATE;
std::string JD;
std::string timeLocal;
std::string timeGMT;
int READ;
int RAMP;
long ID;
int nbOut;
std::string OBSERVATEUR;
std::string OBSERVATORY;
std::string OBSLOCATION;
std::string NAMESEQUENCE;
std::string ASICSERIAL;
std::string SCASERIAL;
std::string SOFTVERSION;
std::string UNITS;

int coldWarmMode;
float MASTERCLOCK;
int SLOWMODE;

int windowMode;//0->fullframe, 1->window
int columnMode;
int preAmpCap;
int preAmpFilter;
std::string type;
int numBlock;
}HEADER;


struct optimization{
  std::string addr;
  std::string val;
};
typedef struct
{
    int keepGoing;
    int sockfd;
    hxrgStatus status;
    std::string name;
    bool fitsMTD;
    int expoMeterCount;

}nirps_acq;
typedef struct{

    STATE nextState;
    STATE state;
    cards card;
    HEADER head;
    parameterDet pDet;
    paramterAcq pAcq;
    Path path;
    MCD mcd;
    nirps_acq nAcq;
    void* im_ptr;//image pointer ()
    size_t imSize;
    bool glitch_test;
    bool save_glitch;
    bool readtelemetry;
    float macie_libv;
    fits2rampconf f2rConfig;
    bool im_ready;
    std::string user;
} instHandle;

#endif // INSTHANDLE_H
