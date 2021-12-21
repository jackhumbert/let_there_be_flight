// -----------------------------------------------------------------------------
// inkCircle
// -----------------------------------------------------------------------------
//
// - Number of segments
//
// -----------------------------------------------------------------------------
//
// class inkCircle extends inkLeafWidget {
//   public func GetSegmentsNumber() -> Uint32
//   public func SetSegmentsNumber(segmentsNumber: Uint32) -> Void
// }
//

@addField(inkCircle)
native let segmentsNumber: Uint32;

@addMethod(inkCircle)
public func GetSegmentsNumber() -> Uint32 {
	return this.segmentsNumber;
}

@addMethod(inkCircle)
public func SetSegmentsNumber(segmentsNumber: Uint32) -> Void {
	this.segmentsNumber = segmentsNumber;
}
