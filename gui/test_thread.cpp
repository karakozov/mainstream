#include "test_thread.h"

#include <cstdlib>
#include <cstdio>

pcie_test_thread::pcie_test_thread(std::vector<acdsp*>& boardList) : m_boardList(boardList)
{
    m_start = false;
}

pcie_test_thread::~pcie_test_thread()
{
    m_start = false;
    QThread::wait();
}

void pcie_test_thread::start_pcie_test(bool start)
{
    m_start = start;

    if(m_start) {
        QThread::start();
    } else {
        m_start = false;
        QThread::wait();
    }
}

void pcie_test_thread::run()
{
    if(m_boardList.empty()) {
        return;
    }

    start_all_fpga(m_boardList);

    unsigned N = m_boardList.size();
    struct timeval start0;
    time_start(&start0);

    while(m_start) {

        struct timeval start1;
        time_start(&start1);

        getCounters(m_boardList, counters0, 2*N);

        IPC_delay(2000);

        getCounters(m_boardList, counters1, 2*N);
        calculateSpeed(dataRate, counters0, counters1, time_stop(start1));
        emit updateCounters(&counters1);
        emit updateDataRate(&dataRate);

        //fprintf(stderr, "Working time: %.2f\n", time_stop(start0)/1000.);
        //showCounters(counters1);
        //showSpeed(dataRate);
    }

    stop_all_fpga(m_boardList);
}
