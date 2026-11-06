#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <cstddef>
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
  virtual void getQueryFilterData(); // B8
  virtual void setMaterials(void **materials, uint16_t materialCount); // C0
  virtual uint16_t getNbMaterials(); // C8
  virtual uint32_t getMaterials(void **userBuffer, uint32_t bufferSize, uint32_t startIndex); // D0
  virtual void  getMaterialFromInternalFaceIndex(uint32_t faceIndex); // D8
  virtual void setContactOffset(float contactOffset); // E0
  virtual float getContactOffset(); // E8
  virtual void setRestOffset(); // F0
  virtual float getRestOffset(); // F8
  virtual void setFlag(uint8_t flag, bool value); // 100
  // 108 to 118 checked against NpShape's vtable in the game's DLL (getFlags writes one byte through
  // the hidden return pointer)
  virtual void setFlags(const uint8_t &flags); // 108
  virtual struct PxShapeFlags getFlags() const; // 110
  virtual bool isExclusive(); // 118
};

// PxShapeFlag
enum : uint8_t {
  PxShapeFlag_eSIMULATION_SHAPE = 1,
  PxShapeFlag_eSCENE_QUERY_SHAPE = 2,
  PxShapeFlag_eTRIGGER_SHAPE = 4,
  PxShapeFlag_eVISUALIZATION = 8
};

// PxFlags<PxShapeFlag::Enum, PxU8>: returned through a hidden pointer, as its own copy constructor makes it
struct PxShapeFlags {
  uint8_t bits = 0;
  PxShapeFlags() = default;
  PxShapeFlags(const PxShapeFlags &aOther) : bits(aOther.bits) {}
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
  // 0x190 and 0x198 in the game's 3.4.2 build: the getter returns the body's max penetration bias negated (checked
  // by disassembly against attachShape's slot, 0xB8, 2026-11-06)
  virtual void setMaxDepenetrationVelocity(float maxDepenVel);
  virtual float getMaxDepenetrationVelocity() const;
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

// Scene queries (PhysX 3.4 PxQueryReport.h / PxQueryFiltering.h layouts).

struct PxVec3 {
  float x, y, z;
};

// PxHitFlag
enum : uint16_t { PxHitFlag_ePOSITION = 1, PxHitFlag_eNORMAL = 2, PxHitFlag_eDISTANCE = 4 };
// PxQueryFlag
enum : uint16_t {
  PxQueryFlag_eSTATIC = 1,
  PxQueryFlag_eDYNAMIC = 2,
  PxQueryFlag_ePREFILTER = 4,
  PxQueryFlag_ePOSTFILTER = 8,
  PxQueryFlag_eANY_HIT = 16,
  PxQueryFlag_eNO_BLOCK = 32
};

struct PxFilterData {
  uint32_t word0 = 0, word1 = 0, word2 = 0, word3 = 0;
};

struct PxQueryFilterData {
  PxFilterData data;                                            // 00
  uint16_t flags = PxQueryFlag_eSTATIC | PxQueryFlag_eDYNAMIC; // 10
  // PxClientID, PX_DEFAULT_CLIENT: the query looks its behaviour flags up by it, so it must be set
  uint8_t clientId = 0;                                         // 12
};

// As the game's build fills it (hits dumped in game: the distance at 0x34 matches the hit position);
// there is a word between faceIndex and flags that the 3.4 headers do not show.
struct PxRaycastHit {
  PxRigidActor *actor;  // 00 PxActorShape
  PxShape *shape;       // 08
  uint32_t faceIndex;   // 10 PxQueryHit
  uint32_t unknown14;   // 14
  uint16_t flags;       // 18 PxLocationHit
  PxVec3 position;      // 1C
  PxVec3 normal;        // 28
  float distance;       // 34
  float u, v;           // 38 PxRaycastHit
};
static_assert(sizeof(PxRaycastHit) == 0x40);
static_assert(offsetof(PxRaycastHit, position) == 0x1C && offsetof(PxRaycastHit, distance) == 0x34);
static_assert(offsetof(PxRaycastHit, normal) == 0x28);

// PxHitCallback<PxRaycastHit> (PxRaycastCallback): the blocking hit in block; touches, if a buffer is
// given, through processTouches.
struct PxRaycastCallback {
  PxRaycastHit block{};   // 08
  bool hasBlock = false;  // 48
  PxRaycastHit *touches;  // 50
  uint32_t maxNbTouches;  // 58
  uint32_t nbTouches = 0; // 5C

  PxRaycastCallback(PxRaycastHit *aTouches = nullptr, uint32_t aMaxNbTouches = 0)
      : touches(aTouches), maxNbTouches(aMaxNbTouches) {}
  virtual bool processTouches(const PxRaycastHit *, uint32_t) { return false; } // 00
  virtual void finalizeQuery() {}                                                // 08
  virtual ~PxRaycastCallback() {}                                                // 10
};

// PxQueryHitType
enum PxQueryHitType : uint32_t { PxQueryHitType_eNONE = 0, PxQueryHitType_eTOUCH = 1, PxQueryHitType_eBLOCK = 2 };

// PxQueryFilterCallback: preFilter runs per shape when the query's flags include ePREFILTER.
struct PxQueryFilterCallback {
  virtual PxQueryHitType preFilter(const PxFilterData &filterData, const PxShape *shape, const PxRigidActor *actor,
                                   uint16_t &queryFlags) = 0;                           // 00
  virtual PxQueryHitType postFilter(const PxFilterData &filterData, const void *hit) = 0; // 08
  virtual ~PxQueryFilterCallback() {}                                                     // 10
};

// PxScene, as NpScene's vtable is laid out in the game's PhysX 3.4.2 build: slot 0 the destructor,
// 1 release() ... 94 raycast, 95 sweep, 96 overlap. Found and re-checkable with
// tools/physx/pxscene_vtable.py (vtable at rva 0x20caf0 in the 2.5 DLL). Only raycast is called, by
// slot. Scene queries are what the game's vehicles run from their fixed update (wheels): call it
// from there.
struct PxScene {
  static constexpr size_t kRaycastSlot = 94;

  bool raycast(const PxVec3 &origin, const PxVec3 &unitDir, float distance, PxRaycastCallback &hitCall,
               uint16_t hitFlags = PxHitFlag_ePOSITION | PxHitFlag_eNORMAL | PxHitFlag_eDISTANCE,
               const PxQueryFilterData &filterData = {}, void *filterCall = nullptr, const void *cache = nullptr) const {
    // PxHitFlags (PxFlags, which has its own copy constructor) goes by reference: the DLL reads the
    // sixth argument as a pointer to the 16 bit flags
    using Raycast = bool (*)(const PxScene *, const PxVec3 &, const PxVec3 &, float, PxRaycastCallback &,
                             const uint16_t &, const PxQueryFilterData &, void *, const void *);
    auto vtable = *reinterpret_cast<Raycast const *const *>(this);
    return vtable[kRaycastSlot](this, origin, unitDir, distance, hitCall, hitFlags, filterData, filterCall, cache);
  }
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


