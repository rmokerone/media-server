
#include "rtsp-push.h"
#include "rtsp-push-internal.h"
#include <time.h>
#include <unistd.h>
#include <time64.h>



int rtsp_push_sendrtp(struct rtsp_push_t* rtsp,const char* filename,int sample,int channel)
{

    char tpktHeader[4]  = {0};
    char rtpHeader[12]  = {0};
    int  nPCMBufferSize = 2048;//每次读取的音频字节
    int  bytes = nPCMBufferSize + sizeof(rtpHeader);//每次发送包总字节数

    char* rtpPacket = (char*)malloc(bytes*2);

    unsigned char* pcmBuffer = (unsigned char*)malloc(sizeof(unsigned char) * nPCMBufferSize);


    tpktHeader[0] = 0x24;
    tpktHeader[1] = 0x00;
    tpktHeader[2] = (bytes >> 8)&0xff;
    tpktHeader[3] = bytes & 0xff;

    memcpy(rtpPacket,tpktHeader,sizeof(tpktHeader));
    char* rtpBffer = rtpPacket + sizeof(tpktHeader);

    FILE *fin= NULL;
    fin = fopen(filename,"rb");
    if(!fin){
        printf("error open file\n");
        return -1;
    }     

    int nRet = 0;
    uint16_t seq_num  = 0;
    uint16_t delay = (uint16_t)(1000 /(double)(sample * channel * 2 / nPCMBufferSize)); //计算每次发送的音频时长    
    if (delay > 30) delay = delay -30;

    rtpBffer[0] = 0x80; //version 2 padding 0
    rtpBffer[1] = 0x60; //payload type 96

    uint32_t timeR = time64_now();
	time64_t clock = time64_now();
    time64_t m_rtp_clock = clock;
    rtpBffer[8]  = (timeR >> 24) & 0xFF;
    rtpBffer[9]  = (timeR >> 16) & 0xFF;
    rtpBffer[10] = (timeR >> 8) & 0xFF;
    rtpBffer[11] = timeR & 0xFF;  //随机CSRC
    while (1)
    {
        clock = time64_now();
        if (m_rtp_clock + delay < clock)
	    {
            if((nRet = fread(pcmBuffer, 1, nPCMBufferSize, fin)) != nPCMBufferSize){
                fseek(fin,0,SEEK_SET);
                continue;
            }

            seq_num ++;
            rtpBffer[2] = (seq_num >> 8) & 0xFF;
            rtpBffer[3] = seq_num & 0xff;       //SEQ

            rtpBffer[4] = (timeR >> 24) & 0xFF;
            rtpBffer[5] = (timeR >> 16) & 0xFF;
            rtpBffer[6] = (timeR >> 8) & 0xFF;
            rtpBffer[7] = timeR & 0xFF;         //SSRC

            memcpy(rtpBffer + sizeof(rtpHeader), pcmBuffer, nRet);
            rtsp->handler.send(rtsp->param,rtsp->aggregate_uri, rtpPacket, bytes + sizeof(tpktHeader));
            timeR += delay;
            m_rtp_clock += delay;
        }
        usleep(1000 * delay/3);
    }
}
