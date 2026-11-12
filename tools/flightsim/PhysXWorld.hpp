#pragma once

// The game's own PhysX 3.4.2 (its PhysX3_x64.dll and the DLLs it loads, from the game's bin/x64, read only) running a
// car body over the city's collision, for what flightsim's own contact cannot show: PhysX does not know a triangle's
// neighbours across two meshes and pushes a shape sliding over the joint. Each mesh of the
// world is added as its own triangle mesh actor, as the game has it.
//
// Built only into flightsim, against the PhysX 3.4 headers (NVIDIA's PhysX-3.4 repository, BSD-3; LTBF_PHYSX34 in
// CMake); the DLLs are loaded at run time and never shipped or committed.
// Nothing here includes PhysX: the rest of flightsim sees plain vectors.

#include "Flight/ScriptMath.hpp"

#include <memory>
#include <string>
#include <vector>

namespace FlightSim {

struct PhysXOptions {
  bool pcm = false;                                  // PxSceneFlag::eENABLE_PCM
  float staticFriction = 0.5f, dynamicFriction = 0.5f, restitution = 0.0f;
  float gravity = 9.81f;
};

class PhysXWorld {
public:
  // Loads PhysX from aDllDir (the game's bin/x64) and makes a scene; null with aError when it cannot.
  static std::unique_ptr<PhysXWorld> Create(const std::string &aDllDir, const PhysXOptions &aOptions,
                                            std::string &aError);
  ~PhysXWorld();

  // One static triangle mesh, three corners a triangle (world), as one collision mesh of the game.
  void AddMesh(const std::vector<Flight::Vec3> &aCorners);
  size_t Meshes() const;

  // The car's shapes, body frame from the centre of mass (X right, Y forward, Z up).
  struct Shape {
    bool box = false;
    Flight::Vec3 center, halfExtents;
    float radius = 0.0f;
  };
  // The car: mass (kg), inertia per body axis (kg m2), its shapes, its centre of mass's pose and velocities (world).
  void SetBody(float aMass, Flight::Vec3 aInertia, const std::vector<Shape> &aShapes, Flight::Vec3 aPosition,
               Flight::Quat aOrientation, Flight::Vec3 aVelocity, Flight::Vec3 aAngularVelocity);
  // Puts the body where the caller has it (after a teleport, a knock).
  void SetState(Flight::Vec3 aPosition, Flight::Quat aOrientation, Flight::Vec3 aVelocity,
                Flight::Vec3 aAngularVelocity);
  // One step: the force (N) and torque (N m) on the body this step (world), PhysX's gravity and contacts, the body's
  // linear damping (1/s) and its spin cap (rad/s).
  void Step(float aDt, Flight::Vec3 aForce, Flight::Vec3 aTorque, float aLinearDamping, float aMaxAngularSpeed);

  Flight::Vec3 Position() const;
  Flight::Quat Orientation() const;
  Flight::Vec3 Velocity() const;
  Flight::Vec3 AngularVelocity() const; // world
  // the contacts the body had in the last step: how many points, and the sum of their impulses (N s)
  int ContactPoints() const;
  float ContactImpulse() const;

private:
  PhysXWorld();
  struct Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace FlightSim
