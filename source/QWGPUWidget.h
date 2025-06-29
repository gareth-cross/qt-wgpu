#pragma once
#include <QTimer>
#include <QWidget>

#include <webgpu/webgpu_cpp.h>

#include "wgpu_context.hpp"

class QWGPUWidget : public QWidget {
  Q_OBJECT

 public:
  QWGPUWidget(QWidget* parent);
  ~QWGPUWidget();

  void run();

 signals:
  void deviceInitialized();

 private slots:
  void onFrameTimerFired();

 private:
  QPaintEngine* paintEngine() const override;
  void paintEvent(QPaintEvent* event) override;
  void showEvent(QShowEvent* event) override;

  std::optional<wgpu_utils::wgpu_context> context_;
  int width_{0};
  int height_{0};

  // Textures we render to:
  wgpu::Texture msaa_texture_;
  wgpu::Texture depth_texture_;

  QTimer frame_timer_;
};
