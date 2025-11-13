#include "AdbHelperTool.h"
#include <QMessageBox>
#include <QDateTime>


AdbHelperTool::AdbHelperTool(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // 1. 窗口基础设置
    //setCentralWidget(centralWidget);
    resize(800, 600);
    setWindowTitle(QStringLiteral("ADB 智能助手 - Demo"));

    // 2. 底部状态栏
    QStatusBar* statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage(QStringLiteral("adb 版本 1.0.0 | 上次刷新：--"));
}

AdbHelperTool::~AdbHelperTool()
{}


// 刷新设备列表（核心逻辑）
void AdbHelperTool::refreshDeviceList()
{
   
}

// 全选/取消全选
void AdbHelperTool::selectAllDevices(bool isChecked)
{
   
}

