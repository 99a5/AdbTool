#include "AdbTool.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AdbTool w;
    w.show();
    return a.exec();
}
