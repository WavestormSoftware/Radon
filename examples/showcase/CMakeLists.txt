add_executable(radon_showcase main.cpp common.cpp tilekit.cpp halcyon.cpp stride.cpp liveops.cpp)
target_link_libraries(radon_showcase PRIVATE radon_demo_common)
rd_warnings(radon_showcase)
# The apps load their typefaces from fonts/ next to the executable (or examples/showcase/fonts/).
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
