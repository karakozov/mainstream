#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdio>
#include <cstdlib>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------
/*
#include <QThread>
#include <QDebug>
#include <io.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define BUFFER_SIZE 0x1000

class StreamReader : public QThread
{
    Q_OBJECT

public:
    StreamReader(QObject *parent);
    virtual ~StreamReader();
    void cleanUp();
    bool setUp();
    void run();

    HANDLE oldStdoutHandle;
    HANDLE stdoutRead;
    HANDLE stdoutWrite;
    int hConHandle;
    bool initDone;

signals:
    void textReady(QString msg);
};

StreamReader::StreamReader(QObject *parent) : QThread(parent)
{
    // void
}

void StreamReader::cleanUp()
{
    // restore stdout
    SetStdHandle (STD_OUTPUT_HANDLE, oldStdoutHandle);

    CloseHandle(stdoutRead);
    CloseHandle(stdoutWrite);
    CloseHandle (oldStdoutHandle);

    hConHandle = -1;
    initDone = false;
}

bool StreamReader::setUp()
{
    if (initDone)
    {
        if (this->isRunning())
            return true;
        else
            cleanUp();
    }

    do
    {
        // save stdout
        oldStdoutHandle = ::GetStdHandle (STD_OUTPUT_HANDLE);
        if (INVALID_HANDLE_VALUE == oldStdoutHandle)
            break;

        if (0 == ::CreatePipe(&stdoutRead, &stdoutWrite, NULL, 0))
            break;

        // redirect stdout, stdout now writes into the pipe
        if (0 == ::SetStdHandle(STD_OUTPUT_HANDLE, stdoutWrite))
            break;

        // new stdout handle
        HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (INVALID_HANDLE_VALUE == lStdHandle)
            break;

        hConHandle = ::_open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
        FILE *fp = ::_fdopen(hConHandle, "w");
        if (!fp)
            break;

        // replace stdout with pipe file handle
        *stdout = *fp;

        // unbuffered stdout
        ::setvbuf(stdout, NULL, _IONBF, 0);

        hConHandle = ::_open_osfhandle((intptr_t)stdoutRead, _O_TEXT);
        if (-1 == hConHandle)
            break;

        return initDone = true;

    } while(false);

    cleanUp();
    return false;
}

void StreamReader::run()
{
    if (!initDone)
    {
        qCritical("Stream reader is not initialized!");
        return;
    }

    qDebug() << "Stream reader thread is running...";

    QString s;
    DWORD nofRead  = 0;
    DWORD nofAvail = 0;
    char buf[BUFFER_SIZE+2] = {0};
    forever
    {
        PeekNamedPipe(stdoutRead, buf, BUFFER_SIZE, &nofRead, &nofAvail, NULL);
        if (nofRead)
        {
            if (nofAvail >= BUFFER_SIZE)
            {
                while (nofRead >= BUFFER_SIZE)
                {
                    memset(buf, 0, BUFFER_SIZE);
                    if (ReadFile(stdoutRead, buf, BUFFER_SIZE, &nofRead, NULL) && nofRead)
                    {
                        s.append(buf);
                    }
                }
            }
            else
            {
                memset(buf, 0, BUFFER_SIZE);
                if (ReadFile(stdoutRead, buf, BUFFER_SIZE, &nofRead, NULL)  && nofRead)
                {
                    s.append(buf);
                }
            }

            // Since textReady must emit only complete lines,
            // watch for LFs
            if (s.endsWith('\n')) // may be emmitted
            {
                emit textReady(s.left(s.size()-2));
                s.clear();
            }
            else    // last line is incomplete, hold emitting
            {
                if (-1 != s.lastIndexOf('\n'))
                {
                    emit textReady(s.left(s.lastIndexOf('\n')-1));
                    s = s.mid(s.lastIndexOf('\n')+1);
                }
            }

            memset(buf, 0, BUFFER_SIZE);
        }
    }

    // clean up on thread finish
    cleanUp();
}
*/
//-----------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tab_2->setEnabled(false);
    m_tableError = ui->tableError;
    m_modelError = ui->tableError->model();
    m_tableRate = ui->tableRate;
    m_modelRate = ui->tableRate->model();
    m_sync = 0;
    m_timer = new QTimer();
    m_timer_counter = 0;

    connect(ui->pbStartConfiguration, SIGNAL(clicked()), this, SLOT(startSystemConfiguration()));
    connect(ui->pbStartPcieTest, SIGNAL(clicked()), this, SLOT(startPciExpressTest()));
    connect(ui->pbStopPcieTest, SIGNAL(clicked()), this, SLOT(stopPciExpressTest()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerIsr()));
}

//-----------------------------------------------------------------------------

MainWindow::~MainWindow()
{
    if(!m_boardList.empty())
        delete_board_list(m_boardList, m_sync);
    if(!m_fpgaList.empty())
        delete_fpga_list(m_fpgaList);
    m_timer->stop();
    delete m_timer;
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

    unsigned brdCount = 0;
    unsigned fpgaCount = create_fpga_list(m_fpgaList, 16, 0);
    if(fpgaCount) {
        brdCount = create_board_list(m_fpgaList, m_boardList, &m_sync, m_params.boardMask);
    }

    ui->ptTrace->appendPlainText("Total FPGA: " + QString::number(fpgaCount));
    ui->ptTrace->appendPlainText("Total BOARD: " + QString::number(brdCount));

    if(!brdCount) {
        ui->tab_2->setEnabled(true);
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
    start_all_fpga(m_boardList);
}

//-----------------------------------------------------------------------------

void MainWindow::stopPciExpressTest()
{
    m_timer->stop();
    m_timer->setInterval(0);
    stop_all_fpga(m_boardList);
}

//-----------------------------------------------------------------------------

void MainWindow::startAdcTest()
{
}

//-----------------------------------------------------------------------------

void MainWindow::stopAdcTest()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void MainWindow::init_display_table(QTableWidget *table)
{
    unsigned N = 4;//m_boardList.size();
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
    std::vector<counter_t>      counters0;
    std::vector<counter_t>      counters1;
    std::vector<pcie_speed_t>   dataRate;

    unsigned N = m_boardList.size();

    ++m_timer_counter;

    getCounters(m_boardList, counters0, 2*N);
    getCounters(m_boardList, counters1, 2*N);

    calculateSpeed(dataRate, counters0, counters1, m_timer_counter*m_timer->interval());

    showCountersGUI(counters1);
    showRateGUI(dataRate);

    statusBar()->showMessage("Working time: " + QString::number(m_timer_counter * m_timer->interval()) + " ms");
}

//-----------------------------------------------------------------------------

void MainWindow::showCountersGUI(std::vector<counter_t>& counters)
{
    for(unsigned row=0; row<counters.size(); row++) {
        counter_t& rd_cnt = counters.at(row);
        for(unsigned col=0; col<rd_cnt.dataVector0.size(); col++) {
            QString s = QString::number(rd_cnt.dataVector0.at(col)) + " / " + QString::number(rd_cnt.dataVector1.at(col));
            m_modelError->setData(m_modelError->index(row, col), s);
        }
    }
}

//-----------------------------------------------------------------------------

void MainWindow::showRateGUI(std::vector<pcie_speed_t>& dataRate)
{
    for(unsigned row=0; row<dataRate.size(); row++) {

        pcie_speed_t& rateVector = dataRate.at(row);

        for(unsigned col=0; col<rateVector.dataVector0.size(); col++) {
            QString s = QString::number(rateVector.dataVector0.at(col));
            m_modelError->setData(m_modelError->index(row, col), s);
        }
    }
}

//-----------------------------------------------------------------------------
