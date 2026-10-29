cmake_minimum_required(VERSION 3.16)
project(SSBUAmiiboTrainer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ssbu_amiibo_core STATIC
    src/ssbu_loadouts.cpp
    src/amiibo_bin.cpp
    src/training_sim.cpp
    src/win32_gui.cpp
)
target_include_directories(ssbu_amiibo_core PUBLIC include)

if(MSVC)
    target_compile_options(ssbu_amiibo_core PRIVATE /W4 /EHsc)
else()
    target_compile_options(ssbu_amiibo_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(ssbu_amiibo_trainer src/main.cpp)
target_link_libraries(ssbu_amiibo_trainer PRIVATE ssbu_amiibo_core)
if(WIN32)
    target_link_libraries(ssbu_amiibo_trainer PRIVATE comctl32 comdlg32 user32 gdi32)
endif()

enable_testing()
add_executable(test_trainer tests/test_trainer.cpp)
target_link_libraries(test_trainer PRIVATE ssbu_amiibo_core)
add_test(NAME TrainerUnitAndIntegrationTests COMMAND test_trainer)
