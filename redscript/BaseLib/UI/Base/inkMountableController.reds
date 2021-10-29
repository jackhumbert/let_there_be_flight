// -----------------------------------------------------------------------------
// inkMountableController
// -----------------------------------------------------------------------------
//
// public abstract class inkMountableController extends inkCustomController {
//   public func Mount(parentWidget: ref<inkCompoundWidget>, opt gameController: wref<inkGameController>) -> Void
//   public func Mount(parentWidget: ref<inkCompoundWidget>, parentController: wref<inkCustomController>) -> Void
//   public func Mount(parentController: wref<inkCustomController>) -> Void
//   public func Mount(parentController: wref<inkLogicController>, opt gameController: wref<inkGameController>) -> Void
//   public func Mount(parentController: wref<inkGameController>) -> Void
//   public func Unmount() -> Void
// }
//

module BaseLib.UI

public abstract class inkMountableController extends inkCustomController {
	public func Mount(parentWidget: ref<inkCompoundWidget>, opt gameController: ref<inkGameController>) -> Void {
		if !this.IsInitialized() {
			this.SetGameController(gameController);

			this.CreateInstance();

			if !IsDefined(this.GetRootWidget()) {
				LogError("[inkMountableController] Root widget is not initilized.");
				return;
			}

			this.Reparent(parentWidget);
			this.InitializeInstance();
		}
	}

	public func Mount(parentWidget: ref<inkCompoundWidget>, opt parentController: ref<inkCustomController>) -> Void {
		this.Mount(parentWidget, parentController.GetGameController());
	}

	public func Mount(parentController: ref<inkCustomController>) -> Void {
		this.Mount(parentController.GetContainerWidget(), parentController.GetGameController());
	}

	public func Mount(parentController: ref<inkLogicController>, opt gameController: ref<inkGameController>) -> Void {
		this.Mount(parentController.GetRootCompoundWidget(), gameController);
	}

	public func Mount(parentController: ref<inkGameController>) -> Void {
		this.Mount(parentController.GetRootCompoundWidget(), parentController);
	}

	public func Unmount() -> Void {
		// TODO: Remove from parent
	}
}
