#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("UDPSoftware");
    QCoreApplication::setOrganizationDomain("udpviper.com");
    QCoreApplication::setApplicationName("TalkerApp");
    MainWindow w;
    w.show();
    return a.exec();
}
