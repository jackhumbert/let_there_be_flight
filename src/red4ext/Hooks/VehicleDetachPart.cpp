#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/DetachPartEvent.hpp>
#include "Extensions/MeshComponent.hpp"
#include "Utils/Utils.hpp"

using namespace RED4ext;

struct Manager {
  struct PartData {
    CName name;
    Handle<ent::IComponent> components[16];
    uint32_t numComponents;
    uint32_t unk10C;
    Handle<ent::IComponent> componentHandles[16];
    uint32_t someCount210;
    uint32_t unk214;
    uint64_t unk218[2];
    uint32_t unk228;
    uint32_t unk22C;
    uint64_t unk230[2];
    uint8_t unk240;
    uint64_t effectToken[2];
  };

  vehicle::BaseObject *vehicle;
  uint8_t set[0x50-0x08];
  uint64_t unk50;
  uint64_t unk58;
  uint64_t unk60;
  uint64_t unk68;
  DynArray<void *> unk70;
  DynArray<PartData> parts;
  HashMap<CName, void*> unk90;
  uint64_t detachedPartExplosionEffect;
  uint32_t unkC8;
  uint32_t unkCC;
};

REGISTER_FLIGHT_HOOK_HASH(void, 2150896908, VehicleDetachPart, const Manager & self, uint32_t index, bool a3) {
  // `Manager` is a hand-written layout; if it drifted, or the index is out of range,
  // just let the game do what it was going to do.
  if (!Utils::LooksLikeDynArray(self.parts, 256) || index >= self.parts.size) {
    LTBF_WARN_ONCE("[VehicleDetachPart] parts header looks wrong or index out of range (index {} size {} capacity {}); not filtering",
                   index, self.parts.size, self.parts.capacity);
    VehicleDetachPart_Original(self, index, a3);
    return;
  }
  if (!self.vehicle) {
    VehicleDetachPart_Original(self, index, a3);
    return;
  }

  const auto * part = &self.parts[index];
  auto fc = EntityExt::From(self.vehicle)->FindComponent<FlightComponent>();
  bool shouldDetach = true;
  if (fc && fc->configuration) {
    auto func = fc->configuration->GetType()->GetFunction("AreThrustersDetachable");
    bool detachable = true;
    if (func) {
      ExecuteFunction(fc->configuration, func, &detachable);
    }
    if (!detachable && Utils::LooksLikeDynArray(fc->configuration->thrusters, 64)) {
      for (auto const & thruster : fc->configuration->thrusters) {
        if (thruster && thruster->meshComponent && (part->name == thruster->meshComponent->name))
          shouldDetach = false;
      }
    }
  }

  if (shouldDetach)
    VehicleDetachPart_Original(self, index, a3);
}
