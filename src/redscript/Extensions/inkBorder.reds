@if(!ModuleExists("Codeware"))
@addField(inkBorder)
public native let thickness: Float;

@addMethod(inkBorder)
public func GetThickness() -> Float{
    return this.thickness;
}
