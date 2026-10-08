# CMakeLists.txt (Root or where tests are defined)
cmake_minimum_required(VERSION 3.10) # Or your project's minimum
if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()
project(PokerSolver LANGUAGES CXX)

# --- Set C++ Standard ---
set(CMAKE_CXX_STANDARD 20) # Use C++17 (required for std::optional, std::filesystem)
set(CMAKE_CXX_STANDARD_REQUIRED True) # Error if C++17 is not supported
set(CMAKE_CXX_EXTENSIONS ON) # Prefer standard features over extensions
# ------------------------

# --- Add AddressSanitizer Flags for Debug Builds ---
# Check if build type is Debug (or default/empty which often implies debug flags)
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "AddressSanitizer disabled for Debug build.")
else()
   message(STATUS "AddressSanitizer disabled for ${CMAKE_BUILD_TYPE} build.")
   # Optionally add optimization flags for Release builds
   # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()
# --------------------------------------------------


find_package(Threads REQUIRED)

# Homebrew libomp workaround for macOS Clang
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include" CACHE STRING "" FORCE)
    set(OpenMP_CXX_LIB_NAMES "omp" CACHE STRING "" FORCE)
    set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib" CACHE STRING "" FORCE)
endif()

find_package(OpenMP REQUIRED)

# +++ Additions for Qt UI +++
# Find Qt6 package and its components.
# Adjust components if you use more Qt modules (e.g., Network, Svg).
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

# Enable automatic moc, uic, and rcc handling.
# CMAKE_AUTOMOC: Enables automatic meta-object code generation.
# CMAKE_AUTOUIC: Enables automatic user interface code generation from .ui files (if any).
# CMAKE_AUTORCC: Enables automatic resource code generation from .qrc files.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fsanitize=address")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fsanitize=address")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Ensure that CMake finds the Qt6 tools (moc, uic, rcc).
# This is often handled automatically if Qt6 is in your PATH or CMAKE_PREFIX_PATH.
# If you have issues, you might need to set CMAKE_PREFIX_PATH to your Qt6 installation directory.
# Example: set(CMAKE_PREFIX_PATH "/path/to/your/Qt/6.x.y/arch")
# +++++++++++++++++++++++++++

# --- Define Poker Solver Core Library ---
add_library(PokerSolverCore
    src/core/poker_card.cpp
    src/core/card_deck.cpp
    src/core/solver_math_utils.cpp
    src/core/jenkins_lookup8.cpp
    src/core/canonical_flop_isomorphism.cpp
    src/tree/extensive_game_tree.cpp
    src/tree/poker_action_edge.cpp
    src/tree/scenario_game_rule.cpp
    src/tree/street_betting_rule.cpp
    src/tree/tree_building_config.cpp
    src/compairer/table_hand_evaluator.cpp
    src/ranges/hole_card_combination.cpp
    src/ranges/range_distribution_manager.cpp
    src/ranges/evaluated_river_combo.cpp
    src/ranges/river_evaluation_cache.cpp
    src/ranges/range_syntax_parser.cpp
    src/solver/equilibrium_solver_base.cpp
    src/solver/public_chance_cfr_solver.cpp
    src/solver/exploitability_calculator.cpp
    src/solver/discounted_cfr_engine.cpp
    src/kuhn/kuhn_game_setup.cpp
)

# Specify include directories for the PokerSolverCore library
# PUBLIC means these includes are also available to targets linking PokerSolverCore
target_include_directories(PokerSolverCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/poker_solver
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann # Add path to nlohmann/json if it's here
)
# For std::filesystem
target_link_libraries(PokerSolverCore PRIVATE Threads::Threads) # Threads might not be needed for filesystem

# Filesystem linking (keep as is)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(PokerSolverCore PRIVATE stdc++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7) # Or appropriate Clang version
    target_link_libraries(PokerSolverCore PRIVATE c++experimental)
endif()

# --- OpenMP Section ---
# Use the flags found by find_package(OpenMP) - should work with Homebrew's Clang
# target_compile_options(PokerSolverCore PRIVATE ${OpenMP_CXX_FLAGS})

# Link the library using the imported target found by find_package
target_link_libraries(PokerSolverCore PUBLIC OpenMP::OpenMP_CXX)
# --- End OpenMP Section ---

# +++ Define the UI Executable +++
set(UI_RESOURCE_FILES
    src/gui/resources.qrc
)

add_executable(poker_solver_ui
    src/gui/main.cpp
    src/gui/solver_main_window.cpp
    src/gui/solver_main_window.h 
    src/gui/range_matrix_dialog.cpp
    src/gui/range_matrix_dialog.h 
    src/gui/board_card_dialog.cpp
    src/gui/board_card_dialog.h
    src/gui/gto_strategy_explorer.cpp
    src/gui/gto_strategy_explorer.h 
    src/gui/solver_background_worker.h
    ${UI_RESOURCE_FILES} 
)

target_include_directories(poker_solver_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)

# Workaround for missing AGL framework in macOS Qt6 builds
if(APPLE)
    if(NOT EXISTS "/tmp/dummy_frameworks/AGL.framework/AGL")
        file(MAKE_DIRECTORY "/tmp/dummy_frameworks/AGL.framework")
        execute_process(COMMAND clang -dynamiclib -install_name @rpath/AGL.framework/AGL -o /tmp/dummy_frameworks/AGL.framework/AGL -x c /dev/null)
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/AGL.framework/AGL")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/AGL.framework")
        execute_process(COMMAND clang -dynamiclib -install_name @rpath/AGL.framework/AGL -o "${CMAKE_CURRENT_BINARY_DIR}/AGL.framework/AGL" -x c /dev/null)
    endif()
    get_target_property(_gui_libs Qt6::Gui INTERFACE_LINK_LIBRARIES)
    if(_gui_libs)
        list(REMOVE_ITEM _gui_libs "-framework AGL")
        set_target_properties(Qt6::Gui PROPERTIES INTERFACE_LINK_LIBRARIES "${_gui_libs}")
    endif()
endif()

# Link the UI executable against Qt6 and your PokerSolverCore library
target_link_libraries(poker_solver_ui PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    PokerSolverCore # Link against your core logic
)
target_link_options(poker_solver_ui PRIVATE "-F/tmp/dummy_frameworks")
# ++++++++++++++++++++++++++++++


# --- Google Test Integration ---
set(INSTALL_GTEST OFF CACHE BOOL "Disable installing gtest")
add_subdirectory(third_party/googletest) # Make sure this path is correct
enable_testing()

# --- Define Your Test Executable ---
add_executable(poker_solver_tests
    tests/poker_card_test.cpp
    tests/card_deck_test.cpp
    tests/hole_card_combination_test.cpp
    tests/table_hand_evaluator_test.cpp
    tests/river_evaluation_cache_test.cpp
    tests/range_syntax_parser_test.cpp
    tests/tree_building_config_test.cpp
    tests/scenario_game_rule_test.cpp
    tests/range_distribution_manager_test.cpp
    tests/suit_permutation_utils_test.cpp
    tests/scenario_json_loader.cpp
    tests/public_chance_cfr_integration_test.cpp
)

# Link your tests against Google Test AND your core library
target_link_libraries(poker_solver_tests PRIVATE
    gtest_main
    PokerSolverCore # This will inherit the OpenMP flags and linking from PokerSolverCore
)
# For std::filesystem in tests if needed (e.g. in test_scenario_loader.cpp)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(poker_solver_tests PRIVATE stdc++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
    target_link_libraries(poker_solver_tests PRIVATE c++experimental)
endif()


# --- Copy Test Data ---
# Copy the hand strength dictionary
set(TEST_DICT_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test_data/five_card_strength.txt)
set(TEST_DICT_DEST_PATH ${CMAKE_CURRENT_BINARY_DIR}/five_card_strength.txt)
add_custom_command(
    TARGET poker_solver_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TEST_DICT_SOURCE_PATH}"
            "${TEST_DICT_DEST_PATH}"
    COMMENT "Copying dictionary for tests..."
)

# --- NEW: Copy JSON Scenario Files ---
# Define the source directory for your JSON test scenarios
set(TEST_JSON_SCENARIOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test_data)

# Define the destination directory in the build folder
set(TEST_JSON_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# Create the destination directory if it doesn't exist
add_custom_command(
    TARGET poker_solver_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TEST_JSON_DEST_DIR}"
    COMMENT "Creating test_data directory in build folder..."
)

# Find all .json files in the source directory and copy them
# This uses a glob, which will find files at configure time.
# If you add new JSON files, you'll need to re-run CMake.
file(GLOB_RECURSE JSON_SCENARIO_FILES RELATIVE "${TEST_JSON_SCENARIOS_DIR}" "${TEST_JSON_SCENARIOS_DIR}/*.json")

foreach(JSON_FILE ${JSON_SCENARIO_FILES})
    set(JSON_SOURCE_PATH "${TEST_JSON_SCENARIOS_DIR}/${JSON_FILE}")
    set(JSON_DEST_PATH "${TEST_JSON_DEST_DIR}/${JSON_FILE}")
    add_custom_command(
        TARGET poker_solver_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${JSON_SOURCE_PATH}"
                "${JSON_DEST_PATH}"
        COMMENT "Copying test scenario: ${JSON_FILE}..."
    )
endforeach()
# ----------------------------------

include(GoogleTest)

add_executable(test_explorer
    src/gui/explorer_verifier.cpp
    src/gui/gto_strategy_explorer.cpp
    src/gui/gto_strategy_explorer.h
    src/gui/range_matrix_dialog.cpp
    src/gui/range_matrix_dialog.h
    src/gui/board_card_dialog.cpp
    src/gui/board_card_dialog.h
    ${UI_RESOURCE_FILES}
)
target_include_directories(test_explorer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)
target_link_libraries(test_explorer
    PRIVATE
    PokerSolverCore
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)
target_link_options(test_explorer PRIVATE "-F/tmp/dummy_frameworks")
