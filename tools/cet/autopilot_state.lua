-- One line of autopilot state: mode, position, speed, and when the Autopilot mode is flying,
-- its status, route segment, target speed, distance left, distance off the route, whether the
-- map is blocked along the current velocity, and the lookahead point.
--   python tools/cet.py run -f tools/cet/autopilot_state.lua
local pc = FlightSystem.GetInstance().playerComponent
local m = pc:GetFlightMode()
local v = Game.GetPlayer():GetMountedVehicle()
local p = v:GetWorldPosition()
local s = pc.stats
local ap = m:GetDescription() == "Autopilot"
return string.format("%s | pos %.0f %.0f %.1f | speed %.1f | %s",
  m:GetDescription(), p.x, p.y, p.z, s.d_speed,
  ap and string.format("status %s seg %d/%d target %.1f remaining %.0f off %.1f blocked %s look %.0f %.0f %.0f",
    m.status, m.segment, #m.route, m.targetSpeed, m.remaining, m.offRouteNow, tostring(m.blockedAhead),
    m.lookahead.x, m.lookahead.y, m.lookahead.z) or "")
