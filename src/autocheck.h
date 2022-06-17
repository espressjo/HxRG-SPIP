#ifndef AUTOCHECK_H
#define AUTOCHECK_H

#include "hxrg_init_config.h"
//#include "struct.h"
//#include "utils.h"
#include "regdef.h"
#include "insthandle.h"

int checkConf(instHandle *handle,std::string user);
bool bitIsSet(unsigned int a, unsigned int b);
unsigned int readGain(unsigned int value);
unsigned int readOutput(unsigned int value);
int readRegister(instHandle *handle,unsigned short reg,uint *value);
int updateHandle(instHandle *handle);//read ASIC and MACIE to update handle variable such as nbOutput, ....
#endif // AUTOCHECK_H
