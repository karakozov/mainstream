#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "fpga.h"
#include "acsync.h"
#include "acdsp.h"
#include "sysconfig.h"
#include "iniparser.h"

#include <QMainWindow>
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

private slots:
    void startSystemConfiguration();
};

#endif // MAINWINDOW_H
