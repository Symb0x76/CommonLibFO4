include_guard(GLOBAL)

include(CMakeParseArguments)

function(commonlibf4_configure_plugin target_name)
    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR "commonlibf4_configure_plugin requires an existing target: ${target_name}")
    endif()

    set(options)
    set(one_value_args
        AUTHOR
        CONTACT
        DESCRIPTION
        LICENSE
        NAME
        PROJECT_NAME
        PROJECT_VERSION
        VERSION)
    cmake_parse_arguments(COMMONLIBF4_PLUGIN "${options}" "${one_value_args}" "" ${ARGN})

    get_target_property(_target_type "${target_name}" TYPE)
    if(NOT _target_type STREQUAL "SHARED_LIBRARY")
        message(FATAL_ERROR "commonlibf4_configure_plugin expects ${target_name} to be a SHARED library.")
    endif()

    if(NOT COMMONLIBF4_PLUGIN_NAME)
        set(COMMONLIBF4_PLUGIN_NAME "${target_name}")
    endif()

    if(NOT COMMONLIBF4_PLUGIN_VERSION)
        if(PROJECT_VERSION)
            set(COMMONLIBF4_PLUGIN_VERSION "${PROJECT_VERSION}")
        else()
            set(COMMONLIBF4_PLUGIN_VERSION "0.0.0")
        endif()
    endif()

    if(NOT COMMONLIBF4_PLUGIN_PROJECT_NAME)
        if(PROJECT_NAME)
            set(COMMONLIBF4_PLUGIN_PROJECT_NAME "${PROJECT_NAME}")
        else()
            set(COMMONLIBF4_PLUGIN_PROJECT_NAME "CommonLibF4")
        endif()
    endif()

    if(NOT COMMONLIBF4_PLUGIN_PROJECT_VERSION)
        if(PROJECT_VERSION)
            set(COMMONLIBF4_PLUGIN_PROJECT_VERSION "${PROJECT_VERSION}")
        else()
            set(COMMONLIBF4_PLUGIN_PROJECT_VERSION "0.0.0")
        endif()
    endif()

    if(NOT COMMONLIBF4_PLUGIN_LICENSE)
        set(COMMONLIBF4_PLUGIN_LICENSE "Unknown License")
    endif()

    string(REPLACE "." ";" _plugin_version_parts "${COMMONLIBF4_PLUGIN_VERSION}")
    list(LENGTH _plugin_version_parts _plugin_version_count)
    while(_plugin_version_count LESS 3)
        list(APPEND _plugin_version_parts 0)
        list(LENGTH _plugin_version_parts _plugin_version_count)
    endwhile()
    list(GET _plugin_version_parts 0 COMMONLIB_PLUGIN_VERSION_MAJOR)
    list(GET _plugin_version_parts 1 COMMONLIB_PLUGIN_VERSION_MINOR)
    list(GET _plugin_version_parts 2 COMMONLIB_PLUGIN_VERSION_PATCH)
    set(COMMONLIB_PLUGIN_VERSION "${COMMONLIBF4_PLUGIN_VERSION}")

    string(REPLACE "." ";" _project_version_parts "${COMMONLIBF4_PLUGIN_PROJECT_VERSION}")
    list(LENGTH _project_version_parts _project_version_count)
    while(_project_version_count LESS 3)
        list(APPEND _project_version_parts 0)
        list(LENGTH _project_version_parts _project_version_count)
    endwhile()
    list(GET _project_version_parts 0 COMMONLIB_PROJECT_VERSION_MAJOR)
    list(GET _project_version_parts 1 COMMONLIB_PROJECT_VERSION_MINOR)
    list(GET _project_version_parts 2 COMMONLIB_PROJECT_VERSION_PATCH)
    set(COMMONLIB_PROJECT_VERSION "${COMMONLIBF4_PLUGIN_PROJECT_VERSION}")

    set(COMMONLIB_PLUGIN_AUTHOR "${COMMONLIBF4_PLUGIN_AUTHOR}")
    set(COMMONLIB_PLUGIN_CONTACT "${COMMONLIBF4_PLUGIN_CONTACT}")
    set(COMMONLIB_PLUGIN_DESCRIPTION "${COMMONLIBF4_PLUGIN_DESCRIPTION}")
    set(COMMONLIB_PLUGIN_LICENSE "${COMMONLIBF4_PLUGIN_LICENSE}")
    set(COMMONLIB_PLUGIN_NAME "${COMMONLIBF4_PLUGIN_NAME}")
    set(COMMONLIB_PROJECT_NAME "${COMMONLIBF4_PLUGIN_PROJECT_NAME}")

    set(_commonlibf4_module_dir "${CMAKE_CURRENT_LIST_DIR}")
    set(_commonlibf4_cpp_template "${_commonlibf4_module_dir}/templates/commonlibf4-plugin.cpp.in")
    set(_commonlibf4_rc_template "${_commonlibf4_module_dir}/templates/commonlib-plugin.rc.in")

    if(NOT EXISTS "${_commonlibf4_cpp_template}")
        get_filename_component(_commonlibf4_root "${_commonlibf4_module_dir}/.." ABSOLUTE)
        set(_commonlibf4_cpp_template "${_commonlibf4_root}/res/commonlibf4-plugin.cpp.in")
        set(_commonlibf4_rc_template "${_commonlibf4_root}/lib/commonlib-shared/res/commonlib-plugin.rc.in")
    endif()

    if(NOT EXISTS "${_commonlibf4_cpp_template}" OR NOT EXISTS "${_commonlibf4_rc_template}")
        message(FATAL_ERROR "CommonLibF4 plugin templates were not found.")
    endif()

    set(_commonlibf4_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/commonlibf4/${target_name}")
    file(MAKE_DIRECTORY "${_commonlibf4_generated_dir}")

    configure_file(
        "${_commonlibf4_cpp_template}"
        "${_commonlibf4_generated_dir}/commonlibf4-plugin.cpp"
        @ONLY)
    configure_file(
        "${_commonlibf4_rc_template}"
        "${_commonlibf4_generated_dir}/commonlib-plugin.rc"
        @ONLY)

    target_sources("${target_name}"
        PRIVATE
            "${_commonlibf4_generated_dir}/commonlibf4-plugin.cpp"
            "${_commonlibf4_generated_dir}/commonlib-plugin.rc")
    target_link_libraries("${target_name}" PRIVATE CommonLibF4::commonlibf4)
endfunction()
