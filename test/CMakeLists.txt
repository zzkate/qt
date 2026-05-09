find_package(Qt6 REQUIRED COMPONENTS Core Test Widgets)
set(CMAKE_AUTOMOC ON)

qt6_add_executable(TestXorProcess
    testmain.cpp
    testXorProcess.cpp
    ../fileworker.cpp
)

target_include_directories(TestXorProcess PRIVATE ..)

target_link_libraries(TestXorProcess PRIVATE
    Qt6::Core
    Qt6::Test
)

find_package(Qt6 REQUIRED COMPONENTS Core Test)
include(GoogleTest)  # Если QtTest + GTest

# Для LCOV покрытия
find_program(LCOV_PATH lcov)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs")

add_test(NAME FileWorkerTests COMMAND TestXorProcess)