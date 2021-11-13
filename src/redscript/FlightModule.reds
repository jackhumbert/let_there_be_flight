// HUDInstruction

// @addField(HUDInstruction)
// public let flightInstruction: ref<FlightInstance>;

// @wrapMethod(HUDInstruction)
// public final static func Construct(self: ref<HUDInstruction>, id: EntityID) -> Void {
//   wrappedMethod(self, id);
//   self.flightInstruction = new FlightInstruction();
//   ModuleInstance.Construct(self.flightInstruction, id);
// }

// HUDManager

// @addField(HUDManager)
// private let m_flightModule: ref<FlightModule>;

// @wrapMethod(HUDManager)
// private final func InitializeModules() -> Void {
//   wrappedMethod();
//   this.m_flightModule.InitializeModule(this, ModuleState.DISABLED);
//   ArrayPush(this.m_modulesArray, this.m_flightModule);
// }

// // Custom classes

// public class FlightModule extends HUDModule {

//   private let m_flightController: ref<FlightModuleInstance>;
// }

// public class FlightModuleInstance extends ModuleInstance {

// }

// public class FLIGHT_Actor extends HUDActor {

// }
