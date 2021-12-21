// -----------------------------------------------------------------------------
// inkScrollArea
// -----------------------------------------------------------------------------
//
// - Constrain content position
// - Content fitting direction
//
// -----------------------------------------------------------------------------
//
// class inkScrollArea extends inkLeafWidget {
//   public func GetFitToContentDirection() -> inkFitToContentDirection
//   public func SetFitToContentDirection(value: inkFitToContentDirection) -> Void
// }
//

@addField(inkScrollArea)
native let constrainContentPosition: Bool;

@addField(inkScrollArea)
native let fitToContentDirection: inkFitToContentDirection;

@addMethod(inkScrollArea)
public func GetConstrainContentPosition() -> Bool {
	return this.constrainContentPosition;
}

@addMethod(inkScrollArea)
public func SetConstrainContentPosition(enabled: Bool) -> Void {
	this.constrainContentPosition = enabled;
}

@addMethod(inkScrollArea)
public func GetFitToContentDirection() -> inkFitToContentDirection {
	return this.fitToContentDirection;
}

@addMethod(inkScrollArea)
public func SetFitToContentDirection(value: inkFitToContentDirection) -> Void {
	this.fitToContentDirection = value;
}
