#pragma once
#include <QMainWindow>
#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include "ADBController.h"            //添加完整定义
#include "DeviceManagerWidget.h"
#include "AppManagerWidget.h"
//#include "widgets/FileManagerWidget.h"
//#include "widgets/LogcatViewerWidget.h"
//#include "widgets/ShellTerminalWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindowClass* ui;
    ADBController* adbController;

    void setupWidgets();
};
