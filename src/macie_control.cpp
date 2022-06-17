
#include "macie_control.h"
#include "uics.h"
#include "init_handler.h"
#include "autocheck.h"
#include <string.h>

extern Log HxRGlog;

void delay(int ms)
/*!
  *\brief Delay in ms
  * Create a delay. ms is delay time in millisecond
  */
{
    clock_t t0 = clock();
    while (clock()<t0+ms*1000)
        ;
}

void simul_mode(instHandle *handle,cmd *cc)
{
    if (cc->argsVal[0].compare("on")==0)
    {
        HxRGlog.writetoVerbose("Simulator switch ON");
        handle->pAcq.simulator = 1;
        sndMsg(cc->sockfd);
    }
    else if (cc->argsVal[0].compare("off")==0) {
        handle->pAcq.simulator = 0;
        HxRGlog.writetoVerbose("Simulator switch OFF");
        sndMsg(cc->sockfd);
    }
    else {
        HxRGlog.writetoVerbose("--Warning-- ::Simulator:: Wrong parameters. Accept only on or off");
        sndMsg(cc->sockfd,"Wrong parameters. Accept only on or off",hxrgCMD_ERR_PARAM_VALUE);
    }
}
uint readRegister(instHandle *handle,std::string regAddr)
{

    unsigned short address;
    sscanf(regAddr.c_str(),"%hx",&address);

    uint value;
    MACIE_STATUS r1 = MACIE_ReadASICReg(handle->card.handle, handle->card.slctASIC, address, &value, false,true);
    HxRGlog.writetoVerbose("Reading register "+regAddr);
    if ((r1 != MACIE_OK))
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return (uint)0;
    }
    HxRGlog.writetoVerbose("Value of register "+regAddr+" is "+std::to_string(value));
    return value;

}
void clocking(instHandle *handle,cmd *cc)
{
    std::string clock = (*cc)["clocking"];
    std::string mplexer = (*cc)["multiplexer"];
    int error=0;
    if (!clock.empty() && clock.compare("normal")==0)
    {
        if (clocking(handle,false)!=0)


        {
            error+=1;
        }
    }
     if (!clock.empty() && clock.compare("enhanced")==0)
    {
         if (clocking(handle,true)!=0)
         {
            error+=1;
         }
    }
    if (!mplexer.empty() && mplexer.compare("2")==0)
    {
        if (HxRG(handle,2)!=0)
        {
            error+=1;
        }
    }
    if (!mplexer.empty() && mplexer.compare("4")==0)
    {
        if (HxRG(handle,4)!=0)
        {
            error+=1;
        }
    }
    if (error!=0)
    {
        sndMsg(cc->sockfd,"Something went wrong in the configuration of the new clocking",hxrgCMD_ERR_VALUE);
    }
    else {
        sndMsg(cc->sockfd,"Clocking set successfully");
    }

}
void readRegister_cmd(instHandle *handle,cmd *cc)
{   char valHex[100];
    memset(valHex,0,100);
    HxRGlog.writeto("Reading register command receive.");
    uint value = readRegister(handle,cc->argsVal[0]);
    sprintf(valHex,"%x",value);

    sndMsg(cc->sockfd,valHex);


}
int writeRegister(instHandle *handle,std::string regAddr,std::string hexVal)
{
    uint value=0;
    unsigned short address=0;
    sscanf(hexVal.c_str(),"%x",&value);
    sscanf(regAddr.c_str(),"%hx",&address);
    HxRGlog.writeto("Writting addr "+regAddr+" with "+hexVal);
    if (MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, address, value, true)!= MACIE_OK)
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
       HxRGlog.writetoVerbose("re-uploading the MCD might solve this issue");
       return -1;
    }

    HxRGlog.writetoVerbose("Register "+regAddr+" set to "+hexVal+" succesfully");
    return 0;

}

bool check_halt(instHandle *handle)
/*
 *Check if halt has been triggered. If yes it will put ASIC back to idle state
 */
{
    if (handle->card.haltTriggered==1)
    {
        if (trigger_halt(handle)!=0)
        {
            HxRGlog.writetoVerbose("Unable to trigger halt");
        }
        else {
            HxRGlog.writetoVerbose("Halt succeeded");
        }
        handle->nAcq.status = hxrgEXP_COMPL_ABORTED;
        return true;
    }
    return false;
}
bool check_halt(instHandle *handle,bool silent)
/*
 * Check if halt has been triggered. If yes it will put ASIC back to idle state
 * Silent will not change VLT's state.
 */
{
    if (handle->card.haltTriggered==1)
    {
        if (trigger_halt(handle)!=0)
        {
            HxRGlog.writetoVerbose("Unable to trigger halt");
        }
        else {
            HxRGlog.writetoVerbose("Halt succeeded");
        }
        if (silent){
            //do nothing
        }
        else {
            handle->nAcq.status = hxrgEXP_COMPL_ABORTED;

        }

        return true;
    }
    return false;
}
int trigger_halt(instHandle *handle)
/*!
  * Use to exit the integration loop.
 */
{

    HxRGlog.writetoVerbose("Halt triggered");
    int state = get_asic_6900(handle);
    if (state<0)
    {
        HxRGlog.writetoVerbose("Unable to read ASIC 0x6900");
        return -1;
    }
    switch (state) {
    case ASIC_FAILED:
        HxRGlog.writetoVerbose("Unable to read ASIC 0x6900");
        return -1;
    case ASIC_IDLE:
        HxRGlog.writetoVerbose("ASIC currently in IDLE. (weird)");
        break;
    case ASIC_ACQ:
        HxRGlog.writetoVerbose("ASIC currently in ACQ. (weird)");
        break;
    default:
        HxRGlog.writetoVerbose("ASIC 0x6900 in undefined state");
        return -1;
    }
    if (set_asic_6900(handle,ASIC_IDLE)!=0)
    {
        HxRGlog.writetoVerbose("Unable got to idle.");
        return -1;
    }
    HxRGlog.writetoVerbose("Halt success.");
    return 0;

}


int reconfigure(instHandle *handle)
/*!
 * The time taken to reconfigure the SIDECAR and detector is very short. However, if the
 * detector is performing an IDLE reset with a very long frame time, there will be a considerable lag
 * between writing 8002 to h6900 and seeing 8000 appear in this register since the reset frame must
 * be completed before the reconfiguration takes place. It is recommended that the user poll this
 * register before starting an exposure sequence
 *
 * Use to request a reconfiguration of the ASIC/detector
 * \param reconf 1->reconfigure the ASIC, reconf->0, go back to idle
 */
{
    int timeout = 200;//200 ms timeout
    uint val;
    int total_time = 0;
    HxRGlog.writeto("reconfigure triggered");
    if (MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x6900, 0x8002, true)!=MACIE_OK)
    {
            HxRGlog.writetoVerbose("Unable to write to 0x6900--reconfigure()");
            return -1;
    }


    for (int i=0;i<60;++i) {
        total_time+=200;
        delay(timeout);
        if (MACIE_ReadASICReg(handle->card.handle, handle->card.slctASIC, 0x6900, &val, false, true) != MACIE_OK)
        {
            HxRGlog.writetoVerbose("Unable to write to 0x6900--reconfigure()");
            return -1;
        }
        if ((val & 2) != 2 )
        {
            HxRGlog.writetoVerbose("h6900 reconfigured in "+std::to_string(total_time)+" msec");
            return 0;
        }

        //HxRGlog.writetoVerbose("h6900 reconfigured in "+std::to_string(total_time)+" msec");

    }
    HxRGlog.writetoVerbose("Failed to reconfigure.");
    return -1;
}
int writeRegister(instHandle *handle,std::string regAddr,uint hexVal)
{

    unsigned short address;

    sscanf(regAddr.c_str(),"%hx",&address);
    HxRGlog.writeto("Writting "+std::to_string(hexVal)+" register "+regAddr);
    if (MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, address, hexVal, true)!= MACIE_OK)
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Re-uploading the MCD might solve this problem.");
        return -1;
    }
    HxRGlog.writetoVerbose("Register "+regAddr+" set to "+std::to_string(hexVal)+" succesfully");
    return 0;

}

void idle(instHandle *handle,cmd *cc)
/*!
  *\brief set the ASIC idle mode
  * Sets the ASIC idle mode. A custom idle should
  * be used instead of this because the mode reset+read
  * is not truely equivalent to a real reset+read.
  * mode 0 -> do nothing in idle mode
  * mode 1 -> continuously take reset frames in idle mode
  * mode 2 -> continuously take reset-read frames in Idle mode
  * --Used as a standardized function--
  */
{
    HxRGlog.writeto("Setting ASIC idle mode...");
    if (handle->pAcq.isOngoing!=0)
    {
        HxRGlog.writeto("Cannot do, exposure detector is integrating.");
        sndMsg(cc->sockfd,"Acquisition ongoing",hxrgCMD_ERR_VALUE);
        return ;
    }
    uint mode =static_cast<uint>(std::atoi(cc->argsVal[0].c_str()));
    if (mode>2)
    {
        HxRGlog.writeto("[Failed] mode out of range");
        sndMsg(cc->sockfd,"mode out of range",hxrgCMD_ERR_VALUE);
        return;
    }
    else if (idle(handle,mode)!=0) {
        HxRGlog.writeto("[Failed] Unable to change idle mode");
        sndMsg(cc->sockfd,"Unable to change idle mode",hxrgCMD_ERR_VALUE);
        return;
    }
    else {
        HxRGlog.writeto("[Success] idle changed to mode "+std::to_string(mode));
        sndMsg(cc->sockfd,"Idle mode set",hxrgCMD_ERR_VALUE);
        handle->pDet.idle = mode;
        return;
    }
}
int idle(instHandle *handle,uint mode)
/*!
  *\brief set the ASIC idle mode
  * Sets the ASIC idle mode. A custom idle should
  * be used instead of this because the mode reset+read
  * is not truely equivalent to a real reset+read.
  * mode 0 -> do nothing in idle mode
  * mode 1 -> continuously take reset frames in idle mode
  * mode 2 -> continuously take reset-read frames in Idle mode
  */
{

   if (mode>2)
    {
        return -1;
    }
    uint original = readRegister(handle,"0x4002");
    if (original==0)
    {
        std::cout<<"A possible problem has occured in the selection of the idle mode"<<std::endl;
    }
    uint mask = 65511;
    uint shift=3;
    original = (original&mask) | mode<<shift;
    if (writeRegister(handle,"0x4002",original)!=0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
uint readBit(uint number,uint k)
/*!
  *\brief Read bit at the k position.
  */
{
    return (number>>k)&1;
}

int start(instHandle *handle)
/*!
  *\brief Communication with the MACIE card
  * Initialize the MACIE and connect to the
  * card ethernet interface. Will return -1
  * if the initialization fail. It is also
  * here that we read the MACIE lib version.
  */
{
    HxRGlog.writetoVerbose("Starting the initialization...");

    handle->card.bRet = MACIE_Init();
    handle->macie_libv = MACIE_LibVersion();//read MACIE library version
    if (handle->card.bRet!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to initialize the MACIE card. bye bye :(");
        return -1;
    }
    handle->card.asBeenInit = 1;
    HxRGlog.writetoVerbose("Check Interface....");
    delay(100);
    handle->card.bRet = MACIE_CheckInterfaces(0,NULL,0,&handle->card.numCards, &handle->card.pCard);
    if(handle->card.bRet == MACIE_OK)
    {
        HxRGlog.writetoVerbose("MACIE_CheckInterfaces succeeded" );
    }
    else
    {
        HxRGlog.writetoVerbose("MACIE_CheckInterfaces failed" );
    }
    if (handle->card.numCards==0)
    {
        HxRGlog.writetoVerbose("numCards=0, shutting down...");
        return -1;
    }
    HxRGlog.writetoVerbose("numCards= "+std::to_string(handle->card.numCards));

    for (int i = 0; i < handle->card.numCards; i++)
    {
        HxRGlog.writetoVerbose("macieSerialNumber= "+std::to_string(handle->card.pCard[i].macieSerialNumber));
        std::cout << "ipAddr=" << static_cast<int>((handle->card.pCard[i].ipAddr)[0]) << "." << static_cast<int>((handle->card.pCard[i].ipAddr)[1]) <<
                            "." << static_cast<int>((handle->card.pCard[i].ipAddr)[2]) << "." << static_cast<int>((handle->card.pCard[i].ipAddr)[3]) << std::endl;
        HxRGlog.writetoVerbose("gigeSpeed= "+std::to_string(handle->card.pCard[i].gigeSpeed));
    }
    return 0;
}

int InitializeASIC(instHandle *handle)
/*!
  *\brief Initialize MACIE & ASIC
  * Initialize MACIE & ASIC card. Upload MACIE register
  * file and upload the ASIC firmware. Will return -1
  * if the initialization fail. 0 if successful.
  */
{
    delay(100);
    HxRGlog.writetoVerbose("Initialize with handle "+std::to_string(handle->card.handle));
    if (static_cast<unsigned short>(handle->card.slctMACIEs) == 0)
    {
        HxRGlog.writetoVerbose("MACIE is not available");
        return -1;
    }
    // step 1: load MACIE firmware from slot 1 or slot 2
    if (MACIE_loadMACIEFirmware(handle->card.handle, handle->card.slctMACIEs, true, &handle->card.val) != MACIE_OK)
    {
        HxRGlog.writetoVerbose("Load MACIE firmware failed: "+(static_cast<std::string>(MACIE_Error())));
        return -1;
    }
    if (handle->card.val != 0xac1e) // return value
    {
        HxRGlog.writetoVerbose("Verification of MACIE firmware load failed: readback of hFFFB= "+std::to_string(handle->card.val));
        return -1;
    }
    HxRGlog.writetoVerbose("Load MACIE firmware succeeded");
    // step 2: download MACIE registers
    delay(100);
    if (MACIE_DownloadMACIEFile(handle->card.handle, handle->card.slctMACIEs, handle->mcd.mrf.c_str()) != MACIE_OK)
    {
        HxRGlog.writetoVerbose("Download MACIE register file Failed: "+(static_cast<std::string>(MACIE_Error())));
        return -1;
    }
    HxRGlog.writetoVerbose("Download MACIE register file succeeded");
    // step 3 reset science data error counters
    get_error(handle);

    delay(100);
    if (MACIE_ResetErrorCounters(handle->card.handle, handle->card.slctMACIEs) != MACIE_OK)
    {
        HxRGlog.writetoVerbose("Reset MACIE error counters failed");
        return -1;
    }
    HxRGlog.writetoVerbose("Reset error counters succeeded");
    // step 4 download ASIC file, for example ASIC mcd file
    delay(100);
    if (MACIE_DownloadASICFile(handle->card.handle, handle->card.slctMACIEs, handle->mcd.mcdASIC.c_str(), true) != MACIE_OK) //m_asicIds
    {
        HxRGlog.writetoVerbose("Download ASIC firmware failed: " + (static_cast<std::string>(MACIE_Error())));
        return -1;
    }
    HxRGlog.writetoVerbose("Download ASIC firmware succeeded: "+handle->mcd.mcdASIC);
    delay(100);
    if (MACIE_ResetErrorCounters(handle->card.handle, handle->card.slctMACIEs) != MACIE_OK)
    {
        HxRGlog.writetoVerbose("Reset MACIE error counters failed");
        return -1;
    }
    delay(100);
    handle->card.avaiASICs = MACIE_GetAvailableASICs(handle->card.handle, false);

    if (static_cast<unsigned short>(handle->card.avaiASICs) == 0)
    {
        HxRGlog.writetoVerbose("MACIE_GetAvailableASICs failed");
        return -1;
    }
    std::cout << "Initialization succeeded " << std::endl;
    HxRGlog.writetoVerbose("Available ASICs= "+std::to_string(static_cast<unsigned short>(handle->card.avaiASICs)));
    HxRGlog.writetoVerbose("Initialization succeeded ");
    return 0;
}
int getHandle(instHandle *handle)
/*!
  *\brief get the MACIE handle.
  * Will return -1 if it fail or 0 if successfull.
  */
{
    handle->card.connection = MACIE_GigE;
    delay(100);
    HxRGlog.writetoVerbose("Get handle for the interface with MACIE serial number "+std::to_string(handle->card.pCard[0].macieSerialNumber)+" and GigE connection ");
    handle->card.handle = MACIE_GetHandle(handle->card.pCard[0].macieSerialNumber, handle->card.connection);
    HxRGlog.writetoVerbose(" Handle = " + std::to_string(handle->card.handle));
    if (handle->card.handle == 0)
    {
        HxRGlog.writetoVerbose(MACIE_Error());
        return -1;
    }
    return 0;
}
int getAvailableMacie(instHandle *handle)
/*!
  *\brief Select MACIE card.
  * Will return -1 if it fail or 0 if successfull.
  */
{
    handle->card.avaiMACIEs = MACIE_GetAvailableMACIEs(handle->card.handle);
    handle->card.slctMACIEs = (static_cast<unsigned short>(handle->card.avaiMACIEs)) & 1;
    HxRGlog.writetoVerbose("Looking for MACIE card.");
    if (handle->card.slctMACIEs == 0)
    {
        std::cout << "Select MACIE = " << handle->card.slctMACIEs << " invalid" << std::endl;
        HxRGlog.writetoVerbose("Selected Macie's card invalid");
        return -1;
    }
    else if (MACIE_ReadMACIEReg(handle->card.handle, handle->card.avaiMACIEs, 0x0300, &handle->card.val) != MACIE_OK)
    {
        HxRGlog.writetoVerbose("MACIE read 0x300 failed: "+static_cast<std::string>(MACIE_Error()));
        return -1;
    }
    else
    {
        HxRGlog.writetoVerbose("MACIE h0300="+std::to_string(handle->card.val));
    }
    return 0;
}
int HxRG(instHandle *handle,int x)
/*
 * Configure the multiplexer of the HxRG to operate the detector as a H4RG or a H2RG.
 * int x can be either 2 or 4.
 * */
{
    if (x !=2 && x !=4)
    {
        HxRGlog.writetoVerbose("Mode can only be 2 or 4.");
        return -1;
    }
    MACIE_STATUS r1;
    if (x==2)
    {
        r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4010, 0x0002, true);
    }
    if (x==4)
    {
        r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4010, 0x0004, true);
    }

    if (r1!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to set the multiplexer register.");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Configuring the multiplexer failed.");
        return -1;
    }
    else {
        if (x==4)
        {
            HxRGlog.writetoVerbose("The multiplexer is now configured for H4RG.");
        }
        else {
            HxRGlog.writetoVerbose("The multiplexer is now configured for H2RG.");
        }

        return 0;
    }

}
int clocking(instHandle *handle,bool enhanced)
/*
 * Change the clocking scheme of the HxRG. If enhanced is set to true, the clocking
 * register will be set to enhanced, reset pixels per pixels. If set to false, the
 * clocking is normal and the reset line by line. 2020-10-19.
 *
    */
{
    HxRGlog.writetoVerbose("Setting register 0x4017.");

    MACIE_STATUS r1;
    if (enhanced)
    {
        r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4017, 0x0001, true);
    }
    else {
        r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4017, 0x0000, true);
    }
    if (r1 !=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to set register 0x4017.");
        return -1;
    }

    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Configuring the clocking failed.");
        return -1;
    }
    if (enhanced)
    {
        HxRGlog.writetoVerbose("Clocking set to enhanced clocking, resets pixels per pixels.");
    }
    else {
        HxRGlog.writetoVerbose("Clocking set to normal clocking, resets line by line.");
    }
    return 0;
}
int initializeRegister(instHandle *handle)
/*!
  *\brief Set some mandatory register
  * It will bring the ASIC to a minimal working state.
  * Will return 0 if successfull, -1 if it fails.
  */
{
    HxRGlog.writetoVerbose("Setting the ASIC to a minimal working state.");

    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4019, handle->pDet.preampInputScheme, true); //h4019<0>(0->VREFMAIN as v1,v2,.., 1->use InPcommon),h4019<1>(0->use input onfiguration set in <0>, 1-> Overide with register h5100)
    //MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x5100, handle->pDet.preampInputVal, true);//value use for h4019
    MACIE_STATUS r3 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4018, 0x0000, true);//HxRGExpModeVal h4018<0>(0->Up the ramp, 1-> Fowler sampling), h4018<1>(0->Full frame, 1->Window Mode)
    MACIE_STATUS r4 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4006, 0x0000, true);//Extra pixels added per row
    MACIE_STATUS r5 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4007, 0x0000, true);//RowEndDelay
    delay(200);
    MACIE_STATUS r6 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4010, handle->pDet.HxRG, true);//H2RG or H4RG
    MACIE_STATUS r7 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4011, handle->pDet.nbOutput, true);//32 output
    MACIE_STATUS r8 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4050, 0x0001, true);//H4RG column-15 de-select
    MACIE_STATUS r9 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4015, 0x0000, true);//h4015<0>(0->Cold, 1-> Warm)
    MACIE_STATUS r10 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4016, 0x0000, true);//h4016<0>(0->no ktc removal, 1-> ktc removal), h4016<1>(0->Reset once per frame, 1->reset once per row)
    MACIE_STATUS r11 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4017, 0x0001, true);//h4017<0>(0->Normal clocking, 1->Enhenced clocking), h4017<1>(0->reset per row(det configuration not preamp config), 1-> Global reset)
    delay(200);

    MACIE_STATUS r12 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4002, 0x000c, true);//h4002<0>(1->enable reset frame data output),h4002<1>(1->enable pulse at the end of each read drop frame),h4002<2>(1->enable clocking sca in drop frame),h4002<4-3>(0->Donithing in IDLE mode, 1-> continuously take reset frames in idle mode, 2-> continuously take reset - read frames in idle)
    MACIE_STATUS r13 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x401b, 0x0000, true);//ASICPreAmpSrcCurVal set to zero it is unbuffered Mode (no source current)
    MACIE_STATUS r14 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x64ff, 0xeaff, true);//upper 16 clocks
    MACIE_STATUS r15 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x6800, 0x0fff, true);//Science Data Interface Registers Part I
    delay(200);
    //MACIE_STATUS r14 = MACIE_WriteASICReg(card->handle, card->slctASIC, 0x64ff, 0x0000, true);//upper 16 clocks
    //MACIE_STATUS r15 = MACIE_WriteASICReg(card->handle, card->slctASIC, 0x6800, 0x0000, true);
    //MACIE_STATUS r17 = MACIE_WriteASICReg(card->handle, card->slctASIC, 0x4008, 0x0048, true);//to calculate exposure time
    //MACIE_STATUS r18 = MACIE_WriteASICReg(card->handle, card->slctASIC, 0x4009, 0x0801, true);//to calculate exposure time
    if ((r1 != MACIE_OK) || ( r3 != MACIE_OK)	|| ( r4 != MACIE_OK) || ( r5 != MACIE_OK)|| ( r6 != MACIE_OK) || ( r7 != MACIE_OK) || ( r8 != MACIE_OK)|| ( r9 != MACIE_OK) || ( r10 != MACIE_OK)|| ( r11 != MACIE_OK)|| ( r12 != MACIE_OK)|| ( r13 != MACIE_OK)|| ( r14 != MACIE_OK)|| ( r15 != MACIE_OK))
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Please re-initialize the software.");
        return -1;
    }
    HxRGlog.writetoVerbose("Register set succesfully");
    return 0;
}

void setColdWarmTest(instHandle *handle,cmd *cc)
/*!
  *\brief Toggle between Cold & Warm mode
  * HxRG detector can be operated in two modes, [Cold/Warm].
  * When operating the detector at cryogenic temperature,
  * use the cold mode. When operating at room temperature
  * use the warm mode. mode = 0->Cold, 1->Warm.
  * In the cold mode we should set the ASIC to no ktc removal
  * and reset once per frame. In the warm mode, set the
  * detector to reset once per row and no ktc removal.
  * --Used as a standardize function--
  */
{
    HxRGlog.writeto("Setting coldwarm mode. to "+cc->argsVal[0]);
    if (handle->pAcq.isOngoing!=0)
    {
        HxRGlog.writeto("[Failed] acquisition is ongoing");
        sndMsg(cc->sockfd,"Acquisition ongoing",hxrgCMD_ERR_VALUE);
        return ;
    }
    int mode = std::atoi(cc->argsVal[0].c_str());
    if (mode<0 || mode>1)
    {
        HxRGlog.writeto("[Failed] Mode out of range");
        sndMsg(cc->sockfd,"Mode out of range",hxrgCMD_ERR_PARAM_VALUE);
        return;
    }
    if (setColdWarmTest(handle,static_cast<uint>(mode))!=0)
    {
        HxRGlog.writeto("[Failed] Unable to set Cold/Warm mode");
        sndMsg(cc->sockfd,"Unable to set Cold/Warm mode",hxrgCMD_ERR_VALUE);
        return;
    }
    if (mode==0)
    {
        HxRGlog.writeto("[Success] Cold/Warm mode set to Cold");
        sndMsg(cc->sockfd,"Cold/Warm mode set to Cold");
        return;
    }
    HxRGlog.writeto("[Success] Cold/Warm mode set to Warm");
    sndMsg(cc->sockfd,"Cold/Warm mode set to Warm");

    return;
}

int setColdWarmTest(instHandle *handle,uint mode)
/*!
  *\brief Toggle between Cold & Warm mode
  * HxRG detector can be operated in two modes, [Cold/Warm].
  * When operating the detector at cryogenic temperature,
  * use the cold mode. When operating at room temperature
  * use the warm mode. mode = 0->Cold, 1->Warm.
  * In the cold mode we should set the ASIC to no ktc removal
  * and reset once per frame. In the warm mode, set the
  * detector to reset once per row and no ktc removal.
  */
{
    if (mode==0){
        HxRGlog.writetoVerbose("Trying to set the cold mode");}
    else if (mode==1) {
        HxRGlog.writetoVerbose("Trying to set the warm mode");
    }
    else {
        HxRGlog.writetoVerbose("Mode is out of range");
        return -1;
    }


    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4015, mode, true);//h4015<0>(0->Cold, 1-> Warm)
    MACIE_STATUS r2=MACIE_OK;
    if (mode==0)
    {
        r2= MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4016, 0x0000, true);//h4016<0>(0->no ktc removal, 1-> ktc removal), h4016<1>(0->Reset once per frame, 1->reset once per row)
    }
    else if (mode==1)
    {
        r2= MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4016, 0x0002, true);//h4016<0>(0->no ktc removal, 1-> ktc removal), h4016<1>(0->Reset once per frame, 1->reset once per row)
    }
    else {//mode is
        return -1;
    }

    if ( r1 != MACIE_OK || r2 != MACIE_OK)
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }

    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("failed to go back to idle");
        return -1;
    }
    if (mode==0){
        HxRGlog.writetoVerbose("HxRG now in cold mode");}
    else {
        HxRGlog.writetoVerbose("HxRG now in warm mode");
    }
    handle->head.coldWarmMode = static_cast<int>(mode);
    return 0;
}
void setNbOutput(instHandle *handle, cmd *cc )
/*!
  *\brief Number of output used
  * Sets the number of amplifier used to read out the
  * detector. Options are 1,4,32. Return -1 if failed.
  * --Used as a standardized function--
  */
{
    if (handle->pAcq.isOngoing!=0)
    {
        sndMsg(cc->sockfd,"Acquisition ongoing",hxrgCMD_ERR_VALUE);
        return ;
    }
    uint output=static_cast<unsigned int>(std::atoi(cc->argsVal[0].c_str()));
    if (output!=1 || output!=4 || output!=32)
    {
        sndMsg(cc->sockfd,"Number of output out of range",hxrgCMD_ERR_PARAM_VALUE);
        return;
    }
    if (setNbOutput(handle,output)!=0)
    {
        sndMsg(cc->sockfd,"Unable to set the number of output",hxrgCMD_ERR_VALUE);
        return;
    }
    sndMsg(cc->sockfd,"Number of output set to "+std::to_string(output));
    return;
}
int setNbOutput(instHandle *handle, uint nbOut )
/*!
  *\brief Number of output used
  * Sets the number of amplifier used to read out the
  * detector. Options are 1,4,32. Return -1 if failed.
  */
{
    HxRGlog.writetoVerbose("Change of number of output requested. [request: "+std::to_string(nbOut)+"]");
    if (nbOut!=1 && nbOut!=4 && nbOut!=32)
    {
        HxRGlog.writetoVerbose("Wrong number of output requested.");
        return -1;
    }
    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4011, nbOut, true);//32 output
    if ( r1 != MACIE_OK)
    {
        std::cout << "ASIC configuration failed - write ASIC registers" << std::endl;
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");

        return -1;
    }

    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("re-upload the MCD.");
        return -1;
    }


    handle->head.nbOut=static_cast<int>(nbOut);
    handle->pDet.nbOutput=nbOut;
    HxRGlog.writetoVerbose("Success, number of output is now "+std::to_string(nbOut));
    handle->pDet.nbOutput = nbOut;

    return 0;

}

int setFF(instHandle *handle)
/*!
  *\brief Set the full frame mode
  * After setting a window, this function is used
  * to go back to full frame mode
  */
{
    HxRGlog.writetoVerbose("Setting back the full frame mode");

    MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4018, 0x0000, true);
    if (r2!= MACIE_OK)
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
    delay(200);

   if (reconfigure(handle)!=0)
   {
       HxRGlog.writetoVerbose("re-upload MCD");
       return -1;
   }



    HxRGlog.writetoVerbose("Full frame mode is now set");
    delay(100);
    return 0;
}

int setWindowMode(instHandle *handle)
/*!
  *\brief set the window mode.
  * Prior to calling this function, the handle
  * winStart/stop member should be define.
  */
{
    HxRGlog.writetoVerbose("Setting window mode.");


    MACIE_STATUS r = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4018, 0x0002, true);
    if (r!= MACIE_OK)
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4020, static_cast<uint>(handle->pAcq.winStartX), true);
    MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4021, static_cast<uint>(handle->pAcq.winStopX), true);
    MACIE_STATUS r3 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4022, static_cast<uint>(handle->pAcq.winStartY), true);
    MACIE_STATUS r4 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4023, static_cast<uint>(handle->pAcq.winStopY), true);

    if ( r1 != MACIE_OK || r2 != MACIE_OK || r3 != MACIE_OK || r4 != MACIE_OK)
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }

    delay(100);

    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Re-upload the MCD.");
        return -1;

    }
    HxRGlog.writetoVerbose("Window mode set successfully.");
    HxRGlog.writetoVerbose("X start: "+std::to_string(handle->pAcq.winStartX)+"X stop: "+std::to_string(handle->pAcq.winStopX));
    HxRGlog.writetoVerbose("Y start: "+std::to_string(handle->pAcq.winStartY)+"Y stop: "+std::to_string(handle->pAcq.winStopY));
    delay(100);
    return 0;
}

int setMux(instHandle *handle,uint mux)
/*!
  *\brief Used for optimization
  * Sets the mux value. Experimental function. Do not use this unless
  * you know what you are doing.
  */
{
    //read value of register h6192 ->value
    HxRGlog.writetoVerbose("Starting ASIC configuration");
    uint value;
    MACIE_STATUS r1 = MACIE_ReadASICReg(handle->card.handle,handle->card.slctASIC, 0x6192, &value, false,true);
    if ((r1 != MACIE_OK))
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
    uint mask = 1023;//mask->0000 0011 1111 1111
    value = (value&mask) | (mux <<10);
    //set value in register h6192
    MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x6192, value, true);
    if ((r2 != MACIE_OK))
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Re-upload the MCD.");
        return -1;

    }
    return 0;
}
void columnDeselect(instHandle *handle,cmd *cc)
/*!
  *\brief Setting Columns
  * mode [0]: H4RG-10
  * mode [1]: H4RG-15 Disable column de-select
  * mode [2]: H4RG-15 Enable column de-select
  * --Used as a standardized function--
  */
{
    if (handle->pAcq.isOngoing!=0)
    {
        sndMsg(cc->sockfd,"Acquisition ongoing",hxrgCMD_ERR_VALUE);
        return ;
    }
    int mode = (std::atoi(cc->argsVal[0].c_str()));
    if (mode<0 || mode>2)
    {
        sndMsg(cc->sockfd,"out of range",hxrgCMD_ERR_PARAM_VALUE);
        return ;
    }
    else if (columnDeselect(handle,mode)!=0) {
        sndMsg(cc->sockfd,"Unable to change mode",hxrgCMD_ERR_VALUE);
        return ;
    }
    else {
        if (mode==0)
        {
            sndMsg(cc->sockfd,"Mode changed to: H4RG-10");
        }
        else if (mode==1) {
            sndMsg(cc->sockfd,"Mode changed to: H4RG-15 Disable column de-select");
        }
        else {
            sndMsg(cc->sockfd,"Mode changed to: H4RG-15 Enable column de-select");
        }
        return;
    }
}
int columnDeselect(instHandle *handle,int mode)
/*!
  *\brief Setting Columns
  * mode [0]: H4RG-10
  * mode [1]: H4RG-15 Disable column de-select
  * mode [2]: H4RG-15 Enable column de-select
  * return 0 if successfull, -1 if not
  */
{
    HxRGlog.writetoVerbose("Setting columndeselect mode to "+std::to_string(mode));
    MACIE_STATUS r1;
    if (mode==0)//H4RG-10
    {
        r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4050, 0x0000, true);//H4RG column-15 de-select

    }
    else if (mode==1)//H4RG-15 Disable column de-select
    {
        r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4050, 0x0001, true);//H4RG column-15 de-select

    }
    else if (mode==2)//H4RG-15 Enable column de-select
    {
        r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4050, 0x0003, true);//H4RG column-15 de-select

    }
    else
    {
        HxRGlog.writetoVerbose("Wrong mode option.");
        return -1;
    }
    if ((r1 != MACIE_OK))
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }

    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Re-upload the MCD.");
        return -1;

    }

    HxRGlog.writetoVerbose("Register set succesfully");

    return 0;
}
int initASIC(instHandle *handle)
/*!
  *\brief Initialize ASIC
  * return 0 if success, -1 if not.
  */
{
    if (InitializeASIC(handle)!=0)
    {
        HxRGlog.writetoVerbose("MACIE_GetAvailableASICs failed");
        return -1;
    }

    delay(100);
    handle->card.slctASIC = handle->card.avaiASICs & 1; // ASIC1

    if (MACIE_ReadASICReg(handle->card.handle, handle->card.slctASIC, 0x6100, &handle->card.val, false, true)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to read ASIC register.");
        return -1;
    }

    printf("\nASIC h6100: %u\n\n",handle->card.val);

    HxRGlog.writetoVerbose("MACIE_GetAvailableASICs succeeded");

    delay(2000);
    if (initializeRegister(handle)!=0){return false;}
    delay(1000);
    return 0;
}
void upMCD(instHandle *handle,cmd *cc)
/*!
  * Upload a microcode. Argument is file. A reconfiguration
  * will be triggered afterward.
  */

{
    if ((*cc)["file"]=="")
    {
        sndMsg(cc->sockfd,"You must specify a file name.",hxrgCMD_ERR_NO_FILENAME);
        return ;
    }
    if (!is_file_exist((*cc)["file"]))
    {
        sndMsg(cc->sockfd,"File does not exsit.",hxrgCMD_ERR_NO_FILENAME);
        return ;
    }
    HxRGlog.writetoVerbose("Uploading MCD: "+(*cc)["file"]);
    if (MACIE_DownloadASICFile(handle->card.handle,handle->card.slctASIC,(*cc)["file"].c_str(),true)!=MACIE_OK)
    {
        sndMsg(cc->sockfd,std::string(MACIE_Error()),hxrgCMD_ERR_VALUE);
        HxRGlog.writetoVerbose(MACIE_Error());
        return ;
    }
    else {
        delay(500);
        if (reconfigure(handle)!=0)
        {
            sndMsg(cc->sockfd,"Unable to reconfigure the ASIC",hxrgCMD_ERR_VALUE);
            return ;

        }
        else {
            sndMsg(cc->sockfd,"MCD upload successfully.");
            return;
        }

    }
}
int uploadMCD(instHandle *handle,std::string file)
/*!
  *\brief Upload a new MCD file
  * return 0 if success, -1 if not.
  */
{
    HxRGlog.writetoVerbose("Uploadeding MCD: "+file);

    delay(100);
    if (MACIE_DownloadASICFile(handle->card.handle,handle->card.slctASIC,file.c_str(),true)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose(MACIE_Error());
        return -1;
    }
    else
    {
        HxRGlog.writetoVerbose("MCD uploaded successfully");
    }

    delay(500);
    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4010, handle->pDet.HxRG, true);//H2RG or H4RG
    MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle,handle->card.slctASIC, 0x4011, handle->pDet.nbOutput, true);//32 output
    MACIE_STATUS r3 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4019, handle->pDet.preampInputScheme, true); //h4019<0>(0->VREFMAIN as v1,v2,.., 1->use InPcommon),h4019<1>(0->use input onfiguration set in <0>, 1-> Overide with register h5100)
    //MACIE_STATUS r4 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x5100, handle->pDet.preampInputVal, true);//value use for h4019
    MACIE_STATUS r5 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4050, 0x0001, true);//H4RG column-15 de-select
    if ((r1 != MACIE_OK) || ( r2 != MACIE_OK) || ( r3 != MACIE_OK)	|| ( r5 != MACIE_OK) )
    {
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
        return -1;
    }
HxRGlog.writetoVerbose("Reconfiguring the ASIC.");
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Re-upload the MCD.");
        return -1;

    }

    HxRGlog.writetoVerbose("ASIC reconfigured successfully.");
    return 0;
}
void setGain(instHandle *handle,cmd *cc)
/*!
  *\brief set Gain
  * Set the gain of the ASIC preamplifier
  * 0 successfull, -1 failed
  * Used as a standardized function
  */
{
    if (handle->pAcq.isOngoing!=0)
    {
        sndMsg(cc->sockfd,"Acquisition ongoing",hxrgCMD_ERR_VALUE);
        return ;
    }
    int gain = std::atoi(cc->argsVal[0].c_str());
    if (gain<0 || gain>15)
    {
        sndMsg(cc->sockfd,"Gain is out of range",hxrgCMD_ERR_PARAM_VALUE);
        return ;
    }
    if (setGain(handle,static_cast<unsigned int>(gain))!=0)
    {
        sndMsg(cc->sockfd,"Failed to set the gain",hxrgCMD_ERR_VALUE);
        return ;
    }
    else {
        std::string GainCode[16]={"-3 dB small Cin","0 dB small Cin","3 dB small Cin","6 dB small Cin","6 dB large Cin","9 dB small Cin","9 dB large Cin","12 dB small Cin","12 dB large Cin","15 dB small Cin","15 dB large Cin","18 dB small Cin","18 dB large Cin","21 dB large Cin","24 dB large Cin","27 dB large Cin"};

        sndMsg(cc->sockfd,"Gain set to "+GainCode[gain]);
        return ;
    }

}
void powerControl(instHandle *handle,cmd *cc)
{

    if (cc->argsVal[0].compare("on")==0)
    {

        MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4013, 0x0000, true);//preamp gain
        if ( r1 != MACIE_OK)
        {

            std::cout << "ASIC configuration failed - write ASIC registers" << std::endl;
            HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
            sndMsg(cc->sockfd,"Unable to set register",hxrgCMD_ERR_PARAM_UNKNOWN);
            return;
        }
        delay(100);
        if (reconfigure(handle)!=0)
        {
            HxRGlog.writetoVerbose("Re-upload the MCD.");
            return ;
        }
        delay(100);
        sndMsg(cc->sockfd);
    }
    else if (cc->argsVal[0].compare("off")==0) {


        MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4013, 0x0001, true);//preamp gain
        if ( r1 != MACIE_OK)
        {

            std::cout << "ASIC configuration failed - write ASIC registers" << std::endl;
            HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");
            sndMsg(cc->sockfd,"Unable to set register",hxrgCMD_ERR_PARAM_UNKNOWN);
            return;
        }
        delay(100);
        if (reconfigure(handle)!=0)
        {
            HxRGlog.writetoVerbose("Re-upload the MCD.");
            return ;
        }
        delay(100);
        sndMsg(cc->sockfd);
    }
    else {
        sndMsg(cc->sockfd,"wrong param value",hxrgCMD_ERR_PARAM_VALUE);
    }
}
int setGain(instHandle *handle,unsigned int gain)
/*!
  *\brief set Gain
  * Set the gain of the ASIC preamplifier
  * 0 successfull, -1 failed
  */
{
    std::string GainCode[16]={"-3 dB small Cin","0 dB small Cin","3 dB small Cin","6 dB small Cin","6 dB large Cin","9 dB small Cin","9 dB large Cin","12 dB small Cin","12 dB large Cin","15 dB small Cin","15 dB large Cin","18 dB small Cin","18 dB large Cin","21 dB large Cin","24 dB large Cin","27 dB large Cin"};

    if (handle->pAcq.simulator!=1){
    HxRGlog.writetoVerbose("Setting Gain");



    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x401a, gain, true);//preamp gain

    if ( r1 != MACIE_OK)
    {
        std::cout << "ASIC configuration failed - write ASIC registers" << std::endl;
        HxRGlog.writetoVerbose("ASIC configuration failed - write ASIC registers");

        return -1;
    }
    delay(100);
    handle->pDet.gain = gain;
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("Re-upload the MCD.");
        return -1;
    }
    delay(100);

    HxRGlog.writetoVerbose("Gain set to "+GainCode[gain]);
    return 0;
    }
    else {
        handle->pDet.gain = gain;
        HxRGlog.writetoVerbose("(simulation) Gain set to "+GainCode[gain]);
        return 0;
    }
}

int setRegister(instHandle *handle)
/*!
  *\brief set detector parameters register
  * This must be caled before triggering and exposure. It
  * will set the read, ramp, reset, group, and drop parameters.
  * return 0,-1 [success/fail]
  */
{
    HxRGlog.writetoVerbose("\tSetting paramters");
    HxRGlog.writetoVerbose("\t\tRead nb: "+std::to_string(handle->pAcq.read));
    HxRGlog.writetoVerbose("\t\tRamp nb: "+std::to_string(handle->pAcq.ramp));
    HxRGlog.writetoVerbose("\t\tReset nb: "+std::to_string(handle->pAcq.reset));
    HxRGlog.writetoVerbose("\t\tGroup nb: "+std::to_string(handle->pAcq.group));
    HxRGlog.writetoVerbose("\t\tDrop nb: "+std::to_string(handle->pAcq.drop));
    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4001, handle->pAcq.read, true);//NREADS
    MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4003, handle->pAcq.ramp, true);//NRamps
    MACIE_STATUS r3 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4000, handle->pAcq.reset, true);//NResets
    MACIE_STATUS r4 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4004, handle->pAcq.group, true);//NGroups
    MACIE_STATUS r5 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4005, handle->pAcq.drop, true);//NDrops
    if ((r1 != MACIE_OK) || ( r2 != MACIE_OK) || ( r3 != MACIE_OK)	|| ( r4 != MACIE_OK) || ( r5 != MACIE_OK) )
    {
        HxRGlog.writetoVerbose("\tASIC configuration failed - write ASIC registers");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("\tRe-upload the MCD.");
        return -1;
    }
    HxRGlog.writetoVerbose("\tParameters set successfully");


    return 0;
}
int setRegister(instHandle *handle,uint ramp)
/*!
  *\brief set detector parameters register
  * This must be caled before triggering and exposure. It
  * will set the read, ramp, reset, group, and drop parameters.
  * ramp is override by uint ramp
  * return 0,-1 [success/fail]
  */
{
    HxRGlog.writetoVerbose("\tSetting paramters");
    HxRGlog.writetoVerbose("\t\tRead nb: "+std::to_string(handle->pAcq.read));
    HxRGlog.writetoVerbose("\t\tRamp nb: "+std::to_string(ramp));
    HxRGlog.writetoVerbose("\t\tReset nb: "+std::to_string(handle->pAcq.reset));
    HxRGlog.writetoVerbose("\t\tGroup nb: "+std::to_string(handle->pAcq.group));
    HxRGlog.writetoVerbose("\t\tDrop nb: "+std::to_string(handle->pAcq.drop));
    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4001, handle->pAcq.read, true);//NREADS
    MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4003, ramp, true);//NRamps
    MACIE_STATUS r3 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4000, handle->pAcq.reset, true);//NResets
    MACIE_STATUS r4 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4004, handle->pAcq.group, true);//NGroups
    MACIE_STATUS r5 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4005, handle->pAcq.drop, true);//NDrops
    if ((r1 != MACIE_OK) || ( r2 != MACIE_OK) || ( r3 != MACIE_OK)	|| ( r4 != MACIE_OK) || ( r5 != MACIE_OK) )
    {
        HxRGlog.writetoVerbose("\tASIC configuration failed - write ASIC registers");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("\tRe-upload the MCD.");
        return -1;
    }
    HxRGlog.writetoVerbose("\tParameters set successfully");


    return 0;
}
int setRegister(instHandle *handle,uint ramp,uint read)
/*!
  *\brief set detector parameters register
  * This must be caled before triggering and exposure. It
  * will set the read, ramp, reset, group, and drop parameters.
  * ramp is override by uint ramp
  * read is override by uint read
  * return 0,-1 [success/fail]
  */
{
    HxRGlog.writetoVerbose("\tSetting paramters");
    HxRGlog.writetoVerbose("\t\tRead nb: "+std::to_string(read));
    HxRGlog.writetoVerbose("\t\tRamp nb: "+std::to_string(ramp));
    HxRGlog.writetoVerbose("\t\tReset nb: "+std::to_string(handle->pAcq.reset));
    HxRGlog.writetoVerbose("\t\tGroup nb: "+std::to_string(handle->pAcq.group));
    HxRGlog.writetoVerbose("\t\tDrop nb: "+std::to_string(handle->pAcq.drop));
    MACIE_STATUS r1 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4001, read, true);//NREADS
    MACIE_STATUS r2 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4003, ramp, true);//NRamps
    MACIE_STATUS r3 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4000, handle->pAcq.reset, true);//NResets
    MACIE_STATUS r4 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4004, handle->pAcq.group, true);//NGroups
    MACIE_STATUS r5 = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x4005, handle->pAcq.drop, true);//NDrops
    if ((r1 != MACIE_OK) || ( r2 != MACIE_OK) || ( r3 != MACIE_OK)	|| ( r4 != MACIE_OK) || ( r5 != MACIE_OK) )
    {
        HxRGlog.writetoVerbose("\tASIC configuration failed - write ASIC registers");
        return -1;
    }
    if (reconfigure(handle)!=0)
    {
        HxRGlog.writetoVerbose("\tRe-upload the MCD.");
        return -1;
    }
    HxRGlog.writetoVerbose("\tParameters set successfully");


    return 0;
}
int createFolder(char *folder,char *saveFolder,const char *path)
/*!
  *\brief mkdir a folder
  * This function takes for arguments the folder name
  * (use makeFolderName function 1st) and saveFolder
  * (the complete path is writen into this array. The
  * last argument path is the path where the folder
  * "folder" will be created. The function will creates
  * the folder saveFolder.
  */
{
    char command[1024];
    int check=0;
    sprintf(command,"mkdir -p %s\n",join(path,folder).c_str());
    sprintf(saveFolder,"%s/",join(path,folder).c_str());
    check = system(command);
    if (check!=0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int makeFolderName(char *folder)
/*!
  *\brief Create a folder name
  * Create a folder name with format yymmddhhmmss.
  */
{
    //This fonction creates a folder name of this form: YYYYMMDDHHMMSS and
    //writes it in the array folder
    time_t t = time(0);
    struct tm * now = localtime(&t);
    int m = now->tm_mon+1,y = now->tm_year+1900,d =now->tm_mday,h = now->tm_hour,min = now->tm_min;
    double s = now->tm_sec;
    sprintf(folder,"%d%2.2d%2.2d%2.2d%2.2d%2.2d",y,m,d,h,min,static_cast<int>(s));
    return 0;
}




int makeHeader(MACIE_FitsHdr *pHeaders,instHandle *handle){
    std::string GainCode[16]={"-3 dB small Cin","0 dB small Cin","3 dB small Cin","6 dB small Cin","6 dB large Cin","9 dB small Cin","9 dB large Cin","12 dB small Cin","12 dB large Cin","15 dB small Cin","15 dB large Cin","18 dB small Cin","18 dB large Cin","21 dB large Cin","24 dB large Cin","27 dB large Cin"};

    strncpy(pHeaders[0].key, "ASICGAIN", sizeof(pHeaders[0].key));
    pHeaders[0].iVal = static_cast<int>(handle->pDet.gain);
    pHeaders[0].valType = HDR_INT;
    strncpy(pHeaders[0].comment, GainCode[static_cast<int>(handle->pDet.gain)].c_str(), sizeof(pHeaders[0].comment));

    strncpy(pHeaders[1].key, "AEEXPT", sizeof(pHeaders[1].key));
    pHeaders[1].fVal = handle->pDet.effExpTime;
    pHeaders[1].valType = HDR_FLOAT;
    strncpy(pHeaders[1].comment, "Average effective exposure time", sizeof(pHeaders[1].comment));

    strncpy(pHeaders[2].key, "ACQDATE", sizeof(pHeaders[2].key));
    strncpy(pHeaders[2].sVal, handle->head.ACQDATE.c_str(), sizeof(pHeaders[2].sVal));
    pHeaders[2].valType = HDR_STR;
    strncpy(pHeaders[2].comment, "Acquisition date", sizeof(pHeaders[2].comment));

    strncpy(pHeaders[3].key, "ASIC_NUM", sizeof(pHeaders[3].key));
    strncpy(pHeaders[3].sVal, handle->head.ASICSERIAL.c_str(), sizeof(pHeaders[3].sVal));
    pHeaders[3].valType = HDR_STR;
    strncpy(pHeaders[3].comment, "ASIC serial number", sizeof(pHeaders[3].comment));

    strncpy(pHeaders[4].key, "SCA_ID", sizeof(pHeaders[4].key));
    strncpy(pHeaders[4].sVal, handle->head.SCASERIAL.c_str(), sizeof(pHeaders[4].sVal));
    pHeaders[4].valType = HDR_STR;
    strncpy(pHeaders[4].comment, "SCA number", sizeof(pHeaders[4].comment));

    strncpy(pHeaders[5].key, "MUXTYPE", sizeof(pHeaders[5].key));
    pHeaders[5].iVal =  static_cast<int>(handle->pDet.HxRG);
    pHeaders[5].valType = HDR_INT;
    strncpy(pHeaders[5].comment, "1- H1RG; 2- H2RG; 4- H4RG", sizeof(pHeaders[5].comment));

    strncpy(pHeaders[6].key, "NBOUTPUT", sizeof(pHeaders[6].key));
    pHeaders[6].iVal =  static_cast<int>(handle->pDet.nbOutput);
    pHeaders[6].valType = HDR_INT;
    strncpy(pHeaders[6].comment, "Number of amplifier used", sizeof(pHeaders[6].comment));

    strncpy(pHeaders[7].key, "HXRGVER", sizeof(pHeaders[7].key));
    strncpy(pHeaders[7].sVal, handle->head.SOFTVERSION.c_str(), sizeof(pHeaders[7].sVal));
    pHeaders[7].valType = HDR_STR;
    strncpy(pHeaders[7].comment, "acquisition software version number", sizeof(pHeaders[7].comment));

    strncpy(pHeaders[8].key, "READ", sizeof(pHeaders[8].key));
    pHeaders[8].iVal= handle->head.READ;
    pHeaders[8].valType = HDR_INT;
    strncpy(pHeaders[8].comment, "Read number in ramp sequence", sizeof(pHeaders[8].comment));

    strncpy(pHeaders[9].key, "RAMP", sizeof(pHeaders[9].key));
    pHeaders[9].iVal = handle->head.RAMP;
    pHeaders[9].valType = HDR_INT;
    strncpy(pHeaders[9].comment, "Ramp sequence number", sizeof(pHeaders[9].comment));

    strncpy(pHeaders[10].key, "JD", sizeof(pHeaders[10].key));
    strncpy(pHeaders[10].sVal, handle->head.JD.c_str(), sizeof(pHeaders[10].sVal));
    pHeaders[10].valType = HDR_STR;
    strncpy(pHeaders[10].comment, "Julian date taken before the file is written on disk", sizeof(pHeaders[10].comment));

    strncpy(pHeaders[11].key, "SEQID", sizeof(pHeaders[11].key));
    char uniqueID[20];
    memset(uniqueID,0,20);
    sprintf(uniqueID,"%ld",handle->head.ID);
    strncpy(pHeaders[11].sVal,uniqueID,sizeof (pHeaders[11].sVal) );
    pHeaders[11].valType = HDR_STR;
    strncpy(pHeaders[11].comment, "Unique Identification number for the sequence", sizeof(pHeaders[11].comment));

    strncpy(pHeaders[12].key, "WARMTST", sizeof(pHeaders[12].key));
    pHeaders[12].iVal = handle->head.coldWarmMode;
    pHeaders[12].valType = HDR_INT;
    strncpy(pHeaders[12].comment, "0- cold test; 1- warm test ", sizeof(pHeaders[12].comment));

    strncpy(pHeaders[13].key, "COLUMN", sizeof(pHeaders[13].key));
    pHeaders[13].iVal = handle->head.columnMode;
    pHeaders[13].valType = HDR_INT;
    strncpy(pHeaders[13].comment, "0->H4RG-10, 1->H4RG-15 Col.Disa., 2->H4RG-15 Col.Ena.", sizeof(pHeaders[13].comment));

    strncpy(pHeaders[14].key, "WINDOW", sizeof(pHeaders[14].key));
    pHeaders[14].iVal = handle->head.windowMode;
    pHeaders[14].valType = HDR_INT;
    strncpy(pHeaders[14].comment, "0->full frame; 1-> window mode", sizeof(pHeaders[14].comment));

    strncpy(pHeaders[15].key, "XSTART", sizeof(pHeaders[15].key));
    pHeaders[15].iVal = static_cast<int>(handle->pAcq.winStartX);
    pHeaders[15].valType = HDR_INT;
    strncpy(pHeaders[15].comment, "start pixel in X", sizeof(pHeaders[15].comment));

    strncpy(pHeaders[16].key, "XSTOP", sizeof(pHeaders[16].key));
    pHeaders[16].iVal = static_cast<int>(handle->pAcq.winStopX);
    pHeaders[16].valType = HDR_INT;
    strncpy(pHeaders[16].comment, "stop pixel in X", sizeof(pHeaders[16].comment));

    strncpy(pHeaders[17].key, "LTIME", sizeof(pHeaders[17].key));
    strncpy(pHeaders[17].sVal, handle->head.timeLocal.c_str(), sizeof(pHeaders[17].sVal));
    pHeaders[17].valType = HDR_STR;
    strncpy(pHeaders[17].comment, "Local date-time.", sizeof(pHeaders[17].comment));

    strncpy(pHeaders[18].key, "GMTTIME", sizeof(pHeaders[18].key));
    strncpy(pHeaders[18].sVal, handle->head.timeGMT.c_str(), sizeof(pHeaders[18].sVal));
    pHeaders[18].valType = HDR_STR;
    strncpy(pHeaders[18].comment, "GMT date-time.", sizeof(pHeaders[18].comment));

    strncpy(pHeaders[19].key, "MACIELIB", sizeof(pHeaders[19].key));
    pHeaders[19].fVal = handle->macie_libv;
    pHeaders[19].valType = HDR_FLOAT;
    strncpy(pHeaders[19].comment, "MACIE library version number.", sizeof(pHeaders[19].comment));

    strncpy(pHeaders[20].key, "SOURCE", sizeof(pHeaders[20].key));
    strncpy(pHeaders[20].sVal, std::string(handle->head.OBSERVATORY+"-"+handle->head.OBSLOCATION).c_str(), sizeof(pHeaders[20].sVal));
    pHeaders[20].valType = HDR_STR;
    strncpy(pHeaders[20].comment, "Source of data", sizeof(pHeaders[20].comment));




//    strncpy(pHeaders[17].key, "mcdOpt", sizeof(pHeaders[17].key));
//    strncpy(pHeaders[17].sVal, handle->mcd.mcdOpt.c_str(), sizeof(pHeaders[17].sVal));
//    pHeaders[17].valType = HDR_STR;
//    strncpy(pHeaders[17].comment, "optimized mcd", sizeof(pHeaders[17].comment));

//    strncpy(pHeaders[18].key, "optAddr", sizeof(pHeaders[18].key));
//    strncpy(pHeaders[18].sVal, opt.addr.c_str(), sizeof(pHeaders[18].sVal));
//    pHeaders[18].valType = HDR_STR;
//    strncpy(pHeaders[18].comment, "address beeing tested", sizeof(pHeaders[18].comment));

//    strncpy(pHeaders[19].key, "optVal", sizeof(pHeaders[19].key));
//    strncpy(pHeaders[19].sVal, opt.val.c_str(), sizeof(pHeaders[19].sVal));
//    pHeaders[19].valType = HDR_STR;
//    strncpy(pHeaders[19].comment, "value beeing tested", sizeof(pHeaders[19].comment));

return 0;
}

void setParam(instHandle *handle,cmd *cc)
/*!
 * Possible arguments are read, reset, drop, ramp, group
 *
 */
{
      std::string param = "";
      param = (*cc)["read"];
      if (param.compare("")!=0)
      {
          HxRGlog.writetoVerbose("Read set to "+param);
          handle->pAcq.read = static_cast<uint>(std::atoi(param.c_str()));
      }
      param = (*cc)["ramp"];
      if (param.compare("")!=0)
      {
          HxRGlog.writetoVerbose("Ramp set to "+param);
          handle->pAcq.ramp = static_cast<uint>(std::atoi(param.c_str()));
      }
      param = (*cc)["drop"];
      if (param.compare("")!=0)
      {
          HxRGlog.writetoVerbose("Drop set to "+param);
          handle->pAcq.drop = static_cast<uint>(std::atoi(param.c_str()));
      }
      param = (*cc)["group"];
      if (param.compare("")!=0)
      {
          HxRGlog.writetoVerbose("Group set to "+param);
          handle->pAcq.group = static_cast<uint>(std::atoi(param.c_str()));
      }
      param = (*cc)["reset"];
      if (param.compare("")!=0)
      {
          HxRGlog.writetoVerbose("Reset set to "+param);
          handle->pAcq.reset = static_cast<uint>(std::atoi(param.c_str()));
      }
      sndMsg(cc->sockfd);


   

}
void acquisition(instHandle *handle,cmd *cc)
{


    //set ramp to a large number
    //handle->pAcq.ramp = static_cast<uint>(7751);

    //set sockfd to handle
    //handle->nAcq.sockfd = cc->sockfd;
    handle->nAcq.keepGoing = 1;
    sndMsg(cc->sockfd);
    //acquisition(handle,handle->pAcq.simulator);
    if (handle->pAcq.simulator==1)
    {
        acquisition_sim(handle);
    }
    else {
        acquisition(handle);
    }
    return ;

}
void acquisition_eng(instHandle *handle,cmd *cc)
{


    //set ramp to a large number
    //handle->pAcq.ramp = static_cast<uint>(7751);

    //set sockfd to handle
    //handle->nAcq.sockfd = cc->sockfd;
    handle->nAcq.keepGoing = 1;
    sndMsg(cc->sockfd);
    //acquisition(handle,handle->pAcq.simulator);
    if (handle->pAcq.simulator==1)
    {
        acquisition_sim(handle);
    }
    else {
        acquisition_eng(handle);
    }
    return ;

}
void nirps_start(instHandle *handle)
/*!
 * \brief
 * \param mode
 */

{

    std::string msg="";
    int fd = create_socket(5020);

    cmd *c = new cmd;
    while (1) {

       c->recvCMD(fd);
       //std::cout<<"[debug] Command recv."<<std::endl;
       if (handle->state==ACQ && handle->pAcq.isOngoing==1)
       {
           handle->nAcq.keepGoing++;
           //std::cout<<"[debug] sending OK"<<std::endl;
           sndMsg(c->sockfd);
       }
       else {
           //transfer command as for the message_handler
           //std::cout<<"[debug] write INCOMINGCMD"<<std::endl;
           write_socket(PORT4MSG,std::string("INCOMINGCMD"));
           //std::cout<<"[debug] read READY"<<std::endl;
           read_socket(PORT4MSG,&msg);

           if (msg.compare(READY)!=0)
           {
               //std::cout<<"[debug] send problem"<<std::endl;
               sndMsg(c->sockfd,"problem with the state handler. Restart the software",hxrgCMD_ERR_VALUE);
               HxRGlog.writetoVerbose("The state handler failed in the acquisition thread. [a]");
               continue;
           }


           // Sending command to state_handler
           c->name = "START-ACQ";
           c->state = ACQ;
           handle->nAcq.status = hxrgEXP_PENDING;
           //std::cout<<"[debug] sending command"<<std::endl;
           c->sendCMD(PORT4MSG);
            //std::cout<<"[debug] rcv cmd"<<std::endl;
           // Validation from state_handler
           read_socket(PORT4MSG,&msg);
           if (msg.compare(READY)!=0)
           {
               //std::cout<<"[debug] send problem"<<std::endl;
               sndMsg(c->sockfd,"problem with the state handler. Restart the software",hxrgCMD_ERR_VALUE);
               HxRGlog.writetoVerbose("The state handler failed in the acquisition thread. [b]");
               continue;
           }
       }

    }


}
int wait_for_halt(instHandle *handle)
{
    int to = 200;// in ms
    for (int i = 0; i < 130; ++i) {
        if (handle->card.haltTriggered==0)
        {
            return 0;
        }
        delay(to);
    }
    return -1;

}
void acq_t(instHandle *handle)
/*!
  *\brief acquitiion thread
  * thread that listen to threaded acquisition extended command.
  * Commands are halt and keepGoing. The halt will trigger the end
  * of a ramp. The current ramp will no be saved. The keepGoing command
  * will tell the acquisition to trigger a new ramp. The acquisition
  * will automatically exit after the second read of the next ramp if
  * the keepGoing command is not received.
  */
{
    std::string msg="";
    int fd = create_socket(5012);
     
    handle->nAcq.sockfd = -1;
    cmd *c = new cmd;
    while (1) {
    
        c->recvCMD(fd);

        


        if (c->name.compare("halt")==0 || c->name.compare("END")==0 || c->name.compare("ABORT")==0)
        {
            if (handle->nAcq.sockfd!=c->sockfd){
                handle->nAcq.sockfd = c->sockfd;
            }
            //trigger halt
            handle->card.haltTriggered=1;
            if (wait_for_halt(handle)!=0)
            {
                std::string m="NOK Halt timeout.\n";
                send(c->sockfd,m.c_str(),strlen(m.c_str()),0);
            }
            else {
                sndMsg(c->sockfd);
            }

        }
        else if (c->name.compare("keepGoing")==0) {
            handle->nAcq.keepGoing+=1;
            sndMsg(c->sockfd);
        }
    }
}

void closeMacie(instHandle *handle,cmd *cc)
{
    if (MACIE_Free()!=MACIE_OK)
    {
        sndMsg(cc->sockfd,"Cannot close the communication device",hxrgCMD_ERR_VALUE);
    }
    else {
        sndMsg(cc->sockfd);
    }
    exit(0);
}
std::string getTime()
/*!
  *Return the GMT date/time in the form yyyy-mm-ddTHH:MM::SS
  */
{
    time_t now = time(0);
    tm *gmtm = gmtime(&now);

    char datetime[23];
    memset(datetime,'\0',23);
    sprintf(datetime,"%2.2d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d",1900+gmtm->tm_year,gmtm->tm_mon,gmtm->tm_mday,gmtm->tm_hour,gmtm->tm_min,gmtm->tm_sec);
    return std::string(datetime);
}

void acquisition(instHandle *handle)
/*
 * Description: trigger an acquisition.
 */
{
    //set a few status
    handle->nAcq.status = hxrgEXP_INTEGRATING;
    handle->card.haltTriggered = 0;
    handle->pAcq.isOngoing=1;//make sure no other command are sent
    long id=0;
    handle->head.ID = id;
    //variables
    double mjd_obs = 0;
    std::string date_obs="";
    int socket_size = 0;
    double m_intTime = 0;
    int nbWords =0;
    uint16_t moreDelay = 32/handle->pDet.nbOutput *6000;//small delay in ms
    double t_intTime = static_cast<double>(handle->pDet.frameX)*static_cast<double>(handle->pDet.frameY)*static_cast<double>(handle->pDet.readoutTime)/handle->pDet.nbOutput;
    double triggerTimeout = (t_intTime * (handle->pAcq.reset) + static_cast<double>(moreDelay)/1000.0);//trigger timeout
    long f_size = frameSize(handle);
    char file[1024];
    int ramp=1;
    int read=1;

    //create data array
    unsigned short *pData= new unsigned short[f_size];

    if (handle->nAcq.fitsMTD){create_TS_folder(handle);}//create folder if we want to save the data
    update_TS_header(handle);//put the date timestamp in the header

    MACIE_FitsHdr pHeaders[21];//old style header
    makeHeader(pHeaders, handle);//populate old style header
    pHeader pHead(pHeaders,21);//declare new style header initialized with old header




    //::::::::::::::::::: set parameter and reconfigure :::::::::::::::::
    delay(500);

    if (setRegister(handle)!=0){
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }


    HxRGlog.writetoVerbose("Configuring science interface.");
    if (MACIE_ConfigureGigeScienceInterface(handle->card.handle, handle->card.slctMACIEs, 0, 0, 42037, &socket_size)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("ConfigureGigeScienceInterface failed.");
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }

    HxRGlog.writetoVerbose("Expected framesize: "+std::to_string(f_size));
    HxRGlog.writetoVerbose("Theoritical integration time: "+std::to_string(t_intTime)+" s.");
    HxRGlog.writetoVerbose("Socket size: "+std::to_string(socket_size)+" KB.");

    //trigger an exposure
    if (get_asic_6900(handle)!=ASIC_IDLE)
    {
        HxRGlog.writetoVerbose("Asic h6900 is not in idle when we expect it to be.");
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }

    if (set_asic_6900(handle,ASIC_ACQ)!=0)
    {
        HxRGlog.writetoVerbose("Unable to trigger an exposure.");
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }

    HxRGlog.writetoVerbose("Trigger succeeded.");
    HxRGlog.writetoVerbose("Waiting for Science Data");
    delay(1000);

    //dont forget to use void set_unique_id(instHandle *handle)

    if (wait_for_data(handle)!=0)
    {
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }
    if (check_halt(handle)){goto endacq;}

    for (ramp=1; ramp<=static_cast<int>(handle->pAcq.ramp);ramp++){
        //::::::::::We need to update a few variable:::::::
        id = set_unique_id(handle);//set unique id
        pHead.edit_entry("SEQID",std::to_string(id));
        pHead.edit_entry("RAMP",ramp);
        handle->head.RAMP = ramp;
        handle->head.ID = id;
        date_obs = "";
        mjd_obs = 0;
        //::::::::::::::::::::::::::::::::::::::::::::::::::
        for (read=1;read<=static_cast<int>(handle->pAcq.read);read++) {
            handle->head.READ = read;//update the read number in header
            pHead.edit_entry("READ",read);
            //we want VLT to know which expometer value or if there is an expometer value.
            //check if halt is triggered
            if (check_halt(handle)){goto endacq;}

            print_head(handle);//print some log lines
            nbWords = 0;
            nbWords = MACIE_ReadGigeScienceData(handle->card.handle, static_cast<unsigned short>(t_intTime*1000)+static_cast<unsigned short>(moreDelay*1000) ,f_size,pData);
            if (read==1)
            {
                //not used for now
                mjd_obs = mjd_time_now()-0.000064506;//we need to subtract 5.5733 second because we'ra at the end of the first read give or take.
                date_obs = mjd2str(mjd_obs);
            }
            if (static_cast<int>(f_size)!=nbWords)
            {
                HxRGlog.writetoVerbose("Size of data read does not match the size of the array");
                HxRGlog.writetoVerbose("Words received: "+std::to_string(nbWords));
                HxRGlog.writetoVerbose("frame size: "+std::to_string(f_size));
            }
            if (!pData){HxRGlog.writetoVerbose("Null frame err: ");
                handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
                goto endacq;}
            HxRGlog.writetoVerbose("Image read successfully.");

            //create time stamp and make header

            handle->head.timeLocal = ts_now_local();
            handle->head.timeGMT = ts_now_gmt();
            handle->head.JD = ts_now_jd();
            //create time stamp and make header
            pHead.edit_entry("LTIME",ts_now_local());
            pHead.edit_entry("GMTTIME",ts_now_gmt());
            pHead.edit_entry("JD",ts_now_jd());


            if (MACIE_WriteFitsFile((char *)get_fname(handle).c_str(),static_cast<unsigned short>(handle->pDet.frameX),static_cast<unsigned short>(handle->pDet.frameY),pData,pHead.get_header_length(),pHead.get_header() )!=MACIE_OK)
            {
                HxRGlog.writetoVerbose("In acquisition loop: Write fits file failed: "+std::string(MACIE_Error()));
                handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
                goto endacq;
            }
            HxRGlog.writetoVerbose("Fits written successfully.");

        }//read

        m_intTime = integration_time(handle);
        HxRGlog.writetoVerbose("Average ramp integration time: "+std::to_string(m_intTime)+" s.");
        HxRGlog.writetoVerbose("Ramp finished");
        handle->nAcq.status = hxrgEXP_COMPL_SUCCESS;//important to trigger temporary image with VLT engineering panel
    }//ramp

    handle->nAcq.status = hxrgEXP_COMPL_SUCCESS;

    endacq:
        if (wait_for_end_of_acq(handle)!=0)
        {
            HxRGlog.writetoVerbose("Wait end of exposure timeout.");
        }
        get_error(handle);
        close_macie_comm(handle);
        handle->card.haltTriggered = 0;
        handle->pAcq.isOngoing=0;
        handle->imSize = 0;
        handle->im_ptr = nullptr;
        delete [] pData;

        return ;

}
void acquisition_sim(instHandle *handle)
/*
 * Description: trigger an acquisition.
 * savedata: [true/false]
 */
{
    //set a few status
    handle->nAcq.status = hxrgEXP_INTEGRATING;
    handle->card.haltTriggered = 0;
    handle->pAcq.isOngoing=1;//make sure no other command are sent
    //variables
    double mjd_obs=0;
    std::string date_obs="";
    int socket_size = 0;
    double m_intTime = 0;
    long id=0;
    int nbWords =0;
    uint16_t moreDelay = 32/handle->pDet.nbOutput *6000;//small delay in ms
    double t_intTime = static_cast<double>(handle->pDet.frameX)*static_cast<double>(handle->pDet.frameY)*static_cast<double>(handle->pDet.readoutTime)/handle->pDet.nbOutput;
    MACIE_FitsHdr pHeaders[21];

    //double triggerTimeout = (t_intTime * (handle->pAcq.reset) + static_cast<double>(moreDelay)/1000.0);//trigger timeout
    long f_size = frameSize(handle);
    //char file[1024];
    int ramp=1;//always at least 1 ramp
    int read=1;//always at least 1 read

    unsigned short *pData= new unsigned short[f_size];

    if (handle->nAcq.fitsMTD){create_TS_folder(handle);}//create folder if we want to save the data
    update_TS_header(handle);//put the date timestamp in the header

    //::::::::::::::::::: set parameter and reconfigure :::::::::::::::::
    delay(500);

    //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    HxRGlog.writetoVerbose("Configuring science interface.");
    HxRGlog.writetoVerbose("Expected framesize: "+std::to_string(f_size));
    HxRGlog.writetoVerbose("Theoritical integration time: "+std::to_string(t_intTime)+" s.");
    HxRGlog.writetoVerbose("Socket size: "+std::to_string(socket_size)+" KB.");

    //trigger an exposure
    delay(500);

    HxRGlog.writetoVerbose("Trigger succeeded.");
    HxRGlog.writetoVerbose("Waiting for Science Data");
    delay(1000);

    //dont forget to use void set_unique_id(instHandle *handle)
    handle->head.timeLocal = ts_now_local();
    handle->head.timeGMT = ts_now_gmt();
    handle->head.JD = ts_now_jd();
    makeHeader(pHeaders, handle);
    pHeader pHead(pHeaders,21);

    //wait for data
    delay(1000);
    if (check_halt(handle)){goto endacq;}
    //we create the header


    for (ramp=1; ramp<=static_cast<int>(handle->pAcq.ramp);ramp++){
        //::::::::::We need to update a few variable:::::::
        id = set_unique_id(handle);//set unique id
        handle->head.RAMP = ramp;
        pHead.edit_entry("RAMP",ramp);
        handle->head.ID = id;
        pHead.edit_entry("SEQID",std::to_string(id));
//pHead["SEQID"]->sVal = id;
        //::::::::::::::::::::::::::::::::::::::::::::::::::
        for (read=1;read<=static_cast<int>(handle->pAcq.read);read++) {
            handle->head.READ = read;//update the read number in header
            pHead.edit_entry("READ",read);
            //check if halt is triggered
            if (check_halt(handle)){goto endacq;}
            print_head(handle);//print some log lines
            nbWords = f_size;

            //create fake data
            for (size_t i=0;i<f_size;i++) {
                pData[i] = static_cast<uint16_t>(static_cast<uint16_t>(i)+read*100);
            }
            delay(5573);

            if (read==1)
            {
                mjd_obs = mjd_time_now()-0.000064506;//we need to subtract 5.5733 second because we'ra at the end of the first read give or take.
                date_obs = mjd2str(mjd_obs);
            }
            if (static_cast<int>(f_size)!=nbWords)
            {
                HxRGlog.writetoVerbose("Size of data read does not match the size of the array");
                HxRGlog.writetoVerbose("Words received: "+std::to_string(nbWords));
                HxRGlog.writetoVerbose("frame size: "+std::to_string(f_size));
            }
            if (!pData){HxRGlog.writetoVerbose("Null frame err: ");
                handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
                goto endacq;}
            HxRGlog.writetoVerbose("Image read successfully.");

            //create time stamp and make header
            pHead.edit_entry("LTIME",ts_now_local());
            pHead.edit_entry("GMTTIME",ts_now_gmt());
            pHead.edit_entry("JD",ts_now_jd());


            //add whatever roll your boat
           // pHead.add_entry("MONTEST",128,"ceci est mon test");

            //we can modify header like this
            //pHead["XSTOP"]->iVal=129;

            if (MACIE_WriteFitsFile((char *)get_fname(handle).c_str(),static_cast<unsigned short>(handle->pDet.frameX),static_cast<unsigned short>(handle->pDet.frameY),pData,pHead.get_header_length(),pHead.get_header() )!=MACIE_OK)
            {
                HxRGlog.writetoVerbose("In acquisition loop: Write fits file failed: "+std::string(MACIE_Error()));
                handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
                goto endacq;
            }
            HxRGlog.writetoVerbose("Fits written successfully.");

        }//read
        //head.add_entry("ARCFILE",handle->nAcq.name,"Archive File Name");//TODO: test this modification

        HxRGlog.writetoVerbose("Average ramp integration time: "+std::to_string(m_intTime)+" s.");
        HxRGlog.writetoVerbose("Ramp finished");
    }//ramp

    handle->nAcq.status = hxrgEXP_COMPL_SUCCESS;

    endacq:
        handle->card.haltTriggered = 0;
        handle->pAcq.isOngoing=0;
        handle->imSize = 0;
        handle->im_ptr = nullptr;
        delete [] pData;
        return ;

}
void close_macie_comm(instHandle *h)
{
    if (MACIE_CloseGigeScienceInterface(h->card.handle, h->card.slctMACIEs)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("MACIE: "+std::string(MACIE_Error()));

    }
    return;
}
void copy_arr(unsigned short **pData,double **dest,long size)
{
    for (long i=0;i<size;i++)
    {
        (*dest)[i] = (*pData)[i];
    }
    return ;
}
void copy_arr(unsigned short **pData,uint16_t **dest,long size)
{
    for (long i=0;i<size;i++)
    {
        (*dest)[i] = (*pData)[i];
    }
    return ;
}
std::string get_fname(instHandle *handle)
{
    char fname[2048];
    memset(fname,0,2048);
    sprintf(fname,"H%dRG_R%2.2d_R%2.2d.fits",static_cast<int>(handle->pDet.HxRG),handle->head.RAMP,handle->head.READ);//handle->pAcq.savePath;

    return join(handle->path.serie_path,fname);
}
void print_head(instHandle *h)
{
    HxRGlog.writetoVerbose("::::::::::::::::::::::");
    HxRGlog.writetoVerbose("Ramp: "+std::to_string(h->head.RAMP)+", Read: "+std::to_string(h->head.READ));
    HxRGlog.writetoVerbose("::::::::::::::::::::::");
}
void set_ESO_header_ramp(instHandle *handle,header *head,long id)
{
    head->add_entry("ESO DET FRAM TYPE",handle->head.type,"Frame type");
    head->add_entry("ESO DET CHIP1 DATE","2020-07-29","[YYYY-MM-DD] Date of installation");
    head->add_entry("ESO DET CHIP1 ID","D18859-A157","Detector chip identification");
    head->add_entry("ESO DET CHIPS",1,"Number of chips in the mosaic");
    head->add_entry("ESO DET DID","ESO-VLT-DIC.NGCDCS","Dictionary");
    head->add_entry("ESO DET NAME","NIRPS","Name of detector system");
    head->add_entry("ESO DET NDIT",1,"Number of Sub-Integrations");
    head->add_entry("ESO DET OUT1 GAIN",1.06,"[ADU/e-] Conversion electrons to ADU");
    head->add_entry("ESO DET OUT1 RON",15.0,"[e-] Readout noise per output");
    head->add_entry("ESO DET OUTPUTS",static_cast<int>(handle->pDet.nbOutput),"Number of outputs");
    if (handle->pAcq.simulator==1)
    {
        head->add_entry("ESO DET SOFW MODE","HW-SIM","Software global operational");
    }
    else {
        head->add_entry("ESO DET SOFW MODE","NORMAL","Software global operational");
    }
    head->add_entry("ESO DET WIN1 BINX",1,"Binning factor along X");
    head->add_entry("ESO DET WIN1 BINY",1,"Binning factor along Y");
    head->add_entry("ESO DET WIN1 NX",handle->pDet.frameX,"# of pixels along X");
    head->add_entry("ESO DET WIN1 NY",handle->pDet.frameY,"# of pixels along Y");
    head->add_entry("ESO DET WIN1 STARTX",1,"Lower left pixel in X");
    head->add_entry("ESO DET WIN1 STARTY",1,"Lower left pixel in Y");
    head->add_entry("UNIQUEID",std::to_string(id),"Unique id number");
}
long frameSize(instHandle *handle)
/*
 * Description: return the frame size given detetcor parameters
 */
{
    if (handle->pAcq.isWindow==1)
    {
        return (static_cast<long>(handle->pAcq.winStopX)-static_cast<long>(handle->pAcq.winStartX)+1)*(static_cast<long>(handle->pAcq.winStopY)-static_cast<long>(handle->pAcq.winStartY)+1);
    }
    return static_cast<long>(handle->pDet.frameX)*static_cast<long>(handle->pDet.frameY);
}
long set_unique_id(instHandle *handle)
/*
 * Set an log the unique ID in the header of each ramp.
 */
{
    long id = unique_id();
    HxRGlog.writetoVerbose("Unique ID: "+std::to_string(id));
    handle->head.ID = id;
    return id;
}
long unique_id()
/*
 * Return a unique long number. Basically the date+time
 */
{
    time_t t = time(nullptr);
    struct tm * now = localtime(&t);
    long id = now->tm_sec+(now->tm_min*100)+(now->tm_hour*10000)+(now->tm_mday*1000000)+(now->tm_mon+1)*100000000+(now->tm_year+1900)*10000000000;

    return id;
}
void update_TS_header(instHandle *handle)
/*
 * Description: Create a header ts with the current date ACQDATE
 */
{
    char Time[12];
    time_t t = time(nullptr);
    struct tm * now = localtime(&t);
    sprintf(Time,"%d-%2.2d-%2.2d",now->tm_year+1900,now->tm_mon+1,now->tm_mday);
    handle->head.ACQDATE=Time;
    return ;
}
void create_TS_folder(instHandle *handle)
/*
 * Description: Creates the folder of the data in format YYYYMMDDHHMMSS, and
 * update the handle->pAcq.savePath element with the new save path.
 */
{
    char folder[16],path[1024];
    makeFolderName(folder);
    createFolder(folder,path,handle->pAcq.savePath.c_str());
    handle->path.serie_path = std::string(path);
}




   
void initAll(instHandle *handle,cmd *cc)
/*!
 * \brief Initialize ASIC & MACIE
 * Used as a standardized function
 */
{
    if (handle->pAcq.isOngoing!=0)
    {
        sndMsg(cc->sockfd,"Acquisition ongoing",hxrgCMD_ERR_NOT_SUPPORTED);
        return ;
    }

    if (handle->pAcq.simulator==0){
        if (handle->card.asBeenInit>0)
        {   HxRGlog.writetoVerbose("Re-initialize MACIE.");
            if (MACIE_Free()!=MACIE_OK)
            {
                sndMsg(cc->sockfd,"Cannot close the communication device",hxrgCMD_ERR_VALUE);
                return;
            }

        }


        if (start(handle)!=0){
            sndMsg(cc->sockfd,"Unable to start MACIE communication",hxrgCMD_ERR_VALUE);
            return ;
        }
        if (getHandle(handle)!=0)
        {
            sndMsg(cc->sockfd,"Unable to get handle [MACIE/ASIC]",hxrgCMD_ERR_VALUE);
            return ;
        }
        if (getAvailableMacie(handle)!=0)
        {
            sndMsg(cc->sockfd,"No available MACIE",hxrgCMD_ERR_VALUE);
            return ;
        }
        if (initASIC(handle)!=0)
        {
            sndMsg(cc->sockfd,"Unable to init ASIC",hxrgCMD_ERR_VALUE);
            return ;
        }
    }
    //set for nirps
    HxRG_init_config::conf_data CFP;
    if (handle->user.compare("")!=0)
    {
        find_config_file(handle,&CFP,handle->user);
        set_config_file(&CFP,handle);
    }


    if (handle->pAcq.simulator==0){
        if (updateHandle(handle)!=0)
        {
            sndMsg(cc->sockfd,"Unable to read some register. Use at your own risk or re-initialize the software.",hxrgCMD_ERR_VALUE);
            return ;

        }
    }

    if (handle->pAcq.simulator==1)
    {
        sndMsg(cc->sockfd);
        return;
    }

        sndMsg(cc->sockfd);

    //everything is OK
    handle->card.asBeenInit=2;



return ;
}
void setup_t(instHandle *handle)
{
    int fd = create_socket(5015);
    cmd *c = new cmd;
    while (1) {

        c->recvCMD(fd);
        SETUP(handle,c);

    }
    return;
}
void SETUP(instHandle *handle,cmd *cc)
/*!
  * Arguments can be;
  *     DET.NDIT:           usually will be 1, but could change,
  *     DET.NDSAMPLES:      Number of reads
  *     DET.FRAM.TYPE:      DARK,BIAS,NORMAL
  *     DET.FRAM.NUMBLOCK:  Number of predefined header bloc
  *     DET.FRAM.NAMING:    Only NAMING is supported by HxRG-SERVER
  *     DET.FRAM.FITSMTD:   Compressed or uncompressed file saving. Only FITSMTD is supported by HxRG-SERVER
  *     DET.FRAM.FILENAME:  Name of the ramp file.
  *     DET.EXPO.MASK1:     Name of posemeter mask1. Mask should be stored in /opt/HxRG-SERVER/cal
  *     DET.EXPO.MASK2:     Name of posemeter mask2. Mask should be stored in /opt/HxRG-SERVER/cal
  */
{
    int error=0;
    std::string msg="";
    std::string arg="";
    arg = (*cc)["DET.NDIT"];
    if (arg.compare("")!=0)
    {
        for (auto &c:arg)
        {
            if (!isdigit(c))
            {
                error+=1;
                break;
            }
        }
        if (error==0)
        {
            handle->pAcq.ramp = static_cast<uint>(std::atoi(arg.c_str()));
        }
    }
    arg = (*cc)["DET.NDSAMPLES"];
    if (arg.compare("")!=0)
    {
        if (std::atoi(arg.c_str())<=0)
        {
            error-=1;
            msg+="NDSAMPLES cannot be < 0, ";
        }
        else {
            handle->pAcq.read = static_cast<uint>(std::atoi(arg.c_str()));
        }
    }
    arg = (*cc)["DET.FRAM.TYPE"];
    if (arg.compare("")!=0)
    {
        if (arg.compare("Normal")==0)
        {
            handle->head.type = "NORMAL";
        }
        else if (arg.compare("Bias")==0){
            handle->head.type = "BIAS";
        }
        else if (arg.compare("Dark")==0){
            handle->head.type = "DARK";
        }
        else {
            msg+="TYPE can only be DARK,BIAS or NORMAL, ";
            error-=1;
        }
    }
    arg = (*cc)["DET.FRAM.NUMBLOCK"];
    if (arg.compare("")!=0)
    {
        if (std::atoi(arg.c_str())>0){
            //flag blocksize 1 bloc == 2880 octets == 36 keywords
            handle->head.numBlock = std::atoi(arg.c_str());
        }
        else {
            msg+="NUMBLOCK must be > 0, ";
            error-=1;
        }

    }
    arg = (*cc)["DET.FRAM.NAMING"];
    if (arg.compare("")!=0)
    {
        if (arg.compare("Request")!=0)
        {
            msg+="Only NAMING=Request is supported, ";
            error-=1;
        }
    }
    arg = (*cc)["DET.FRAM.FITSMTD"];
    if (arg.compare("")!=0)
    {
        switch (std::atoi(arg.c_str())) {
        case hxrgFITSMTD_NONE :
        {
            handle->nAcq.fitsMTD = false;
            break;
        }
        case hxrgFITSMTD_COMPRESSED:
        {
            msg+="Only FITSMTD option uncompressed or NONE is supported, ";
            error-=1;
            break;
        }
        case hxrgFITSMTD_UNCOMPRESSED:
        {
            handle->nAcq.fitsMTD = true;
            break;
        }
        case hxrgFITSMTD_BOTH:
        {
            msg+="Only FITSMTD option uncompressed or NONE is supported, ";
            error-=1;
            break;
        }
        }

    }
    arg = (*cc)["DET.FRAM.FILENAME"];
    if (arg.compare("")!=0)
    {
        handle->nAcq.name = checkExt(arg);
    }
    //Posemeter mask 1
    arg = (*cc)["DET.EXPO.MASK1"];
    if (arg.compare("")!=0)
    {
        handle->pAcq.posemeter_mask = checkExt(arg);
    }
    // Posemeter mask 2
    arg = (*cc)["DET.EXPO.MASK2"];
    if (arg.compare("")!=0)
    {

        handle->pAcq.posemeter_mask_optional = checkExt(arg);
    }
    if (error<0)
    {
        sndMsg(cc->sockfd,msg,hxrgCMD_ERR_PARAM_VALUE);
    }
    else {
        sndMsg(cc->sockfd);
    }

}
std::string join(std::string path,std::string filename)
/*!
  * Join a path and filename.
  */
{
    if (path.compare("")==0)
    {
        return filename;
    }
    if (filename.compare("")==0)
    {
        return path;
    }
    if (path[path.size()-1]=='/')
    {
        return path+filename;
    }
    else {
        return path+'/'+filename;
    }
}
std::string checkExt(std::string fname)
{
    if (fname.size()>=4)
    {
        std::string ext = fname.substr(fname.size()-4,4);
        if (ext.compare("fits")!=0)
        {
            return fname+".fits";
        }
        else {
            return fname;
        }
    }
    else {
        return fname+".fits";
    }
}

void c_idle(instHandle *handle, cmd *cc)
/*!
  * Custom idle mode. Very similar to the acquisition function,
  * but without saving files on disk.
  */
{

    while (1) {
        forced_idle(handle);
        if (handle->card.haltTriggered==1)
        {
            break;
        }
    }
    handle->card.haltTriggered=0;
    return;

}
uint read_val(instHandle *handle, uint addr)
{
    uint val=0;
    if (MACIE_ReadASICReg(handle->card.handle, handle->card.slctASIC, addr, &val, false, true) != MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to read register "+std::to_string(addr));
    }

    return val;
}
void check_register(instHandle *handle,std::string tag)
{
//check a few register to investigate problems
    HxRGlog.writetoVerbose(tag);
    for (uint reg : {0x6900,0x4002,0x4000,0x4001,0x4003,0x4004,0x4005}) {
        HxRGlog.writetoVerbose("h"+std::to_string(reg)+": "+std::to_string(read_val(handle,reg)));
        delay(100);
    }

}
int wait_for_data(instHandle *handle)
{
    unsigned long n=0;
    unsigned short moreDelay=0;//ok
    long frameSize = static_cast<long>(handle->pDet.frameX)*static_cast<long>(handle->pDet.frameY);
    float frametime = static_cast<float>(frameSize)/(static_cast<float>(handle->pDet.nbOutput)) * handle->pDet.readoutTime;
    moreDelay = 32/handle->pDet.nbOutput *10;
    float triggerTimeout = (frametime * (handle->pAcq.reset) + moreDelay);

    for (int i = 1; i <= 100; i++)
    {
        n = MACIE_AvailableScienceData(handle->card.handle);
        if (n >0)
        {
            std::cout << "Available science data = " << n << " bytes, Loop = " << i << std::endl;
            HxRGlog.writetoVerbose("Available science data = "+std::to_string(n)+ " bytes, Loop = "+std::to_string(i));
            break;
        }

        delay(static_cast<int>(triggerTimeout*static_cast<float>(10.0)));

    }

    if (n <= 0)
    {
        HxRGlog.writetoVerbose("In acquisition Loop: trigger timeout: no available science data");
        return -1;
    }
    return 0;

}
void get_error(instHandle *handle)
{
    std::string error_code[33]={
        "UART Port: Parity Errors",
        "UART Port: Stopbit Errors",
        "UART Port: Timeout Errors",
        "USB Port: Timeout Errors",
        "GigE Port: Timeout Errors",
        "ASIC 1 Configuration: Acknowledge Errors Type 1",
        "ASIC 1 Configuration: Acknowledge Errors Type 2",
        "ASIC 1 Configuration: Start Bit Timeout Errors",
        "ASIC 1 Configuration: Stop Bit Timeout Errors",
        "ASIC 1 Configuration: Stop Bit Errors",
        "ASIC 1 Configuration: Parity Errors",
        "ASIC 1 Configuration: Data Errors",
        "ASIC 1 Configuration: CRC errors",
        "ASIC 2 Configuration: Acknowledge Errors Type 1",
        "ASIC 2 Configuration: Acknowledge Errors Type 2",
        "ASIC 2 Configuration: Start Bit Timeout Errors",
        "ASIC 2 Configuration: Stop Bit Timeout Errors",
        "ASIC 2 Configuration: Stop Bit Errors",
        "ASIC 2 Configuration: Parity Errors",
        "ASIC 2 Configuration: Data Errors",
        "ASIC 2 Configuration: CRC errors",
        "Main Science FIFO: Science FIFO Overflow Errors",
        "Main Science FIFO: spare",
        "Main Science FIFO: spare",
        "Main Science FIFO: spare",
        "ASIC 1 Science Data: Stop Errors",
        "ASIC 1 Science Data: Parity Errors",
        "ASIC 1 Science Data: Data Errors",
        "ASIC 1 Science Data: CRC Errors",
        "ASIC 2 Science Data: Stop Errors",
        "ASIC 2 Science Data: Parity Errors",
        "ASIC 2 Science Data: Data Errors",
        "ASIC 2 Science Data: CRC Errors"};
    unsigned short counterArray[MACIE_ERROR_COUNTERS]={1};
    if (MACIE_GetErrorCounters(handle->card.handle, handle->card.slctMACIEs,counterArray)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("unable to read error counter");
        return;
    }
    std::cout<<":::::::::: Error counter :::::::::::::::::::"<<std::endl;
    int error=0;
    for (int i=0;i<MACIE_ERROR_COUNTERS;i++) {
        if (counterArray[i]!=0)
        {
            error+=1;
            HxRGlog.writetoVerbose("\t"+error_code[i]);
        }

    }

        HxRGlog.writetoVerbose(std::to_string(error)+" error found");

}

double integration_time(instHandle *handle)
{
    uint a=0,b=0;
    if (MACIE_ReadASICReg(handle->card.handle, handle->card.slctASIC, 0x400d, &a, false, true)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to read h400d.");
    }
    else {
        HxRGlog.writetoVerbose("h400d: "+std::to_string(a));
    }
    if (MACIE_ReadASICReg(handle->card.handle, handle->card.slctASIC, 0x400c, &b, false, true)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to read h400c.");
    }
    else {
        HxRGlog.writetoVerbose("h400c: "+std::to_string(b));

    }
    double intTime = (static_cast<double>(a)*(65536.0)+static_cast<double>(b))*2.0*0.00001;
    HxRGlog.writetoVerbose("Effective exposure time: "+std::to_string(intTime)+" s");
    return intTime;
}

void write_txt(std::string filename,std::string txt)
{
    std::ofstream ofs;
    std::time_t t = std::time(0);   // get time now
    std::tm* now = std::localtime(&t);
    char tt[20]={0};
    sprintf(tt,"%2.2d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d",
            now->tm_year + 1900,
            now->tm_mon,
            now->tm_mday,
            now->tm_hour,
            now->tm_min,
            now->tm_sec);

    ofs.open (filename, std::ofstream::out | std::ofstream::app);
    ofs <<std::string(tt)+'\t'+txt<<'\n';
    std::cout<<std::string(tt)+'\t'+txt<<'\n'<<std::endl;
    ofs.flush();
    ofs.close();
    return;
}



int wait_for_end_of_acq(instHandle *handle)
/*
 * Description: Wait for the h6900 to return to idle.
 * return -1 if it does not return.
 */
{
    int to=0;
    for (int i=0;i<100;i++) {
        delay(200);
        to+=200;
        if (ASIC_IDLE == static_cast<uint>(get_asic_6900(handle)))
        {
            HxRGlog.writetoVerbose("End of exposure in "+std::to_string(to)+" ms");
            return 0;
        }
    }
    HxRGlog.writetoVerbose("End of exposure never came. Timeout at "+std::to_string(to)+" ms.");
    return -1;
}
int get_asic_6900(instHandle *handle)
/*
 * Description: Get the value of ASIC h6900.
 */
{
    uint idle_status=0;
    delay(200);
    if (MACIE_ReadASICReg(handle->card.handle, handle->card.slctASIC, 0x6900, &idle_status, false, true)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to read register 0x6900.");
        return ASIC_FAILED;
    }

    if ((idle_status & 0x8000)==idle_status)
    {
        return ASIC_IDLE;
    }
    else if ((idle_status & 0x8001)==idle_status)
    {
        return ASIC_ACQ;
    }
    if ((idle_status & 0x8002)==idle_status)
    {
        return ASIC_RECONF;
    }

    else {
        return ASIC_FAILED;
    }
}
int set_asic_6900(instHandle *handle,uint state)
{

    int macie_value=0;
    switch (state) {
        case ASIC_IDLE:
        {
            macie_value = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x6900, 0x8000, true);
            break;
        }
        case ASIC_ACQ:
        {
            macie_value = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x6900, 0x8001, true);
            break;
        }
        case ASIC_RECONF:
        {
            macie_value = MACIE_WriteASICReg(handle->card.handle, handle->card.slctASIC, 0x6900, 0x8002, true);
            break;
        }
        default:
        {
            HxRGlog.writetoVerbose("wrong value for ASIC h6900");
            break;
        }
    }
    if (macie_value!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("Unable to write register 0x6900");
        return ASIC_FAILED;
    }
    int to = 0;
    for (int i=0;i<80;i++) {
        delay(200);
        to+=200;
        if (static_cast<uint>(get_asic_6900(handle)) ==state)
        {
            HxRGlog.writetoVerbose("h6900 set in "+std::to_string(to)+" ms.");
            return 0;
        }
    }
    HxRGlog.writetoVerbose("Unable to set h6900. Timeout is more than "+std::to_string(to)+" ms.");
    return ASIC_FAILED;
}

void forced_idle(instHandle *handle)
/*
 * Description: trigger an acquisition.
 * savedata: [true/false]
 */
{
    //set a few status
    HxRGlog.writetoVerbose(  "::::::::::::::::::::::::::::::::");
    HxRGlog.writetoVerbose(  ":::           IDLE           :::");
    HxRGlog.writetoVerbose(  "::::::::::::::::::::::::::::::::");
    handle->card.haltTriggered = 0;
    handle->pAcq.isOngoing=2;//make sure no other command are sent
    int ramp2do = 65530;
    int read2do = 1;
    //variables
    int socket_size = 0;
    double m_intTime = 0;
    int nbWords =0;
    uint16_t moreDelay = 32/handle->pDet.nbOutput *6000;//small delay in ms
    double t_intTime = static_cast<double>(handle->pDet.frameX)*static_cast<double>(handle->pDet.frameY)*static_cast<double>(handle->pDet.readoutTime)/handle->pDet.nbOutput;
   // MACIE_FitsHdr pHeaders[19];
    long f_size = frameSize(handle);
    int ramp=1;
    int read=1;
    //class
    unsigned short *pData= new unsigned short[f_size];

    //::::::::::::::::::: set parameter and reconfigure :::::::::::::::::
    delay(500);
    if (handle->pAcq.simulator==0 && handle->card.asBeenInit>0){
    if (setRegister(handle,static_cast<uint>(ramp2do),static_cast<uint>(read2do))!=0){
        goto endacq;
    }}

    //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    HxRGlog.writetoVerbose("Configuring science interface.");
    if (handle->pAcq.simulator==0 && handle->card.asBeenInit>0){
    if (MACIE_ConfigureGigeScienceInterface(handle->card.handle, handle->card.slctMACIEs, 0, 0, 42037, &socket_size)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("ConfigureGigeScienceInterface failed.");
        goto endacq;
    }}

    HxRGlog.writetoVerbose("Expected framesize: "+std::to_string(f_size));
    HxRGlog.writetoVerbose("Theoritical integration time: "+std::to_string(t_intTime)+" s.");
    HxRGlog.writetoVerbose("Socket size: "+std::to_string(socket_size)+" KB.");

    //trigger an exposure
    if (handle->pAcq.simulator==0 && handle->card.asBeenInit>0){
    if (get_asic_6900(handle)!=ASIC_IDLE)
    {
        HxRGlog.writetoVerbose("Asic h6900 is not in idle when we expect it to be.");
        goto endacq;
    }

    if (set_asic_6900(handle,ASIC_ACQ)!=0)
    {
        HxRGlog.writetoVerbose("Unable to trigger an exposure.");
        goto endacq;
    }}

    HxRGlog.writetoVerbose("Trigger succeeded.");
    HxRGlog.writetoVerbose("Waiting for Science Data");
    delay(1000);

    //dont forget to use void set_unique_id(instHandle *handle)
    if (handle->pAcq.simulator==0 && handle->card.asBeenInit>0){
    if (wait_for_data(handle)!=0){goto endacq;}
    }
    if (check_halt(handle,true)){goto endacq;}


    for (ramp=1; ramp<=ramp2do;ramp++){
        for (read=1;read<=read2do;read++) {

            //check if halt is triggered
            if (check_halt(handle,true)){goto endacq;}

            if (handle->pAcq.simulator==0 && handle->card.asBeenInit>0){
            nbWords = 0;
            nbWords = MACIE_ReadGigeScienceData(handle->card.handle, static_cast<unsigned short>(t_intTime*1000)+static_cast<unsigned short>(moreDelay*1000) ,f_size,pData);
            if (static_cast<int>(f_size)!=nbWords)
            {
                HxRGlog.writetoVerbose("Size of data read does not match the size of the array");
                HxRGlog.writetoVerbose("Words received: "+std::to_string(nbWords));
                HxRGlog.writetoVerbose("frame size: "+std::to_string(f_size));
            }}
            else {

            delay(5573);
            for (size_t i=0;i<f_size;i++) {
                pData[i] = static_cast<uint16_t>(i);
            }}
            if (!pData){
                HxRGlog.writetoVerbose("Null frame err: ");
                goto endacq;
            }
            HxRGlog.writetoVerbose("Idle image read successfully.");

        }//read
        if (handle->pAcq.simulator==0 && handle->card.asBeenInit>0){
        m_intTime = integration_time(handle);

        HxRGlog.writetoVerbose("Average ramp integration time: "+std::to_string(m_intTime)+" s.");
        }
    }//ramp


    endacq:
    if (handle->pAcq.simulator==0 && handle->card.asBeenInit>0){
        if (wait_for_end_of_acq(handle)!=0)
        {
            HxRGlog.writetoVerbose("Wait end of exposure timeout.");
        }
        get_error(handle);
        close_macie_comm(handle);
    }
        //handle->card.haltTriggered = 0; To tell the idle loop to escape
        handle->pAcq.isOngoing=0;
        handle->imSize = 0;
        handle->im_ptr = nullptr;
        delete [] pData;

        return ;

}

void acquisition_eng(instHandle *handle)
/*
 * Description: trigger an angineering acquisition.
 * This will include the HxRG telemetry in the header
 * as well as the complete MACIE card telemetry.
 *
 * TODO -> clear all the fits2ramp and posemeter references
 */
{
    //set a few status
    handle->nAcq.status = hxrgEXP_INTEGRATING;
    handle->card.haltTriggered = 0;
    handle->pAcq.isOngoing=1;//make sure no other command are sent
    int ramp2do = 65530;
    //variables
    std::vector<double> v;
    std::vector<double> c;
    double mjd_obs = 0;
    std::string date_obs="";
    int socket_size = 0;
    double m_intTime = 0;
    long id=0;
    int nbWords =0;
    uint16_t moreDelay = 32/handle->pDet.nbOutput *6000;//small delay in ms
    double t_intTime = static_cast<double>(handle->pDet.frameX)*static_cast<double>(handle->pDet.frameY)*static_cast<double>(handle->pDet.readoutTime)/handle->pDet.nbOutput;

    //float pTlmVals[79]{0};  //to store the MACIE telemetry
    double triggerTimeout = (t_intTime * (handle->pAcq.reset) + static_cast<double>(moreDelay)/1000.0);//trigger timeout
    long f_size = frameSize(handle);
    char file[1024];
    int ramp=1;
    int read=1;
    std::string nl_uid="",bias_uid="";
    //class
    unsigned short *pData= new unsigned short[f_size];
    //double *f2rData = new double[f_size];

    HxRGlog.writetoVerbose("Updating masks.");
    if (handle->nAcq.fitsMTD){create_TS_folder(handle);}//create folder if we want to save the data
    update_TS_header(handle);//put the date timestamp in the header

    MACIE_FitsHdr pHeaders[21];//old style header
    makeHeader(pHeaders, handle);//populate old style header
    pHeader pHead(pHeaders,21);//declare new style header initialized with old header




    //::::::::::::::::::: set parameter and reconfigure :::::::::::::::::
    delay(500);



    if (setRegister(handle)!=0){
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }
    ramp2do = static_cast<int>(handle->pAcq.ramp);

    //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    HxRGlog.writetoVerbose("Configuring science interface.");
    if (MACIE_ConfigureGigeScienceInterface(handle->card.handle, handle->card.slctMACIEs, 0, 0, 42037, &socket_size)!=MACIE_OK)
    {
        HxRGlog.writetoVerbose("ConfigureGigeScienceInterface failed.");
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }

    HxRGlog.writetoVerbose("Expected framesize: "+std::to_string(f_size));
    HxRGlog.writetoVerbose("Theoritical integration time: "+std::to_string(t_intTime)+" s.");
    HxRGlog.writetoVerbose("Socket size: "+std::to_string(socket_size)+" KB.");

    //trigger an exposure
    if (get_asic_6900(handle)!=ASIC_IDLE)
    {
        HxRGlog.writetoVerbose("Asic h6900 is not in idle when we expect it to be.");
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }

    if (set_asic_6900(handle,ASIC_ACQ)!=0)
    {
        HxRGlog.writetoVerbose("Unable to trigger an exposure.");
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }

    HxRGlog.writetoVerbose("Trigger succeeded.");
    HxRGlog.writetoVerbose("Waiting for Science Data");
    delay(1000);

    //dont forget to use void set_unique_id(instHandle *handle)

    if (wait_for_data(handle)!=0)
    {
        handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
        goto endacq;
    }
    if (check_halt(handle)){goto endacq;}

    for (ramp=1; ramp<=ramp2do;ramp++){
        //::::::::::We need to update a few variable:::::::
        id = set_unique_id(handle);//set unique id
        pHead.edit_entry("SEQID",std::to_string(id));
        handle->nAcq.keepGoing-=1;
        pHead.edit_entry("RAMP",ramp);
        handle->head.RAMP = ramp;
        handle->head.ID = id;
        date_obs = "";
        mjd_obs = 0;
        //::::::::::::::::::::::::::::::::::::::::::::::::::
        for (read=1;read<=static_cast<int>(handle->pAcq.read);read++) {
            handle->head.READ = read;//update the read number in header
            pHead.edit_entry("READ",read);
            //we want VLT to know which expometer value or if there is an expometer value.


            //check if halt is triggered
            if (check_halt(handle)){goto endacq;}

            print_head(handle);//print some log lines
            nbWords = 0;
            nbWords = MACIE_ReadGigeScienceData(handle->card.handle, static_cast<unsigned short>(t_intTime*1000)+static_cast<unsigned short>(moreDelay*1000) ,f_size,pData);
            if (read==1)
            {
                //-0.000064506;
                mjd_obs = mjd_time_now()-0.000064506;//we need to subtract 5.5733 second because we'ra at the end of the first read give or take.
                date_obs = mjd2str(mjd_obs);
            }
            if (static_cast<int>(f_size)!=nbWords)
            {
                HxRGlog.writetoVerbose("Size of data read does not match the size of the array");
                HxRGlog.writetoVerbose("Words received: "+std::to_string(nbWords));
                HxRGlog.writetoVerbose("frame size: "+std::to_string(f_size));
            }
            if (!pData){HxRGlog.writetoVerbose("Null frame err: ");
                handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
                goto endacq;}
            HxRGlog.writetoVerbose("Image read successfully.");

            //create time stamp and make header
            handle->head.timeLocal = ts_now_local();
            handle->head.timeGMT = ts_now_gmt();
            handle->head.JD = ts_now_jd();
            //create time stamp and make header
            pHead.edit_entry("LTIME",ts_now_local());
            pHead.edit_entry("GMTTIME",ts_now_gmt());
            pHead.edit_entry("JD",ts_now_jd());


            //makeHeader_with_telemetry(pHeaders,handle,v,c,pTlmVals,regname,regdef);

            if (read>=2){
                handle->nAcq.expoMeterCount=read -1;
            }
            else {
                handle->nAcq.expoMeterCount=-1;
            }
            if (MACIE_WriteFitsFile((char *)get_fname(handle).c_str(),static_cast<unsigned short>(handle->pDet.frameX),static_cast<unsigned short>(handle->pDet.frameY),pData,pHead.get_header_length(),pHead.get_header() )!=MACIE_OK)
            {
                HxRGlog.writetoVerbose("In acquisition loop: Write fits file failed: "+std::string(MACIE_Error()));
                handle->nAcq.status = hxrgEXP_COMPL_FAILURE;
                goto endacq;
            }
            HxRGlog.writetoVerbose("Fits written successfully.");

        }//read
        //head.add_entry("ARCFILE",handle->nAcq.name,"Archive File Name");//TODO: test this modification

        m_intTime = integration_time(handle);
        HxRGlog.writetoVerbose("Average ramp integration time: "+std::to_string(m_intTime)+" s.");
        HxRGlog.writetoVerbose("Ramp finished");
        handle->nAcq.status = hxrgEXP_COMPL_SUCCESS;//important to trigger temporary image with VLT engineering panel
    }//ramp

    handle->nAcq.status = hxrgEXP_COMPL_SUCCESS;

    endacq:
        if (wait_for_end_of_acq(handle)!=0)
        {
            HxRGlog.writetoVerbose("Wait end of exposure timeout.");
        }
        get_error(handle);
        close_macie_comm(handle);
        handle->card.haltTriggered = 0;
        handle->pAcq.isOngoing=0;
        handle->imSize = 0;
        handle->im_ptr = nullptr;
        delete [] pData;
        return ;
}

//int makeHeader_with_telemetry(MACIE_FitsHdr *pHeaders,instHandle *handle,std::vector<double> v,std::vector<double> c,float *pTlmVals,std::vector<std::string> regname,std::vector<std::string> regdef)
//{
//    std::string GainCode[16]={"-3 dB small Cin","0 dB small Cin","3 dB small Cin","6 dB small Cin","6 dB large Cin","9 dB small Cin","9 dB large Cin","12 dB small Cin","12 dB large Cin","15 dB small Cin","15 dB large Cin","18 dB small Cin","18 dB large Cin","21 dB large Cin","24 dB large Cin","27 dB large Cin"};

//    strncpy(pHeaders[0].key, "ASICGAIN", sizeof(pHeaders[0].key));
//    pHeaders[0].iVal = static_cast<int>(handle->pDet.gain);
//    pHeaders[0].valType = HDR_INT;
//    strncpy(pHeaders[0].comment, GainCode[static_cast<int>(handle->pDet.gain)].c_str(), sizeof(pHeaders[0].comment));

//    strncpy(pHeaders[1].key, "AEEXPT", sizeof(pHeaders[1].key));
//    pHeaders[1].fVal = handle->pDet.effExpTime;
//    pHeaders[1].valType = HDR_FLOAT;
//    strncpy(pHeaders[1].comment, "Average effective exposure time", sizeof(pHeaders[1].comment));

//    strncpy(pHeaders[2].key, "AcqDate", sizeof(pHeaders[2].key));
//    strncpy(pHeaders[2].sVal, handle->head.ACQDATE.c_str(), sizeof(pHeaders[2].sVal));
//    pHeaders[2].valType = HDR_STR;
//    strncpy(pHeaders[2].comment, "Acquisition date", sizeof(pHeaders[2].comment));

//    strncpy(pHeaders[3].key, "ASIC_NUM", sizeof(pHeaders[3].key));
//    strncpy(pHeaders[3].sVal, handle->head.ASICSERIAL.c_str(), sizeof(pHeaders[3].sVal));
//    pHeaders[3].valType = HDR_STR;
//    strncpy(pHeaders[3].comment, "ASIC serial number", sizeof(pHeaders[3].comment));

//    strncpy(pHeaders[4].key, "SCA_ID", sizeof(pHeaders[4].key));
//    strncpy(pHeaders[4].sVal, handle->head.SCASERIAL.c_str(), sizeof(pHeaders[4].sVal));
//    pHeaders[4].valType = HDR_STR;
//    strncpy(pHeaders[4].comment, "SCA number", sizeof(pHeaders[4].comment));

//    strncpy(pHeaders[5].key, "MUXTYPE", sizeof(pHeaders[5].key));
//    pHeaders[5].iVal =  static_cast<int>(handle->pDet.HxRG);
//    pHeaders[5].valType = HDR_INT;
//    strncpy(pHeaders[5].comment, "1- H1RG; 2- H2RG; 4- H4RG", sizeof(pHeaders[5].comment));

//    strncpy(pHeaders[6].key, "NBOUTPUT", sizeof(pHeaders[6].key));
//    pHeaders[6].iVal =  static_cast<int>(handle->pDet.nbOutput);
//    pHeaders[6].valType = HDR_INT;
//    strncpy(pHeaders[6].comment, "Number of amplifier used", sizeof(pHeaders[6].comment));

//    strncpy(pHeaders[7].key, "HXRGVER", sizeof(pHeaders[7].key));
//    strncpy(pHeaders[7].sVal, handle->head.SOFTVERSION.c_str(), sizeof(pHeaders[7].sVal));
//    pHeaders[7].valType = HDR_STR;
//    strncpy(pHeaders[7].comment, "acquisition software version number", sizeof(pHeaders[7].comment));

//    strncpy(pHeaders[8].key, "READ", sizeof(pHeaders[8].key));
//    pHeaders[8].iVal= handle->head.READ;
//    pHeaders[8].valType = HDR_INT;
//    strncpy(pHeaders[8].comment, "Read number in ramp sequence", sizeof(pHeaders[8].comment));

//    strncpy(pHeaders[9].key, "RAMP", sizeof(pHeaders[9].key));
//    pHeaders[9].iVal = handle->head.RAMP;
//    pHeaders[9].valType = HDR_INT;
//    strncpy(pHeaders[9].comment, "Ramp sequence number", sizeof(pHeaders[9].comment));

//    strncpy(pHeaders[10].key, "JD", sizeof(pHeaders[10].key));
//    strncpy(pHeaders[10].sVal, handle->head.JD.c_str(), sizeof(pHeaders[10].sVal));
//    pHeaders[10].valType = HDR_STR;
//    strncpy(pHeaders[10].comment, "Julian date taken before the file is written on disk", sizeof(pHeaders[10].comment));

//    strncpy(pHeaders[11].key, "SEQID", sizeof(pHeaders[11].key));
//    char uniqueID[20];
//    memset(uniqueID,0,20);
//    sprintf(uniqueID,"%ld",handle->head.ID);
//    strncpy(pHeaders[11].sVal,uniqueID,sizeof (pHeaders[11].sVal) );
//    pHeaders[11].valType = HDR_STR;
//    strncpy(pHeaders[11].comment, "Unique Identification number for the sequence", sizeof(pHeaders[11].comment));

//    strncpy(pHeaders[12].key, "WARMTST", sizeof(pHeaders[12].key));
//    pHeaders[12].iVal = handle->head.coldWarmMode;
//    pHeaders[12].valType = HDR_INT;
//    strncpy(pHeaders[12].comment, "0- cold test; 1- warm test ", sizeof(pHeaders[12].comment));

//    strncpy(pHeaders[13].key, "COLUMN", sizeof(pHeaders[13].key));
//    pHeaders[13].iVal = handle->head.columnMode;
//    pHeaders[13].valType = HDR_INT;
//    strncpy(pHeaders[13].comment, "0->H4RG-10, 1->H4RG-15 Col.Disa., 2->H4RG-15 Col.Ena.", sizeof(pHeaders[13].comment));

//    strncpy(pHeaders[14].key, "WINDOW", sizeof(pHeaders[14].key));
//    pHeaders[14].iVal = handle->head.windowMode;
//    pHeaders[14].valType = HDR_INT;
//    strncpy(pHeaders[14].comment, "0->full frame; 1-> window mode", sizeof(pHeaders[14].comment));

//    strncpy(pHeaders[15].key, "Xstart", sizeof(pHeaders[15].key));
//    pHeaders[15].iVal = static_cast<int>(handle->pAcq.winStartX);
//    pHeaders[15].valType = HDR_INT;
//    strncpy(pHeaders[15].comment, "start pixel in X", sizeof(pHeaders[15].comment));

//    strncpy(pHeaders[16].key, "Xstop", sizeof(pHeaders[16].key));
//    pHeaders[16].iVal = static_cast<int>(handle->pAcq.winStopX);
//    pHeaders[16].valType = HDR_INT;
//    strncpy(pHeaders[16].comment, "stop pixel in X", sizeof(pHeaders[16].comment));

//    strncpy(pHeaders[17].key, "LTIME", sizeof(pHeaders[17].key));
//    strncpy(pHeaders[17].sVal, handle->head.timeLocal.c_str(), sizeof(pHeaders[17].sVal));
//    pHeaders[17].valType = HDR_STR;
//    strncpy(pHeaders[17].comment, "Local date-time.", sizeof(pHeaders[17].comment));

//    strncpy(pHeaders[18].key, "GMTTIME", sizeof(pHeaders[18].key));
//    strncpy(pHeaders[18].sVal, handle->head.timeGMT.c_str(), sizeof(pHeaders[18].sVal));
//    pHeaders[18].valType = HDR_STR;
//    strncpy(pHeaders[18].comment, "GMT date-time.", sizeof(pHeaders[18].comment));

//    strncpy(pHeaders[19].key, "SOURCE", sizeof(pHeaders[20].key));
//    strncpy(pHeaders[19].sVal, std::string(handle->head.OBSERVATORY+"-"+handle->head.OBSLOCATION).c_str(), sizeof(pHeaders[19].sVal));
//    pHeaders[19].valType = HDR_STR;
//    strncpy(pHeaders[19].comment, "Source of data", sizeof(pHeaders[19].comment));
//    //add the telemetry
//    if (v.size()>=8)
//    {



//    memset( pHeaders[19+1].sVal,0,72);
//    sprintf(pHeaders[19+1].sVal,"%lf,%lf",v[0],c[0]);
//    strncpy(pHeaders[19+1].key, regname[0].c_str(), sizeof(pHeaders[19+1].key));
//    pHeaders[19+1].valType = HDR_STR;
//    strncpy(pHeaders[19+1].comment, regdef[0].c_str(), sizeof(pHeaders[19+1].comment));

//    memset( pHeaders[20+1].sVal,0,72);
//    sprintf(pHeaders[20+1].sVal,"%lf,%lf",v[1],c[1]);
//    strncpy(pHeaders[20+1].key, regname[1].c_str(), sizeof(pHeaders[20+1].key));
//    pHeaders[20+1].valType = HDR_STR;
//    strncpy(pHeaders[20+1].comment, regdef[1].c_str(), sizeof(pHeaders[20+1].comment));

//    memset( pHeaders[21+1].sVal,0,72);
//    sprintf(pHeaders[21+1].sVal,"%lf,%lf",v[2],c[2]);
//    strncpy(pHeaders[21+1].key, regname[2].c_str(), sizeof(pHeaders[21+1].key));
//    pHeaders[21+1].valType = HDR_STR;
//    strncpy(pHeaders[21+1].comment, regdef[2].c_str(), sizeof(pHeaders[21+1].comment));

//    memset( pHeaders[22+1].sVal,0,72);
//    sprintf(pHeaders[22+1].sVal,"%lf,%lf",v[3],c[3]);
//    strncpy(pHeaders[22+1].key, regname[3].c_str(), sizeof(pHeaders[22+1].key));
//    pHeaders[22+1].valType = HDR_STR;
//    strncpy(pHeaders[22+1].comment,  regdef[3].c_str(), sizeof(pHeaders[22+1].comment));

//    memset( pHeaders[23+1].sVal,0,72);
//    sprintf(pHeaders[23+1].sVal,"%lf,%lf",v[4],c[4]);
//    strncpy(pHeaders[23+1].key, regname[4].c_str(), sizeof(pHeaders[23+1].key));
//    pHeaders[23+1].valType = HDR_STR;
//    strncpy(pHeaders[23+1].comment, regdef[4].c_str(), sizeof(pHeaders[23+1].comment));

//    memset( pHeaders[24+1].sVal,0,72);
//    sprintf(pHeaders[24+1].sVal,"%lf,%lf",v[5],c[5]);
//    strncpy(pHeaders[24+1].key, regname[5].c_str(), sizeof(pHeaders[24+1].key));
//    pHeaders[24+1].valType = HDR_STR;
//    strncpy(pHeaders[24+1].comment, regdef[5].c_str(), sizeof(pHeaders[24+1].comment));

//    memset( pHeaders[25+1].sVal,0,72);
//    sprintf(pHeaders[25+1].sVal,"%lf,%lf",v[6],c[6]);
//    strncpy(pHeaders[25+1].key, regname[6].c_str(), sizeof(pHeaders[25+1].key));
//    pHeaders[25+1].valType = HDR_STR;
//    strncpy(pHeaders[25+1].comment, regdef[6].c_str(), sizeof(pHeaders[25+1].comment));

//    memset( pHeaders[26+1].sVal,0,72);
//    sprintf(pHeaders[26+1].sVal,"%lf,%lf",v[7],c[7]);
//    strncpy(pHeaders[26+1].key, regname[7].c_str(), sizeof(pHeaders[26+1].key));
//    pHeaders[26+1].valType = HDR_STR;
//    strncpy(pHeaders[26+1].comment, regdef[7].c_str(), sizeof(pHeaders[26+1].comment));
//}
//    strncpy(pHeaders[27+1].key, "MACIELIB", sizeof(pHeaders[27+1].key));
//    pHeaders[27+1].fVal = handle->macie_libv;
//    pHeaders[27+1].valType = HDR_FLOAT;
//    strncpy(pHeaders[27+1].comment, "MACIE library version number.", sizeof(pHeaders[27+1].comment));

//    //dump MACIE telemetry in header (79 entries)
//    char kwbuffer[8];

//    for (int i=0;i<79;i++) {
//        memset(kwbuffer,0,8);
//        sprintf(kwbuffer,"MTLM%2.2d",i+1);

//        strncpy(pHeaders[28+1+i].key, kwbuffer, sizeof(pHeaders[28+1+i].key));
//        pHeaders[28+1+i].fVal = *(pTlmVals+i);
//        pHeaders[28+1+i].valType = HDR_FLOAT;
//        strncpy(pHeaders[28+1+i].comment, macie_tlm_def[i], sizeof(pHeaders[28+1+i].comment));

//    }

//return 0;
//}
bool isWrittable(std::string path)
/*
 * Check if directory is writtable or not
 */
{
    int result = access(path.c_str(), W_OK);
    if (result == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
