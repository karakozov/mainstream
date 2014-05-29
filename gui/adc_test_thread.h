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

    void run();
};

#endif // PCIE_TEST_THREAD_H
