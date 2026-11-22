# Copyright © 2021 CCP ehf.
cmake_minimum_required(VERSION 3.16)

project(Exefile VERSION 1.0.0)

include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpDocsGenerator.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpBuildConfigurations.cmake)

find_package(crashpad CONFIG REQUIRED)
find_package(carbon-core CONFIG REQUIRED)
find_package(Python3 COMPONENTS Development REQUIRED)

# Windows and macOS keep both platform files. Linux compiles only its own
# platform files, so Objective-C++ is not enabled on this host.
set(PLATFORM_SOURCES
        ExeFileOsX.mm
        ExeFileWin32.cpp
        FileSystemOsX.mm
        FileSystemWin32.cpp
        )
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_SOURCES
            ExeFileLinux.cpp
            FileSystemLinux.cpp
            )
endif()

ccp_add_executable(Exefile
        autoversion.h
        BlueInterface.cpp
        BlueInterface.h
        CommandArguments.cpp
        CommandArguments.h
        Crashpad.cpp
        Crashpad.h
        ExeFile.cpp
        ExeFile.h
        ${PLATFORM_SOURCES}
        FileSystem.h
        resource.h
        StdAfx.cpp
        StdAfx.h
        version.h
        # Icon resource file on Windows, ignored on macOS
        ExeFile.rc
        )

if(WIN32)
    # manifest file contains supported OS versions for windows
    target_sources(Exefile PRIVATE .manifest)
endif()

target_precompile_headers(Exefile PRIVATE StdAfx.h)

set_target_properties(Exefile PROPERTIES OUTPUT_NAME exefile)

if(WIN32)
    target_link_libraries(Exefile PRIVATE Winmm)
    set_target_properties(Exefile PROPERTIES WIN32_EXECUTABLE True)
endif()

if(APPLE)
    enable_language(OBJCXX)
    target_link_libraries(Exefile PRIVATE "-framework Foundation")
    set_target_properties(
            Exefile
            PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@executable_path/."
            # turn off IPO/LTO for macOS because it has issues with mixing opaque and non-opaque pointers
            INTERPROCEDURAL_OPTIMIZATION OFF
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(Exefile PRIVATE ${CMAKE_DL_LIBS})
    set_target_properties(
            Exefile
            PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "\$ORIGIN"
            INSTALL_RPATH_USE_LINK_PATH FALSE
            # Crashpad's static libraries in this closure are ordinary objects.
            INTERPROCEDURAL_OPTIMIZATION OFF
    )
endif()

target_compile_definitions(Exefile PRIVATE BLUEBUILD_STATIC CRASH_REPORTS_ENABLED)

target_link_libraries(Exefile PUBLIC CcpCore crashpad::crashpad)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Extensions are separate modules. The static interpreter has to be complete
    # and visible, or their Python calls bind nowhere.
    target_include_directories(Exefile PRIVATE ${Python3_INCLUDE_DIRS})
    target_link_options(Exefile PRIVATE
        "SHELL:-Wl,--export-dynamic"
        "SHELL:-Wl,--whole-archive ${Python3_LIBRARIES} -Wl,--no-whole-archive")
else()
    target_link_libraries(Exefile PUBLIC Python3::Python)
endif()

target_include_directories(Exefile
        PRIVATE
        ${BLUE_INCLUDE_DIRECTORY}
        )

set_target_properties(Exefile PROPERTIES OUTPUT_NAME "exefile")
set_target_properties(Exefile PROPERTIES CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "exefile")

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set (INSTALL_TO_MONOLITH OFF CACHE BOOL "Install this package to a perforce branch vendor folder - provided for backwards compatability")

    get_target_property(crashpad_handler_location crashpad::handler IMPORTED_LOCATION_RELEASE)
    if(NOT crashpad_handler_location)
        get_target_property(crashpad_handler_location crashpad::handler IMPORTED_LOCATION)
    endif()

    if (INSTALL_TO_MONOLITH)
        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        if(crashpad_handler_location)
        install(
                IMPORTED_RUNTIME_ARTIFACTS crashpad::handler
                DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        )
        endif()
        install(
                TARGETS Exefile
                EXPORT ExefileTargets
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        )
    else() # "VCPKG" INSTALL_MODE, but there's no need for another option, as this will be the default
        if(crashpad_handler_location)
        install(IMPORTED_RUNTIME_ARTIFACTS crashpad::handler)
        endif()
        install(
                TARGETS Exefile
                EXPORT ExefileTargets
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                RUNTIME DESTINATION bin/
        )
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(ExefileLinuxHostBehavior
            tests/linux_host_behavior.cpp
            CommandArguments.cpp
            FileSystemLinux.cpp
            ExeFileLinux.cpp
            Crashpad.cpp
            StdAfx.cpp
            )
    target_include_directories(ExefileLinuxHostBehavior PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_precompile_headers(ExefileLinuxHostBehavior PRIVATE StdAfx.h)
    target_compile_definitions(ExefileLinuxHostBehavior PRIVATE BLUEBUILD_STATIC CRASH_REPORTS_ENABLED FE8_EXEFILE_HOST_TEST)
    target_link_libraries(ExefileLinuxHostBehavior PRIVATE CcpCore crashpad::crashpad Python3::Python ${CMAKE_DL_LIBS})
    set_target_properties(
            ExefileLinuxHostBehavior
            PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "\$ORIGIN"
            INSTALL_RPATH_USE_LINK_PATH FALSE
            INTERPROCEDURAL_OPTIMIZATION OFF
    )
    enable_testing()
    add_test(NAME ExefileLinuxHostBehavior COMMAND ExefileLinuxHostBehavior)
    set_tests_properties(ExefileLinuxHostBehavior PROPERTIES TIMEOUT 60)
endif()
