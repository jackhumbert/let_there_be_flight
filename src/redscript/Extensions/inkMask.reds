@if(!ModuleExists("Codeware"))
enum inkMaskDataSource {
    TextureAtlas = 0,
    DynamicTexture = 1
}

// public native class inkTextureAtlas {

// }

// @addField(inkMask)
// native let textureAtlas: inkTextureAtlas;
// @addField(inkMask)
// native let texturePart: CName;
@if(!ModuleExists("Codeware"))
@addField(inkMask)
public native let dynamicTextureMask: CName;

@addMethod(inkMask)
func SetDynamicTextureMask(value: CName) {
    this.dynamicTextureMask = value;
}

@if(!ModuleExists("Codeware"))
@addField(inkMask)
public native let dataSource: inkMaskDataSource;

@if(!ModuleExists("Codeware"))
@addMethod(inkMask)
func SetDataSource(value: inkMaskDataSource) {
    this.dataSource = value;
}

@addField(inkMask)
let useNineSliceScale: Bool;

@addField(inkMask)
let nineSliceScale: inkMargin;

@addMethod(inkMask)
public func UsesNineSliceScale() -> Bool {
	return this.useNineSliceScale;
}

@addMethod(inkMask)
public func SetNineSliceScale(enable: Bool) -> Void {
	this.useNineSliceScale = enable;
}

@addMethod(inkMask)
public func GetNineSliceGrid() -> inkMargin {
	return this.nineSliceScale;
}

@addMethod(inkMask)
public func SetNineSliceGrid(grid: inkMargin) -> Void {
	this.nineSliceScale = grid;
}

@if(!ModuleExists("Codeware"))
@addField(inkMask)
public native let invertMask: Bool;

@addMethod(inkMask)
func SetInvertMask(value: Bool) {
    this.invertMask = value;
}

@if(!ModuleExists("Codeware"))
@addField(inkMask)
public native let maskTransparency: Float;

@addMethod(inkMask)
func SetMaskTransparency(value: Float) {
    this.maskTransparency = value;
}


// @addMethod(inkMask)
// func SetAtlasResource(textureAtlas: ResRef) {
//     this.textureAtlas = textureAtlas;
// }
@addMethod(inkMask)
public native func SetAtlasResource(atlasResourcePath: ResRef) -> Bool;
