cmake_minimum_required(VERSION 3.16)

project(knowledgeAnswer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        main.cpp

        # 应用外壳（登录 / 主窗口 / 路由 / 占位页）
        app/Application.h            app/Application.cpp
        app/LoginDialog.h           app/LoginDialog.cpp
        app/MainWindow.h            app/MainWindow.cpp
        app/PageRouter.h            app/PageRouter.cpp
        app/PlaceholderPage.h       app/PlaceholderPage.cpp
        app/SystemAdminPage.h       app/SystemAdminPage.cpp

        # 跨模块基础设施
        core/network/ApiClient.h    core/network/ApiClient.cpp
        core/auth/Session.h         core/auth/Session.cpp
        core/config/Settings.h      core/config/Settings.cpp
        core/logging/Logger.h
        core/notify/Notify.h

        # 资源（主题样式）
        resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(knowledgeAnswer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(knowledgeAnswer SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(knowledgeAnswer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# 头文件按源码根做相对包含，如 #include "core/network/ApiClient.h"
target_include_directories(knowledgeAnswer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(knowledgeAnswer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.knowledgeAnswer)
endif()
set_target_properties(knowledgeAnswer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS knowledgeAnswer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(knowledgeAnswer)
endif()
