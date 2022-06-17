#ifndef INIT_HANDLER_H
#define INIT_HANDLER_H

#include <iostream>
#include "insthandle.h"
#include "hxrg_init_config.h"
#include <fstream>
#include <sys/types.h>
#include <dirent.h>
#include "macie_control.h"

bool check_folder(std::string path);
bool is_file_exist(std::string fileName);
int check_ext(std::string filename,std::string ext);
int initConfig(instHandle *handle,const char initConfFile[]);
int find_config_file(instHandle *handle,HxRG_init_config::conf_data *CFP,std::string user);
int set_config_file(HxRG_init_config::conf_data *CFP, instHandle *handle);
#endif // INIT_HANDLER_H
