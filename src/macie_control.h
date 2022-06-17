#ifndef MACIE_CONTROL_H
#define MACIE_CONTROL_H
#include <sys/types.h>
#include <sys/socket.h>
#define READY "ready"
#define ASIC_IDLE 0
#define ASIC_ACQ 1
#define ASIC_FAILED -1
#define ASIC_RECONF 2
#include "insthandle.h"
#include "uics_cmds.h"
#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <thread>
#include "f2r.h"
#include "inst_time.h"
#include <unistd.h>
#include "pheader.h"
#include "hxrgerrcode.h"

void upMCD(instHandle *handle,cmd *cc);
void acquisition_eng(instHandle *handle,cmd *cc);
void acquisition_eng(instHandle *handle);
int makeHeader_with_telemetry(MACIE_FitsHdr *pHeaders, instHandle *handle, std::vector<double> v, std::vector<double> c, float *pTlmVals, std::vector<std::string> regname, std::vector<std::string> regdef);
void print_head(instHandle *h);
void close_macie_comm(instHandle *h);
std::string get_fname(instHandle *handle);
void copy_arr(unsigned short **pData,double **dest,long size);
void copy_arr(unsigned short **pData,uint16_t **dest,long size);
bool check_halt(instHandle *handle);
bool check_halt(instHandle *handle,bool silent);
long unique_id();
void set_ESO_header_ramp(instHandle *handle,header *head,long id);
long set_unique_id(instHandle *handle);
int setRegister(instHandle *handle,uint ramp);
int setRegister(instHandle *handle,uint ramp,uint read);
long frameSize(instHandle *handle);
void update_TS_header(instHandle *handle);
void create_TS_folder(instHandle *handle);
double integration_time(instHandle *handle);
void get_error(instHandle *handle);
int wait_for_data(instHandle *handle);
int wait_for_end_of_acq(instHandle *handle);
int set_asic_6900(instHandle *handle,uint state);
int get_asic_6900(instHandle *handle);
//int set_asic_acq(instHandle *handle);
//int  set_asic_idle(instHandle *handle);
void nirps_start(instHandle *handle);
//function for client
void simul_mode(instHandle *handle,cmd *cc);
int trigger_halt(instHandle *handle);
void clocking(instHandle *handle,cmd *cc);
int HxRG(instHandle *handle,int x);
int reconfigure(instHandle *handle);
int clocking(instHandle *handle,bool enhanced);
void initAll(instHandle *handle,cmd *cc);
void powerControl(instHandle *handle,cmd *cc);
void c_idle(instHandle *handle,cmd *cc);
void forced_idle(instHandle *handle);
//void writeRegister_cmd(instHandle *handle,cmd *cc);
std::string checkExt(std::string fname);
void readRegister_cmd(instHandle *handle,cmd *cc);
std::string checkExt(std::string fname);
void SETUP(instHandle *handle,cmd *cc);
void setup_t(instHandle *handle);
uint readBit(uint number,uint k);
int InitializeASIC(instHandle *handle);
int setColdWarmTest(instHandle *handle,uint mode);
void setColdWarmTest(instHandle *handle,cmd *cc);
void delay(int ms);
std::string getTime();
int setNbOutput(instHandle *handle, uint nbOut );
void setNbOutput(instHandle *handle, cmd *cc );
void acq_t(instHandle *handle);
int setFF(instHandle *handle);
int setWindowMode(instHandle *handle);
int getHandle(instHandle *handle);
int getAvailableMacie(instHandle *handle);
int initializeRegister(instHandle *handle);
int setMux(instHandle *handle,uint mux);
int columnDeselect(instHandle *handle,int mode);
void columnDeselect(instHandle *handle,cmd *cc);
void initAll(instHandle *handle,cmd *cc);
int initASIC(instHandle *handle);
int uploadMCD(instHandle *handle,std::string file);
int setRegister(instHandle *handle);
int setGain(instHandle *handle,unsigned int gain);
void setGain(instHandle *handle,cmd *cc);
int makeFolderName(char *folder);
int createFolder(char *folder,char *saveFolder,const char *path);
int sign(int a);
void closeMacie(instHandle *handle,cmd *cc);
//double jd(void);
int makeHeader(MACIE_FitsHdr *pHeaders,instHandle *handle);
//void acquisition(instHandle *handle,int mode);
void acquisition(instHandle *handle,cmd *cc);
void acquisition(instHandle *handle);
void acquisition_sim(instHandle *handle);
uint readRegister(instHandle *handle,std::string regAddr);
int writeRegister(instHandle *handle,std::string regAddr,std::string hexVal);
int writeRegister(instHandle *handle,std::string regAddr,uint hexVal);
int idle(instHandle *handle,uint mode);
void idle(instHandle *handle,cmd *cc);

void setParam(instHandle *handle,cmd *cc);
void write_txt(std::string filename,std::string txt);
std::string join(std::string path,std::string filename);

bool isWrittable(std::string path);
const std::vector<uint16_t> registe_4_header{0x6004,0x6006,0x6002,0x6000,0x6008,0x600a,0x600c,0x600e,
                                             0x6028,0x602a,0x602c,0x602e,0x6030,0x6032,0x6034,0x6036,
                                             0x6005,0x6007,0x6003,0x6001,0x6009,0x600b,0x600d,0x600f,
                                             0x6029,0x602b,0x602d,0x602f,0x6031,0x6033,0x6035,0x6037};
#endif // MACIE_CONTROL_H
