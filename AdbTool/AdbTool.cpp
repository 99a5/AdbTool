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

    //onDetectAdb();
   /* if (QFile::exists(lastHistoryFile)) {
        QFile hf(lastHistoryFile);
        if (hf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(hf.readAll()).split('\n', Qt::SkipEmptyParts);
            for (auto& l : lines) appendLog("[HIST] " + l);
        }
    }*/
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
    leAdbPath = new QLineEdit(this);
    btnDetect = new QPushButton(QStringLiteral("检测 ADB"), this);
    btnBatchRefresh = new QPushButton(QStringLiteral("连接设备"));
    btnBatchRefresh = new QPushButton(QStringLiteral("刷新设备列表"));

    logView = new QTextEdit(this);
    logView->setReadOnly(true);
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


    progressBar = new QProgressBar(this);
}

void AdbTool::initUI()
{
    leAdbPath->setToolTip(QStringLiteral("ADB 可执行文件路径，默认使用系统 PATH 中的 adb"));

    // ========= ADB 设置区域 =========
    QGroupBox* adbGroup = new QGroupBox(QStringLiteral("ADB 设置"), this);
    QVBoxLayout* adbLay = new QVBoxLayout(adbGroup);

    // ADB 路径行
    QHBoxLayout* adbPathLay = new QHBoxLayout();
    adbPathLay->addWidget(new QLabel(QStringLiteral("ADB Path:")));
    adbPathLay->addWidget(leAdbPath);
    adbPathLay->addWidget(btnDetect);
    adbLay->addLayout(adbPathLay);

    // 按钮行
    QHBoxLayout* adbBtnLay = new QHBoxLayout();
    adbBtnLay->addWidget(btnBatchRefresh);
    adbBtnLay->addWidget(btnBatchInstall);
    adbBtnLay->addWidget(btnBatchStartApp);
    adbLay->addLayout(adbBtnLay);

    // ========== 多选设备表格 ==========
    deviceTable = new QTableWidget(this);
    deviceTable->setColumnCount(3);
    QStringList headers = { QStringLiteral("选择"), QStringLiteral("型号"), QStringLiteral("PID") };
    deviceTable->setHorizontalHeaderLabels(headers);

    //  表头样式美化
    QHeaderView* header = deviceTable->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
 
    // 表格基础外观优化
    deviceTable->setStyleSheet(R"(
    QTableWidget {
        background-color: #fafafa;
        alternate-background-color: #f0f0f0;
        border: 1px solid #d0d0d0;
        border-radius: 6px;
        gridline-color: #e0e0e0;
        font-size: 14px;
        selection-background-color: #d7ebff;
    }

    QTableWidget::item:hover {
        background-color: #f2f9ff;
    }

    QTableWidget::item:selected {
        background-color: #d7ebff;
    }

    QScrollBar:vertical {
        width: 10px;
        background: transparent;
        margin: 0px;
    }

    QScrollBar::handle:vertical {
        background: #c0c0c0;
        border-radius: 4px;
        min-height: 20px;
    }

    QScrollBar::handle:vertical:hover {
        background: #a0a0a0;
    }
)");

    //其他行为设置
    deviceTable->verticalHeader()->setVisible(false);
    deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceTable->setSelectionMode(QAbstractItemView::NoSelection);
    deviceTable->setAlternatingRowColors(true);
    deviceTable->setFocusPolicy(Qt::NoFocus);
    deviceTable->setShowGrid(true);
    deviceTable->setContentsMargins(8, 8, 8, 8);
    deviceTable->setStyleSheet(deviceTable->styleSheet() + "QTableWidget { padding: 8px; }");

    adbLay->addWidget(deviceTable);


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
    resize(600, 800);
    setWindowTitle(QStringLiteral("ADB 智能助手 - Demo"));
}

void AdbTool::connectSlots()
{
    connect(btnBatchRefresh, &QPushButton::clicked, this, &AdbTool::refreshDeviceList);


    connect(btnDetect, &QPushButton::clicked, this, &AdbTool::onDetectAdb);
    //connect(btnBrowseApk, &QPushButton::clicked, [this]() {
    //    QString f = QFileDialog::getOpenFileName(this, QStringLiteral("选择 APK"), QString(), "APK Files (*.apk)");
    //    if (!f.isEmpty()) leApkPath->setText(f);
    //    });
    //connect(btnInstall, &QPushButton::clicked, this, &AdbTool::onInstallApk);
    //connect(btnPush, &QPushButton::clicked, this, &AdbTool::onPushFile);
    //connect(btnPull, &QPushButton::clicked, this, &AdbTool::onPullFile);
    //connect(btnStartApp, &QPushButton::clicked, this, &AdbTool::onStartApp);
    //connect(btnExportLog, &QPushButton::clicked, this, &AdbTool::onExportLog);

    //connect(btnStartMon, &QPushButton::clicked, this, &AdbTool::onStartCpuMonitor);
    //connect(btnStopMon, &QPushButton::clicked, this, &AdbTool::onStopCpuMonitor);

    //connect(adbProcess, &QProcess::readyReadStandardOutput, this, &AdbTool::readAdbOutput);
    //connect(adbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
    //    this, &AdbTool::adbFinished);

    //connect(cpuTimer, &QTimer::timeout, this, &AdbTool::cpuPoll);
}

void AdbTool::refreshDeviceList()
{
    deviceTable->setRowCount(0);

    QStringList devices = getAdbDeviceList(); // 你已有的 ADB 列表方法
    for (const QString& dev : devices) {
        QString model = getDeviceModel(dev); // 可调用 adb -s <dev> shell getprop ro.product.model
        int row = deviceTable->rowCount();
        deviceTable->insertRow(row);

        // 选择框
        QCheckBox* cb = new QCheckBox();
        QWidget* w = new QWidget();
        QHBoxLayout* l = new QHBoxLayout(w);
        l->addWidget(cb);
        l->setAlignment(Qt::AlignCenter);
        l->setContentsMargins(0, 0, 0, 0);
        w->setLayout(l);
        deviceTable->setCellWidget(row, 0, w);

        // 型号
        deviceTable->setItem(row, 1, new QTableWidgetItem(model));

        // PID
        deviceTable->setItem(row, 2, new QTableWidgetItem(dev));

        connect(cb, &QCheckBox::stateChanged, this, [=](int) {
            updateSelectedDevices();
            });
    }
}

void AdbTool::updateSelectedDevices()
{
    selectedDevices.clear();
    int rows = deviceTable->rowCount();
    for (int i = 0; i < rows; ++i) {
        QWidget* w = deviceTable->cellWidget(i, 0);
        if (!w) continue;
        QCheckBox* cb = w->findChild<QCheckBox*>();
        if (cb && cb->isChecked()) {
            QString pid = deviceTable->item(i, 2)->text();
            selectedDevices.append(pid);
        }
    }
}

void AdbTool::onExecute()
{
    if (selectedDevices.isEmpty()) {
        QMessageBox::warning(this, "提示", "请至少选择一个设备！");
        return;
    }

    for (const QString& dev : selectedDevices) {
        // 执行单设备逻辑
       // runAdbCommand(dev);
    }
}

QStringList AdbTool::getAdbDeviceList()
{
    QStringList devices;

    QString adbPath = leAdbPath->text().trimmed();
    if (adbPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请先设置 ADB 路径"));
        return devices;
    }

    QProcess process;
    process.setProgram(adbPath);
    process.setArguments({ "devices" });
    process.start();
    process.waitForFinished();

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.contains("List of devices")) continue;
        if (line.contains("offline") || line.contains("unauthorized")) continue;

        QString pid = line.section('\t', 0, 0).trimmed();
        if (!pid.isEmpty())
            devices.append(pid);
    }

    return devices;
}

QString AdbTool::getDeviceModel(const QString& deviceId)
{
    QString adbPath = leAdbPath->text().trimmed();
    QProcess process;
    process.start(adbPath, { "-s", deviceId, "shell", "getprop", "ro.product.model" });
    process.waitForFinished();

    QString model = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    return model.isEmpty() ? QStringLiteral("未知型号") : model;
}

// --- drag/drop
//void AdbTool::dragEnterEvent(QDragEnterEvent* event)
//{
//    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
//}
//void AdbTool::dropEvent(QDropEvent* event)
//{
//    auto urls = event->mimeData()->urls();
//    if (!urls.isEmpty()) {
//        QString path = urls.first().toLocalFile();
//        if (path.endsWith(".apk", Qt::CaseInsensitive)) {
//            leApkPath->setText(path);
//            appendLog("拖入 APK: " + path);
//        }
//        else {
//            // store for push
//            cbMonitorProcess->setEditText(path);
//            appendLog("拖入文件: " + path);
//        }
//    }
//}

//// --- helpers
//QString AdbTool::adbCommand(const QStringList& args)
//{
//    //QString aPath = leAdbPath->text().trimmed();
//    //QString cmd = QProcess::programFilePath(); // not used
//    // build full command display (for history)
//    //QString full = aPath + " " + args.join(" ");
//   // QFile hf(lastHistoryFile);
//   // if (hf.open(QIODevice::Append | QIODevice::Text)) {
//   //     hf.write(full.toUtf8() + "\n");
//   // }
//   // return full;
//    return "";
//}

void AdbTool::appendLog(const QString& s)
{
    QString t = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logView->append(QString("[%1] %2").arg(t, s));
}

void AdbTool::onDetectAdb()
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
            leAdbPath->setText(internalAdbPath);
        }
        else {
            appendLog(QStringLiteral("未找到内置 adb: ") + internalAdbPath);
            leAdbPath->setText("");
        }
    }
    else {
        leAdbPath->setText(adbPath);
    }
    appendLog("adb version: " + out.trimmed());
}

//void AdbTool::onListDevices()
//{
//   // appendLog(QStringLiteral("列出设备..."));
//   // QString program = leAdbPath->text().trimmed();
//   // QProcess p;
//   // p.start(program, QStringList() << "devices");
//   // if (!p.waitForStarted(2000)) {
//   //     appendLog(QStringLiteral("无法启动 adb"));
//   //     return;
//   // }
//   // if (!p.waitForFinished(5000)) {
//   //     p.kill();
//   //     appendLog(QStringLiteral("adb devices 超时"));
//   //     return;
//   // }
//   // QString out = p.readAllStandardOutput();
//   // appendLog(out);
//   //// cbDevices->clear();
//   // // parse devices
//   // auto lines = out.split('\n', Qt::SkipEmptyParts);
//   // for (auto& ln : lines) {
//   //     if (ln.contains("\tdevice")) {
//   //         QString dev = ln.split('\t').first().trimmed();
//   //         cbDevices->addItem(dev);
//   //     }
//   // }
//   // if (cbDevices->count() > 0) {
//   //     currentDeviceSerial = cbDevices->currentText();
//   //     appendLog(QStringLiteral("当前设备: " )+ currentDeviceSerial);
//   // }
//}

//void AdbTool::onInstallApk()
//{
//    QString apk = leApkPath->text().trimmed();
//    if (apk.isEmpty() || !QFile::exists(apk)) {
//        appendLog(QStringLiteral("请选择有效的 APK"));
//        return;
//    }
//    QString program = leAdbPath->text().trimmed();
//    QStringList args;
//    if (!currentDeviceSerial.isEmpty()) {
//        args << "-s" << currentDeviceSerial;
//    }
//    args << "install" << "-r" << apk;
//
//    appendLog(QStringLiteral("执行: ") + program + " " + args.join(" "));
//    // run in background and read output
//    adbProcess->start(program, args);
//    progressBar->setRange(0, 0); // indeterminate
//}
//
//void AdbTool::onPushFile()
//{
//    QString src = QFileDialog::getOpenFileName(this, QStringLiteral("选择本地文件推送到设备"));
//    if (src.isEmpty()) return;
//    QString dst = QInputDialog::getText(this, QStringLiteral("目标路径"), QStringLiteral("设备目标路径 (如 /sdcard/Download/ 或 /data/local/tmp/):"));
//    if (dst.isEmpty()) return;
//    QString program = leAdbPath->text().trimmed();
//    QStringList args;
//    if (!currentDeviceSerial.isEmpty()) {
//        args << "-s" << currentDeviceSerial;
//    }
//    args << "push" << src << dst;
//    appendLog("执行: " + program + " " + args.join(" "));
//    adbProcess->start(program, args);
//    progressBar->setRange(0, 0);
//}
//
//void AdbTool::onPullFile()
//{
//    QString src = QInputDialog::getText(this, QStringLiteral("设备文件路径"), QStringLiteral("设备文件路径 (如 /sdcard/Download/log.txt):"));
//    if (src.isEmpty()) return;
//    QString dst = QFileDialog::getExistingDirectory(this, QStringLiteral("选择本地保存目录"));
//    if (dst.isEmpty()) return;
//    QString program = leAdbPath->text().trimmed();
//    QStringList args;
//    if (!currentDeviceSerial.isEmpty()) {
//        args << "-s" << currentDeviceSerial;
//    }
//    args << "pull" << src << dst;
//    appendLog("执行: " + program + " " + args.join(" "));
//    adbProcess->start(program, args);
//    progressBar->setRange(0, 0);
//}
//
//void AdbTool::onStartApp()
//{
//    QString pkg = lePackageName->text().trimmed();
//    if (pkg.isEmpty()) {
//        appendLog(QStringLiteral("请输入包名/Activity，例如 com.example/.MainActivity"));
//        return;
//    }
//    QString program = leAdbPath->text().trimmed();
//    QStringList args;
//    if (!currentDeviceSerial.isEmpty()) args << "-s" << currentDeviceSerial;
//    args << "shell" << "am" << "start" << "-n" << pkg;
//    appendLog(QStringLiteral("执行: ") + program + " " + args.join(" "));
//    adbProcess->start(program, args);
//    progressBar->setRange(0, 0);
//}
//
//void AdbTool::onExportLog()
//{
//    QString savePath = QFileDialog::getSaveFileName(this, QStringLiteral("保存 log 压缩包"), QString(), "Zip Files (*.zip)");
//    if (savePath.isEmpty()) return;
//    // create tmp file for log
//    QString tmpLog = QDir::temp().filePath(QString("logcat_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
//    QString program = leAdbPath->text().trimmed();
//    QStringList args;
//    if (!currentDeviceSerial.isEmpty()) args << "-s" << currentDeviceSerial;
//    args << "logcat" << "-d";
//    appendLog(QStringLiteral("导出 log: ") + tmpLog);
//    QProcess p;
//    p.start(program, args);
//    if (!p.waitForFinished(15000)) {
//        appendLog(QStringLiteral("logcat 导出超时或失败"));
//        p.kill();
//        return;
//    }
//    QByteArray out = p.readAllStandardOutput();
//    QFile f(tmpLog);
//    if (f.open(QIODevice::WriteOnly)) {
//        f.write(out);
//        f.close();
//    }
//    else {
//        appendLog(QStringLiteral("写入临时 log 失败：") + tmpLog);
//        return;
//    }
//    // zip
//    QFile zipFile(savePath);
//    if (zipFile.exists()) zipFile.remove();
//    if (!zipFile.open(QIODevice::WriteOnly)) {
//        appendLog(QStringLiteral("无法创建 zip: ") + savePath);
//        return;
//    }
//   // QZipWriter zip(&zipFile);
//  //  zip.addFile(QFileInfo(tmpLog).fileName(), QByteArray(out));
//  //  zip.close();
//   // zipFile.close();
//  //  appendLog("导出并压缩完成: " + savePath);
//}
//
//void AdbTool::onStartCpuMonitor()
//{
//    QString target = cbMonitorProcess->currentText().trimmed();
//    if (target.isEmpty()) {
//        appendLog(QStringLiteral("请输入监控进程名或 pid"));
//        return;
//    }
//    cpuSeries->clear();
//    cpuXIndex = 0;
//    cpuTimer->start(2000); // 每2秒采样一次
//    appendLog(QStringLiteral("开始 CPU 监控: ") + target);
//}
//
//void AdbTool::onStopCpuMonitor()
//{
//    cpuTimer->stop();
//    appendLog(QStringLiteral("停止 CPU 监控"));
//}
//
//// --- process callbacks
//void AdbTool::readAdbOutput()
//{
//    QByteArray out = adbProcess->readAllStandardOutput();
//    if (!out.isEmpty()) appendLog(QString::fromUtf8(out));
//}
//
//void AdbTool::adbFinished(int exitCode, QProcess::ExitStatus status)
//{
//    Q_UNUSED(status);
//    appendLog(QString(QStringLiteral("adb 进程结束, code=%1")).arg(exitCode));
//    progressBar->setRange(0, 100);
//    progressBar->setValue(100);
//}
//
//// cpu poll
//void AdbTool::cpuPoll()
//{
//    QString target = cbMonitorProcess->currentText().trimmed();
//    if (target.isEmpty()) return;
//
//    QString program = leAdbPath->text().trimmed();
//    QString deviceArg;
//    if (!currentDeviceSerial.isEmpty()) deviceArg = "-s " + currentDeviceSerial + " ";
//
//    // 获取 top 输出（只取一次）
//    QProcess p;
//    // -n 1 获取一次，具体不同设备 top 参数差异较大，这里采用通用方案
//    QStringList args;
//    if (!currentDeviceSerial.isEmpty()) args << "-s" << currentDeviceSerial;
//    args << "shell" << "top" << "-b" << "-n" << "1";
//    p.start(program, args);
//    if (!p.waitForFinished(5000)) {
//        appendLog(QStringLiteral("cpuPoll: top 超时"));
//        p.kill();
//        return;
//    }
//    QString out = p.readAllStandardOutput();
//    QString lineFound;
//    // 尝试按进程名匹配
//    for (auto ln : out.split('\n')) {
//        if (ln.contains(target)) {
//            lineFound = ln.trimmed();
//            break;
//        }
//    }
//    // 如果没有找到，尝试匹配 pid
//    if (lineFound.isEmpty()) {
//        for (auto ln : out.split('\n')) {
//            if (ln.contains(QRegExp("\\b" + QRegExp::escape(target) + "\\b"))) {
//                lineFound = ln.trimmed();
//                break;
//            }
//        }
//    }
//    if (lineFound.isEmpty()) {
//        appendLog(QStringLiteral("未在 top 输出中找到目标进程（可能包名/进程名不匹配）"));
//        return;
//    }
//    // 尝试解析 CPU 数值 (不同安卓 top 格式不同，这里取第3列或包含 % 的字段)
//    QStringList cols = lineFound.simplified().split(' ');
//    double cpuVal = 0.0;
//    // try find a column with '%' or a numeric value <=100
//    for (auto& c : cols) {
//        if (c.contains('%')) {
//            bool ok = false;
//            QString num = c;
//            num.remove('%');
//            double v = num.toDouble(&ok);
//            if (ok) { cpuVal = v; break; }
//        }
//    }
//    if (cpuVal == 0.0) {
//        // fallback: find first number-like between 0 and 100
//        for (auto& c : cols) {
//            bool ok = false;
//            double v = c.toDouble(&ok);
//            if (ok && v >= 0 && v <= 100) { cpuVal = v; break; }
//        }
//    }
//    cpuSeries->append(cpuXIndex, cpuVal);
//    axisX->setRange(qMax(0, cpuXIndex - 30), cpuXIndex + 1);
//    cpuXIndex++;
//    appendLog(QString("CPU sample: %1 (%2)").arg(cpuVal).arg(lineFound));
//}