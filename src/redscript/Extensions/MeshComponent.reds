// Native field declarations for engine classes.
//
// Codeware declares most of these too. redscript keeps the first declaration it
// sees and warns "a field with this name is already defined in the class, this
// will have no effect" for the rest, so every field Codeware also provides is
// gated on Codeware being absent. The gated names and types match Codeware's
// exactly, so the rest of the mod reads the same field either way.
//
// Fields without a gate are ones Codeware does not declare (entityTags,
// meshApperance). Note meshApperance keeps its historic misspelling: it is our
// own field, it does not collide with Codeware's meshAppearance, and third
// party flight configurations reference it by that name.

// Entity

// @addField(Entity)
// @runtimeProperty("offset", "0x50")
// public native let currentAppearance: CName;

@addField(Entity)
@runtimeProperty("offset", "0x138")
public native let entityTags: array<CName>;

// @addMethod(Entity)
// public native func AddComponent(component: ref<IComponent>);

@addMethod(Entity)
public native func AddSlot(boneName: CName, slotName: CName, relativePosition: Vector3, relativeRotation: Quaternion);

// IComponent

@if(!ModuleExists("Codeware"))
@addField(IComponent)
@runtimeProperty("offset", "0x40")
public native let name: CName;

@if(!ModuleExists("Codeware"))
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

@if(!ModuleExists("Codeware"))
@addField(MeshComponent)
@runtimeProperty("offset", "0x178")
public native let visualScale: Vector3;

@if(!ModuleExists("Codeware"))
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

@if(!ModuleExists("Codeware"))
@addField(MeshComponent)
@runtimeProperty("offset", "0x188")
public native let objectTypeID: ERenderObjectType;

@addField(MeshComponent)
@runtimeProperty("offset", "0x190")
public native let meshApperance: CName;

@if(!ModuleExists("Codeware"))
@addField(MeshComponent)
@runtimeProperty("offset", "0x198")
public native let chunkMask: Uint64;

@if(!ModuleExists("Codeware"))
@addField(MeshComponent)
@runtimeProperty("offset", "0x1A4")
public native let motionBlurScale: Float;

@if(!ModuleExists("Codeware"))
enum entMeshComponentLODMode {
    AlwaysVisible = 0,
    Appearance = 1,
    AppearanceProxy = 2,
}

@if(!ModuleExists("Codeware"))
@addField(MeshComponent)
@runtimeProperty("offset", "0x1A8")
public native let LODMode: entMeshComponentLODMode;

@if(!ModuleExists("Codeware"))
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

@if(!ModuleExists("Codeware"))
@addField(PhysicalMeshComponent)
@runtimeProperty("offset", "0x228")
public native let visibilityAnimationParam: CName;

@if(!ModuleExists("Codeware"))
@addField(PhysicalMeshComponent)
@runtimeProperty("offset", "0x23A")
public native let startInactive: Bool;
