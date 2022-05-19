public native class FlightAudio {
  // defined in red4ext part
  public native func Start(emitterName: String, eventName: String) -> Void;
  public native func Play(eventName: String) -> Void;
  public native func Stop(emitterName: String) -> Void;
  // public native func Update(emitterName: String, eventLocation: Vector3, eventForward: Vector3, eventUp: Vector3, volume: Float) -> Void;
  public native func Update(emitterName: String, eventLocation: Vector4, volume: Float) -> Void;

  public let parameters: array<String>;

  public let speed: Float;
  public let surge: Float;
  public let yawDiff: Float;
  public let lift: Float;
  public let yaw: Float;
  public let pitchDiff: Float;
  public let brake: Float;
  public let inside: Float;
  public let damage: Float;
  public let water: Float;

  public let listenerPosition: Vector4;
  public let listenerUp: Vector4;
  public let listenerForward: Vector4;

  private let m_positionProviders: ref<inkHashMap>;
  private let m_orientationProviders: ref<inkHashMap>;
  private let m_positions: ref<inkHashMap>;
  private let m_orientations: ref<inkHashMap>;
  private let slots: array<CName>;

  public static func Create() -> ref<FlightAudio> {
    let self = new FlightAudio();

    self.speed = 0.0;
    self.surge = 0.0;
    self.yawDiff = 0.0;
    self.lift = 0.0;
    self.yaw = 0.0;
    self.pitchDiff = 0.0;
    self.brake = 0.0;
    self.inside = 0.0;
    self.damage = 0.0;
    self.water = 0.0;

    self.parameters = [
      "speed",
      "surge",
      "yawDiff",
      "lift",
      "yaw",
      "pitchDiff",
      "brake",
      "inside",
      "damage",
      "water"
    ];

    self.listenerPosition = new Vector4(0.0, 0.0, 0.0, 0.0);
    self.listenerUp = new Vector4(0.0, 0.0, 1.0, 0.0);
    self.listenerForward = new Vector4(0.0, 1.0, 0.0, 0.0);

    self.m_positionProviders = new inkHashMap();
    self.m_positions = new inkHashMap();
    self.m_orientationProviders = new inkHashMap();
    self.m_orientations = new inkHashMap();
    self.slots = [
      // n"steering_wheel",
      // n"roofhatch",
      // n"glove_box",
      // n"Base",
      // n"dseat_a",
      // n"dseat_b",
      // n"bumper_front_a",
      // n"bumper_front_b",
      // n"mirror_front_left",
      // n"mirror_front_right",
      n"wheel_front",
      n"wheel_back",
      n"wheel_front_left",
      n"wheel_front_right",
      n"wheel_back_left",
      n"wheel_back_right",
      // n"bumper_back",
      n"window_front_left_a",
      n"window_front_right_a"
    ];
    return self;
  }

  public func AddSlotProvider(entity: ref<Entity>, slot: CName) {
    this.SetPositionProvider(slot, IPositionProvider.CreateSlotPositionProvider(entity, slot));
    // this.SetOrientationProvider(slot, IOrientationProvider.CreateEntityOrientationProvider(null, slot, entity));
  }

  public func AddSlotProviders(entity: ref<Entity>) {
    for slot in this.slots {
      this.SetPositionProvider(slot, IPositionProvider.CreateSlotPositionProvider(entity, slot));
      // this.SetOrientationProvider(slot, IOrientationProvider.CreateEntityOrientationProvider(null, slot, entity));
    }
  }
  
  public func UpdateSlotProvider(slot: CName) {
    this.UpdatePosition(slot);
    // this.UpdateOrientation(slot);
  }

  public func UpdateSlotProviders() {
    for slot in this.slots {
      this.UpdateSlotProvider(slot);
    }
  }

  public func DrawSlotPositions(ui: ref<FlightControllerUI>) {
    for slot in this.slots {
      let position = this.GetPosition(slot);
      ui.DrawMark(position);
      ui.DrawText(position, NameToString(slot));
    }
  }

	public func UpdatePosition(name: CName) -> Void {
		let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));
		if this.m_positionProviders.KeyExist(key) {
      let position = new Vector4Wrapper();
		  (this.m_positionProviders.Get(key) as IPositionProvider).CalculatePosition(position.vector);
      if this.m_positions.KeyExist(key) {
        this.m_positions.Set(key, position);
      } else {
        this.m_positions.Insert(key, position);
      }
    }
  }

	// public func UpdateOrientation(name: CName) -> Void {
	// 	let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));
	// 	if this.m_orientationProviders.KeyExist(key) {
  //     let orientation = new OrientationWrapper();
	// 	  (this.m_orientationProviders.Get(key) as EntityOrientationProvider).CalculateOrientation(orientation.quaternion);
  //     if this.m_orientations.KeyExist(key) {
  //       this.m_orientations.Set(key, orientation);
  //     } else {
  //       this.m_orientations.Insert(key, orientation);
  //     }
  //   }
	// }

	public func GetPosition(name: CName) -> Vector4 {
    let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));
		if this.m_positions.KeyExist(key) {
		  return (this.m_positions.Get(key) as Vector4Wrapper).vector;
    } else {
      return Vector4.EmptyVector();
    }
  }

	public func SetPositionProvider(name: CName, positionProvider: ref<IPositionProvider>) -> Void {
		let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));

		if this.m_positionProviders.KeyExist(key) {
			this.m_positionProviders.Set(key, positionProvider);
		} else {
			this.m_positionProviders.Insert(key, positionProvider);
		}
    this.UpdatePosition(name);
	}

	// public func SetOrientationProvider(name: CName, orientationProvider: ref<IOrientationProvider>) -> Void {
	// 	let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));

	// 	if this.m_orientationProviders.KeyExist(key) {
	// 		this.m_orientationProviders.Set(key, orientationProvider);
	// 	} else {
	// 		this.m_orientationProviders.Insert(key, orientationProvider);
	// 	}
  //   this.UpdateOrientation(name);
	// }
}

public class Vector4Wrapper {
  public let vector: Vector4;
}

public class OrientationWrapper {
  public let quaternion: Quaternion;
}