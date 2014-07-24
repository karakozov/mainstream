#include <qglobal.h>
#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 0))
#include <QApplication>
#else
#include <QtGui/QApplication>
#endif
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
