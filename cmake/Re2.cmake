include(FetchContent)

if(TARGET re2::re2)
    message(STATUS "protovalidate-cc: Using pre-existing re2 targets")
else()
    find_package(re2 CONFIG)
    if(re2_FOUND)
        message(STATUS "protovalidate-cc: Using external re2 ${re2_VERSION}")
    else()
        if(PROTOVALIDATE_CC_ENABLE_VENDORING)
            set(RE2_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/deps/patches/re2/0001-Add-RE2_INSTALL-option.patch)
            FetchContent_Declare(
                re2
                GIT_REPOSITORY "https://github.com/google/re2.git"
                GIT_TAG "2024-04-01"
                GIT_SHALLOW TRUE
                PATCH_COMMAND git apply --check -R ${RE2_PATCH} || git apply ${RE2_PATCH}
            )
            set(RE2_INSTALL OFF)
            FetchContent_MakeAvailable(re2)
        else()
            message(FATAL_ERROR "protovalidate-cc: Could not find absl package and vendoring is disabled.")
        endif()
    endif()
endif()
