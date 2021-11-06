

// native let colliders: array<ref<physicsICollider>>;
// native let shapes: array<ref<entColliderComponentShape>>;
// native let simulationType: physicsSimulationType;
// native let startInactive: Bool;
// native let useCCD: Bool;
// native let sendOnStoppedMovingEvents: Bool;
@addField(ColliderComponent)
native let massOverride: Float;
@addField(ColliderComponent)
native let volume: Float;
@addField(ColliderComponent)
native let mass: Float;
@addField(ColliderComponent)
native let inertia: Vector3;
@addField(ColliderComponent)
native let comOffset: Transform;
// native let filterData: ref<physicsFilterData>;
@addField(ColliderComponent)
native let isEnabled: Bool;
// native let dynamicTrafficSetting: TrafficGenDynamicTrafficSetting;
// native func CreatePhysicalBodyInterface(bodyIndex: Uint32) -> ref<entPhysicalBodyInterface>;
