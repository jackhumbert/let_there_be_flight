// Entity

// @addField(Entity)
// @runtimeProperty("offset", "0x50")
// public native let currentAppearance: CName;

@addField(Entity)
@runtimeProperty("offset", "0x138")
public native let entityTags: array<CName>;

@addMethod(Entity)
public native func AddComponent(component: ref<IComponent>);

@addMethod(Entity)
public native func AddSlot(boneName: CName, slotName: CName, relativePosition: Vector3, relativeRotation: Quaternion);

// IComponent

@addField(IComponent)
@runtimeProperty("offset", "0x40")
public native let name: CName;

@addField(IComponent)
@runtimeProperty("offset", "0x48")
public native let appearanceName: CName;

// IPlacedComponent

// bindName is component name - can be SlotComponent like vehicle_slots
@addMethod(IPlacedComponent)
public native func SetParentTransform(bindName: CName, slotName: CName);

// MeshComponent

@addMethod(MeshComponent)
public native func SetMesh(mesh: ResRef);

@addField(MeshComponent)
@runtimeProperty("offset", "0x178")
public native let visualScale: Vector3;

enum ERenderObjectType {
    ROT_Static = 0,
    ROT_Terrain = 1,
    ROT_Road = 2,
    ROT_CustomCharacter1 = 12,
    ROT_CustomCharacter2 = 13,
    ROT_CustomCharacter3 = 14,
    ROT_MainPlayer = 15,
    ROT_NoAO = 16,
    ROT_NoLighting = 17,
    ROT_NoTXAA = 18,
    ROT_Skinned = 20,
    ROT_Character = 21,
    ROT_Foliage = 22,
    ROT_Grass = 23,
    ROT_Vehicle = 24,
    ROT_Weapon = 25,
    ROT_Particle = 26,
    ROT_Enemy = 27,
}

@addField(MeshComponent)
@runtimeProperty("offset", "0x188")
public native let objectTypeID: ERenderObjectType;

@addField(MeshComponent)
@runtimeProperty("offset", "0x190")
public native let meshApperance: CName;

@addField(MeshComponent)
@runtimeProperty("offset", "0x198")
public native let chunkMask: Uint64;

@addField(MeshComponent)
@runtimeProperty("offset", "0x1A4")
public native let motionBlurScale: Float;

enum entMeshComponentLODMode {
    AlwaysVisible = 0,
    Appearance = 1,
    AppearanceProxy = 2,
}

@addField(MeshComponent)
@runtimeProperty("offset", "0x1A8")
public native let LODMode: entMeshComponentLODMode;

@addField(MeshComponent)
@runtimeProperty("offset", "0x1AB")
public native let order: Uint8;

// @addField(MeshComponent)
// @runtimeProperty("offset", "0x1AC")
// public native let castShadows: Bool;

// @addField(MeshComponent)
// @runtimeProperty("offset", "0x1AD")
// public native let castLocalShadows: Bool;

// PhysicalMeshComponent

@addField(PhysicalMeshComponent)
@runtimeProperty("offset", "0x228")
public native let visibilityAnimationParam: CName;

@addField(PhysicalMeshComponent)
@runtimeProperty("offset", "0x23A")
public native let startInactive: Bool;