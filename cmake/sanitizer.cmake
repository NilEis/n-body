﻿include(CheckCCompilerFlag)
function(add_sanitizer target flag)
    if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
        message("Looking for -fsanitize=${flag}")
        set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=${flag}")
        check_c_compiler_flag(-fsanitize=${flag} HAVE_FLAG_SANITIZER)
        if (HAVE_FLAG_SANITIZER)
            message("Adding -fsanitize=${flag}")
            target_compile_options(${target} PRIVATE -fsanitize=${flag} -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=${flag})
        else ()
            message("-fsanitize=${flag} unavailable")
        endif ()
    endif ()
endfunction()