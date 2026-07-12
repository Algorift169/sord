cmake_minimum_required(VERSION 3.16)
project(sord LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FTXUI REQUIRED IMPORTED_TARGET ftxui)
pkg_check_modules(LIBHPDF QUIET IMPORTED_TARGET libhpdf)

set(CORE_SOURCES
    src/editor/page.cpp
    src/editor/page_manager.cpp
    src/editor/document.cpp
    src/editor/editor.cpp
    src/layout/page_layout.cpp
    src/renderer/page_renderer.cpp
    src/renderer/editor_renderer.cpp
    src/app/application.cpp
    src/app/save_manager.cpp
)

if(LIBHPDF_FOUND)
    list(APPEND CORE_SOURCES src/exporter/pdf_exporter.cpp)
else()
    list(APPEND CORE_SOURCES src/exporter/pdf_exporter_stub.cpp)
endif()

add_library(sord_core STATIC ${CORE_SOURCES})

target_include_directories(sord_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PUBLIC ${FTXUI_INCLUDE_DIRS})
target_link_libraries(sord_core PUBLIC PkgConfig::FTXUI)
if(LIBHPDF_FOUND)
    target_link_libraries(sord_core PUBLIC PkgConfig::LIBHPDF)
    target_include_directories(sord_core PUBLIC ${LIBHPDF_INCLUDE_DIRS})
    target_compile_definitions(sord_core PUBLIC HAVE_LIBHPDF=1)
else()
    target_compile_definitions(sord_core PUBLIC HAVE_LIBHPDF=0)
endif()

add_executable(sord src/main.cpp)
target_link_libraries(sord PRIVATE sord_core)

# Helper function to add a test executable
function(add_sord_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE sord_core)
    target_compile_features(${name} PRIVATE cxx_std_20)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_sord_test(test_document_title tests/test_document_title.cpp)
add_sord_test(test_document_insert_char tests/test_document_insert_char.cpp)
add_sord_test(test_document_insert_newline tests/test_document_insert_newline.cpp)
add_sord_test(test_document_backspace tests/test_document_backspace.cpp)
add_sord_test(test_document_move_cursor tests/test_document_move_cursor.cpp)
add_sord_test(test_page_basics tests/test_page_basics.cpp)
add_sord_test(test_page_manager_basics tests/test_page_manager_basics.cpp)
add_sord_test(test_page_manager_add_remove tests/test_page_manager_add_remove.cpp)
add_sord_test(test_page_manager_reorder tests/test_page_manager_reorder.cpp)
add_sord_test(test_page_layout tests/test_page_layout.cpp)
add_sord_test(test_save_manager_path tests/test_save_manager_path.cpp)
add_sord_test(test_save_manager_save tests/test_save_manager_save.cpp)
add_sord_test(test_editor_input tests/test_editor_input.cpp)
add_sord_test(test_pdf_exporter tests/test_pdf_exporter.cpp)

add_custom_target(distclean
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}
    COMMENT "Removing build directory"
)
