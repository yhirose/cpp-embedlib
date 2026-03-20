# cpp-embedlib: CMake macro for embedding files into C++ binaries.
# SPDX-License-Identifier: MIT
#
# Usage:
#   include(cpp-embedlib)
#   cpp_embedlib_add(MyAssets FOLDER www/ NAMESPACE Web)

# Remember the directory where this script lives (for locating the generate script)
set(_CPP_EMBEDLIB_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

# Header-only integration for cpp-httplib (requires httplib::httplib to be available)
if(NOT TARGET cpp-embedlib-httplib)
    add_library(cpp-embedlib-httplib INTERFACE)
    target_include_directories(cpp-embedlib-httplib INTERFACE
        "${_CPP_EMBEDLIB_CMAKE_DIR}/../integrations"
        "${_CPP_EMBEDLIB_CMAKE_DIR}/../include"
    )
endif()

# Short hash for identifier collision resolution
function(_cpp_embedlib_short_hash _str _out_var)
    string(MD5 _hash "${_str}")
    string(SUBSTRING "${_hash}" 0 8 _short_hash)
    set(${_out_var} "${_short_hash}" PARENT_SCOPE)
endfunction()

function(cpp_embedlib_add _target_name)
    cmake_parse_arguments(_ARG "" "FOLDER;NAMESPACE;BASE_DIR" "FILES;MIME_TYPES" ${ARGN})

    # Default namespace to target name
    if(NOT _ARG_NAMESPACE)
        set(_ARG_NAMESPACE "${_target_name}")
    endif()

    # Validate namespace: single identifier or nested (e.g. Foo::Bar)
    if(NOT _ARG_NAMESPACE MATCHES "^[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*$")
        message(FATAL_ERROR
            "cpp-embedlib: NAMESPACE '${_ARG_NAMESPACE}' is not a valid C++ namespace name.")
    endif()

    # Collect files
    set(_all_files "")

    if(_ARG_FOLDER)
        # Normalize folder path
        cmake_path(SET _folder_path NORMALIZE "${_ARG_FOLDER}")
        file(GLOB_RECURSE _folder_files
            CONFIGURE_DEPENDS
            "${_folder_path}/*"
        )
        list(APPEND _all_files ${_folder_files})
    endif()

    if(_ARG_FILES)
        list(APPEND _all_files ${_ARG_FILES})
    endif()

    # Remove duplicates
    list(REMOVE_DUPLICATES _all_files)

    # Check for empty file list
    list(LENGTH _all_files _file_count)
    if(_file_count EQUAL 0)
        if(_ARG_FOLDER)
            message(FATAL_ERROR
                "cpp-embedlib: no files found in FOLDER '${_ARG_FOLDER}'. "
                "Check the path or use FILES instead.")
        else()
            message(FATAL_ERROR
                "cpp-embedlib: no files specified. Use FOLDER or FILES.")
        endif()
    endif()

    # Determine base directory for relative paths
    if(_ARG_BASE_DIR)
        cmake_path(SET _base_dir NORMALIZE "${_ARG_BASE_DIR}")
    elseif(_ARG_FOLDER)
        set(_base_dir "${_folder_path}")
    else()
        set(_base_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # Output directory for generated files
    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/cpp-embedlib/${_target_name}")
    file(MAKE_DIRECTORY "${_gen_dir}")

    # Generate script path
    set(_generate_script "${_CPP_EMBEDLIB_CMAKE_DIR}/cpp-embedlib-generate.cmake")

    # Track identifiers for collision detection
    set(_all_identifiers "")
    # Lists for header generation
    set(_all_rel_paths "")
    set(_all_sym_names "")
    set(_generated_sources "")

    foreach(_file IN LISTS _all_files)
        # Normalize path separators (Windows compatibility)
        cmake_path(CONVERT "${_file}" TO_CMAKE_PATH_LIST _file)

        # Get relative path from base directory
        cmake_path(RELATIVE_PATH _file BASE_DIRECTORY "${_base_dir}" OUTPUT_VARIABLE _rel_path)

        # Convert to C++ identifier
        string(MAKE_C_IDENTIFIER "${_rel_path}" _sym_name)
        set(_sym_name "_data_${_sym_name}")

        # Check for identifier collision
        if("${_sym_name}" IN_LIST _all_identifiers)
            # Resolve collision with hash suffix
            _cpp_embedlib_short_hash("${_rel_path}" _hash)
            set(_sym_name "${_sym_name}_${_hash}")
            message(WARNING
                "cpp-embedlib: identifier collision detected for '${_rel_path}'. "
                "Using '${_sym_name}' instead.")
        endif()
        list(APPEND _all_identifiers "${_sym_name}")

        # Output .cpp file path
        set(_output_cpp "${_gen_dir}/${_sym_name}.cpp")

        # Register build-time code generation
        add_custom_command(
            OUTPUT "${_output_cpp}"
            COMMAND "${CMAKE_COMMAND}"
                "-DINPUT_FILE=${_file}"
                "-DOUTPUT_FILE=${_output_cpp}"
                "-DSYM_NAME=${_sym_name}"
                "-DNAMESPACE=${_ARG_NAMESPACE}"
                -P "${_generate_script}"
            DEPENDS "${_file}" "${_generate_script}"
            COMMENT "cpp-embedlib: generating ${_sym_name}"
            VERBATIM
        )

        list(APPEND _generated_sources "${_output_cpp}")
        list(APPEND _all_rel_paths "${_rel_path}")
        list(APPEND _all_sym_names "${_sym_name}")
    endforeach()

    # Extract directory prefixes from file paths.
    # e.g. "css/style.css" → "css", "js/lib/utils.js" → "js", "js/lib"
    set(_all_dirs "")
    foreach(_rel_path IN LISTS _all_rel_paths)
        set(_remaining "${_rel_path}")
        while(TRUE)
            cmake_path(GET _remaining PARENT_PATH _parent)
            if("${_parent}" STREQUAL "" OR "${_parent}" STREQUAL ".")
                break()
            endif()
            list(APPEND _all_dirs "${_parent}")
            set(_remaining "${_parent}")
        endwhile()
    endforeach()
    list(REMOVE_DUPLICATES _all_dirs)

    # Build a map from path → sym_name for file entries
    # (CMake doesn't have real maps, so we use parallel lists and list(FIND))
    set(_file_paths "${_all_rel_paths}")
    set(_file_syms "${_all_sym_names}")

    # Merge file paths + directory paths into a single sorted list
    set(_all_entry_paths ${_all_rel_paths})
    list(APPEND _all_entry_paths ${_all_dirs})
    list(REMOVE_DUPLICATES _all_entry_paths)
    list(SORT _all_entry_paths)

    # Build sorted parallel lists
    set(_sorted_paths "")
    set(_sorted_syms "")
    set(_sorted_is_dirs "")
    foreach(_path IN LISTS _all_entry_paths)
        list(FIND _all_dirs "${_path}" _dir_idx)
        if(NOT _dir_idx EQUAL -1)
            # Directory entry
            list(APPEND _sorted_paths "${_path}")
            list(APPEND _sorted_syms "_DIR_")
            list(APPEND _sorted_is_dirs "1")
        else()
            # File entry — look up its symbol
            list(FIND _file_paths "${_path}" _file_idx)
            list(GET _file_syms ${_file_idx} _sym)
            list(APPEND _sorted_paths "${_path}")
            list(APPEND _sorted_syms "${_sym}")
            list(APPEND _sorted_is_dirs "0")
        endif()
    endforeach()

    set(_all_rel_paths "${_sorted_paths}")
    set(_all_sym_names "${_sorted_syms}")

    # Generate unified header content
    set(_extern_decls "")
    set(_entry_items "")

    list(LENGTH _all_rel_paths _count)
    math(EXPR _last "${_count} - 1")
    foreach(_i RANGE 0 ${_last})
        list(GET _all_rel_paths ${_i} _rel_path)
        list(GET _all_sym_names ${_i} _sym_name)
        list(GET _sorted_is_dirs ${_i} _is_dir)

        if(_is_dir)
            # Directory entry: empty data, is_dir = true
            string(APPEND _entry_items
                "    { \"${_rel_path}\", {}, true },\n")
        else()
            # File entry: extern data, is_dir = false
            string(APPEND _extern_decls
                "extern const unsigned char ${_sym_name}[];\n"
                "extern const std::uint32_t ${_sym_name}_size;\n")

            string(APPEND _entry_items
                "    { \"${_rel_path}\", {_impl::${_sym_name}, _impl::${_sym_name}_size}, false },\n")
        endif()
    endforeach()

    # Build custom MIME type function if MIME_TYPES is provided
    set(_mime_func "")
    if(_ARG_MIME_TYPES)
        # Parse pairs: .ext "type" .ext2 "type2"
        list(LENGTH _ARG_MIME_TYPES _mime_count)
        math(EXPR _mime_mod "${_mime_count} % 2")
        if(NOT _mime_mod EQUAL 0)
            message(FATAL_ERROR
                "cpp-embedlib: MIME_TYPES must be pairs of extension and type, "
                "e.g. MIME_TYPES .vue \"text/html\" .ts \"text/typescript\"")
        endif()

        set(_mime_checks "")
        math(EXPR _mime_last "${_mime_count} - 1")
        foreach(_i RANGE 0 ${_mime_last} 2)
            math(EXPR _j "${_i} + 1")
            list(GET _ARG_MIME_TYPES ${_i} _ext)
            list(GET _ARG_MIME_TYPES ${_j} _type)

            if(NOT _ext MATCHES "^\\.[a-zA-Z0-9]+$")
                message(FATAL_ERROR
                    "cpp-embedlib: invalid MIME_TYPES extension '${_ext}'. "
                    "Must start with '.' followed by alphanumeric characters.")
            endif()
            if(NOT _type MATCHES "^[a-zA-Z0-9][a-zA-Z0-9!#$&^_.+-]*/[a-zA-Z0-9][a-zA-Z0-9!#$&^_.+-]*$")
                message(FATAL_ERROR
                    "cpp-embedlib: invalid MIME type '${_type}'. "
                    "Must be in 'type/subtype' format.")
            endif()

            string(APPEND _mime_checks
                "        if (ext == \"${_ext}\") return \"${_type}\";\n")
        endforeach()

        set(_mime_func
"// Custom MIME type lookup: user-defined → built-in → application/octet-stream
inline std::string_view mime_type(std::string_view path) {
    auto dot = path.rfind('.');
    if (dot != std::string_view::npos) {
        auto ext = path.substr(dot);
${_mime_checks}    }
    return ::cpp_embedlib::detail::mime_type(path);
}
")
    endif()

    # Write header via file(GENERATE) — content determined at configure time,
    # written at build time. Uses extern declarations only, so no dependency
    # on the actual .cpp generation.
    set(_header_path "${_gen_dir}/${_target_name}.h")
    file(GENERATE OUTPUT "${_header_path}" CONTENT
"// Auto-generated by cpp-embedlib. DO NOT EDIT.
#pragma once
#include <cpp-embedlib/cpp-embedlib.h>
#include <cstdint>
#include <span>
#include <string_view>

namespace ${_ARG_NAMESPACE} {
namespace _impl {
${_extern_decls}} // namespace _impl

inline const ::cpp_embedlib::detail::StorageEntry _entries[] = {
${_entry_items}};

inline const auto FS =
    ::cpp_embedlib::EmbeddedFS{std::span{_entries}};

${_mime_func}} // namespace ${_ARG_NAMESPACE}
")

    # Create static library target
    add_library(${_target_name} STATIC ${_generated_sources})
    target_include_directories(${_target_name} PUBLIC
        "${_gen_dir}"
        "${_CPP_EMBEDLIB_CMAKE_DIR}/../include"
    )
    target_compile_features(${_target_name} PUBLIC cxx_std_20)
endfunction()
