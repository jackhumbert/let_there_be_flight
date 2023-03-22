#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <cstdint>

// imported from PhysX3_x64.dll

namespace physx {

struct PxTransform {
  RED4ext::Quaternion q;
  RED4ext::Vector3 p;
};

class PxBase {
public:
  virtual void release(); // 00
  virtual const char* getConcreteTypeName() const; // 08
  virtual bool isReleasable(); // 10

protected:
  virtual ~PxBase(); // 18
  virtual bool isKindOf(const char* superClass) const; // 20
  uint16_t mConcreteType;
  uint16_t mBaseFlags;
};

class PxRigidActor;

class PxShape : PxBase {
public:
  virtual void release2(); // 28
  virtual void acquireReference(); // 30
  virtual void getGeometryType(); // 38
  virtual void setGeometry(); // 40
  virtual void getGeometry(); // 48
  virtual void getBoxGeometry(); // 50
  virtual void getSphereGeometry(); // 58
  virtual void getCapsuleGeometry(); // 60
  virtual void getPlaneGeometry(); // 68
  virtual void getConvexMeshGeometry(); // 70
  virtual void getTriangleMeshGeometry(); // 78
  virtual void getHeightFieldGeometry(); // 80
  virtual PxRigidActor* getActor() const; // 88
  virtual void setLocalPose(const PxTransform& pose); // 90
  virtual PxTransform getLocalPose() const; // 98
  virtual void setSimulationFilterData(void *data); // A0
  virtual void getSimulationFilterData(); // A8
  virtual void setQueryFilterData(void *data); // B0
  virtual void getQueryFilterData();
  virtual void setMaterials(void **materials, uint16_t materialCount);
  virtual uint16_t getNbMaterials();
  virtual uint32_t getMaterials(void **userBuffer, uint32_t bufferSize, uint32_t startIndex);
  virtual void  getMaterialFromInternalFaceIndex(uint32_t faceIndex);
  virtual void setContactOffset(float contactOffset);
  virtual float getContactOffset();
  virtual void setRestOffset();
  virtual float getRestOffset();
  virtual void setFlag(uint8_t flag, bool value);
};

class PxActor : public PxBase {
public:
  // virtual void release(); // 00
  virtual uint16_t getType() const; // 28
  virtual void *getScene() const; // 30
  virtual void setName(const char *name); // 38
  virtual const char *getName() const; // 40
  virtual void* getWorldBounds(float inflation = 1.01f) const; // 48
  virtual void setActorFlag(uint16_t flag, bool value); // 50
  virtual void setActorFlags(uint16_t inFlags); // 58
  virtual uint16_t getActorFlags() const; // 60
  virtual void setDominanceGroup(void* dominanceGroup); // 68
  virtual void * getDominanceGroup() const; // 70
  virtual void setOwnerClient(void* inClient); // 78
  virtual void* getOwnerClient() const; // 80
  virtual void setClientBehaviorFlags(void*); // 88
  virtual uint16_t getClientBehaviorFlags() const; // 90
  virtual void *getAggregate() const; // 98

  void *userData;

// protected:
  // virtual ~PxActor() {}
  // virtual bool isKindOf(const char *name) const {
    // return !::strcmp("PxActor", name) || PxBase::isKindOf(name);
  // }
};

class PxRigidActor : public PxActor {
public:
  // virtual void release();

  virtual PxTransform getGlobalPose(); // A0
  virtual void setGlobalPose(); // A8

  virtual PxShape* createShape(); // B0
  virtual void attachShape(physx::PxShape& shape); // B8
  virtual void detachShape(physx::PxShape& shape, bool wakeOnLostTouch = true); // C0
  virtual uint32_t getNbShapes() const; // C8
  virtual uint32_t getShapes(physx::PxShape **, uint32_t bufferSize, uint32_t startIndex=0) const; // D0

  virtual void getNbConstraints();
  virtual void getConstraints();

// protected:
//   virtual ~PxRigidActor() {}
//   virtual bool isKindOf(const char* name) const {
//     return !::strcmp("PxRigidActor", name) || PxActor::isKindOf(name);
//   }
};

struct PxRigidBody : PxRigidActor
{
  virtual void setCMassLocalPose(void *pose);
  virtual void getCMassLocalPose();
  virtual void setMass();
  virtual void getMass();
  virtual void getInvMass();
  virtual void setMassSpaceInertiaTensor();
  virtual void getMassSpaceInertiaTensor(float);
  virtual void getMassSpaceInvInertiaTensor();
  virtual void getLinearVelocity(float);
  virtual void setLinearVelocity();
  virtual void getAngularVelocity();
  virtual void setAngularVelocity();
  virtual bool addForce();
  virtual bool addTorque();
  virtual bool clearForce();
  virtual bool clearTorque();
  virtual bool setRigidBodyFlag(uint32_t flag, bool value);
  virtual bool setRigidBodyFlags();
  virtual bool getRigidBodyFlags();
  virtual bool setMinCCDAdvanceCoefficient();
  virtual bool getMinCCDAdvanceCoefficient();
  virtual bool setMaxDepenetrationVelocity();
  virtual bool getMaxDepenetrationVelocity();
  virtual bool setMaxContactImpulse();
  virtual void getMaxContactImpulse();
};

struct PxRigidDynamic : PxRigidBody {

  virtual void setKinematicTarget();
  virtual void getKinematicTarget();
  virtual void setLinearDamping(float);
  virtual void getLinearDamping();
  virtual void setAngularDamping(float);
  virtual void getAngularDamping();
  virtual void setMaxAngularVelocity();
  virtual void getMaxAngularVelocity();
  virtual bool isSleeping();
  virtual bool setSleepThreshold();
  virtual bool getSleepThreshold();
  virtual bool setStabilizationThreshold();
  virtual bool getStabilizationThreshold();
  virtual bool getRigidDynamicLockFlags();
  virtual bool setRigidDynamicLockFlag();
  virtual bool setRigidDynamicLockFlags();
  virtual bool setWakeCounter();
  virtual bool getWakeCounter();
  virtual bool wakeUp();
  virtual bool putToSleep();
  virtual void setSolverIterationCounts(uint32_t minPositionIters, uint32_t minVelocityIters);
  virtual void getSolverIterationCounts(uint32_t minPositionIters, uint32_t minVelocityIters);
  virtual bool getContactReportThreshold();
  virtual bool setContactReportThreshold();
  virtual bool getConcreteTypeName();
};

struct PxGeometry {};
struct PxMaterial {};

struct PxPhysics {
  virtual ~PxPhysics();
  virtual void  release();
  virtual void * getFoundation();
  virtual void * createAggregate(uint32_t maxSize, bool enableSelfCollision);
  virtual void * getTolerancesScale();
  virtual void * createTriangleMesh(void *stream);
  virtual void * getNbTriangleMeshes();
  virtual void * getTriangleMeshes(void *userBuffer, uint32_t bufferSize, uint32_t startIndex);
  virtual void * createHeightField();
  virtual void * getNbHeightFields();
  virtual void * getHeightFields();
  virtual void * createConvexMesh();
  virtual void * getNbConvexMeshes();
  virtual void * getConvexMeshes();
  virtual void * createClothFabric();
  virtual void * createClothFabric2(void *);
  virtual void * getNbClothFabrics();
  virtual void * getClothFabrics();
  virtual void * createScene();
  virtual void * getNbScenes();
  virtual void * getScenes();
  virtual void * createRigidStatic(PxTransform *);
  virtual void * createRigidDynamic(PxTransform *);
  virtual void * createParticleSystem(uint32_t maxParticles, bool perParticleRestOffset);
  virtual void * createParticleFluid(uint32_t maxParticles, bool perParticleRestOffset);
  virtual void * createCloth(void *globalPose, void *fabric, void *particles, uint32_t flags);
  virtual void * createPruningStructure();
  virtual void * createShape(const PxGeometry& geometry, PxMaterial*const * materials, uint16_t materialCount, bool isExclusive = false, uint16_t shapeFlags = 11 );
  virtual void * getNbShapes();
  virtual void * getShapes(PxShape** userBuffer, uint32_t bufferSize, uint32_t startIndex=0);
  virtual void * createConstraint(void *actor0, void *actor1, void *connector, void *shaders, uint32_t dataSize);
  virtual void * createArticulation();
  virtual void * createMaterial();
  virtual void * getNbMaterials();
  virtual void * getMaterials();
  virtual void * registerDeletionListener();
  virtual void * unregisterDeletionListener();
  virtual void * registerDeletionListenerObjects();
  virtual void * unregisterDeletionListenerObjects();
  virtual void * getPhysicsInsertionCallback();
};


} // namespace physx

inline physx::PxPhysics* PxGetPhysics() {
  auto hdl = LoadLibraryA("PhysX3_x64.dll");
  if (hdl) {
    auto func = reinterpret_cast<decltype(&PxGetPhysics)>(GetProcAddress(hdl, "PxGetPhysics"));
    return func();
  } else {
    return nullptr;
  }
}


