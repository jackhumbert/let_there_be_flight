@addMethod(inkWidget)
public native func CreateEffect(typeName: CName, effectName: CName) -> Void;

enum inkEBlurDimension
{
   Horizontal = 0,
   Vertical = 1
}

@addMethod(inkWidget)
public native func SetBlurDimension(effectName: CName, blurDimension : inkEBlurDimension) -> Bool;
