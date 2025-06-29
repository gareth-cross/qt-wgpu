#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QTime>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  m_pScene = ui->centralwidget;
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::connectSlots() {
  connect(m_pScene, &QDirect3D12Widget::deviceInitialized, this,
          &MainWindow::init);
  connect(m_pScene, &QDirect3D12Widget::ticked, this, &MainWindow::tick);
  connect(m_pScene, &QDirect3D12Widget::rendered, this, &MainWindow::render);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  event->ignore();
  m_pScene->release();
//   QTime dieTime = QTime::currentTime().addMSecs(500);
//   while (QTime::currentTime() < dieTime)
//     QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

  event->accept();
}

void MainWindow::init(bool success) {
  if (!success) {
    QMessageBox::critical(this, "ERROR",
                          "Direct3D widget initialization failed.",
                          QMessageBox::Ok);
    return;
  }

  QTimer::singleShot(500, this, [&] { m_pScene->run(); });
  disconnect(m_pScene, &QDirect3D12Widget::deviceInitialized, this,
             &MainWindow::init);
}

void MainWindow::tick() {}

void MainWindow::render(ID3D12GraphicsCommandList *cl) {}
