#include "init_handler.h"
#include "uics.h"
#include "hxrgstatus.h"

extern Log HxRGlog;

bool is_file_exist(std::string fileName)
{
    std::ifstream infile(fileName.c_str());
    return infile.good();
}

bool check_folder(std::string path)
{
    DIR* dir = opendir(path.c_str());
    if (dir) {
        /* Directory exists. */
        closedir(dir);
        return true;
    } else if (ENOENT == errno) {
        return false;
    } else {
        return false;
    }
}
int initConfig(instHandle *handle,const char initConfFile[])//"/home/initMacie/HxRG.init"
/*!
  *\brief Initialize instrument Handle with some config file.
  * Config file can be found here:
  */
{


    HxRG_init_config config_file_reader;
    HxRG_init_config::init_data init;

    if (config_file_reader.read_conf(initConfFile)!=0){return -1;}
    config_file_reader.get_param();
    config_file_reader.get_parsed_data(&init);
    //verify si 4096, 1024 ou 2048 (H1RG,H2RG ou H4RG)
    if (init.framesize!=1024 && init.framesize!=2048 && init.framesize!=4096){std::cout<<"verify HxRG.init file [FRAMESIZE]"<<std::endl;return -1;}
    handle->pDet.frameX = init.framesize;
    handle->pDet.frameY = init.framesize;
    if (init.path.empty()){std::cout<<"verify HxRG.init file [PATH]"<<std::endl;return -1;}
    if (init.path[0]!='/'){std::cout<<"verify HxRG.init file [PATH]"<<std::endl;return -1;}
    if (init.path[init.path.size()-1]!='/'){init.path+='/';}
    if (!check_folder(init.path)){std::cout<<"[Warning] "<<init.path<<" does not exist"<<std::endl;}
    //populate the path
    handle->pDet.preampInputScheme = 0001;// UTR sampling
    if (init.pathlog.compare("")!=0)
    {
        if (init.pathlog[init.pathlog.size()-1]!='/')
        {
            init.pathlog+='/';
        }
        handle->path.pathLog = init.pathlog;

    }
    else {
        handle->path.pathLog = init.path+"log/";
    }
    handle->path.pathMCD = init.path+"mcd/";
    handle->path.pathReg = init.path+"register/";
    if (init.pathcfg.compare("")!=0)
    {
        if (init.pathcfg[init.pathcfg.size()-1]!='/')
        {
            init.pathcfg+='/';
        }
        handle->path.pathConf = init.pathcfg;

    }
    else {
        handle->path.pathConf = init.path+"config/";
    }
    handle->path.pathData = init.path+"data/";
    handle->path.pathRacine = init.path+"data/";
    handle->path.pathPython = init.path+"python/";
    handle->path.serie_path = "";
    handle->card.isReconfiguring = 0;
    handle->pAcq.savePath = init.path+"data/";
    handle->glitch_test = true;//by default we perform glitch test
    handle->save_glitch = false;//by default we do not save the glitch test file
    handle->readtelemetry = false;//by default do not read telemetry at the end of initialization
    handle->macie_libv = 0.0;//MACIE LIB version
    handle->pDet.effExpTime = static_cast<float>(init.effexptime);
    handle->pDet.HxRG = init.HxRG;
    handle->pDet.readoutTime = static_cast<float>(init.pixrotime);

    handle->mcd.mrf = init.mrf;
    handle->mcd.mcdASIC = init.mcd_asic;
    handle->im_ptr = nullptr;
    handle->imSize = 0;
    handle->pAcq.simulator = 0;
    handle->head.numBlock = 1;
    handle->head.type = std::string("NORMAL");
    handle->head.SCASERIAL = init.sca_serial;
    handle->head.ASICSERIAL = init.asic_serial;
    handle->head.OBSLOCATION = init.obs_location;
    handle->head.OBSERVATORY = init.observatory;
    if (not init.soft_version.empty()){
    handle->head.SOFTVERSION=init.soft_version;}
    else {
        handle->head.SOFTVERSION="N/A";
    }
   // if (connecting_user.compare("dev")!=0){
    if (handle->mcd.mcdASIC.empty() && !is_file_exist(handle->path.pathMCD+"HxRG_Main.mcd")){std::cout<<"Please place HxRG_Main.mcd in "+init.path+"mcd/"<<std::endl;return -1;}
    if (handle->mcd.mrf.empty() && !is_file_exist(handle->path.pathMCD+"MACIE_Registers_Slow.mrf")){std::cout<<"Please place the .mrf file in "+init.path+"mcd/"<<std::endl;return -1;}
    //}
    //other variable
    handle->user = init.user;
    handle->card.asBeenInit = 0;
    handle->head.ACQDATE="N/A";
    handle->head.JD = "";
    handle->head.timeGMT = "";
    handle->head.timeLocal="";
    handle->head.coldWarmMode=0;
    handle->head.ID=0;
    handle->card.haltTriggered = 0;
    handle->head.MASTERCLOCK=10.0;
    handle->head.NAMESEQUENCE="N/A";
    handle->head.nbOut=32;
    handle->nAcq.name = "ramp.fits";
    handle->pDet.nbOutput=static_cast<unsigned int>(32);
    handle->head.OBJET="objet";
    handle->head.OBSERVATEUR="N/A";
    handle->head.RAMP=1;
    handle->nAcq.fitsMTD = true;
    handle->nAcq.expoMeterCount =-1;
    handle->head.READ=0;
    handle->head.SLOWMODE=0;
    handle->head.UNITS="adu";
    handle->pAcq.posemeter_mask = "";
    handle->pAcq.posemeter_mask_optional = "";
    handle->pAcq.flux=0;
    handle->pAcq.flux_optional=0;
    handle->pAcq.drop=0;
    handle->pAcq.group = 1;
     handle->head.columnMode=0;
     handle->head.windowMode=0;
    handle->nAcq.status = hxrgEXP_COMPL_SUCCESS;//demande à gerard
    //config fits2ramp parameters
    handle->im_ready = false;
    handle->f2rConfig.nl = true;
    handle->f2rConfig.oddeven = false;
    handle->f2rConfig.bias = true;
    handle->f2rConfig.toponly = true;
    handle->f2rConfig.nl_uid = "";
    handle->f2rConfig.nl_status = false;
    handle->f2rConfig.bias_uid = "";
    handle->f2rConfig.bias_status = false;

    // opt.addr="0x0000";
    //opt.val="0";


    handle->pAcq.isWindow=0;
    handle->pAcq.ramp=1;

    handle->pAcq.reset=1;
    handle->pAcq.read=2;
    handle->pAcq.winStartY=0;
    handle->pAcq.winStartX=0;
    handle->pAcq.calpath = "/opt/HxRG-SERVER/cal";
    if (handle->pDet.HxRG==2)
    {
        handle->pAcq.winStopX=2047;
    }
    else if (handle->pDet.HxRG==4)
    {
        handle->pAcq.winStopX=4096;
    }
    else
    {
        handle->pDet.HxRG=1024;
    }
    return 0;
    }

int find_config_file(instHandle *handle,HxRG_init_config::conf_data *CFP,std::string user)
{
    HxRG_init_config config_file_reader;
    //HxRG_init_config::conf_data CFP;

    std::string conf(handle->path.pathConf+user+".conf");
    std::cout<<conf<<std::endl;
    if (is_file_exist(conf))
    {
        HxRGlog.writetoVerbose("A configuration file found for "+user);
        //read the configuration file and store the value in CFP structure
        config_file_reader.read_conf(handle->path.pathConf+user+".conf");
        config_file_reader.get_param();

        config_file_reader.get_parsed_data(CFP);

        config_file_reader.print_structure();

        return 0;

    }
    else {
        HxRGlog.writetoVerbose("No configuration file found for "+user);
        return -1;
    }

}
int check_ext(std::string filename,std::string ext)
{
    std::string buff("");
    for (std::string::iterator it=filename.end()-4;it!=filename.end();it++)
    {buff+=*it;}

    if (buff.compare(ext)!=0)
    {return 1;}

    return 0;

}
int set_config_file(HxRG_init_config::conf_data *CFP, instHandle *handle)
{
    //check if mcd is defined and upload HxRG_Main and optimized mcd
   if (handle->pAcq.simulator==0){

//        if (check_ext(CFP->mcd,std::string(".mcd"))==0)
//        {
//            if (uploadMCD(handle,CFP->mcd)!=0)
//            {
//                return -1;
//            }
//            HxRGlog.writetoVerbose("mcd "+CFP->mcd+" uploaded succesfully");
//            handle->mcd.mcdOpt=CFP->mcd;
//        }

//        delay(2000);
        if (check_ext(CFP->optimized_mcd,std::string(".mcd"))==0)
        {
            if (uploadMCD(handle,CFP->optimized_mcd)!=0)
            {
                HxRGlog.writetoVerbose("Unable to upload optimized MCD");
                return -1;
            }

            //HxRGlog.writetoVerbose("mcd "+CFP->optimized_mcd+" uploaded succesfully");
            handle->mcd.mcdOpt=CFP->optimized_mcd;
        }
        delay(100);
        if (MACIE_ResetErrorCounters(handle->card.handle,handle->card.slctMACIEs)!=MACIE_OK)
        {
            HxRGlog.writetoVerbose("Unable to reset error counter.");
            return -1;
        }
   }

    delay(1000);
    //set gain
    handle->pDet.gain=static_cast<unsigned int>(CFP->gain);
    if (handle->pAcq.simulator==0){
    if (setGain(handle,handle->pDet.gain)!=0){
        HxRGlog.writetoVerbose("Unable to set gain from config file.");
        return -1;
    }
    else {
        HxRGlog.writetoVerbose("Gain set succesfully from config file.");
    }
    }

    //set param I & II
    handle->pAcq.ramp = static_cast<unsigned int>(CFP->ramp);
    HxRGlog.writetoVerbose("ramp set succesfully from config file.");
    handle->pAcq.read = static_cast<unsigned int>(CFP->read);

    HxRGlog.writetoVerbose("read set succesfully from config file.");
    handle->pAcq.reset = static_cast<unsigned int>(CFP->reset);
    HxRGlog.writetoVerbose("reset set succesfully from config file.");

    handle->pDet.idle= static_cast<unsigned int>(CFP->idle);
if (handle->pAcq.simulator==0){
    if (idle(handle,handle->pDet.idle)==0){
    HxRGlog.writetoVerbose("idle mode set succesfully from config file.");}
    else {
        HxRGlog.writetoVerbose("Unable to set idle mode from config file.");
        return -1;
    }
}
    handle->pAcq.group = static_cast<unsigned int>(CFP->group);
    HxRGlog.writetoVerbose("group set succesfully from config file.");
    handle->pAcq.drop = static_cast<unsigned int>(CFP->drop);
    HxRGlog.writetoVerbose("drop set succesfully from config file.");
//apply the above change
    if (handle->pAcq.simulator==0){
    setRegister(handle);
}
    //set CW mode
    if (handle->pAcq.simulator==0){
    if (setColdWarmTest(handle,CFP->cw)!=0)
    {   HxRGlog.writetoVerbose("Unable to set CW mode from config file");
        return -1;}
    //set nb. of output amp
    delay(300);
    }
    if (handle->pAcq.simulator==0){
    if (setNbOutput(handle,CFP->output)!=0)
    {
        HxRGlog.writetoVerbose("Unable to set number of output mode from config file");
           return -1;
    }
    }
    delay(300);
    //set col. deselect
    if (handle->pAcq.simulator==0){
    if (columnDeselect(handle,CFP->col)!=0)
    {
        HxRGlog.writetoVerbose("Unable to set col.deselect mode from config file");
                return -1;
    }
    }
    //set the default data path
    if (not CFP->datapath.empty())
    {
    handle->pAcq.savePath = CFP->datapath;
    if (!handle->pAcq.savePath.empty() && handle->pAcq.savePath[handle->pAcq.savePath.length()-1] == '\n')
        {
        handle->pAcq.savePath.erase(handle->pAcq.savePath.length()-1);
    }
    handle->path.pathData = CFP->datapath;
    if (!handle->path.pathData.empty() && handle->path.pathData[handle->path.pathData.length()-1] == '\n')
        {
        handle->path.pathData.erase(handle->path.pathData.length()-1);
    }




    HxRGlog.writeto("Save path: "+handle->pAcq.savePath);

    }
    if (not CFP->calpath.empty())
    {
    handle->pAcq.calpath = CFP->calpath;
    if (!handle->pAcq.calpath.empty() && handle->pAcq.calpath[handle->pAcq.calpath.length()-1] == '\n')
        {
        handle->pAcq.calpath.erase(handle->pAcq.calpath.length()-1);
    }
    HxRGlog.writeto("Save path: "+handle->pAcq.calpath);
    }
    return 0;
}
