#include "hxrg_get_status.h"
#include "uics.h"


void send_data(instHandle *handle)
/*!
  * \brief Send the status to the VLT workstation
  * Fucntion that sent the exposermeter, dit, ndit, revnum, filename
  * exposure time, number of samples, fileformat and exposure status
  * to the VLT WS.
  */
{
    int fd = create_socket(5017);//ip to be confirmed

    cmd *cc = new cmd;
    while (1) {
        cc->recvCMD(fd);

        if (handle->im_ptr!=nullptr )
        {

            static char *encodedData = nullptr;
            size_t encodedLength = islb64EncodeAlloc((const char *)handle->im_ptr,sizeof(handle->im_ptr)*handle->imSize,&encodedData);//probably sizeof(im_ptr)*length data
            if (encodedData==nullptr)
            {
                free(encodedData);
                handle->im_ptr = nullptr;
                continue;
            }
            encodedData[encodedLength++] = '\n';


            std::string reply="1 "+std::to_string(sizeof(handle->im_ptr)*handle->imSize);
            sndMsg(cc->sockfd,reply.c_str());
            std::cout<<"sending data ..."<<std::endl;
            std::string test = std::string(encodedData).substr(0,100);
                std::cout<<test<<std::endl;
            send(cc->sockfd,encodedData,encodedLength,0);
            std::cout<<"data sent"<<std::endl;
            handle->im_ready = false;
        }
        else {
            sndMsg(cc->sockfd,"no data",hxrgCMD_ERR_VALUE);
        }
         handle->im_ptr = nullptr;

    }

}
void send_status(instHandle *handle)
/*!
  * \brief Send the status to the VLT workstation
  * Fucntion that sent the exposermeter, dit, ndit, revnum, filename
  * exposure time, number of samples, fileformat and exposure status
  * to the VLT WS.
  */
{
    int fd = create_socket(5014);

    cmd *cc = new cmd;
    while (1) {
        cc->recvCMD(fd);
        //fetch all information
        hxrgSTATUS status;
        if (handle->pAcq.isOngoing || handle->card.asBeenInit==1 || handle->card.isReconfiguring==1)
        {
            status.isBusy = static_cast<int>(1);
        }
        else {
            status.isBusy = static_cast<int>(0);
        }

        status.dit = static_cast<double>(handle->pDet.effExpTime);
        status.nDit = static_cast<int>(handle->pAcq.ramp);
        status.revNum = 1;
        status.expTime = static_cast<int>((handle->pAcq.read+1)*handle->pDet.effExpTime);//ramp must always be 1
        status.dataReady = static_cast<int>(handle->im_ready);
        //status.fileName = handle->nAcq.name;
        memset(status.fileName,'\0',128);
        strcpy(status.fileName,handle->nAcq.name.c_str());
        status.countDown =status.expTime - static_cast<int>(handle->head.READ*handle->pDet.effExpTime);

        status.expStatus = handle->nAcq.status;
        status.expoMeter[0] = handle->pAcq.flux;
        status.expoMeter[1] = handle->pAcq.flux_optional;
        memset(status.frameType,'\0',32);
        strcpy(status.frameType,handle->head.type.c_str());
        //status.frameType = handle->head.type;
        status.ndSamples = static_cast<int>(handle->pAcq.read);
        if (handle->nAcq.fitsMTD)
        {
            status.fileFormat = 2; //uncompressed
        }
        else {
            status.fileFormat = 0; //uncompressed
        }
        //new stuff
        status.gain = handle->pDet.gain;
        status.warmMode = handle->head.coldWarmMode;
        memset(status.expoMeterMask[0],'\0',32);
        memset(status.expoMeterMask[1],'\0',32);
        strcpy(status.expoMeterMask[0],handle->pAcq.posemeter_mask.c_str());
        strcpy(status.expoMeterMask[1],handle->pAcq.posemeter_mask_optional.c_str());
        status.expoMeterCount = handle->nAcq.expoMeterCount;

        if (handle->card.asBeenInit>1)
        {
            status.isInitialized=1;
        }
        else {
            status.isInitialized=0;
        }


        static char *encodedData = nullptr;
        size_t encodedLength = islb64EncodeAlloc((const char *)&status,sizeof (status),&encodedData);
        if (encodedData==nullptr)
        {
            free(encodedData);
            continue ;//umh pas sur de ça
        }
        encodedData[encodedLength++] = '\n';
        size_t size_status = sizeof (status);
        std::string reply="1 "+std::to_string(size_status);
        //  cmd -> OK dataId sizeData
        sndMsg(cc->sockfd,reply.c_str());
        send(cc->sockfd,encodedData,encodedLength,0);
        //send data

    }

}





