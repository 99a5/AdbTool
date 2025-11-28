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

	/*
	* @brief 初始化UI
	* @param submitType
	*/
	void initUI();

	/*
	* @brief 槽函数绑定
	*/
	void connectSlots();

	/*
	* @brief 检测 adb
	*/
	void detectAdbPath();


	/*
	* @brief 更新设备列表
	*/
	void updateDeviceList();

	/*
	* @brief 获取设备列表
	*/
	std::vector<DeviceInfo> getAdbDeviceList();

	/*
	* @brief 获取勾选的设备
	*/
	std::vector<DeviceInfo> getCheckedDevices();

	/*
	* @brief 赋值设备状态
	* @param DeviceInfo dev
	* @param bool ok
	*/
	void setDeviceOnlineState(const DeviceInfo& dev, bool ok);


	/*
	* @brief  ADB 命令封装
	* @param QString& serial
	* @param QString& cmd
	* return QString
	*/
	QString runAdb(const QString& serial, const QString& cmd);

	/*
	* @brief  ADB 命令封装
	* @param QStringList& args
	* return QString
	*/
	QString runAdbRaw(const QStringList& args);

	/*
	* @brief  ADB 命令封装
	* @param DeviceInfo& dev
	* @param QStringList& args
	* @param QString& runningText
	*/
	void runAdbAsync(const DeviceInfo& dev,
		const QStringList& args,
		const QString& runningText);

	/*
	* @brief  日志输出
	* @param  QString& text
	*/
	void appendLog(const QString& text);

	/*
	* @brief  读取已安装应用列表
	* @param  QString& serial
	*/
	void loadInstalledApps(const QString& serial);

	/*
	* @brief  筛选应用列表
	*/
	void filterAppList();

private slots:
	
	/*
	* @brief  刷新设备
	*/
	void onRefreshDevices();

	/*
	* @brief  全选
	*/
	void onSelectAllDevices();

	/*
	* @brief  取消全选
	*/
	void onUnselectAllDevices();

	/*
	* @brief  勾选
	* @param  int  index
	* @param  bool checked
	*/
	void onDeviceCheckStateChanged(int index, bool checked);

	/*
	* @brief  重启
	*/
	void onBatchReboot();

	void onBrowseApk();
	void onBatchInstall();
	void onBatchUninstall();
	void onBrowsePushFile();
	void onBatchPush();
	void onBrowsePullDir();
	void onBatchPull();

	void onCurrentDeviceChanged(int index);
	void onAppSelected();
	void onInstallSingle();
	void onUninstallSingle();
	void onForceStopSingle();

	void onRunCustomCommand();
	void onStartTerminal();
	void onLogExport();


private:
	Ui::AdbHelperToolClass ui;
	QStatusBar* statusBar = nullptr;

	QString m_adbPath;									// adb 路径
	QString versionOutput;								// 版本信息

	std::vector<DeviceInfo> m_allDevices;				// 设备列表（刷新得到）
	std::vector<DeviceInfo> vecDeviceInfos;
	DeviceInfo m_selectedDevices;
	DeviceInfo m_currentDevice;							// 当前用于单设备页面的设备

	std::vector<AppInfo> m_currentAppList;				// 单设备应用列表

	QMap<QString, QProcess*> m_runningProcess;			// 用于跟踪异步执行的进程
};
