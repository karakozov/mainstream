#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdio>
#include <cstdlib>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tab_2->setEnabled(false);
    ui->tab_3->setEnabled(false);
    m_tableError = ui->tableError;
    m_modelError = ui->tableError->model();
    m_tableRate = ui->tableRate;
    m_modelRate = ui->tableRate->model();
    m_sync = 0;
    m_timer = new QTimer();
    m_timer_counter = 0;
    m_pcie_thread = 0;
    m_adc_thread = 0;

    connect(ui->pbStartConfiguration, SIGNAL(clicked()), this, SLOT(startSystemConfiguration()));
    connect(ui->pbStartPcieTest, SIGNAL(clicked()), this, SLOT(startPciExpressTest()));
    connect(ui->pbStopPcieTest, SIGNAL(clicked()), this, SLOT(stopPciExpressTest()));
    connect(ui->pbStartAdcTest, SIGNAL(clicked()), this, SLOT(startAdcTest()));
    connect(ui->pbStopAdcTest, SIGNAL(clicked()), this, SLOT(stopAdcTest()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerIsr()));
}


//-----------------------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete m_pcie_thread;
    delete m_adc_thread;

    m_timer->stop();
    delete m_timer;

    if(!m_boardList.empty())
        delete_board_list(m_boardList, m_sync);
    if(!m_fpgaList.empty())
        delete_fpga_list(m_fpgaList);
    delete ui;
}

//-----------------------------------------------------------------------------

void MainWindow::startSystemConfiguration()
{
    bool ok;

    m_params.testMode = ui->cbTestMode->currentIndex();

    m_params.boardMask = ui->leBoardMask->text().toInt(&ok, 16);
    m_params.fpgaMask = ui->leFpgaMask->text().toInt(&ok, 16);
    m_params.adcMask = ui->leAdcMask->text().toInt(&ok, 16);

    m_params.dmaChannel = ui->leDmaChannel->text().toInt(&ok, 16);
    m_params.dmaBlockSize = ui->leDmaBlockSize->text().toInt(&ok, 16);
    m_params.dmaBuffersCount = ui->leDmaBuffersCount->text().toInt(&ok, 10);
    m_params.dmaBlockCount = ui->leDmaBlocksCount->text().toInt(&ok, 10);

    m_params.syncMode = ui->leSyncMode->text().toInt(&ok, 16);
    m_params.syncSelClkOut = ui->leSyncSelClkOut->text().toInt(&ok, 16);
    m_params.syncFd = ui->leSyncFD->text().toFloat(&ok);
    m_params.syncFo = ui->leSyncFO->text().toFloat(&ok);

    unsigned brdCount = 0;
    unsigned fpgaCount = create_fpga_list(m_fpgaList, 16, 0);
    if(fpgaCount) {
        brdCount = create_board_list(m_fpgaList, m_boardList, &m_sync, m_params.boardMask);
        if(m_sync) {
            m_sync->PowerON(true);
            m_sync->progFD(m_params.syncMode, m_params.syncSelClkOut, m_params.syncFd, m_params.syncFo);
        }
    }

    ui->ptTrace->appendPlainText("Total FPGA: " + QString::number(fpgaCount));
    ui->ptTrace->appendPlainText("Total BOARD: " + QString::number(brdCount));

    if(brdCount) {
        ui->tab_2->setEnabled(true);
        ui->tab_3->setEnabled(true);
        m_pcie_thread = new pcie_test_thread(m_boardList);
        connect(m_pcie_thread,SIGNAL(updateCounters(std::vector<counter_t>*)),this,SLOT(showCountersGUI(std::vector<counter_t>*)));
        connect(m_pcie_thread,SIGNAL(updateDataRate(std::vector<pcie_speed_t>*)),this,SLOT(showRateGUI(std::vector<pcie_speed_t>*)));

        m_adc_thread = new adc_test_thread(m_boardList);
        connect(m_adc_thread, SIGNAL(updateInfo(QString)), this, SLOT(showAdcTrace(QString)));
    }

    init_display_table(m_tableError);
    init_display_table(m_tableRate);
}

//-----------------------------------------------------------------------------

void MainWindow::startPciExpressTest()
{
    unsigned period = ui->leUpdateInfoPeriod->text().toInt();

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(period);
    m_timer->start();

    if(m_pcie_thread)
        m_pcie_thread->start_pcie_test(true);
}

//-----------------------------------------------------------------------------

void MainWindow::stopPciExpressTest()
{
    if(m_pcie_thread)
        m_pcie_thread->start_pcie_test(false);

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(0);
}

//-----------------------------------------------------------------------------

void MainWindow::startAdcTest()
{
    unsigned period = ui->leUpdateInfoPeriodAdc->text().toInt();

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(period);
    m_timer->start();

    if(m_adc_thread)
        m_adc_thread->start_adc_test(true, m_params);
}

//-----------------------------------------------------------------------------

void MainWindow::stopAdcTest()
{
    if(m_adc_thread)
        m_adc_thread->start_adc_test(false, m_params);

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void MainWindow::init_display_table(QTableWidget *table)
{
    unsigned N = m_boardList.size();
    unsigned ROW = N;
    unsigned COL = 2*N;

    //---------------------------------------

    table->setRowCount(ROW);
    table->setColumnCount(COL);

    QStringList vList1;
    for(unsigned i=0; i<ROW; i++) {
        vList1 << "RX_CHN" + QString::number(i);
    }

    table->setVerticalHeaderLabels(vList1);

    QStringList hList1;
    for(unsigned i=0; i<COL; i++) {
        hList1 << "TX_CHN" + QString::number(i);
    }

    table->setHorizontalHeaderLabels(hList1);
    table->verticalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
}

//-----------------------------------------------------------------------------

void MainWindow::timerIsr()
{
    ++m_timer_counter;
    statusBar()->showMessage("Test working time: " + QString::number(m_timer_counter * m_timer->interval()) + " ms");
}

//-----------------------------------------------------------------------------

void MainWindow::showCountersGUI(std::vector<counter_t>* counters)
{
    for(unsigned row=0; row<counters->size(); row++) {

        counter_t& rd_cnt = counters->at(row);

        for(unsigned col=0; col<rd_cnt.dataVector0.size(); col++) {

            QString s = QString::number(rd_cnt.dataVector0.at(col)) + " / " + QString::number(rd_cnt.dataVector1.at(col));

            m_modelError->setData(m_modelError->index(row, col), s);
        }
    }
}

//-----------------------------------------------------------------------------

void MainWindow::showRateGUI(std::vector<pcie_speed_t>* dataRate)
{
    for(unsigned row=0; row<dataRate->size(); row++) {

        pcie_speed_t& rateVector = dataRate->at(row);

        for(unsigned col=0; col<rateVector.dataVector0.size(); col++) {

            QString s = QString::number(rateVector.dataVector0.at(col));

            m_modelRate->setData(m_modelError->index(row, col), s);
        }
    }
}

//-----------------------------------------------------------------------------

void MainWindow::showAdcTrace(QString buffer)
{
    ui->ptTraceAdc->appendPlainText(buffer);
}
