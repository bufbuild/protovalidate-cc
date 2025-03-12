# Provides string formatting, replacing placeholders in the form of "{key}" with
# their corresponding value. The first parameter is an output variable to store
# the result in. The second parameter is the value to be operated on. The next
# variables are pairs of keys and values to replace with.
#
# For example, the following code:
#     SharedDeps_FormatValue(RESULT "v{version}" "version" "1.0")
#     message(STATUS ${RESULT})
#
# ...will output:
#     -- v1.0
function(SharedDeps_FormatValue output_variable value)
    set(pairs ${ARGN})
    set(result ${value})

    list(LENGTH pairs pair_count)
    math(EXPR pair_count "${pair_count} / 2 - 1")

    foreach(i RANGE ${pair_count})
        math(EXPR key_index "${i} * 2 + 0")
        math(EXPR value_index "${i} * 2 + 1")
        list(GET pairs ${key_index} key)
        list(GET pairs ${value_index} value)
        string(REPLACE "{${key}}" "${value}" result "${result}")
    endforeach()

    set(${output_variable} "${result}" PARENT_SCOPE)
endfunction()

# Gets the dependency meta value from meta_key of the dependency dependency_name
# from json_data and stores it in output_variable.
#
# For example, the following code:
#     SharedDeps_GetMetaValue(RESULT "absl" "version" "{
#       \"absl\": {
#         \"meta\": {
#           \"version\": \"20240722.0\"
#         }
#       }
#     }")
#     message(STATUS "${RESULT}")
#
# ...will output:
#     -- 20240722.0
function(SharedDeps_GetMetaValue output_variable dependency_name meta_key json_data)
    string(JSON meta_type ERROR_VARIABLE error_var TYPE "${json_data}" "${dependency_name}" "meta" "${meta_key}")
    if(error_var)
        message(FATAL_ERROR "Failed to get ${meta_key} type for ${dependency_name}: ${error_var}")
    endif()
    if(NOT "${meta_type}" STREQUAL "STRING")
        message(FATAL_ERROR "Unexpected type ${meta_type} for meta key ${meta_key} of ${dependency_name}.")
    endif()
    string(JSON meta_value ERROR_VARIABLE error_var GET "${json_data}" "${dependency_name}" "meta" "${meta_key}")
    if(error_var)
        message(FATAL_ERROR "Failed to get ${meta_key} value for ${dependency_name}: ${error_var}")
    endif()
    set(${output_variable} "${meta_value}" PARENT_SCOPE)
endfunction()

# Gets the formatted dependency source value from source_key of the dependency
# dependency_name from json_data and stores it in output_variable. The input
# json_data should contain a map of keys of the dependency names, to a map of
# two keys, source and meta. The meta map is a map of strings to strings that
# contains metadata about the dependency. The source map is a map of strings to
# strings or arrays of strings, and the value strings may contain format values,
# which can reference meta values, in the form of "{name}" where "name" is a key
# in the meta map.
#
# The output variable will be a CMake list of formatted values.
#
# For example, the following code:
#     SharedDeps_GetSourceValue(RESULT "absl" "urls" "{
#       \"absl\": {
#         \"meta\": {
#           \"version\": \"20240722.0\"
#         },
#         \"source\": {
#           \"urls\": [
#             \"https://github.com/abseil/abseil-cpp/archive/{version}.tar.gz\",
#             \"https://example.com/abseil/abseil-cpp/{version}.tgz\"
#           ]
#         }
#       }
#     }")
#     message(STATUS "${RESULT}")
#
# ...will output:
#     -- https://github.com/abseil/abseil-cpp/archive/20240722.0;https://example.com/abseil/abseil-cpp/20240722.0.tgz
function(SharedDeps_GetSourceValue output_variable dependency_name source_key json_data)
    set(error_var "")

    # Part 1: Construct meta value pairs

    string(JSON meta_object ERROR_VARIABLE error_var GET "${json_data}" "${dependency_name}" "meta")
    if(error_var)
        message(FATAL_ERROR "Failed to get meta values for ${dependency_name}: ${error_var}")
    endif()

    set(pairs "")
    string(JSON meta_length ERROR_VARIABLE error_var LENGTH "${meta_object}")
    if(error_var)
        message(FATAL_ERROR "Failed to get meta value count for ${dependency_name}: ${error_var}")
    endif()

    math(EXPR meta_length "${meta_length} - 1")
    foreach(i RANGE ${meta_length})
        string(JSON meta_key ERROR_VARIABLE error_var MEMBER "${meta_object}" ${i})
        if(error_var)
            message(FATAL_ERROR "Failed to get meta value index ${i} for ${dependency_name}: ${error_var}")
        endif()
        string(JSON meta_type ERROR_VARIABLE error_var TYPE "${meta_object}" "${meta_key}")
        if(error_var)
            message(FATAL_ERROR "Failed to get meta type of ${meta_key} for ${dependency_name}: ${error_var}")
        endif()
        if(NOT "${meta_type}" STREQUAL "STRING")
            message(FATAL_ERROR "Unexpected meta type ${meta_type} for ${meta_key} of ${dependency_name}")
        endif()
        string(JSON meta_value ERROR_VARIABLE error_var GET "${meta_object}" ${meta_key})
        if(error_var)
            message(FATAL_ERROR "Failed to get meta value index ${i} for ${dependency_name}: ${error_var}")
        endif()
        list(APPEND pairs "${meta_key};${meta_value}")
    endforeach()

    # Part 2: Get and format source value

    string(JSON source_type ERROR_VARIABLE error_var TYPE "${json_data}" "${dependency_name}" "source" "${source_key}")
    if(error_var)
        message(FATAL_ERROR "Failed to get ${source_key} type for ${dependency_name}: ${error_var}")
    endif()

    string(JSON raw_source ERROR_VARIABLE error_var GET "${json_data}" "${dependency_name}" "source" "${source_key}")
    if(error_var)
        message(FATAL_ERROR "Failed to get ${source_key} value for ${dependency_name}: ${error_var}")
    endif()

    set(formatted_value "")
    if("${source_type}" STREQUAL "STRING")
        SharedDeps_FormatValue(formatted_value "${raw_source}" "${pairs}")
    elseif("${source_type}" STREQUAL "ARRAY")
        string(JSON array_length ERROR_VARIABLE error_var LENGTH "${raw_source}")
        if(error_var)
            message(FATAL_ERROR "Failed to get ${source_key} array length for ${dependency_name}: ${error_var}")
        endif()

        math(EXPR array_length "${array_length} - 1")
        foreach(i RANGE ${array_length})
            string(JSON array_element ERROR_VARIABLE error_var GET "${raw_source}" ${i})
            if(error_var)
                message(FATAL_ERROR "Failed to get array element ${i} in ${source_key} for ${dependency_name}: ${error_var}")
            endif()
            SharedDeps_FormatValue(formatted_item "${array_element}" "${pairs}")
            list(APPEND formatted_value "${formatted_item}")
        endforeach()
    else()
        message(FATAL_ERROR "Unsupported source value type ${source_type} for ${source_key} on ${dependency_name}")
    endif()

    set(${output_variable} "${formatted_value}" PARENT_SCOPE)
endfunction()
