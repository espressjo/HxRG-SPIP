#ifndef HXRG_INIT_CONFIG_H
#define HXRG_INIT_CONFIG_H

#include <iostream>
#include <string>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <vector>

class HxRG_init_config
{
public:
    HxRG_init_config();
    ~HxRG_init_config();
    std::ifstream conf_file,init_file;
    int get_param();
    int read_conf(std::string filename);

    void print_structure(void);


    struct conf_data {
        std::string mcd;
        std::string optimized_mcd;
        std::string datapath;
        std::string calpath;

        int gain;
        int read;
        int drop;
        int reset;
        int ramp;
        int col;
        int output;
        int cw;
        int group;
        int idle;
    } parser;
     struct init_data {
        std::string path;
        std::string pathlog;
        std::string pathcfg;
        int framesize;
        double effexptime;
        unsigned int HxRG;
        std::string mcd_asic;
        std::string mrf;
        std::string asic_serial;
        std::string observatory;
        std::string obs_location;
        std::string sca_serial;
        std::string soft_version;
        std::string user;
        double pixrotime;
     } init_parser;

    int verify_configuration(void);
    void get_parsed_data(struct conf_data *p);
    void get_parsed_data(struct init_data *p);
private:
    int parse_val(std::string);
    int parse_init(std::string);
    std::string extension;
    std::string remove_space(std::string words);

};
#endif // HXRG_INIT_CONFIG_H
