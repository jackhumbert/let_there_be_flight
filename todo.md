? screen effect at high speed
? screen effect at high g
* disable passenger exit until not in flight
  * special convos?

2025
* disable crosshair in UI when using mounted weapons
* move camera center to crosshair during driver combat
* animate flight camera moves
* use input contexts for flight options?
* move input hints to inputContext classes
* prevent thrusters from moving during photo mode
* parachute
* summon flying vehicle

-15_-16_0_1 1450 - CriticalCollisionNode_017: Uk12 == 1
related to safe areas? how are those defined
-8_-8_0_2

for vehicle?
scriptInterface.GetTargetingSystem().SetIsMovingFast(scriptInterface.owner, true);

Audio/Ambience/Zones
audio::AmbientSoundSystem



  public final static func GetDistanceToGround(const scriptInterface: ref<StateGameScriptInterface>) -> Float {
    let distanceToGround: Float;
    let geometryDescription: ref<GeometryDescriptionQuery>;
    let geometryDescriptionResult: ref<GeometryDescriptionResult>;
    let queryFilter: QueryFilter;
    let currentPosition: Vector4 = DefaultTransition.GetPlayerPosition(scriptInterface);
    QueryFilter.AddGroup(queryFilter, n"Static");
    QueryFilter.AddGroup(queryFilter, n"Terrain");
    QueryFilter.AddGroup(queryFilter, n"PlayerBlocker");
    geometryDescription = new GeometryDescriptionQuery();
    geometryDescription.AddFlag(worldgeometryDescriptionQueryFlags.DistanceVector);
    geometryDescription.filter = queryFilter;
    geometryDescription.refPosition = currentPosition;
    geometryDescription.refDirection = new Vector4(0.00, 0.00, -1.00, 0.00);
    geometryDescription.primitiveDimension = new Vector4(0.50, 0.10, 0.10, 0.00);
    geometryDescription.maxDistance = 100.00;
    geometryDescription.maxExtent = 100.00;
    geometryDescription.probingPrecision = 10.00;
    geometryDescription.probingMaxDistanceDiff = 100.00;
    geometryDescriptionResult = scriptInterface.GetSpatialQueriesSystem().GetGeometryDescriptionSystem().QueryExtents(geometryDescription);
    if Equals(geometryDescriptionResult.queryStatus, worldgeometryDescriptionQueryStatus.NoGeometry) || NotEquals(geometryDescriptionResult.queryStatus, worldgeometryDescriptionQueryStatus.OK) {
      return -1.00;
    };
    distanceToGround = AbsF(geometryDescriptionResult.distanceVector.Z);
    return distanceToGround;
  }
