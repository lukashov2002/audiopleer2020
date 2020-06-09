cmake_minimum_required(VERSION 3.15)
project(sound)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
add_library(fmod INTERFACE)
find_package(Threads)

#
include(FetchContent)
FetchContent_Declare(
        nana
        GIT_REPOSITORY
        https://github.com/cnjinhao/nana.git
        GIT_TAG v1.7.4)

FetchContent_GetProperties(nana)
if (NOT nana_POPULATED)
    FetchContent_Populate(nana)
    add_subdirectory(${nana_SOURCE_DIR} ${nana_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()
#

target_link_directories(fmod INTERFACE "${PROJECT_SOURCE_DIR}/FMOD/lib/x64/")
target_link_libraries(fmod INTERFACE fmod_vc)

set_target_properties(fmod PROPERTIES
        #IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/FMOD/lib/x64/fmod_vc.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/FMOD/inc/"
        )

add_executable(sound main.cpp common_platform.cpp common.cpp)
target_link_libraries(sound PUBLIC fmod Threads::Threads nana::nana)

add_executable(sound_test main_test.cpp functions_for_test.hpp common.cpp common_platform.cpp)
target_link_libraries(sound_test PUBLIC fmod Threads::Threads nana::nana)

enable_testing()
add_test(main_test sound_test)