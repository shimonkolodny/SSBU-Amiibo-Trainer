cmake_minimum_required(VERSION 3.16)
project(SSBUAmiiboTrainer VERSION 2.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /EHsc /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(ssbu_amiibo_core STATIC
    src/ssbu_loadouts.cpp
    src/amiibo_bin.cpp
    src/gpu_compute.cpp
    src/training_sim.cpp
)
target_include_directories(ssbu_amiibo_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    target_link_libraries(ssbu_amiibo_core PUBLIC d3d11 dxgi d3dcompiler)
else()
    target_link_libraries(ssbu_amiibo_core PUBLIC ${CMAKE_DL_LIBS})
endif()

add_executable(ssbu_amiibo_trainer
    src/main.cpp
    src/win32_gui.cpp
)
target_link_libraries(ssbu_amiibo_trainer PRIVATE ssbu_amiibo_core)

if(WIN32)
    target_link_libraries(ssbu_amiibo_trainer PRIVATE user32 gdi32 comdlg32 comctl32)
endif()

enable_testing()
add_executable(test_trainer tests/test_trainer.cpp)
target_link_libraries(test_trainer PRIVATE ssbu_amiibo_core)
add_test(NAME TrainerUnitAndSimTests COMMAND test_trainer)
