# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS "hq_components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_definitions("PROJECT_PARAMETERS=1")
project(main)
