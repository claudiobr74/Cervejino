#pragma once
#include "ui.h"
#include "board.h"
#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
namespace hw {
class Sensors {
  OneWire wire_{board::oneWire};
  brew::SensorFilter filters_[1];
  brew::Ms started_=0;
  bool pending_=false, startedOk_=false;
  std::array<std::array<uint8_t,8>,1> addresses_{};
  bool scratch(const uint8_t* rom,uint8_t* data);
public:
  bool bind(brew::Config& config);
  void tick(brew::Ms now,const brew::Config& config,brew::Inputs& in);
};
class Lcd {
  uint8_t address_=0x27;
  brew::Screen wanted_{},shown_{};
  bool healthy_=false, initialised_=false;
  unsigned stage_=0,cell_=0;
  brew::Ms due_=0,check_=0;
  bool nibble(uint8_t data);
  bool byte(uint8_t data,bool rs);
public:
  void begin(uint8_t address);
  void set(brew::Screen s){wanted_=brew::fit(s);}
  void tick(brew::Ms now);
  bool ready()const{return initialised_;}
  bool healthy()const{return healthy_&&initialised_;}
  uint8_t address()const{return address_;}
};
void initOutputs();
void apply(const brew::Outputs& out);
void off();
}
