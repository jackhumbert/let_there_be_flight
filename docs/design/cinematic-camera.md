# The cinematic camera in flight

The Autopilot's cinematic camera (the autodrive key's camera toggle) is vanilla's
`vehicleCinematicCameraComponent`: groups of shots, each with a root that places the camera, effects
that move it and stop conditions that end it, picked by weight when a shot ends. Its shots were made for
a car on a road at road speed. LTBF keeps the component and its shot data and changes how the shots are
placed and picked while the car flies (`FlightCinematic.Flying()`), in
`src/redscript/Extensions/vehicleCinematicCamera.reds` (wraps on the vanilla classes) with the maths in
C++ (`src/red4ext/Camera/Cinematic.*`, pure and tested in `tests/cinematic_tests.cpp`, bound through
`FlightCinematicShot` and `FlightModeAutopilot.CinematicFlyby`). On the ground nothing changes.

## Shots attached to the car

`cameraShotRoot_FixedOnVehicleNonSuspensionTracking` and `...SuspensionTracking` place the camera at an
offset in a frame that follows the car. In flight both use a level frame: the car's forward made
level, eased with a 0.4 s time constant (`Cinematic::LevelHeading`); the velocity's level part when the
car points nearly straight up or down. The car banks and pitches inside the frame; the camera does not
swing round it. The frame is asked for more than once a frame, so the heading eases only when the
game's clock has moved on, and a heading older than half a second (the shot's last run) is dropped.

## Framing

Every shot is aimed in `vehicleCinematicCameraShot.Update` after vanilla has placed it
(`FlightCinematic.AimAtCar`, `Cinematic::AimAtCarLead`), the horizon level:

- pushed out along the line from the middle of the car's bounding box until the box fits 65% of the
  frame's height and 70% of its width by the camera's reported field of view, never nearer than 4.5 m
  (`Cinematic::FitDistance`). The box is turned with the car's heading only and its width and height are
  both the radius of its cross-section, so it fits at any bank (`FitDistanceAnyBank`): fitted to the
  box as it sat, the camera pumped in and out with every turn, since a banked car stands taller in a
  level view. The distance is eased per shot, out with a 0.3 s time constant and back in with 1.5 s
  (`Cinematic::EaseDistance`), so a change of view direction does not pump it either;
- looking ahead of the car along its motion across the line of sight by up to 10 degrees
  (`Cinematic::AimPoint`, full at 10 m/s across), so the car sits behind the frame's centre with room in
  front of it, more the faster it crosses the view.

## Flybys

The shots with a `cameraShotRoot_FixedShot` root are fixed in the world: vanilla puts the camera about
30 m ahead of where the car starts. In flight they are planned beside the route instead
(`Cinematic::PlanFlyby`):

- The pass point is where the car will be after 3 s (sooner when the route ends first, never under
  1.5 s), read from the Autopilot's route and trajectory (`FlightModeAutopilot.CinematicFlyby`); without
  a route, along the car's velocity (`FlightCinematicShot.FlybyStraight`).
- The camera is beside it, square to the route and level, at the shot's own height, far enough out that
  the car crosses the view at no more than 0.75 radians a second as planned (the speed past the camera
  over the distance; the car strays a little from the planned line), 6 to 30 m, and at least the shot's
  own offset.
- Faster than 15 m/s, the camera drifts along the route's direction at the pass, so the car goes past it
  at 15 m/s.
- The side is the one the camera is on now, so a cut to a flyby does not cross the line; then the same
  6 m higher; then the other side; then all four with the pass a second sooner.
- A spot is taken when the camera's path over the shot is open air in the nav map and its view of the
  car is clear at four moments (start, half way, the pass, half again after), its last 6 m left out (the
  nav map is blocked round the car near a wall or the ground). With no such spot the shot is not picked.

The plan is made anew each time the shot is weighed as a candidate (`GetCanBeSelectedFromContext` resets
the root and places the camera) and again at its start (`Start` resets the root first), so the shot is
picked on the plan it runs with. The distance stop for a flyby is 1.25
times where the camera started (`Cinematic::StopLimit`), so it ends once the car is past and away.

## Which shot is picked

- `GetCanBeSelectedFromContext`: a flyby with a clear plan; an attached shot on vanilla's rule (the
  side of the car the camera is on, or the middle), not one of the road shots that sit under a metre
  from the car (`FlightCinematic.Unused`: the wheel and hood shots), and with its camera clear of
  collision by the camera's own test (`CheckCollisionsAtPosition`, as vanilla's collision stop) now,
  0.75 s and 1.5 s ahead at the car's velocity. The nav map is no test for a camera: it has space
  blocked wherever the car would not fit.
- Either kind only when its view of the car at the start turns at least 30 degrees from the camera's
  view now (`FlightCinematic.JumpCut`): less reads as a jump cut.
- `GetShotProbabilityFromContext`: the shot's own probability, times 3 for a flyby, times 4 again when it
  is not among the last three shots run. Vanilla's bonuses are not used in flight: the one for a shot
  starting near the camera picked the shot next to the last one (a cut to nearly the same view).
- `ShouldStop`: AttachedFront1 stops after 2.5 s (`FlightCinematic.MaxDuration`).

The camera's groups and shots are logged once a session (`[CinematicCamera] group ...`), and each shot's
start with its flyby plan (`[CinematicCamera] shot ... flyby side 1 pass 3.0 s drift 30.0 m/s`).
