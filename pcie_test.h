#ifndef PCIE_TEST_H
#define PCIE_TEST_H

#include "acdsp.h"
#include "acsync.h"

#include <vector>

//-----------------------------------------------------------------------------

#define ADC_TRD             4
#define MEM_TRD             5
#define STREAM_BLK_SIZE     0x10000
#define STREAM_BLK_NUM      4

//-----------------------------------------------------------------------------

#define ADC_SAMPLE_FREQ         (500000000.0)
#define ADC_CHANNEL_MASK        (0xF)
#define ADC_START_MODE          (0x3 << 4)
#define ADC_MAX_CHAN            (0x4)

//-----------------------------------------------------------------------------
#define USE_SIGNAL 0
//-----------------------------------------------------------------------------

typedef struct counter_t {
    std::vector<u32>    dataVector0;
    std::vector<u32>    dataVector1;
} counter_t;

//-----------------------------------------------------------------------------

typedef struct pcie_speed_t {
    std::vector<float>  dataVector0;
    std::vector<u32>    dataVector1;
} pcie_speed_t;

//-----------------------------------------------------------------------------

template <class T>
void clearCounters(std::vector<T>& param)
{
    for(unsigned i=0; i<param.size(); i++) {
        T& entry = param.at(i);
        entry.dataVector0.clear();
        entry.dataVector1.clear();
    }
    param.clear();
}

//-----------------------------------------------------------------------------

void getCounters(std::vector<acdsp*>& boardList,
                 std::vector<counter_t>& counters, unsigned tx_number);

void calculateSpeed(std::vector<pcie_speed_t>& dataRate,
                    std::vector<counter_t>& counters0,
                    std::vector<counter_t>& counters1, float dt);

void start_all_fpga(std::vector<acdsp*>& boardList);

void stop_all_fpga(std::vector<acdsp*>& boardList);

#ifndef USE_GUI
bool start_pcie_test(std::vector<acdsp*>& boards, struct app_params_t& params);
#endif

//-----------------------------------------------------------------------------

#endif // PCIE_TEST_H
