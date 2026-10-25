-- One-line JSON snapshot of the player's vehicle and LTBF's flight state.
local player = Game.GetPlayer()
local v = Game.GetMountedVehicle(player)
if not v then return '{"mounted":false}' end
local p = v:GetWorldPosition()
local vel = v:GetLinearVelocity()
local speed = math.sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z)
local fs = FlightSystem.GetInstance()
local c = fs and fs.playerComponent or nil
local active, mode = false, "none"
if c then
  active = c.active
  local m = c.modes[c.mode + 1]
  mode = m and m:GetDescription() or tostring(c.mode)
end
return string.format(
  '{"mounted":true,"vehicle":"%s","pos":[%.2f,%.2f,%.2f],"vel":[%.2f,%.2f,%.2f],"speed":%.2f,"flight":%s,"mode":"%s"}',
  TDBID.ToStringDEBUG(v:GetRecordID()), p.x, p.y, p.z, vel.x, vel.y, vel.z, speed, tostring(active), mode)
