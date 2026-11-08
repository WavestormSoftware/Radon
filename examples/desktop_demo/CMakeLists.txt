if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR ANDROID OR IOS)
    return()
endif()
add_executable(radon_desktop_demo main.cpp)
target_link_libraries(radon_desktop_demo PRIVATE radon_demo_common)
rd_warnings(radon_desktop_demo)
# Copy the owner-supplied font next to the binary when it exists; the build succeeds without it.
if(EXISTS ${CMAKE_SOURCE_DIR}/examples/assets/font.ttf)
    add_custom_command(TARGET radon_desktop_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/examples/assets/font.ttf
                $<TARGET_FILE_DIR:radon_desktop_demo>/assets/font.ttf)
endif()
