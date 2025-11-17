#pragma once

#include <QWidget>
#include <QCheckBox>
#include "ui_DeviceItemWidget.h"
#include "Common.h"

class DeviceItemWidget : public QWidget
{
	Q_OBJECT

public:
	DeviceItemWidget(QWidget *parent = nullptr);
	~DeviceItemWidget();

	void setDeviceInfo(const DeviceInfo deviceInfo);
	DeviceInfo getDeviceInfo() { return m_deviceInfo; }

	void setIndex(int index) { m_index = index; }
	int getIndex()  { return m_index; }

	void setCheckState(bool state);      // 勾选或取消勾选
	bool checkState() ;             // 返回当前勾选状态

signals:
	void sigChangeCheck(int index, bool checked);

public slots:
	void slotCheckStateChanged(int state);


private:
	Ui::DeviceItemWidgetClass ui;
	DeviceInfo m_deviceInfo;     // 内部保存设备信息
	int m_index = -1;            // 当前列表项的序号
};
