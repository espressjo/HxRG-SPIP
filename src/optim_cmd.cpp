#include "optim_cmd.h"

void testoptim(instHandle *handle, cmd *cc)
/*
 * Accept multiple key value:
 * 1) read [all,register_name]
 * 2) ? register_name -> list register
 * The following key-value pair must include this; > register_name
 * 3) setVoltage [0v,3.3v], by default range is auto
 * 4) setCurrent [uA], range is always auto
 * 6) setCompSelect [0,3]
 * 7) setIsink [enable/disable]
 * 8) setDACBuffer [0.002,60.0] mA
 * 9) setRange [hi/low] low (0v - 2v), hi (2v - 3.3v)
 * 10)setCurrentPower [up/down]
 * 11)
 */
{
    std::string arg;
    asic asicReg(handle->card.handle,handle->card.slctASIC);

    arg = (*cc)["?"];
    if (arg.compare("all")==0)
    {
        //list all register
        sndMsg(cc->sockfd,"\n"+asicReg.ls());
        return;

    }
    arg = (*cc)["read"];
    if (arg.compare("")!=0)
    {
        //list all register
        if (arg.compare("setMainRefCur")==0)
        {
            Main_RefCurrent mrc(handle->card.handle,handle->card.slctASIC);
            sndMsg(cc->sockfd,mrc.to_string());
            return;
        }
        else if (arg.compare("all")==0)
        {
            asicReg.read_all();
            std::string buff="";
            regV *reg=nullptr;
            for (std::string r : {"Vreset","Dsub","VbiasGate","VbiasPower","CellDrain","Drain","VDDA","VDD",
                 "VpreAmpRef1","VpreAmpRef2","VrefMain","VPCFbias","VRP","VRN","VpreMidRef","Vcm"}) {
                reg = asicReg[r];
                buff+="\n"+reg->to_string();

            }
            sndMsg(cc->sockfd,buff);
            //print all
            return;
        }
        else {

            if (asicReg.isValid(arg))
            {
                regV *reg = asicReg[arg];
                reg->read_asic();
                sndMsg(cc->sockfd,"\n"+reg->to_string());
                return ;
            }
            else {
                sndMsg(cc->sockfd,arg+" does not exsit");
                return ;
            }
        }
   }
    arg = (*cc)[">"];
    if (arg.compare("")!=0)
    {
        bool reconf=false;
        //main controll section
        if (!asicReg.isValid(arg))
        {//check if register is defined
            sndMsg(cc->sockfd,"register not found");
            return;
        }

        regV *reg = asicReg[arg];
        reg->read_asic();
        //setVoltage
        arg = (*cc)["setVoltage"];
        if (arg.compare("")!=0)
        {
            double V = 0;
            V = std::stod(arg.c_str());
            arg = (*cc)["setRange"];
            if (arg.compare("")!=0)
            {
                if (arg.compare("low")==0)
                {
                    reg->setRangeLow();
                    reg->setVoltage(V,false);
                }
                else if (arg.compare("hi")==0)
                {
                    reg->setRangeHi();
                    reg->setVoltage(V,false);
                }
                else {
                    printf("setRange only accept hi/low, going with autorange");
                    reg->setVoltage(V);
                }
            }
            else {
                reg->setVoltage(V);
            }

            reconf =true;
        }
        //setCurrent
        arg = (*cc)["setCurrent"];
        if (arg.compare("")!=0)
        {
            reg->setCurrant(std::stod(arg.c_str()));
            reconf =true;
        }
        //setCompSelect
        arg = (*cc)["setCompSelect"];
        if (arg.compare("")!=0)
        {
            reg->setCompSelect(static_cast<uint16_t>(std::atoi(arg.c_str())));
            reconf =true;
        }
        //setIsink
        arg = (*cc)["setIsink"];
        if (arg.compare("")!=0)
        {
            if (arg.compare("enable")==0)
            {
                reg->Isink_enable();
            }
            else if (arg.compare("disable")==0) {
                reg->Isink_disable();
            }
            else {
                printf("setIsink accept only enable/disable.\n");
            }
            reconf =true;
        }
        //setDACBuffer
        arg = (*cc)["setDACBuffer"];
        if (arg.compare("")!=0)
        {
            reg->setDACbuffer(static_cast<double>(std::atof(arg.c_str())));
            reconf =true;
        }
        arg = (*cc)["setDACPower"];
        if (arg.compare("")!=0)
        {
            if (arg.compare("up")==0)
            {
                reg->setDACpowerUp();
                reconf =true;
            }
            else if (arg.compare("down")==0)
            {
                reg->setDACpowerDown();
                reconf =true;
            }

        }
        //setCurrentPower
        arg = (*cc)["setCurrentPower"];
        if (arg.compare("")!=0)
        {
            if (arg.compare("up")==0)
            {
                reg->setCurrentPowerUp();
            }
            else if (arg.compare("down")==0) {
                reg->setCurrentPowerDown();
            }
            reconf =true;
        }


        //for now we test if it works
        if (reconf)
        {
            reg->write_asic(true);
            //printf("reconfiguration called.\n");
        }
        reg->read_asic();
        sndMsg(cc->sockfd,"\n"+reg->to_string());
        return;
    }
    arg = (*cc)["telemetry"];
    if (arg.compare("")!=0)
    {
        if (!asicReg.isValid(arg))
        {//check if register is defined
            sndMsg(cc->sockfd,"register not found");
            return;
        }

        regV *reg = asicReg[arg];
        sndMsg(cc->sockfd,reg->telemetry_t());
        return ;
    }
    if ((*cc)["setMainRefCur"].compare("")!=0)
    {
        double volt = std::atof((*cc)["setMainRefCur"].c_str());
        if (volt<0.5 || volt > 2)
        {
            sndMsg(cc->sockfd,"range can only be between 0.5 and 2 V.",hxrgCMD_ERR_VALUE);
            return ;
        }
        else {
            Main_RefCurrent mrc(handle->card.handle,handle->card.slctASIC);
            if (mrc.setRefCurrent(volt)!=0)
            {
                sndMsg(cc->sockfd,"Fail to set Main ref. currant.",hxrgCMD_ERR_VALUE);
                return;
            }
            else {
                sndMsg(cc->sockfd,mrc.to_string());
                return ;
            }
        }
    }
    sndMsg(cc->sockfd);
    return;
}
