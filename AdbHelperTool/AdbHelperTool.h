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

class AdbHelperTool : public QMainWindow
{
    Q_OBJECT

public:
    AdbHelperTool(QWidget *parent = nullptr);
    ~AdbHelperTool();
private slots:
    void refreshDeviceList();  // 刷新设备列表
    void selectAllDevices(bool isChecked);  // 全选/取消全选

private:
    QTabWidget* m_tabWidget;  // 顶部页签
    QWidget* m_deviceListPage;  // 设备列表页
    QListWidget* m_deviceList;  // 设备列表控件
    QLabel* m_deviceCountLabel;  // 设备数量标题
    QPushButton* m_restartBtn;  // 批量重启按钮
    QPushButton* m_refreshBtn;  // 刷新按钮
    QCheckBox* m_allSelectBox;  // 全选复选框

    // 模拟设备数据结构（实际需解析 adb 命令结果）
    struct DeviceInfo {
        QString name;       // 设备型号
        QString system;     // 系统版本
        QString serial;     // 序列号
    };

private:
    Ui::AdbHelperToolClass ui;
};
