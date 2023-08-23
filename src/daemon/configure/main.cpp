#include "vaudit-configure.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    VauditConfigureDbus bus;
    return a.exec();
}
