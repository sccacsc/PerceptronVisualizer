cmake_minimum_required(VERSION 3.16)

project(PerceptronVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(PerceptronVisualizer
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/NetworkModel.h
    src/NetworkModel.cpp
    src/PlaneWidget.h
    src/PlaneWidget.cpp
    src/ArchitectureWidget.h
    src/ArchitectureWidget.cpp
)

target_link_libraries(PerceptronVisualizer PRIVATE Qt6::Widgets)
