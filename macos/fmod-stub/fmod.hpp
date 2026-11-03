#pragma once

// LTBF R41 (macOS, LTBF_MAC_NO_AUDIO builds only): a no-op stand-in for the part of the FMOD API that Let There Be Flight uses, so the
// plugin builds and runs without the FMOD libraries. Every call succeeds and does nothing; FlightAudio is silent. Not FMOD code.

#include <cstring>

typedef int FMOD_RESULT;
enum : int
{
    FMOD_OK = 0
};

typedef unsigned int FMOD_INITFLAGS;
typedef unsigned int FMOD_STUDIO_INITFLAGS;
typedef unsigned int FMOD_STUDIO_LOAD_BANK_FLAGS;
enum : unsigned int
{
    FMOD_INIT_NORMAL = 0,
    FMOD_INIT_3D_RIGHTHANDED = 0x4,
    FMOD_INIT_PROFILE_ENABLE = 0x10000,
    FMOD_STUDIO_INIT_NORMAL = 0,
    FMOD_STUDIO_INIT_LIVEUPDATE = 0x1,
    FMOD_STUDIO_LOAD_BANK_NORMAL = 0
};
enum FMOD_SPEAKERMODE
{
    FMOD_SPEAKERMODE_STEREO = 3
};
enum FMOD_OUTPUTTYPE
{
    FMOD_OUTPUTTYPE_AUTODETECT = 0
};
enum FMOD_STUDIO_STOP_MODE
{
    FMOD_STUDIO_STOP_ALLOWFADEOUT = 0,
    FMOD_STUDIO_STOP_IMMEDIATE = 1
};

struct FMOD_VECTOR
{
    float x, y, z;
};

struct FMOD_3D_ATTRIBUTES
{
    FMOD_VECTOR position;
    FMOD_VECTOR velocity;
    FMOD_VECTOR forward;
    FMOD_VECTOR up;
};

namespace FMOD
{
class System
{
public:
    FMOD_RESULT setSoftwareFormat(int, FMOD_SPEAKERMODE, int) { return FMOD_OK; }
    FMOD_RESULT setOutput(FMOD_OUTPUTTYPE) { return FMOD_OK; }
};
} // namespace FMOD
