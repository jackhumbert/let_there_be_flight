#pragma once

// LTBF R41 (LTBF_MAC_NO_AUDIO builds only): see fmod.hpp in this folder.

#include "fmod.hpp"

inline const char* FMOD_ErrorString(FMOD_RESULT)
{
    return "FMOD is not in this build (LTBF_MAC_NO_AUDIO)";
}
