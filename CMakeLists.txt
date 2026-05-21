cmake_minimum_required(VERSION 3.16)
project(sonopix VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SFML REQUIRED sfml-all)
pkg_check_modules(Lua REQUIRED lua>5.4)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/MainWindow.cpp
    src/AudioEngine.cpp
    src/Effects.cpp
    src/shaders/image_effects.cpp
)

add_compile_definitions(APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${SFML_INCLUDE_DIRS}
    ${Lua_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SFML_LIBRARIES}
    ${Lua_LIBRARIES}
)

