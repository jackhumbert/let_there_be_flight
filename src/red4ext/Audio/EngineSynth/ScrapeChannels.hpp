#pragma once

// The player's thrusters' scrapes, each a voice of its own (a ScrapeSynth), placed at the scrape and played on the
// Scrape bus, as the game plays its own scraping from an emitter moved to the contact (vehicle::Audio's
// scrapingEmitter). In the thruster's synth the scrape took the thruster's volume (a rear one's faded by speed), the
// Engine group's limiter and the acoustics' floor echo, whose comb swept through it. Started and stopped with the
// thruster's engine voice (FlightAudio::StartEngine, StopEngine), by the thruster's id.
//
// Game thread: everything here. The generators run on the backend's audio thread.

#include "Scrape.hpp"

#include "Audio/Voices/Pool.hpp"

#include <string>

namespace EngineSynth::ScrapeChannels {

bool Start(const std::string &aId);
// each frame: where the scrape is (world) and what it is; a silent one is paused by the voice pool
void Update(const std::string &aId, const Voices::Vec3 &aAt, const ScrapeInputs &aInputs);
void Stop(const std::string &aId);
void StopAll();

} // namespace EngineSynth::ScrapeChannels
