cmake_minimum_required(VERSION 3.16)



project(knowledgeAnswer VERSION 0.1 LANGUAGES CXX)



set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOMOC ON)

set(CMAKE_AUTORCC ON)



set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)



find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Charts)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Charts)



set(PROJECT_SOURCES

        main.cpp



        # 应用外壳（登录 / 主窗口 / 路由 / 占位页）

        app/Application.h            app/Application.cpp

        app/LoginDialog.h           app/LoginDialog.cpp

        app/MainWindow.h            app/MainWindow.cpp

        app/PageRouter.h            app/PageRouter.cpp

        app/RefreshablePage.h

        app/PlaceholderPage.h       app/PlaceholderPage.cpp

        app/DashboardPage.h         app/DashboardPage.cpp

        app/SystemAdminPage.h       app/SystemAdminPage.cpp

        app/KnowledgeBasePage.h     app/KnowledgeBasePage.cpp

        app/ArticleManagePage.h     app/ArticleManagePage.cpp

        app/AuditCenterPage.h       app/AuditCenterPage.cpp

        app/ArticleEditorDialog.h   app/ArticleEditorDialog.cpp

        app/SearchPage.h            app/SearchPage.cpp

        app/ArticleDetailDialog.h   app/ArticleDetailDialog.cpp

        app/ArticleDetailPanel.h    app/ArticleDetailPanel.cpp

        app/ArticleFeedCard.h       app/ArticleFeedCard.cpp

        app/ProfilePage.h           app/ProfilePage.cpp

        app/UserProfilePanel.h      app/UserProfilePanel.cpp

        app/ChangePasswordDialog.h  app/ChangePasswordDialog.cpp

        app/PinTagsEditDialog.h     app/PinTagsEditDialog.cpp

        app/ShareDialog.h           app/ShareDialog.cpp

        app/FavoritePage.h          app/FavoritePage.cpp

        app/SharePage.h             app/SharePage.cpp

        app/StatisticsPage.h        app/StatisticsPage.cpp

        app/OpenApiPage.h           app/OpenApiPage.cpp

        app/QaPage.h                app/QaPage.cpp

        app/AiConfigPage.h          app/AiConfigPage.cpp



        # 通用组件

        common/AvatarLabel.h        common/AvatarLabel.cpp

        common/TagStyle.h           common/TagStyle.cpp

        common/TableStyle.h         common/TableStyle.cpp
        common/NavTreeDelegate.h    common/NavTreeDelegate.cpp
        common/ThemeIcons.h         common/ThemeIcons.cpp
        common/NavIcons.h           common/NavIcons.cpp
        common/ComboStyle.h         common/ComboStyle.cpp
        common/ChartTheme.h         common/ChartTheme.cpp
        common/ChartCard.h          common/ChartCard.cpp



        # 跨模块基础设施

        core/network/ApiClient.h    core/network/ApiClient.cpp

        core/auth/Session.h         core/auth/Session.cpp

        core/config/Settings.h      core/config/Settings.cpp

        core/config/PinnedTagsStore.h core/config/PinnedTagsStore.cpp

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



target_include_directories(knowledgeAnswer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})



target_link_libraries(knowledgeAnswer PRIVATE

    Qt${QT_VERSION_MAJOR}::Widgets

    Qt${QT_VERSION_MAJOR}::Network

    Qt${QT_VERSION_MAJOR}::Charts

)



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

