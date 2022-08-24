cmake_minimum_required(VERSION 3.5)

project(RealSenseNirFramesRecorder LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network REQUIRED)

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)

find_library(REALSENSE2_FOUND realsense2 HINTS ${LIBRARY_DIR} REQUIRED)

add_subdirectory(modules)
include_directories(modules)

set(PROG_SRC
    main.cpp
    imageframe.h
    imageframe.cpp
    cameraapplication.cpp
    cameraapplication.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    consolewatcher.cpp
    consolewatcher.h
    cameragrabber.cpp
    cameragrabber.h
    cameraslist.cpp
    cameraslist.h
    remotesyncserver.cpp
    remotesyncserver.h
    remotesyncclient.cpp
    remotesyncclient.h)

if(ANDROID)
  add_library(RealSenseNirFramesRecorder SHARED
    ${PROG_SRC}
  )
else()
  add_executable(RealSenseNirFramesRecorder
    ${PROG_SRC}
  )
endif()

target_link_libraries(RealSenseNirFramesRecorder PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network realsense2 ${STEREOVISION_LIB} JPEG::JPEG PNG::PNG TIFF::TIFF)

install (FILES desktops/RealSenseNirFramesRecorder.desktop DESTINATION usr/share/applications)
install (TARGETS RealSenseNirFramesRecorder DESTINATION usr/bin)
