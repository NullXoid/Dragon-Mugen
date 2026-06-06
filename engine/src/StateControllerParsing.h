#pragma once

// Internal App.cpp parser implementation coordinator.
// These split headers depend on App.cpp-local CNS/state-controller parse types and helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

#include "StateControllerTriggerParsing.h"
#include "StateControllerParamParsing.h"
#include "StateControllerDefinitionLoading.h"
