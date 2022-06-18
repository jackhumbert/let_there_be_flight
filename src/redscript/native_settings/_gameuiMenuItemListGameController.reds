@wrapMethod(gameuiMenuItemListGameController)
protected final func AddMenuItem(label: String, spawnEvent: CName) -> Void {
    if Equals(spawnEvent, n"OnSwitchToDlc") {
        this.AddMenuItem("Mods", n"OnSwitchToSettings");
    }
    wrappedMethod(label, spawnEvent);
}

@wrapMethod(gameuiMenuItemListGameController)
protected cb func OnMenuItemActivated(index: Int32, target: ref<ListItemController>) -> Bool {
    let data = target.GetData() as ListItemData;
    if IsDefined(data) {
        NativeSettings.GetInstance().fromMods = Equals(data.label, "Mods");
    }
    return wrappedMethod(index, target);
}