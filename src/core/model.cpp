#include "cervejino/model.h"
#include <algorithm>
namespace brew {
static bool range(float x,float a,float b){return std::isfinite(x)&&x>=a&&x<=b;}
static bool goodStep(const Step& s){return range(s.target,1,100)&&range(s.cap,0,100)&&range(s.ramp,0,10)&&s.seconds>0&&s.seconds<=86400&&s.pump.onSec>=1&&s.pump.onSec<=86400&&s.pump.offSec<=86400;}
bool valid(const Recipe& r){
  if(r.name.empty()||r.name.size()>24||r.description.size()>240||r.steps.empty()||r.steps.size()>10||r.additions.size()>6) return false;
  if(!range(r.liters,1,1000)||!range(r.strike,1,95)||!range(r.boilReference,80,104)||!range(r.boilPower,0,100)||!range(r.coolTarget,1,90)||!range(r.whirlpoolMax,1,100)||r.boilSec<1||r.boilSec>86400||r.whirlpoolSec>86400) return false;
  for(const auto& s:r.steps) if(!goodStep(s)) return false;
  for(const auto& a:r.additions) if(a.name.empty()||a.name.size()>24||a.remainingSec>r.boilSec) return false;
  return goodStep(r.mashoutStep);
}
bool valid(const Config& c){
  return range(c.watts[0],1,20000)&&c.enabled[0]&&c.windowMs>=1000&&c.windowMs<=10000&&
    range(c.kp,0,100)&&range(c.ki,0,5)&&range(c.tolerance,0.1f,5)&&c.stableSec>=1&&c.stableSec<=600&&
    c.heatTimeoutSec>=60&&c.heatTimeoutSec<=86400&&range(c.maxTemp,80,115)&&range(c.pumpMax,20,105)&&
    range(c.maxRate,0.1f,10)&&c.staleMs>=1000&&c.staleMs<=10000&&c.pumpMinMs>=1000&&c.pumpMinMs<=60000&&
    c.primeCycles<=10&&c.primeOnSec>=1&&c.primeOnSec<=60&&c.primeOffSec>=1&&c.primeOffSec<=60&&
    (!c.primeCycles||(c.primeOnSec*1000>=c.pumpMinMs&&c.primeOffSec*1000>=c.pumpMinMs))&&
    c.flowGraceMs>=1000&&c.flowGraceMs<=30000&&range(c.offset[0],-10,10)&&
    c.lcdAddress>=0x08&&c.lcdAddress<=0x77&&c.boilStableSec>=10&&c.boilStableSec<=600&&range(c.boilPlateauBand,0.1,2);
}
const char* label(State s){
  static const char* names[]={"Pronto","Conferir agua","Escorva","Aquecendo graos","Adicionar graos","Mostura","Mash-out","Retirar cesto","Aquecer fervura","Confirmar fervura","Fervura","Whirlpool","Resfriando","Finalizado","Pausado","Cancelado","FALHA","Recuperar sessao","Manual","Graos colocados?","Mostura concluida","Iniciar fervura?","Fervura concluida"};
  unsigned i=static_cast<unsigned>(s);return i<23?names[i]:"Estado invalido";
}
const char* label(Fault f){
  static const char* names[]={"Sem falha","Configuracao","Sensor T","Sobretemperatura","LCD/I2C","Emergencia","Nivel insuficiente","Sem fluxo","Tempo aquecimento","Memoria/integridade","Controle atrasado"};
  unsigned i=static_cast<unsigned>(f);return i<12?names[i]:"Falha invalida";
}
Power allocate(float percent,const Config& c){
  Power p;if(!valid(c)||!std::isfinite(percent))return p;
  p.duty[0]=clamp(percent,0,100)/100.f;p.requestedWatts=p.allowedWatts=p.duty[0]*c.watts[0];return p;
}
std::array<bool,1> pulse(const Power& p,Ms now,uint32_t window){
  if(!window)return {false};
  return {now%window<static_cast<uint32_t>(clamp(p.duty[0],0,1)*window)};
}
Sample SensorFilter::update(float raw,bool ok,Ms now,const Config& c,unsigned index){
  if(index>0)return {};
  raw+=c.offset[index];
  bool plausible=ok&&range(raw,-10,120);
  if(plausible&&sample_.valid&&now>sample_.at) plausible=std::fabs(raw-sample_.raw)<=c.maxRate*((now-sample_.at)/1000.f)+0.25f;
  if(!plausible){sample_.valid=false;return sample_;}
  float filtered=sample_.valid?sample_.filtered+0.35f*(raw-sample_.filtered):raw;
  sample_={raw,filtered,now,true};return sample_;
}
void Controller::log(const std::string& e){if(events.size()>=32)events.erase(events.begin());events.push_back(e);}
bool Controller::idle()const{return state==State::Ready||state==State::Complete||state==State::Cancelled;}
bool Controller::active()const{return !idle()&&state!=State::Fault&&state!=State::Recovery;}
void Controller::allOff(){output={};if(pump_)pumpChanged_=last_;pump_=false;integral_=0;requestPercent=0;}
void Controller::trip(Fault f){if(state!=State::Fault||fault!=f){log(std::string("Falha: ")+label(f));revision++;}fault=f;state=State::Fault;allOff();}
void Controller::enter(State s){state=s;phaseTime_=0;heatWait_=0;effective=0;elapsed=0;stableTime_=0;stable_=false;integral_=0;plateauTime_=0;rampTarget_=inputs.t[0].filtered;revision++;log(label(s));}
bool Controller::sensorsReady(Ms now)const{for(const auto& t:inputs.t)if(!t.valid||now<t.at||now-t.at>config.staleMs||!std::isfinite(t.raw)||!std::isfinite(t.filtered))return false;return true;}
const Step* Controller::currentStep()const{if(state==State::Mash&&stepIndex<recipe.steps.size())return &recipe.steps[stepIndex];if(state==State::MashOut)return &recipe.mashoutStep;return nullptr;}
Ms Controller::remaining()const{
  Ms duration=0;const Step* s=currentStep();
  if(s)duration=Ms(s->seconds)*1000;else if(state==State::Boil)duration=Ms(recipe.boilSec)*1000;else if(state==State::Whirlpool)duration=Ms(recipe.whirlpoolSec)*1000;else if(state==State::Manual)duration=Ms(manual.seconds)*1000;
  return duration>effective?duration-effective:0;
}
void Controller::advance(){
  switch(state){
    case State::Strike:enter(State::AddGrain);break;
    case State::Mash:if(++stepIndex<recipe.steps.size())enter(State::Mash);else enter(recipe.mashout?State::MashOut:State::MashDone);break;
    case State::MashOut:enter(State::MashDone);break;
    case State::Boil:enter(State::EndBoil);break;
    case State::Whirlpool:enter(State::Cooling);break;
    default:break;
  }allOff();
}
float Controller::thermal(float set,float cap,float dt){
  float e=set-inputs.t[0].filtered;
  float trial=integral_+config.ki*e*dt;
  float u=config.kp*e+trial;
  if((u>=0&&u<=cap)||(u>cap&&e<0)||(u<0&&e>0))integral_=trial;
  return clamp(config.kp*e+integral_,0,cap);
}
bool Controller::command(Command cmd,Ms now,bool confirmed){
  if(cmd==Command::Cancel){if(!confirmed)return false;enter(State::Cancelled);allOff();return true;}
  if(cmd==Command::Pause&&active()&&state!=State::Paused){previous_=state;state=State::Paused;allOff();revision++;log("Pausa do usuario");return true;}
  if(cmd==Command::AckAddition&&confirmed){acknowledged=fired;revision++;log("Adicoes reconhecidas");return true;}
  if(cmd==Command::Discard&&state==State::Recovery&&confirmed){enter(State::Cancelled);allOff();return true;}
  if(cmd==Command::Ack&&state==State::Fault&&confirmed){fault=Fault::None;enter(State::Ready);allOff();return true;}
  if(!valid(config)||!valid(recipe)||(!simulation_&&!config.commissioned)||!sensorsReady(now)||!inputs.lcdOk||!inputs.emergencyOk||(config.requireLevel&&!inputs.levelOk))return false;
  if(inputs.t[0].raw>=config.maxTemp)return false;
  if(cmd==Command::Start&&idle()&&confirmed){stepIndex=0;fired=acknowledged=0;enter(State::Precheck);return true;}
  if(cmd==Command::ManualStart&&idle()&&confirmed&&range(manual.target,1,100)&&range(manual.power,0,100)&&manual.seconds>=1&&manual.seconds<=86400){enter(State::Manual);return true;}
  if(cmd==Command::ManualStop&&state==State::Manual){enter(State::Complete);allOff();return true;}
  if(cmd==Command::Resume&&state==State::Paused&&confirmed){state=previous_;stable_=false;stableTime_=0;integral_=0;revision++;log("Retomada confirmada");return true;}
  if(cmd==Command::Recover&&state==State::Recovery&&confirmed){state=previous_;stable_=false;stableTime_=0;phaseTime_=0;integral_=0;plateauTime_=0;rampTarget_=inputs.t[0].filtered;revision++;log("Recuperacao confirmada");return true;}
  if(cmd==Command::Confirm&&confirmed){
    if(state==State::Precheck){enter(config.primeCycles?State::Prime:State::Strike);return true;}
    if(state==State::AddGrain){enter(State::ConfirmGrain);return true;}
    if(state==State::ConfirmGrain){stepIndex=0;enter(State::Mash);return true;}
    if(state==State::MashDone){enter(State::RemoveBasket);return true;}
    if(state==State::EndBoil){enter(recipe.whirlpoolSec?State::Whirlpool:State::Cooling);return true;}
    if(state==State::RemoveBasket){enter(State::ReadyBoil);return true;}
    if(state==State::ReadyBoil){enter(State::HeatBoil);return true;}
    if(state==State::HeatBoil||state==State::ConfirmBoil){enter(State::Boil);return true;}
  }
  if(cmd==Command::Skip&&confirmed&&(state==State::Mash||state==State::MashOut||state==State::Boil||state==State::Whirlpool)){log("Etapa pulada");advance();return true;}
  return false;
}
void Controller::tick(Ms now,const Inputs& in){
  inputs=in;Ms dt=clockSet_?now-last_:0;last_=now;clockSet_=true;
  if(active()&&dt>1000){trip(Fault::ControlStall);return;}
  if(!valid(config)||!valid(recipe)){trip(Fault::Config);return;}
  if(!in.emergencyOk){trip(Fault::Emergency);return;}
  if(!in.lcdOk){trip(Fault::Display);return;}
  for(unsigned i=0;i<1;i++)if(in.t[i].valid&&in.t[i].raw>=config.maxTemp){trip(Fault::OverTemp);return;}
  if(!active()){allOff();return;}
  for(unsigned i=0;i<1;i++)if(!in.t[i].valid||now<in.t[i].at||now-in.t[i].at>config.staleMs||!std::isfinite(in.t[i].raw)||!std::isfinite(in.t[i].filtered)){trip(Fault::Sensor1);return;}
  if(config.requireLevel&&!in.levelOk){trip(Fault::Level);return;}
  if(state==State::Paused){allOff();return;}
  phaseTime_+=dt;elapsed+=dt;target=0;requestPercent=0;bool wantPump=false,heatNeedsPump=false;
  const Step* s=currentStep();
  if(s){
    target=s->target;
    float increment=s->ramp*(dt/60000.f);
    if(s->ramp<=0)rampTarget_=target;else if(rampTarget_<target)rampTarget_=std::min(target,rampTarget_+increment);else rampTarget_=std::max(target,rampTarget_-increment);
    uint64_t cycle=Ms(s->pump.onSec+s->pump.offSec)*1000;
    wantPump=s->pump.enabled&&(phaseTime_%cycle<Ms(s->pump.onSec)*1000);
    heatNeedsPump=!config.heatWithoutPump;
    bool band=std::fabs(in.t[0].filtered-target)<=config.tolerance&&std::fabs(rampTarget_-target)<0.01f;
    if(band){stableTime_+=dt;if(stableTime_>=Ms(config.stableSec)*1000)stable_=true;}else{stableTime_=0;if(!config.countOutOfBand)stable_=false;}
    if(stable_&&(band||config.countOutOfBand))effective+=dt;
    if(effective>=Ms(s->seconds)*1000){advance();return;}
    requestPercent=thermal(rampTarget_,s->cap,dt/1000.f);
    if(!stable_)heatWait_+=dt;else heatWait_=0;
    if(heatWait_>Ms(config.heatTimeoutSec)*1000){trip(Fault::HeatTimeout);return;}
  }else switch(state){
    case State::Prime:{Ms cycle=Ms(config.primeOnSec+config.primeOffSec)*1000;wantPump=phaseTime_%cycle<Ms(config.primeOnSec)*1000;if(phaseTime_>=cycle*config.primeCycles){enter(State::Strike);allOff();return;}break;}
    case State::Strike:target=recipe.strike;wantPump=true;heatNeedsPump=!config.heatWithoutPump;requestPercent=thermal(target,100,dt/1000.f);if(in.t[0].filtered>=target-config.tolerance){advance();return;}break;
    case State::HeatBoil:
      target=recipe.boilReference;requestPercent=100;wantPump=true;
      if(config.autoBoil&&in.t[0].filtered>=target-config.tolerance){
        if(!plateauTime_)plateauMin_=plateauMax_=in.t[0].filtered;
        plateauMin_=std::min(plateauMin_,in.t[0].filtered);plateauMax_=std::max(plateauMax_,in.t[0].filtered);
        if(plateauMax_-plateauMin_>config.boilPlateauBand)plateauTime_=0;else plateauTime_+=dt;
        if(plateauTime_>=Ms(config.boilStableSec)*1000){enter(State::Boil);allOff();return;}
      }else plateauTime_=0;
      break;
    case State::Boil:
      target=recipe.boilReference;requestPercent=recipe.boilPower;effective+=dt;
      for(unsigned i=0;i<recipe.additions.size();i++)if(!(fired&(1u<<i))&&remaining()<=Ms(recipe.additions[i].remainingSec)*1000){fired|=1u<<i;revision++;log("Adicionar: "+recipe.additions[i].name);}
      if(effective>=Ms(recipe.boilSec)*1000){advance();return;}break;
    case State::Whirlpool:wantPump=in.t[0].raw<=recipe.whirlpoolMax;if(wantPump&&pump_)effective+=dt;if(effective>=Ms(recipe.whirlpoolSec)*1000){advance();return;}break;
    case State::Cooling:if(in.t[0].filtered<=recipe.coolTarget){enter(State::Complete);allOff();return;}break;
    case State::Manual:effective+=dt;target=manual.target;wantPump=manual.pump;heatNeedsPump=!config.heatWithoutPump;requestPercent=manual.heating?(manual.byPower?manual.power:thermal(target,100,dt/1000.f)):0;if(effective>=Ms(manual.seconds)*1000){enter(State::Complete);allOff();return;}break;
    default:break;
  }
  if((state==State::Strike||state==State::HeatBoil)&&phaseTime_>Ms(config.heatTimeoutSec)*1000){trip(Fault::HeatTimeout);return;}
  bool pumpSafe=in.t[0].raw<config.pumpMax;
  if(!pumpSafe)wantPump=false;
  // OFF is always immediate; minimum off-time controls the next start.
  if(!wantPump&&pump_){pump_=false;pumpChanged_=now;}
  if(wantPump&&!pump_&&now-pumpChanged_>=config.pumpMinMs){pump_=true;pumpChanged_=now;}
  output.pump=pump_;
  if(config.requireFlow&&pump_&&!in.flowOk){noFlow_+=dt;if(noFlow_>=config.flowGraceMs){trip(Fault::Flow);return;}}else noFlow_=0;
  bool permitHeat=(!heatNeedsPump||(wantPump&&pump_))&&(!config.requireFlow||in.flowOk)&& (simulation_||config.commissioned);
  if(!permitHeat){requestPercent=0;integral_=0;}
  output.power=allocate(requestPercent,config);auto pins=pulse(output.power,now,config.windowMs);
  output.heater[0]=pins[0];
}
Checkpoint Controller::checkpoint()const{return {1,state,previous_,stepIndex,effective,elapsed,fired,acknowledged,recipe};}
bool Controller::restore(const Checkpoint& p){
  if(p.schema!=1||!valid(p.recipe)||p.step>p.recipe.steps.size()||static_cast<unsigned>(p.state)>22||static_cast<unsigned>(p.resumeState)>22||p.effective>86400000ULL||p.elapsed>604800000ULL||p.fired>0x3f||p.acknowledged>0x3f||(p.acknowledged&~p.fired))return false;
  if(p.state==State::Ready||p.state==State::Complete||p.state==State::Cancelled)return true;
  State resume=p.state==State::Paused?p.resumeState:p.state;
  if(resume==State::Fault||resume==State::Recovery||resume==State::Manual||resume==State::Paused){enter(State::Cancelled);return true;}
  if(resume==State::Mash&&p.step>=p.recipe.steps.size())return false;
  recipe=p.recipe;stepIndex=p.step;effective=p.effective;elapsed=p.elapsed;fired=p.fired;acknowledged=p.acknowledged;previous_=resume;state=State::Recovery;allOff();revision++;return true;
}
}
