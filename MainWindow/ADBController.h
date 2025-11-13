#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>

class ADBController : public QObject
{
    Q_OBJECT
public:
    explicit ADBController(QObject* parent = nullptr);

    void setAdbPath(const QString& path);
    QStringList listDevices();
    QString deviceInfo(const QString& serial);
    bool installApk(const QString& serial, const QString& apkPath);
    bool uninstallApp(const QString& serial, const QString& package);
    bool pushFile(const QString& serial, const QString& local, const QString& remote);
    bool pullFile(const QString& serial, const QString& remote, const QString& local);
    QString execShell(const QString& serial, const QString& cmd);
    void startLogcat(const QString& serial);
    void stopLogcat();

signals:
    void logcatOutput(QString line);
    void commandFinished(QString cmd, int exitCode);

private:
    QString adbPath;
    QProcess* logcatProcess = nullptr;

    QString runCommand(const QString& cmd);
};
