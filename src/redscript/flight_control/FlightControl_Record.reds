public class FlightControl_Record {
  public let mass: Float;
}

@addMethod(TweakDBInterface)
public final static func GetFlightRecord(path: TweakDBID) -> ref<FlightControl_Record> {
  let record = new FlightControl_Record();
  let mass_path = path;
  TDBID.Prepend(mass_path, t"FlightControl.");
  TDBID.Append(mass_path, t".mass");
  record.mass = TweakDBInterface.GetFloat(mass_path, 0.0);
  return record;
}