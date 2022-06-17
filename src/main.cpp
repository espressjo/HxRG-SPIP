#include <iostream>
#include "insthandle.h"
#include "uics.h"
#include "init_handler.h"
#include "hxrg_get_status.h"
#include "argument_parser.h"
#include "hxrg_conf.h"
using namespace std;
#define READY "ready"
#ifndef INITPATH
#define INITPATH "/opt/HxRG-SPIP/config"
#endif
Log HxRGlog;
//#define INITPATH "/opt/HxRG-ENG/config"
//void lincorr(instHandle *handle,cmd *cc);
void f2rampParam(instHandle *handle,cmd *cc);
//void setGainDummy(instHandle *handle,cmd *cc)
///*!
//  *\brief Fake a gain change.
//  * Gives DCS the impression that it can change the gain.
//  * The wrong sequence of command is sent during init in DCS,
//  * therefore we fake the setGain().
//  * From now on, the gain is only controlled by the config file.
//  * This is a temporary modification.
//  */
//{
//    if (handle->pAcq.isOngoing!=0)
//    {
//        sndMsg(cc->sockfd,"Acquisition ongoing",nidcsCMD_ERR_VALUE);
//        return ;
//    }
//    int gain = std::atoi(cc->argsVal[0].c_str());
//    if (gain<0 || gain>15)
//    {
//        sndMsg(cc->sockfd,"Gain is out of range",nidcsCMD_ERR_PARAM_VALUE);
//        return ;
//    }
//    else {
//        std::string GainCode[16]={"-3 dB small Cin","0 dB small Cin","3 dB small Cin","6 dB small Cin","6 dB large Cin","9 dB small Cin","9 dB large Cin","12 dB small Cin","12 dB large Cin","15 dB small Cin","15 dB large Cin","18 dB small Cin","18 dB large Cin","21 dB large Cin","24 dB large Cin","27 dB large Cin"};
//        sndMsg(cc->sockfd,"Gain set to "+GainCode[gain]);
//        return ;
//    }

//}
void getImStatus(instHandle *handle,cmd *cc)//getUniqueId
/*
 * return if an image is available. if it is it will print some pixels value
 *
 */
{
    int fd = create_socket(7041);
    cmd *c = new cmd;
    std::string buff="";
    while (1) {
        c->recvCMD(fd);
        if (handle->im_ready)
        {   buff.clear();
            for (int i=0;i<100;i++)
            {
                buff+=std::to_string(((double*)handle->im_ptr)[i]);
            }
            sndMsg(c->sockfd,buff);
            handle->im_ready = false;
        }

        sndMsg(c->sockfd,"no data ready");
       }
}
void getUniqueId(instHandle *handle,cmd *cc)//getUniqueId
/*
 * After an acquisition sequence is finished, this cmd
 * will return the unique ID number of the sequence.
 */
{
    int fd = create_socket(6041);
    cmd *c = new cmd;
    while (1) {
        c->recvCMD(fd);
        sndMsg(c->sockfd,std::to_string(handle->head.ID));
       }
}
void printHandle(instHandle *handle,cmd *cc)
{
    std::string msg="";
    int fd = create_socket(5011);
    //cmdList all(CMDCONFFILE);
    cmd *c = new cmd;

    while (1) {
        c->recvCMD(fd);

        std::cout<<"State: "<< handle->state<<std::endl;
        std::cout<<"Next State: "<< handle->nextState<<std::endl;
        std::cout<<"Gain: "<< handle->pDet.gain<<std::endl;
        std::cout<<"Read: "<< handle->pAcq.read<<std::endl;
        std::cout<<"Ramp: "<< handle->pAcq.ramp<<std::endl;
        std::cout<<"IsOnGoing: "<< handle->pAcq.isOngoing<<std::endl;

        std::cout<<"asBeenInit: "<< handle->card.asBeenInit<<std::endl;
        std::cout<<"Mask1: "<< handle->pAcq.posemeter_mask<<std::endl;
        std::cout<<"Mask2: "<< handle->pAcq.posemeter_mask_optional<<std::endl;
        std::cout<<"ExpoMeterCount: "<< handle->nAcq.expoMeterCount<<std::endl;
        std::cout<<"Simulator: "<< handle->pAcq.simulator<<std::endl;
        std::cout<<"ColdWarmMode: "<< handle->head.coldWarmMode<<std::endl;

      sndMsg(c->sockfd);

    }
}
void acq_status(instHandle *handle,cmd *cc)
{
    int fd = create_socket(5040);
    cmd *c = new cmd;
    while (1) {
        c->recvCMD(fd);
        if (handle->pAcq.isOngoing!=0 and handle->state==ACQ)
        {
            sndMsg(c->sockfd,std::to_string(1) );
        }
        else {
            sndMsg(c->sockfd,std::to_string(0) );
        }
    }

}
void init_status(instHandle *handle,cmd *cc)
{
    int fd = create_socket(5041);
    cmd *c = new cmd;
    while (1) {
        c->recvCMD(fd);
        if (handle->card.asBeenInit>0)
        {
            sndMsg(c->sockfd,std::to_string(1) );//1-> MACIE_INIT has been called, 2-> init seq. is finished and successfull.
        }
        else {
            sndMsg(c->sockfd,std::to_string(0) );
        }

    }
}

void exit_acq(instHandle *handle,cmd *cc)
{
    handle->card.haltTriggered = 1;
    while(handle->pAcq.isOngoing)
    {
        std::cout<<"waithing for halt to be done"<<std::endl;
        sleep(1);
    }
}

int main(int argc, char *argv[])
{
    //check if there is arguments
    parser parse(argc,argv);
    if (parse.isarg("--help") || parse.isarg("-h"))
    {
        parse.helper();
        return 0;
    }
    //start the log
    instHandle handle;
    std::cout<<INITPATH<<std::endl;
    initConfig(&handle,join(INITPATH,"HxRG.init").c_str());

    cout<<"log path: "<<handle.path.pathLog<<endl;
    HxRGlog.setPath(handle.path.pathLog);
    HxRGlog.writeto("STARTUP");

    //start the message handler
    //with an appropriate level of log
    msgHandler msgH;
    if (parse.isarg("--loglevel"))
    {
        int level = std::atoi(parse.get("--loglevel").c_str());
        if (level>=0 && level<=3)
        {
            msgH.setlPath(handle.path.pathLog);
            msgH.setllevel(level);
        }

    }
    if (parse.isarg("--linearize"))
    {
        handle.f2rConfig.nl = true;
    }

    std::thread t_msg(&msgHandler::run,&msgH);
    t_msg.detach();
    sleep(1);


    //check some argument




    if (parse.isarg("--keep_gfile"))
    {
        handle.save_glitch = true;
    }
    if (parse.isarg("--no_glitch"))
    {
        handle.glitch_test = false;
    }
    if (parse.isarg("--telemetry"))
    {
        handle.readtelemetry = true;
    }



    state_handler sHandler(&handle);

    //mod 2020-12-03 read setGainDummy for more details
    //sHandler.s_config->add_callback("setgain",setGain);
    sHandler.s_config->add_callback("upload",upMCD);
    //sHandler.s_config->add_callback("setgain",setGain);
    sHandler.s_config->add_callback("coldwarmmode",setColdWarmTest);
    sHandler.s_config->add_callback("nboutput",setNbOutput);
    sHandler.s_config->add_callback("columnmode",columnDeselect);
    sHandler.s_config->add_callback("closeMacie",closeMacie);
    sHandler.s_config->add_callback("clocking",clocking);
    sHandler.s_init->add_callback("init",initAll);
    //sHandler.s_init->executeOnlyOnce(CONFIG);
    sHandler.s_config->add_callback("read",readRegister_cmd);//readRegister_cmd
    sHandler.s_acq->add_callback("START-ACQ",acquisition);
    sHandler.s_acq->add_callback("START-ENG",acquisition_eng);
    sHandler.s_acq->add_callback("printHandle",printHandle);
    sHandler.s_acq->add_callback("acqStatus",acq_status);
    sHandler.s_config->add_callback("setParam",setParam);
    sHandler.s_config->add_callback("simulator",simul_mode);
    sHandler.s_config->add_callback("power",powerControl);
    sHandler.s_config->add_callback("f2r",f2rampParam);


    //sHandler.s_config->add_callback("SETUP",SETUP);

    sHandler.s_acq->add_callback("exit",exit_acq);
    //sHandler.s_idle->start_with("idle");
    sHandler.s_idle->add_callback("idle",c_idle);
    sHandler.s_idle->add_callback("exit",exit_acq);

    //IDLE mode is onEND
    //sHandler.s_acq->executeOnlyOnce(IDLE);
   // sHandler.s_config->executeOnlyOnce(IDLE);
    //sHandler.s_init->executeOnlyOnce(IDLE);

    //because it is used in thread even when init is not called
    handle.pAcq.simulator = 0;// set the simulator to off (0).
    handle.pAcq.isOngoing = 0;
    //void nirps_start(instHandle *handle)
    cmd *cc;
    std::thread t_acq_mgr(&nirps_start,&handle);
    t_acq_mgr.detach();
    std::thread t_getData(&send_data,&handle);
    t_getData.detach();
    std::thread t_print(&printHandle,&handle,cc);
    t_print.detach();
    std::thread t_acqStatus(&acq_status,&handle,cc);
    t_acqStatus.detach();
    std::thread t_initStatus(&init_status,&handle,cc);
    t_initStatus.detach();
    std::thread t_unique_id(&getUniqueId,&handle,cc);//return the unique IDvoid getUniqueId(instHandle *handle,cmd *cc)
    t_unique_id.detach();

    std::thread t_im_status(&getImStatus,&handle,cc);
    t_im_status.detach();

    std::thread A(&acq_t,&handle);
    std::thread t_setup(&setup_t,&handle);
    t_setup.detach();
    std::thread t_get_status(&send_status,&handle);
    t_get_status.detach();
    A.detach();
    sleep(1);
    sHandler.run();



    return 0;
}

void f2rampParam(instHandle *handle,cmd *cc)
/*
 * Temporary function to activate/deactivate non-linearity correction
 */
{
    std::string param = "";
    param = (*cc)["oddeven"];
    if (param.compare("")!=0)
    {
        if (param.compare("true")==0 || param.compare("T")==0)
        {
            HxRGlog.writetoVerbose("fits2ramp oddeven param set.");
            handle->f2rConfig.oddeven = true;
        }

        else if (param.compare("false")==0 || param.compare("F")==0)
        {
            HxRGlog.writetoVerbose("fits2ramp oddeven param unset.");
            handle->f2rConfig.oddeven = false;
        }
    }
    param = (*cc)["toponly"];
    if (param.compare("")!=0)
    {
        if (param.compare("true")==0 || param.compare("T")==0)
        {
            HxRGlog.writetoVerbose("fits2ramp top and side ref. px. in use.");
            handle->f2rConfig.toponly = true;
        }

        else if (param.compare("false")==0 || param.compare("F")==0)
        {
            HxRGlog.writetoVerbose("fits2ramp all ref. px. used.");
            handle->f2rConfig.toponly = false;
        }
    }
    param = (*cc)["bias"];
    if (param.compare("")!=0)
    {
        if (param.compare("true")==0 || param.compare("T")==0)
        {
            HxRGlog.writetoVerbose("Bias correction enable.");
            handle->f2rConfig.bias = true;
        }

        else if (param.compare("false")==0 || param.compare("F")==0)
        {
            HxRGlog.writetoVerbose("Bias correction disable.");
            handle->f2rConfig.bias = false;
        }
    }
    param = (*cc)["nl"];
    if (param.compare("")!=0)
    {
        if (param.compare("true")==0 || param.compare("T")==0)
        {
            HxRGlog.writetoVerbose("Non-linearity enable.");
            handle->f2rConfig.nl = true;
        }

        else if (param.compare("false")==0 || param.compare("F")==0)
        {
            HxRGlog.writetoVerbose("Non-linearity disable.");
            handle->f2rConfig.nl = false;
        }
    }


    sndMsg(cc->sockfd);
}



//void lincorr(instHandle *handle,cmd *cc)
///*
// * Temporary function to activate/deactivate non-linearity correction
// */
//{
//    if (cc->argsVal[0].compare("true")==0)
//    {
//        handle->f2rConfig.nl=true;
//    }
//    else if (cc->argsVal[0].compare("false")==0){
//        handle->f2rConfig.nl=false;
//    }
//    else {
//        sndMsg(cc->sockfd,"Argument can only be true/false",nidcsCMD_ERR_VALUE);
//        return;

//    }
//    sndMsg(cc->sockfd);
//}
