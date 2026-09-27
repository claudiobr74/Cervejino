#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace brew {
using Ms = uint64_t;
inline float clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
struct PumpCycle { bool enabled = true; uint32_t onSec = 600, offSec = 30; };
struct Step {
  float target = 65, cap = 70, ramp = 0;
  uint32_t seconds = 3600;
  PumpCycle pump;
};
struct Addition { std::string name = "Lupulo"; uint32_t remainingSec = 600; };
struct Recipe {
  std::string name = "Receita 01", description = "Demonstracao; ajustar antes de usar";
  float liters = 20, strike = 55, boilReference = 97, boilPower = 65, coolTarget = 25;
  std::vector<Step> steps{Step{}};
  bool mashout = true;
  Step mashoutStep{76, 70, 0, 600, {true, 600, 30}};
  uint32_t boilSec = 3600, whirlpoolSec = 0;
  float whirlpoolMax = 80;
  std::vector<Addition> additions{{"Lupulo", 600}};
};
struct Config {
  float watts[1]{1500};
  bool enabled[1]{true};
  uint32_t windowMs = 2000;
  float kp = 10, ki = 0.03f, tolerance = 0.5f;
  uint32_t stableSec = 15, heatTimeoutSec = 7200;
  bool countOutOfBand = false, heatWithoutPump = false;
  float maxTemp = 105, pumpMax = 80, maxRate = 2;
  bool autoBoil = true, sound = true;
  uint32_t boilStableSec = 60;
  float boilPlateauBand = 0.3f;
  uint32_t staleMs = 2500, pumpMinMs = 3000;
  uint32_t primeOnSec = 3, primeOffSec = 3, primeCycles = 3;
  float offset[1]{0};
  bool requireLevel = false, requireFlow = false;
  uint32_t flowGraceMs = 5000;
  bool commissioned = false;
  uint8_t lcdAddress = 0x27;
  std::array<std::array<uint8_t,8>,1> rom{};
};
struct Sample { float raw = 0, filtered = 0; Ms at = 0; bool valid = false; };
struct Inputs {
  Sample t[1];
  bool lcdOk = true, emergencyOk = true, levelOk = true, flowOk = true;
};
enum class State : uint8_t {
  Ready, Precheck, Prime, Strike, AddGrain, Mash, MashOut, RemoveBasket,
  HeatBoil, ConfirmBoil, Boil, Whirlpool, Cooling, Complete, Paused,
  Cancelled, Fault, Recovery, Manual, ConfirmGrain, MashDone, ReadyBoil, EndBoil
};
enum class Fault : uint8_t { None, Config, Sensor1, OverTemp,
  Display, Emergency, Level, Flow, HeatTimeout, Persistence, ControlStall };
enum class Command : uint8_t { Start, Confirm, Pause, Resume, Cancel, Ack, Skip,
  ManualStart, ManualStop, AckAddition, Recover, Discard };
struct Manual { bool byPower = false, heating = true; float target = 65, power = 0; bool pump = false; uint32_t seconds = 600; };
struct Power { float duty[1]{0}, requestedWatts = 0, allowedWatts = 0; };
struct Outputs { bool heater[1]{false}, pump = false; Power power; };
struct Checkpoint {
  uint32_t schema = 1;
  State state = State::Ready, resumeState = State::Ready;
  uint32_t step = 0;
  Ms effective = 0, elapsed = 0;
  uint32_t fired = 0, acknowledged = 0;
  Recipe recipe;
};
bool valid(const Config& c);
bool valid(const Recipe& r);
const char* label(State s);
const char* label(Fault f);
Power allocate(float percent, const Config& c);
std::array<bool,1> pulse(const Power& p, Ms now, uint32_t window);
class SensorFilter {
  Sample sample_;
public:
  Sample update(float raw, bool conversionValid, Ms now, const Config& c, unsigned index);
  Sample sample() const { return sample_; }
};
class Controller {
public:
  Config config;
  Recipe recipe;
  Manual manual;
  State state = State::Ready;
  Fault fault = Fault::None;
  Outputs output;
  Inputs inputs;
  uint32_t revision = 0, stepIndex = 0, fired = 0, acknowledged = 0;
  Ms effective = 0, elapsed = 0;
  float target = 0, requestPercent = 0;
  std::vector<std::string> events;
  explicit Controller(bool simulation = false) : simulation_(simulation) {}
  void tick(Ms now, const Inputs& in);
  bool command(Command cmd, Ms now, bool confirmed = false);
  void trip(Fault reason);
  bool idle() const;
  bool active() const;
  bool sensorsReady(Ms now) const;
  Checkpoint checkpoint() const;
  bool restore(const Checkpoint& p);
  void log(const std::string& e);
  Ms remaining() const;
  const Step* currentStep() const;
private:
  bool simulation_ = false, pump_ = false, clockSet_ = false, stable_ = false;
  State previous_ = State::Ready;
  Ms last_ = 0, phaseTime_ = 0, stableTime_ = 0, pumpChanged_ = 0, noFlow_ = 0;
  float integral_ = 0, rampTarget_ = 0, plateauMin_ = 0, plateauMax_ = 0;
  Ms plateauTime_ = 0, heatWait_ = 0;
  void enter(State s);
  void advance();
  float thermal(float set, float cap, float dt);
  void allOff();
};
}
