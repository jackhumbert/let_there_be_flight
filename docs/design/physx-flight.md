# PhysX-owned flight (experimental)

Mod Settings > General Flight Settings > **PhysX Flight Physics (experimental)**, default off.
Code: `src/red4ext/Physics/ChassisContacts.cpp` (hooks), `src/red4ext/Flight/Component.cpp` (force routing).

## Why

Vanilla vehicles are moved by CDPR's own integrator (`vehicle::RigidBody`, 60 Hz fixed steps,
gravity + wheel contacts only). PhysX resolves chassis contacts asynchronously and the result
reaches the integrator once per frame as a velocity delta (`PhysXSynchronizer::SyncFromPhysX`).
In flight the wheels do not hold the car up, so anything resting or sliding on the body jitters
(see `drone-ground-jitter-analysis.md`). Patching the integrator side (contact clamps, contact
point estimates) kept producing new artefacts. Letting PhysX simulate the body solves gravity,
contacts and material friction in one place.

## What the engine offers (2.31 RE, 2026-10-01)

- No PhysX-only mode exists. Even exploded wrecks keep the integrator; `vehicle::BaseObject+0x675`
  ("has exploded", set once in the HasExplodedEvent handler, never cleared) only moves gravity to
  PhysX (proxy state 22) in `RigidBody::ApplyGravityForce`.
- `vehicle::BaseObject::TogglePhysics(DisabledPhysicsReason, bool)` (#2592348558, the SDK's
  `SetPhysicsState`) toggles a bitmask at +0x260; any set bit drops the vehicle from the fixed
  steps (no integrator, no PushToPhysX) and usually makes the chassis kinematic. Bits seen:
  0x1 traffic, 0x8 autopilot, 0x10 asleep, 0x20 streaming/spawn, 0x40 parked, 0x80 scene/not
  movable, 0x100 simplified movement, 0x400 gameplay hidden. Not used here.
- Per frame: `SyncFromPhysX` (#547299556) -> N x [PreSolve gravity/wheels -> FixedUpdate ->
  `RigidBody::IntegrateMotion` (#2021199885)] -> `PhysXSynchronizer::PushToPhysX` (#3628605463),
  which writes PhysX linear/angular velocity (proxy states 4/5) chasing the RigidBody pose.

## Design (all three sync points, only while flight is active and the setting is on)

| Point | Vanilla | PhysX-owned |
|---|---|---|
| SyncFromPhysX | apply PhysX velocity minus pushed velocity as an impulse | copy pose (`ApplyNewXform` #3129742900), linear and angular velocity from the PhysX body |
| IntegrateMotion | integrate the RigidBody | skipped |
| PushToPhysX | velocity that chases the RigidBody pose | PhysX velocity + LTBF thrust impulse / mass (+ inverse world inertia * torque impulse) - g * frame dt; keep awake |

- LTBF's per-frame force/torque (already impulses: script multiplies by dt and mass) is held on
  the FlightComponent (`pendingImpulse`) instead of the RigidBody accumulator and applied in the
  PushToPhysX hook.
- Gravity is added to the written velocity rather than enabling PhysX gravity (state 22), so no
  PhysX state is left changed when flight ends.
- The synchronizer's tracking velocities (+0x10/+0x20, state +0 = 0) are kept equal to what
  PhysX has, so turning the mode off resumes the vanilla delta sync without a jump.
- Kinematic chassis (AVs) are left on the vanilla path.
- Proxy writes: `ChassisComponent::CreateWriter` (#3550813847) -> `ProxyWriter::SetLinearVelocity`
  (#1416501174), `SetAngularVelocity` (#1900746789), `SetIsSleeping` (#100078185) -> `Commit`
  (#200045) -> `~ConcurrentProxyWriter` (#1189351231, releases the lock). Reads:
  `GetBodyTransform` (#411244519), `GetBodyLinearVelocity` (#1665341632),
  `GetBodyAngularVelocity` (#2145786159), `GetBodyIsKinematic` (#856298520).

## Risks / to check in game

- Thrust lands one frame later (state writes are buffered until the next PhysX step).
- Feel changes for every flight mode; PIDs, damping and the brake stability cap
  (`dampStabilityK`, calibrated against the integrator) may need retuning in this mode.
- NPC vehicles can still be put to sleep by `SuspensionBase::UpdateSleeping` (bit 0x10).
- The chassis-contact workarounds in the same file only run when the mode is off.
