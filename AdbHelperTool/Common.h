
#pragma once
#include <QString>

// 设备数据结构（解析 adb 命令结果）
struct DeviceInfo {
    QString name;       // 设备型号
    QString system;     // 系统版本
    QString serial;     // 序列号
};