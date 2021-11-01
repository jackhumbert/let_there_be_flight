public class physicsSystemBodyParams {
    public let simulationType: physicsSimulationType;
    public let linearDamping: Float;
    public let angularDamping: Float;
    public let solverIterationsCountPosition: Uint32;
    public let solverIterationsCountVelocity: Uint32;
    public let maxDepenetrationVelocity: Float;
    public let maxAngularVelocity: Float;
    public let maxContactImpulse: Float;
    public let mass: Float;
    public let inertia: Vector3;
    public let comOffset: Transform;
}