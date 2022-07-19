#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>

template <typename T> T *CreateWithVFT() {
  auto inst = new T();
  return inst;
}

struct IRuntimeClass {
  static constexpr const uintptr_t VFT = 0x35E6D08 + 0xC00;
  static constexpr const size_t VFT_SIZE = 0x80;



  IRuntimeClass() { 
    *(uintptr_t *)this = VFT + reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr)); 
  }

  template<typename T> static T *CreateInstance() {
    uintptr_t* newVft = malloc(T::VFT_SIZE);
    auto inst = new T();
    auto vft = *(uintptr_t)inst;
    for (auto vf = *vft; vf < vft + T::VFT_SIZE; vf += 8) {
      newVft = vf;
      newVft += 8;
    }
    return inst;
  }
};

struct RuntimeClass : IRuntimeClass {
  RuntimeClass() : IRuntimeClass() {

  }
};