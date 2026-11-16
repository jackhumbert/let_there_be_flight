// The VehicleSystem's own record spawn (game::VehicleSystem::RequestNewVehicle), called by
// FlightSummonSystem (docs/design/vehicle-summon.md) and run from C++ (src/red4ext/Summon/
// FlightCarSummon.cpp): the vehicle the record stands for is created at the given transform.
// The id is the vehicle record itself; the spawned entity's id comes back through
// LastSpawnEntity once the game materializes it - the summon machine's tick reads that.
public native class FlightCarSummon extends IScriptable {
  public native static func SpawnVehicleAt(record: TweakDBID, x: Float, y: Float, z: Float, yaw: Float) -> Bool;
  public native static func LastSpawnEntity() -> EntityID;
  // The Autopilot's mode and Land at Destination both on in Mod Settings: it can take a summoned vehicle and
  // put it down. Asked before a spawn.
  public native static func AutopilotLands() -> Bool;
}