#pragma once
#include <QEvent>
#include <QTimer>
#include <QWidget>

#include <chrono>

#include <webgpu/webgpu_cpp.h>

#include "wgpu_context.hpp"

class QWGPUWidget : public QWidget {
  Q_OBJECT

 public:
  QWGPUWidget(QWidget* parent);
  ~QWGPUWidget() = default;

  void run();

 signals:
  void deviceInitialized();

 private slots:
  void onFrameTimerFired();

 private:
  QPaintEngine* paintEngine() const override;
  void paintEvent(QPaintEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void resizeEvent(QResizeEvent*) override;

  std::optional<wgpu_utils::wgpu_context> context_{};
  int width_{0};
  int height_{0};

  // Textures we render to:
  wgpu::Texture msaa_texture_{};
  wgpu::Texture depth_texture_{};

  // Pipeline and bind group layout for a simple triangle
  wgpu::RenderPipeline pipeline_{};
  wgpu::BindGroupLayout bg_layout_{};
  wgpu::Buffer uniform_buffer_{};

  QTimer frame_timer_;
  std::optional<std::chrono::steady_clock::time_point> start_time_;
};
