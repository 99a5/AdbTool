#include "AdbHelperTool.h"
#include "DeviceItemWidget.h"

#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>

// 超时时间常量（ms）
static const int ADB_START_TIMEOUT = 3000;
static const int ADB_FINISH_TIMEOUT = 60000; // 60s，安装大包时可能需要更长

AdbHelperTool::AdbHelperTool(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    resize(720, 480);
    setWindowTitle(QStringLiteral("ADB 智能助手 - Demo"));

    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage(QStringLiteral("adb 版本 1.0.0 | 上次刷新：--"));

    // 如果你希望启动时读取真实设备，可以取消下面测试数据
    // --- 测试数据（可删除） ---
    vecDeviceInfos.clear();
    DeviceInfo deviceInfo{ QStringLiteral("设备1"), QStringLiteral("Android 10"), "1234567890" };
    DeviceInfo deviceInfo2{ QStringLiteral("设备2"),  QStringLiteral("Android 10"), "12345678901" };
    DeviceInfo deviceInfo3{ QStringLiteral("设备3"),  QStringLiteral("Android 10") , "12345678902" };
    vecDeviceInfos.push_back(deviceInfo);
    vecDeviceInfos.push_back(deviceInfo2);
    vecDeviceInfos.push_back(deviceInfo3);
    // -------------------------

    connectSlots();

    detectAdbPath();

}

AdbHelperTool::~AdbHelperTool()
{
    // 确保释放正在运行的进程
    for (auto proc : m_runningProcess) {
        if (proc && proc->state() != QProcess::NotRunning) {
            proc->kill();
            proc->waitForFinished(1000);
        }
        delete proc;
    }
    m_runningProcess.clear();
}

void AdbHelperTool::initUI()
{
}

QString AdbHelperTool::runAdb(const QString& serial, const QString& cmd)
{
    QString adbExe = m_adbPath.isEmpty() ? "adb" : m_adbPath;

    QStringList args = QProcess::splitCommand(cmd);
    if (!serial.isEmpty()) {
        args.prepend(serial);
        args.prepend("-s");
    }

    QProcess proc;
    proc.setProgram(adbExe);
    proc.setArguments(args);

    proc.start();
    if (!proc.waitForStarted(ADB_START_TIMEOUT)) {
        appendLog(QStringLiteral("ADB 启动失败: %1").arg(adbExe));
        return {};
    }

    if (!proc.waitForFinished(ADB_FINISH_TIMEOUT)) {
        appendLog("ADB 执行超时，已终止");
        proc.kill();
        return {};
    }

    QString stdoutText = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    QString stderrText = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();

    if (!stderrText.isEmpty())
        appendLog(QStringLiteral("ADB 错误输出: %1").arg(stderrText));

    return stdoutText;
}

QString AdbHelperTool::runAdbRaw(const QStringList& args)
{
    QString adbExe = m_adbPath.isEmpty() ? "adb" : m_adbPath;

    QProcess proc;
    proc.setProgram(adbExe);
    proc.setArguments(args);

    proc.start();
    if (!proc.waitForStarted(ADB_START_TIMEOUT)) {
        appendLog(QStringLiteral("ADB 启动失败: %1").arg(adbExe));
        return {};
    }

    if (!proc.waitForFinished(ADB_FINISH_TIMEOUT)) {
        appendLog(QStringLiteral("ADB 执行超时"));
        proc.kill();
        return {};
    }

    QString stdoutText = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    QString stderrText = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();

    if (!stderrText.isEmpty())
        appendLog(QStringLiteral("ADB 错误输出: %1").arg(stderrText));

    return stdoutText;
}

void AdbHelperTool::runAdbAsync(const DeviceInfo& dev,const QStringList& args,const QString& runningText)
{
    QString adbExe = m_adbPath.isEmpty() ? "adb" : m_adbPath;

    appendLog(QString("[%1] %2 ...").arg(dev.serial, runningText));

    QProcess* proc = new QProcess(this);

    // 用 serial 作为 key → 保证每台设备只有一个运行进程
    if (m_runningProcess.contains(dev.serial)) {
        appendLog(QString("[%1] 正在执行其他操作，已跳过").arg(dev.serial));
        delete proc;
        return;
    }
    m_runningProcess[dev.serial] = proc;

    // 捕获日志
    connect(proc, &QProcess::readyReadStandardOutput, this, [=]() {
        QString out = QString::fromLocal8Bit(proc->readAllStandardOutput()).trimmed();
        if (!out.isEmpty())
            appendLog(QString("[%1 OUT] %2").arg(dev.serial, out));
        });

    connect(proc, &QProcess::readyReadStandardError, this, [=]() {
        QString err = QString::fromLocal8Bit(proc->readAllStandardError()).trimmed();
        if (!err.isEmpty())
            appendLog(QString("[%1 ERR] %2").arg(dev.serial, err));
        });

    // 完成回调
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [=](int exitCode, QProcess::ExitStatus status) {

            if (status == QProcess::CrashExit) {
                appendLog(QString("[%1] 失败：进程崩溃").arg(dev.serial));
            }
            else if (exitCode == 0) {
                appendLog(QString("[%1] 成功").arg(dev.serial));
            }
            else {
                appendLog(QString("[%1] 失败 (exit=%2)").arg(dev.serial).arg(exitCode));
            }

            m_runningProcess.remove(dev.serial);
            proc->deleteLater();
        });

    // 启动
    proc->start(adbExe, args);
}

void AdbHelperTool::appendLog(const QString& s)
{
    QString t = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    if (ui.logView)
        ui.logView->append(QString("[%1] %2").arg(t, s));
    qDebug() << s;
}

void AdbHelperTool::connectSlots()
{
    connect(ui.btnBatchRefresh, &QPushButton::clicked, this, &AdbHelperTool::onRefreshDevices);
    connect(ui.selectAllBth, &QPushButton::clicked, this, &AdbHelperTool::onSelectAllDevices);
    connect(ui.cancelSelectBth, &QPushButton::clicked, this, &AdbHelperTool::onUnselectAllDevices);
    connect(ui.rebootDeviceBth, &QPushButton::clicked, this, &AdbHelperTool::onBatchReboot);

    connect(ui.btnBrowseApk, &QPushButton::clicked, this, &AdbHelperTool::onBrowseApk);
    connect(ui.btnBatchInstall, &QPushButton::clicked, this, &AdbHelperTool::onBatchInstall);
    connect(ui.btnBatchUninstall, &QPushButton::clicked, this, &AdbHelperTool::onBatchUninstall);
    connect(ui.pushLocalFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBrowsePushFile);
    connect(ui.batchPushFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBatchPush);
    connect(ui.pullLocalFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBrowsePullDir);
    connect(ui.batchPullFileBth, &QPushButton::clicked, this, &AdbHelperTool::onBatchPull);

    connect(ui.deviceSwitchBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentDeviceChanged(int)));

    connect(ui.cbSystemApp, &QCheckBox::stateChanged, this, &AdbHelperTool::filterAppList);
    connect(ui.cbThirdParty, &QCheckBox::stateChanged, this, &AdbHelperTool::filterAppList);
    connect(ui.leSearch, &QLineEdit::textChanged, this, &AdbHelperTool::filterAppList);
    connect(ui.listApp, &QListWidget::itemSelectionChanged, this, &AdbHelperTool::onAppSelected);


    connect(ui.runAdbBth, &QPushButton::clicked, this, &AdbHelperTool::onRunCustomCommand);
    connect(ui.startTerminalBth, &QPushButton::clicked, this, &AdbHelperTool::onStartTerminal);
    connect(ui.logExportBth, &QPushButton::clicked, this, &AdbHelperTool::onLogExport);



    ui.dragAreaEdit->setAcceptDrops(true);
    ui.dragAreaEdit->installEventFilter(this);
}

std::vector<DeviceInfo> AdbHelperTool::getCheckedDevices()
{
    std::vector<DeviceInfo> result;

    int count = ui.listWidget->count();
    for (int i = 0; i < count; ++i)
    {
        QListWidgetItem* item = ui.listWidget->item(i);
        DeviceItemWidget* widget = qobject_cast<DeviceItemWidget*>(ui.listWidget->itemWidget(item));

        if (widget && widget->checkState())
            result.push_back(widget->getDeviceInfo());
    }

    return result;
}

void AdbHelperTool::setDeviceOnlineState(const DeviceInfo& dev, bool ok)
{
}

void AdbHelperTool::detectAdbPath()
{
    appendLog(QStringLiteral("检测 adb..."));

    QString adbPath;
    QString version;

    // 尝试系统 adb
    {
        QProcess p;
        p.start("adb", QStringList() << "version");
        if (p.waitForStarted(ADB_START_TIMEOUT) && p.waitForFinished(ADB_FINISH_TIMEOUT)) {
            QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
            if (!out.isEmpty()) {
                version = out.split('\n', Qt::SkipEmptyParts).value(0).trimmed();
                // 不能可靠得到 "Installed as" 路径，留空让后续用 m_adbPath
            }
        }
    }

    // 如果没有发现系统 adb，则尝试内置 adb
    if (version.isEmpty()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString internalAdbPath = appDir + "/platform-tools/adb.exe";
        if (QFile::exists(internalAdbPath)) {
            adbPath = internalAdbPath;
            QProcess proc;
            proc.start(adbPath, QStringList() << "version");
            if (proc.waitForStarted(ADB_START_TIMEOUT) && proc.waitForFinished(ADB_FINISH_TIMEOUT)) {
                version = QString::fromLocal8Bit(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts).value(0).trimmed();
            }
        }
    }

    m_adbPath = adbPath; // 如果为空，后续调用会回退到系统 adb("adb")
    versionOutput = version.isEmpty() ? QStringLiteral("未知") : version;

    appendLog(QStringLiteral("adb version: ") + versionOutput);

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    statusBar->showMessage(QStringLiteral("版本 %1 | 上次刷新：%2").arg(versionOutput, timeStr));
}

void AdbHelperTool::updateDeviceList()
{
}

std::vector<DeviceInfo> AdbHelperTool::getAdbDeviceList()
{
    std::vector<DeviceInfo> devices;

    QString adbExe = m_adbPath.isEmpty() ? "adb" : m_adbPath;
    QProcess process;
    process.setProgram(adbExe);
    process.setArguments({ "devices" });
    process.start();
    if (!process.waitForStarted(ADB_START_TIMEOUT)) {
        appendLog(QStringLiteral("adb 启动失败: %1").arg(adbExe));
        return devices;
    }
    if (!process.waitForFinished(5000)) {
        appendLog(QStringLiteral("adb devices 超时"));
        process.kill();
        return devices;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        if (line.contains("List of devices")) continue;
        if (!line.contains("\tdevice")) continue;

        QString serial = line.section('\t', 0, 0).trimmed();
        if (serial.isEmpty()) continue;

        DeviceInfo info;
        info.serial = serial;
        info.name = runAdb(serial, "shell getprop ro.product.model").trimmed();
        if (info.name.isEmpty()) info.name = "Unknown";
        info.system = runAdb(serial, "shell getprop ro.build.version.release").trimmed();
        if (info.system.isEmpty()) info.system = "Unknown";

        devices.push_back(info);
    }

    return devices;
}


/********************************** 设备管理页面 **************************************************/

void AdbHelperTool::onRefreshDevices()
{
    ui.listWidget->clear();
    ui.deviceSwitchBox->clear();

    // 真实拉取设备列表（注：如果你希望保留测试数据，请注释掉下面这行）
    // vecDeviceInfos = getAdbDeviceList();

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
        ui.deviceSwitchBox->addItem(displayText, iter.serial);
        ui.deviceCombo->addItem(displayText, iter.serial);

        connect(itemWidget, &DeviceItemWidget::sigChangeCheck, this, &AdbHelperTool::onDeviceCheckStateChanged);
    }

    ui.label_2->setText(QString::number(ui.listWidget->count()));

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    statusBar->showMessage(QStringLiteral("版本 %1 | 上次刷新：%2").arg(versionOutput, timeStr));

    if (ui.deviceSwitchBox->count() > 0)
        ui.deviceSwitchBox->setCurrentIndex(0);

    if (ui.deviceSwitchBox->count() > 0)
        ui.deviceCombo->setCurrentIndex(0);
}

void AdbHelperTool::onSelectAllDevices()
{
    m_allDevices.clear();
    int count = ui.listWidget->count();
    for (int i = 0; i < count; ++i)
    {
        QListWidgetItem* item = ui.listWidget->item(i);
        DeviceItemWidget* widget = qobject_cast<DeviceItemWidget*>(ui.listWidget->itemWidget(item));
        if (widget)
        {
            widget->setCheckState(true);
            m_allDevices.push_back(widget->getDeviceInfo());
        }
    }
}

void AdbHelperTool::onUnselectAllDevices()
{
    m_allDevices.clear();
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

void AdbHelperTool::onDeviceCheckStateChanged(int index, bool checked)
{
    QListWidgetItem* item = ui.listWidget->item(index);
    if (!item) return;

    DeviceItemWidget* widget = qobject_cast<DeviceItemWidget*>(ui.listWidget->itemWidget(item));
    if (!widget) return;

    const DeviceInfo& devInfo = widget->getDeviceInfo();

    if (checked) {
        auto it = std::find_if(m_allDevices.begin(), m_allDevices.end(),
            [&](const DeviceInfo& d) { return d.serial == devInfo.serial; });
        if (it == m_allDevices.end())
            m_allDevices.push_back(devInfo);
    }
    else {
        auto it = std::remove_if(m_allDevices.begin(), m_allDevices.end(),
            [&](const DeviceInfo& d) { return d.serial == devInfo.serial; });
        if (it != m_allDevices.end())
            m_allDevices.erase(it, m_allDevices.end());
    }

    qDebug() << "当前选中设备数量:" << m_allDevices.size();
}

void AdbHelperTool::onBatchReboot()
{
    if (m_allDevices.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选需要重启的设备"));
        return;
    }

    appendLog(QStringLiteral("开始重启 %1 台设备...").arg(m_allDevices.size()));

    for (const auto& dev : m_allDevices)
    {
        QProcess* process = new QProcess(this);
        m_runningProcess[dev.serial] = process;

        QString adbExe = m_adbPath.isEmpty() ? "adb" : m_adbPath;
        QStringList args = { "-s", dev.serial, "reboot" };

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, dev, process](int exitCode, QProcess::ExitStatus status) {
                if (status == QProcess::CrashExit) {
                    appendLog(QString("设备 %1 重启失败：进程崩溃").arg(dev.serial));
                }
                else if (exitCode == 0) {
                    appendLog(QString("设备 %1 重启成功").arg(dev.serial));
                }
                else {
                    appendLog(QString("设备 %1 重启失败 (exit=%2)").arg(dev.serial).arg(exitCode));
                }
                process->deleteLater();
                m_runningProcess.remove(dev.serial);
            });

        process->start(adbExe, args);
    }
}

/********************************** 批量操作页面 **************************************************/

void AdbHelperTool::onBrowseApk()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 APK 文件"), "", QStringLiteral("APK 文件 (*.apk)"));
    if (!file.isEmpty()) ui.leApkPath->setText(file);
}

void AdbHelperTool::onBatchInstall()
{
    QString apk = ui.leApkPath->text().trimmed();
    if (apk.isEmpty()) return;
    if (m_allDevices.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
        return;
    }

    appendLog(QStringLiteral("开始 %1 台设备安装应用...").arg(m_allDevices.size()));

    m_runningProcess.clear();

    for (const auto& dev : m_allDevices)
    {
        QStringList args = { "-s", dev.serial, "install", "-r", apk };
        runAdbAsync(dev, args, QStringLiteral("安装中..."));
    }
}

void AdbHelperTool::onBatchUninstall()
{
    QString pkg = ui.lePackageName->text().trimmed();
    if (pkg.isEmpty()) return;
    if (m_allDevices.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
        return;
    }

    appendLog(QStringLiteral("开始 %1 台设备卸载应用...").arg(m_allDevices.size()));
    m_runningProcess.clear();

    for (const auto& dev : m_allDevices)
    {
        QStringList args = { "-s", dev.serial, "uninstall", pkg };
        runAdbAsync(dev, args, QStringLiteral("卸载中..."));
    }
}

void AdbHelperTool::onBrowsePushFile()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择要推送的文件"));
    if (!file.isEmpty()) ui.pushLocalFileEdit->setText(file);
}

void AdbHelperTool::onBatchPush()
{
    QString local = ui.pushLocalFileEdit->text().trimmed();
    QString remote = ui.pushRemotFileEdit->text().trimmed();
    if (local.isEmpty() || remote.isEmpty()) return;
    if (m_allDevices.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
        return;
    }

    appendLog(QStringLiteral("开始 %1 台设备推送文件...").arg(m_allDevices.size()));
    m_runningProcess.clear();

    for (const auto& dev : m_allDevices)
    {
        QStringList args = { "-s", dev.serial, "push", local, remote };
        runAdbAsync(dev, args, QStringLiteral("推送中..."));
    }
}

void AdbHelperTool::onBrowsePullDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择保存目录"));
    if (!dir.isEmpty()) ui.pullLocalFileEdit->setText(dir);
}

void AdbHelperTool::onBatchPull()
{
    QString local = ui.pullLocalFileEdit->text().trimmed();
    QString remote = ui.pullRemotFileEdit->text().trimmed();
    if (local.isEmpty() || remote.isEmpty()) return;
    if (m_allDevices.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先勾选设备"));
        return;
    }

    appendLog(QStringLiteral("开始 %1 台设备拉取文件...").arg(m_allDevices.size()));
    m_runningProcess.clear();

    for (const auto& dev : m_allDevices)
    {
        QStringList args = { "-s", dev.serial, "pull", remote, local };
        runAdbAsync(dev, args, QStringLiteral("拉取中..."));
    }
}


/********************************** 单设备操作页面 *********************************/

void AdbHelperTool::onCurrentDeviceChanged(int index)
{
    if (index < 0 || index >= vecDeviceInfos.size()) return;

    m_selectedDevices = vecDeviceInfos[index];
    loadInstalledApps(m_selectedDevices.serial);
}

void AdbHelperTool::loadInstalledApps(const QString& serial)
{
    ui.listApp->clear();
    m_currentAppList.clear();

    QStringList args;
    args << "-s" << serial << "shell" << "pm" << "list" << "packages" << "-f";

    QString out = runAdbRaw(args);
    QStringList lines = out.split('\n', Qt::SkipEmptyParts);

    for (QString& line : lines)
    {
        bool isSystem = line.contains("/system/");
        QString pkg = line.section('=', 1, 1).trimmed();
        QString appPath = line.section('=', 0, 0).mid(QString("package:").length()).trimmed();
        QString appName = QFileInfo(appPath).baseName();
        m_currentAppList.push_back({ appName, pkg, isSystem });
    }

    filterAppList();
}

void AdbHelperTool::filterAppList()
{
    ui.listApp->clear();

    bool showSystem = ui.cbSystemApp->isChecked();
    bool showThird = ui.cbThirdParty->isChecked();
    QString key = ui.leSearch->text().trimmed();

    for (auto& app : m_currentAppList)
    {
        if (app.systemApp && !showSystem) continue;
        if (!app.systemApp && !showThird) continue;
        if (!key.isEmpty() && !app.packageName.contains(key, Qt::CaseInsensitive)) continue;

        ui.listApp->addItem(app.packageName);
    }
}

void AdbHelperTool::onAppSelected()
{
    bool has = ui.listApp->currentItem() != nullptr;
    ui.btnUninstallApp->setEnabled(has);
    ui.btnForceStop->setEnabled(has);
}

void AdbHelperTool::onInstallSingle()
{
    QString apk = QFileDialog::getOpenFileName(this, QStringLiteral("选择 APK"), "", "*.apk");
    if (apk.isEmpty()) return;

    QString serial = ui.deviceSwitchBox->currentData().toString();
    runAdb("install -r \"" + apk + "\"", serial);
}

void AdbHelperTool::onUninstallSingle()
{
    if (!ui.listApp->currentItem()) return;
    QString pkg = ui.listApp->currentItem()->text();
    QString serial = ui.deviceSwitchBox->currentData().toString();
    runAdb("uninstall " + pkg, serial);
}

void AdbHelperTool::onForceStopSingle()
{
    if (!ui.listApp->currentItem()) return;
    QString pkg = ui.listApp->currentItem()->text();
    QString serial = ui.deviceSwitchBox->currentData().toString();
    runAdb("shell am force-stop " + pkg, serial);
}

/********************************** 工具页面 *********************************/

void AdbHelperTool::onRunCustomCommand()
{
    if (!ui.commandEdit) return; // 假设命令输入框叫 commandEdit

    QString serial = ui.deviceSwitchBox->currentData().toString(); // 获取选中设备
    QString text = ui.commandEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        appendLog(QStringLiteral("请输入命令"));
        return;
    }

    // 支持多行命令，按行执行
    QStringList cmdLines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString& cmd : cmdLines) {
        appendLog(QString(QStringLiteral("[%1] 执行命令: %2")).arg(serial.isEmpty() ? QStringLiteral("全部设备") : serial, cmd));

        QStringList args = QProcess::splitCommand(cmd);
        if (!serial.isEmpty()) {
            args.insert(0, "-s");
            args.insert(1, serial);
        }

        QProcess* proc = new QProcess(this);

        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus status) {
                QString result;
                if (status == QProcess::CrashExit) {
                    result = QString(QStringLiteral("[%1] 命令执行失败：进程崩溃")).arg(serial);
                }
                else if (exitCode == 0) {
                    result = QString(QStringLiteral("[%1] 命令执行成功")).arg(serial);
                }
                else {
                    result = QString(QStringLiteral("[%1] 命令执行失败，exit=%2")).arg(serial).arg(exitCode);
                }
                appendLog(result);
                proc->deleteLater();
            });

        proc->start(m_adbPath.isEmpty() ? "adb" : m_adbPath, args);
    }
}

void AdbHelperTool::onStartTerminal()
{
    // 1. 获取当前选择的设备序列号（你原 Python 里的 pid）
    QString serial;
    QString text;
    if (ui.deviceSwitchBox->currentIndex() >= 0) {
        serial = ui.deviceSwitchBox->currentData().toString(); // 获取选中设备
        text = ui.commandEdit->toPlainText().trimmed();
    }


    if (serial.isEmpty()) {
        appendLog("请先选择设备");
        return;
    }

    // 2. 查找 adb.exe
    QString adbPath = QStandardPaths::findExecutable("adb");
    if (adbPath.isEmpty()) {
        appendLog("无法找到 adb，请确认已配置到 PATH");
        return;
    }

    // 3. 查找 cmd.exe
    QString cmdPath = QStandardPaths::findExecutable("cmd.exe");
    if (cmdPath.isEmpty()) {
        appendLog("无法找到 cmd.exe");
        return;
    }

    // 4. 构造命令：进入 adb shell
    // 等价于 Python:  start adb -s <pid> shell
    //QString command = QString("%1 -s %2 shell").arg(adbPath, serial);
    QStringList args;
    args << "/C" << "start";
    //args << "/K" << command;   // /K 保持 cmd 打开并执行命令

    // 5. 启动终端
    bool ok = QProcess::startDetached(cmdPath, args);
    if (!ok) {
        appendLog("启动 adb 终端失败");
    }
}

void AdbHelperTool::onLogExport()
{
    // 1. 获取设备 serial
    QString serial;
    QString pname;
    if (ui.deviceSwitchBox->currentIndex() >= 0) {
        serial = ui.deviceSwitchBox->currentData().toString();
        pname = ui.deviceSwitchBox->currentText();
    }

    if (serial.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择设备！"));
        return;
    }

    // 2. 保存路径选择
    QString nowtime = QDateTime::currentDateTime().toString("yyyyMMddHH");
    QString defaultName = pname + QStringLiteral("_log_") + nowtime + QStringLiteral(".log");

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存日志到文件"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultName,
        QStringLiteral("日志文件 (*.log)")
    );

    if (fileName.isEmpty())
        return;

    // 3. 查找 adb
    QString adbPath = QStandardPaths::findExecutable("adb");
    if (adbPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("找不到 adb，请确认已配置 PATH"));
        return;
    }

    // 4. 查找 cmd.exe
    QString cmdPath = QStandardPaths::findExecutable("cmd.exe");
    if (cmdPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("找不到 cmd.exe"));
        return;
    }

    // 5. 启动实时查看窗口（等价 Python 的 cmd1）
    QString viewCmd = QStringLiteral("start \"%1\" -s %2 logcat -v time -s \"tencent\"")
        .arg(adbPath, serial);

    QProcess::startDetached(cmdPath, { QStringLiteral("/C"), viewCmd });

    // 6. 后台写文件（等价 Python 的 cmd2）
    QStringList args;
    args << "-s" << serial
        << "logcat" << "-v" << "time"
        << "-s" << "tencent";

    QProcess* logProcess = new QProcess(this);

    QFile* file = new QFile(fileName, logProcess);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法打开文件写入日志！"));
        return;
    }

    connect(logProcess, &QProcess::readyReadStandardOutput, this, [=]() {
        file->write(logProcess->readAllStandardOutput());
        file->flush();
        });

    connect(logProcess, &QProcess::readyReadStandardError, this, [=]() {
        file->write(logProcess->readAllStandardError());
        file->flush();
        });

    logProcess->start(adbPath, args);

    if (!logProcess->waitForStarted(2000)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("logcat 启动失败！"));
        return;
    }

    QMessageBox::information(
        this,
        QStringLiteral("成功"),
        QStringLiteral("日志开始抓取并保存到文件：\n%1\n\n关闭程序后自动停止抓取。").arg(fileName)
    );
}

