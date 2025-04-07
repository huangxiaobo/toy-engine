cmake_minimum_required(VERSION 3.26.1)
project(qtpropertybrowser LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt 默认动态链接，如果没有添加到系统环境变量，需要复制相关dll到相同文件夹中
set(CMAKE_PREFIX_PATH "/opt/Qt/6.8.1/gcc_arm64/lib/cmake/")

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Core 
    Gui
    OpenGLWidgets
)

add_library(${PROJECT_NAME} STATIC 
    src/qtpropertybrowser.cpp
    src/qtpropertymanager.cpp
    src/qteditorfactory.cpp
    src/qtvariantproperty.cpp
    src/qttreepropertybrowser.cpp
    src/qtbuttonpropertybrowser.cpp
    src/qtgroupboxpropertybrowser.cpp
    src/qtpropertybrowserutils.cpp

    # 8 headers
    src/qtpropertybrowser.h
    src/qtpropertymanager.h
    src/qteditorfactory.h
    src/qtvariantproperty.h
    src/qttreepropertybrowser.h
    src/qtbuttonpropertybrowser.h
    src/qtgroupboxpropertybrowser.h
    src/qtpropertybrowserutils_p.h
)

target_include_directories(${PROJECT_NAME} PUBLIC src/)
# A CMake pattern to have headers that are not seen by the client of this library.
# So headers which not have to be visible for client-code should be placed in src/
target_include_directories(${PROJECT_NAME} PRIVATE src/)

if (MSVC)
    # PRIVATE specifies that the specified compile options are only for the ${PROJECT_NAME} itself,
    # and not for any targets that link to it.
    # warning level 4 and all warnings as errors
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /EHsc /std:c++20)
else ()
    # PRIVATE specifies that the specified compile options are only for the ${PROJECT_NAME} itself,
    # and not for any targets that link to it.
    # lots of warnings and all warnings as errors
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
endif ()

target_link_libraries(qtpropertybrowser PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)