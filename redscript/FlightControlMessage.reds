// public class FlightControlMessage extends OnscreenMessageGameController {

//   protected cb func OnInitialize() -> Bool {
//     let variant: Variant;
//     this.m_root = this.GetRootWidget();
//     this.m_root.SetVisible(false);
//     this.m_blackboardDef = GetAllBlackboardDefs().UI_Notifications;
//     this.m_blackboard = this.GetBlackboardSystem().Get(this.m_blackboardDef);
//     this.m_screenMessageUpdateCallbackId = this.m_blackboard.RegisterDelayedListenerVariant(this.m_blackboardDef.FlightControlMessage, this, n"FlightControlMessageUpdate");
//     variant = this.m_blackboard.GetVariant(this.m_blackboardDef.FlightControlMessage);
//     // if VariantIsValid(variant) {
//     this.m_screenMessage = FromVariant(variant);
//     // };
//     this.CreateAnimations();
//   }  
  
//   protected cb func OnUnitialize() -> Bool {
//     this.m_blackboard.UnregisterDelayedListener(this.m_blackboardDef.FlightControlMessage, this.m_screenMessageUpdateCallbackId);
//   }

//   protected cb func FlightControlMessageUpdate(value: Variant) -> Bool {
//     this.m_screenMessage = FromVariant(value);
//     this.UpdateWidgets();
//   }

//   private final func UpdateWidgets() -> Void {
//     this.m_root.StopAllAnimations();
//     if this.m_screenMessage.isShown {
//       inkTextRef.SetLetterCase(this.m_mainTextWidget, textLetterCase.UpperCase);
//       inkTextRef.SetText(this.m_mainTextWidget, this.m_screenMessage.message);
//       this.m_root.SetVisible(true);
//       this.m_animProxyShow = this.PlayLibraryAnimation(n"CInematic_Subtitle");
//     } else {
//       this.m_root.SetVisible(false);
//     };
//   }

//   protected cb func OnTimeout(anim: ref<inkAnimProxy>) -> Bool {
//     if anim.IsFinished() {
//       this.m_blackboard.SetVariant(this.m_blackboardDef.FlightControlMessage, ToVariant(NoScreenMessage()));
//     };
//   }

// }

// @addField(UI_NotificationsDef)
// public let FlightControlMessage: BlackboardID_Variant;