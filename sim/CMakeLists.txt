cmake_minimum_required(VERSION 3.16)
project(dao_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DAO_SIM_BUILD_ENGINE "Build WebSocket simulation engine" ON)
option(DAO_SIM_BUILD_TESTS "Build Catch2 unit tests" ON)

include(FetchContent)

set(DAO_SIM_CORE_SOURCES
  src/graph.cpp
  src/event_queue.cpp
  src/neuron.cpp
  src/validate.cpp
  src/dao_snn.cpp
  src/dao_ode.cpp
  src/roche.cpp
  src/criticality.cpp
  src/output.cpp
  src/metrics.cpp
  src/sim_world.cpp
)

add_library(sim_core STATIC ${DAO_SIM_CORE_SOURCES})
target_include_directories(sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(sim_validate src/validate_main.cpp)
target_link_libraries(sim_validate PRIVATE sim_core)

if(DAO_SIM_BUILD_ENGINE)
  find_package(Threads REQUIRED)
  FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.4
  )
  set(USE_TLS OFF CACHE BOOL "" FORCE)
  set(USE_ZLIB OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(ixwebsocket)

  add_executable(sim_engine
    src/main.cpp
    src/telemetry.cpp
    src/ws_server.cpp
  )
  target_link_libraries(sim_engine PRIVATE sim_core ixwebsocket Threads::Threads)
  target_include_directories(sim_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()

if(DAO_SIM_BUILD_TESTS)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
  )
  FetchContent_MakeAvailable(Catch2)
  include(CTest)
  enable_testing()

  add_executable(test_graph tests/test_graph.cpp)
  target_link_libraries(test_graph PRIVATE sim_core Catch2::Catch2WithMain)
  add_test(NAME test_graph COMMAND test_graph)

  add_executable(test_queue_long_delay tests/test_queue_long_delay.cpp)
  target_link_libraries(test_queue_long_delay PRIVATE sim_core Catch2::Catch2WithMain)
  add_test(NAME test_queue_long_delay COMMAND test_queue_long_delay)

  add_executable(test_dao_oscillation tests/test_dao_oscillation.cpp)
  target_link_libraries(test_dao_oscillation PRIVATE sim_core Catch2::Catch2WithMain)
  add_test(NAME test_dao_oscillation COMMAND test_dao_oscillation)

  add_executable(test_criticality tests/test_criticality.cpp)
  target_link_libraries(test_criticality PRIVATE sim_core Catch2::Catch2WithMain)
  add_test(NAME test_criticality COMMAND test_criticality)
endif()
