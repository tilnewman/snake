cmake_minimum_required(VERSION 3.8)
project("Snake" VERSION 0.5.0 LANGUAGES CXX)


file(GLOB source_files *.?pp)
add_executable(${PROJECT_NAME} ${source_files})


find_package(SFML 2.5 COMPONENTS window graphics audio REQUIRED)
target_link_libraries(${PROJECT_NAME} sfml-system sfml-window sfml-graphics sfml-audio)


#compiler/linker options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    target_compile_options(
        ${PROJECT_NAME}
        PUBLIC
        /std:c++latest
        /permissive-
        /fp:fast
        -DWIN32_LEAN_AND_MEAN
        /MP
        /W4
        /wd26812
        /wd26451
        /w14254
        /w14263
        /w14265
        /w14287
        /w14296
        /w14311
        /w14545
        /w14546
        /w14547
        /w14549
        /w14555
        /w14619
        /w14640
        /w14826
        /w14905
        /w14906
        /w14928
        /we4289)

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

    target_compile_options(
        ${PROJECT_NAME}
        PUBLIC
        -DNDEBUG
        -lstdc++
        -O3
        -std=c++17
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-covered-switch-default
        -Wno-deprecated
        -Wno-disabled-macro-expansion
        -Wno-error
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-newline-eof
        -Wno-padded
        -Wno-unused-command-line-argument
        -Wno-weak-vtables
        -Wno-poison-system-directories
        -Wno-switch-enum
        -Wno-documentation-unknown-command
        -Wno-ctad-maybe-unsupported
        -Woverloaded-virtual
        -Wcast-align
        -Wformat=2)


    option(ASAN "Using Clang's Address Sanitizer" ON)

    if(ASAN)
	message(" *** Using Clang's Address Sanitizer *** (-DASAN=OFF will disable it)")
        target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
        target_link_libraries(${PROJECT_NAME} -fsanitize=address -fno-omit-frame-pointer)
    endif()

elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")

    target_compile_options(
        ${PROJECT_NAME}
        PUBLIC
        -std=c++17
        -O3
        -DNDEBUG
        -lstdc++
        -ffast-math
        -pedantic
        -Wall
        -Wconversion
        -Weffc++
        -Wextra
        -Wfatal-errors
        -Wfloat-equal
        -Wshadow
        -Wsign-conversion
        -Wstrict-null-sentinel
        -Wswitch-default
        -Wno-switch-enum
        -Wundef
        -Wunreachable-code
        -Wunused-parameter
        -Wcast-align
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wnull-dereference
        -Wuseless-cast
        -Wformat=2)

else()

    message(FATAL_ERROR " Unknwon Compiler: ${CMAKE_CXX_COMPILER_ID}")

endif()
