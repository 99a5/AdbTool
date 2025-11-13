#pragma once
#include <QWidget>
#include "ADBController.h"

class DeviceManagerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DeviceManagerWidget(ADBController* adb, QWidget* parent = nullptr);

private:
    ADBController* adbController;
};
