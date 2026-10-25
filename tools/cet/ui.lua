-- Ink UI helpers for tools/cet.py: find widgets by text or name and put the game's own cursor on them.
-- Prepend this file to a chunk, then call UI.*:
--   UI.dump("inkMenuLayer")                 every visible widget: path, class, text
--   UI.dump("inkMenuLayer", "mod")          only lines containing "mod" (case-insensitive)
--   UI.cursor("inkMenuLayer", "MOD SETTINGS")
--     moves the game's menu cursor onto the first visible widget whose text or name matches;
--     follow with `tools/sendkey.ps1 -Keys lmb` for a real click through the game's input path.
--     Works on the pause menu; inside Mod Settings the game ignores it. The reliable click is
--     `tools/sendkey.ps1 -X <x> -Y <y>` with coordinates read off a screenshot.ps1 capture.
-- Layers: inkMenuLayer (menus), inkHUDLayer (HUD). Matching is case-insensitive substring.
UI = UI or {}

local function lower(s) return string.lower(s or "") end

local function className(w)
  local ok, c = pcall(function() return w:GetClassName().value end)
  return ok and c or "?"
end

local function textOf(w)
  local ok, t = pcall(function() return w:GetText() end)
  if ok and t and t ~= "" then return t end
  return nil
end

local function visible(w)
  local ok, v = pcall(function() return w:IsVisible() end)
  return not ok or v
end

-- depth-first over a widget tree; fn(widget, path) returning true stops the walk
local function walk(w, path, fn)
  if not w or not visible(w) then return false end
  local name = w:GetName().value
  path = path .. "/" .. name
  if fn(w, path) then return true end
  local ok, n = pcall(function() return w:GetNumChildren() end)
  if ok and n then
    for i = 0, n - 1 do
      if walk(w:GetWidgetByIndex(i), path, fn) then return true end
    end
  end
  return false
end

local function controllers(layerName)
  local layer = Game.GetInkSystem():GetLayer(CName.new(layerName))
  if not layer then return {} end
  local ok, list = pcall(function() return layer:GetGameControllers() end)
  if ok and list and #list > 0 then return list end
  local c = layer:GetGameController()
  return c and { c } or {}
end

function UI.dump(layerName, filter)
  local out, f = {}, filter and lower(filter) or nil
  for ci, ctrl in ipairs(controllers(layerName)) do
    walk(ctrl:GetRootWidget(), "#" .. ci .. ":" .. className(ctrl), function(w, path)
      local line = string.format("%s [%s]%s", path, className(w), textOf(w) and (' "' .. textOf(w) .. '"') or "")
      if not f or string.find(lower(line), f, 1, true) then out[#out + 1] = line end
    end)
  end
  return table.concat(out, "\n")
end

-- first visible widget whose text or name contains `match`; returns widget, owning controller, path.
-- Searches the layer's controllers last to first: the newest (topmost) menu owns the cursor, and
-- SetCursorOverWidget through an outer controller does nothing for a submenu's widgets.
function UI.find(layerName, match)
  local m = lower(match)
  local list = controllers(layerName)
  for ci = #list, 1, -1 do
    local ctrl = list[ci]
    local found, foundPath
    walk(ctrl:GetRootWidget(), "#" .. ci, function(w, path)
      local t = textOf(w)
      if (t and string.find(lower(t), m, 1, true)) or string.find(lower(w:GetName().value), m, 1, true) then
        found, foundPath = w, path
        return true
      end
    end)
    if found then return found, ctrl, foundPath end
  end
end

function UI.cursor(layerName, match)
  local w, ctrl, path = UI.find(layerName, match)
  if not w then return "not found: " .. match end
  ctrl:SetCursorOverWidget(w, 0.0, true)
  return "cursor on " .. path
end
