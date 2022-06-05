public class QuickHackFlightMalfunction extends ActionBool {

  public final func SetProperties() -> Void {
    this.actionName = n"QuickHackFlightMalfunction";
    this.prop = DeviceActionPropertyFunctions.SetUpProperty_Bool(n"QuickHackFlightMalfunction", true, n"LocKey#11337", n"LocKey#11337");
  }

  public final func SetProperties(interaction: CName) -> Void {
    this.actionName = interaction;
    this.prop = DeviceActionPropertyFunctions.SetUpProperty_Bool(interaction, true, interaction, interaction);
  }

  public const func GetInteractionIcon() -> wref<ChoiceCaptionIconPart_Record> {
    return TweakDBInterface.GetChoiceCaptionIconPartRecord(t"ChoiceCaptionParts.DistractIcon");
  }
}

public class VehicleFlightMalfunction extends ActionBool {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightMalfunction";
    this.prop = DeviceActionPropertyFunctions.SetUpProperty_Bool(this.actionName, true, n"UI-Settings-Group-Flight", n"UI-Settings-Group-Flight");
  }
}