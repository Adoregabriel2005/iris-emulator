cmake_minimum_required(VERSION 3.16)
project(IrisEmulator VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(IRIS_BUILD_APP "Build the Qt frontend (requires Qt, SDL2 and Gearlynx)" ON)
include(CTest)
include(cmake/JaguarCore.cmake)
if(BUILD_TESTING)
    add_executable(test_jaguar tools/test_jaguar.cpp)
    set_target_properties(test_jaguar PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    target_link_libraries(test_jaguar PRIVATE iris_jaguar)
    add_test(NAME jaguar_core COMMAND test_jaguar)
    add_executable(jaguar_probe tools/jaguar_probe.cpp)
    set_target_properties(jaguar_probe PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    target_link_libraries(jaguar_probe PRIVATE iris_jaguar)
endif()
if(NOT IRIS_BUILD_APP)
    return()
endif()

set(GEARLYNX_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/build/src" CACHE PATH "Gearlynx source directory")
file(GLOB GEARLYNX_SRC "${GEARLYNX_INCLUDE_DIR}/*.cpp")
if(NOT EXISTS "${GEARLYNX_INCLUDE_DIR}/gearlynx.h")
    message(FATAL_ERROR "Set GEARLYNX_INCLUDE_DIR to the Gearlynx src directory")
endif()

set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "" FORCE)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "" FORCE)

find_package(Qt6 COMPONENTS Widgets Svg Network OpenGL OpenGLWidgets REQUIRED)
find_package(SDL2 REQUIRED)
find_package(ZLIB REQUIRED)

find_path(DISCORD_RPC_INCLUDE_DIR discord_rpc.h PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include" REQUIRED)
find_library(DISCORD_RPC_LIB discord-rpc PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" REQUIRED)

qt_add_executable(irisemulator
    src/main.cpp
    src/resources/iris_emulator.ico
    iris_emulator.rc
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.ui
    src/ui/DisplayWidget.cpp
    src/ui/DisplayWidget.h
    src/ui/OverlayWidget.cpp
    src/ui/OverlayWidget.h
    src/ui/ControllerSettingsDialog.cpp
    src/ui/ControllerSettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/Themes.cpp
    src/ui/Themes.h
    src/ui/GameList/GameListWidget.cpp
    src/ui/GameList/GameListWidget.h
    src/ui/GameList/GameListWidget.ui
    src/ui/GameList/GameListModel.cpp
    src/ui/GameList/GameListModel.h
    src/ui/CoverDownloadDialog.cpp
    src/ui/CoverDownloadDialog.h
    src/ui/DebugWindow.cpp
    src/ui/DebugWindow.h
    src/ui/JaguarDebugWindow.cpp
    src/ui/JaguarDebugWindow.h
    src/ui/EmulatorLogWindow.cpp
    src/ui/EmulatorLogWindow.h
    src/ui/WelcomeDialog.cpp
    src/ui/WelcomeDialog.h
    src/ui/AboutDialog.cpp
    src/ui/AboutDialog.h
    src/EmulatorCore.cpp
    src/SDLInput.cpp
    src/SDLInput_Jaguar.cpp
    src/CPU6507.cpp
    src/TIA.cpp
    src/RIOT.cpp
    src/mapper/MapperNone.cpp
    src/mapper/MapperBanked.cpp
    src/MapperFactory.cpp
    src/mapper/MapperF8.h
    src/mapper/MapperF6.h
    src/mapper/MapperF4.h
    src/mapper/MapperE0.h
    src/mapper/MapperE7.h
    src/mapper/MapperSC.h
    src/mapper/Mapper3F.h
    src/mapper/Mapper3E.h
    src/mapper/MapperFA.h
    src/mapper/MapperFE.h
    src/jaguar/JaguarSystem.cpp
    src/jaguar/JaguarSystem.h
    src/jaguar/JaguarPatchDB.h
    src/lynx/LynxSystem.cpp
    src/lynx/LynxSystem.h
    src/IEmulatorCore.h
    src/DiscordRPC.cpp
    src/DiscordRPC.h
    ${GEARLYNX_SRC}
    src/resources/icons/resources.qrc
)

target_include_directories(irisemulator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/lynx
    ${CMAKE_SOURCE_DIR}/src/audio
    ${CMAKE_SOURCE_DIR}/src/audio/vst3sdk
    ${CMAKE_SOURCE_DIR}/src/mapper
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/GameList
    "${JAG_DIR}"
    "${JAG_DIR}/m68000"
    ${CMAKE_SOURCE_DIR}/src/jaguar
    ${GEARLYNX_INCLUDE_DIR}
    ${DISCORD_RPC_INCLUDE_DIR}
    $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(irisemulator PRIVATE Qt6::Widgets Qt6::Svg Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets SDL2::SDL2 ${DISCORD_RPC_LIB} ZLIB::ZLIB iris_jaguar)

get_filename_component(DISCORD_RPC_LIB_DIR ${DISCORD_RPC_LIB} DIRECTORY)
get_filename_component(VCPKG_INSTALLED_DIR ${DISCORD_RPC_LIB_DIR} DIRECTORY)
set(DISCORD_RPC_DLL "${VCPKG_INSTALLED_DIR}/bin/discord-rpc.dll")
if(EXISTS ${DISCORD_RPC_DLL})
    add_custom_command(TARGET irisemulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DISCORD_RPC_DLL}"
            "$<TARGET_FILE_DIR:irisemulator>"
    )
endif()

set(ZLIB_DLL "${VCPKG_INSTALLED_DIR}/bin/zlib1.dll")
if(EXISTS ${ZLIB_DLL})
    add_custom_command(TARGET irisemulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ZLIB_DLL}"
            "$<TARGET_FILE_DIR:irisemulator>"
    )
endif()

install(TARGETS irisemulator RUNTIME DESTINATION bin)

# Test tool for savestates
add_executable(test_savestate
    tools/test_savestate.cpp
    src/EmulatorCore.cpp
    src/IEmulatorCore.h
    src/CPU6507.cpp
    src/TIA.cpp
    src/RIOT.cpp
    src/mapper/MapperNone.cpp
    src/mapper/MapperBanked.cpp
    src/MapperFactory.cpp
    src/jaguar/JaguarSystem.cpp
    src/lynx/LynxSystem.cpp
    src/DiscordRPC.cpp
    ${GEARLYNX_SRC}
)
target_include_directories(test_savestate PRIVATE 
    ${CMAKE_SOURCE_DIR}/src 
    ${CMAKE_SOURCE_DIR}/src/mapper
    ${CMAKE_SOURCE_DIR}/src/lynx
    ${CMAKE_SOURCE_DIR}/src/jaguar
    ${CMAKE_SOURCE_DIR}/src/audio
    "${JAG_DIR}"
    "${JAG_DIR}/m68000"
    ${GEARLYNX_INCLUDE_DIR}
    ${DISCORD_RPC_INCLUDE_DIR}
    $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(test_savestate PRIVATE Qt6::Widgets Qt6::Svg Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets SDL2::SDL2 ${DISCORD_RPC_LIB} ZLIB::ZLIB iris_jaguar)
