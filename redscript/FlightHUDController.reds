// import BaseLib.UI.*

// public class FlightHUDController extends inkCustomController {
// 	protected let m_root: wref<inkFlex>;

// 	protected let m_log: wref<inkVerticalPanel>;

// 	protected cb func OnCreate() -> Void {
// 		let root: ref<inkFlex> = new inkFlex();
// 		root.SetName(n"flightControl");
// 		root.SetAnchor(inkEAnchor.LeftFillVerticaly);
// 		root.SetMargin(new inkMargin(16.0, 12.0, 0.0, 12.0));
// 		root.SetChildOrder(inkEChildOrder.Backward);

// 		let log: ref<inkVerticalPanel> = new inkVerticalPanel();
// 		log.SetName(n"log");
// 		log.SetAnchor(inkEAnchor.Fill);
// 		log.SetChildOrder(inkEChildOrder.Backward);
// 		log.SetOpacity(0.6);
// 		log.Reparent(root);

// 		this.m_root = root;
// 		this.m_log = log;

// 		this.SetRootWidget(root);
// 	}

// 	public func AddEntry(text: String) -> Void {
// 		let entry: ref<inkText> = new inkText();
// 		entry.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
// 		entry.SetFontStyle(n"Regular");
// 		entry.SetFontSize(24);
// 		entry.SetTintColor(ThemeColors.Bittersweet());
// 		entry.SetText(text);
// 		entry.Reparent(this.m_log);
 
// 		this.FadeInEntry(entry);
// 		this.TrimEntries();

// 		LogChannel(n"DEBUG", "[InkPlayground] " + text);
// 	}

// 	protected func GetMaxEntries() -> Int32 = 7

// 	protected func TrimEntries() -> Void {
// 		if this.m_log.GetNumChildren() > this.GetMaxEntries() {
// 			this.FadeOutEntry(this.m_log.GetWidgetByIndex(this.m_log.GetNumChildren() - this.GetMaxEntries() - 1));
// 		}
// 	}

// 	protected func FadeInEntry(entry: ref<inkWidget>) -> Void {
// 		let marginAnim: ref<inkAnimMargin> = new inkAnimMargin();
// 		marginAnim.SetStartMargin(new inkMargin(40.0, 0.0, 0.0, 0.0));
// 		marginAnim.SetEndMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));
// 		marginAnim.SetMode(inkanimInterpolationMode.EasyOut);
// 		marginAnim.SetDuration(0.25);

// 		let alphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
// 		alphaAnim.SetStartTransparency(0.0);
// 		alphaAnim.SetEndTransparency(1.0);
//     	alphaAnim.SetMode(inkanimInterpolationMode.EasyIn);
// 		alphaAnim.SetDuration(0.5);

// 		let animDef: ref<inkAnimDef> = new inkAnimDef();
// 		animDef.AddInterpolator(marginAnim);
// 		animDef.AddInterpolator(alphaAnim);

// 		entry.PlayAnimation(animDef);
// 	}

// 	protected func FadeOutEntry(entry: ref<inkWidget>) -> Void {
// 		let alphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
// 		alphaAnim.SetStartTransparency(1.0);
// 		alphaAnim.SetEndTransparency(0.0);
// 		alphaAnim.SetMode(inkanimInterpolationMode.EasyOut);
// 		alphaAnim.SetDuration(0.25);

// 		let animDef: ref<inkAnimDef> = new inkAnimDef();
// 		animDef.AddInterpolator(alphaAnim);

// 		let animProxy: ref<inkAnimProxy> = entry.PlayAnimation(animDef);
// 		animProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnFadeOutEnd");
// 	}

// 	protected cb func OnFadeOutEnd(animProxy: ref<inkAnimProxy>) -> Bool {
// 		this.m_log.RemoveChildByIndex(0);
// 	}

// 	public static func Create() -> ref<FlightHUDController> {
// 		let instance: ref<FlightHUDController> = new FlightHUDController();
// 		instance.Build();

// 		return instance;
// 	}
// }

@addField(gameuiCrosshairContainerController)
private let m_flightControlActiveCallbackId: ref<CallbackHandle>;

@addField(gameuiCrosshairContainerController)
private let m_isFlying: Bool;

@addField(gameuiCrosshairContainerController)
public let flightControlUI: ref<FlightControlUI>;

@wrapMethod(gameuiCrosshairContainerController)
protected cb func OnInitialize() -> Bool {
    wrappedMethod();
    this.m_isFlying = false;
    this.flightControlUI = FlightControlUI.Create(this, this.m_rootWidget);
}

@wrapMethod(gameuiCrosshairContainerController)
protected cb func OnUninitialize() -> Bool {
    wrappedMethod();
}

@wrapMethod(gameuiCrosshairContainerController)
protected cb func OnMountChanged(mounted: Bool) -> Bool {
    wrappedMethod(mounted);
    if IsDefined(this.m_Player) && this.m_Player.IsControlledByLocalPeer() {
        if mounted {
            this.m_flightControlActiveCallbackId = FlightControl.GetInstance().GetBlackboard().RegisterListenerBool(GetAllBlackboardDefs().FlightControl.ShouldShowUI, this, n"OnFlightActiveChanged");
        } else {
            FlightControl.GetInstance().GetBlackboard().UnregisterListenerBool(GetAllBlackboardDefs().FlightControl.ShouldShowUI, this.m_flightControlActiveCallbackId);
        }
    }
}

@replaceMethod(gameuiCrosshairContainerController)
private final func UpdateRootVisibility() -> Void {
    this.GetRootWidget().SetVisible(!this.m_isUnarmed || !this.m_isMounted || this.m_isFlying);
}

@addMethod(gameuiCrosshairContainerController)
protected cb func OnFlightActiveChanged(active: Bool) -> Bool {
    this.m_isFlying = active;
    this.UpdateRootVisibility();
}

// @addMethod(gameuiCrosshairContainerController)
// protected cb func OnFlightControlUpdate(elevation: Float) -> Bool {
//     let fc = FlightControl.GetInstance().stats;
//     let position = GameInstance.GetCameraSystem(FlightControl.GetInstance().gameInstance).ProjectPoint(fc.d_position + fc.d_forward);
//     this.flightControlStatus.SetTranslation(position.X * -3840.0, position.Y * -2160.0);
//     this.flightControlStatus.SetText("Elevation: " + FloatToStringPrec(elevation, 0) + "m");
// }



@replaceMethod(VehicleTransition)
protected final func PauseStateMachines(stateContext: ref<StateContext>, executionOwner: ref<GameObject>) -> Void {
    let upperBody: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    let equipmentRightHand: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    let equipmentLeftHand: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    let coverAction: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    let stamina: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    let aimAssistContext: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    let cameraContext: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    if stateContext.IsStateActive(n"UpperBody", n"forceEmptyHands") {
        upperBody.stateMachineIdentifier.definitionName = n"UpperBody";
        executionOwner.QueueEvent(upperBody);
    };
    equipmentRightHand.stateMachineIdentifier.referenceName = n"RightHand";
    equipmentRightHand.stateMachineIdentifier.definitionName = n"Equipment";
    executionOwner.QueueEvent(equipmentRightHand);
    equipmentLeftHand.stateMachineIdentifier.referenceName = n"LeftHand";
    equipmentLeftHand.stateMachineIdentifier.definitionName = n"Equipment";
    executionOwner.QueueEvent(equipmentLeftHand);
    coverAction.stateMachineIdentifier.definitionName = n"CoverAction";
    executionOwner.QueueEvent(coverAction);
    if DefaultTransition.GetBlackboardIntVariable(executionOwner, GetAllBlackboardDefs().PlayerStateMachine.Stamina) == EnumInt(gamePSMStamina.Rested) {
        stamina.stateMachineIdentifier.definitionName = n"Stamina";
        executionOwner.QueueEvent(stamina);
    };
    aimAssistContext.stateMachineIdentifier.definitionName = n"AimAssistContext";
    executionOwner.QueueEvent(aimAssistContext);
    cameraContext.stateMachineIdentifier.definitionName = n"CameraContext";
    executionOwner.QueueEvent(cameraContext);
}

@replaceMethod(VehicleTransition)
protected final func ResumeStateMachines(executionOwner: ref<GameObject>) -> Void {
    let upperBody: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    let equipmentRightHand: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    let equipmentLeftHand: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    let coverAction: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    let stamina: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    let aimAssistContext: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    let locomotion: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    let cameraContext: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    upperBody.stateMachineIdentifier.definitionName = n"UpperBody";
    executionOwner.QueueEvent(upperBody);
    equipmentRightHand.stateMachineIdentifier.referenceName = n"RightHand";
    equipmentRightHand.stateMachineIdentifier.definitionName = n"Equipment";
    executionOwner.QueueEvent(equipmentRightHand);
    equipmentLeftHand.stateMachineIdentifier.referenceName = n"LeftHand";
    equipmentLeftHand.stateMachineIdentifier.definitionName = n"Equipment";
    executionOwner.QueueEvent(equipmentLeftHand);
    coverAction.stateMachineIdentifier.definitionName = n"CoverAction";
    executionOwner.QueueEvent(coverAction);
    stamina.stateMachineIdentifier.definitionName = n"Stamina";
    executionOwner.QueueEvent(stamina);
    aimAssistContext.stateMachineIdentifier.definitionName = n"AimAssistContext";
    executionOwner.QueueEvent(aimAssistContext);
    locomotion.stateMachineIdentifier.definitionName = n"Locomotion";
    executionOwner.QueueEvent(locomotion);
    cameraContext.stateMachineIdentifier.definitionName = n"CameraContext";
    executionOwner.QueueEvent(cameraContext);
}