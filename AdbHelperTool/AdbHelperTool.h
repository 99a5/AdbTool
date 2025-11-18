#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AdbHelperTool.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QProcess>
#include "Common.h"

class AdbHelperTool : public QMainWindow
{
    Q_OBJECT

public:
    AdbHelperTool(QWidget *parent = nullptr);
    ~AdbHelperTool();

    void connectSlots();

    //批量操作界面
    void initBatcjUI();

private slots:

    // 刷新设备列表
    void refreshDeviceList();    

    // 全选
    void slotSelectAll();     

    // 反选
    void slotCancelSelectAll(); 

    // 单选
    void slotChangeCheck(int index, bool checked);

    // 批量重启设备
    void slotRebootDeviceBth();

    // 获取选中设备列表
    std::vector<DeviceInfo> getCheckedDeviceList();


    //选择 APK 文件
    void onBrowseApk();
    //批量安装
    void onBatchInstall();
    //批量卸载
    void onBatchUninstall();

    //选择要推送的文件
    void onBrowsePushLocal();
    //批量推送
    void onBatchPush();

    //选择保存目录
    void onBrowsePullLocal();
    //批量拉取
    void onBatchPull();




private:
    // 获取adb设备列表
    std::vector<DeviceInfo> getAdbDeviceList();

    // 检测adb路径
    void detectAdbPath();

    //日支输出
    void appendLog(const QString& s);
    void updateProgressLog();
    void appendProgressLog(const QString& log);


    // 运行adb命令
    void runAdbCommandWithLog(const DeviceInfo& dev,const QStringList& args,const QString& runningText);
    QString runAdbCommand(const QString& serial, const QString& cmd);


private:
    Ui::AdbHelperToolClass ui;
    QStatusBar* statusBar;
    QString m_adbPath;
    std::vector<DeviceInfo> vecDeviceInfos;

    QMap<QString, QProcess*> m_runningProcesses;
    std::vector<DeviceInfo> m_vecDevices;           //选中设备

    QString versionOutput;
};
