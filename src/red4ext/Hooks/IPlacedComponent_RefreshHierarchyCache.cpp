#include <Utils/FlightModule.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IAttachment.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/ITransformAttachment.hpp>
#include <RED4ext/SharedSpinLock.hpp>
#include <mutex>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <vector>

// The game's ent::IPlacedComponent::RefreshHierarchyCache gathers a component's transform children into a
// 256-entry array on its stack (CollectOutgoingTransformAttachments, with no bounds check) before it lists them
// in the component's relationships. A component with more than 256 children overwrites the array's count and the
// game crashes on the next write (Cyberpunk2077.exe+0x40404A on 2.5). Cars that another mod has filled with parts
// get there: Immersive Car Modification put 278 on a Caliburn's vehicle_slots, and it crashed at take-off, at
// Autopilot engage, and once while loading with LTBF turned off. For such a component this does the same work
// with a list that grows; every other component goes through the game's own code.

using namespace RED4ext;

namespace {
// 2.5 layouts, from RefreshHierarchyCache (0x140403DE0) and CollectOutgoingTransformAttachments (0x140403FD4)
constexpr uint32_t kStackCapacity = 256;
constexpr size_t kAttachments = 0x70;       // DynArray<Handle<ent::IAttachment>>, its live entries from kFirst
constexpr size_t kFirst = 0x80;             // uint16_t: entries before it are not live
constexpr size_t kAttachmentsLock = 0x8A;   // red::old::RWSpinLock (SharedSpinLock)
constexpr size_t kPlacedFlags = 0x88;       // a child is listed when bit 2 is set
constexpr size_t kRelationships = 0xA0;     // DynArray<Relationship>
constexpr size_t kNeedsRefresh = 0xB2;      // bool, cleared once the cache is rebuilt
constexpr uint8_t kAttachmentActive = 8;    // ent::ITransformAttachment flags (+0x50)
constexpr uint8_t kAttachmentDisabled = 2;

struct Relationship {
  WeakHandle<ent::IPlacedComponent> component;
  WeakHandle<ISerializable> attachment;
};
RED4EXT_ASSERT_SIZE(Relationship, 0x20);

template <typename T> T &At(void *aBase, size_t aOffset) {
  return *reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(aBase) + aOffset);
}

uint32_t LiveAttachments(ent::IPlacedComponent *aComponent) {
  auto &attachments = At<DynArray<Handle<ent::IAttachment>>>(aComponent, kAttachments);
  return attachments.size - At<uint16_t>(aComponent, kFirst);
}

// What the game's RefreshHierarchyCache does, without its 256 limit.
void RefreshWithoutLimit(ent::IPlacedComponent *aComponent) {
  static auto *transformAttachmentType = CRTTISystem::Get()->GetClass("entITransformAttachment");
  static auto *placedComponentType = CRTTISystem::Get()->GetClass("entIPlacedComponent");

  auto &relationships = At<DynArray<Relationship>>(aComponent, kRelationships);
  relationships.Clear();

  std::vector<ent::ITransformAttachment *> children;
  {
    std::shared_lock guard(At<SharedSpinLock>(aComponent, kAttachmentsLock));
    auto &attachments = At<DynArray<Handle<ent::IAttachment>>>(aComponent, kAttachments);
    for (uint32_t i = At<uint16_t>(aComponent, kFirst); i < attachments.size; ++i) {
      auto *attachment = attachments[i].GetPtr();
      if (!attachment || !attachment->GetType()->IsA(transformAttachmentType)) {
        continue;
      }
      auto *transform = static_cast<ent::ITransformAttachment *>(attachment);
      if ((transform->flags & kAttachmentActive) && !(transform->flags & kAttachmentDisabled)) {
        children.push_back(transform);
      }
    }
  }

  relationships.Reserve(static_cast<uint32_t>(children.size()));
  for (auto *child : children) {
    Handle<ent::IComponent> destination = child->destination.Lock();
    if (!destination || !destination->GetType()->IsA(placedComponentType) ||
        !(At<uint8_t>(destination.GetPtr(), kPlacedFlags) & 2)) {
      continue;
    }
    relationships.EmplaceBack();
    Relationship &added = relationships[relationships.size - 1];
    added.component = *reinterpret_cast<Handle<ent::IPlacedComponent> *>(&destination);
    added.attachment = static_cast<const WeakHandle<ISerializable> &>(*child);
  }
  At<bool>(aComponent, kNeedsRefresh) = false;
}
} // namespace

// void ent::IPlacedComponent::RefreshHierarchyCache()
REGISTER_FLIGHT_HOOK_HASH(void, 153883440, IPlacedComponent_RefreshHierarchyCache, ent::IPlacedComponent *aComponent) {
  const uint32_t live = LiveAttachments(aComponent);
  if (live <= kStackCapacity) {
    IPlacedComponent_RefreshHierarchyCache_Original(aComponent);
    return;
  }
  static std::once_flag warned;
  std::call_once(warned, [live] {
    spdlog::warn("[RefreshHierarchyCache] a component with {} attachments, more than the game's {}: listed without "
                 "the limit (logged once)",
                 live, kStackCapacity);
  });
  RefreshWithoutLimit(aComponent);
}
