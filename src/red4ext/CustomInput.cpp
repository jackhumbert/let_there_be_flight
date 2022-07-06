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

enum EControllerType {
  UndefinedController = 0x0,
  PS4 = 0x1,
  PS5 = 0x2,
  Xbox = 0x5,
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
  virtual EControllerType GetType() = 0;
  virtual void sub_28() { }
  virtual void sub_30() { }
  virtual uint32_t GetIndex() = 0;
  virtual bool IsEnabled(uint32_t* a1) = 0;

protected:
  uint32_t userIndex;
  XINPUT_STATE inputState;
};

//ULONG capValues[8];
//ULONG usageValues[0x80];
//
//std::vector<RawGameController> myRawGameControllers;
//concurrency::critical_section myLock{};


struct Joystick {

  bool connected;
  RawGameController rawGameController = RawGameController(nullptr);

  bool buttons[0x100];
  std::vector<GameControllerSwitchPosition> switches;
  std::vector<double> axes;

  bool buttonsNew[0x100];
  std::vector<GameControllerSwitchPosition> switchesNew;
  std::vector<double> axesNew;

  Joystick(RawGameController const &rgc) {
    rawGameController = rgc;
    axes.resize(rawGameController.AxisCount());
    axesNew.resize(rawGameController.AxisCount());
    connected = true;
  }

  void Update() {
    rawGameController.GetCurrentReading(buttonsNew, switchesNew, axesNew);
  }
};

std::vector<Joystick> joysticks;
concurrency::critical_section controllerListLock;

class CustomGamepad : BaseGamepad {
public:
  void __fastcall Initialize(uint32_t gamepadIndex) {
    userIndex = gamepadIndex;
    RawGameController::RawGameControllerAdded(
        [](winrt::Windows::Foundation::IInspectable const &, RawGameController const &addedController) {
          concurrency::critical_section::scoped_lock{controllerListLock};
          for (auto &joystick : joysticks) {
            if (!joystick.connected) {
              joystick.rawGameController = addedController;
              joystick.connected = true;
              return;
            }
          }
          auto joystick = new Joystick(addedController);
          joysticks.push_back(*joystick);
        });

    RawGameController::RawGameControllerRemoved(
        [](winrt::Windows::Foundation::IInspectable const &, RawGameController const &removedController) {
          concurrency::critical_section::scoped_lock{controllerListLock};
          for (auto &joystick : joysticks) {
            if (joystick.rawGameController == removedController) {
              joystick.connected = false;
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

  RED4ext::EInputKey GetKeyForButton(uint32_t button) {
    switch (button) {
    case (1 - 1):
      return RED4ext::EInputKey::IK_Joy1;
    case (2 - 1):
      return RED4ext::EInputKey::IK_Joy2;
    case (3 - 1):
      return RED4ext::EInputKey::IK_Joy3;
    case (4 - 1):
      return RED4ext::EInputKey::IK_Pad_Start;
    case (6 - 1):
      return RED4ext::EInputKey::IK_Pad_Y_TRIANGLE;
    case (7 - 1):
      return RED4ext::EInputKey::IK_Pad_B_CIRCLE;
    case (8 - 1):
      return RED4ext::EInputKey::IK_Pad_A_BOX;
    case (9 - 1):
      return RED4ext::EInputKey::IK_Pad_X_SQUARE;
    case (10 - 1):
      return RED4ext::EInputKey::IK_Pad_RightThumb;
    case (11 - 1):
      return RED4ext::EInputKey::IK_Pad_RightShoulder;
    case (50 - 1):
      return RED4ext::EInputKey::IK_Joy4;
    case (51 - 1):
      return RED4ext::EInputKey::IK_Joy5;
    case (52 - 1):
      return RED4ext::EInputKey::IK_Joy6;
    case (54 - 1):
      return RED4ext::EInputKey::IK_Pad_LeftThumb;
    case (56 - 1):
      return RED4ext::EInputKey::IK_Pad_Back_Select;
    case (61 - 1):
      return RED4ext::EInputKey::IK_Pad_DigitDown;
    case (62 - 1):
      return RED4ext::EInputKey::IK_Pad_DigitUp;
    case (63 - 1):
      return RED4ext::EInputKey::IK_Pad_DigitLeft;
    case (64 - 1):
      return RED4ext::EInputKey::IK_Pad_DigitRight;
    case (66 - 1):
      return RED4ext::EInputKey::IK_Pad_LeftShoulder;
    default:
      return RED4ext::EInputKey::IK_None;
    }
  }

  RED4ext::EInputKey GetKeyForAxis(uint32_t axis) {
    switch (axis) {
    case 0: 
      return RED4ext::EInputKey::IK_Pad_LeftAxisX;
    case 1: 
      return RED4ext::EInputKey::IK_Pad_LeftAxisY;
    case 2: 
      return RED4ext::EInputKey::IK_JoyR;
    case 3: 
      return RED4ext::EInputKey::IK_Pad_RightAxisX;
    case 4: 
      return RED4ext::EInputKey::IK_Pad_RightAxisY;
    case 5: 
      return RED4ext::EInputKey::IK_Joy6;
    default: 
      return RED4ext::EInputKey::IK_None;
    }
  }

  double GetInversionForAxis(uint32_t axis) {
    switch (axis) {
    case 0: 
      return 1.0;
    case 1: 
      return -1.0;
    case 2: 
      return 1.0;
    case 3: 
      return 1.0;
    case 4: 
      return -1.0;
    case 5: 
      return 1.0;
    default: 
      return 1.0;
    }
  }

  Input *__fastcall GetInputs(RED4ext::DynArray<Input> *inputs, HWND hwnd) { 
    for (auto &joystick : joysticks) {
      if (!joystick.connected)
        continue;
      joystick.Update();
      auto buttonCount = 0x100;
      for (int i = 0; i < buttonCount; ++i) {
        if (joystick.buttons[i] != joystick.buttonsNew[i]) {
          joystick.buttons[i] = joystick.buttonsNew[i];
          auto key = GetKeyForButton(i);
          if (key != RED4ext::EInputKey::IK_None) {
            auto input = new Input();
            UpdateInput(input, key, joystick.buttons[i] ? EInputAction::IACT_Press : EInputAction::IACT_Release, 1.0, 0,
                        0, hwnd,
                        userIndex);
            inputs->EmplaceBack(*input);
          }
        }
      }
      for (int i = 0; i < joystick.axesNew.size(); ++i) {
        if (joystick.axes[i] != joystick.axesNew[i]) {
          joystick.axes[i] = joystick.axesNew[i];
          auto key = GetKeyForAxis(i);
          if (key != RED4ext::EInputKey::IK_None) {
            auto input = new Input();
            UpdateInput(input, key, EInputAction::IACT_Axis, (joystick.axes[i] - 0.5) * 2.0 * GetInversionForAxis(i), 0,
                        0, hwnd,
                        userIndex);
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
    return input;
  };

  RED4ext::CName *__fastcall GetCName(RED4ext::CName *cname) { 
    *cname = RED4ext::CName("xpad");
    return cname;
  }

  EControllerType __fastcall GetType() { 
    return EControllerType::Xbox;
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

//// 48 83 EC 48 33 C0 48 C7 44 24 20 01 00 06 00 48
//bool SetupRawInputDevices();
//constexpr uintptr_t SetupRawInputDevicesAddr = 0x7988F0;
//decltype(&SetupRawInputDevices) SetupRawInputDevices_Original;
//
//RAWINPUTDEVICE deviceList[2];
//
//bool SetupRawInputDevices() {
//
//  deviceList[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
//  deviceList[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
//  deviceList[0].dwFlags = 0;
//  deviceList[0].hwndTarget = 0;
//
//  deviceList[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
//  deviceList[1].usUsage = HID_USAGE_GENERIC_MOUSE;
//  deviceList[1].dwFlags = 0;
//  deviceList[1].hwndTarget = 0;
//
//
//  //deviceList[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
//  //deviceList[2].usUsage = HID_USAGE_GENERIC_GAMEPAD;
//  ////deviceList[2].dwFlags = RIDEV_INPUTSINK;
//  //deviceList[2].dwFlags = 0;
//  //deviceList[2].hwndTarget = 0;
//
//  //deviceList[3].usUsagePage = HID_USAGE_PAGE_GENERIC;
//  //deviceList[3].usUsage = HID_USAGE_GENERIC_JOYSTICK;
//  ////deviceList[3].dwFlags = RIDEV_INPUTSINK;
//  //deviceList[3].dwFlags = 0;
//  //deviceList[3].hwndTarget = 0;
//
//  //UINT deviceCount = sizeof(deviceList) / sizeof(*deviceList);
//  return RegisterRawInputDevices(deviceList, 2, 0x10);
//}


//// 48 89 6C 24 20 57 41 56 41 57 48 81 EC 90 00 00
//__int64 __fastcall ProcessMouseAndKeyboardInputs(RAWINPUT *rawInput, __int64 a2, HWND hwnd);
//constexpr uintptr_t ProcessMouseAndKeyboardInputsAddr = 0x798100;
//decltype(&ProcessMouseAndKeyboardInputs) ProcessMouseAndKeyboardInputs_Original;
//
//__int64 __fastcall ProcessMouseAndKeyboardInputs(RAWINPUT *rawInput, __int64 a2, HWND hwnd) {
//  auto og = ProcessMouseAndKeyboardInputs_Original(rawInput, a2, hwnd);
//  //if (rawInput->header.dwType == RIM_TYPEHID) {
//  //  UINT size = 0;
//  //  GetRawInputDeviceInfo(rawInput->header.hDevice, RIDI_PREPARSEDDATA, 0, &size);
//  //  _HIDP_PREPARSED_DATA *data = (_HIDP_PREPARSED_DATA *)malloc(size);
//  //  bool gotPreparsedData = GetRawInputDeviceInfo(rawInput->header.hDevice, RIDI_PREPARSEDDATA, data, &size) > 0;
//  //  if (gotPreparsedData) {
//  //    HIDP_CAPS caps;
//  //    HidP_GetCaps(data, &caps);
//
//  //    HIDP_VALUE_CAPS *valueCaps = (HIDP_VALUE_CAPS *)malloc(caps.NumberInputValueCaps * sizeof(HIDP_VALUE_CAPS));
//  //    HidP_GetValueCaps(HidP_Input, valueCaps, &caps.NumberInputValueCaps, data);
//  //    for (USHORT i = 0; i < caps.NumberInputValueCaps && i < 8; ++i) {
//  //      HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, valueCaps[i].Range.UsageMin, &capValues[i], data,
//  //                         (PCHAR)rawInput->data.hid.bRawData, rawInput->data.hid.dwSizeHid);
//  //    }
//  //    free(valueCaps);
//
//  //    HIDP_BUTTON_CAPS *buttonCaps = (HIDP_BUTTON_CAPS *)malloc(caps.NumberInputButtonCaps * sizeof(HIDP_BUTTON_CAPS));
//  //    HidP_GetButtonCaps(HidP_Input, buttonCaps, &caps.NumberInputButtonCaps, data);
//  //    for (USHORT i = 0; i < caps.NumberInputButtonCaps; ++i) {
//  //      ULONG usageCount = buttonCaps->Range.UsageMax - buttonCaps->Range.UsageMin + 1;
//  //      USAGE *usages = (USAGE *)malloc(sizeof(USAGE) * usageCount);
//  //      HidP_GetUsages(HidP_Input, buttonCaps[i].UsagePage, 0, usages, &usageCount, data,
//  //                     (PCHAR)rawInput->data.hid.bRawData, rawInput->data.hid.dwSizeHid);
//  //      for (ULONG usageIndex = 0; usageIndex < usageCount && usageIndex < 0x80; ++usageIndex) {
//  //        usageValues[usageIndex] = usages[usageIndex];
//  //      }
//  //      free(usages);
//  //    }
//  //    free(buttonCaps);
//  //  }
//  //  free(data);
//  //}
//  return og;
//}

struct CustomInputModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(InitializeXPadAddr), &InitializeXPad,
                                  reinterpret_cast<void **>(&InitializeXPad_Original)))
      ;
    //while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(SetupRawInputDevicesAddr), &SetupRawInputDevices,
    //                              reinterpret_cast<void **>(&SetupRawInputDevices_Original)))
    //  ;
    //while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessMouseAndKeyboardInputsAddr),
    //                              &ProcessMouseAndKeyboardInputs,
    //                              reinterpret_cast<void **>(&ProcessMouseAndKeyboardInputs_Original)))
    //  ;
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(InitializeXPadAddr));
    //aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(SetupRawInputDevicesAddr));
    //aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessMouseAndKeyboardInputsAddr));
  }
};

REGISTER_FLIGHT_MODULE(CustomInputModule);