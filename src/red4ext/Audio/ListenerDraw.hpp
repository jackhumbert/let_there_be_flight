#pragma once

// Where LTBF's sound is heard from, drawn in 3D through the Debug Draw mod (red4ext/plugins/debug_draw)
// when it is installed: a sphere at the listener (green for V's soundListener, cyan for the cinematic
// camera), an arrow 3 m along its facing, a white line along its up and a label; over everything, each
// frame, while the developer setting drawListener is on. Debug Draw's natives are looked up by name, so
// without it this does nothing.

#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>

namespace ListenerDraw {

// game thread, each frame: the listener FlightAudio was given (W the position, Y forward, Z up)
void Draw(const RED4ext::Matrix &aListener, bool aCamera);

} // namespace ListenerDraw
