cmake_minimum_required(VERSION 3.13)
project(print_history)

# Find SQLite3
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Sources
set(PRINT_HISTORY_SOURCES
    PrintHistoryManager.hpp
    PrintHistoryManager.cpp
    PrintHistoryWidget.hpp
    PrintHistoryWidget.cpp
)

# Create the library
add_library(print_history STATIC ${PRINT_HISTORY_SOURCES})

# Include directories
target_include_directories(print_history PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(print_history PRIVATE ${SQLITE3_INCLUDE_DIRS})

# Link libraries
target_link_libraries(print_history PRIVATE ${SQLITE3_LIBRARIES})

# Compiler definitions
target_compile_definitions(print_history PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Find wxWidgets - this should already be available in the parent project
if(TARGET wx_base)
    target_link_libraries(print_history PRIVATE wx_base wx_core)
else()
    # Fallback for standard wxWidgets installation
    find_package(wxWidgets REQUIRED COMPONENTS base core)
    target_link_libraries(print_history PRIVATE ${wxWidgets_LIBRARIES})
    target_include_directories(print_history PRIVATE ${wxWidgets_INCLUDE_DIRS})
    target_compile_definitions(print_history PRIVATE ${wxWidgets_DEFINITIONS})
    target_compile_options(print_history PRIVATE ${wxWidgets_CXX_FLAGS})
endif()