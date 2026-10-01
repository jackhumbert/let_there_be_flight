# Vehicles resting on their chassis jitter in flight (drone mode on the ground)

Symptom (2026-09-30): a drone-mode vehicle sitting on the ground (or on its roof, side, or a
curb) twitches up and down and the speedometer shows up to 2 mph while it is not moving.
Live sampling through the CET bridge (`tools/cet.py`): LTBF's own forces were ~0, height
varied by ~3 cm, and vertical velocity always read about -g*dt (-0.5 to -0.9 m/s at ~18 fps).

## Engine call order (2.31, from the IDB)

Per frame, per vehicle:

1. `vehicle::PhysXSynchronizer::SyncFromPhysX` 0x1401A56DC (#547299556), via
   `BaseObject::ProcessPhysicsUpdateRequest` -> `SuspensionBase::UpdatePhysicsRequest`.
   Reads last frame's PhysX pose and velocity. Applies only the *difference* between the
   PhysX velocity and the velocity that was pushed (`PushToPhysX`) as an impulse into the
   rigid body (clamped by `ClampHugeLinearImpulse`), then snaps the pose when not
   extrapolating.
2. N fixed 60 Hz steps (`BaseObject::BuildJobs_FixedUpdate`), each:
   - PreSolve: `RigidBody::ApplyGravityForce` 0x1401A3490, wheel forces, `RigidBody::Recalculate`.
   - FixedUpdate: wheel contact solve (`vehicle::Solver`, wheels only), then
     `RigidBody::IntegrateMotion` 0x1406F8F40 (#2021199885) = Recalculate -> GetDesiredWorldTransform -> ApplyNewXform.
3. On the last step: `PhysXSynchronizer::PushToPhysX` 0x1401A4F20 drives the PhysX body with a
   velocity that chases the rigid body pose (`(rb.pos - px.pos) / T`).

Chassis contacts never reach the rigid body except through step 1
(`BaseObject::NotifyContacts` only does FX, audio, damage and events). Inside the fixed
steps nothing constrains the chassis, so a body resting on chassis shapes sinks by gravity
every step and is corrected once per frame: the phantom -g*dt velocity and the jitter. Wheels
normally hold a car up through the vehicle solver, which is why this only shows in flight.

## Fix (src/red4ext/Physics/ChassisContacts.cpp)

- Post-hook `SyncFromPhysX`: the velocity change it applied is the contact correction; its
  direction is cached on the FlightComponent as the support normal.
- Pre-hook `IntegrateMotion` (every fixed step): while support was reported in the last 3
  frames, clamp the velocity into that surface to 0.05 m/s. The small creep keeps PhysX
  reporting the contact; off a ledge PhysX stops pushing back, support lapses and the
  vehicle falls normally. Applies to every flight mode, only while flight is active.

Rejected first attempts (script side, `FlightModeDrone`): counter-gravity while "resting"
(still jittered ~1.5 cm, held cars up over edges) and a per-frame velocity correction
(always one frame late: gravity is re-added every fixed step after the script runs).
