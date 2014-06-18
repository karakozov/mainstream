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
    explicit acsync(Fpga *fpga);
    virtual ~acsync();

    // EXIT
    void setExitFlag(bool exit);
    bool exitFlag();

    // TRD INTERFACE
    void RegPokeInd(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekInd(U32 fpgaNum, S32 trdNo, S32 rgnum);
    void RegPokeDir(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekDir(U32 fpgaNum, S32 trdNo, S32 rgnum);

    bool checkFrequencyParam(U32 mode, float FD, float FO);
    bool progFD(U32 mode, U32 selout, float FD, float FO);

    void calcADF4002(U32 FO, U32 Fvco, U32 variant = 0);
    void progADF4002(U32 FO, U32 Fvco, U32 variant = 0);
    void FreqMultipler1(U32 mode, U32 FO);
    void FreqMultiplerDivider(U32 mode, float FD, float FO);
    void PowerON(bool on);
    void ResetSync(bool on);

private:
    Fpga*                    m_fpga;
    bool                     m_exit;
    float                    m_FD;
    U32                      m_mode;
    fpga_trd_t               m_sync_trd;
    adf4002_t                m_adf_regs;
    U08                      m_Cx[17];
    U32                      m_FVCO_ADF4002;
    bool                     m_cleanup;
    U32                      m_slotNumber;

    bool initFpga();

    void selclkMode0(U32 FO);
    void selclkMode1(U32 FO);
    void selclkMode2(U32 FO);
    void selclkMode3(U32 FO);
    void selclkout(U32 sel);

    U32 gcd(U32 a, U32 b);
    void writeADF4002(U16 reg, U32 data);
    void fillCxDx();
    U08 getCxDxScale(int code);

    void getMultCxDxMode0(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2);
    void getMultCxDxMode1(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2);
    void getMultCxDxMode2(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2);

    //-----------------------------------------------------------------------------

    Fpga *FPGA(unsigned fpgaNum = 0);
};

#endif // __ACSYNC_H__
