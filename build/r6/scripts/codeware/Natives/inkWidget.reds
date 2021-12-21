// -----------------------------------------------------------------------------
// inkWidget
// -----------------------------------------------------------------------------
//
// - Parent widget
// - Primary logic controller
// - Secondary logic controllers
// - Custom user data
//
// -----------------------------------------------------------------------------
//
// class inkWidget {
//   public func GetParentWidget() -> wref<inkWidget>
//   public func SetController(controller: ref<inkLogicController>) -> Void
//   public func AddSecondaryController(controller: ref<inkLogicController>) -> Void
//   public func SetUserData(userData: ref<inkUserData>) -> Void
// }
//

@addField(inkWidget)
native let parentWidget: wref<inkWidget>;

@addField(inkWidget)
native let logicController: ref<inkLogicController>;

@addField(inkWidget)
native let secondaryControllers: array<ref<inkLogicController>>;

@addField(inkWidget)
native let userData: array<ref<inkUserData>>;

@addField(inkWidget)
native let canSupportFocus: Bool;

@addMethod(inkWidget)
public func GetParentWidget() -> wref<inkWidget> {
	return this.parentWidget;
}

@addMethod(inkWidget)
public func SetController(controller: ref<inkLogicController>) -> Void {
	this.logicController = controller;
}

@addMethod(inkWidget)
public func AddSecondaryController(controller: ref<inkLogicController>) -> Void {
	ArrayPush(this.secondaryControllers, controller);
}

@addMethod(inkWidget)
public func SetUserData(userData: ref<inkUserData>) -> Void {
	ArrayPush(this.userData, userData);
}

@addMethod(inkWidget)
public func CanSupportFocus() -> Bool {
	return this.canSupportFocus;
}

@addMethod(inkWidget)
public func SetSupportFocus(enabled: Bool) -> Void {
	this.canSupportFocus = enabled;
}
