#include "argument_parser.h"

parser::parser (int argc, char *argv[]) {
    char arg[128],value[128];
    std::string aa,vv;
    nb_of_args = argc-1;
    for (int i = 1; i < argc; ++i) {
            vv = "none";
            aa = "";
            if (parser::value(argv[i]))
            {
                parser::split(argv[i],&aa,&vv);
                args.push_back(aa);
                arg_vals.push_back(vv);
            }
            else {
                args.push_back(std::string(argv[i]));
                arg_vals.push_back(vv);
            }

        }
}
bool parser::value(std::string arg)
/*
 * return wether or not there is a value with the argument.
 */
{
    for (auto &c: arg)
    {
        if (c=='=')
        {
            return true;
        }
    }
    return false;
}
void parser::split(std::string txt,std::string *arg,std::string *val)
/*
 * split arg=value pair. It will populate arg and val.
 */
{
    char buffer[128],a[128],v[128];
    memset(buffer,0,128);
    memset(a,0,128);
    memset(v,0,128);
    int i=0;
    for (auto &c:txt) {

        if (c=='=')
        {
            buffer[i]= ' ';
        }
        else {
            buffer[i]=c;
        }
    i++;
    }
    sscanf(buffer,"%s %s",a,v);
    *arg = a;
    *val = v;
}

void parser::print(void)
/*
 * print all argument and ther evalues if any
 */
{
    for (int i=0;i<nb_of_args;i++)
    {
        std::cout<<args[i]<< ": "<<arg_vals[i]<<std::endl;
    }
}
//bool isarg(std::string);
//std::string get(std::string arg);
std::string parser::get(std::string arg)
/*
 * return the value of argument arg
 */
{
    for (int i=0;i<nb_of_args;i++) {
        if (args[i].compare(arg)==0)
        {
            return arg_vals[i];
        }
    }
    return std::string("");
}
bool parser::isarg(std::string arg)
/*
 * return wether the argument as been parsed or not
 */
{
    for (int i=0;i<nb_of_args;i++) {
        if (args[i].compare(arg)==0)
        {
            return true;
        }
    }
    return false;
}
void parser::helper()
{
    std::cout<<"\n\t:::: HxRG-SERVER help ::::\n"<<std::endl;
    std::cout<<"--linearize:\tApply non-linearity correction. (/cal/path/nonlin.fits must exist, otherwise nonlinearity is not performed)"<<std::endl;
    std::cout<<"--loglevel:\tlevel of log. \n\t(0) default log, \n\t(1) log UICS mostly msg handler\n\t(2) log uics cmds, sndMsg and state handler, \n\t(3) all UICS socket errors."<<std::endl;
    std::cout<<"--keep_gfile:\tSave glitch test file. (file called glitch.fits)"<<std::endl;
    std::cout<<"--no_glitch:\tDo not perform glitch test. Will init faster"<<std::endl;
    std::cout<<"--telemetry:\tPrint the Vbias power telemetry after initialization."<<std::endl;
    std::cout<<"--data_path:\tDefine a new save path for data. Will be override by config file."<<std::endl;
    std::cout<<"--cal_path:\tDefine new path for calibration file. Will be override by config file."<<std::endl;
    std::cout<<"\n"<<std::endl;
}
