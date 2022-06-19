// @wrapMethod(PauseMenuGameController)
// protected cb func OnMenuItemActivated(index: Int32, target: ref<ListItemController>) -> Bool {
//     let data = target.GetData() as ListItemData;
//     if IsDefined(data) {
//         NativeSettings.GetInstance().fromMods = Equals(data.label, "Mods");
//     }
//     return wrappedMethod(index, target);
// }