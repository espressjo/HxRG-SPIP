#include "autocheck.h"
#include <iostream>
#include "uics.h"

extern Log HxRGlog;

bool bitIsSet(unsigned int a, unsigned int b)
{   //return if a<b> is set where a is hexadecimal value. E.g., a=0x0006, 0x0006<b> = true
    //b starts at 0.
    unsigned int c = 2*b;
    if (b==0){c=1;}
    if ((a&c)==c){
        return true;}
    else
        return false;
}
unsigned int readGain(unsigned int value)
{
    return value&15;
}

unsigned int readOutput(unsigned int value)
{
    return value&63;
}
bool _is_file_exist(std::string fileName)
{
    std::ifstream infile(fileName.c_str());
    return infile.good();
}
int checkConf(instHandle *handle,std::string user)
{   //read the user config file and check if the handle is up to date. If it's not, return -1
    HxRG_init_config config_file_reader;
    HxRG_init_config::conf_data *CFP = new HxRG_init_config::conf_data;
    std::string conf(handle->path.pathConf+user+".conf");
    HxRGlog.writetoVerbose("[autocheck] Starting autocheck...");
    if (!_is_file_exist(conf))
    {
        HxRGlog.writetoVerbose("[autocheck] Configuration file not found for "+user);
        return -1;}

    HxRGlog.writetoVerbose("[autocheck] A configuration file found for "+user);

    config_file_reader.read_conf(handle->path.pathConf+user+".conf");
    config_file_reader.get_param();
    config_file_reader.get_parsed_data(CFP);
    config_file_reader.print_structure();

    unsigned int val;
    int mode = 0;
    //check the gain

    if (readRegister(handle,GAIN,&val)!=0){HxRGlog.writetoVerbose("[autocheck] failed to read register.");return -1;}
    if (readGain(val)!=static_cast<uint>(CFP->gain)){HxRGlog.writetoVerbose("[autocheck] gain NOTOK");return -1;}
    HxRGlog.writetoVerbose("[autocheck] gain OK");
    //check output
    if (readRegister(handle,OUTPUT,&val)!=0){HxRGlog.writetoVerbose("[autocheck] failed to read register.");return -1;}
    if (readOutput(val)!=static_cast<uint>(CFP->output)){HxRGlog.writetoVerbose("[autocheck] output NOTOK");return -1;}
    HxRGlog.writetoVerbose("[autocheck] output OK");
    //checl column mode
    if (readRegister(handle,COL,&val)!=0){HxRGlog.writetoVerbose("[autocheck] failed to read register.");return -1;}
    if (bitIsSet(val,0)==1 && bitIsSet(val,1)==0){mode = 1;}
    else if (bitIsSet(val,0)==0){mode = 0;}
    else if (bitIsSet(val,0)==1 && bitIsSet(val,1)==1){mode = 2;}
    if (mode!=CFP->col){HxRGlog.writetoVerbose("[autocheck] columnMode NOTOK");return -1;}
    HxRGlog.writetoVerbose("[autocheck] columnMode OK");
    //check cw
    if (readRegister(handle,CW,&val)!=0){HxRGlog.writetoVerbose("[autocheck] failed to read register.");return -1;}
    if (static_cast<int>(bitIsSet(val,0))!=CFP->cw){HxRGlog.writetoVerbose("[autocheck] cw NOTOK");return -1;}
    HxRGlog.writetoVerbose("[autocheck] cw OK");

    return 0;
}

int readRegister(instHandle *handle,unsigned short reg,uint *value)
{

    MACIE_STATUS r1 = MACIE_ReadASICReg(handle->card.handle,handle->card.slctASIC, reg, value, false,true);
    if ((r1 != MACIE_OK))
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }

    return 0;
}
int updateHandle(instHandle *handle)
{
    unsigned int val;
//    ASICGAIN=                   12 / 18 dB large Cin
    readRegister(handle,GAIN,&val);
    handle->pDet.gain = readGain(val);
//    NBOUTPUT=                   32 / Number of amplifier used
    readRegister(handle,OUTPUT,&val);
    handle->pDet.nbOutput = readOutput(val);
//    READ    =                    1 / Read number in ramp sequence
    readRegister(handle,RREAD,&val);
    handle->pAcq.read = val;
//    RAMP    =                    1 / Ramp sequence number
    readRegister(handle,RRAMP,&val);
    handle->pAcq.ramp = val;
//    RESET    =
    readRegister(handle,RESET,&val);
    handle->pAcq.reset = val;
//    DROP    =
    readRegister(handle,DROP,&val);
    handle->pAcq.drop = val;
//    GROUP    =
    readRegister(handle,GROUP,&val);
    handle->pAcq.group = val;
//    WARMTST =                    0 / 0- cold test; 1- warm test
    readRegister(handle,CW,&val);
    std::cout<<"CW: "<<val<<std::endl;
    handle->head.coldWarmMode = static_cast<int>(bitIsSet(val,0));
//    COLUMN  =                    0 / 0->H4RG-10, 1->H4RG-15 Col.Disa., 2->H4RG-15 Co
    readRegister(handle,COL,&val);
    if(bitIsSet(val,0)==0){handle->head.columnMode = 0;}
    else if (bitIsSet(val,0) && bitIsSet(val,1)==0) {handle->head.columnMode = 1;}
    else if (bitIsSet(val,0) && bitIsSet(val,1)) {handle->head.columnMode = 2;}
//    WINDOW  =                    0 / 0->full frame; 1-> window mode
    readRegister(handle,EXPMODE,&val);
    handle->head.windowMode = static_cast<int>(bitIsSet(val,1));

//    MCDOPT  = '/opt/HxRG-SERVER/mcd/HxRG_Main.mcd' / optimized mcd
//??

    return 0;
}

/*

ASICGAIN=                   12 / 18 dB large Cin
AEEXPT  =               5.5733 / Average effective exposure time
ACQDATE = '2020-04-08'         / Acquisition date
ASIC_NUM= '157     '           / ASIC serial number
SCA_ID  = '18859   '           / SCA number
MUXTYPE =                    4 / 1- H1RG; 2- H2RG; 4- H4RG
NBOUTPUT=                   32 / Number of amplifier used
HXRGVER = '1.3.1   '           / acquisition software version number
READ    =                    1 / Read number in ramp sequence
RAMP    =                    1 / Ramp sequence number
JULIAN  = '2458948.149282'     / Julian date taken before the file is written on
SEQID   =          -2068111430 / Unique Identification number for the sequence
WARMTST =                    0 / 0- cold test; 1- warm test
COLUMN  =                    0 / 0->H4RG-10, 1->H4RG-15 Col.Disa., 2->H4RG-15 Co
WINDOW  =                    0 / 0->full frame; 1-> window mode
XSTART  =                    0 / start pixel in X
XSTOP   =                 4096 / stop pixel in X
MCDOPT  = '/opt/HxRG-SERVER/mcd/HxRG_Main.mcd' / optimized mcd
OPTADDR = '0x0000  '           / address beeing tested
OPTVAL  = '0       '           / value beeing tested
        */
