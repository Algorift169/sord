cmake_minimum_required(VERSION 3.16)
project(sord LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FTXUI REQUIRED IMPORTED_TARGET ftxui)
pkg_check_modules(LIBHPDF QUIET IMPORTED_TARGET libhpdf)

set(SOURCES
    src/main.cpp
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
    list(APPEND SOURCES src/exporter/pdf_exporter.cpp)
else()
    list(APPEND SOURCES src/exporter/pdf_exporter_stub.cpp)
endif()

add_executable(sord ${SOURCES})

target_include_directories(sord PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sord PRIVATE PkgConfig::FTXUI)
if(LIBHPDF_FOUND)
    target_link_libraries(sord PRIVATE PkgConfig::LIBHPDF)
    target_compile_definitions(sord PRIVATE HAVE_LIBHPDF=1)
else()
    target_compile_definitions(sord PRIVATE HAVE_LIBHPDF=0)
endif()

add_executable(sord_tests
    tests/document_test.cpp
    src/editor/page.cpp
    src/editor/page_manager.cpp
    src/editor/document.cpp
)

target_include_directories(sord_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(sord_tests PRIVATE cxx_std_20)

add_test(NAME document_tests COMMAND sord_tests)

add_custom_target(distclean
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}
    COMMENT "Removing build directory"
)
