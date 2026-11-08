# Android builds a shared library named "main" that SDL's activity loads; iOS builds an app bundle.
set(_font ${CMAKE_SOURCE_DIR}/examples/assets/font.ttf)
if(ANDROID)
    add_library(main SHARED main.cpp)
    target_link_libraries(main PRIVATE radon_demo_common)
    rd_warnings(main)
elseif(IOS)
    set(_sources main.cpp)
    if(EXISTS ${_font})
        list(APPEND _sources ${_font})
        set_source_files_properties(${_font} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    else()
        message(WARNING "examples/assets/font.ttf is missing: the iOS demo will run without text")
    endif()
    add_executable(radon_mobile_demo MACOSX_BUNDLE ${_sources})
    target_link_libraries(radon_mobile_demo PRIVATE radon_demo_common)
    set_target_properties(radon_mobile_demo PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER net.wavestorm.radon.demo
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
    rd_warnings(radon_mobile_demo)
endif()
