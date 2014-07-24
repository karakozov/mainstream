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
    m_systemConfigured = false;

    connect(ui->pbResetSync, SIGNAL(clicked(bool)), this, SLOT(resetSync(bool)));
    connect(ui->pbStartConfiguration, SIGNAL(clicked()), this, SLOT(startSystemConfiguration()));
    connect(ui->pbStartPcieTest, SIGNAL(clicked()), this, SLOT(startPciExpressTest()));
    connect(ui->pbStopPcieTest, SIGNAL(clicked()), this, SLOT(stopPciExpressTest()));
    connect(ui->pbStartAdcTest, SIGNAL(clicked()), this, SLOT(startAdcTest()));
    connect(ui->pbStopAdcTest, SIGNAL(clicked()), this, SLOT(stopAdcTest()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerIsr()));
    connect(ui->pbSyncConfigure, SIGNAL(clicked()), this, SLOT(startSync()));
    connect(ui->cbSlot1, SIGNAL(clicked()), this, SLOT(setBoardMask()));
    connect(ui->cbSlot2, SIGNAL(clicked()), this, SLOT(setBoardMask()));
    connect(ui->cbSlot3, SIGNAL(clicked()), this, SLOT(setBoardMask()));
    connect(ui->cbSlot4, SIGNAL(clicked()), this, SLOT(setBoardMask()));
    connect(ui->cbSlot5, SIGNAL(clicked()), this, SLOT(setBoardMask()));
    connect(ui->cbSlot6, SIGNAL(clicked()), this, SLOT(setBoardMask()));
    connect(ui->pbFpgaT, SIGNAL(clicked()), this, SLOT(updateFpgaTemperature()));

    //---------------------------------------------------------------------------------------

    connect(ui->pbSyncv11Configure,SIGNAL(clicked()),this,SLOT(startSystemConfiguration()));
    connect(ui->pbSyncv11Sel0,SIGNAL(clicked(bool)),this,SLOT(Selclk0(bool)));
    connect(ui->pbSyncv11Sel1,SIGNAL(clicked(bool)),this,SLOT(Selclk1(bool)));
    connect(ui->pbSyncv11Sel2,SIGNAL(clicked(bool)),this,SLOT(Selclk2(bool)));
    connect(ui->pbSyncv11Sel3,SIGNAL(clicked(bool)),this,SLOT(Selclk3(bool)));
    connect(ui->pbSyncv11Sel4,SIGNAL(clicked(bool)),this,SLOT(Selclk4(bool)));
    connect(ui->pbSyncv11Sel5,SIGNAL(clicked(bool)),this,SLOT(Selclk5(bool)));
    connect(ui->pbSyncv11Sel6,SIGNAL(clicked(bool)),this,SLOT(Selclk6(bool)));
    connect(ui->pbSyncv11Sel7,SIGNAL(clicked(bool)),this,SLOT(Selclk7(bool)));
    connect(ui->pbSyncv11SelOut,SIGNAL(clicked(bool)),this,SLOT(SelclkOut(bool)));

    connect(ui->pbSyncv11Powpll,SIGNAL(clicked(bool)),this,SLOT(EnpowPll(bool)));
    connect(ui->pbSyncv11Powocxo,SIGNAL(clicked(bool)),this,SLOT(EnpowOcxo(bool)));
    connect(ui->pbSyncv11Powclksm,SIGNAL(clicked(bool)),this,SLOT(EnpowClksm(bool)));
    connect(ui->pbSyncv11Master,SIGNAL(clicked(bool)),this,SLOT(Master(bool)));
    connect(ui->pbSyncv11Rst,SIGNAL(clicked(bool)),this,SLOT(Reset(bool)));
    connect(ui->pbSyncv11ENx5,SIGNAL(clicked(bool)),this,SLOT(Enx5(bool)));
    connect(ui->pbSyncv11ENx8,SIGNAL(clicked(bool)),this,SLOT(Enx8(bool)));

    connect(ui->pbSyncv11CheckParams,SIGNAL(clicked()),this,SLOT(CheckParams()));
    connect(ui->pbSyncv11SetupCxDx,SIGNAL(clicked()),this,SLOT(SetupCxDx()));
    connect(ui->cbSyncv11Mode,SIGNAL(currentIndexChanged(int)),this,SLOT(ParametersChange()));
    connect(ui->cbSyncv11FD,SIGNAL(currentIndexChanged(int)),this,SLOT(ParametersChange()));
    connect(ui->cbSyncv11FO,SIGNAL(currentIndexChanged(int)),this,SLOT(ParametersChange()));

    connect(ui->pbSyncv11SetMode,SIGNAL(clicked()),this,SLOT(SetSelectedMode()));

    //---------------------------------------------------------------------------------------

    ui->pbStartConfiguration->setEnabled(!m_systemConfigured);

    setBoardMask();
}


//-----------------------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete m_pcie_thread;
    delete m_adc_thread;

    m_timer->stop();
    delete m_timer;

    delete_board_list(m_boardList, m_sync);
    delete_fpga_list(m_fpgaList);

    delete ui;
}

//-----------------------------------------------------------------------------

void MainWindow::updateSystemParams()
{
    bool ok;

    m_params.testMode = ui->cbTestMode->currentIndex();

    m_params.boardMask = ui->leBoardMask->text().toInt(&ok, 16);
    m_params.fpgaMask = ui->leFpgaMask->text().toInt(&ok, 16);
    m_params.adcMask = ui->leAdcMask->text().toInt(&ok, 16);
    m_params.adcStart = ui->cbAdcStartSource->currentIndex() == 0 ? 0x3 : 0x2;
    m_params.adcStartInv = ui->cbAdcStartInversion->currentText().toInt(&ok, 16);

    m_params.dmaChannel = ui->leDmaChannel->text().toInt(&ok, 16);
    m_params.dmaBlockSize = ui->leDmaBlockSize->text().toInt(&ok, 16);
    m_params.dmaBuffersCount = ui->leDmaBuffersCount->text().toInt(&ok, 10);
    m_params.dmaBlockCount = ui->leDmaBlocksCount->text().toInt(&ok, 10);

    m_params.syncMode = ui->cbSyncMode->currentText().toInt(&ok, 16);
    m_params.syncSelClkOut = ui->cbSyncSelClkOut->currentText().toInt(&ok, 16);
    m_params.syncFd = ui->cbSyncFD->currentText().toFloat(&ok);
    m_params.syncFo = ui->cbSyncFO->currentText().toFloat(&ok);
}

//-----------------------------------------------------------------------------

void MainWindow::startSync()
{
    updateSystemParams();

    if(m_sync) {
        if(!m_sync->checkFrequencyParam(m_params.syncMode, m_params.syncFd, m_params.syncFo)) {
            statusBar()->showMessage("Error: Invalid AC_SYNC parameters!");
            return;
        }
        m_sync->PowerON(true);
        m_sync->progFD(m_params.syncMode, m_params.syncFd, m_params.syncFo);
        m_sync->ResetSync(false);
    }
}

//-----------------------------------------------------------------------------

void MainWindow::startSystemConfiguration()
{
    updateSystemParams();

    if(m_systemConfigured) {
        statusBar()->showMessage("System already configured!");
        return;
    }


    unsigned brdCount = 0;
    unsigned fpgaCount = 0;

    try {
        fpgaCount = create_fpga_list(m_fpgaList, 16, 0);
    } catch(...) {
        ui->ptTrace->appendPlainText("Exception was generated in the program. FPGA list not formed");
    }
    try {
        if(fpgaCount) {
            brdCount = create_board_list(m_fpgaList, m_boardList, &m_sync, m_params.boardMask);
        }
    } catch(except_info_t err) {
        QString msg = err.info.c_str();
        ui->ptTrace->appendPlainText(msg);
    }
    catch(...) {
        ui->ptTrace->appendPlainText("Exception was generated in the program. Board list not formed");
    }

    ui->ptTrace->appendPlainText("Total FPGA: " + QString::number(fpgaCount));
    ui->ptTrace->appendPlainText("Total BOARD: " + QString::number(brdCount));

    if(brdCount) {
        ui->tab_2->setEnabled(true);
        ui->tab_3->setEnabled(true);
        m_pcie_thread = new pcie_test_thread(m_boardList);
        connect(m_pcie_thread,SIGNAL(updateCounters(std::vector<counter_t>*)),this,SLOT(showCountersGUI(std::vector<counter_t>*)));
        connect(m_pcie_thread,SIGNAL(updateDataRate(std::vector<pcie_speed_t>*)),this,SLOT(showRateGUI(std::vector<pcie_speed_t>*)));

        m_adc_thread = new adc_test_thread(m_boardList, m_sync);
        connect(m_adc_thread, SIGNAL(updateInfo(QString)), this, SLOT(showAdcTrace(QString)));

        m_systemConfigured = true;
        ui->pbStartConfiguration->setEnabled(!m_systemConfigured);
    }

    init_display_table(m_tableError);
    init_display_table(m_tableRate);

    if(m_systemConfigured) {
        statusBar()->showMessage("System was successfully configured: FPGA - " + QString::number(fpgaCount) + " BOARD - " + QString::number(brdCount));
    } else {
        statusBar()->showMessage("ERROR: System configuration failed: FPGA - " + QString::number(fpgaCount) + " BOARD - " + QString::number(brdCount));
    }
}

//-----------------------------------------------------------------------------

void MainWindow::startPciExpressTest()
{
    updateSystemParams();

    startSync();

    unsigned period = ui->leUpdateInfoPeriod->text().toInt();

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(period);
    m_timer->start();

    if(m_pcie_thread)
        m_pcie_thread->start_pcie_test(true);

    ui->tab->setEnabled(false);
}

//-----------------------------------------------------------------------------

void MainWindow::stopPciExpressTest()
{
    if(m_pcie_thread)
        m_pcie_thread->start_pcie_test(false);

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(0);

    ui->tab->setEnabled(true);
}

//-----------------------------------------------------------------------------

void MainWindow::startAdcTest()
{
    updateSystemParams();

    startSync();

    unsigned period = ui->leUpdateInfoPeriodAdc->text().toInt();

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(period);
    m_timer->start();

    if(m_adc_thread)
        m_adc_thread->start_adc_test(true, m_params);

    ui->tab->setEnabled(false);
}

//-----------------------------------------------------------------------------

void MainWindow::stopAdcTest()
{
    if(m_adc_thread)
        m_adc_thread->start_adc_test(false, m_params);

    m_timer->stop();
    m_timer_counter = 0;
    m_timer->setInterval(0);

    ui->tab->setEnabled(true);
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
    if((m_timer_counter%5) == 0)
        updateFpgaTemperature();
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
    if(ui->ptTraceAdc->document()->lineCount() == 256)
        ui->ptTraceAdc->clear();
    ui->ptTraceAdc->appendPlainText(buffer);
}

//-----------------------------------------------------------------------------

void MainWindow::setBoardMask()
{
    u32 mask = 0;

    if(ui->cbSlot1->isChecked())
        mask |=  (0x1 << 0);
    else
        mask &= ~(0x1 << 0);

    if(ui->cbSlot2->isChecked())
        mask |=  (0x1 << 1);
    else
        mask &= ~(0x1 << 1);

    if(ui->cbSlot3->isChecked())
        mask |=  (0x1 << 2);
    else
        mask &= ~(0x1 << 2);

    if(ui->cbSlot4->isChecked())
        mask |=  (0x1 << 3);
    else
        mask &= ~(0x1 << 3);

    if(ui->cbSlot5->isChecked())
        mask |=  (0x1 << 4);
    else
        mask &= ~(0x1 << 4);

    if(ui->cbSlot6->isChecked())
        mask |=  (0x1 << 5);
    else
        mask &= ~(0x1 << 5);

    ui->leBoardMask->setText("0x" + QString::number(mask, 16));
    m_params.boardMask = mask;
}

//-----------------------------------------------------------------------------

void MainWindow::updateFpgaTemperature()
{
    for(unsigned i=0; i<m_boardList.size(); i++) {

        acdsp *brd = m_boardList.at(i);

        if(!brd) continue;

        if(brd->slotNumber() == 3) {
            float t30 = 0;
            if(brd->getFpgaTemperature(0, t30)) {
                ui->lb_fpgaT30->setText(QString::number(t30));
            }
            float t31 = 0;
            if(brd->getFpgaTemperature(1, t31)) {
                ui->lb_fpgaT31->setText(QString::number(t31));
            }
        }
        if(brd->slotNumber() == 4) {
            float t40 = 0;
            if(brd->getFpgaTemperature(0, t40)) {
                ui->lb_fpgaT40->setText(QString::number(t40));
            }
            float t41 = 0;
            if(brd->getFpgaTemperature(1, t41)) {
                ui->lb_fpgaT41->setText(QString::number(t41));
            }
        }
        if(brd->slotNumber() == 5) {
            float t50 = 0;
            if(brd->getFpgaTemperature(0, t50)) {
                ui->lb_fpgaT50->setText(QString::number(t50));
            }
            float t51 = 0;
            if(brd->getFpgaTemperature(1, t51)) {
                ui->lb_fpgaT51->setText(QString::number(t51));
            }
        }
        if(brd->slotNumber() == 6) {
            float t60 = 0;
            if(brd->getFpgaTemperature(0, t60)) {
                ui->lb_fpgaT60->setText(QString::number(t60));
            }
            float t61 = 0;
            if(brd->getFpgaTemperature(1, t61)) {
                ui->lb_fpgaT61->setText(QString::number(t61));
            }
        }
    }
}

//-----------------------------------------------------------------------------

void MainWindow::resetSync(bool reset)
{
    updateSystemParams();

    if(reset) {
        ui->pbResetSync->setText("Unreset AC_SYNC");
        if(m_sync) m_sync->ResetSync(true);
    } else {
        ui->pbResetSync->setText("Reset AC_SYNC");
        if(m_sync) m_sync->ResetSync(false);
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MainWindow::Selclk0(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK0;
        else
            selclk &= ~SELCLK0;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::Selclk1(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK1;
        else
            selclk &= ~SELCLK1;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::Selclk2(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK2;
        else
            selclk &= ~SELCLK2;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::Selclk3(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK3;
        else
            selclk &= ~SELCLK3;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::Selclk4(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK4;
        else
            selclk &= ~SELCLK4;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::Selclk5(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK5;
        else
            selclk &= ~SELCLK5;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::Selclk6(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK6;
        else
            selclk &= ~SELCLK6;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::Selclk7(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLK7;
        else
            selclk &= ~SELCLK7;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::SelclkOut(bool set)
{
    if(m_sync) {
        U32 selclk = m_sync->RegPeekInd(0, 4, 0xF);
        if(set)
            selclk |= SELCLKOUT;
        else
            selclk &= ~SELCLKOUT;
        m_sync->RegPokeInd(0, 4, 0xF, selclk);
    }
}
void MainWindow::EnpowPll(bool set)
{
    if(m_sync) {
        U32 mode1 = m_sync->RegPeekInd(0, 4, 0x9);
        if(set)
            mode1 |= ENPOW_PLL;
        else
            mode1 &= ~ENPOW_PLL;
        m_sync->RegPokeInd(0, 4, 0x9, mode1);
    }
}
void MainWindow::EnpowOcxo(bool set)
{
    if(m_sync) {
        U32 mode1 = m_sync->RegPeekInd(0, 4, 0x9);
        if(set)
            mode1 |= ENPOW_OCXO;
        else
            mode1 &= ~ENPOW_OCXO;
        m_sync->RegPokeInd(0, 4, 0x9, mode1);
    }
}
void MainWindow::EnpowClksm(bool set)
{
    if(m_sync) {
        U32 mode1 = m_sync->RegPeekInd(0, 4, 0x9);
        if(set)
            mode1 |= ENPOW_CLKSM;
        else
            mode1 &= ~ENPOW_CLKSM;
        m_sync->RegPokeInd(0, 4, 0x9, mode1);
    }
}
void MainWindow::Master(bool set)
{
    if(m_sync) {
        U32 mode1 = m_sync->RegPeekInd(0, 4, 0x9);
        if(set)
            mode1 |= PRESENCE_MASTER;
        else
            mode1 &= ~PRESENCE_MASTER;
        m_sync->RegPokeInd(0, 4, 0x9, mode1);
    }
}
void MainWindow::Reset(bool set)
{
    if(m_sync) {
        U32 mode1 = m_sync->RegPeekInd(0, 4, 0x9);
        if(set)
            mode1 |= ST_IN_RST;
        else
            mode1 &= ~ST_IN_RST;
        m_sync->RegPokeInd(0, 4, 0x9, mode1);
    }
}
void MainWindow::Enx5(bool set)
{
    if(m_sync) {
        U32 mode1 = m_sync->RegPeekInd(0, 4, 0x9);
        if(set)
            mode1 |= ENx5;
        else
            mode1 &= ~ENx5;
        m_sync->RegPokeInd(0, 4, 0x9, mode1);
    }
}
void MainWindow::Enx8(bool set)
{
    if(m_sync) {
        U32 mode1 = m_sync->RegPeekInd(0, 4, 0x9);
        if(set)
            mode1 |= ENx8;
        else
            mode1 &= ~ENx8;
        m_sync->RegPokeInd(0, 4, 0x9, mode1);
    }
}
void MainWindow::ParametersChange()
{
    if(m_sync) {

        U8 C4 = 0;
        U8 C5 = 0;
        U8 D1 = 0;
        U8 D2 = 0;

        if(!m_sync->checkFrequencyParam(ui->cbSyncv11Mode->currentIndex(),
                                        ui->cbSyncv11FD->currentText().toFloat(),
                                        ui->cbSyncv11FO->currentText().toFloat())) {
            statusBar()->showMessage("ERROR: Invalid frequency combination!");
            return;
        } else {
            statusBar()->showMessage("OK: Frequency combination is valid!");
        }

        m_sync->getCxDxValues(ui->cbSyncv11Mode->currentIndex(),
                              ui->cbSyncv11FD->currentText().toFloat(),
                              ui->cbSyncv11FO->currentText().toFloat(),
                              C4, C5, D1, D2);

        ui->leSyncv11C4->setText(QString::number(C4,10));
        ui->leSyncv11C5->setText(QString::number(C5,10));
        ui->leSyncv11D1->setText(QString::number(D1,10));
        ui->leSyncv11D2->setText(QString::number(D2,10));

        U8 c4_code = 0;
        U8 c5_code = 0;
        U8 d1_code = 0;
        U8 d2_code = 0;
        U32 div01_code = 0;
        U32 div23_code = 0;

        m_sync->GetCxDxEncoded(ui->cbSyncv11Mode->currentIndex(),
                        ui->cbSyncv11FD->currentText().toFloat(),
                        ui->cbSyncv11FO->currentText().toFloat(),
                        c4_code, c5_code, d1_code, d2_code, div01_code, div23_code);

        ui->lbC4Encoded->setText(QString::number(c4_code,10));
        ui->lbC5Encoded->setText(QString::number(c5_code,10));
        ui->lbD1Encoded->setText(QString::number(d1_code,10));
        ui->lbD2Encoded->setText(QString::number(d2_code,10));

        ui->leSyncv11DIV01->setText("0x"+QString::number(div01_code,16));
        ui->leSyncv11DIV23->setText("0x"+QString::number(div23_code,16));
    }
}
void MainWindow::CheckParams()
{
    if(m_sync) {
        if(!m_sync->checkFrequencyParam(ui->cbSyncv11Mode->currentIndex(),
                                        ui->cbSyncv11FD->currentText().toFloat(),
                                        ui->cbSyncv11FO->currentText().toFloat())) {
            statusBar()->showMessage("ERROR: Invalid frequency combination!");
        } else {
            statusBar()->showMessage("OK: Frequency combination is valid!");
        }
        ParametersChange();
    }
}
void MainWindow::SetupCxDx()
{
    if(m_sync) {

        bool ok;

        U8 C4 = ui->leSyncv11C4->text().toInt(&ok,10);
        U8 C5 = ui->leSyncv11C5->text().toInt(&ok,10);
        U8 D1 = ui->leSyncv11D1->text().toInt(&ok,10);
        U8 D2 = ui->leSyncv11D2->text().toInt(&ok,10);

        U32 DIV01 = ((m_sync->getCxDxScale(C5) << 6) | m_sync->getCxDxScale(C4));
        U32 DIV23 = ((m_sync->getCxDxScale(D2) << 6) | m_sync->getCxDxScale(D1));

        m_sync->RegPokeInd(0, 4, 0x10, DIV01);
        m_sync->RegPokeInd(0, 4, 0x11, DIV23);

        ui->lbC4Encoded->setText(QString::number(m_sync->getCxDxScale(C4),10));
        ui->lbC5Encoded->setText(QString::number(m_sync->getCxDxScale(C5),10));
        ui->lbD1Encoded->setText(QString::number(m_sync->getCxDxScale(D1),10));
        ui->lbD2Encoded->setText(QString::number(m_sync->getCxDxScale(D2),10));

        ui->lbDIV01Encoded->setText("0x"+QString::number(DIV01,16));
        ui->lbDIV23Encoded->setText("0x"+QString::number(DIV23,16));
    }
}
void MainWindow::SetSelectedMode()
{
    updateSystemParams();

    if(m_sync) {
        bool res = m_sync->progFD(ui->cbSyncv11Mode->currentIndex(),
                       ui->cbSyncv11FD->currentText().toFloat(),
                       ui->cbSyncv11FO->currentText().toFloat());
        if(res)
            statusBar()->showMessage("Set new mode: " + ui->cbSyncv11Mode->currentText());
        else
            statusBar()->showMessage("Set new mode: ERROR!");
    }
}
