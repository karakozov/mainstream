#ifndef ADC_TEST_THREAD_H
#define ADC_TEST_THREAD_H

#include "pcie_test.h"
#include "sysconfig.h"
#include "iniparser.h"
#include "acdsp.h"
#include "isvi.h"
#include <QObject>
#include <QThread>
#include <vector>
#include <string>

typedef std::vector<void*> dmabuf_t;
typedef std::vector<dmabuf_t> brdbuf_t;
typedef std::vector<IPC_handle> datafiles_t;
typedef std::vector<datafiles_t> isvidata_t;
typedef std::vector<std::string> flgnames_t;
typedef std::vector<flgnames_t> isviflg_t;
typedef std::vector<std::string> hdr_t;
typedef std::vector<hdr_t> isvihdr_t;

class adc_test_thread : public QThread
{
    Q_OBJECT

public:
    adc_test_thread(std::vector<acdsp*>& boardList);
    virtual ~adc_test_thread();

    void start_adc_test(bool star, struct app_params_t& params);

private:
    bool                        m_start;
    std::vector<acdsp*>&        m_boardList;
    struct app_params_t         m_params;

    void prepareIsvi(isvidata_t& isviFiles, isviflg_t& flgNames, isvihdr_t& isviHdrs);
    void prepareDma(std::vector<brdbuf_t>& dmaBuffers);
    void startAdcDma();
    void stopAdcDma();
    void startAdcDmaMem();
    void stopAdcDmaMem();
    void dataFromAdc();
    void dataFromMemAsMem();
    void run();
};

#endif // PCIE_TEST_THREAD_H
