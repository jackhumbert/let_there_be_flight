#pragma once

// The Autopilot's world in the game (Navigation::World, docs/design/native-ports.md, M6 step 6): the nav map, the
// landing pads and the gates (MapWorld over NavMap::Get(), Helipads::Get(), Gates::Get()), with the two landing rays
// cast through the engine as FlightModeAutopilot.reds cast them (the last commit with it: c3e24ab), from the
// component's spatial queries system (FlightComponent.sqs, as FlightGround reads it):
//   - VehicleRay (SpotTaken): gameSpatialQueriesSystem.SyncRaycastByCollisionGroup(from, to, n"Vehicle", out result,
//     staticOnly false, dynamicOnly false), its Bool;
//   - GroundRay (SpotSupported): SyncRaycastByQueryFilter(from, to, filter, out result, false, false) with
//     QueryFilter.AddGroup Static, Terrain and VehicleBlocker, then TraceResult.IsValid(result) and result.position.
// The engine functions are found by name once (signatures: the game's spatialQueriesSystem.script); when one is
// missing, or the component has no sqs, a ray hits nothing (warned once), as an unstreamed world does.

#include "World.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>

namespace Navigation {

struct FlightWorld : MapWorld {
  FlightWorld();

  bool VehicleRay(const Vec3 &aFrom, const Vec3 &aTo) const override;
  bool GroundRay(const Vec3 &aFrom, const Vec3 &aTo, Vec3 &aHit) const override;

  // the FlightComponent whose sqs the rays are cast through (the mode's own; it outlives the mode's use of it)
  RED4ext::IScriptable *component = nullptr;
};

} // namespace Navigation
