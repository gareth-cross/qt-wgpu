#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "QDirect3D12Widget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void connectSlots();

private:
  void closeEvent(QCloseEvent *event) override;

  Ui::MainWindow *ui;

  QDirect3D12Widget *m_pScene;

public slots:
  void init(bool success);
  void tick();
  void render(ID3D12GraphicsCommandList *cl);
};
#endif // MAINWINDOW_H
