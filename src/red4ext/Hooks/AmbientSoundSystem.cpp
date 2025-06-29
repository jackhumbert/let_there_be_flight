#include "Utils/FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/FixedPoint.hpp>

using namespace RED4ext;

class IHookable {
public:
  template<typename R, uint32_t Hash, typename... Args>
  inline R Hook(Args... args) {
    static auto func = UniversalRelocFunc<R (*)(IHookable *, Args...)>(Hash);
    return func(this, std::forward<Args>(args)...);
  }

  template<typename R, uint32_t Hash, typename... Args>
  inline R Hook(Args... args) const {
    static auto func = UniversalRelocFunc<R (*)(IHookable const *, Args...)>(Hash);
    return func(this, std::forward<Args>(args)...);
  }
};

// template<typename R, uint32_t Hash, typename... Args>
// class HookBase : UniversalRelocBase {
//     using T = R (*)(Args);
//   public:
//     // HookBase(uint32_t aHash)
//     //     : m_address(reinterpret_cast<T>(Resolve(aHash)))
//     // {
//     // }
//     HookBase(Args&&... args)
//         : m_address(reinterpret_cast<T>(Resolve(Hash)))
//     {

//     }

//     // inline operator T() const
//     // {
//     //     return m_address;
//     // }

//     // R operator()(Args&&... args) {
//     //   return m_address(std::forward<Args>(args));  
//     // }

//     inline operator R(Args&&... args) const {
//       return m_address(std::forward<Args>(args)); 
//     }

// private:
//     T m_address;
// };

// template<typename T>
// class Hook
// {
// };

// template<typename R, typename... Args>
// class Hook<R (*)(Args...)> : public HookBase<R (*)(Args...)>
// {
//     using HookBase<R (*)(Args...)>::HookBase;
// };

// template<typename C, typename R, typename... Args>
// class Hook<R (C::*)(Args...)> : public HookBase<R (*)(C*, Args...)>
// {
//     using HookBase<R (*)(C*, Args...)>::HookBase;
// };

struct AmbientAudioInstance {

};

struct AudParameter {

};

struct AmbientPaletteTag {
  CName name;
};

struct AmbientPaletteBrush : IHookable {
  float distributionBucketSize;
  float virtualHearingRadius;
  float hearingDistanceCooldown;
  uint32_t unk0C;
  DynArray<void *> eventsPool;
  CName radioStationMetadata;
  float maxAttenuation;
  
  // void ComputeMaxAttenuation() const { 
  //   return Hook<void, 2200966535>(this);
  // }
  void ComputeMaxAttenuation() const { 
    return Hook<void, 2200966535>(this);
  }
};

// audio::LocomotionStateEventDictionary
// audio::MetadataDictionary<audio::EventOverrideDictionaryItem,CName,CName>
struct MetadataDictionary : ISerializable {
  void * unk30;
  void * unk38;
  void * unk40;
  void * unk48;
  void * unk50;
  Map<CName, AmbientPaletteBrush> brushes;
  uint64_t unk68;
  uint64_t unk70;
  uint64_t unk78;
  uint64_t unk80;
  uint64_t unk88;
  uint64_t unk90;
  uint64_t unk98;
  uint64_t unkA0;
  uint64_t unkA8;
  uint64_t unkB0;
  uint64_t unkB8;
  uint64_t unkC0;
  uint64_t unkC8;
  uint64_t unkD0;
  uint64_t unkD8;
  uint64_t unkE0;
  uint64_t unkE8;
  uint64_t unkF0;
  uint64_t unkF8;
  uint64_t unk100;
};
RED4EXT_ASSERT_OFFSET(MetadataDictionary, brushes, 0x58);

struct AmbientPaletteBrushCategory {
  void * unk00;
  void * unk08;
  uint64_t unk10[3];
  uint32_t id;
  CName name;
  MetadataDictionary * audioMetadatas;
};



// template<typename T>
// class MyRelocFunc
// {
// };

// template<typename R, typename... Args>
// class MyRelocFunc<R (*)(Args...)> : public UniversalRelocFunc<R (*)(Args...)>
// {
//     using UniversalRelocFunc<R (*)(Args...)>::UniversalRelocFunc;
// };

// template<typename C, typename R, typename... Args>
// class MyRelocFunc<R (C::*)(Args...)> : public UniversalRelocFunc<R (*)(C*, Args...)>
// {
//     using UniversalRelocFunc<R (*)(C*, Args...)>::UniversalRelocFunc;
// };


struct AmbientPalette : IHookable {
  DynArray<AmbientPaletteBrushCategory const *> * brushCategories;

  AmbientPaletteBrush * GetBrush(CName name) const {
    auto result = Hook<AmbientPaletteBrush *, 3218282947>(name);
    return result;
  }

  // UniversalRelocFunc<AmbientPaletteBrush * (*)(CName)> GetBrush = new(3218282947);
  // AmbientPaletteBrush * (*GetBrush)(CName) = UniversalRelocFunc<AmbientPaletteBrush * (*)(CName)>(3218282947);

  
  // MyRelocFunc<AmbientPaletteBrush * (AmbientPalette::*)(CName)> GetBrush = { 3218282947 };
};

// AmbientPaletteBrush * AmbientPalette::GetBrush(CName name) { 
//   RED4ext::UniRelocFunc<decltype(&AmbientPalette::GetBrush)> call(3218282947);
//   auto result = call(this, name);
//   return result;
// }

struct AmbientPaletteEntry {
  CName brushCategory;
};

struct AmbientPaletteBucket : IHookable {
  uint32_t freeEmitterIndex;
  uint32_t unk04;
  CName eventName;
  uint16_t tagIndex;
  uint16_t unk12;
  int16_t unk14;
  uint64_t unk18;
  uint64_t unk20;
  DynArray<void *> unk28;

  bool Update(double a1, Vector4 const & a2, float a3, DynArray<AmbientPaletteTag> const & a4) {
    return Hook<bool, 4016842136>(a1, a2, a3, a4);
  }
  
  void CleanUpInvalidTags(DynArray<AmbientPaletteTag> const & tags) {
    return Hook<void, 2373657855>(tags);
  }
};

struct BrushMap : HashMap<int, AmbientPaletteBucket> {

};

struct AmbientPaletteSpace : IHookable {

  Map<CName, BrushMap> cname_brushMap;
  // uint32_t unk24;
  Map<CName, uint32_t> CName_uint_28;
  AmbientPalette *palette;
  uint64_t unk58;
  uint64_t unk60;

  void HandleBucketSoundEmission(AmbientPaletteBucket & bucket, AmbientPaletteBrush const * brush, CName brushName, DynArray<AmbientPaletteTag> const & tags) {
    Hook<void, 2926132544>(bucket, brush, brushName, tags);
  }
};

struct AmbientSoundSystem {
  DynArray<WeakHandle<AmbientAudioInstance>> audioInstances00;
  DynArray<WeakHandle<IScriptable>> weakHandles10;
  DynArray<AudParameter> AudParameter20; // not present at start
  Map<CName, CName> CName_CName30; // event overrides?
  // uint32_t unk54;
  Map<CName, WeakHandle<AmbientAudioInstance>> oldAudioInstances58; // old instances?
  // uint32_t unk7C;
  DynArray<Handle<AmbientAudioInstance>> AmbientAudioInstance80;
  DynArray<AudParameter> AudParameter90; // not present at start
  DynArray<CName> CNameA0;
  DynArray<CName> CNameB0;
  uint64_t unkC0;
  DynArray<AmbientPaletteEntry> AmbientPaletteEntryC8; // Dead Puppet IDs?
  uint8_t spinlock1;
  uint8_t spinlockC8;
  uint8_t spinlock30;
  uint8_t spinlock4;
  uint8_t spinlock5;
  uint8_t spinlock6;
  uint8_t spinlock7;
  uint8_t spinlock8;
  DynArray<CName> ambientPaletteNamesE0; // ambient palette related 
  // amb_pal_g_aircons_01 
  // amb_pal_exploration_city
  // badlands_foliage_palette
  // amb_pal_exploration_bl
  DynArray<AmbientPaletteBrushCategory const *> brushCategories;
  Handle<AmbientPalette> AmbientPalette100;
  float unk110;
};

RED4EXT_ASSERT_SIZE(AmbientSoundSystem, 0x118);

// gets listen vectors from SoundSystem
// loops through audioInstances00, calls Tick, increment count based on unk38F, updates emitter positions
// loops through oldAudioInstances58 and removes
// updates priorities
// REGISTER_FLIGHT_HOOK_HASH(void, 853347970, AmbientSoundSystem_Tick, AmbientSoundSystem *self, float delta) {
//   return AmbientSoundSystem_Tick_Original(self, delta);
// }

// REGISTER_FLIGHT_HOOK_HASH(void, 2073240708, AmbientPaletteSpace_Update, AmbientPaletteSpace *self, 
//   double delta, Vector4 const & position, DynArray<AmbientPaletteTag> const & tags) {
//   //AmbientPaletteSpace_Update_Original(self, delta, position, tags);
  
//   if (self->palette) {
//     self->unk60 = 0;
//     self->cname_brushMap.ForEach([self, delta, position, tags](const CName & brushName, BrushMap & brushmap) {
//       auto brush = self->palette->GetBrush(brushName);
//       if (brush) {
//         if (brush->maxAttenuation < 0.0) {
//           brush->ComputeMaxAttenuation();
//         }
//         auto distance = fminf(brush->virtualHearingRadius, brush->maxAttenuation);
//         brushmap.ForEach([self, delta, position, distance, tags, brush, brushName](const int & id, AmbientPaletteBucket & bucket) {
//           if (bucket.Update(delta, position, distance, tags)) {
//             self->HandleBucketSoundEmission(bucket, brush, brushName, tags);
//           }
//           bucket.CleanUpInvalidTags(tags);
//         });
//       }
//     });
//   }
// }