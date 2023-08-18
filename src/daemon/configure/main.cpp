#include "vaudit-configure.h"
#include <QCoreApplication>

//https://www.freesion.com/article/878895117/ dbus
//https://www.cnblogs.com/xia-weiwen/archive/2017/05/04/6806709.html sql
//https://blog.csdn.net/qq_42283621/article/details/119303877
/*
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make clean;make
*/
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    VauditConfigureDbus bus;
    return a.exec();
}
