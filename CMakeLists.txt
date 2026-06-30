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

find_package(SDL3_image CONFIG QUIET)

if(NOT SDL3_image_FOUND AND DRAGON_FETCH_SDL3)
    set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(SDL3_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG release-3.4.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3_image)
endif()

find_package(SDL3_mixer CONFIG QUIET)

if(NOT SDL3_mixer_FOUND AND DRAGON_FETCH_SDL3)
    set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MIDI_NATIVE OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(SDL3_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG release-3.2.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3_mixer)
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
    engine/src/ArenaConfig.cpp
    engine/src/ArenaSetupOverlay.cpp
    engine/src/ControlMapping.cpp
    engine/src/ControlsOptionsMenu.cpp
    engine/src/Compatibility.cpp
    engine/src/ControlsStore.cpp
    engine/src/DragonUi.cpp
    engine/src/DragonProgression.cpp
    engine/src/DragonProgressionProfile.cpp
    engine/src/MugenText.cpp
    engine/src/FightData.cpp
    engine/src/FightDebugLog.cpp
    engine/src/FightHudOverlay.cpp
    engine/src/FightResultOverlay.cpp
    engine/src/FramePerformance.cpp
    engine/src/CharacterSelectOverlay.cpp
    engine/src/MugenData.cpp
    engine/src/SelectionState.cpp
    engine/src/Sff.cpp
    engine/src/Snd.cpp
    engine/src/MainMenuOverlay.cpp
    engine/src/OptionsMenuOverlay.cpp
    engine/src/PauseMenuOverlay.cpp
    engine/src/StageSelectOverlay.cpp
    engine/src/StoryStageSelectOverlay.cpp
    engine/src/UiMenuList.cpp
    engine/src/UiRenderPrimitives.cpp
    engine/src/UiSpriteView.cpp
    engine/src/FrontendMenu.cpp
    engine/src/TrainingCommandInputRenderer.cpp
    engine/src/TrainingCommandOverlay.cpp
    engine/src/TrainingDebugOverlay.cpp
    engine/src/TrainingOptionsBehavior.cpp
    engine/src/TrainingOptionsOverlay.cpp
    engine/src/VsScreenOverlay.cpp
    engine/src/Input.cpp
    engine/src/VerificationScenario.cpp
    engine/src/VerificationScenarioArena.cpp
    engine/src/VerificationScenarioArenaOpenBor.cpp
    engine/src/VerificationScenarioEvilKen.cpp
    engine/src/VerificationScenarioEvilKenAdvanced.cpp
    engine/src/VerificationScenarioEvilRyu.cpp
    engine/src/VerificationScenarioEvilRyuSpecials.cpp
    engine/src/VerificationScenarioCoreSmoke.cpp
    engine/src/VerificationScenarioClassicCombat.cpp
    engine/src/VerificationScenarioModeSmoke.cpp
    engine/src/VerificationScenarioOptionsControls.cpp
    engine/src/VerificationScenarioFightOutcomes.cpp
    engine/src/VerificationScenarioDragonProgression.cpp
    engine/src/VerificationScenarioKfmHit.cpp
    engine/src/VerificationScenarioLoading.cpp
    engine/src/VerificationScenarioMovement.cpp
    engine/src/VerificationScenarioPerformance.cpp
    engine/src/VerificationScenarioRosterCompatibility.cpp
    engine/src/VerificationScenarioScottCompatibility.cpp
    engine/src/VerificationScenarioShop.cpp
    engine/src/VerificationScenarioStoryMode.cpp
    engine/src/VerificationScenarioSpecials.cpp
    engine/src/VerificationScenarioTrainingSpecials.cpp
    engine/src/VerificationScenarioTrainingDemoAll.cpp
    engine/src/VerificationScenarioTrainingCommands.cpp
    engine/src/VerificationScenarioTrainingGeometry.cpp
    engine/src/App.cpp
)

target_include_directories(dragon_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
)

target_link_libraries(dragon_core
    PUBLIC
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_mixer::SDL3_mixer
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
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3_image::SDL3_image>
            $<TARGET_FILE_DIR:dragon_mugen>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3_mixer::SDL3_mixer>
            $<TARGET_FILE_DIR:dragon_mugen>
    )
endif()
