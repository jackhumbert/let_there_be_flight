@addField(inkBorder)
native let thickness: Float;

@addMethod(inkBorder)
public func SetThickness(thickness: Float) {
    this.thickness = thickness;
}
@addMethod(inkBorder)
public func GetThickness() -> Float{
    return this.thickness;
}