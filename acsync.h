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

#define ENPOW_PLL           0x1
#define ENPOW_OCXO          0x2
#define ENPOW_CLKSM         0x4
#define PRESENCE_MASTER     0x8
#define ST_IN_RST           0x100
#define ENx5                0x200
#define ENx8                0x400

#define SELCLK0             0x1
#define SELCLK1             0x2
#define SELCLK2             0x4
#define SELCLK3             0x8
#define SELCLK4             0x10
#define SELCLK5             0x20
#define SELCLK6             0x40
#define SELCLK7             0x80
#define SELCLKOUT           0x100

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
    bool progFD(U32 mode, float FD, float FO);

    void calcADF4002(float FO, U32 Fvco, U32 variant = 0);
    void progADF4002(float FO, U32 Fvco, U32 variant = 0);
    void FreqMultipler1(U32 mode, U32 FO);
    void FreqMultiplerDivider(U32 mode, float FD, float FO);
    void GetCxDxEncoded(U32 mode, float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2, U32& DIV01, U32& DIV23);
    void PowerON(bool on);
    void ResetSync(bool on);
    void getCxDxValues(U32 mode, float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2);
    U08 getCxDxScale(int code);

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

    void selclkMode1(U32 FO);
    void selclkMode2(U32 FO);
    void selclkMode3(U32 FO);
    void selclkMode4(U32 FO);
    //void selclkout(U32 sel);

    U32 gcd(U32 a, U32 b);
    void writeADF4002(U16 reg, U32 data);
    void fillCxDx();

    void getMultCxDxMode0(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2);
    void getMultCxDxMode1(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2);
    void getMultCxDxMode2(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2);

    //-----------------------------------------------------------------------------

    Fpga *FPGA(unsigned fpgaNum = 0);
};

#endif // __ACSYNC_H__
