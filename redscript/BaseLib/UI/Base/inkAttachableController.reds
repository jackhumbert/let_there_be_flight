// -----------------------------------------------------------------------------
// inkAttachableController
// -----------------------------------------------------------------------------
//
// public abstract class inkAttachableController extends inkCustomController {
//   public func Attach(rootWidget: ref<inkCompoundWidget>, opt gameController: wref<inkGameController>) -> Void
//   public func Detach() -> Void
// }
//

module BaseLib.UI

public abstract class inkAttachableController extends inkCustomController {
	public func Attach(rootWidget: ref<inkCompoundWidget>, opt gameController: wref<inkGameController>) -> Void {
		if !this.IsInitialized() {
			this.SetRootWidget(rootWidget);
			this.SetGameController(gameController);

			this.CreateInstance();
			this.InitializeInstance();
		}
	}

	public func Detach() -> Void {
		if this.IsInitialized() {
			this.UninitializeInstance();

			this.SetRootWidget(null);
			this.SetGameController(null);
		}
	}
}
