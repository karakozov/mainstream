#ifndef __ACSYNC_H__
#define __ACSYNC_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "utypes.h"
#include "ddwambpex.h"
#include "ctrlstrm.h"
#include "fpga.h"

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define ACSYNC_FPGA_COUNT          1
#define ACSYNC_DMA_CHANNEL_NUM     4

//-----------------------------------------------------------------------------

class Fpga;

//-----------------------------------------------------------------------------

typedef struct _adf4002_t {
    U32 addr;
    U32 reg0;
    U32 reg1;
    U32 reg2;
    U32 reg3;
} adf4002_t;

//-----------------------------------------------------------------------------

class acsync
{
public:
    acsync(U32 fpgaNum);
    virtual ~acsync();

    // EXIT
    void setExitFlag(bool exit);
    bool exitFlag();

    // TRD INTERFACE
    void RegPokeInd(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekInd(U32 fpgaNum, S32 trdNo, S32 rgnum);
    void RegPokeDir(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekDir(U32 fpgaNum, S32 trdNo, S32 rgnum);

    bool checkFrequencyParam(float FD, float FO);
    bool progFD(U32 mode, U32 selout, float FD, float FO);

    void calcADF4002(U32 FO, U32 Fvco, U32 variant = 0);
    void progADF4002(U32 FO, U32 Fvco, U32 variant = 0);
    void FreqMultipler1(U32 mode, U32 FO);
    void FreqMultipler2(U32 mode, float FD);
    void FreqDivider(U32 mode, float FD);
    void PowerON(bool on);

private:
    std::vector<Fpga*>       m_fpga;
    bool                     m_exit;
    float                    m_FD;
    U32                      m_mode;
    fpga_trd_t               m_sync_trd;
    adf4002_t                m_adf_regs;
    U08                      m_Cx[17];
    U32                      m_FVCO_ADF4002;

    void createFpgaDevices(U32 fpgaNum);
    void deleteFpgaDevices();
    void selclkMode0(U32 FO);
    void selclkMode1(U32 FO);
    void selclkMode2(U32 FO);
    void selclkout(U32 sel);

    U32 gcd(U32 a, U32 b);
    void writeADF4002(U16 reg, U32 data);
    void fillMultipler2();
    U08 getMultipler2Scale(int code);
    U08 getDividerScale(int code);

    void getMultCxMode0(float FD, U08& C4, U08& C5);
    void getMultCxMode1(float FD, U08& C4, U08& C5);
    void getMultCxMode2(float FD, U08& C4, U08& C5);

    void getDivDxMode0(float FD, U08& D1, U08& D2);
    void getDivDxMode1(float FD, U08& D1, U08& D2);
    void getDivDxMode2(float FD, U08& D1, U08& D2);

    //-----------------------------------------------------------------------------

    Fpga *FPGA(unsigned fpgaNum);
};

#endif // __ACSYNC_H__
