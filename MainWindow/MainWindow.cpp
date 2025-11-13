#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui->setupUi(this);
    adbController = new ADBController(this);
    setupWidgets();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupWidgets()
{
    // 使用 QStackedWidget + 左侧导航列表
    ui->stackedWidget->addWidget(new DeviceManagerWidget(adbController));
    ui->stackedWidget->addWidget(new AppManagerWidget(adbController));
   // ui->stackedWidget->addWidget(new FileManagerWidget(adbController));
   // ui->stackedWidget->addWidget(new LogcatViewerWidget(adbController));
    //ui->stackedWidget->addWidget(new ShellTerminalWidget(adbController));

    // 可以连接左侧列表的点击信号切换 stackedWidget
}