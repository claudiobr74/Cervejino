#pragma once
// PROVISIONAL ESP32 DevKit / WROOM-32. Verify the actual board before wiring.
namespace board {
constexpr int heater1=25, pump=27, buzzer=26;
constexpr int oneWire=4, sda=21, scl=22;
constexpr int buttons[4]={16,17,18,19};
constexpr int emergency=32, level=33, flow=23;
// Drivers must be OFF with inputs floating/reset; active HIGH only in this profile.
constexpr bool activeHigh=true;
}
