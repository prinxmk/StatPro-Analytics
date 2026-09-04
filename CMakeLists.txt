cmake_minimum_required(VERSION 3.21)
project(StatProAnalytics VERSION 0.6.5 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_add_executable(StatProAnalytics
    src/main.cpp
    src/core/AppState.h src/core/AppState.cpp
    src/data/DataSet.h src/data/DataSet.cpp
    src/data/CsvImporter.h src/data/CsvImporter.cpp
    src/data/ProjectManager.h src/data/ProjectManager.cpp
    src/analysis/AnalysisEngine.h src/analysis/AnalysisEngine.cpp
    src/ui/MainWindow.h src/ui/MainWindow.cpp
)
target_link_libraries(StatProAnalytics PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
if(WIN32)
    set_target_properties(StatProAnalytics PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
install(TARGETS StatProAnalytics RUNTIME DESTINATION .)
