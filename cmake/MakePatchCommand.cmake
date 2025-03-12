function(MakePatchCommand output_variable patch_files)
    # Be careful when trying to reason about this logic. Keep in mind that CMake
    # lists are just semicolon-separated strings, and if you aren't careful
    # about when you quote and when you don't, you could wind up in a world of
    # hurt very easily.
    set(patch_command_list "true")
    foreach(patch_file ${patch_files})
        string(APPEND patch_command_list
            " && (echo 'Applying patch ${patch_file}' && patch --dry-run -sfR -p1 < ${patch_file} || patch -p1 < ${patch_file})"
        )
    endforeach()
    set(${output_variable} /bin/sh -c "${patch_command_list}" PARENT_SCOPE)
endfunction()
