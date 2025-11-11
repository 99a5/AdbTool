#include "AdbTool.h"
#include <QtWidgets>
#include <QtCharts>
//#include <QZipWriter>
#include <QFile>

AdbTool::AdbTool(QWidget *parent)
    : QMainWindow(parent),
    adbProcess(new QProcess(this)),
    cpuTimer(new QTimer(this))
{
    ui.setupUi(this);

    initData();
    initUI();
    connectSlots();

    // try auto-detect on start
    onDetectAdb();

    // load history (very simple)
    if (QFile::exists(lastHistoryFile)) {
        QFile hf(lastHistoryFile);
        if (hf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(hf.readAll()).split('\n', Qt::SkipEmptyParts);
            for (auto& l : lines) appendLog("[HIST] " + l);
        }
    }
}

AdbTool::~AdbTool()
{
    cpuTimer->stop();
    if (adbProcess->state() == QProcess::Running) adbProcess->kill();

}

void AdbTool::initData()
{
    central = new QWidget(this);
    lay = new QVBoxLayout(central);

    topRow = new QHBoxLayout();
    leAdbPath = new QLineEdit("adb", this);

    btnDetect = new QPushButton(QStringLiteral("检测 ADB"), this);
    btnListDevices = new QPushButton(QStringLiteral("列出设备"), this);
    cbDevices = new QComboBox(this);

    apkRow = new QHBoxLayout();
    leApkPath = new QLineEdit(this);

    leApkPath->setPlaceholderText(QStringLiteral("拖入 APK 或点击选择..."));
    btnBrowseApk = new QPushButton(QStringLiteral("选择 APK"));
    btnInstall = new QPushButton(QStringLiteral("安装 APK"));
    lePackageName = new QLineEdit(this);

    btnStartApp = new QPushButton(QStringLiteral("启动 App"));

    ppRow = new QHBoxLayout();
    btnPush = new QPushButton(QStringLiteral("推送 文件 (Push)"));
    btnPull = new QPushButton(QStringLiteral("拉取 文件 (Pull)"));

    logRow = new QHBoxLayout();
    btnExportLog = new QPushButton(QStringLiteral("导出 logcat 并压缩"));

    monRow = new QHBoxLayout();
    cbMonitorProcess = new QComboBox(this);

    btnStartMon = new QPushButton(QStringLiteral("开始 CPU 监控"));
    btnStopMon = new QPushButton(QStringLiteral("停止 CPU 监控"));

    cpuSeries = new QLineSeries();
    chart = new QChart();

    axisX = new QValueAxis;
    axisY = new QValueAxis;

    logView = new QTextEdit(this);
    logView->setReadOnly(true);

    progressBar = new QProgressBar(this);
}

void AdbTool::initUI()
{

    leAdbPath->setToolTip(QStringLiteral("ADB 可执行文件路径，默认使用系统 PATH 中的 adb"));

    // ========= ADB 设置区域 =========
    QGroupBox* adbGroup = new QGroupBox(QStringLiteral("ADB 设置"), this);
    QGridLayout* adbLay = new QGridLayout(adbGroup);

    adbLay->addWidget(new QLabel(QStringLiteral("ADB Path:")), 0, 0);
    adbLay->addWidget(leAdbPath, 0, 1);
    adbLay->addWidget(btnDetect, 0, 2);

    adbLay->addWidget(new QLabel(QStringLiteral("设备:")), 1, 0);
    adbLay->addWidget(cbDevices, 1, 1);
    adbLay->addWidget(btnListDevices, 1, 2);


    // ========= APK 安装区域 =========
    QGroupBox* apkGroup = new QGroupBox(QStringLiteral("APK 安装与启动"), this);
    QGridLayout* apkLay = new QGridLayout(apkGroup);

    apkLay->addWidget(new QLabel(QStringLiteral("APK 文件:")), 0, 0);
    apkLay->addWidget(leApkPath, 0, 1);
    apkLay->addWidget(btnBrowseApk, 0, 2);
    apkLay->addWidget(btnInstall, 0, 3);

    apkLay->addWidget(new QLabel(QStringLiteral("Package/Main:")), 1, 0);
    apkLay->addWidget(lePackageName, 1, 1, 1, 2);
    apkLay->addWidget(btnStartApp, 1, 3);

    // ========= Push / Pull 区 =========
    QGroupBox* fileGroup = new QGroupBox(QStringLiteral("文件交互 Push / Pull"), this);
    QHBoxLayout* fileLay = new QHBoxLayout(fileGroup);
    fileLay->addWidget(btnPush);
    fileLay->addWidget(btnPull);

    // ========= 日志导出 =========
    QGroupBox* logGroup = new QGroupBox(QStringLiteral("日志导出"), this);
    QHBoxLayout* logLay = new QHBoxLayout(logGroup);
    logLay->addWidget(btnExportLog);

    // ========= CPU 监控区 =========
    QGroupBox* cpuGroup = new QGroupBox(QStringLiteral("CPU 监控"), this);
    QVBoxLayout* cpuLay = new QVBoxLayout(cpuGroup);

    QHBoxLayout* cpuCtrl = new QHBoxLayout();
    cpuCtrl->addWidget(new QLabel(QStringLiteral("进程:")));
    cbMonitorProcess->setEditable(true);
    cbMonitorProcess->setPlaceholderText("输入进程名或 pid");
    cpuCtrl->addWidget(cbMonitorProcess);
    cpuCtrl->addWidget(btnStartMon);
    cpuCtrl->addWidget(btnStopMon);

    cpuLay->addLayout(cpuCtrl);

    // 将图表放到 ChartView 中
    chartView = new QtCharts::QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    cpuLay->addWidget(chartView);

    // ========= 日志文本输出 =========
    QGroupBox* outGroup = new QGroupBox(QStringLiteral("输出日志"));
    QVBoxLayout* outLay = new QVBoxLayout(outGroup);
    outLay->addWidget(logView);
    outLay->addWidget(progressBar);

    // 将所有分组加入主布局
    lay->addWidget(adbGroup);
    lay->addWidget(apkGroup);
    lay->addWidget(fileGroup);
    lay->addWidget(logGroup);
    lay->addWidget(cpuGroup);
    lay->addWidget(outGroup);

    setCentralWidget(central);
    resize(800, 600);
    setWindowTitle(QStringLiteral("ADB 智能助手 - Demo"));
}

void AdbTool::connectSlots()
{
    connect(btnDetect, &QPushButton::clicked, this, &AdbTool::onDetectAdb);
    connect(btnListDevices, &QPushButton::clicked, this, &AdbTool::onListDevices);
    connect(btnBrowseApk, &QPushButton::clicked, [this]() {
        QString f = QFileDialog::getOpenFileName(this, QStringLiteral("选择 APK"), QString(), "APK Files (*.apk)");
        if (!f.isEmpty()) leApkPath->setText(f);
        });
    connect(btnInstall, &QPushButton::clicked, this, &AdbTool::onInstallApk);
    connect(btnPush, &QPushButton::clicked, this, &AdbTool::onPushFile);
    connect(btnPull, &QPushButton::clicked, this, &AdbTool::onPullFile);
    connect(btnStartApp, &QPushButton::clicked, this, &AdbTool::onStartApp);
    connect(btnExportLog, &QPushButton::clicked, this, &AdbTool::onExportLog);

    connect(btnStartMon, &QPushButton::clicked, this, &AdbTool::onStartCpuMonitor);
    connect(btnStopMon, &QPushButton::clicked, this, &AdbTool::onStopCpuMonitor);

    connect(adbProcess, &QProcess::readyReadStandardOutput, this, &AdbTool::readAdbOutput);
    connect(adbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, &AdbTool::adbFinished);

    connect(cpuTimer, &QTimer::timeout, this, &AdbTool::cpuPoll);
}



// --- drag/drop
void AdbTool::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}
void AdbTool::dropEvent(QDropEvent* event)
{
    auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString path = urls.first().toLocalFile();
        if (path.endsWith(".apk", Qt::CaseInsensitive)) {
            leApkPath->setText(path);
            appendLog("拖入 APK: " + path);
        }
        else {
            // store for push
            cbMonitorProcess->setEditText(path);
            appendLog("拖入文件: " + path);
        }
    }
}

// --- helpers
QString AdbTool::adbCommand(const QStringList& args)
{
    //QString aPath = leAdbPath->text().trimmed();
    //QString cmd = QProcess::programFilePath(); // not used
    // build full command display (for history)
    //QString full = aPath + " " + args.join(" ");
   // QFile hf(lastHistoryFile);
   // if (hf.open(QIODevice::Append | QIODevice::Text)) {
   //     hf.write(full.toUtf8() + "\n");
   // }
   // return full;
    return "";
}

void AdbTool::appendLog(const QString& s)
{
    QString t = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logView->append(QString("[%1] %2").arg(t, s));
}

// --- button slots
void AdbTool::onDetectAdb()
{
    appendLog(QStringLiteral("检测 adb..."));
    QProcess p;
    QString program = leAdbPath->text().trimmed();
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
    appendLog("adb version: " + out.trimmed());
}

void AdbTool::onListDevices()
{
    appendLog(QStringLiteral("列出设备..."));
    QString program = leAdbPath->text().trimmed();
    QProcess p;
    p.start(program, QStringList() << "devices");
    if (!p.waitForStarted(2000)) {
        appendLog(QStringLiteral("无法启动 adb"));
        return;
    }
    if (!p.waitForFinished(5000)) {
        p.kill();
        appendLog(QStringLiteral("adb devices 超时"));
        return;
    }
    QString out = p.readAllStandardOutput();
    appendLog(out);
    cbDevices->clear();
    // parse devices
    auto lines = out.split('\n', Qt::SkipEmptyParts);
    for (auto& ln : lines) {
        if (ln.contains("\tdevice")) {
            QString dev = ln.split('\t').first().trimmed();
            cbDevices->addItem(dev);
        }
    }
    if (cbDevices->count() > 0) {
        currentDeviceSerial = cbDevices->currentText();
        appendLog(QStringLiteral("当前设备: " )+ currentDeviceSerial);
    }
}

void AdbTool::onInstallApk()
{
    QString apk = leApkPath->text().trimmed();
    if (apk.isEmpty() || !QFile::exists(apk)) {
        appendLog(QStringLiteral("请选择有效的 APK"));
        return;
    }
    QString program = leAdbPath->text().trimmed();
    QStringList args;
    if (!currentDeviceSerial.isEmpty()) {
        args << "-s" << currentDeviceSerial;
    }
    args << "install" << "-r" << apk;

    appendLog(QStringLiteral("执行: ") + program + " " + args.join(" "));
    // run in background and read output
    adbProcess->start(program, args);
    progressBar->setRange(0, 0); // indeterminate
}

void AdbTool::onPushFile()
{
    QString src = QFileDialog::getOpenFileName(this, QStringLiteral("选择本地文件推送到设备"));
    if (src.isEmpty()) return;
    QString dst = QInputDialog::getText(this, QStringLiteral("目标路径"), QStringLiteral("设备目标路径 (如 /sdcard/Download/ 或 /data/local/tmp/):"));
    if (dst.isEmpty()) return;
    QString program = leAdbPath->text().trimmed();
    QStringList args;
    if (!currentDeviceSerial.isEmpty()) {
        args << "-s" << currentDeviceSerial;
    }
    args << "push" << src << dst;
    appendLog("执行: " + program + " " + args.join(" "));
    adbProcess->start(program, args);
    progressBar->setRange(0, 0);
}

void AdbTool::onPullFile()
{
    QString src = QInputDialog::getText(this, QStringLiteral("设备文件路径"), QStringLiteral("设备文件路径 (如 /sdcard/Download/log.txt):"));
    if (src.isEmpty()) return;
    QString dst = QFileDialog::getExistingDirectory(this, QStringLiteral("选择本地保存目录"));
    if (dst.isEmpty()) return;
    QString program = leAdbPath->text().trimmed();
    QStringList args;
    if (!currentDeviceSerial.isEmpty()) {
        args << "-s" << currentDeviceSerial;
    }
    args << "pull" << src << dst;
    appendLog("执行: " + program + " " + args.join(" "));
    adbProcess->start(program, args);
    progressBar->setRange(0, 0);
}

void AdbTool::onStartApp()
{
    QString pkg = lePackageName->text().trimmed();
    if (pkg.isEmpty()) {
        appendLog(QStringLiteral("请输入包名/Activity，例如 com.example/.MainActivity"));
        return;
    }
    QString program = leAdbPath->text().trimmed();
    QStringList args;
    if (!currentDeviceSerial.isEmpty()) args << "-s" << currentDeviceSerial;
    args << "shell" << "am" << "start" << "-n" << pkg;
    appendLog(QStringLiteral("执行: ") + program + " " + args.join(" "));
    adbProcess->start(program, args);
    progressBar->setRange(0, 0);
}

void AdbTool::onExportLog()
{
    QString savePath = QFileDialog::getSaveFileName(this, QStringLiteral("保存 log 压缩包"), QString(), "Zip Files (*.zip)");
    if (savePath.isEmpty()) return;
    // create tmp file for log
    QString tmpLog = QDir::temp().filePath(QString("logcat_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
    QString program = leAdbPath->text().trimmed();
    QStringList args;
    if (!currentDeviceSerial.isEmpty()) args << "-s" << currentDeviceSerial;
    args << "logcat" << "-d";
    appendLog(QStringLiteral("导出 log: ") + tmpLog);
    QProcess p;
    p.start(program, args);
    if (!p.waitForFinished(15000)) {
        appendLog(QStringLiteral("logcat 导出超时或失败"));
        p.kill();
        return;
    }
    QByteArray out = p.readAllStandardOutput();
    QFile f(tmpLog);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(out);
        f.close();
    }
    else {
        appendLog(QStringLiteral("写入临时 log 失败：") + tmpLog);
        return;
    }
    // zip
    QFile zipFile(savePath);
    if (zipFile.exists()) zipFile.remove();
    if (!zipFile.open(QIODevice::WriteOnly)) {
        appendLog(QStringLiteral("无法创建 zip: ") + savePath);
        return;
    }
   // QZipWriter zip(&zipFile);
  //  zip.addFile(QFileInfo(tmpLog).fileName(), QByteArray(out));
  //  zip.close();
   // zipFile.close();
  //  appendLog("导出并压缩完成: " + savePath);
}

void AdbTool::onStartCpuMonitor()
{
    QString target = cbMonitorProcess->currentText().trimmed();
    if (target.isEmpty()) {
        appendLog(QStringLiteral("请输入监控进程名或 pid"));
        return;
    }
    cpuSeries->clear();
    cpuXIndex = 0;
    cpuTimer->start(2000); // 每2秒采样一次
    appendLog(QStringLiteral("开始 CPU 监控: ") + target);
}

void AdbTool::onStopCpuMonitor()
{
    cpuTimer->stop();
    appendLog(QStringLiteral("停止 CPU 监控"));
}

// --- process callbacks
void AdbTool::readAdbOutput()
{
    QByteArray out = adbProcess->readAllStandardOutput();
    if (!out.isEmpty()) appendLog(QString::fromUtf8(out));
}

void AdbTool::adbFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    appendLog(QString(QStringLiteral("adb 进程结束, code=%1")).arg(exitCode));
    progressBar->setRange(0, 100);
    progressBar->setValue(100);
}

// cpu poll
void AdbTool::cpuPoll()
{
    QString target = cbMonitorProcess->currentText().trimmed();
    if (target.isEmpty()) return;

    QString program = leAdbPath->text().trimmed();
    QString deviceArg;
    if (!currentDeviceSerial.isEmpty()) deviceArg = "-s " + currentDeviceSerial + " ";

    // 获取 top 输出（只取一次）
    QProcess p;
    // -n 1 获取一次，具体不同设备 top 参数差异较大，这里采用通用方案
    QStringList args;
    if (!currentDeviceSerial.isEmpty()) args << "-s" << currentDeviceSerial;
    args << "shell" << "top" << "-b" << "-n" << "1";
    p.start(program, args);
    if (!p.waitForFinished(5000)) {
        appendLog(QStringLiteral("cpuPoll: top 超时"));
        p.kill();
        return;
    }
    QString out = p.readAllStandardOutput();
    QString lineFound;
    // 尝试按进程名匹配
    for (auto ln : out.split('\n')) {
        if (ln.contains(target)) {
            lineFound = ln.trimmed();
            break;
        }
    }
    // 如果没有找到，尝试匹配 pid
    if (lineFound.isEmpty()) {
        for (auto ln : out.split('\n')) {
            if (ln.contains(QRegExp("\\b" + QRegExp::escape(target) + "\\b"))) {
                lineFound = ln.trimmed();
                break;
            }
        }
    }
    if (lineFound.isEmpty()) {
        appendLog(QStringLiteral("未在 top 输出中找到目标进程（可能包名/进程名不匹配）"));
        return;
    }
    // 尝试解析 CPU 数值 (不同安卓 top 格式不同，这里取第3列或包含 % 的字段)
    QStringList cols = lineFound.simplified().split(' ');
    double cpuVal = 0.0;
    // try find a column with '%' or a numeric value <=100
    for (auto& c : cols) {
        if (c.contains('%')) {
            bool ok = false;
            QString num = c;
            num.remove('%');
            double v = num.toDouble(&ok);
            if (ok) { cpuVal = v; break; }
        }
    }
    if (cpuVal == 0.0) {
        // fallback: find first number-like between 0 and 100
        for (auto& c : cols) {
            bool ok = false;
            double v = c.toDouble(&ok);
            if (ok && v >= 0 && v <= 100) { cpuVal = v; break; }
        }
    }
    cpuSeries->append(cpuXIndex, cpuVal);
    axisX->setRange(qMax(0, cpuXIndex - 30), cpuXIndex + 1);
    cpuXIndex++;
    appendLog(QString("CPU sample: %1 (%2)").arg(cpuVal).arg(lineFound));
}