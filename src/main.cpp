#include <QtGui/QApplication>
#include "main_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("UDPSoftware");
    QCoreApplication::setOrganizationDomain("udpviper.com");
    QCoreApplication::setApplicationName("SmoothTalker");
    MainWindow w;
    w.show();
    return a.exec();
}
