cmake_minimum_required(VERSION 3.16)
project(IrisEmulator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB GEARLYNX_SRC "${CMAKE_SOURCE_DIR}/build/src/*.cpp")
set(GEARLYNX_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/build/src")

# Force Qt6 from official installer (not vcpkg) to avoid DLL conflicts
set(Qt6_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6" CACHE PATH "" FORCE)
set(Qt6CoreTools_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6CoreTools" CACHE PATH "" FORCE)
set(Qt6GuiTools_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6GuiTools" CACHE PATH "" FORCE)
set(Qt6WidgetsTools_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6WidgetsTools" CACHE PATH "" FORCE)
set(Qt6Core_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6Core" CACHE PATH "" FORCE)
set(Qt6Gui_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6Gui" CACHE PATH "" FORCE)
set(Qt6Widgets_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6Widgets" CACHE PATH "" FORCE)
set(Qt6Svg_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6Svg" CACHE PATH "" FORCE)
set(Qt6Network_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6Network" CACHE PATH "" FORCE)
set(Qt6EntryPointPrivate_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6EntryPointPrivate" CACHE PATH "" FORCE)
set(Qt6ZlibPrivate_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6ZlibPrivate" CACHE PATH "" FORCE)

# Disable vcpkg auto DLL copy — it copies vcpkg Qt DLLs that conflict with official Qt
set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "" FORCE)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "" FORCE)

find_package(Qt6 COMPONENTS Widgets Svg Network REQUIRED)
find_package(SDL2 REQUIRED)

# discord-rpc via vcpkg (sem CMake config, usar find_library)
find_path(DISCORD_RPC_INCLUDE_DIR discord_rpc.h PATHS "D:/vcpkg/installed/x64-windows/include" REQUIRED)
find_library(DISCORD_RPC_LIB discord-rpc PATHS "D:/vcpkg/installed/x64-windows/lib" REQUIRED)

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
    src/EmulatorCore.cpp
    src/SDLInput.cpp
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
    src/lynx/LynxSystem.cpp
    src/lynx/LynxSystem.h
    src/IEmulatorCore.h
    src/DiscordRPC.cpp
    src/DiscordRPC.h
    src/ui/CoverDownloadDialog.cpp
    src/ui/CoverDownloadDialog.h
    src/ui/DebugWindow.cpp
    src/ui/DebugWindow.h
    ${GEARLYNX_SRC}
    src/resources/icons/resources.qrc
)

target_include_directories(irisemulator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/lynx
    ${CMAKE_SOURCE_DIR}/src/mapper
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/GameList
    ${GEARLYNX_INCLUDE_DIR}
    ${DISCORD_RPC_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(irisemulator PRIVATE Qt6::Widgets Qt6::Svg Qt6::Network SDL2::SDL2 ${DISCORD_RPC_LIB})

qt_add_executable(test_savestate
    tools/test_savestate.cpp
    src/EmulatorCore.cpp
    src/IEmulatorCore.h
    src/CPU6507.cpp
    src/TIA.cpp
    src/RIOT.cpp
    src/mapper/MapperNone.cpp
    src/mapper/MapperBanked.cpp
    src/MapperFactory.cpp
)
target_include_directories(test_savestate PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/mapper)
target_link_libraries(test_savestate PRIVATE Qt6::Widgets SDL2::SDL2)

install(TARGETS irisemulator RUNTIME DESTINATION bin)
