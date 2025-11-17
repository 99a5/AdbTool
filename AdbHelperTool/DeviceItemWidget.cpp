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
	m_deviceInfo = deviceInfo;

	ui.label_3->setText(deviceInfo.name);
	ui.label_7->setText(deviceInfo.system);
	ui.label_5->setText(deviceInfo.serial);
}

void DeviceItemWidget::slotCheckStateChanged(int state)
{
	bool checked = (state == Qt::Checked);
	emit sigChangeCheck(m_index, checked);
}

void DeviceItemWidget::setCheckState(bool state)
{
	// ÏÈ×èÖ¹ÐÅºÅ´¥·¢
	ui.checkBox->blockSignals(true);
	ui.checkBox->setChecked(state);
	ui.checkBox->blockSignals(false); // ½â³ý×èÖ¹
}

bool DeviceItemWidget::checkState() 
{ 
	return ui.checkBox->isChecked();
}
