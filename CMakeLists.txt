project(xournal)
cmake_minimum_required(VERSION 3.1)

set(MAIN_LIB_TARGET "libxournal")
set(MAIN_EXE_TARGET "xournal")

find_package(GTK2 2.10.0 REQUIRED)
find_package(PkgConfig REQUIRED)

#function(target_requires_pkg PARAM_TARGET_NAME PARAM_PKG_NAME PARAM_PKG_PREFIX)
#    pkg_check_modules(${PARAM_PKG_PREFIX} ${PARAM_PKG_NAME})
#
#
#endfunction()

  
pkg_check_modules(GNOME_CANVAS "libgnomecanvas-2.0 >= 2.4.0")
pkg_check_modules(POPPER_GLIB "poppler-glib >= 0.5.4")
pkg_check_modules(PANGOFT2 "pangoft2 >= 1.0")

#the name of the file containing main() that shouldn't be included in MAIN_LIB_TARGET
set(MAIN_SRC_FILE ${CMAKE_SOURCE_DIR}/src/main.c)

#WARNING: CMAKE WON'T BE ABLE TO DETECT WHEN NEW SOURCE FILES ARE ADDED!
#don't recurse in src/
FILE(GLOB MAIN_LIB_SRC src/*.cpp src/*.c src/*.cxx)
LIST(REMOVE_ITEM MAIN_LIB_SRC ${MAIN_SRC_FILE})


target_include_directories(${MAIN_LIB_TARGET} PRIVATE ${GTK2_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/src)
#include_directories(${CMAKE_SOURCE_DIR}/include)

#make sure we can find the pkg-config libraries
link_directories(${GNOME_CANVAS_LIBRARY_DIRS})
link_directories(${POPPLER_GLIB_LIBRARY_DIRS})
link_directories(${PANGOFT2_LIBRARY_DIRS})

target_include_directories(${MAIN_LIB_TARGET} PRIVATE ${})
target_include_directories(${MAIN_LIB_TARGET} PRIVATE ${GNOME_CANVAS_INCLUDEDIR})
target_include_directories(${MAIN_LIB_TARGET} PRIVATE ${POPPER_GLIB_INCLUDEDIR})
target_include_directories(${MAIN_LIB_TARGET} PRIVATE ${PANGOFT2_INCLUDEDIR})


add_library(${MAIN_LIB_TARGET} ${MAIN_LIB_SRC})


add_executable(${MAIN_EXE_TARGET} ${MAIN_SRC_FILE})

target_link_libraries(${MAIN_EXE_TARGET} ${MAIN_LIB_TARGET})
target_link_libraries(${MAIN_LIB_TARGET} PRIVATE ${GTK2_LIBRARIES})
target_add_definitions(${MAIN_LIB_TARGET} ${GTK2_DEFINITIONS})
