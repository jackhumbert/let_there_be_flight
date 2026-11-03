#pragma once

// LTBF R41 (LTBF_MAC_NO_AUDIO builds only): see fmod.hpp in this folder.

#include "fmod.hpp"

namespace FMOD
{
namespace Studio
{
class EventInstance
{
public:
    FMOD_RESULT start() { return FMOD_OK; }
    FMOD_RESULT stop(FMOD_STUDIO_STOP_MODE) { return FMOD_OK; }
    FMOD_RESULT release() { return FMOD_OK; }
    FMOD_RESULT setPaused(bool) { return FMOD_OK; }
    FMOD_RESULT setVolume(float) { return FMOD_OK; }
    FMOD_RESULT setPitch(float) { return FMOD_OK; }
    FMOD_RESULT setTimelinePosition(int) { return FMOD_OK; }
    FMOD_RESULT set3DAttributes(const FMOD_3D_ATTRIBUTES*) { return FMOD_OK; }
    FMOD_RESULT setParameterByName(const char*, float, bool = false) { return FMOD_OK; }
};

class EventDescription
{
public:
    // No instance is ever handed out: callers get a valid, silent stand-in.
    FMOD_RESULT createInstance(EventInstance** aInstance)
    {
        static EventInstance s_silent;
        if (aInstance)
            *aInstance = &s_silent;
        return FMOD_OK;
    }
};

class Bank
{
public:
    FMOD_RESULT unload() { return FMOD_OK; }
};

class Bus
{
public:
    FMOD_RESULT setVolume(float) { return FMOD_OK; }
    FMOD_RESULT setPaused(bool) { return FMOD_OK; }
};

class System
{
public:
    static FMOD_RESULT create(System** aSystem, unsigned int = 0)
    {
        static System s_system;
        if (aSystem)
            *aSystem = &s_system;
        return FMOD_OK;
    }
    FMOD_RESULT getCoreSystem(FMOD::System** aCore) const
    {
        static FMOD::System s_core;
        if (aCore)
            *aCore = &s_core;
        return FMOD_OK;
    }
    FMOD_RESULT initialize(int, FMOD_STUDIO_INITFLAGS, FMOD_INITFLAGS, void*) { return FMOD_OK; }
    FMOD_RESULT loadBankFile(const char*, FMOD_STUDIO_LOAD_BANK_FLAGS, Bank** aBank)
    {
        static Bank s_bank;
        if (aBank)
            *aBank = &s_bank;
        return FMOD_OK;
    }
    FMOD_RESULT getBus(const char*, Bus** aBus) const
    {
        static Bus s_bus;
        if (aBus)
            *aBus = &s_bus;
        return FMOD_OK;
    }
    FMOD_RESULT getEvent(const char*, EventDescription** aDescription) const
    {
        static EventDescription s_description;
        if (aDescription)
            *aDescription = &s_description;
        return FMOD_OK;
    }
    FMOD_RESULT setParameterByName(const char*, float, bool = false) { return FMOD_OK; }
    FMOD_RESULT setListenerAttributes(int, const FMOD_3D_ATTRIBUTES*, const FMOD_VECTOR* = nullptr) { return FMOD_OK; }
    FMOD_RESULT update() { return FMOD_OK; }
    FMOD_RESULT release() { return FMOD_OK; }
};
} // namespace Studio
} // namespace FMOD
