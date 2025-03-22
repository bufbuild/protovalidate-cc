function(ValidateGnuPatchVersion output_variable item)
    # TODO: On Windows, programs named "patch.exe" will cause UAC elevation
    # by default. This is very annoying. Unfortunately, it also seems hard to
    # detect: there is an undocumented routine in Shell32 that supposedly
    # returns whether or not a given executable needs elevation in the current
    # context, but it does not seem to detect this special edge case. That means
    # that if the GnuWin32 version of patch.exe, which is too old for us anyway,
    # happens to be on the %PATH%, it will cause an annoying UAC prompt during
    # the configuration stage. It would be nice to figure out a way to avoid
    # this :|

    # Get the output of running with --version.
    execute_process(
        COMMAND "${item}" --version
        TIMEOUT 5
        OUTPUT_VARIABLE output
        RESULT_VARIABLE result
    )

    if(NOT result EQUAL 0)
        message(WARNING "protovalidate-cc: patch executable ${item} returned ${result}; skipping")
        set(${output_variable} FALSE PARENT_SCOPE)
        return()
    endif()

    if(NOT output MATCHES "GNU patch ([0-9]+)[.]([0-9]+)")
        message(WARNING "protovalidate-cc: patch executable ${item} did not seem to return a version in --version, output: ${output}; skipping")
        set(${output_variable} FALSE PARENT_SCOPE)
        return()
    endif()

    set(major_version ${CMAKE_MATCH_1})
    set(minor_version ${CMAKE_MATCH_2})

    if(major_version LESS 2 OR (major_version EQUAL 2 AND minor_version LESS 7))
        message(WARNING "protovalidate-cc: patch executable ${item} too old (${major_version}.${minor_version}); GNU patch 2.7 or higher is required. skipping")
        set(${output_variable} FALSE PARENT_SCOPE)
        return()
    endif()

    message(STATUS "protovalidate-cc: found GNU patch ${major_version}.${minor_version}")
    set(${output_variable} TRUE PARENT_SCOPE)
endfunction()

function(MakePatchCommand output_variable patch_files)
    set(validator_args "")
    if(WIN32)
        set(validator_args VALIDATOR ValidateGnuPatchVersion)
    endif()

    # Try to find patch executable.
    find_program(PATCH_EXECUTABLE
        NAMES "patch"
              "patch.exe"
        PATHS "$ENV{ProgramFiles}\\Git\\usr\\bin\\"
              "$ENV{ProgramFiles\(x86\)}\\Git\\usr\\bin\\"
              "C:\\msys64\\usr\\bin\\"
        ${validator_args}
    )
    if(NOT PATCH_EXECUTABLE)
        message(FATAL_ERROR "GNU patch 2.7+ not found. Please install it, or set PATCH_EXECUTABLE.")
    endif()

    # Be careful when trying to reason about this logic. Keep in mind that CMake
    # lists are just semicolon-separated strings, and if you aren't careful
    # about when you quote and when you don't, you could wind up in a world of
    # hurt very easily.
    if (WIN32)
        set(patch_command "set _z=")
        foreach(patch_file ${patch_files})
            string(APPEND patch_command
                " && @echo Applying patch ${patch_file} && "
                "\"${PATCH_EXECUTABLE}\" --dry-run -sfR -p1 -i \"${patch_file}\" ||"
                "\"${PATCH_EXECUTABLE}\" -p1 -i \"${patch_file}\""
            )
        endforeach()
        # It seems impossible to generate a working command on Windows without a
        # temporary file. We can use cmd /C much like sh -c, but CMake seems to
        # munge both semicolons and quoted strings in a way that makes it seem
        # unlikely you can do this as a one-liner.
        string(SHA256 patch_command_hash "${patch_command}")
        string(SUBSTRING "${patch_command_hash}" 0 8 patch_command_shorthash)
        set(batch_file "${CMAKE_CURRENT_BINARY_DIR}/apply-patches-${patch_command_shorthash}.cmd")
        file(WRITE "${batch_file}" "${patch_command}")
        set(${output_variable} "${batch_file}" PARENT_SCOPE)
    else()
        set(patch_command_list "true")
        foreach(patch_file ${patch_files})
            string(APPEND patch_command_list
                " && (echo 'Applying patch ${patch_file}' && ${PATCH_EXECUTABLE} --dry-run -sfR -p1 < ${patch_file} || ${PATCH_EXECUTABLE} -p1 < ${patch_file})"
            )
        endforeach()
        set(${output_variable} /bin/sh -c "${patch_command_list}" PARENT_SCOPE)
    endif()
endfunction()
