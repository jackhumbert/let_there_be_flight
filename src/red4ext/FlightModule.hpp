#pragma once

#include <RED4ext/RED4ext.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <string>

struct FlightModule {
  virtual void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle){};
  virtual void RegisterTypes(){};
  virtual void PostRegisterTypes(){};
  virtual void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle){};
};

class FlightModuleFactory {
  //std::map<std::string, std::function<FlightModule *()>> s_creators;
  std::vector<std::function<void(const RED4ext::Sdk *, RED4ext::PluginHandle)>> s_loads;
  std::vector<std::function<void(const RED4ext::Sdk *, RED4ext::PluginHandle)>> s_unloads;
  std::vector<std::function<void()>> s_registers;
  std::vector<std::function<void()>> s_postRegisters;

public:
  static FlightModuleFactory &getInstance() {
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
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    for (const auto &unload : s_unloads) {
      unload(aSdk, aHandle);
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

// FlightModuleFactory::getInstance()

template <class T> class FlightModuleRegister {
public:
  explicit FlightModuleRegister(const std::string &name) { FlightModuleFactory::getInstance().registerClass<T>(name); }
};

#define REGISTER_FLIGHT_MODULE(derived_class)                                                                            \
  FlightModuleRegister<derived_class> s_##derived_class##Creator(#derived_class);
