cmake_minimum_required(VERSION 3.16)
project(
  qt-wgpu
  VERSION 0.1
  LANGUAGES CXX)

include(cmake/functions.cmake)

# Import third-party before we enable the Qt stuff.
add_subdirectory(dependencies)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui)

set(PROJECT_SOURCES
    source/main.cpp
    source/MainWindow.cpp
    source/MainWindow.h
    source/MainWindow.ui
    source/QWGPUWidget.cpp
    source/QWGPUWidget.h
    source/wgpu_context.cc
    source/wgpu_context.hpp
    source/wgpu_error_scope.cc
    source/wgpu_error_scope.hpp
    source/wgpu_fmt.hpp
    source/wgpu_setup.cc
    source/wgpu_setup.hpp
    source/wgpu_textures.cc
    source/wgpu_textures.hpp)

if(APPLE)
  # Need Objective-C++ implementation of CreateSurfaceForWidget on mac.
  list(APPEND PROJECT_SOURCES source/create_surface_descriptor.mm)
endif()

qt_add_executable(qt-wgpu MANUAL_FINALIZATION ${PROJECT_SOURCES})

target_link_libraries(qt-wgpu PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
if(UNIX AND NOT APPLE)
  target_link_libraries(qt-wgpu PRIVATE Qt${QT_VERSION_MAJOR}::Gui)
endif()

target_link_libraries(qt-wgpu PRIVATE webgpu_dawn fmt::fmt-header-only
                                      magic_enum::magic_enum)
target_include_directories(qt-wgpu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

if(APPLE)
  target_link_libraries(qt-wgpu PRIVATE "-framework QuartzCore")
endif()

# Copy dawn DLL to be adjacent to our executable on windows.
if(WIN32)
  target_copy_binaries(SOURCE_TARGET webgpu_dawn DEST_TARGET qt-wgpu)
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.qt-wgpu)
endif()
set_target_properties(
  qt-wgpu
  PROPERTIES ${BUNDLE_ID_OPTION} MACOSX_BUNDLE_BUNDLE_VERSION
             ${PROJECT_VERSION} MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR} MACOSX_BUNDLE
             FALSE WIN32_EXECUTABLE
             FALSE)

include(GNUInstallDirs)
install(
  TARGETS qt-wgpu
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_finalize_executable(qt-wgpu)

if(WIN32)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt REQUIRED)
  message(STATUS "windeployqt: ${WINDEPLOYQT_EXECUTABLE}")

  # windeployqt seems to copy the wrong directx compiler version, which then
  # causes dawn to fail to initialize the DXC compiler. Skip these DLLs...
  add_custom_command(
    TARGET qt-wgpu
    POST_BUILD
    COMMAND
      ${WINDEPLOYQT_EXECUTABLE} --dir $<TARGET_FILE_DIR:qt-wgpu>
      $<TARGET_FILE:qt-wgpu> --no-system-d3d-compiler --no-system-dxc-compiler
    COMMENT "Deploying Qt dependencies...")
endif(WIN32)
