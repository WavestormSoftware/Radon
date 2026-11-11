add_executable(radon_web main.cpp)
target_link_libraries(radon_web PRIVATE radon_demo_common)
rd_warnings(radon_web)
set_target_properties(radon_web PROPERTIES SUFFIX ".html")
target_link_options(radon_web PRIVATE
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sGL_ENABLE_GET_PROC_ADDRESS=1
    -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1048576 -sENVIRONMENT=web
    --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
set_property(TARGET radon_web APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
# The font is embedded so no filesystem or server is needed. Without it the demo prints an error.
if(EXISTS ${CMAKE_SOURCE_DIR}/examples/assets/font.ttf)
    target_link_options(radon_web PRIVATE --embed-file ${CMAKE_SOURCE_DIR}/examples/assets/font.ttf@/font.ttf)
else()
    message(WARNING "examples/assets/font.ttf is missing: the web demo will run without text")
endif()
