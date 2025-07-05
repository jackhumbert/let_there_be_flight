public func CreateEmptyThruster() -> ref<MeshComponent> {
  let mc = new MeshComponent();
  return mc;
}

public func CreateCorpoThruster(opt appearanceName: CName) -> ref<MeshComponent> {
  let mc = new PhysicalMeshComponent();
  mc.SetMesh(r"user\\jackhumbert\\meshes\\engine_corpo.mesh");
  if Equals(appearanceName, n"None") {
    mc.meshApperance = n"default";
  } else {
    mc.meshApperance = appearanceName;
  }
  mc.motionBlurScale = 0.1;
  mc.LODMode = entMeshComponentLODMode.Appearance;
  return mc;
}

public func CreateNomadThruster(opt appearanceName: CName) -> ref<MeshComponent> {
  let mc = new PhysicalMeshComponent();
  mc.SetMesh(r"user\\jackhumbert\\meshes\\engine_nomad.mesh");
  if Equals(appearanceName, n"None") {
    mc.meshApperance = n"default";
  } else {
    mc.meshApperance = appearanceName;
  }
  mc.motionBlurScale = 0.1;
  mc.LODMode = entMeshComponentLODMode.Appearance;
  return mc;
}