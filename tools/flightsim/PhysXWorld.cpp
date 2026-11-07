#include "PhysXWorld.hpp"

#include <PxPhysicsAPI.h>

#include <windows.h>

#include <cmath>
#include <cstdio>
#include <malloc.h>

using namespace physx;

namespace FlightSim {

namespace {

PxVec3 To(Flight::Vec3 v) { return {v.x, v.y, v.z}; }
Flight::Vec3 From(const PxVec3 &v) { return {v.x, v.y, v.z}; }
PxQuat To(Flight::Quat q) { return {q.i, q.j, q.k, q.r}; }

class Allocator : public PxAllocatorCallback {
public:
  void *allocate(size_t aSize, const char *, const char *, int) override { return _aligned_malloc(aSize, 16); }
  void deallocate(void *aPtr) override { _aligned_free(aPtr); }
};

class Errors : public PxErrorCallback {
public:
  void reportError(PxErrorCode::Enum aCode, const char *aMessage, const char *aFile, int aLine) override {
    if (aCode == PxErrorCode::eDEBUG_INFO)
      return;
    std::fprintf(stderr, "PhysX %d: %s (%s:%d)\n", static_cast<int>(aCode), aMessage, aFile, aLine);
  }
};

// every task run at once on the calling thread, as PxDefaultCpuDispatcher with no worker threads
class Dispatcher : public PxCpuDispatcher {
public:
  void submitTask(PxBaseTask &aTask) override {
    aTask.run();
    aTask.release();
  }
  PxU32 getWorkerCount() const override { return 0; }
};

// every pair collides and reports its contacts, as PxDefaultSimulationFilterShader does for unfiltered pairs
PxFilterFlags Shader(PxFilterObjectAttributes, PxFilterData, PxFilterObjectAttributes, PxFilterData,
                     PxPairFlags &aFlags, const void *, PxU32) {
  aFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
           PxPairFlag::eNOTIFY_CONTACT_POINTS;
  return PxFilterFlag::eDEFAULT;
}

class Contacts : public PxSimulationEventCallback {
public:
  int points = 0;
  float impulse = 0.0f;
  void onContact(const PxContactPairHeader &, const PxContactPair *aPairs, PxU32 aCount) override {
    PxContactPairPoint buffer[64];
    for (PxU32 i = 0; i < aCount; ++i) {
      const PxU32 n = aPairs[i].extractContacts(buffer, 64);
      for (PxU32 j = 0; j < n; ++j) {
        ++points;
        impulse += buffer[j].impulse.magnitude();
      }
    }
  }
  void onConstraintBreak(PxConstraintInfo *, PxU32) override {}
  void onWake(PxActor **, PxU32) override {}
  void onSleep(PxActor **, PxU32) override {}
  void onTrigger(PxTriggerPair *, PxU32) override {}
  void onAdvance(const PxRigidBody *const *, const PxTransform *, const PxU32) override {}
};

using CreateFoundation = PxFoundation *(*)(PxU32, PxAllocatorCallback &, PxErrorCallback &);
using CreateBasePhysics = PxPhysics *(*)(PxU32, PxFoundation &, const PxTolerancesScale &, bool, PxPvd *);
using CreateCooking = PxCooking *(*)(PxU32, PxFoundation &, const PxCookingParams &);

// CDPR's build of PhysX 3.4 (its source tree says physx342) reports 3.4.3 and refuses the public 3.4.2's number
constexpr PxU32 kGameVersion = 0x03040300;

// Its PxShape has one virtual more than the public 3.4.2 header's, early on: setLocalPose is the vtable's 0x90, not
// 0x88 (deps/physx/PhysX3.hpp, checked in game; through the public header the shapes' offsets were lost and every shape
// sat at the centre of mass). PxRigidActor, PxRigidBody and PxRigidDynamic match the public header (the game's
// setMaxDepenetrationVelocity, 0x190, is the header's). Only the PxShape calls made here go through the game's slots.
void SetLocalPose(PxShape *aShape, const PxTransform &aPose) {
  using Fn = void (*)(PxShape *, const PxTransform &);
  (*reinterpret_cast<Fn **>(aShape))[0x90 / 8](aShape, aPose);
}
} // namespace

struct PhysXWorld::Impl {
  Allocator allocator;
  Errors errors;
  Dispatcher dispatcher;
  Contacts contacts;
  HMODULE foundationDll = nullptr, physicsDll = nullptr, cookingDll = nullptr;
  PxFoundation *foundation = nullptr;
  PxPhysics *physics = nullptr;
  PxCooking *cooking = nullptr;
  PxScene *scene = nullptr;
  PxMaterial *material = nullptr;
  PxRigidDynamic *body = nullptr;
  size_t meshes = 0;
};

PhysXWorld::PhysXWorld() : impl(std::make_unique<Impl>()) {}

PhysXWorld::~PhysXWorld() {
  if (impl->scene)
    impl->scene->release();
  if (impl->cooking)
    impl->cooking->release();
  if (impl->physics)
    impl->physics->release();
  if (impl->foundation)
    impl->foundation->release();
}

std::unique_ptr<PhysXWorld> PhysXWorld::Create(const std::string &aDllDir, const PhysXOptions &aOptions,
                                                std::string &aError) {
  std::unique_ptr<PhysXWorld> w(new PhysXWorld());
  Impl &m = *w->impl;
  // each DLL by its full path, its own folder searched for the ones it loads; PhysX loads PhysX3Common and the rest late
  // (delay load), which searches the DLL directory, not the loading DLL's folder
  {
    std::string dir = aDllDir;
    for (char &c : dir)
      c = c == '/' ? '\\' : c;
    SetDllDirectoryA(dir.c_str());
  }
  auto load = [&](const char *aName) {
    std::string path = aDllDir + "\\" + aName;
    for (char &c : path)
      c = c == '/' ? '\\' : c; // LOAD_WITH_ALTERED_SEARCH_PATH takes backslashes only
    HMODULE h = LoadLibraryExA(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!h)
      aError = "cannot load " + path;
    return h;
  };
  if (!(m.foundationDll = load("PxFoundation_x64.dll")) || !(m.physicsDll = load("PhysX3_x64.dll")) ||
      !(m.cookingDll = load("PhysX3Cooking_x64.dll")))
    return nullptr;
  auto createFoundation = reinterpret_cast<CreateFoundation>(GetProcAddress(m.foundationDll, "PxCreateFoundation"));
  auto createPhysics = reinterpret_cast<CreateBasePhysics>(GetProcAddress(m.physicsDll, "PxCreateBasePhysics"));
  auto createCooking = reinterpret_cast<CreateCooking>(GetProcAddress(m.cookingDll, "PxCreateCooking"));
  if (!createFoundation || !createPhysics || !createCooking) {
    aError = "a PhysX entry point is missing";
    return nullptr;
  }
  m.foundation = createFoundation(PX_FOUNDATION_VERSION, m.allocator, m.errors);
  if (!m.foundation) {
    aError = "PxCreateFoundation failed (version)";
    return nullptr;
  }
  PxTolerancesScale scale;
  m.physics = createPhysics(kGameVersion, *m.foundation, scale, false, nullptr);
  m.cooking = m.physics ? createCooking(kGameVersion, *m.foundation, PxCookingParams(scale)) : nullptr;
  if (!m.physics || !m.cooking) {
    aError = "PxCreateBasePhysics or PxCreateCooking failed";
    return nullptr;
  }
  PxSceneDesc desc(scale);
  desc.gravity = PxVec3(0.0f, 0.0f, -aOptions.gravity);
  desc.cpuDispatcher = &m.dispatcher;
  desc.filterShader = Shader;
  desc.simulationEventCallback = &m.contacts;
  if (aOptions.pcm)
    desc.flags |= PxSceneFlag::eENABLE_PCM;
  m.scene = m.physics->createScene(desc);
  m.material = m.physics->createMaterial(aOptions.staticFriction, aOptions.dynamicFriction, aOptions.restitution);
  if (!m.scene || !m.material) {
    aError = "the scene could not be made";
    return nullptr;
  }
  return w;
}

void PhysXWorld::AddMesh(const std::vector<Flight::Vec3> &aCorners) {
  Impl &m = *impl;
  if (aCorners.size() < 3)
    return;
  std::vector<PxVec3> points;
  std::vector<PxU32> indices;
  points.reserve(aCorners.size());
  for (size_t i = 0; i < aCorners.size(); ++i) {
    points.push_back(To(aCorners[i]));
    indices.push_back(static_cast<PxU32>(i));
  }
  PxTriangleMeshDesc desc;
  desc.points.count = static_cast<PxU32>(points.size());
  desc.points.stride = sizeof(PxVec3);
  desc.points.data = points.data();
  desc.triangles.count = static_cast<PxU32>(indices.size() / 3);
  desc.triangles.stride = 3 * sizeof(PxU32);
  desc.triangles.data = indices.data();
  // the corners welded, as a cooked game mesh has them, so neighbours within one mesh are known
  PxCookingParams params = m.cooking->getParams();
  params.meshPreprocessParams |= PxMeshPreprocessingFlag::eWELD_VERTICES;
  params.meshWeldTolerance = 0.001f;
  m.cooking->setParams(params);
  PxTriangleMesh *mesh = m.cooking->createTriangleMesh(desc, m.physics->getPhysicsInsertionCallback());
  if (!mesh)
    return;
  PxRigidStatic *actor = m.physics->createRigidStatic(PxTransform(PxIdentity));
  PxShape *shape = m.physics->createShape(PxTriangleMeshGeometry(mesh), *m.material, true);
  actor->attachShape(*shape);
  shape->release();
  mesh->release();
  m.scene->addActor(*actor);
  ++m.meshes;
}

size_t PhysXWorld::Meshes() const { return impl->meshes; }

void PhysXWorld::SetBody(float aMass, Flight::Vec3 aInertia, const std::vector<Shape> &aShapes,
                         Flight::Vec3 aPosition, Flight::Quat aOrientation, Flight::Vec3 aVelocity,
                         Flight::Vec3 aAngularVelocity) {
  Impl &m = *impl;
  if (m.body) {
    m.scene->removeActor(*m.body);
    m.body->release();
  }
  m.body = m.physics->createRigidDynamic(PxTransform(To(aPosition), To(aOrientation)));
  for (const Shape &s : aShapes) {
    PxShape *shape = s.box ? m.physics->createShape(PxBoxGeometry(To(s.halfExtents)), *m.material, true)
                           : m.physics->createShape(PxSphereGeometry(s.radius), *m.material, true);
    SetLocalPose(shape, PxTransform(To(s.center)));
    m.body->attachShape(*shape);
    shape->release();
  }
  m.body->setMass(aMass);
  m.body->setMassSpaceInertiaTensor(To(aInertia));
  m.body->setCMassLocalPose(PxTransform(PxIdentity));
  m.body->setAngularDamping(0.0f);
  m.body->setSleepThreshold(0.0f);
  m.scene->addActor(*m.body);
  m.body->setLinearVelocity(To(aVelocity));
  m.body->setAngularVelocity(To(aAngularVelocity));
}

void PhysXWorld::SetState(Flight::Vec3 aPosition, Flight::Quat aOrientation, Flight::Vec3 aVelocity,
                          Flight::Vec3 aAngularVelocity) {
  Impl &m = *impl;
  if (!m.body)
    return;
  m.body->setGlobalPose(PxTransform(To(aPosition), To(aOrientation)));
  m.body->setLinearVelocity(To(aVelocity));
  m.body->setAngularVelocity(To(aAngularVelocity));
}

void PhysXWorld::Step(float aDt, Flight::Vec3 aForce, Flight::Vec3 aTorque, float aLinearDamping,
                      float aMaxAngularSpeed) {
  Impl &m = *impl;
  if (!m.body)
    return;
  m.body->setLinearDamping(aLinearDamping);
  m.body->setMaxAngularVelocity(aMaxAngularSpeed);
  m.body->addForce(To(aForce), PxForceMode::eFORCE);
  m.body->addTorque(To(aTorque), PxForceMode::eFORCE);
  m.contacts.points = 0;
  m.contacts.impulse = 0.0f;
  m.scene->simulate(aDt);
  m.scene->fetchResults(true);
}

Flight::Vec3 PhysXWorld::Position() const { return From(impl->body->getGlobalPose().p); }
Flight::Quat PhysXWorld::Orientation() const {
  const PxQuat q = impl->body->getGlobalPose().q;
  return {q.x, q.y, q.z, q.w};
}
Flight::Vec3 PhysXWorld::Velocity() const { return From(impl->body->getLinearVelocity()); }
Flight::Vec3 PhysXWorld::AngularVelocity() const { return From(impl->body->getAngularVelocity()); }
int PhysXWorld::ContactPoints() const { return impl->contacts.points; }
float PhysXWorld::ContactImpulse() const { return impl->contacts.impulse; }

} // namespace FlightSim
