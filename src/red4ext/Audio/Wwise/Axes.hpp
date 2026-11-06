#pragma once

// The game's coordinates to Wwise's: the game is z up, Wwise y up, so the game's (x, y, z) is Wwise's (x, z, y) (the
// spike: a sound placed to the listener's right that way is heard on the right).

#include "Types.hpp"

namespace Wwise {

struct GameVector {
  double x, y, z;
};

// a transform SetPosition accepts: the facing and the up as unit vectors at right angles. The up is made square to the
// facing; with no usable up (zero, or along the facing) the world's up is used, then the game's +y. With no usable
// facing, the game's +y is the facing.
AkWorldTransform ToWwise(const GameVector &aPosition, const GameVector &aForward, const GameVector &aUp);

} // namespace Wwise
