#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QTime>

#include "QWGPUWidget.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  gpuWidget_ = ui->centralWidget;

  connect(gpuWidget_, &QWGPUWidget::deviceInitialized, this, &MainWindow::init);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::init() {
  disconnect(gpuWidget_, &QWGPUWidget::deviceInitialized, this, &MainWindow::init);
  qInfo() << "Device initialized, starting render loop.";
  gpuWidget_->run();
}
