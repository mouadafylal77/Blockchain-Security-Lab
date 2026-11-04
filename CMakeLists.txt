cmake_minimum_required(VERSION 3.21)
project(BlockchainSecurityLab VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED COMPONENTS Crypto)
add_library(blockchain_core
    src/core/sha256.cpp
    src/core/block.cpp
    src/core/blockchain.cpp
    src/core/block_tree.cpp)
target_include_directories(blockchain_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(blockchain_core PUBLIC OpenSSL::Crypto)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(blockchain_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(blockchain_simulation src/sim/config.cpp src/sim/network.cpp src/sim/simulator.cpp
    src/metrics/json_exporter.cpp)
target_include_directories(blockchain_simulation SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_link_libraries(blockchain_simulation PUBLIC blockchain_core)
target_compile_definitions(blockchain_simulation PRIVATE BSL_VERSION="${PROJECT_VERSION}")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(blockchain_simulation PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(blockchain_sim src/main.cpp src/cli.cpp)
target_link_libraries(blockchain_sim PRIVATE blockchain_simulation)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_blockchain tests/test_blockchain.cpp)
    target_link_libraries(test_blockchain PRIVATE blockchain_core)
    foreach(case_name sha256 merkle valid_chain invalid_parent tampering genesis pow boundaries deterministic)
        add_test(NAME core_${case_name} COMMAND test_blockchain ${case_name})
    endforeach()
    add_test(NAME cli_demo COMMAND blockchain_sim --demo)
    set_tests_properties(cli_demo PROPERTIES PASS_REGULAR_EXPRESSION "chain_valid=true")
    add_test(NAME cli_help COMMAND blockchain_sim --help)
    add_test(NAME cli_invalid_argument COMMAND blockchain_sim --unknown)
    set_tests_properties(cli_invalid_argument PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli_extra_argument COMMAND blockchain_sim --demo extra)
    set_tests_properties(cli_extra_argument PROPERTIES WILL_FAIL TRUE)
    set_tests_properties(core_sha256 core_merkle core_valid_chain core_invalid_parent
        core_tampering core_genesis core_pow core_boundaries core_deterministic
        cli_demo cli_help cli_invalid_argument cli_extra_argument PROPERTIES TIMEOUT 20)
    add_executable(test_simulation tests/test_simulation.cpp)
    target_link_libraries(test_simulation PRIVATE blockchain_simulation)
    target_include_directories(test_simulation SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
    target_compile_definitions(test_simulation PRIVATE BSL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    foreach(case_name event_order rng network network_disorder hash_power local_views pending duplicates fork_choice reorganization
            invalid_blocks reproducibility config_invalid limits metrics demo_fork)
        add_test(NAME sim_${case_name} COMMAND test_simulation ${case_name})
        set_tests_properties(sim_${case_name} PROPERTIES TIMEOUT 30)
    endforeach()
    add_test(NAME cli_simulation COMMAND blockchain_sim --config "${CMAKE_CURRENT_SOURCE_DIR}/configs/baseline.json"
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-results/cli")
    add_test(NAME cli_missing_config COMMAND blockchain_sim --config "${CMAKE_CURRENT_BINARY_DIR}/missing.json"
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-results/missing")
    set_tests_properties(cli_simulation PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "blocks=")
    set_tests_properties(cli_missing_config PROPERTIES TIMEOUT 20 WILL_FAIL TRUE)
endif()
