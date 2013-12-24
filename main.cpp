
#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "acdsp.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

void delay(int ms)
{
    struct timeval tv = {0, 0};
    tv.tv_usec = 1000*ms;

    select(0,NULL,NULL,NULL,&tv);
}

//-----------------------------------------------------------------------------

IPC_handle createDataFile(const char *fname);
bool createFlagFile(const char *fname);
bool lockDataFile(const char* fname, int counter);

//-----------------------------------------------------------------------------

#define STREAM_TRD          4
#define STREAM_BLK_SIZE     0x4000
#define STREAM_BLK_NUM      4

//-----------------------------------------------------------------------------

#define ADC_SAMPLE_FREQ         (500000000.0)
#define ADC_CHANNEL_MASK        (0x8)
#define ADC_START_MODE          (0x3 << 4)
#define ADC_MAX_CHAN            (0x8)

//-----------------------------------------------------------------------------
#define USE_SIGNAL 1
//-----------------------------------------------------------------------------

#if USE_SIGNAL
static int g_StopFlag = 0;
void stop_exam(int sig)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "SIGNAL = %d\n", sig);
    g_StopFlag = 1;
}
#endif

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    if(argc != 5) {
        fprintf(stderr, "usage: mainstream <FPGA number> <DMA channel> <ADC mask> <ADC frequency>\n");
        return -1;
    }

#if USE_SIGNAL
    signal(SIGINT, stop_exam);
#else
    IPC_initKeyboard();
#endif

    try {

        U32 fpgaNum = strtol(argv[1], 0, 16);
        U32 DmaChan = strtol(argv[2], 0, 16);
        U32 AdcMask = strtol(argv[3], 0, 16);
        float AdcFreq = strtof(argv[4], 0);
        void *pBuffers[4] = {0,0,0,0,};

        fprintf(stderr, "fpgaNum: 0x%d\n", fpgaNum);
        fprintf(stderr, "DmaChan: 0x%d\n", DmaChan);
        fprintf(stderr, "AdcMask: 0x%x\n", AdcMask);
        fprintf(stderr, "AdcFreq: %f\n", AdcFreq);

        acdsp brd;
/*
        brd.setSi57xFreq(AdcFreq);

        fprintf(stderr, "Init FPGA\n");
        brd.initFpga(fpgaNum);

        fprintf(stderr, "Start testing DMA: %d\n", DmaChan);
        fprintf(stderr, "DMA information:\n" );
        brd.infoDma(fpgaNum);

#ifndef ALLOCATION_TYPE_1
        BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, STREAM_BLK_NUM, STREAM_BLK_SIZE, pBuffers, NULL, };
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(fpgaNum, DmaChan, &sSCA);
#else
        BRDstrm_Stub *pStub = 0;
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(fpgaNum, DmaChan, pBuffers,STREAM_BLK_SIZE,STREAM_BLK_NUM,1,BRDstrm_DIR_IN,0x1000,&pStub);
#endif
        delay(100);

        char fname[64];
        snprintf(fname, sizeof(fname), "data_%d.bin", fpgaNum);
        IPC_handle isviFile = createDataFile(fname);

        char flgname[64];
        snprintf(flgname, sizeof(flgname), "data_%d.flg", fpgaNum);
        createFlagFile(flgname);

        fprintf(stderr, "Set DMA source\n");
        brd.setDmaSource(fpgaNum, DmaChan, STREAM_TRD);

        fprintf(stderr, "Set DMA direction\n");
        brd.setDmaDirection(fpgaNum, DmaChan, BRDstrm_DIR_IN);

        fprintf(stderr, "Set ADC channels mask\n");
        brd.RegPokeInd(fpgaNum,STREAM_TRD,0x10,AdcMask);

        fprintf(stderr, "Set ADC start mode\n");
        brd.RegPokeInd(fpgaNum,STREAM_TRD,0x17,ADC_START_MODE);

        fprintf(stderr, "Start DMA\n");
        brd.startDma(fpgaNum, DmaChan, 0);

        delay(10);

        fprintf(stderr, "Start ADC\n");
        brd.RegPokeInd(fpgaNum,STREAM_TRD,0,0x2038);

        u32 *data0 = (u32*)pBuffers[0];
        u32 *data1 = (u32*)pBuffers[1];
        u32 *data2 = (u32*)pBuffers[2];
        u32 *data3 = (u32*)pBuffers[3];

        unsigned counter = 0;

        while(1) {

            if( brd.waitDmaBuffer(fpgaNum, DmaChan, 2000) < 0 ) {

                u32 status = brd.RegPeekDir(fpgaNum,STREAM_TRD,0x0);
                fprintf( stderr, "ERROR TIMEOUT! STATUS = 0x%.4X\n", status);
                break;

            } else {

                brd.writeBuffer(fpgaNum, DmaChan, isviFile, 0);

                lockDataFile(flgname, counter);
            }

            u32 status = brd.RegPeekDir(fpgaNum,STREAM_TRD,0x0);
            fprintf(stderr, "%d: STATUS = 0x%.4X [0x%.8x 0x%.8x 0x%.8x 0x%.8x]\r", ++counter, (u16)status, data0[0], data1[0], data2[0], data3[0]);

            brd.stopDma(fpgaNum, DmaChan);
            brd.RegPokeDir(fpgaNum,STREAM_TRD,0,0x3);
            brd.startDma(fpgaNum, DmaChan, 0);
            delay(10);
            brd.RegPokeDir(fpgaNum,STREAM_TRD,0,0x2038);

#if USE_SIGNAL
            if(g_StopFlag) {
#else
            if(IPC_kbhit()) {
#endif
                fprintf(stderr, "\n");
                break;
            }

            IPC_delay(50);
        }

        fprintf(stderr, "Stop DMA channel\n");
        brd.stopDma(fpgaNum, DmaChan);

        fprintf(stderr, "Free DMA memory\n");
        brd.freeDmaMemory(fpgaNum, DmaChan);

        fprintf(stderr, "Reset DMA FIFO\n");
        brd.resetDmaFifo(fpgaNum, DmaChan);

        IPC_closeFile(isviFile);
*/

    } catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }

#if !USE_SIGNAL
    IPC_cleanupKeyboard();
#endif

    return 0;
}

//-----------------------------------------------------------------------------

void WriteFlagSinc(const char *fileName, int newData, int newHeader)
{
    IPC_handle handle = NULL;
    int val[2] = {0, 0};

    while( !handle ) {
        handle = IPC_openFile(fileName, IPC_OPEN_FILE | IPC_FILE_RDWR);
    }
    val[0] = newData;
    val[1] = newHeader;
    int res = IPC_writeFile(handle, val, sizeof(val));
    if(res < 0) {
        fprintf( stderr, "Error write flag sync\r\n" );
    }
    IPC_closeFile(handle);
}

//-----------------------------------------------------------------------------

int  ReadFlagSinc(const char *fileName)
{
    IPC_handle handle = NULL;
    int flg;

    while( !handle ) {
        handle = IPC_openFile(fileName, IPC_OPEN_FILE | IPC_FILE_RDWR);
    }
    int res = IPC_readFile(handle, &flg, sizeof(flg));
    if(res < 0) {
        fprintf( stderr, "Error read flag sync\r\n" );
    }
    IPC_closeFile(handle);

    //fprintf( stderr, "%s(): FLG = 0x%x\n", __func__, flg );

    return flg;
}

//-----------------------------------------------------------------------------

bool lockDataFile(const char* fname, int counter)
{
    if(counter==0) {
        WriteFlagSinc(fname, 0xffffffff,0xffffffff);
    } else {
        WriteFlagSinc(fname, 0xffffffff,0x0);
    }

    for(int ii=0;;ii++) {

        int rr = ReadFlagSinc(fname);
#if USE_SIGNAL
        if(g_StopFlag) {
#else
        if(IPC_kbhit()) {
#endif
            break;
        }
        if(rr==0) {
        } break;
        if((rr==(int)0xffffffff) && (ii>0x10000)) {
        } break;
    }

    return true;
}

//-----------------------------------------------------------------------------

IPC_handle createDataFile(const char *fname)
{
    //IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR | IPC_FILE_DIRECT);
    IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR);
    if(!fd) {
        fprintf(stderr, "%s(): Error create file: %s\n", __FUNCTION__, fname);
        throw;
    }

    fprintf(stderr, "%s(): %s - Ok\n", __FUNCTION__, fname);

    return fd;
}

//-----------------------------------------------------------------------------

bool createFlagFile(const char *fname)
{
    //IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR | IPC_FILE_DIRECT);
    IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR);
    if(!fd) {
        fprintf(stderr, "%s(): Error create file: %s\n", __FUNCTION__, fname);
        throw;
    }

    int val[2] = {0, 0};
    int res = IPC_writeFile(fd, val, sizeof(val));
    if(res < 0) {
        fprintf( stderr, "Error write flag sync\r\n" );
    }

    IPC_closeFile(fd);

    return true;
}

//-----------------------------------------------------------------------------
/*
void WriteIsviParams(IPC_handle hfile, unsigned long long nNumberOfBytes)
{
    char	fileInfo[2048];
    char    str_buf[512];

    sprintf(fileInfo, "\r\nDEVICE_NAME_________ " );
    strcat(fileInfo, "Ac_DSP");

    U32 chanMask;
    int num_chan = 0;
    int	chans[ADC_MAX_CHAN];
    U32 mask = 1;
    BRD_ValChan val_chan;
    for(ULONG iChan = 0; iChan < adc_cfg.NumChans; iChan++)
    {
        if(chanMask & mask)
            chans[num_chan++] = iChan;
        val_chan.chan = iChan;
        BRD_ctrl(hADC, 0, BRDctrl_ADC_GETGAIN, &val_chan);
        if(adc_cfg.ChanType == 1)
            val_chan.value = pow(10., val_chan.value/20); // dB -> разы
        gains[iChan] = val_chan.value;
        volt_offset[iChan] = 0.0;
        mask <<= 1;
    }
    if(BRDC_strstr(g_AdcSrvName, _BRDC("ADC1624X192")) || BRDC_strstr(g_AdcSrvName, _BRDC("ADC1624X128")))
    {
        int i = 0, j = 0;
        num_chan = 0;
        for(int i = 0; i < 16; i++)
            num_chan += (chanMask >> i) & 0x1;
        for(i = 0; i < 16; i++)
        {
            if(chanMask & (0x1 << i))
                chans[j++] = i;
            i++;
        }
        for(i = 1; i < 16; i++)
        {
            if(chanMask & (0x1 << i))
                chans[j++] = i;
            i++;
        }
    }

    if(!num_chan)
    {
        BRDC_printf(_BRDC("Number of channels for Isvi Parameters is 0 - Error!!!\n"));
        return;
    }
    BRD_ItemArray chan_order;
    chan_order.item = num_chan;
    chan_order.itemReal = 0;
    chan_order.pItem = &chans;
    BRD_ctrl(hADC, 0, BRDctrl_ADC_GETCHANORDER, &chan_order);

    sprintf(str_buf, "\r\nNUMBER_OF_CHANNELS__ %d", num_chan);
    lstrcatA(fileInfo, str_buf);

    sprintf(str_buf, "\r\nNUMBERS_OF_CHANNELS_ ");
    lstrcatA(fileInfo, str_buf);
    str_buf[0] = 0;
    for(int iChan = 0; iChan < num_chan; iChan++)
    {
        char buf[16];
        sprintf(buf, "%d,", chans[iChan]);
        lstrcatA(str_buf, buf);
    }
    lstrcatA(fileInfo, str_buf);


    ULONG sample_size = format ? format : sizeof(short);
    if(format == 0x80) // упакованные 12-разрядные данные
        sample_size = 2;
    ULONG samples = ULONG(nNumberOfBytes / sample_size / num_chan);
    if(g_Cycle > 1)
         samples *= g_Cycle;
    sprintf(str_buf, "\r\nNUMBER_OF_SAMPLES___ %d", samples );
    lstrcatA(fileInfo, str_buf);

    sprintf(str_buf, "\r\nSAMPLING_RATE_______ %f", g_samplRate );
    lstrcatA(fileInfo, str_buf);

    sprintf(str_buf, "\r\nBYTES_PER_SAMPLES___ %d", sample_size);
    lstrcatA(fileInfo, str_buf);

    if(format == 0x80) // два 12-и битных отсчета упаковано в 3 байта
        lstrcatA(fileInfo, "\r\nSAMPLES_PER_BYTES___ 3");
    else
        lstrcatA(fileInfo, "\r\nSAMPLES_PER_BYTES___ 1");

    if(is_complex)
        sprintf(str_buf, "\r\nIS_COMPLEX_SIGNAL?__ YES");
    else
        sprintf(str_buf, "\r\nIS_COMPLEX_SIGNAL?__ NO");
    lstrcatA(fileInfo, str_buf);

    //double fc[MAX_CHAN];
    sprintf(str_buf, "\r\nSHIFT_FREQUENCY_____ ");
    lstrcatA(fileInfo, str_buf);
    str_buf[0] = 0;
    int num_fc = num_chan;
    for(int iChan = 0; iChan < num_fc; iChan++)
    {
        //char buf[16];
        //fc[iChan] = 0.0;
        //sprintf(buf, "%.2f,", fc[iChan]);
        //lstrcat(str_buf, buf);
        lstrcatA(str_buf, "0.0,");
    }
    lstrcatA(fileInfo, str_buf);

    sprintf(str_buf, "\r\nGAINS_______________ ");
    lstrcatA(fileInfo, str_buf);
    str_buf[0] = 0;
    for(int iChan = 0; iChan < num_chan; iChan++)
    {
        char buf[16];
        sprintf(buf,"%f,", gains[chans[iChan]]);
        lstrcatA(str_buf, buf);
    }
    lstrcatA(fileInfo, str_buf);

    sprintf(str_buf, "\r\nVOLTAGE_OFFSETS_____ ");
    lstrcatA(fileInfo, str_buf);
    str_buf[0] = 0;
    for(int iChan = 0; iChan < num_chan; iChan++)
    {
        char buf[16];
        sprintf(buf,"%f,", volt_offset[iChan]);
        lstrcatA(str_buf, buf);
    }
    lstrcatA(fileInfo, str_buf);

    sprintf(str_buf, "\r\nVOLTAGE_RANGE_______ %f", adc_cfg.InpRange / 1000.);
    lstrcatA(fileInfo, str_buf);

//	int BitsPerSample = !format ? adc_cfg.Bits : 8;
//	if(is_complex)
//		BitsPerSample = 16;
    int BitsPerSample = (format == 1) ? 8 : adc_cfg.Bits;
    if(BRDC_strstr(g_AdcSrvName, _BRDC("ADC1624X192")) || BRDC_strstr(g_AdcSrvName, _BRDC("ADC818X800")))
        if(format == 0 || format == 2)
            BitsPerSample = 16;
    sprintf(str_buf, "\r\nBIT_RANGE___________ %d", BitsPerSample);
    lstrcatA(fileInfo, str_buf);
    lstrcatA(fileInfo, "\r\n");

    if(g_PretrigMode)
    {
        ULONG event_mark = ULONG(g_bStartEvent / sample_size / num_chan);
        sprintf(str_buf, "\r\nPRETRIGGER_SAMPLE___ %d", event_mark);
        lstrcatA(fileInfo, str_buf);
    }

    int len = lstrlenA(fileInfo);

#if defined(__IPC_WIN__) || defined(__IPC_LINUX__)
    int bResult = IPC_writeFile(hfile, fileInfo, len);
        if(bResult < 0)
            BRDC_printf(_BRDC("Write ISVI info error\n"));

}
*/
//-----------------------------------------------------------------------------

