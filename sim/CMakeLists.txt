cmake_minimum_required(VERSION 3.16)
project(dao_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DAO_SIM_BUILD_DYNAMICS "Build dao SNN/ODE/criticality/sim_world (phase 2+)" ON)
option(DAO_SIM_BUILD_ENGINE "Build WebSocket simulation engine" ON)
option(DAO_SIM_BUILD_TESTS "Build Catch2 unit tests" ON)

if(DAO_SIM_BUILD_ENGINE AND NOT DAO_SIM_BUILD_DYNAMICS)
  message(FATAL_ERROR "DAO_SIM_BUILD_ENGINE=ON requires DAO_SIM_BUILD_DYNAMICS=ON")
endif()

set(DAO_SIM_PHASE0_SOURCES
  src/graph.cpp
  src/event_queue.cpp
  src/neuron.cpp
  src/validate.cpp
)

set(DAO_SIM_DYNAMICS_SOURCES
  src/dao_snn.cpp
  src/dao_ode.cpp
  src/roche.cpp
  src/criticality.cpp
  src/output.cpp
  src/metrics.cpp
  src/sim_world.cpp
)

set(DAO_SIM_CORE_SOURCES ${DAO_SIM_PHASE0_SOURCES})
if(DAO_SIM_BUILD_DYNAMICS)
  list(APPEND DAO_SIM_CORE_SOURCES ${DAO_SIM_DYNAMICS_SOURCES})
endif()

add_library(sim_core STATIC ${DAO_SIM_CORE_SOURCES})
target_include_directories(sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(sim_validate src/validate_main.cpp)
target_link_libraries(sim_validate PRIVATE sim_core)

if(DAO_SIM_BUILD_DYNAMICS)
  add_executable(sim_dao_bench src/dao_bench_main.cpp)
  target_link_libraries(sim_dao_bench PRIVATE sim_core)

  add_executable(sim_fields_bench src/fields_bench_main.cpp)
  target_link_libraries(sim_fields_bench PRIVATE sim_core)

  add_executable(sim_sigma_bench src/sigma_bench_main.cpp)
  target_link_libraries(sim_sigma_bench PRIVATE sim_core)
endif()

# Vendored deps under sim/third_party — no FetchContent / no GitHub at configure time.
if(DAO_SIM_BUILD_ENGINE)
  find_package(Threads REQUIRED)
  set(USE_TLS OFF CACHE BOOL "" FORCE)
  set(USE_ZLIB OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory(third_party/IXWebSocket EXCLUDE_FROM_ALL)

  add_executable(sim_engine
    src/main.cpp
    src/telemetry.cpp
    src/ws_server.cpp
  )
  target_link_libraries(sim_engine PRIVATE sim_core ixwebsocket Threads::Threads)
  target_include_directories(sim_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()

if(DAO_SIM_BUILD_TESTS)
  set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
  set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
  add_subdirectory(third_party/Catch2 EXCLUDE_FROM_ALL)
  include(CTest)
  enable_testing()

  # Phase 0 tests (always)
  add_executable(test_graph tests/test_graph.cpp)
  target_link_libraries(test_graph PRIVATE sim_core Catch2::Catch2WithMain)
  add_test(NAME test_graph COMMAND test_graph)

  add_executable(test_queue_long_delay tests/test_queue_long_delay.cpp)
  target_link_libraries(test_queue_long_delay PRIVATE sim_core Catch2::Catch2WithMain)
  add_test(NAME test_queue_long_delay COMMAND test_queue_long_delay)

  # Phase 2 tests (only with dynamics)
  if(DAO_SIM_BUILD_DYNAMICS)
    add_executable(test_dao_oscillation tests/test_dao_oscillation.cpp)
    target_link_libraries(test_dao_oscillation PRIVATE sim_core Catch2::Catch2WithMain)
    add_test(NAME test_dao_oscillation COMMAND test_dao_oscillation)

    add_executable(test_criticality tests/test_criticality.cpp)
    target_link_libraries(test_criticality PRIVATE sim_core Catch2::Catch2WithMain)
    add_test(NAME test_criticality COMMAND test_criticality)
  endif()
endif()
