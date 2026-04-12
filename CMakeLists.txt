cmake_minimum_required(VERSION 3.16)
project(IrisEmulator VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB GEARLYNX_SRC "${CMAKE_SOURCE_DIR}/build/src/*.cpp")
set(GEARLYNX_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/build/src")

# Jaguar core (Virtual Jaguar RX reference)
set(JAGUAR_REF_DIR "${CMAKE_SOURCE_DIR}/JAGUARCOREREFERENCE/Virtual-Jaguar-Rx-master/src")

file(GLOB JAGUAR_SRC
    "${JAGUAR_REF_DIR}/*.cpp"
)
# Exclude reference GUI/debug/ELF dependent units not needed in Iris runtime
list(FILTER JAGUAR_SRC EXCLUDE REGEX ".*/debugger/.*")
set(M68K_SRC
    "${JAGUAR_REF_DIR}/m68000/m68kinterface.c"
    "${JAGUAR_REF_DIR}/m68000/cpuextra.c"
    "${JAGUAR_REF_DIR}/m68000/readcpu.c"
    "${JAGUAR_REF_DIR}/m68000/m68kdasm.c"
    "${JAGUAR_REF_DIR}/m68000/obj/cpuemu.c"
    "${JAGUAR_REF_DIR}/m68000/obj/cpustbl.c"
    "${JAGUAR_REF_DIR}/m68000/obj/cpudefs.c"
)

# VST3 SDK minimal sources needed for IID definitions and FUID
set(VST3_SDK_DIR "${CMAKE_SOURCE_DIR}/src/audio/vst3sdk")
set(VST3_SRC
    "${VST3_SDK_DIR}/pluginterfaces/base/funknown.cpp"
    "${VST3_SDK_DIR}/pluginterfaces/base/conststringtable.cpp"
    "${VST3_SDK_DIR}/pluginterfaces/base/ustring.cpp"
    "${CMAKE_SOURCE_DIR}/src/audio/vst3_iids.cpp"
)
set(JAGUAR_INCLUDE_DIR "${JAGUAR_REF_DIR}")

# Suppress const-string and other legacy C warnings in the VJ core
# Also disable the interactive debug dialogs (QMessageBox popups) and
# the Alpine/Debugger features that require the VJ GUI
if(MSVC)
    set_source_files_properties(${JAGUAR_SRC} PROPERTIES
        COMPILE_OPTIONS "/Zc:strictStrings-;/wd4996;/wd4244;/wd4267"
        COMPILE_DEFINITIONS "ALPINE_FUNCTIONS=0;CPU_DEBUG_MEMORY=0;CPU_DEBUG_TRACING=0;IRIS_HEADLESS=1"
    )
endif()

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

set(Qt6OpenGL_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6OpenGL" CACHE PATH "" FORCE)
set(Qt6OpenGLWidgets_DIR "D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6OpenGLWidgets" CACHE PATH "" FORCE)

# Disable vcpkg auto DLL copy — it copies vcpkg Qt DLLs that conflict with official Qt
set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "" FORCE)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "" FORCE)

find_package(Qt6 COMPONENTS Widgets Svg Network OpenGL OpenGLWidgets REQUIRED)
find_package(SDL2 REQUIRED)
find_package(ZLIB REQUIRED)

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
    src/mapper/Mapper3F.h
    src/mapper/Mapper3E.h
    src/mapper/MapperFA.h
    src/mapper/MapperFE.h
    src/lynx/LynxSystem.cpp
    src/lynx/LynxSystem.h
    src/jaguar/JaguarSystem.cpp
    src/jaguar/JaguarSystem.h
    src/audio/VSTHost.cpp
    src/audio/VSTHost.h
    src/IEmulatorCore.h
    src/DiscordRPC.cpp
    src/DiscordRPC.h
    src/ui/CoverDownloadDialog.cpp
    src/ui/CoverDownloadDialog.h
    src/ui/DebugWindow.cpp
    src/ui/DebugWindow.h
    src/ui/WelcomeDialog.cpp
    src/ui/WelcomeDialog.h
    src/ui/AboutDialog.cpp
    src/ui/AboutDialog.h
    src/ui/JaguarIntroWidget.cpp
    src/ui/JaguarIntroWidget.h
    ${GEARLYNX_SRC}
    ${JAGUAR_SRC}
    ${M68K_SRC}
    ${VST3_SRC}
    src/resources/icons/resources.qrc
)

target_include_directories(irisemulator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/lynx
    ${CMAKE_SOURCE_DIR}/src/jaguar
    ${CMAKE_SOURCE_DIR}/src/audio
    ${CMAKE_SOURCE_DIR}/src/audio/vst3sdk
    ${CMAKE_SOURCE_DIR}/src/mapper
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/GameList
    ${GEARLYNX_INCLUDE_DIR}
    ${JAGUAR_INCLUDE_DIR}
    ${JAGUAR_INCLUDE_DIR}/m68000
    ${CMAKE_SOURCE_DIR}/src/jaguar
    ${DISCORD_RPC_INCLUDE_DIR}
    $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(irisemulator PRIVATE Qt6::Widgets Qt6::Svg Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets SDL2::SDL2 ${DISCORD_RPC_LIB} ZLIB::ZLIB)

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
