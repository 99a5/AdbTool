#include "DeviceManagerWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>

DeviceManagerWidget::DeviceManagerWidget(ADBController* adb, QWidget* parent)
    : QWidget(parent), adbController(adb)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QListWidget* list = new QListWidget(this);
    QPushButton* refreshBtn = new QPushButton("Ë¢ÐÂÉè±¸", this);
    layout->addWidget(list);
    layout->addWidget(refreshBtn);

    connect(refreshBtn, &QPushButton::clicked, [=]() {
        list->clear();
        QStringList devices = adbController->listDevices();
        list->addItems(devices);
        });
}
