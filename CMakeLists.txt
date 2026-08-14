cmake_minimum_required(VERSION 3.16)

project(ComplexCalc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)

if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    set(QT_LIBRARIES Qt6::Widgets)
else()
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
    set(QT_LIBRARIES Qt5::Widgets)
endif()

add_executable(ComplexCalc
    main.cpp
)

target_link_libraries(ComplexCalc PRIVATE
    ${QT_LIBRARIES}
)