#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include <RED4ext/Scripting/Natives/Generated/EInputKey.hpp>
#include "FlightModule.hpp"
#include <stdio.h>
#include <Windows.h>
#include <Dbt.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <concrt.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Foundation.h>
using namespace winrt;
using namespace Windows::Gaming::Input;

struct XINPUT_GAMEPAD {
  WORD wButtons;
  BYTE bLeftTrigger;
  BYTE bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
};

struct XINPUT_STATE {
  DWORD dwPacketNumber;
  XINPUT_GAMEPAD Gamepad;
};

enum EInputAction {
  IACT_None = 0x0,
  IACT_Press = 0x1,
  IACT_Release = 0x2,
  IACT_Axis = 0x3,
};

enum EInputDevice {
  INVALID = 0x0,
  KBD_MOUSE = 0x1,
  ORBIS = 0x2,
  DURANGO = 0x3,
  STEAM = 0x4,
  XINPUT_PAD = 0x5,
  STADIA = 0x6,
  NINTENDO_SWITCH = 0x7,
  EID_COUNT = 0xC,
};


struct Input {
  RED4ext::EInputKey key;
  EInputAction action;
  float value;
  uint32_t unk0C;
  uint32_t unk10;
  uint32_t unk14;
  uint32_t unk18; // two disables
  uint32_t unk1C;
  HWND hwnd;
  uint64_t index;
  uint64_t unk30;
  uint8_t unk38;
  uint8_t unk39;
  uint8_t unk3A;
  uint8_t unk3B;
  uint8_t unk3C;
  uint8_t unk3D;
  uint8_t unk3E;
  uint8_t unk3F;
};

Input *__fastcall UpdateInput(Input *input, RED4ext::EInputKey key, EInputAction action, float value, int a5, int a6, HWND hwnd, uint32_t index) {
  input->hwnd = hwnd;
  input->index = index;
  input->unk0C = a5;
  input->unk10 = a6;
  input->value = value;
  input->unk18 = 1;
  input->unk30 = 0i64;
  input->unk38 = 0;
  input->key = key;
  input->action = action;
  return input;
}

class BaseGamepad {
  virtual BaseGamepad* Release(byte a1) = 0;
  virtual Input* GetInputs(RED4ext::DynArray<Input> *inputs, HWND hwnd) = 0;
  virtual Input* ResetInputs(RED4ext::DynArray<Input> *inputs, HWND hwnd) = 0;
  virtual RED4ext::CName* GetCName(RED4ext::CName*) = 0;
  virtual EInputDevice GetType() = 0;
  virtual void sub_28() { }
  virtual void sub_30() { }
  virtual uint32_t GetIndex() = 0;
  virtual bool IsEnabled(uint32_t* a1) = 0;

protected:
  uint32_t userIndex;
  XINPUT_STATE inputState;
};

concurrency::critical_section controllerListLock;

struct ICustomGameController : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

  RED4ext::DynArray<bool> buttons;
  RED4ext::DynArray<GameControllerSwitchPosition> switches;
  RED4ext::DynArray<float> axes;

  bool buttonsNew[0x100];
  std::vector<GameControllerSwitchPosition> switchesNew;
  std::vector<double> axesNew;

  RED4ext::DynArray<RED4ext::EInputKey> buttonKeys;
  RED4ext::DynArray<RED4ext::EInputKey> axisKeys;
  RED4ext::DynArray<bool> axisInversions;

  bool connected;
  RawGameController rawGameController = RawGameController(nullptr);

  void Setup();

  void Update();
};

RED4ext::TTypedClass<ICustomGameController> cls("ICustomGameController");

RED4ext::CClass *ICustomGameController::GetNativeType() { return &cls; }

void ICustomGameController::Setup() {

  connected = true;

  auto numButtons = rawGameController.ButtonCount();
  for (int i = 0; i < numButtons; ++i) {
    buttons.EmplaceBack(false);
    buttonKeys.EmplaceBack(RED4ext::EInputKey::IK_None);
  }

  auto numAxes = rawGameController.AxisCount();
  for (int i = 0; i < numAxes; ++i) {
    axes.EmplaceBack(false);
    axisKeys.EmplaceBack(RED4ext::EInputKey::IK_None);
    axisInversions.EmplaceBack(false);
  }
  axesNew.resize(numAxes);

  auto numSwitches = rawGameController.SwitchCount();
  for (int i = 0; i < numSwitches; ++i) {
    switches.EmplaceBack(GameControllerSwitchPosition::Center);
  }
  switchesNew.resize(numSwitches);

  auto onInit = GetType()->GetFunction("OnSetup");
  if (onInit) {
    auto stack = RED4ext::CStack(this, nullptr, 0, nullptr, 0);
    onInit->Execute(&stack);
  }
};

 void ICustomGameController::Update() {
  if (rawGameController) {
    rawGameController.GetCurrentReading(buttonsNew, switchesNew, axesNew);
  }

  auto onUpdate = GetType()->GetFunction("OnUpdate");
  if (onUpdate) {
    auto stack = RED4ext::CStack(this, nullptr, 0, nullptr, 0);
    onUpdate->Execute(&stack);
  }
 };

 void SetButtonScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
   uint32_t button;
   RED4ext::EInputKey key;
   RED4ext::GetParameter(aFrame, &button);
   RED4ext::GetParameter(aFrame, &key);

   aFrame->code++;

   auto icgc = reinterpret_cast<ICustomGameController *>(aContext);

   if (button < icgc->buttonKeys.size) {
     icgc->buttonKeys[button] = key;
   }
 }

 void SetAxisScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
   uint32_t axis;
   RED4ext::EInputKey key;
   bool inverted;
   RED4ext::GetParameter(aFrame, &axis);
   RED4ext::GetParameter(aFrame, &key);
   RED4ext::GetParameter(aFrame, &inverted);

   aFrame->code++;

   auto icgc = reinterpret_cast<ICustomGameController *>(aContext);

   if (axis < icgc->axisKeys.size) {
     icgc->axisKeys[axis] = key;
     icgc->axisInversions[axis] = inverted;
   }
 }

class CustomGamepad : BaseGamepad {
public:
  RED4ext::DynArray<ICustomGameController> controllers;

  void __fastcall Initialize(uint32_t gamepadIndex) {
    userIndex = gamepadIndex;
    RawGameController::RawGameControllerAdded(
        [this](winrt::Windows::Foundation::IInspectable const &, RawGameController const &addedController) {
          concurrency::critical_section::scoped_lock{controllerListLock};
          for (auto &controller : controllers) {
            if (!controller.connected) {
              controller.rawGameController = addedController;
              controller.connected = true;
              return;
            }
          }

          auto rtti = RED4ext::CRTTISystem::Get();
          char className[100];
          sprintf(className, "CustomGameController_%04X_%04X", addedController.HardwareVendorId(), addedController.HardwareProductId());
          auto controllerCls = rtti->GetClassByScriptName(className);
          if (!controllerCls) {
            controllerCls = rtti->GetClassByScriptName("CustomGameController");
          }
          if (controllerCls) {
            auto controller = reinterpret_cast<ICustomGameController *>(controllerCls->AllocInstance(true));
            controllerCls->ConstructCls(controller);

            auto handle = RED4ext::Handle<ICustomGameController>(controller);
            controller->ref = RED4ext::WeakHandle(*reinterpret_cast<RED4ext::Handle<RED4ext::ISerializable> *>(&handle));
            controller->unk30 = controllerCls;

            controller->rawGameController = addedController;
            controller->Setup();
            controllers.EmplaceBack(*controller);
          }
        });

    RawGameController::RawGameControllerRemoved(
        [this](winrt::Windows::Foundation::IInspectable const &, RawGameController const &removedController) {
          concurrency::critical_section::scoped_lock{controllerListLock};
          for (auto &controller : controllers) {
            if (controller.rawGameController == removedController) {
              controller.connected = false;
              return;
            }
          }
        });
  }

  BaseGamepad *__fastcall Release(byte a1) {
    //if ((a1 & 1) != 0)
      //free(this);
    return this;
  }

  Input *__fastcall GetInputs(RED4ext::DynArray<Input> *inputs, HWND hwnd) { 
    for (auto &controller : controllers) {
      if (!controller.connected)
        continue;


      controller.Update();

      auto buttonCount = controller.rawGameController.ButtonCount();
      for (int i = 0; i < buttonCount; ++i) {
        if (controller.buttons[i] != controller.buttonsNew[i]) {
          controller.buttons[i] = controller.buttonsNew[i];
          auto key = controller.buttonKeys[i];
          if (key != RED4ext::EInputKey::IK_None) {
            auto input = new Input();
            UpdateInput(input, key, controller.buttons[i] ? EInputAction::IACT_Press : EInputAction::IACT_Release, 1.0, 0,
                        0, hwnd,
                        userIndex);
            inputs->EmplaceBack(*input);
          }
        }
      }
      auto axisCount = controller.rawGameController.AxisCount();
      for (int i = 0; i < axisCount; ++i) {
        if (controller.axes[i] != controller.axesNew[i]) {
          controller.axes[i] = controller.axesNew[i];
          auto key = controller.axisKeys[i];
          if (key != RED4ext::EInputKey::IK_None) {
            auto input = new Input();
            UpdateInput(input, key, EInputAction::IACT_Axis, (controller.axes[i] - 0.5) * 2.0 * (controller.axisInversions[i] ? -1.0 : 1.0), 0,
                        0, hwnd, userIndex);
            inputs->EmplaceBack(*input);
          }
        }
      }
    }
    return inputs->end();
  };

  Input *__fastcall ResetInputs(RED4ext::DynArray<Input> *inputs, HWND hwnd) {
    auto input = new Input();
    UpdateInput(input, RED4ext::EInputKey::IK_Pad_RightAxisX, EInputAction::IACT_Axis, 0.0, 0, 0, hwnd, userIndex);
    inputs->EmplaceBack(*input);
    UpdateInput(input, RED4ext::EInputKey::IK_Pad_RightAxisY, EInputAction::IACT_Axis, 0.0, 0, 0, hwnd, userIndex);
    inputs->EmplaceBack(*input);
    UpdateInput(input, RED4ext::EInputKey::IK_Pad_LeftAxisX, EInputAction::IACT_Axis, 0.0, 0, 0, hwnd, userIndex);
    inputs->EmplaceBack(*input);
    UpdateInput(input, RED4ext::EInputKey::IK_Pad_LeftAxisY, EInputAction::IACT_Axis, 0.0, 0, 0, hwnd, userIndex);
    inputs->EmplaceBack(*input);
    return input;
  };

  RED4ext::CName *__fastcall GetCName(RED4ext::CName *cname) { 
    *cname = RED4ext::CName("xpad");
    return cname;
  }

  EInputDevice __fastcall GetType() { 
    return EInputDevice::XINPUT_PAD;
  }

  uint32_t __fastcall GetIndex() {
    return this->userIndex;
  }

  bool __fastcall IsEnabled(uint32_t *a1) {
    return true;
  }
};

// 48 8D 05 A9 3C 9B 02 89 51 08 48 89 01 0F 57 C0
CustomGamepad *__fastcall InitializeXPad(CustomGamepad *, uint32_t);
constexpr uintptr_t InitializeXPadAddr = 0x795360;
decltype(&InitializeXPad) InitializeXPad_Original;


CustomGamepad *__fastcall InitializeXPad(CustomGamepad *gamepad, uint32_t gamepadIndex) {
  if (gamepadIndex == 3) {
    gamepad = new CustomGamepad();
    gamepad->Initialize(gamepadIndex);
  } else {
    InitializeXPad_Original(gamepad, gamepadIndex);
  }
  return gamepad;
}

struct CustomInputModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(InitializeXPadAddr), &InitializeXPad,
                                  reinterpret_cast<void **>(&InitializeXPad_Original)))
      ;
  
  }

  void RegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    cls.flags = {.isNative = true};
    //cls.flags = {.isAbstract = true, .isNative = true, .isImportOnly = true};
    cls.parent = rtti->GetClass("IScriptable");
    rtti->RegisterType(&cls);

    //RED4ext::CNamePool::Add("GameControllerSwitchPosition");
    //auto gcsp = RED4ext::CEnum::CEnum("GameControllerSwitchPosition", 4, {.isScripted = true});
    //gcsp.hashList.EmplaceBack(RED4ext::CName("Center"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("Up"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("UpRight"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("Right"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("DownRight"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("Down"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("DownLeft"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("Left"));
    //gcsp.hashList.EmplaceBack(RED4ext::CName("UpLeft"));
    //gcsp.valueList.EmplaceBack(0);
    //gcsp.valueList.EmplaceBack(1);
    //gcsp.valueList.EmplaceBack(2);
    //gcsp.valueList.EmplaceBack(3);
    //gcsp.valueList.EmplaceBack(4);
    //gcsp.valueList.EmplaceBack(5);
    //gcsp.valueList.EmplaceBack(6);
    //gcsp.valueList.EmplaceBack(7);
    //gcsp.valueList.EmplaceBack(8);
    //rtti->RegisterType(&gcsp);

  }

  void PostRegisterTypes() {
    RED4ext::CNamePool::Add("array:EInputKey");
    //RED4ext::CNamePool::Add("array:GameControllerSwitchPosition");
    auto rtti = RED4ext::CRTTISystem::Get();

    RED4ext::CProperty::Flags flags = {
        //.b21 = true,
        //.b29 = true,
        //.b31 = true,
        //.b35 = true,
        //.b36 = true,
    };

    cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:Bool"), "buttons", nullptr,
                                                  offsetof(ICustomGameController, buttons), "CustomGameController", flags));
    //cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:GameControllerSwitchPosition"), "switches", .ullptr,
                                                  //offsetof(ICustomGameController, switches)));
    cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:Float"), "axes", nullptr,
                                                  offsetof(ICustomGameController, axes), "CustomGameController", flags));
    //cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:EInputKey"), "buttonKeys", nullptr,
    //                                              offsetof(ICustomGameController, buttonKeys), "CustomGameController", flags));
    //cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:EInputKey"), "axisKeys", nullptr,
    //                                              offsetof(ICustomGameController, axisKeys), "CustomGameController", flags));
    //cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:Bool"), "axisInversions", nullptr,
    //                                              offsetof(ICustomGameController, axisInversions), "CustomGameController", flags));

    auto setButton =
        RED4ext::CClassFunction::Create(&cls, "SetButton", "SetButton", &SetButtonScripts, {.isNative = true});
    cls.RegisterFunction(setButton);
    auto setAxis = RED4ext::CClassFunction::Create(&cls, "SetAxis", "SetAxis", &SetAxisScripts, {.isNative = true});
    cls.RegisterFunction(setAxis);
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(InitializeXPadAddr));
  }
};

REGISTER_FLIGHT_MODULE(CustomInputModule);