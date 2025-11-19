
#pragma once
#include <QString>

// 设备数据结构（解析 adb 命令结果）
struct DeviceInfo {
    QString name;       // 设备型号
    QString system;     // 系统版本
    QString serial;     // 序列号
};

// 保存单设备应用信息
struct AppInfo {
    QString appName;    // 应用名称
    QString packageName; // 应用包名
    bool systemApp;     // 是否系统应用
};