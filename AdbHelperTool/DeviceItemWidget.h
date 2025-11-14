#pragma once

#include <QWidget>
#include "ui_DeviceItemWidget.h"
#include "Common.h"

class DeviceItemWidget : public QWidget
{
	Q_OBJECT

public:
	DeviceItemWidget(QWidget *parent = nullptr);
	~DeviceItemWidget();

	void setDeviceInfo(const DeviceInfo deviceInfo);

	void setIndex(int index) { m_index = index; }

signals:
	void sigChangeCheck(int index, bool checked);

public slots:
	void slotCheckStateChanged();


private:
	Ui::DeviceItemWidgetClass ui;

	int m_index;
};
