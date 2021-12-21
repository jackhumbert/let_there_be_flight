// -----------------------------------------------------------------------------
// Codeware.Scheduling.ControllerDelayCallback
// -----------------------------------------------------------------------------

module Codeware.Scheduling

public class ControllerDelayCallback extends DelayCallback {
	public let controller: wref<IScriptable>;

	public let event: ref<Event>;

	public func Call() -> Void {
		if IsDefined(this.controller) {
			// inkGameController
			if this.controller.IsA(n"gameuiWidgetGameController") {
				(this.controller as inkGameController).QueueEvent(this.event);
				return;
			}

			// inkLogicController
			if this.controller.IsA(n"inkWidgetLogicController") {
				(this.controller as inkLogicController).QueueEvent(this.event);
				return;
			}
		}
	}
}
