add_library(rd_test_common INTERFACE)
target_link_libraries(rd_test_common INTERFACE radon)
target_include_directories(rd_test_common INTERFACE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(rd_test_common SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/src/third_party)
target_compile_definitions(rd_test_common INTERFACE RD_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_executable(radon_tests main.cpp test_ids.cpp test_input.cpp test_layout.cpp test_draw_list.cpp
    test_text.cpp test_widgets.cpp test_widgets2.cpp test_widgets3.cpp test_textbox.cpp test_v03.cpp)
if(RD_BUILD_C_API)
    target_sources(radon_tests PRIVATE test_c_api.cpp)
endif()
target_link_libraries(radon_tests PRIVATE rd_test_common)
rd_warnings(radon_tests)
add_test(NAME radon_tests COMMAND radon_tests)

add_executable(test_alloc test_alloc.cpp)
target_link_libraries(test_alloc PRIVATE rd_test_common)
rd_warnings(test_alloc)
add_test(NAME test_alloc COMMAND test_alloc)
