# The following four lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
# set(EXTRA_COMPONENT_DIRS main build)
# project(NVS_http)
# # Укажите имя проекта и язык C++
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)


project(NVS_http)
