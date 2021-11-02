public class physicsSystemJoint extends physicsISystemObject {
    public let localToWorld: Matrix;
    // public let pinA: handle:physicsPhysicalJointPin;
    // public let pinB: handle:physicsPhysicalJointPin;
    // public let linearLimit: physicsPhysicsJointLinearLimit;
    // public let twistLimit: physicsPhysicsJointAngularLimitPair;
    // public let swingLimit: physicsPhysicsJointLimitConePair;
    // public let driveY: physicsPhysicsJointDrive;
    // public let driveX: physicsPhysicsJointDrive;
    // public let driveZ: physicsPhysicsJointDrive;
    // public let driveTwist: physicsPhysicsJointDrive;
    // public let driveSwing: physicsPhysicsJointDrive;
    // public let driveSLERP: physicsPhysicsJointDrive;
    // public let driveVelocity: physicsPhysicsJointDriveVelocity;
    public let drivePosition: Matrix;
    public let projectionEnabled: Bool;
    public let linearTolerance: Float;
    public let angularTolerance: Float;
    public let isBreakable: Bool;
    public let breakingForce: Float;
    public let breakingTorque: Float;
}