public class FlightNavPath {
  let controller: ref<FlightController>;

  public let mmcc: ref<MinimapContainerController>;
  let spacing: Float;
  let distanceToPath: Float;
  let closestPoint: Float;
  let interpolationDistance: Float;
  
  let navPathQuestFX: array<ref<FxInstance>>;
  let navPathPlayerFX: array<ref<FxInstance>>;
  let navPathYellowResource: FxResource;
  let navPathBlueResource: FxResource;
  let navPathWhiteResource: FxResource;
  let navPathTealResource: FxResource;
  let FXPoints: Int32;

  public static func Create(controller: ref<FlightController>) -> ref<FlightNavPath> {
    let self = new FlightNavPath();
    self.controller = controller;
    self.spacing = 2.5; // meters
    self.distanceToPath = 50.0; // meters
    self.closestPoint = 5.0; // meters
    self.interpolationDistance = 50.0; // meters
    self.FXPoints = 100;
    self.navPathYellowResource = Cast<FxResource>(r"user\\jackhumbert\\effects\\world_navigation_yellow.effect");
    self.navPathBlueResource = Cast<FxResource>(r"user\\jackhumbert\\effects\\world_navigation_blue.effect");
    self.navPathWhiteResource = Cast<FxResource>(r"user\\jackhumbert\\effects\\world_navigation_white.effect");
    self.navPathTealResource = Cast<FxResource>(r"user\\jackhumbert\\effects\\world_navigation_teal.effect");
    return self;
  }

  public func Update() {
    if IsDefined(this.mmcc) {
      this.UpdateNavPathPoints(this.navPathQuestFX, this.mmcc.questPoints, this.navPathYellowResource);
      this.UpdateNavPathPoints(this.navPathPlayerFX, this.mmcc.playerPoints, this.navPathBlueResource);
    }
  }

  public func Stop() {
    for fx in this.navPathQuestFX {
      fx.BreakLoop();
    }
    for fx in this.navPathPlayerFX {
      fx.BreakLoop();
    }
  }

  private func UpdateNavPathPoints(out fxs:array<ref<FxInstance>>, points: array<Vector4>, resource: FxResource) -> Void {
    // let lastPoint: Vector4 = new Vector4(0.0, 0.0, 0.0, 0.0);
    let lastPoint: Vector4 = this.controller.player.GetWorldPosition();
    let lastFxPoint: Vector4 = this.controller.player.GetWorldPosition();
    let pointsDrawn = 0;

    // need to extrapolate the path from closest two points to the current position to get rid of the "walk to road" segment

    let outsidePoints: array<Vector4>;
    for point in points {
      let adjustedPoint = this.AdjustPointToDirection(point, this.distanceToPath);
      let distance = Vector4.Distance2D(adjustedPoint, this.controller.player.GetWorldPosition());
      if distance > this.interpolationDistance && point.Z != 100.00 && point.Z != 125.00 {
        ArrayPush(outsidePoints, point);
      } else { 
        if ArraySize(outsidePoints) < 20 {
          ArrayClear(outsidePoints);
        }
      }
      if distance > 1000.0 && ArraySize(outsidePoints) > 20 {
        break;
      }
    }

    if ArraySize(outsidePoints) > 5 {
      let point = outsidePoints[0];
      let inc = 1.0;
      let distance = 10000.0;
      let lastDistance = 10001.0;
      while distance > this.interpolationDistance && lastDistance > distance {
        point = Vector4.Normalize(outsidePoints[0] - outsidePoints[4]) * inc + outsidePoints[0];
        inc += 1.0;
        lastDistance = distance;
        distance = Vector4.Distance2D(point, this.controller.player.GetWorldPosition());
      }
      if inc > 0.0 {
        ArrayInsert(outsidePoints, 0, point);
      }
      inc = this.spacing * 2.0;
      let a = outsidePoints[0];
      let b = outsidePoints[4];
      let pathDirection = Vector4.Normalize(a - b);
      pathDirection.Z = 0.0;
      let playerDirection = Vector4.Normalize2D(this.controller.player.GetWorldForward());
      playerDirection.Z = 0.0;
      while inc < this.interpolationDistance {
        point = Vector4.Interpolate(pathDirection * inc + a, playerDirection * (this.interpolationDistance - inc) + this.controller.player.GetWorldPosition(), inc / this.interpolationDistance);
        inc += this.spacing * 2.0;
        ArrayInsert(outsidePoints, 0, point);
      }
    }

    for point in outsidePoints {
      let tweenPointDistance = Vector4.Distance(point, lastPoint);
      // let correctedPoint = this.AdjustPointToDirection(point, this.distanceToPath);
      if (tweenPointDistance > this.spacing) {
        // if Vector4.Distance(correctedPoint, this.controller.player.GetWorldPosition()) > this.closestPoint {
        //   this.UpdateNavPathFX(fxs, pointsDrawn, correctedPoint, Quaternion.BuildFromDirectionVector(correctedPoint - lastFxPoint));
        //   pointsDrawn += 1;
        //   if (pointsDrawn >= this.FXPoints)
        //   {
        //     break;
        //   }
        // }
        let rounded = Cast<Float>(RoundF(tweenPointDistance / this.spacing));
        let tweenPointSpacing = this.spacing + (tweenPointDistance - rounded * this.spacing) / rounded;
        let x = tweenPointSpacing;
        while (x <= tweenPointDistance) {
          let midPoint = point / tweenPointDistance * x + lastPoint / tweenPointDistance * (tweenPointDistance - x);      
          let correctedMidPoint = this.AdjustPointToDirectionRaycast(midPoint, this.distanceToPath);
          if Vector4.Distance(correctedMidPoint, this.controller.player.GetWorldPosition()) > this.closestPoint {
            this.UpdateNavPathFX(fxs, pointsDrawn, correctedMidPoint, Quaternion.BuildFromDirectionVector(correctedMidPoint - lastFxPoint), resource);
            pointsDrawn += 1;
            if (pointsDrawn >= this.FXPoints)
            {
              break;
            }
          }
          lastFxPoint = correctedMidPoint;
          x += tweenPointSpacing;
        }
        if (pointsDrawn >= this.FXPoints)
        {
          break;
        }
        lastPoint = point;
      }
    }
    if pointsDrawn < this.FXPoints {
      while pointsDrawn < this.FXPoints {   
        this.UpdateNavPathFX(fxs, pointsDrawn, new Vector4(0.0, 0.0, -1000.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0), resource);
        pointsDrawn += 1;
      }
    }
  }

  private func UpdateNavPathFX(out fxs: array<ref<FxInstance>>, i: Int32, p: Vector4, q: Quaternion, resource: FxResource) {
    let wT0: WorldTransform;
    WorldTransform.SetPosition(wT0, p + q * new Vector4(-0.7464371, -2.5, 0.09832764, 0.0));
    WorldTransform.SetOrientation(wT0, q * new Quaternion(-0.5, 0.5, -0.5, 0.5));
    if ArraySize(fxs) < (i + 1) {
      ArrayPush(fxs, GameInstance.GetFxSystem(this.controller.gameInstance).SpawnEffect(resource, wT0));
    } else {
      if !fxs[i].IsValid() {
        fxs[i] = GameInstance.GetFxSystem(this.controller.gameInstance).SpawnEffect(resource, wT0);
      } else {
        fxs[i].UpdateTransform(wT0);
      }
    }
    fxs[i].SetBlackboardValue(n"alpha", MinF(Vector4.Distance2D(this.controller.player.GetWorldPosition(), p) / this.distanceToPath, 1.0) / 50.0);
  }

  private func AdjustPointToDirection(point: Vector4, threshold: Float) -> Vector4 {
    let distance2D = Vector4.Distance2D(point, this.controller.player.GetWorldPosition());
    if (distance2D < threshold) {
      let factor = PowF(1.0 - distance2D / threshold, 2.0);
      // point.Z = point.Z * (1.0 - factor) + this.controller.player.GetWorldPosition().Z * factor;
      // let pointAlongDirection = Vector4.NearestPointOnEdge(point, this.controller.player.GetWorldPosition() + (this.controller.stats.d_forward * -threshold) + this.controller.stats.d_velocity, this.controller.player.GetWorldPosition() + (this.controller.stats.d_forward * threshold) + this.controller.stats.d_velocity);
      let speed = Vector4.Length2D(this.controller.player.GetVelocity());
      let speedFactor = speed / 200.0 + MinF(speed / 20.0, 0.5);
      let offset = Vector4.Interpolate(Vector4.Normalize2D(this.controller.player.GetWorldForward()), Vector4.Normalize2D(this.controller.player.GetVelocity()), speedFactor);
      offset.Z = 0.0;
      let pointAlongDirection = Vector4.NearestPointOnEdge(point, this.controller.player.GetWorldPosition() + offset * -threshold, this.controller.player.GetWorldPosition() + offset * threshold);
      point.X = point.X * (1.0 - factor) + pointAlongDirection.X * factor;
      point.Y = point.Y * (1.0 - factor) + pointAlongDirection.Y * factor;
      point.Z = point.Z * (1.0 - factor) + pointAlongDirection.Z * factor;
    }
    return point;
  }

  private func AdjustPointToDirectionRaycast(point: Vector4, threshold: Float) -> Vector4 {
    let ogPoint = point;
    let point = this.AdjustPointToDirection(point, threshold);
    let distance2D = Vector4.Distance2D(ogPoint, this.controller.player.GetWorldPosition());
    if (distance2D < threshold) {
      let findGround: TraceResult;
      GameInstance.GetSpatialQueriesSystem(this.controller.player.GetGame()).SyncRaycastByCollisionGroup(point, point + new Vector4(0.0, 0.0, 1.0, 0.0), n"VehicleBlocker", findGround, false, false);
      if !TraceResult.IsValid(findGround) { 
        GameInstance.GetSpatialQueriesSystem(this.controller.player.GetGame()).SyncRaycastByCollisionGroup(point, point + new Vector4(0.0, 0.0, 20.0, 0.0), n"VehicleBlocker", findGround, false, false);
        if TraceResult.IsValid(findGround) {
          point.Z = MaxF(findGround.position.Z + 0.01, point.Z);
        }
      }
    }
    return point;
  }
}