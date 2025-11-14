#include "AdbHelperTool.h"
#include <QMessageBox>
#include <QDateTime>
#include <QProcess>
#include <QFile>
#include"DeviceItemWidget.h"

AdbHelperTool::AdbHelperTool(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // 1. 窗口基础设置
    //setCentralWidget(centralWidget);
    resize(720, 480);
    setWindowTitle(QStringLiteral("ADB 智能助手 - Demo"));

    // 2. 底部状态栏
    QStatusBar* statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage(QStringLiteral("adb 版本 1.0.0 | 上次刷新：--"));

    connectSlots();

    detectAdbPath();
}

AdbHelperTool::~AdbHelperTool()
{}

void AdbHelperTool::connectSlots()
{
    connect(ui.btnBatchRefresh, &QPushButton::clicked, this, &AdbHelperTool::refreshDeviceList);
    connect(ui.selectAllBth, &QPushButton::clicked, this, &AdbHelperTool::slotSelectAll);
    connect(ui.cancelSelectBth, &QPushButton::clicked, this, &AdbHelperTool::slotCancelSelectAll);
}


// 刷新设备列表（核心逻辑）
void AdbHelperTool::refreshDeviceList()
{
    ui.listWidget->clear();

    std::vector<DeviceInfo> vecDeviceInfos =  getAdbDeviceList();

    for (auto iter : vecDeviceInfos)
    {
        QListWidgetItem* item = new QListWidgetItem(ui.listWidget);
        item->setSizeHint(QSize(500, 45));

        DeviceItemWidget* itemWidget = new DeviceItemWidget();
        ui.listWidget->addItem(item);
        ui.listWidget->setItemWidget(item, itemWidget);
        int index = ui.listWidget->row(item);
        itemWidget->setIndex(index);

        itemWidget->setDeviceInfo(iter);

        //手动勾选
        connect(itemWidget, &DeviceItemWidget::sigChangeCheck, this, &AdbHelperTool::slotChangeCheck);
    }
}

void AdbHelperTool::slotSelectAll()
{
   
}

void AdbHelperTool::slotCancelSelectAll()
{
   
}

void AdbHelperTool::slotChangeCheck()
{
   
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

void AdbHelperTool::detectAdbPath()
{
    appendLog(QStringLiteral("检测 adb..."));
    QProcess p;
    QString program = "adb";
    p.start(program, QStringList() << "version");
    if (!p.waitForStarted(2000)) {
        appendLog(QStringLiteral("无法启动 adb: ") + program);
        return;
    }
    if (!p.waitForFinished(3000)) {
        appendLog(QStringLiteral("adb 检测超时"));
        p.kill();
        return;
    }
    QString out = p.readAllStandardOutput();
    QString adbPath;

    QStringList lines = out.split('\n', Qt::SkipEmptyParts);
    for (QString line : lines) {
        line = line.trimmed();
        if (line.startsWith("Installed as")) {
            adbPath = line.mid(QString("Installed as").length()).trimmed();
            break;
        }
    }

    if (adbPath.isEmpty()) {
        appendLog(QStringLiteral("未检测到系统 adb，尝试使用软件内置 adb"));

        // 1️⃣ 获取当前程序路径
        QString appDir = QCoreApplication::applicationDirPath();

        // 2️⃣ 拼接内置 adb 路径
        QString internalAdbPath = appDir + "/platform-tools/adb.exe";

        if (QFile::exists(internalAdbPath)) {
            appendLog(QStringLiteral("使用内置 adb: ") + internalAdbPath);
            m_adbPath = internalAdbPath;
        }
        else {
            appendLog(QStringLiteral("未找到内置 adb: ") + internalAdbPath);
            m_adbPath ="";
        }
    }
    else {
        m_adbPath= adbPath;
    }
    appendLog("adb version: " + out.trimmed());
}
