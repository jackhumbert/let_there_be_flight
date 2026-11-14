#pragma once

// FlightBoundaries: the game side of Navigation::Boundaries, the natives behind FlightBoundariesSystem
// (src/redscript/Navigation/FlightBoundaries.reds). That scriptable system follows the quest facts the boundary
// gates name the way the game's own systems follow facts: once the player is attached it registers a listener per
// fact with the quests system (QuestsSystem.RegisterListener, as uiInventoryScriptableSystem does) and calls
// Refresh, and each listener calls Refresh again; the listeners go in its OnDetach.
//
// Refresh reads the facts from the quests system, reached through the game instance under its RTTI name
// questQuestsSystem (or the interface it is kept under), calling its GetFactStr by name (the game's
// questSystem.script signature). When the system or the function is missing every gate reads 0, logged once.

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightBoundaries : RED4ext::IScriptable {
  // the facts the gates read (for the listeners)
  static RED4ext::DynArray<RED4ext::CName> GetFacts();
  // reads the facts and lays the areas in force over the nav map (from the next query on); false when there are
  // no boundaries or no nav map
  static bool Refresh();

  // For checks in the game, read only: how many areas are in force, the mask's key ("0" for none), and whether
  // the column at the position is blocked by an area in force.
  static int32_t GetAreasInForce();
  static RED4ext::CString GetMaskKey();
  static bool IsBlocked(RED4ext::Vector4 aPosition);

  RTTI_IMPL_TYPEINFO(FlightBoundaries);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightBoundaries, {
  RTTI_METHOD(GetFacts);
  RTTI_METHOD(Refresh);
  RTTI_METHOD(GetAreasInForce);
  RTTI_METHOD(GetMaskKey);
  RTTI_METHOD(IsBlocked);
});
