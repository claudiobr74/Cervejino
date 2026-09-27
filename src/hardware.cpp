#include "cervejino/hardware.h"
#include <algorithm>
namespace hw {
bool Sensors::scratch(const uint8_t* rom,uint8_t* data){
  if(!wire_.reset())return false;wire_.select(rom);wire_.write(0xBE);
  for(unsigned i=0;i<9;i++)data[i]=wire_.read();
  return OneWire::crc8(data,8)==data[8];
}
bool Sensors::bind(brew::Config& config){
  std::vector<std::array<uint8_t,8>> found;uint8_t a[8];wire_.reset_search();
  while(wire_.search(a)){if(a[0]==0x28&&OneWire::crc8(a,7)==a[7]){std::array<uint8_t,8> r;std::copy(a,a+8,r.begin());found.push_back(r);}}
  if(found.size()!=1)return false;
  std::sort(found.begin(),found.end());config.rom[0]=found[0];pending_=false;return true;
}
void Sensors::tick(brew::Ms now,const brew::Config& config,brew::Inputs& in){
  if(addresses_!=config.rom){addresses_=config.rom;pending_=false;for(auto& f:filters_)f=brew::SensorFilter{};}
  if(!pending_){
    startedOk_=true;
    for(unsigned i=0;i<1;i++){
      auto& rom=addresses_[i];uint8_t data[9];
      if(rom[0]!=0x28||OneWire::crc8(rom.data(),7)!=rom[7]||!scratch(rom.data(),data)){startedOk_=false;in.t[i].valid=false;continue;}
      // Volatile scratchpad markers detect a sensor power reset during conversion.
      // Do NOT COPY SCRATCHPAD: EEPROM defaults remain distinguishable.
      if(!wire_.reset()){startedOk_=false;continue;}
      wire_.select(rom.data());wire_.write(0x4E);wire_.write(0x3C);wire_.write(0xC3);wire_.write(0x7F);
    }
    if(!wire_.reset())startedOk_=false;
    wire_.skip();wire_.write(0x44,0);started_=now;pending_=true;return;
  }
  if(now-started_<800)return;
  bool completed=wire_.read_bit()==1;
  for(unsigned i=0;i<1;i++){
    uint8_t d[9]{};bool ok=startedOk_&&completed&&scratch(addresses_[i].data(),d)&&d[2]==0x3C&&d[3]==0xC3&&d[4]==0x7F;
    int16_t raw=int16_t((uint16_t(d[1])<<8)|d[0]);in.t[i]=filters_[i].update(raw/16.f,ok,now,config,i);
  }pending_=false;
}
bool Lcd::nibble(uint8_t data){
  // Common PCF8574 backpack: P0 RS, P1 RW, P2 EN, P3 backlight, P4..7 data.
  // Three bytes in one bounded I2C transaction, no blocking LCD library.
  Wire.beginTransmission(address_);Wire.write(uint8_t(data|8));Wire.write(uint8_t(data|12));Wire.write(uint8_t((data|8)&~4));
  bool ok=Wire.endTransmission()==0;if(!ok)healthy_=false;return ok;
}
bool Lcd::byte(uint8_t data,bool rs){return nibble((data&0xF0)|(rs?1:0))&&nibble((data<<4)|(rs?1:0));}
void Lcd::begin(uint8_t address){address_=address;Wire.begin(board::sda,board::scl,100000);Wire.setTimeOut(5);stage_=0;due_=millis()+50;healthy_=false;initialised_=false;for(auto& r:shown_)r=std::string(20,'\xff');}
void Lcd::tick(brew::Ms now){
  if(now<due_)return;
  if(!initialised_){
    Wire.beginTransmission(address_);if(Wire.endTransmission()!=0){healthy_=false;due_=now+500;return;}
    healthy_=true;bool ok=true;
    if(stage_<3)ok=nibble(0x30);else if(stage_==3)ok=nibble(0x20);else if(stage_==4)ok=byte(0x28,false);else if(stage_==5)ok=byte(0x0C,false);else if(stage_==6)ok=byte(0x06,false);else if(stage_==7)ok=byte(0x01,false);
    if(!ok){stage_=0;due_=now+500;return;}stage_++;due_=now+5;if(stage_>=8)initialised_=true;return;
  }
  if(now-check_>=250){check_=now;Wire.beginTransmission(address_);if(Wire.endTransmission()!=0){healthy_=false;initialised_=false;stage_=0;due_=now+500;return;}healthy_=true;}
  static const uint8_t rowBase[]={0x00,0x40,0x14,0x54};
  for(unsigned i=0;i<80;i++){unsigned index=(cell_+i)%80,row=index/20,col=index%20;if(wanted_[row].size()!=20)continue;if(wanted_[row][col]!=shown_[row][col]){
    if(byte(0x80|(rowBase[row]+col),false)&&byte(wanted_[row][col],true)){shown_[row][col]=wanted_[row][col];cell_=(index+1)%80;}else{initialised_=false;stage_=0;due_=now+500;}break;}}
}
void off(){
#if CERVEJINO_ENABLE_OUTPUTS && !CERVEJINO_SIM
  digitalWrite(board::heater1,LOW);digitalWrite(board::pump,LOW);
#endif
}
void initOutputs(){
#if CERVEJINO_ENABLE_OUTPUTS && !CERVEJINO_SIM
  for(int p:{board::heater1,board::pump}){digitalWrite(p,LOW);pinMode(p,OUTPUT);}
#endif
}
void apply(const brew::Outputs& out){
#if CERVEJINO_ENABLE_OUTPUTS && !CERVEJINO_SIM
  digitalWrite(board::heater1,out.heater[0]);digitalWrite(board::pump,out.pump);
#else
  (void)out;
#endif
}
}
