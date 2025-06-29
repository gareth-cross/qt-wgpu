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

void MainWindow::connectSlots() {
  // Q_ASSERT(ui->view);
  // connect(m_pScene, &QDirect3D12Widget::ticked, this,
  // &MainWindow::tick); connect(m_pScene, &QDirect3D12Widget::rendered, this,
  // &MainWindow::render);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  event->ignore();
  // m_pScene->release();
  event->accept();
}

void MainWindow::init() {
  disconnect(gpuWidget_, &QWGPUWidget::deviceInitialized, this, &MainWindow::init);
  qInfo() << "Device initialized, starting render loop.";
  gpuWidget_->run();
}
