public class physicsSystemBody extends physicsISystemObject {
    public let params: physicsSystemBodyParams;
    public let localToModel: Transform;
    public let collisionShapes: array<physicsICollider>;
    public let mappedBoneName: CName;
    public let mappedBoneToBody: Transform;
    public let isQueryBodyOnly: Bool;
}