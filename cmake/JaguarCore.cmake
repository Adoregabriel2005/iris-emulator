# Explicitly link the real processors and peripherals. Link-only substitutes must
# never be part of a runnable emulator (nor of its regression tests).
set(JAG_DIR "${CMAKE_CURRENT_LIST_DIR}/../JAGUAR REFERENCE 1/src")
set(JAG_CORE_FILES
    blitter cdintf cdrom crc32 dsp eeprom event filedb gpu jagbios jagbios2
    jagcdbios jagdevcdbios jagstub1bios jagstub2bios jagdasm jaguar jerry
    joystick log memory memtrack mmu modelsBIOS op tom universalhdr wavetable)
set(JAG_CORE_SOURCES)
foreach(name IN LISTS JAG_CORE_FILES)
    list(APPEND JAG_CORE_SOURCES "${JAG_DIR}/${name}.cpp")
endforeach()
add_library(iris_jaguar STATIC ${JAG_CORE_SOURCES}
    "${JAG_DIR}/m68000/m68kinterface.c"
    "${JAG_DIR}/m68000/cpuextra.c"
    "${JAG_DIR}/m68000/readcpu.c"
    "${JAG_DIR}/m68000/m68kdasm.c"
    "${JAG_DIR}/m68000/obj/cpuemu.c"
    "${JAG_DIR}/m68000/obj/cpustbl.c"
    "${JAG_DIR}/m68000/obj/cpudefs.c"
    "${CMAKE_CURRENT_LIST_DIR}/../src/jaguar/JaguarHost.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/jaguar/JaguarAudio.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/jaguar/JaguarFileLoader.cpp")
set_target_properties(iris_jaguar PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(iris_jaguar PUBLIC "${JAG_DIR}" "${JAG_DIR}/m68000"
    "${CMAKE_CURRENT_LIST_DIR}/../src/jaguar")
target_compile_definitions(iris_jaguar PRIVATE IRIS_SINGLE_THREAD=1)
if(MSVC)
    target_compile_options(iris_jaguar PRIVATE /Zc:strictStrings- /wd4996)
else()
    target_compile_options(iris_jaguar PRIVATE -Wno-write-strings)
endif()
if(IRIS_BUILD_APP)
    find_package(ZLIB REQUIRED)
    target_sources(iris_jaguar PRIVATE "${JAG_DIR}/unzip.cpp")
    target_compile_definitions(iris_jaguar PRIVATE IRIS_JAGUAR_ZIP=1)
    target_link_libraries(iris_jaguar PRIVATE ZLIB::ZLIB)
endif()
