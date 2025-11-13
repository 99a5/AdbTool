#include "ADBController.h"
#include <QDebug>

ADBController::ADBController(QObject* parent) : QObject(parent)
{
    adbPath = "adb"; // 默认系统 PATH 下 adb
}

void ADBController::setAdbPath(const QString& path)
{
    adbPath = path;
}

QStringList ADBController::listDevices()
{
    QString output = runCommand(adbPath + " devices");
   /* QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    QStringList devices;
    for (const QString& line : lines) {
        if (line.endsWith("\tdevice")) {
            devices.append(line.split("\t").first());
        }
    }
    return devices;*/
    QStringList devices;
    return devices;
}

QString ADBController::runCommand(const QString& cmd)
{
    //QProcess proc;
    //proc.start(cmd);
    //proc.waitForFinished(5000); // 可调超时
    //return proc.readAllStandardOutput();
    return"";
}
