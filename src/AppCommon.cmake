set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

option(CROSS_LINUX "Cross-compile for Linux (if not native)" off)
option(CROSS_WIN32 "Cross-compile for Win32" off)

if(CROSS_WIN32)
    message("Building for Windows")
    set(CMAKE_SYSTEM_NAME "Windows")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)

    set(CMAKE_C_COMPILER x86_64-w64-mingw32.shared-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32.shared-g++)

    set(WX_PREFIX "${CMAKE_SOURCE_DIR}/../../3rdparty/wxWidgets/install-win32")
elseif(CROSS_LINUX OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("Building for Linux")
    set(CMAKE_SYSTEM_NAME "Linux")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)

    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)

    set(WX_PREFIX "${CMAKE_SOURCE_DIR}/../../3rdparty/wxWidgets/install-linux")
    set(LIB_RPATH "$ORIGIN/../lib")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" ) # macOS
    message("Building for macOS")
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)

    set(WX_PREFIX "${CMAKE_SOURCE_DIR}/../../3rdparty/wxWidgets/install-macOS")
    set(LIB_RPATH "@executable_path/../Frameworks/")
else()
    message(FATAL_ERROR "Unsupported platform or configuration")
endif()


# Use wx-config to get the proper compile and link flags
set(WX_CONFIG "${WX_PREFIX}/bin/wx-config")
execute_process(
    COMMAND ${WX_CONFIG} --cxxflags
    OUTPUT_VARIABLE WX_CXXFLAGS 
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${WX_CONFIG} --libs
    OUTPUT_VARIABLE WX_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("CFlags: ${WX_CXXFLAGS}")
message("LFlags: ${WX_LDFLAGS}")

# Extract and correctly place cxxflags
string(REPLACE " " ";" WX_CXXFLAG_LIST "${WX_CXXFLAGS}")
foreach(FLAG ${WX_CXXFLAG_LIST})
    string(FIND "${FLAG}" "-I" pos)
    if (pos EQUAL -1)
        add_compile_options(${FLAG})
    else()
        string(SUBSTRING ${FLAG} 2 -1 FLAG)
        include_directories(${FLAG})
    endif()
endforeach()
link_libraries(${WX_LDFLAGS})

add_compile_options("-DVERSION=${CMAKE_PROJECT_VERSION}")
add_executable(${APP_TARGET} WIN32 ${SOURCES})

if (DEFINED LIB_RPATH)
    set_target_properties(${APP_TARGET} PROPERTIES
        INSTALL_RPATH ${LIB_RPATH}
        BUILD_WITH_INSTALL_RPATH true
    )
endif()

# Export version value
write_file(VERSION "${CMAKE_PROJECT_VERSION}")

