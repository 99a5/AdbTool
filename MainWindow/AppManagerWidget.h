#pragma once

#include <QWidget>
#include "ADBController.h"
class AppManagerWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AppManagerWidget(ADBController* adb, QWidget* parent = nullptr);

private:
	ADBController* adbController;
};
