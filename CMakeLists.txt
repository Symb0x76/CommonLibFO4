cmake_minimum_required(VERSION 3.24)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR AND NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _commonlibf4_vcpkg_root)
    set(_commonlibf4_vcpkg_toolchain "${_commonlibf4_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
    if(EXISTS "${_commonlibf4_vcpkg_toolchain}")
        set(CMAKE_TOOLCHAIN_FILE "${_commonlibf4_vcpkg_toolchain}" CACHE FILEPATH "vcpkg toolchain file")
    endif()
endif()

project(CommonLibF4 VERSION 0.0.0 LANGUAGES CXX RC)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(NOT WIN32)
    message(FATAL_ERROR "CommonLibF4 only supports Windows targets.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "CommonLibF4 requires a 64-bit toolchain.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "CommonLibF4 requires MSVC or clang-cl.")
endif()

option(COMMONLIBF4_INSTALL "Install CommonLibF4 targets and package exports" ${PROJECT_IS_TOP_LEVEL})
option(COMMONLIBF4_ENABLE_INI "Enable REX::INI settings support" OFF)
option(COMMONLIBF4_ENABLE_JSON "Enable REX::JSON settings support" OFF)
option(COMMONLIBF4_ENABLE_TOML "Enable REX::TOML settings support" OFF)
option(COMMONLIBF4_ENABLE_XBYAK "Enable xbyak support for REL::Trampoline" OFF)

add_subdirectory(lib/commonlib-shared)

file(GLOB_RECURSE commonlibf4_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE commonlibf4_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_library(commonlibf4 STATIC)
add_library(CommonLibF4::commonlibf4 ALIAS commonlibf4)

target_sources(commonlibf4
    PRIVATE
        ${commonlibf4_sources}
        ${commonlibf4_headers})

target_compile_features(commonlibf4 PUBLIC cxx_std_23)
target_include_directories(commonlibf4
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(commonlibf4 PUBLIC commonlib-shared)
target_precompile_headers(commonlibf4
    PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include/F4SE/Impl/PCH.h>")

if(MSVC)
    target_compile_options(commonlibf4 PRIVATE /utf-8)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${commonlibf4_sources} ${commonlibf4_headers})

if(COMMONLIBF4_INSTALL)
    install(TARGETS commonlibf4
        EXPORT CommonLibF4Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/include/F4SE"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/RE"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/Scaleform"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/res/commonlibf4.natvis"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/CommonLibF4")

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CommonLibF4Plugin.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CommonLibF4")

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/res/commonlibf4-plugin.cpp.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/commonlib-shared/res/commonlib-plugin.rc.in"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CommonLibF4/templates")

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CommonLibF4Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CommonLibF4Config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CommonLibF4")

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/CommonLibF4ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CommonLibF4Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CommonLibF4ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CommonLibF4")

    install(EXPORT CommonLibF4Targets
        FILE CommonLibF4Targets.cmake
        NAMESPACE CommonLibF4::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CommonLibF4")
endif()
