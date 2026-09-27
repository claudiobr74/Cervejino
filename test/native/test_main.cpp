#include "cervejino/model.h"
#include "cervejino/ui.h"
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace brew;
#define CHECK(x) do{if(!(x))throw std::runtime_error(std::string(#x)+" line "+std::to_string(__LINE__));}while(0)
static Inputs good(Ms now,float temp=25){Inputs in;in.t[0]={temp,temp,now,true};return in;}
static void run(Controller& c,Ms& n,Ms duration,float t){for(Ms i=0;i<duration;i+=100){n+=100;c.tick(n,good(n,t));}}
static Controller manual(Ms& n){Controller c(true);c.config.heatWithoutPump=true;c.manual.byPower=true;c.manual.power=100;c.manual.seconds=86400;c.tick(n,good(n));CHECK(c.command(Command::ManualStart,n,true));return c;}
int main(){
  std::vector<std::pair<const char*,std::function<void()>>> tests={
    {"single hardware profile",[]{Config c;CHECK(sizeof(c.watts)/sizeof(float)==1);CHECK(valid(c));CHECK(valid(Recipe{}));}},
    {"invalid config and NaN",[]{Config c;c.kp=std::numeric_limits<float>::quiet_NaN();CHECK(!valid(c));CHECK(allocate(100,c).duty[0]==0);}},
    {"zero and full duty",[]{Config c;for(Ms n=0;n<6000;n++) {CHECK(!pulse(allocate(0,c),n,2000)[0]);CHECK(pulse(allocate(100,c),n,2000)[0]);}}},
    {"PWM intermediate integral",[]{Config c;for(int pct=0;pct<=100;pct++){unsigned on=0;auto p=allocate(pct,c);for(Ms n=0;n<2000;n++)on+=pulse(p,n,2000)[0];CHECK(std::abs(int(on)-pct*20)<=1);}}},
    {"PWM long monotonic time",[]{Config c;CHECK(pulse(allocate(50,c),0,2000)==pulse(allocate(50,c),Ms(1)<<40,2000) || ((Ms(1)<<40)%2000)>=1000);}},
    {"infinite power rejected",[]{Config c;CHECK(allocate(INFINITY,c).duty[0]==0);CHECK(allocate(-1,c).duty[0]==0);CHECK(allocate(200,c).duty[0]==1);}},
    {"single sensor sufficient",[]{Ms n=10000;auto c=manual(n);run(c,n,100,25);CHECK(c.state==State::Manual);CHECK(c.output.power.duty[0]==1);}},
    {"uncommissioned physical start blocked",[]{Controller c(false);c.tick(10000,good(10000));CHECK(!c.command(Command::Start,10000,true));}},
    {"start requires confirmation",[]{Controller c(true);c.tick(10000,good(10000));CHECK(!c.command(Command::Start,10000,false));}},
    {"sensor invalid shuts immediately",[]{Ms n=10000;auto c=manual(n);run(c,n,100,25);auto in=good(n+1);in.t[0].valid=false;c.tick(n+1,in);CHECK(c.fault==Fault::Sensor1);CHECK(!c.output.heater[0]&&!c.output.pump);}},
    {"stale sensor shuts immediately",[]{Ms n=10000;auto c=manual(n);auto in=good(0);c.tick(++n,in);CHECK(c.fault==Fault::Sensor1);}},
    {"raw overtemperature bypasses filter",[]{Ms n=10000;auto c=manual(n);auto in=good(n+1,25);in.t[0].raw=106;c.tick(++n,in);CHECK(c.fault==Fault::OverTemp);CHECK(!c.output.heater[0]);}},
    {"display loss stops outputs",[]{Ms n=10000;auto c=manual(n);auto in=good(++n);in.lcdOk=false;c.tick(n,in);CHECK(c.fault==Fault::Display);CHECK(!c.output.heater[0]);}},
    {"emergency opens",[]{Ms n=10000;auto c=manual(n);auto in=good(++n);in.emergencyOk=false;c.tick(n,in);CHECK(c.fault==Fault::Emergency);}},
    {"level opens",[]{Ms n=10000;auto c=manual(n);c.config.requireLevel=true;auto in=good(++n);in.levelOk=false;c.tick(n,in);CHECK(c.fault==Fault::Level);}},
    {"flow unproven inhibits heat",[]{Ms n=10000;auto c=manual(n);c.config.requireFlow=true;c.manual.pump=true;for(int i=0;i<70;i++){n+=100;auto in=good(n);in.flowOk=false;c.tick(n,in);CHECK(!c.output.heater[0]);}CHECK(c.fault==Fault::Flow);}},
    {"pause freezes and off",[]{Ms n=10000;auto c=manual(n);run(c,n,1000,25);auto elapsed=c.effective;CHECK(c.command(Command::Pause,n));CHECK(!c.output.heater[0]&&!c.output.pump);run(c,n,2000,25);CHECK(c.effective==elapsed);CHECK(!c.command(Command::Resume,n,false));CHECK(c.command(Command::Resume,n,true));run(c,n,1000,25);CHECK(c.effective==elapsed+1000);}},
    {"cancel confirmation",[]{Ms n=10000;auto c=manual(n);CHECK(!c.command(Command::Cancel,n,false));CHECK(c.command(Command::Cancel,n,true));CHECK(c.state==State::Cancelled);CHECK(!c.output.heater[0]);}},
    {"ack fault cannot auto resume",[]{Ms n=10000;auto c=manual(n);c.trip(Fault::Sensor1);CHECK(c.command(Command::Ack,n,true));CHECK(c.state==State::Ready);CHECK(!c.output.heater[0]);}},
    {"pump temperature limit",[]{Ms n=10000;auto c=manual(n);c.manual.pump=true;run(c,n,100,81);CHECK(!c.output.pump);}},
    {"pump off dwell after pause",[]{Ms n=10000;auto c=manual(n);c.manual.pump=true;run(c,n,100,25);CHECK(c.output.pump);CHECK(c.command(Command::Pause,n));n+=100;c.tick(n,good(n));CHECK(c.command(Command::Resume,n,true));run(c,n,100,25);CHECK(!c.output.pump);run(c,n,3000,25);CHECK(c.output.pump);}},
    {"85 degrees after real conversion valid",[]{SensorFilter s;Config c;auto t=s.update(85,true,1000,c,0);CHECK(t.valid&&t.raw==85);}},
    {"invalid conversion not accepted",[]{SensorFilter s;Config c;CHECK(!s.update(85,false,1000,c,0).valid);}},
    {"rate jump rejected",[]{SensorFilter s;Config c;s.update(25,true,1000,c,0);CHECK(!s.update(60,true,1800,c,0).valid);}},
    {"offset individual",[]{SensorFilter s;Config c;c.offset[0]=1.5;CHECK(s.update(25,true,1000,c,0).raw==26.5);}},
    {"hold waits for stability",[]{Controller c(true);Ms n=10000;c.recipe.steps[0].seconds=30;c.config.stableSec=2;c.state=State::Mash;c.tick(n,good(n,60));run(c,n,3000,60);CHECK(c.effective==0);run(c,n,1000,65);CHECK(c.effective==0);run(c,n,2000,65);CHECK(c.effective>0);auto e=c.effective;run(c,n,1000,62);CHECK(c.effective==e);}},
    {"pump pause inhibits heater",[]{Controller c(true);Ms n=10000;c.state=State::Mash;c.recipe.steps[0].pump={true,3,3};c.tick(n,good(n,50));run(c,n,2000,50);CHECK(c.output.pump);run(c,n,1500,50);CHECK(!c.output.pump&&!c.output.heater[0]);}},
    {"ramp limits setpoint",[]{Controller c(true);Ms n=10000;c.recipe.steps[0].ramp=1;c.tick(n,good(n,25));c.state=State::AddGrain;CHECK(c.command(Command::Confirm,n,true));CHECK(c.command(Command::Confirm,n,true));run(c,n,1000,25);CHECK(c.output.power.duty[0]<0.01f);}},
    {"double malt confirmation",[]{Controller c(true);Ms n=10000;c.tick(n,good(n));c.state=State::AddGrain;CHECK(c.command(Command::Confirm,n,true));CHECK(c.state==State::ConfirmGrain);CHECK(!c.output.heater[0]);CHECK(c.command(Command::Confirm,n,true));CHECK(c.state==State::Mash);}},
    {"boil plateau starts timer",[]{Controller c(true);Ms n=10000;c.config.boilStableSec=10;c.state=State::HeatBoil;c.tick(n,good(n,98));run(c,n,9900,98);CHECK(c.state==State::HeatBoil);run(c,n,200,98);CHECK(c.state==State::Boil);}},
    {"rising temperature not boiling plateau",[]{Controller c(true);Ms n=10000;c.config.boilStableSec=10;c.state=State::HeatBoil;c.tick(n,good(n,97));for(int i=1;i<=150;i++){n+=100;c.tick(n,good(n,97+i*0.02f));}CHECK(c.state==State::HeatBoil);}},
    {"boil events unique across pause",[]{Controller c(true);Ms n=10000;c.recipe.boilSec=10;c.recipe.additions={{"A",9},{"B",0}};c.state=State::Boil;c.tick(n,good(n,98));run(c,n,1500,98);CHECK(c.fired==1);c.command(Command::AckAddition,n,true);c.command(Command::Pause,n);run(c,n,1000,98);c.command(Command::Resume,n,true);run(c,n,9000,98);CHECK(c.fired==3&&c.acknowledged==1);CHECK(c.state==State::EndBoil);CHECK(!c.output.heater[0]);}},
    {"recovery never starts outputs",[]{Controller original(true);Ms n=10000;original.state=State::Boil;original.fired=1;original.recipe.additions={{"A",10}};original.tick(n,good(n,98));run(original,n,3000,98);Controller recovered(true);CHECK(recovered.restore(original.checkpoint()));CHECK(recovered.state==State::Recovery);CHECK(!recovered.output.heater[0]);recovered.tick(n,good(n,98));CHECK(recovered.command(Command::Recover,n,true));CHECK(recovered.fired==1);CHECK(recovered.effective==3000);}},
    {"corrupt checkpoint rejects",[]{Controller c(true);Checkpoint p;p.state=State::Mash;p.step=99;CHECK(!c.restore(p));p.step=0;p.acknowledged=4;CHECK(!c.restore(p));}},
    {"control stall fails closed",[]{Ms n=10000;auto c=manual(n);n+=2000;c.tick(n,good(n));CHECK(c.fault==Fault::ControlStall);}},
    {"heating timeout",[]{Controller c(true);Ms n=10000;c.config.heatTimeoutSec=60;c.state=State::Strike;c.tick(n,good(n));run(c,n,61000,25);CHECK(c.fault==Fault::HeatTimeout);}},
    {"button boot held cannot confirm",[]{Button b;CHECK(b.update(true,0,false)==Press::None);CHECK(b.update(true,2000,false)==Press::None);CHECK(b.update(false,2100,false)==Press::None);CHECK(b.update(false,2200,false)==Press::None);CHECK(b.update(true,2300,false)==Press::None);CHECK(b.update(true,2400,false)==Press::Short);CHECK(b.update(true,2500,false)==Press::None);}},
    {"button bounce ignored",[]{Button b;b.update(false,0,false);b.update(false,100,false);CHECK(b.update(true,110,false)==Press::None);CHECK(b.update(false,120,false)==Press::None);CHECK(b.update(true,125,false)==Press::None);CHECK(b.update(true,170,false)==Press::Short);}},
    {"repeat only edit keys",[]{Button b;b.update(false,0,true);b.update(false,100,true);b.update(true,110,true);CHECK(b.update(true,150,true)==Press::None);CHECK(b.update(true,800,true)==Press::Repeat);CHECK(b.update(false,900,true)==Press::None);CHECK(b.update(false,950,true)==Press::None);}},
    {"LCD exactly twenty by four",[]{App a(true);for(int i=0;i<100;i++){auto s=a.screen(10000);CHECK(s.size()==4);for(auto& row:s)CHECK(row.size()==20);a.key(Key::Down,Press::Short,10000);}}},
    {"UI fresh timestamp on confirmation",[]{App a(true);a.controller.tick(10000,good(10000));a.key(Key::Ok,Press::Short,10000);a.controller.tick(10500,good(10500));a.key(Key::Ok,Press::Short,10500);CHECK(a.controller.state==State::Precheck);}},
    {"local recipe creation without network",[]{App a(true);a.key(Key::Down,Press::Short,0);a.key(Key::Ok,Press::Short,0);a.key(Key::Down,Press::Short,0);a.key(Key::Ok,Press::Short,0);for(int i=0;i<80;i++)a.key(Key::Down,Press::Short,0);a.key(Key::Ok,Press::Short,0);a.key(Key::Ok,Press::Short,0);CHECK(a.recipes.size()==2);CHECK(a.saveRequested);}},
    {"six hop additions limit",[]{Recipe r;r.additions.resize(7);CHECK(!valid(r));r.additions.resize(6);CHECK(valid(r));}},
    {"complete brew sequence",[]{Controller c(true);Ms n=10000;c.config.primeCycles=0;c.config.stableSec=1;c.recipe.strike=30;c.recipe.steps={{32,70,0,2,{true,600,30}}};c.recipe.mashout=false;c.recipe.boilSec=2;c.recipe.additions.clear();c.recipe.coolTarget=25;c.tick(n,good(n));CHECK(c.command(Command::Start,n,true));CHECK(c.command(Command::Confirm,n,true));run(c,n,100,30);CHECK(c.state==State::AddGrain);c.command(Command::Confirm,n,true);c.command(Command::Confirm,n,true);run(c,n,3100,32);CHECK(c.state==State::MashDone);c.command(Command::Confirm,n,true);CHECK(c.state==State::RemoveBasket);c.command(Command::Confirm,n,true);CHECK(c.state==State::ReadyBoil);c.command(Command::Confirm,n,true);CHECK(c.state==State::HeatBoil);c.command(Command::Confirm,n,true);run(c,n,2100,98);CHECK(c.state==State::EndBoil);c.command(Command::Confirm,n,true);CHECK(c.state==State::Cooling);run(c,n,100,25);CHECK(c.state==State::Complete);CHECK(!c.output.heater[0]&&!c.output.pump);}}
  };
  unsigned failed=0;for(auto& t:tests){try{t.second();std::cout<<"PASS "<<t.first<<"\n";}catch(const std::exception& e){failed++;std::cerr<<"FAIL "<<t.first<<": "<<e.what()<<"\n";}}
  std::cout<<tests.size()-failed<<"/"<<tests.size()<<" tests passed\n";return failed?1:0;
}
