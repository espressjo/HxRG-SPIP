#include "hxrg_init_config.h"

HxRG_init_config::HxRG_init_config()
{
    parser={"","","","",0,1,0,1,1,0,32,0,0,1};
    init_parser={"","","",0,0,0,"","","","","","","","",0};
    extension = "";

}

int HxRG_init_config::read_conf(std::string filename)
{   //gonna accept two type of file .init or .conf


    for (std::string::iterator it=filename.end()-5;it!=filename.end();it++)
    {extension+=*it;}

    if (extension.compare(".init")==0)
    {
        init_file.open(filename);
        if (not init_file.is_open()){return -1;}
        return 0;
    }
    else if (extension.compare(".conf")==0)
    {

        conf_file.open(filename);
        if (not conf_file.is_open()){return -1;}
        return 0;
    }
    else {

        return -1;
    }
}
int HxRG_init_config::get_param()
{
    std::string buff;
    if (extension.compare(".conf")==0){

    while(getline(conf_file,buff))
    {
        if (parse_val(buff)!=0){

            return -1;
        }

    }
    return 0;}
    else if (extension.compare(".init")==0) {
        while(getline(init_file,buff))
        {
            if (parse_val(buff)!=0){

                return -1;
            }

        }
        return 0;
    }

    return -1;
}

void HxRG_init_config::print_structure()
{

    if (extension.compare(".conf")==0){
        std::cout<<""<<std::endl;
    std::cout<<":::::::::::: Config file ::::::::::::::"<<std::endl;
    std::cout<<"\tGain: "<<parser.gain<<std::endl;
    std::cout<<"\tRead: "<<parser.read<<std::endl;
    std::cout<<"\tReset: "<<parser.reset<<std::endl;
    std::cout<<"\tDrop: "<<parser.drop<<std::endl;
    std::cout<<"\tRamp: "<<parser.ramp<<std::endl;
    std::cout<<"\tCol. Deselect: "<<parser.col<<std::endl;
    std::cout<<"\tOutput Amp: "<<parser.output<<std::endl;
    std::cout<<"\tOpt. MCD: "<<parser.optimized_mcd<<std::endl;
    std::cout<<"\tC/W Mode: "<<parser.cw<<std::endl;
    std::cout<<"\tGroups: "<<parser.group<<std::endl;
    std::cout<<"\tIdle Mode: "<<parser.idle<<std::endl;
    std::cout<<"\tData Path: "<<parser.datapath<<std::endl;
    std::cout<<"\tCalibration Path: "<<parser.calpath<<std::endl;}
    else if (extension.compare(".init")==0) {
        std::cout<<""<<std::endl;
        std::cout<<":::::::::::: Config file ::::::::::::::"<<std::endl;
        std::cout<<"\tFrame size: "<<init_parser.framesize<<std::endl;
        std::cout<<"\tPix read out time: "<<init_parser.pixrotime<<std::endl;
        std::cout<<"\tEffective exp. time: "<<init_parser.effexptime<<std::endl;
        std::cout<<"\tHxRG: "<<init_parser.HxRG<<std::endl;
        std::cout<<"\tAsic serial: "<<init_parser.asic_serial<<std::endl;
        std::cout<<"\tObservatory: "<<init_parser.observatory<<std::endl;
        std::cout<<"\tObservatory location: "<<init_parser.obs_location<<std::endl;
        std::cout<<"\tSAC serial: "<<init_parser.sca_serial<<std::endl;
        std::cout<<"\tMRF: "<<init_parser.mrf<<std::endl;
        std::cout<<"\tPath: "<<init_parser.path<<std::endl;
        std::cout<<"\tPath log: "<<init_parser.pathlog<<std::endl;
        std::cout<<"\tPath config: "<<init_parser.pathcfg<<std::endl;
        std::cout<<"\tASIC mcd: "<<init_parser.mcd_asic<<std::endl;
        std::cout<<"\tSoftware Version: "<<init_parser.soft_version<<std::endl;
        }
std::cout<<":::::::::::::::::::::::::::::::::::::::::::\n"<<std::endl;
}
int HxRG_init_config::parse_val(std::string line)
{   std::vector<std::string> list;
    std::string buff("");
    unsigned char comment=0;
    for(char& c : line)
    {
        if (c=='#'){buff="";comment=1;break;}
        if (c==' ' || c=='\t')
        {
            list.push_back(buff);
            buff.clear();
        }
        buff+=c;
    }
    if (comment!=1){
    list.push_back(buff);}
    comment=0;

    if (list.size()>2)

    {
        for (size_t i=2;i<list.size();i++){

        list[1]+=list[i];

        }
    }

    if (extension.compare(".conf")==0 && list.size()>1){
    if (list[0].compare("GAIN")==0){parser.gain = std::atoi(list[1].c_str());}
    if (list[0].compare("RESET")==0){parser.reset = std::atoi(list[1].c_str());}

    if (list[0].compare("READ")==0){parser.read = std::atoi(list[1].c_str());}
    if (list[0].compare("DROP")==0){parser.drop = std::atoi(list[1].c_str());}
    if (list[0].compare("RAMP")==0){parser.ramp = std::atoi(list[1].c_str());}
    if (list[0].compare("COLDESELECT")==0){parser.col = std::atoi(list[1].c_str());}
    if (list[0].compare("OUTPOUT")==0){parser.output = std::atoi(list[1].c_str());}
    if (list[0].compare("OPTMCD")==0){parser.optimized_mcd = remove_space(list[1].data());}
    if (list[0].compare("GROUP")==0){parser.group = std::atoi(list[1].c_str());}
    if (list[0].compare("IDLE")==0){parser.idle = std::atoi(list[1].c_str());}
    if (list[0].compare("CW")==0){parser.cw = std::atoi(list[1].c_str());}
    if (list[0].compare("DATAPATH")==0){parser.datapath = remove_space(list[1].data());}
    if (list[0].compare("CALPATH")==0){parser.calpath = remove_space(list[1].data());}
    return 0;}
    else if (extension.compare(".init")==0 && list.size()>1) {
        if (list[0].compare("FRAMESIZE")==0){init_parser.framesize = std::atoi(list[1].c_str());}
        if (list[0].compare("PIXRO")==0){init_parser.pixrotime = std::atof(list[1].c_str());}
        if (list[0].compare("EFFEXPTIME")==0){init_parser.effexptime = std::atof(list[1].c_str());}
        if (list[0].compare("HxRG")==0){init_parser.HxRG = std::atoi(list[1].c_str());}
        if (list[0].compare("ASICSERIAL")==0){init_parser.asic_serial = remove_space(list[1].data());}
        if (list[0].compare("MRF")==0){init_parser.mrf = remove_space(list[1].data());}
        if (list[0].compare("OBSERVATORY")==0){init_parser.observatory = remove_space(list[1].data());}
        if (list[0].compare("OBSLOCATION")==0){init_parser.obs_location = remove_space(list[1].data());}
        if (list[0].compare("SCASERIAL")==0){init_parser.sca_serial = remove_space(list[1].data());}
        if (list[0].compare("PATH")==0){init_parser.path = remove_space(list[1].data());}
        if (list[0].compare("PATHCFG")==0){init_parser.pathcfg = remove_space(list[1].data());}
        if (list[0].compare("PATHLOG")==0){init_parser.pathlog = remove_space(list[1].data());}
        if (list[0].compare("MCDASIC")==0){init_parser.mcd_asic = remove_space(list[1].data());}
        if (list[0].compare("SOFTV")==0){init_parser.soft_version = remove_space(list[1].data());}
        if (list[0].compare("USER")==0){init_parser.user = remove_space(list[1].data());}

        return 0;
    }
    return 0;
}
void HxRG_init_config::get_parsed_data(struct conf_data *p)
{
    *p = parser;
}
void HxRG_init_config::get_parsed_data(struct init_data *p)
{
    *p = init_parser;
}
int HxRG_init_config::verify_configuration()
{
    if (extension.compare(".init")==0)
    {std::cout<<"verification is useless with .init file."<<std::endl;}
    int error=0;

    if (parser.gain<0 || parser.gain>15){
        error+=1;
        parser.gain=0;}
    if (parser.reset<0){
        error+=1;
        parser.reset=1;}
    if (parser.drop<0){
        error+=1;
        parser.drop=0;}
    if (parser.ramp<1){
        error+=1;
        parser.ramp=1;}
    if (parser.col<0 || parser.col>2){
        error+=1;
        parser.col=1;}
    if (parser.output!=1 || parser.output!=4 || parser.output!=32){
        error+=1;
        parser.output=32;}
    if (parser.idle<0 || parser.idle>2){
        error+=1;
        parser.idle=1;}
    if (parser.cw<0 || parser.cw>1){
        error+=1;
        parser.cw=0;}



return error;


}
std::string HxRG_init_config::remove_space(std::string words)
{   std::string buff("");

    for (char& c : words)
    {
        if (c!=' ' && c!='\t')
        {buff+=c;}
    }
    return buff;
}
HxRG_init_config::~HxRG_init_config()
{
    if (conf_file.is_open()){
    conf_file.close();}
    if (init_file.is_open()){
    init_file.close();}
}
