-- Switch the player's flight mode to Autopilot without the autodrive hold (same steps as
-- FlightController.CycleMode / SetMode, minus the HUD message). Flight must be active. The
-- in-game way is the vanilla autodrive hold (G / L3) while flying.
--   python tools/cet.py run -f tools/cet/autopilot_mode.lua
local sys = FlightSystem.GetInstance()
local pc = sys.playerComponent
local idx = -1
for i, m in ipairs(pc.modes) do
  if m:GetDescription() == "Autopilot" then idx = i - 1 end
end
if idx < 0 then return "no autopilot mode" end
sys.ctlr.mode = idx
local evt = VehicleFlightModeChangeEvent.new()
evt.mode = idx
Game.GetPlayer():GetMountedVehicle():QueueEvent(evt)
return "autopilot is mode " .. idx .. " of " .. #pc.modes
