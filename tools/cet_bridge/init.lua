-- claude_bridge: run Lua sent by tools/cet.py inside the game (CET mod).
--
-- CET's sandbox has no sockets, so the transport is two files in this mod's folder:
--   request.txt   first line "<id>", the rest is Lua source (written by the CLI)
--   response.txt  first line "<id>\t<ok|error>", the rest is captured print output and
--                 the chunk's return values (written here)
-- A request runs once, on the game's main thread, when its id differs from the last one.

local POLL_INTERVAL = 0.1

local lastId = nil
local sinceLastPoll = 0.0

local function readRequest()
  local f = io.open("request.txt", "r")
  if not f then return nil end
  local text = f:read("*a")
  f:close()
  local id, code = text:match("^([^\r\n]*)\r?\n(.*)$")
  return id, code
end

local function writeResponse(id, status, body)
  local f = io.open("response.txt", "w")
  if not f then return end
  f:write(id, "\t", status, "\n", body)
  f:close()
end

-- LuaJIT (CET) is 5.1: no table.pack / table.unpack
local function pack(...) return { n = select("#", ...), ... } end
local unpackFn = table.unpack or unpack

local function stringify(...)
  local parts = {}
  for i = 1, select("#", ...) do
    parts[#parts + 1] = tostring((select(i, ...)))
  end
  return table.concat(parts, "\t")
end

-- CET's sandbox drops `load`; fall back to loadstring, then loadfile on a scratch file in
-- this mod's folder (the only place the sandbox lets us read).
local function compile(src)
  if load then
    return load(src, "claude", "t")
  end
  if loadstring then
    return loadstring(src, "claude")
  end
  local f = io.open("chunk.lua", "w")
  if not f then return nil, "cannot write chunk.lua" end
  f:write(src)
  f:close()
  return loadfile("chunk.lua")
end

local function execute(code)
  local out = {}
  local function capture(...) out[#out + 1] = stringify(...) end

  -- try as an expression first so "Game.GetPlayer()" returns its value
  local chunk, err = compile("return " .. code)
  if not chunk then
    chunk, err = compile(code)
  end
  if not chunk then
    return "error", tostring(err)
  end

  -- capture print() for the duration of the call
  local realPrint = print
  print = capture
  local results = pack(pcall(chunk))
  print = realPrint

  if not results[1] then
    out[#out + 1] = tostring(results[2])
    return "error", table.concat(out, "\n")
  end
  if results.n > 1 then
    out[#out + 1] = stringify(unpackFn(results, 2, results.n))
  end
  return "ok", table.concat(out, "\n")
end

registerForEvent("onUpdate", function(delta)
  sinceLastPoll = sinceLastPoll + delta
  if sinceLastPoll < POLL_INTERVAL then return end
  sinceLastPoll = 0.0

  local id, code = readRequest()
  if not id or id == "" or id == lastId then return end
  lastId = id
  local ran, status, body = pcall(execute, code or "")
  if not ran then
    status, body = "error", "bridge: " .. tostring(status)
  end
  writeResponse(id, status, body)
end)

registerForEvent("onInit", function()
  -- ignore whatever request was left over from a previous session
  lastId = readRequest()
  writeResponse(lastId or "", "ok", "claude_bridge ready")
  print("[claude_bridge] ready")
end)
