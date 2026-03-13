cmake_minimum_required(VERSION 3.16)

project(Mosfer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMosfer
    main.cpp
)

qt_add_qml_module(appMosfer
    URI Mosfer
    VERSION 1.0
    QML_FILES
        Main.qml
)

set_target_properties(appMosfer PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMosfer
    PRIVATE Qt6::Quick
)

add_subdirectory(platform/linux)
add_subdirectory(platform/macos)
add_subdirectory(platform/windows)

include(GNUInstallDirs)
install(TARGETS appMosfer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
