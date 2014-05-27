#ifndef PCIE_TEST_THREAD_H
#define PCIE_TEST_THREAD_H

#include "pcie_test.h"
#include "sysconfig.h"
#include "acdsp.h"
#include "isvi.h"
#include <QObject>
#include <QThread>
#include <vector>

class pcie_test_thread : public QThread
{
    Q_OBJECT

public:
    pcie_test_thread(std::vector<acdsp*>& boardList);
    virtual ~pcie_test_thread();

    void start_pcie_test(bool start);

private:
    bool                        m_start;
    std::vector<acdsp*>&        m_boardList;
    std::vector<counter_t>      counters0;
    std::vector<counter_t>      counters1;
    std::vector<pcie_speed_t>   dataRate;

    void run();

signals:
    void updateCounters(std::vector<counter_t>* cnt);
    void updateDataRate(std::vector<pcie_speed_t>* rate);
};

#endif // PCIE_TEST_THREAD_H
