
#include "isvi.h"

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
    IPC_handle fd = IPC_openFile(fname, IPC_CREATE_FILE | IPC_FILE_RDWR);
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

