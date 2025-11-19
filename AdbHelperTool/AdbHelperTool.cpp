#include "AdbHelperTool.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QDebug>
#include"DeviceItemWidget.h"

AdbHelperTool::AdbHelperTool(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	// 1. 窗口基础设置
	//setCentralWidget(centralWidget);
	resize(720, 480);
	setWindowTitle(QStringLiteral("ADB 智能助手 - Demo"));

	// 2. 底部状态栏
	statusBar = new QStatusBar(this);
	setStatusBar(statusBar);
	statusBar->showMessage(QStringLiteral("adb 版本 1.0.0 | 上次刷新：--"));

	vecDeviceInfos.clear();
	DeviceInfo deviceInfo;
	deviceInfo.name = QStringLiteral("设备1");
	deviceInfo.serial = "1234567890";
	deviceInfo.system = "Android 10";
	DeviceInfo deviceInfo2;
	deviceInfo2.name = QStringLiteral("设备2");
	deviceInfo2.serial = "12345678901";
	deviceInfo2.system = "Android 10";
	DeviceInfo deviceInfo3;
	deviceInfo3.name = QStringLiteral("设备3");
	deviceInfo3.serial = "12345678902";
	deviceInfo3.system = "Android 10";
	vecDeviceInfos.push_back(deviceInfo);
	vecDeviceInfos.push_back(deviceInfo2);
	vecDeviceInfos.push_back(deviceInfo3);
	connectSlots();

	detectAdbPath();

	initBatcjUI();
}

AdbHelperTool::~AdbHelperTool()
{}

void AdbHelperTool::connectSlots()
{
	connect(ui.btnBatchRefresh, &QPushButton::clicked, this, &AdbHelperTool::refreshDeviceList);
	connect(ui.selectAllBth, &QPushButton::clicked, this, &AdbHelperTool::slotSelectAll);
	connect(ui.cancelSelectBth, &QPushButton::clicked, this, &AdbHelperTool::slotCancelSelectAll);
	connect(ui.rebootDeviceBth, &QPushButton::clicked, this, &AdbHelperTool::slotRebootDeviceBth);

	// 批量安装
	connect(ui.btnBrowseApk, &QPushButton::clicked, this, &AdbHelperTool::onBrowseApk);
	connect(ui.btnBatchInstall, &QPushButton::clicked, this, &AdbHelperTool::onBatchInstall);

	// 批量卸载
	connect(ui.btnBatchUninstall, &QPushButton::clicked, this, &AdbHelperTool::onBatchUninstall);

	// 批量推文件
	connect(ui.pushLocalFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBrowsePushLocal);
	connect(ui.batchPushFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBatchPush);

	// 批量拉文件
	connect(ui.pullLocalFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBrowsePullLocal);
	connect(ui.batchPullFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBatchPull);

	//单设备页面
	//切换设备
	connect(ui.deviceSwitchBox, SIGNAL(currentIndexChanged(int)),this, SLOT(slotChangeDevice(int)));

	//系统应用
	connect(ui.cbSystemApp, &QCheckBox::stateChanged, this, &AdbHelperTool::filterAppList);
	//第三方应用
	connect(ui.cbThirdParty, &QCheckBox::stateChanged, this, &AdbHelperTool::filterAppList);
	//搜索应用
	connect(ui.leSearch, &QLineEdit::textChanged, this, &AdbHelperTool::filterAppList);
	//应用列表
	connect(ui.listApp, &QListWidget::itemSelectionChanged,
		this, &AdbHelperTool::slotAppSelected);
	//安装应用
	//connect(ui.btnInstallApk, &QPushButton::clicked, this, &AdbHelperTool::slotInstallApk);
	////卸载
	//connect(ui.btnUninstallApp, &QPushButton::clicked, this, &AdbHelperTool::slotUninstall);
	////强停
	//connect(ui.btnForceStop, &QPushButton::clicked, this, &AdbHelperTool::slotForceStop);
	////选择文件
	//connect(ui.btnSelectUploadFile, &QPushButton::clicked, this, &AdbHelperTool::slotChooseFile);
	////上传文件
	//connect(ui.btnUploadFile, &QPushButton::clicked, this, &AdbHelperTool::slotUploadFile);

	// 拖拽区域
	ui.dragAreaEdit->setAcceptDrops(true);
	ui.dragAreaEdit->installEventFilter(this);

}

void AdbHelperTool::initBatcjUI()
{

}

std::vector<DeviceInfo> AdbHelperTool::getCheckedDeviceList()
{
	std::vector<DeviceInfo> result;

	int count = ui.listWidget->count();
	for (int i = 0; i < count; ++i)
	{
		QListWidgetItem* item = ui.listWidget->item(i);
		DeviceItemWidget* widget = qobject_cast<DeviceItemWidget*>(ui.listWidget->itemWidget(item));

		if (widget && widget->checkState())  // ← 返回布尔
			result.push_back(widget->getDeviceInfo());
	}

	return result;
}

void AdbHelperTool::detectAdbPath()
{
	appendLog(QStringLiteral("检测 adb..."));
	QString adbPath;

	// 先尝试系统 adb
	QProcess p;
	p.start("adb", QStringList() << "version");
	if (p.waitForStarted(2000) && p.waitForFinished(3000)) {
		QString out = p.readAllStandardOutput();
		QStringList lines = out.split('\n', Qt::SkipEmptyParts);
		for (QString line : lines) {
			line = line.trimmed();
			if (line.startsWith("Installed as")) {
				adbPath = line.mid(QString("Installed as").length()).trimmed();
				break;
			}
		}
		if (adbPath.isEmpty()) {
			// 系统 adb 没找到，取第一行作为版本
			versionOutput = lines.isEmpty() ? QStringLiteral("未知") : lines.first().trimmed();
		}
		else {
			versionOutput = lines.isEmpty() ? QStringLiteral("未知") : lines.first().trimmed();
		}
	}

	// 如果系统 adb 未找到，则使用内置 adb
	if (adbPath.isEmpty()) {
		appendLog(QStringLiteral("未检测到系统 adb，尝试使用软件内置 adb"));
		QString appDir = QCoreApplication::applicationDirPath();
		QString internalAdbPath = appDir + "/platform-tools/adb.exe";

		if (QFile::exists(internalAdbPath)) {
			appendLog(QStringLiteral("使用内置 adb: ") + internalAdbPath);
			adbPath = internalAdbPath;

			// 获取内置 adb 版本
			QProcess proc;
			proc.start(adbPath, QStringList() << "version");
			if (proc.waitForStarted(2000) && proc.waitForFinished(3000)) {
				versionOutput = QString::fromLocal8Bit(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts).value(0).trimmed();
			}
			else {
				versionOutput = QStringLiteral("未知");
			}
		}
		else {
			appendLog(QStringLiteral("未找到内置 adb: ") + internalAdbPath);
			adbPath = "";
			versionOutput = QStringLiteral("未知");
		}
	}

	m_adbPath = adbPath;

	appendLog(QStringLiteral("adb version: ") + versionOutput);

	// ===== 更新状态栏 =====
	QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
	statusBar->showMessage(
		QStringLiteral("版本 %1 | 上次刷新：%2").arg(versionOutput, timeStr)
	);
}

std::vector<DeviceInfo> AdbHelperTool::getAdbDeviceList()
{
	std::vector<DeviceInfo> devices;

	// 1. 检查 adb 路径
	if (m_adbPath.isEmpty()) {
		QMessageBox::warning(nullptr, QStringLiteral("错误"), QStringLiteral("请先设置 ADB 路径"));
		return devices;
	}

	// 2. 获取 adb devices 列表
	QProcess process;
	process.setProgram(m_adbPath);
	process.setArguments({ "devices" });
	process.start();
	process.waitForFinished();

	QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
	QStringList lines = output.split('\n', Qt::SkipEmptyParts);

	for (const QString& line : lines) {

		// 跳过标题行
		if (line.contains("List of devices")) continue;

		// 格式： 0123456789ABCDEF    device
		if (!line.contains("\tdevice")) continue;

		QString serial = line.section('\t', 0, 0).trimmed();
		if (serial.isEmpty()) continue;

		DeviceInfo info;
		info.serial = serial;

		// 3. 读取设备型号
		info.name = runAdbCommand(serial, "shell getprop ro.product.model").trimmed();
		if (info.name.isEmpty()) info.name = "Unknown";

		// 4. 读取系统版本
		info.system = runAdbCommand(serial, "shell getprop ro.build.version.release").trimmed();
		if (info.system.isEmpty()) info.system = "Unknown";

		devices.push_back(info);
	}

	return devices;
}

QString AdbHelperTool::runAdbCommand(const QString& serial, const QString& cmd)
{
	QProcess proc;
	proc.setProgram(m_adbPath);

	// 将命令拆分
	QStringList args = cmd.split(' ', Qt::SkipEmptyParts);
	args.insert(0, "-s");
	args.insert(1, serial);

	proc.setArguments(args);
	proc.start();
	proc.waitForFinished();

	return QString::fromLocal8Bit(proc.readAllStandardOutput());
}

void AdbHelperTool::appendLog(const QString& s)
{
	QString t = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
	ui.logView->append(QString("[%1] %2").arg(t, s));
}

/********************************** 设备管理页面 **************************************************/
void AdbHelperTool::refreshDeviceList()
{
	ui.listWidget->clear();

	//std::vector<DeviceInfo> vecDeviceInfos =  getAdbDeviceList();

	// 更新设备切换
	ui.deviceSwitchBox->clear();
	for (const auto& iter : vecDeviceInfos)
	{
		QListWidgetItem* item = new QListWidgetItem(ui.listWidget);
		item->setSizeHint(QSize(500, 45));

		DeviceItemWidget* itemWidget = new DeviceItemWidget();
		ui.listWidget->addItem(item);
		ui.listWidget->setItemWidget(item, itemWidget);
		int index = ui.listWidget->row(item);
		itemWidget->setIndex(index);

		itemWidget->setDeviceInfo(iter);

		QString displayText = iter.name + " (" + iter.serial + ")";
		ui.deviceSwitchBox->addItem(displayText, iter.serial); // userData = serial
		//ui.deviceSwitchBox->addItem(QStringLiteral("全部设备"));	

		//手动勾选
		connect(itemWidget, &DeviceItemWidget::sigChangeCheck, this, &AdbHelperTool::slotChangeCheck);
	}

	// 更新连接设备数量
	ui.label_2->setText(QString::number(ui.listWidget->count()));


	//更新状态栏时间
	QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
	statusBar->showMessage(
		QStringLiteral("版本 %1 | 上次刷新：%2").arg(versionOutput, timeStr)
	);
	
	if (ui.deviceSwitchBox->count() > 0)
	{
		ui.deviceSwitchBox->setCurrentIndex(0);
		//slotSwitchDevice(0);
	}
}

void AdbHelperTool::slotSelectAll()
{
	m_vecDevices.clear();
	int count = ui.listWidget->count();
	for (int i = 0; i < count; ++i)
	{
		QListWidgetItem* item = ui.listWidget->item(i);
		DeviceItemWidget* widget = qobject_cast<DeviceItemWidget*>(ui.listWidget->itemWidget(item));
		if (widget)
		{
			widget->setCheckState(true);
			m_vecDevices.push_back(widget->getDeviceInfo());
		}
	}
}

void AdbHelperTool::slotCancelSelectAll()
{
	m_vecDevices.clear();
	int count = ui.listWidget->count();
	for (int i = 0; i < count; ++i)
	{
		QListWidgetItem* item = ui.listWidget->item(i);
		DeviceItemWidget* widget = qobject_cast<DeviceItemWidget*>(ui.listWidget->itemWidget(item));
		if (widget)
		{
			widget->setCheckState(false);
		}
	}
}

void AdbHelperTool::slotChangeCheck(int index, bool checked)
{
	// 获取对应的 DeviceItemWidget
	QListWidgetItem* item = ui.listWidget->item(index);
	if (!item) return;

	DeviceItemWidget* widget = qobject_cast<DeviceItemWidget*>(ui.listWidget->itemWidget(item));
	if (!widget) return;

	const DeviceInfo& devInfo = widget->getDeviceInfo();

	if (checked)
	{
		// 勾选：如果未在 vecDeviceInfos 中则添加
		auto it = std::find_if(m_vecDevices.begin(), m_vecDevices.end(),
			[&](const DeviceInfo& d) { return d.serial == devInfo.serial; });
		if (it == m_vecDevices.end())
			m_vecDevices.push_back(devInfo);
	}
	else
	{
		// 取消勾选：从 vecDeviceInfos 中移除
		auto it = std::remove_if(m_vecDevices.begin(), m_vecDevices.end(),
			[&](const DeviceInfo& d) { return d.serial == devInfo.serial; });
		if (it != m_vecDevices.end())
			m_vecDevices.erase(it, m_vecDevices.end());
	}

	// 可选：输出当前选中数量，用于调试
	qDebug() << "当前选中设备数量:" << m_vecDevices.size();
	
}

void AdbHelperTool::slotRebootDeviceBth()
{
	//auto checkedDevices = getCheckedDeviceList();
	if (m_vecDevices.empty()) {
		QMessageBox::information(this, "提示", "请先勾选需要重启的设备");
		return;
	}

	appendLog(QString("开始重启 %1 台设备...").arg(m_vecDevices.size()));

	for (const auto& dev : m_vecDevices)
	{
		QProcess* process = new QProcess(this);

		QStringList args;
		args << "-s" << dev.serial << "reboot";

		connect(process,
			QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this,
			[this, dev, process](int exitCode, QProcess::ExitStatus) {
				if (exitCode == 0) {
					appendLog(QString("设备 %1 重启成功").arg(dev.serial));
				}
				else {
					appendLog(QString("设备 %1 重启失败 (exit=%2)").arg(dev.serial).arg(exitCode));
				}
				process->deleteLater();
			});


		process->start(m_adbPath, args);
	}
}

/********************************** 批量操作页面 **************************************************/

void AdbHelperTool::onBrowseApk()
{
	QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 APK 文件"), "", QStringLiteral("APK 文件 (*.apk)"));
	if (!file.isEmpty())
		ui.leApkPath->setText(file);
}

void AdbHelperTool::onBatchInstall()
{
	QString apk = ui.leApkPath->text().trimmed();
	if (apk.isEmpty()) return;
	if (vecDeviceInfos.empty()) {
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
		return;
	}

	updateProgressLog();
	appendLog(QStringLiteral("开始 %1 台设备安装应用...").arg(vecDeviceInfos.size()));
	appendProgressLog(QStringLiteral("开始 %1 台设备安装应用...").arg(vecDeviceInfos.size()));

	for (const auto& dev : vecDeviceInfos)
	{
		QStringList args = { "-s", dev.serial, "install", "-r", apk };
		runAdbCommandWithLog(dev, args, QStringLiteral("安装中..."));
	}
}

void AdbHelperTool::onBatchUninstall()
{
	QString pkg = ui.lePackageName->text().trimmed();
	if (pkg.isEmpty()) return;
	if (m_vecDevices.empty()) {
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
		return;
	}

	appendLog(QStringLiteral("开始 %1 台设备卸载应用...").arg(vecDeviceInfos.size()));
	appendProgressLog(QStringLiteral("开始 %1 台设备卸载应用...").arg(vecDeviceInfos.size()));

	updateProgressLog();

	for (const auto& dev : vecDeviceInfos)
	{
		QStringList args = { "-s", dev.serial, "uninstall", pkg };
		runAdbCommandWithLog(dev, args, QStringLiteral("卸载中..."));
	}
}

void AdbHelperTool::onBrowsePushLocal()
{
	QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择要推送的文件"));
	if (!file.isEmpty())
		ui.pushLocalFileEdit->setText(file);
}

void AdbHelperTool::onBatchPush()
{
	QString local = ui.pushLocalFileEdit->text().trimmed();
	QString remote = ui.pushRemotFileEdit->text().trimmed();
	if (local.isEmpty() || remote.isEmpty()) return;

	if (m_vecDevices.empty()) {
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
		return;
	}

	updateProgressLog();
	appendLog(QStringLiteral("开始 %1 台设备推送文件...").arg(vecDeviceInfos.size()));
	appendProgressLog(QStringLiteral("开始 %1 台设备推送文件...").arg(vecDeviceInfos.size()));

	for (const auto& dev : vecDeviceInfos)
	{
		QStringList args = { "-s", dev.serial, "push", local, remote };
		runAdbCommandWithLog(dev, args, QStringLiteral("推送中..."));
	}
}

void AdbHelperTool::onBrowsePullLocal()
{
	QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择保存目录"));
	if (!dir.isEmpty())
		ui.pullLocalFileEdit->setText(dir);
}

void AdbHelperTool::onBatchPull()
{
	QString local = ui.pullLocalFileEdit->text().trimmed();
	QString remote = ui.pullRemotFileEdit->text().trimmed();
	if (local.isEmpty() || remote.isEmpty()) return;

	if (m_vecDevices.empty()) {
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
		return;
	}

	updateProgressLog();
	appendLog(QStringLiteral("开始 %1 台设备拉取文件...").arg(vecDeviceInfos.size()));
	appendProgressLog(QStringLiteral("开始 %1 台设备拉取文件...").arg(vecDeviceInfos.size()));

	for (const auto& dev : vecDeviceInfos)
	{
		QStringList args = { "-s", dev.serial, "pull", remote, local };
		runAdbCommandWithLog(dev, args, QStringLiteral("拉取中..."));
	}
}

void AdbHelperTool::runAdbCommandWithLog(const DeviceInfo& dev,const QStringList& args,const QString& runningText)
{
	appendLog(QString("[%1] %2").arg(dev.serial, runningText));
	appendProgressLog(QString("[%1] %2").arg(dev.serial, runningText));

	QProcess* proc = new QProcess(this);
	m_runningProcesses[dev.serial] = proc;

	connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
		this, [=](int exitCode, QProcess::ExitStatus status)
		{
			if (status == QProcess::CrashExit)
			{
				appendLog(QString("[%1] %2").arg(dev.serial, QStringLiteral("失败：进程崩溃")));
				appendProgressLog(QString("[%1] %2").arg(dev.serial, QStringLiteral("失败：进程崩溃")));
			}
			else if (exitCode == 0)
			{
				appendLog(QString("[%1] %2").arg(dev.serial, QStringLiteral("成功")));
				appendProgressLog(QString("[%1] %2").arg(dev.serial, QStringLiteral("成功")));
			}
			else {
				appendLog(QString("[%1] %2 exit=%3").arg(dev.serial, QStringLiteral("失败"), QString::number(exitCode)));
				appendProgressLog(QString("[%1] %2 exit=%3").arg(dev.serial, QStringLiteral("失败"), QString::number(exitCode)));
			}

			proc->deleteLater();
			m_runningProcesses.remove(dev.serial);
		});

	proc->start("adb", args);
}

void AdbHelperTool::updateProgressLog()
{
	ui.progressEdit->clear();   // 清空日志
	m_runningProcesses.clear();
}

void AdbHelperTool::appendProgressLog(const QString& log)
{
	ui.progressEdit->append(log);
}

/********************************** 单设备操作页面 *********************************/

void AdbHelperTool::slotChangeDevice(int index)
{
	if (index < 0 || index >= vecDeviceInfos.size())
		return;

	m_dev = vecDeviceInfos[index];

	loadAppList(m_dev.serial);
}

void AdbHelperTool::loadAppList( QString serial)
{
	ui.listApp->clear();
	currentAppList.clear();

	QStringList args;
	args << "-s" << serial << "shell" << "pm" << "list" << "packages" << "-f";

	QString out = runAdbAndGetOutput(args);  // 返回 adb 输出字符串

	QStringList lines = out.split("\n", Qt::SkipEmptyParts);

	for (QString& line : lines)
	{
		bool isSystem = line.contains("/system/");

		// line 格式示例: package:/data/app/com.example.test-1/base.apk=com.example.test
		QString pkg = line.section('=', 1, 1).trimmed();
		QString appPath = line.section('=', 0, 0).mid(QString("package:").length()).trimmed();
		QString appName = QFileInfo(appPath).baseName();  // 用 APK 文件名做简单名称

		currentAppList.push_back({ appName, pkg, isSystem });
	}

	filterAppList();
}

void AdbHelperTool::filterAppList()
{
	ui.listApp->clear();

	bool showSystem = ui.cbSystemApp->isChecked();
	bool showThird = ui.cbThirdParty->isChecked();
	QString key = ui.leSearch->text().trimmed();

	for (auto& app : currentAppList)
	{
		if (app.systemApp && !showSystem) continue;
		if (!app.systemApp && !showThird) continue;
		if (!key.isEmpty() && !app.packageName.contains(key, Qt::CaseInsensitive)) continue;

		ui.listApp->addItem(app.packageName);
	}
}

QString AdbHelperTool::runAdbAndGetOutput(const QStringList& args)
{
	QString adbExe = m_adbPath.isEmpty() ? "adb" : m_adbPath;

	QProcess process;
	process.setProgram(adbExe);
	process.setArguments(args);

	process.start();

	if (!process.waitForStarted(3000)) {
		appendLog(QStringLiteral("adb 启动失败: ") + adbExe);
		return "";
	}

	if (!process.waitForFinished(10000)) {
		appendLog(QStringLiteral("adb 执行超时"));
		process.kill();
		return "";
	}

	QString out = QString::fromLocal8Bit(process.readAllStandardOutput());
	QString err = QString::fromLocal8Bit(process.readAllStandardError());

	if (!err.trimmed().isEmpty()) {
		appendLog(QStringLiteral("adb 错误: ") + err);
	}

	return out.trimmed();
}

void AdbHelperTool::slotAppSelected()
{
	bool has = ui.listApp->currentItem() != nullptr;

	ui.btnUninstallApp->setEnabled(has);
	ui.btnForceStop->setEnabled(has);
}

void AdbHelperTool::slotInstallApk()
{
	QString apk = QFileDialog::getOpenFileName(this, QStringLiteral("选择 APK"), "", "*.apk");
	if (apk.isEmpty()) return;

	QString serial = ui.deviceSwitchBox->currentData().toString(); // 修正点

	runAdb("install -r \"" + apk + "\"", serial);

}

void AdbHelperTool::slotUninstall()
{
	if (!ui.listApp->currentItem()) return;

	QString pkg = ui.listApp->currentItem()->text();
	QString serial = ui.deviceSwitchBox->currentData().toString();

	runAdb("uninstall " + pkg, serial);
}

void AdbHelperTool::slotForceStop()
{
	if (!ui.listApp->currentItem()) return;

	QString pkg = ui.listApp->currentItem()->text();
	QString serial = ui.deviceSwitchBox->currentData().toString();

	runAdb("shell am force-stop " + pkg, serial);
}


QString AdbHelperTool::runAdb(const QString& cmd, const QString& serial)
{
	QProcess process;
	QStringList args;

	// 指定设备
	if (!serial.isEmpty()) {
		args << "-s" << serial;
	}

	// 拆分命令字符串，例如 "install -r xxx.apk"
	args += QProcess::splitCommand(cmd);

	process.start("adb", args);
	process.waitForFinished();

	QString out = QString::fromLocal8Bit(process.readAllStandardOutput());
	QString err = QString::fromLocal8Bit(process.readAllStandardError());

	if (!err.isEmpty())
		qDebug() << "[ADB ERROR]" << err;

	return out + err;
}



