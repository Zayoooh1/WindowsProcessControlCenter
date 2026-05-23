cmake_minimum_required(VERSION 3.24)

project(WindowsProcessControlCenter
    VERSION 0.1.0
    DESCRIPTION "Modern Windows desktop shell for process control workflows"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WEBVIEW2_VERSION "1.0.3967.48")
set(WEBVIEW2_PACKAGE_DIR "${CMAKE_BINARY_DIR}/_deps/webview2-${WEBVIEW2_VERSION}")
set(WEBVIEW2_PACKAGE_FILE "${CMAKE_BINARY_DIR}/_deps/microsoft.web.webview2.${WEBVIEW2_VERSION}.nupkg")
set(WEBVIEW2_PACKAGE_ZIP "${CMAKE_BINARY_DIR}/_deps/microsoft.web.webview2.${WEBVIEW2_VERSION}.zip")
set(WEBVIEW2_PACKAGE_URL "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/${WEBVIEW2_VERSION}/microsoft.web.webview2.${WEBVIEW2_VERSION}.nupkg")

if(NOT EXISTS "${WEBVIEW2_PACKAGE_DIR}/build/native/include/WebView2.h")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps")
    message(STATUS "Downloading Microsoft.Web.WebView2 ${WEBVIEW2_VERSION}")
    file(DOWNLOAD
        "${WEBVIEW2_PACKAGE_URL}"
        "${WEBVIEW2_PACKAGE_FILE}"
        SHOW_PROGRESS
        STATUS WEBVIEW2_DOWNLOAD_STATUS
    )
    list(GET WEBVIEW2_DOWNLOAD_STATUS 0 WEBVIEW2_DOWNLOAD_RESULT)
    if(NOT WEBVIEW2_DOWNLOAD_RESULT EQUAL 0)
        list(GET WEBVIEW2_DOWNLOAD_STATUS 1 WEBVIEW2_DOWNLOAD_MESSAGE)
        message(FATAL_ERROR "Failed to download WebView2 SDK: ${WEBVIEW2_DOWNLOAD_MESSAGE}")
    endif()

    file(COPY_FILE "${WEBVIEW2_PACKAGE_FILE}" "${WEBVIEW2_PACKAGE_ZIP}" ONLY_IF_DIFFERENT)
    file(REMOVE_RECURSE "${WEBVIEW2_PACKAGE_DIR}")
    file(ARCHIVE_EXTRACT INPUT "${WEBVIEW2_PACKAGE_ZIP}" DESTINATION "${WEBVIEW2_PACKAGE_DIR}")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WEBVIEW2_ARCH "x64")
else()
    set(WEBVIEW2_ARCH "x86")
endif()

set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_PACKAGE_DIR}/build/native/include")
set(WEBVIEW2_LOADER_LIB "${WEBVIEW2_PACKAGE_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2LoaderStatic.lib")

set(LEGACY_IMGUI_SOURCES
    src/platform/D3D11Renderer.cpp
    src/platform/D3D11Renderer.h
    src/ui/UiLayer.cpp
    src/ui/UiLayer.h
    src/ui/UiTheme.cpp
    src/ui/UiTheme.h
)

add_executable(WindowsProcessControlCenter WIN32
    src/main.cpp
    src/app/Application.cpp
    src/app/Application.h
    src/core/ProcessInfo.h
    src/core/GpuPreferenceManager.cpp
    src/core/GpuPreferenceManager.h
    src/core/ProcessActions.cpp
    src/core/ProcessActions.h
    src/core/ProcessProvider.cpp
    src/core/ProcessProvider.h
    src/platform/Win32Window.cpp
    src/platform/Win32Window.h
    src/ui_web/WebMessageBridge.cpp
    src/ui_web/WebMessageBridge.h
    src/ui_web/WebViewHost.cpp
    src/ui_web/WebViewHost.h
)

target_include_directories(WindowsProcessControlCenter PRIVATE
    src
    ${WEBVIEW2_INCLUDE_DIR}
)

target_compile_definitions(WindowsProcessControlCenter PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

if(MSVC)
    target_compile_options(WindowsProcessControlCenter PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
endif()

target_link_libraries(WindowsProcessControlCenter PRIVATE
    ${WEBVIEW2_LOADER_LIB}
    advapi32
    ole32
    shell32
    shlwapi
    user32
    version
)

set_target_properties(WindowsProcessControlCenter PROPERTIES
    OUTPUT_NAME "WindowsProcessControlCenter"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:WindowsProcessControlCenter>"
)

add_custom_command(TARGET WindowsProcessControlCenter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/web"
        "$<TARGET_FILE_DIR:WindowsProcessControlCenter>/web"
    COMMENT "Copying WebView frontend assets"
)
