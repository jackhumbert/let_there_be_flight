#include "ListenerDraw.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/Color.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Utils.hpp>

#include <chrono>

namespace ListenerDraw {

namespace {

// Debug Draw's natives (its DebugDraw.reds declares them)
struct Natives {
  RED4ext::CBaseFunction *solidSphere = nullptr, *arrow = nullptr, *line = nullptr, *text = nullptr;
  bool Ready() const { return solidSphere && arrow && line && text; }
};

// looked up once Debug Draw has registered them; retried every few seconds until then
const Natives &Find() {
  static Natives natives;
  static std::chrono::steady_clock::time_point lastTry;
  const auto now = std::chrono::steady_clock::now();
  if (!natives.Ready() && now - lastTry > std::chrono::seconds(5)) {
    lastTry = now;
    auto rtti = RED4ext::CRTTISystem::Get();
    natives.solidSphere = rtti->GetFunction("DebugDraw_SolidSphere");
    natives.arrow = rtti->GetFunction("DebugDraw_Arrow");
    natives.line = rtti->GetFunction("DebugDraw_Line");
    natives.text = rtti->GetFunction("DebugDraw_Text");
  }
  return natives;
}

RED4ext::Vector4 Along(const RED4ext::Vector4 &aFrom, const RED4ext::Vector4 &aAxis, float aMetres) {
  return {aFrom.X + aAxis.X * aMetres, aFrom.Y + aAxis.Y * aMetres, aFrom.Z + aAxis.Z * aMetres, 1.0f};
}

} // namespace

void Draw(const RED4ext::Matrix &m, bool aCamera) {
  const Natives &n = Find();
  if (!n.Ready()) {
    return;
  }
  RED4ext::Vector4 at{m.W.X, m.W.Y, m.W.Z, 1.0f};
  RED4ext::Vector4 ahead = Along(at, m.Y, 3.0f), up = Along(at, m.Z, 1.0f), label = Along(at, m.Z, 1.2f);
  RED4ext::Color color = aCamera ? RED4ext::Color{60, 220, 255, 255} : RED4ext::Color{80, 255, 120, 255};
  RED4ext::Color white{255, 255, 255, 255};
  // one frame, drawn over the scene, on LTBF's own channel
  float seconds = 0.0f, radius = 0.2f, hairline = 0.0f, thick = 0.02f, scale = 1.0f;
  bool overlay = true;
  RED4ext::CName channel = "ltbf_listener";
  RED4ext::CString name = aCamera ? "listener: camera" : "listener: soundListener";
  // global natives: no instance
  const RED4ext::ScriptInstance none = nullptr;
  RED4ext::ExecuteFunction(none, n.solidSphere, nullptr, at, radius, color, seconds, overlay, channel, hairline);
  RED4ext::ExecuteFunction(none, n.arrow, nullptr, at, ahead, color, seconds, overlay, channel, thick);
  RED4ext::ExecuteFunction(none, n.line, nullptr, at, up, white, seconds, overlay, channel, thick);
  RED4ext::ExecuteFunction(none, n.text, nullptr, label, name, color, seconds, channel, scale);
}

} // namespace ListenerDraw
