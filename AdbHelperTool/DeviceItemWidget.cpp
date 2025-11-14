#include "DeviceItemWidget.h"

DeviceItemWidget::DeviceItemWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.checkBox, &QCheckBox::stateChanged, this, &DeviceItemWidget::slotCheckStateChanged);
}

DeviceItemWidget::~DeviceItemWidget()
{}

void DeviceItemWidget::setDeviceInfo(const DeviceInfo deviceInfo)
{
	ui.label_3->setText(deviceInfo.name);
	ui.label_7->setText(deviceInfo.system);
	ui.label_5->setText(deviceInfo.serial);
}

void DeviceItemWidget::slotCheckStateChanged()
{
	emit sigChangeCheck(m_index, ui.checkBox->isChecked());
}
