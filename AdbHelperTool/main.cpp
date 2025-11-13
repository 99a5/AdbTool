#include "AdbHelperTool.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AdbHelperTool w;
    w.show();
    return a.exec();
}
