// -----------------------------------------------------------------------------
// inkShape
// -----------------------------------------------------------------------------
//
// - Constrain content position
// - Content fitting direction
//
// -----------------------------------------------------------------------------
//
// class inkShape extends inkBaseShapeWidget {
//   public final native func ChangeShape(shapeName: CName) -> Void;
// }
//

enum inkEShapeVariant {
    Fill = 0,
    Border = 1,
    FillAndBorder = 2,
}

enum inkEEndCapStyle {
    BUTT = 0, 
    SQUARE = 1, 
    ROUND = 2,
    JOINED = 3,
}

enum inkEJointStyle {
    MITER = 0,
    BEVEL = 1,
    ROUND = 2,
}

// @addField(inkShape)
// native let shapeResource: ResRef; ?? idk

@addField(inkShape)
native let shapeName:  CName;

@addField(inkShape)
native let shapeVariant:inkEShapeVariant;

@addField(inkShape)
native let keepInBounds: Bool;

@addField(inkShape)
native let nineSliceScale: inkMargin;

@addField(inkShape)
native let useNineSlice: Bool;

@addField(inkShape)
native let contentHAlign: inkEHorizontalAlign;

@addField(inkShape)
native let contentVAlign: inkEVerticalAlign;

@addField(inkShape)
native let borderColor: HDRColor;

@addField(inkShape)
native let borderOpacity: Float;

@addField(inkShape)
native let fillOpacity: Float;

@addField(inkShape)
native let lineThickness: Float;

@addField(inkShape)
native let endCapStyle: inkEEndCapStyle;

@addField(inkShape)
native let jointStyle: inkEJointStyle;

@addField(inkShape)
native let vertexList: array<Vector2>;

// Gets

@addMethod(inkShape)
public func GetShapeName() -> CName {
    return this.shapeName;
}

@addMethod(inkShape)
public func GetShapeVariant() -> inkEShapeVariant {
    return this.shapeVariant;
}

@addMethod(inkShape)
public func GetKeepInBounds() -> Bool {
    return this.keepInBounds;
}

@addMethod(inkShape)
public func GetNineSliceScale() -> inkMargin {
    return this.nineSliceScale;
}

@addMethod(inkShape)
public func GetUseNineSlice() -> Bool {
    return this.useNineSlice;
}

@addMethod(inkShape)
public func GetContentHAlign() -> inkEHorizontalAlign {
    return this.contentHAlign;
}

@addMethod(inkShape)
public func GetContentVAlign() -> inkEVerticalAlign {
    return this.contentVAlign;
}

@addMethod(inkShape)
public func GetBorderColor() -> HDRColor {
    return this.borderColor;
}

@addMethod(inkShape)
public func GetBorderOpacity() -> Float {
    return this.borderOpacity;
}

@addMethod(inkShape)
public func GetFillOpacity() -> Float {
    return this.fillOpacity;
}

@addMethod(inkShape)
public func GetLineThickness() -> Float {
    return this.lineThickness;
}

@addMethod(inkShape)
public func GetEndCapStyle() -> inkEEndCapStyle {
    return this.endCapStyle;
}

@addMethod(inkShape)
public func GetJointStyle() -> inkEJointStyle {
    return this.jointStyle;
}

@addMethod(inkShape)
public func GetVertexList() -> array<Vector2> {
    return this.vertexList;
}

// Sets

// could be implemented natively
// @addMethod(inkShape)
// public native func SetShapeResource(shapeResource: ResRef);

@addMethod(inkShape)
public func SetShapeName(shapeName: CName) {
    this.shapeName = shapeName;
}

@addMethod(inkShape)
public func SetShapeVariant(shapeVariant: inkEShapeVariant) {
    this.shapeVariant = shapeVariant;
}

@addMethod(inkShape)
public func SetKeepInBounds(keepInBounds: Bool) {
    this.keepInBounds = keepInBounds;
}

@addMethod(inkShape)
public func SetNineSliceScale(nineSliceScale: inkMargin) {
    this.nineSliceScale = nineSliceScale;
}

@addMethod(inkShape)
public func SetUseNineSlice(useNineSlice: Bool) {
    this.useNineSlice = useNineSlice;
}

@addMethod(inkShape)
public func SetContentHAlign(contentHAlign: inkEHorizontalAlign) {
    this.contentHAlign = contentHAlign;
}

@addMethod(inkShape)
public func SetContentVAlign(contentVAlign: inkEVerticalAlign) {
    this.contentVAlign = contentVAlign;
}

@addMethod(inkShape)
public func SetBorderColor(borderColor: HDRColor) {
    this.borderColor = borderColor;
}

@addMethod(inkShape)
public func SetBorderOpacity(borderOpacity: Float) {
    this.borderOpacity = borderOpacity;
}

@addMethod(inkShape)
public func SetFillOpacity(fillOpacity: Float) {
    this.fillOpacity = fillOpacity;
}

@addMethod(inkShape)
public func SetLineThickness(lineThickness: Float) {
    this.lineThickness = lineThickness;
}

@addMethod(inkShape)
public func SetEndCapStyle(endCapStyle: inkEEndCapStyle) {
    this.endCapStyle = endCapStyle;
}

@addMethod(inkShape)
public func SetJointStyle(jointStyle: inkEJointStyle) {
    this.jointStyle = jointStyle;
}

@addMethod(inkShape)
public func SetVertexList(vertexList: array<Vector2>) {
    this.vertexList = vertexList;
}