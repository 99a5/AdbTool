#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AdbTool.h"
#pragma once
#include <QProcess>
#include <QTimer>
#include <QLineSeries>
#include <QValueAxis>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QtCharts>
#include <QtCharts/QChartView>
#include <QChart>

QT_CHARTS_USE_NAMESPACE

class QPushButton;
class QTextEdit;
class QProgressBar;
class QComboBox;
class QLabel;
class QChartView;


class AdbTool : public QMainWindow
{
    Q_OBJECT

public:
    AdbTool(QWidget *parent = nullptr);
    ~AdbTool();

   void connectSlots();
   void initData();
   void initUI();


protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onDetectAdb();
    void onListDevices();
    void onInstallApk();
    void onPushFile();
    void onPullFile();
    void onStartApp();
    void onExportLog();
    void onStartCpuMonitor();
    void onStopCpuMonitor();

    // adb callbacks
    void readAdbOutput();
    void adbFinished(int exitCode, QProcess::ExitStatus status);

    // cpu monitor
    void cpuPoll();

private:
    void appendLog(const QString& s);
    QString adbCommand(const QStringList& args);

    // UI elements
    QWidget* central;
    QVBoxLayout* lay;
    QHBoxLayout* topRow;
    QHBoxLayout* ppRow;
    QHBoxLayout* logRow;
    QHBoxLayout* monRow;
    QChart* chart;
    QHBoxLayout* apkRow;

    QPushButton* btnDetect;
    QPushButton* btnListDevices;
    QComboBox* cbDevices;
    QPushButton* btnInstall;
    QPushButton* btnPush;
    QPushButton* btnPull;
    QPushButton* btnStartApp;
    QPushButton* btnExportLog;
    QPushButton* btnBrowseApk;
    QPushButton* btnStartMon;
    QPushButton* btnStopMon;

    QTextEdit* logView;
    QProgressBar* progressBar;
    QLineEdit* leAdbPath;
    QLineEdit* leApkPath;
    QLineEdit* lePackageName;
    QComboBox* cbMonitorProcess;

    // chart
    QtCharts::QChartView* chartView;
    QLineSeries* cpuSeries;
    QValueAxis* axisX;
    QValueAxis* axisY;

    QProcess* adbProcess;
    QTimer* cpuTimer;
    int cpuXIndex = 0;

    QString currentDeviceSerial;
    QString lastHistoryFile = "adb_history.txt";
private:
    Ui::AdbToolClass ui;
};
