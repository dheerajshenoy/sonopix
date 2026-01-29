cmake_minimum_required(VERSION 3.16)
project(sonify VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SFML REQUIRED sfml-all)

add_executable(sonify
    src/main.cpp
    src/MainWindow.cpp
)

add_compile_definitions(APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
)

target_include_directories(sonify PRIVATE ${CMAKE_SOURCE_DIR}/include)


target_include_directories(sonify PRIVATE ${SFML_INCLUDE_DIRS})
target_link_libraries(sonify PRIVATE ${SFML_LIBRARIES})

