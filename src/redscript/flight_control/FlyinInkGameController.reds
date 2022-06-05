// @wrapMethod(RadioInkGameController)
// public func Refresh(state: EDeviceStatus) -> Void {
//   FlightLog.Info("[RadioInkGameController] Refresh");
//   wrappedMethod(state);
// }

// public class FlyinInkGameController extends DeviceInkGameControllerBase {

//   @runtimeProperty("category", "Widget Refs")
//   protected edit browsable let m_stationNameWidget: inkTextRef;

//   @runtimeProperty("category", "Widget Refs")
//   protected edit browsable let m_stationLogoWidget: inkImageRef;

//   public func Refresh(state: EDeviceStatus) -> Void {
//     FlightLog.Info("[FlyinInkGameController] Refresh");
//     this.SetupWidgets();
//     switch state {
//       case EDeviceStatus.ON:
//         this.TurnOn();
//         break;
//       case EDeviceStatus.OFF:
//         this.TurnOff();
//         break;
//       case EDeviceStatus.UNPOWERED:
//         break;
//       case EDeviceStatus.DISABLED:
//         break;
//       default:
//     };
//     this.Refresh(state);
//   }

//   protected func GetOwner() -> ref<VehicleObject> {
//     return this.GetOwnerEntity() as VehicleObject;
//   }

//   private final func TurnOff() -> Void {
//     inkWidgetRef.SetVisible(this.m_stationNameWidget, false);
//     this.m_rootWidget.SetState(n"Off");
//     this.TriggerAnimationByName(n"eqLoop2", EInkAnimationPlaybackOption.GO_TO_START);
//     this.TriggerAnimationByName(n"eqLoop3", EInkAnimationPlaybackOption.GO_TO_START);
//     this.TriggerAnimationByName(n"eqLoop5", EInkAnimationPlaybackOption.GO_TO_START);
//     this.TriggerAnimationByName(n"eqLoop7", EInkAnimationPlaybackOption.GO_TO_START);
//     inkImageRef.SetTexturePart(this.m_stationLogoWidget, n"no_station");
//   }

//   private final func TurnOn() -> Void {
//     if NotEquals(this.m_cashedState, EDeviceStatus.ON) {
//       this.m_rootWidget.SetState(n"Default");
//       this.TriggerAnimationByName(n"eqLoop2", EInkAnimationPlaybackOption.PLAY);
//       this.TriggerAnimationByName(n"eqLoop3", EInkAnimationPlaybackOption.PLAY);
//       this.TriggerAnimationByName(n"eqLoop5", EInkAnimationPlaybackOption.PLAY);
//       this.TriggerAnimationByName(n"eqLoop7", EInkAnimationPlaybackOption.PLAY);
//       inkWidgetRef.SetVisible(this.m_stationNameWidget, true);
//     };
//     inkTextRef.SetLocalizedTextScript(this.m_stationNameWidget, this.GetOwner().GetDisplayName());
//     this.SetupStationLogo();
//   }

//   private final func SetupStationLogo() -> Void {
//     let texturePart = n"no_station";
//     inkImageRef.SetTexturePart(this.m_stationLogoWidget, texturePart);
//   }
// }
