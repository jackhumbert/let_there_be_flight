#pragma once

#include <RED4ext/RED4ext.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <spdlog/spdlog.h>

struct FlightModule {
  virtual void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle){};
  virtual void RegisterTypes(){};
  virtual void PostRegisterTypes(){};
  virtual void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle){};
};

class IFlightModuleHook : FlightModule {
public:
  std::string m_name;
  uintptr_t m_address;
  void *m_hook;
  void **m_original;

  virtual void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) override {
    spdlog::info("Attaching {} at 0x{:X}", m_name, m_address);
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(this->m_address), this->m_hook, this->m_original)) {
      spdlog::info("  trying again");
    }
  };

  virtual void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) override {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(this->m_address));
  };
};

class FlightModuleFactory {
  //std::map<std::string, std::function<FlightModule *()>> s_creators;
  std::vector<std::function<void(const RED4ext::Sdk *, RED4ext::PluginHandle)>> s_loads;
  std::vector<std::function<void(const RED4ext::Sdk *, RED4ext::PluginHandle)>> s_unloads;
  std::vector<std::function<void()>> s_registers;
  std::vector<std::function<void()>> s_postRegisters;
  std::vector<IFlightModuleHook*> s_hooks;

public:
  static FlightModuleFactory &GetInstance() {
    static FlightModuleFactory s_instance;
    return s_instance;
  }

  template <class T> void registerClass(const std::string &name) {
    //modules.emplace_back(new T());
    //s_creators.insert({name, []() -> FlightModule * { return new T(); }});
    s_loads.emplace_back(
        [](const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) -> void { (new T())->Load(aSdk, aHandle); });
    s_unloads.emplace_back(
        [](const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) -> void { (new T())->Unload(aSdk, aHandle); });
    s_registers.emplace_back([]() -> void { (new T())->RegisterTypes(); });
    s_postRegisters.emplace_back([]() -> void { (new T())->PostRegisterTypes(); });
    //s_registers.insert({name, &T::RegisterTypes});
    //s_postRegisters.insert({name, &T::PostRegisterTypes});
    //s_unloads.insert({name, &T::Unload});
  }

  void registerHook(IFlightModuleHook *moduleHook) {
    s_hooks.emplace_back(moduleHook);
  }

  //FlightModule *create(const std::string &name) {
  //  const auto it = s_creators.find(name);
  //  if (it == s_creators.end())
  //    return nullptr; // not a derived class
  //  return (it->second)();
  //}

  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    for (const auto &load : s_loads) {
      load(aSdk, aHandle);
    }
    for (const auto &hook : s_hooks) {
      hook->Load(aSdk, aHandle);
    }
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    for (const auto &unload : s_unloads) {
      unload(aSdk, aHandle);
    }
    for (const auto &hook : s_hooks) {
      hook->Unload(aSdk, aHandle);
    }
  }

  void RegisterTypes() {
    for (const auto &s_register : s_registers) {
      s_register();
    }
  }

  void PostRegisterTypes() {
    for (const auto &postRegister : s_postRegisters) {
      postRegister();
    }
  }
};

// FlightModuleFactory::GetInstance()

template <class T> class FlightModuleRegister {
public:
  explicit FlightModuleRegister(const std::string &name) { FlightModuleFactory::GetInstance().registerClass<T>(name); }
};

#define REGISTER_FLIGHT_MODULE(derived_class)                                                                            \
  FlightModuleRegister<derived_class> s_##derived_class##Creator(#derived_class);

class FlightModuleHook : IFlightModuleHook {
public: 
  explicit FlightModuleHook(std::string name, uintptr_t address, void * hook, void ** original) {
    this->m_name = name;
    this->m_address = address;
    this->m_hook = hook;
    this->m_original = original;
    FlightModuleFactory::GetInstance().registerHook(this);
  }
};

//#define BasicFuncAddr 0x1
//
//inline void BasicFunc(void * a1) {
//
//}

//decltype(&BasicFunc) BasicFunc_Original;
//FlightModuleHook<decltype(BasicFunc)> s_BasicFunc(BasicFuncAddr, BasicFunc, BasicFunc_Original);

#define REGISTER_FLIGHT_HOOK(retType, func, ...) \
  retType func(__VA_ARGS__); \
  decltype(&func) func##_Original; \
  FlightModuleHook s_##func##_Hook(#func, func##Addr, reinterpret_cast<void*>(&func), reinterpret_cast<void**>(&func##_Original)); \
  retType func(__VA_ARGS__)

//REGISTER_FLIGHT_HOOK(BasicFunc);