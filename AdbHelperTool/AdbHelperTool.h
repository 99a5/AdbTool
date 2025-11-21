#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QStatusBar>
#include <QMap>
#include "ui_AdbHelperTool.h"
#include "Common.h"

class AdbHelperTool : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdbHelperTool(QWidget* parent = nullptr);
    ~AdbHelperTool();

private:
    /* ---------------------- 初始化与连接 ---------------------- */
    void initUI();
    void connectSlots();
    void detectAdbPath();

    /* ---------------------- 设备管理 ---------------------- */
    void updateDeviceList();
    std::vector<DeviceInfo> getAdbDeviceList();
    std::vector<DeviceInfo> getCheckedDevices();
    void setDeviceOnlineState(const DeviceInfo& dev, bool ok);

    /* ---------------------- ADB 命令封装 ---------------------- */
    QString runAdb(const QString& serial, const QString& cmd);
    QString runAdbRaw(const QStringList& args);
    void runAdbAsync(const DeviceInfo& dev,
        const QStringList& args,
        const QString& runningText);

    /* ---------------------- 日志输出 ---------------------- */
    void appendLog(const QString& text);

    /* ---------------------- 应用管理（单设备页面） ---------------------- */
    void loadInstalledApps(const QString& serial);
    void filterAppList();

private slots:
    /* ---------------------- 设备选项 ---------------------- */
    void onRefreshDevices();
    void onSelectAllDevices();
    void onUnselectAllDevices();
    void onDeviceCheckStateChanged(int index, bool checked);
    void onBatchReboot();

    /* ---------------------- 批量设备操作 ---------------------- */
    /* ---------------------- APK 操作 ---------------------- */
    void onBrowseApk();
    void onBatchInstall();
    void onBatchUninstall();

    /* ---------------------- 推送/拉取 ---------------------- */
    void onBrowsePushFile();
    void onBatchPush();

    void onBrowsePullDir();
    void onBatchPull();

    /* ---------------------- 单设备应用操作 ---------------------- */
    void onCurrentDeviceChanged(int index);
    void onAppSelected();
    void onInstallSingle();
    void onUninstallSingle();
    void onForceStopSingle();

    void onRunCustomCommand();
    void onStartTerminal();
    void onLogExport();


private:
    /* ---------------------- UI / 状态 ---------------------- */
    Ui::AdbHelperToolClass ui;
    QStatusBar* statusBar = nullptr;

    QString m_adbPath;
    QString versionOutput;

    //设备列表（刷新得到）
    std::vector<DeviceInfo> m_allDevices;
    std::vector<DeviceInfo> vecDeviceInfos;
    DeviceInfo m_selectedDevices;
    //当前用于单设备页面的设备
    DeviceInfo m_currentDevice;

    //单设备应用列表
    std::vector<AppInfo> m_currentAppList;

    //用于跟踪异步执行的进程
    QMap<QString, QProcess*> m_runningProcess;
};
