#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AdbHelperTool.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include "Common.h"

class AdbHelperTool : public QMainWindow
{
    Q_OBJECT

public:
    AdbHelperTool(QWidget *parent = nullptr);
    ~AdbHelperTool();

    void connectSlots();



private slots:

    // 刷新设备列表
    void refreshDeviceList();    

    // 全选
    void slotSelectAll();     

    // 反选
    void slotCancelSelectAll(); 

    // 单选
    void slotChangeCheck(); 

private:
    // 获取adb设备列表
    std::vector<DeviceInfo> getAdbDeviceList();

    // 检测adb路径
    void detectAdbPath();

    //日支输出
    void appendLog(const QString& s);

    QString runAdbCommand(const QString& serial, const QString& cmd);

    QTabWidget* m_tabWidget;  // 顶部页签
    QWidget* m_deviceListPage;  // 设备列表页
    QListWidget* m_deviceList;  // 设备列表控件
    QLabel* m_deviceCountLabel;  // 设备数量标题
    QPushButton* m_restartBtn;  // 批量重启按钮
    QPushButton* m_refreshBtn;  // 刷新按钮
    QCheckBox* m_allSelectBox;  // 全选复选框


private:
    Ui::AdbHelperToolClass ui;

    QString m_adbPath;
};
