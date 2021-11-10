public native class inkQuadShape extends inkBaseShapeWidget {
    // native let textureAtlas: ResRef;
    native let texturePart: CName;
    native let vertexList: array<Vector2>;

    // public func GetTextureAtlas() -> ResRef {
    //     return this.textureAtlas;
    // }
    public func GetTexturePart() -> CName {
        return this.texturePart;
    }
    public func GetVertexList() -> array<Vector2> {
        return this.vertexList;
    }
}