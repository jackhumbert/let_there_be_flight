-- Mount the player in the driver seat of the nearest vehicle whose record ID contains MATCH
-- (set MATCH before this chunk, e.g. "MATCH = 'shion'"; nil = any vehicle). Returns what it mounted.
local match = MATCH and string.lower(MATCH) or nil
local player = Game.GetPlayer()
local q = Game["TSQ_ALL;"]()
q.maxDistance = 40
local _, parts = Game.GetTargetingSystem():GetTargetParts(player, q)
local best, bestDist
for _, part in ipairs(parts or {}) do
  local e = part:GetComponent(part):GetEntity()
  if e and e:IsVehicle() then
    local rec = string.lower(TDBID.ToStringDEBUG(e:GetRecordID()))
    if not match or string.find(rec, match, 1, true) then
      local d = Vector4.Distance(player:GetWorldPosition(), e:GetWorldPosition())
      if not bestDist or d < bestDist then best, bestDist = e, d end
    end
  end
end
if not best then return "no matching vehicle within 40 m" end

local seat = "seat_front_left"
local data = NewObject("handle:gameMountEventData")
data.isInstant = true
data.slotName = seat
data.mountParentEntityId = best:GetEntityID()
data.entryAnimName = "forcedTransition"
local slotId = NewObject("gamemountingMountingSlotId")
slotId.id = seat
local info = NewObject("gamemountingMountingInfo")
info.childId = player:GetEntityID()
info.parentId = best:GetEntityID()
info.slotId = slotId
local request = NewObject("handle:gamemountingMountingRequest")
request.lowLevelMountingInfo = info
request.mountData = data
Game.GetMountingFacility():Mount(request)
return string.format("mounting %s (%.1f m)", TDBID.ToStringDEBUG(best:GetRecordID()), bestDist)
