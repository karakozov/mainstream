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
    m_sync = 0;
    connect(ui->pbStartConfiguration, SIGNAL(clicked()), this, SLOT(startSystemConfiguration()));
}

//-----------------------------------------------------------------------------

MainWindow::~MainWindow()
{
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
    m_params.syncFd = ui->leSyncFO->text().toFloat(&ok);

    unsigned fpgaCount = create_fpga_list(m_fpgaList, 16, 0);
    if(fpgaCount) {
        unsigned brdCount = create_board_list(m_fpgaList, m_boardList, &m_sync, m_params.boardMask);
    }

    ui->ptTrace->appendPlainText("");
}

//-----------------------------------------------------------------------------
