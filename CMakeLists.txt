cmake_minimum_required(VERSION 3.16)
project(qt-wgpu VERSION 0.1 LANGUAGES CXX)

# Import before we enable the Qt stuff.
add_subdirectory(dependencies)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        QDirect3D12Widget.cpp
        QDirect3D12Widget.h
        MainWindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(qt-wgpu
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(qt-wgpu PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(qt-wgpu PRIVATE DirectX-Headers)

 target_link_libraries(qt-wgpu PRIVATE
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.qt-wgpu)
endif()
set_target_properties(qt-wgpu PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS qt-wgpu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(qt-wgpu)
endif()

find_program(WINDEPLOYQT_EXECUTABLE windeployqt REQUIRED)
add_custom_command(
    TARGET qt-wgpu
    POST_BUILD
    COMMAND ${WINDEPLOYQT_EXECUTABLE} --dir $<TARGET_FILE_DIR:qt-wgpu> $<TARGET_FILE:qt-wgpu>
    COMMENT "Deploying Qt dependencies..."
)
