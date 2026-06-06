cmake_minimum_required(VERSION 3.24)

project(DragonMugen
    VERSION 0.1.0
    LANGUAGES CXX
)

include(FetchContent)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRAGON_FETCH_SDL3 "Fetch SDL3 when a local SDL3 package is not available" ON)

find_package(SDL3 CONFIG QUIET)

if(NOT SDL3_FOUND AND DRAGON_FETCH_SDL3)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.8
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endif()

add_custom_target(dragon_architecture_guard
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/tools/guard_architecture.py
        ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking Dragon MUGEN architecture boundaries"
)

add_custom_target(dragon_dev_check
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/tools/dev_check.py
        ${CMAKE_CURRENT_SOURCE_DIR}
        --skip-build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Dragon MUGEN development checks"
)

add_library(dragon_core
    engine/src/MugenText.cpp
    engine/src/FightData.cpp
    engine/src/MugenData.cpp
    engine/src/SelectionState.cpp
    engine/src/Sff.cpp
    engine/src/Snd.cpp
    engine/src/MainMenuOverlay.cpp
    engine/src/OptionsMenuOverlay.cpp
    engine/src/PauseMenuOverlay.cpp
    engine/src/StageSelectOverlay.cpp
    engine/src/UiRenderPrimitives.cpp
    engine/src/FrontendMenu.cpp
    engine/src/TrainingOptionsBehavior.cpp
    engine/src/Input.cpp
    engine/src/VerificationScenario.cpp
    engine/src/App.cpp
)

target_include_directories(dragon_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
)

target_link_libraries(dragon_core
    PUBLIC SDL3::SDL3
)

add_dependencies(dragon_core dragon_architecture_guard)

add_executable(dragon_mugen
    engine/src/main.cpp
)

target_link_libraries(dragon_mugen
    PRIVATE dragon_core
)

if(WIN32)
    add_custom_command(TARGET dragon_mugen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:dragon_mugen>
    )
endif()
