#include <memory>

#import <AppKit/AppKit.h>
#include <QuartzCore/CAMetalLayer.h>
#include <webgpu/webgpu_cpp.h>

#include <QWidget>

// Based on: https://github.com/google/dawn/blob/main/src/dawn/glfw/utils_metal.mm
std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)> CreateSurfaceDescriptor(QWidget* const widget) {
  @autoreleasepool {
    // See: https://github.com/KhronosGroup/MoltenVK/issues/78
    NSView* const view = reinterpret_cast<NSView*>(widget->winId());

    // Create a CAMetalLayer that we will pass to SurfaceSourceMetalLayer.
    [view setWantsLayer:YES];
    [view setLayer:[CAMetalLayer layer]];

    // Use retina if the window was created with retina support.
    if (view.window) {
      [[view layer] setContentsScale:[view.window backingScaleFactor]];
    }

    wgpu::SurfaceSourceMetalLayer* desc = new wgpu::SurfaceSourceMetalLayer();
    desc->layer = [view layer];
    return {desc, [](wgpu::ChainedStruct* desc) { delete static_cast<wgpu::SurfaceSourceMetalLayer*>(desc); }};
  }
}
