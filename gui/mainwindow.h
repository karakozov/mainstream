#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "fpga.h"
#include "acsync.h"
#include "acdsp.h"
#include "sysconfig.h"
#include "iniparser.h"
#include "pcie_test.h"
#include "isvi.h"
#include "pcie_test_thread.h"
#include "adc_test_thread.h"

#include <QtWidgets/QMainWindow>
#include <QAbstractItemModel>
#include <QtWidgets/QTableWidget>
#include <QTimer>
#include <vector>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;

    struct app_params_t m_params;

    std::vector<Fpga*> m_fpgaList;
    std::vector<acdsp*> m_boardList;
    acsync* m_sync;

    QTimer* m_timer;
    unsigned m_timer_counter;
    pcie_test_thread *m_pcie_thread;
    adc_test_thread  *m_adc_thread;

    QAbstractItemModel *m_modelError;
    QTableWidget *m_tableError;
    QAbstractItemModel *m_modelRate;
    QTableWidget *m_tableRate;

    bool m_systemConfigured;

    void init_display_table(QTableWidget *table);
    void init_display_labels();

private slots:
    void updateSystemParams();
    void startSystemConfiguration();
    void startSync();
    void startPciExpressTest();
    void stopPciExpressTest();
    void startAdcTest();
    void stopAdcTest();
    void timerIsr();
    void showCountersGUI(std::vector<counter_t>* counters);
    void showRateGUI(std::vector<pcie_speed_t>* dataRate);
    void showAdcTrace(QString buffer);
    void setBoardMask();
    void updateFpgaTemperature();
    void resetSync(bool reset);
    void readDelayFromFpga();
    void writeDelayToFpga();
    void setFpgaStartDelay30(int val);
    void setFpgaStartDelay31(int val);
    void setFpgaStartDelay40(int val);
    void setFpgaStartDelay41(int val);
    void setFpgaStartDelay50(int val);
    void setFpgaStartDelay51(int val);
    void setFpgaStartDelay60(int val);
    void setFpgaStartDelay61(int val);
    void enableDelaySlots(bool connect);
};

#endif // MAINWINDOW_H
