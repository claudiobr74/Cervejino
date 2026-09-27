#include "cervejino/serialization.h"
namespace codec {
using namespace brew;
static void step(JsonObject o,const Step& s){o["target"]=s.target;o["cap"]=s.cap;o["ramp"]=s.ramp;o["seconds"]=s.seconds;o["pump"]=s.pump.enabled;o["onSec"]=s.pump.onSec;o["offSec"]=s.pump.offSec;}
static bool step(JsonObjectConst o,Step& s){
  if(!o["target"].is<float>()||!o["cap"].is<float>()||!o["ramp"].is<float>()||!o["seconds"].is<uint32_t>()||!o["pump"].is<bool>()||!o["onSec"].is<uint32_t>()||!o["offSec"].is<uint32_t>())return false;
  s.target=o["target"];s.cap=o["cap"];s.ramp=o["ramp"];s.seconds=o["seconds"];s.pump.enabled=o["pump"];s.pump.onSec=o["onSec"];s.pump.offSec=o["offSec"];return true;
}
void recipe(JsonObject o,const Recipe& r){
  o["schema"]=schema;o["name"]=r.name;o["description"]=r.description;o["liters"]=r.liters;o["strike"]=r.strike;
  auto a=o.createNestedArray("steps");for(const auto& s:r.steps)step(a.createNestedObject(),s);
  o["mashout"]=r.mashout;step(o.createNestedObject("mashoutStep"),r.mashoutStep);
  o["boilReference"]=r.boilReference;o["boilPower"]=r.boilPower;o["boilSec"]=r.boilSec;o["coolTarget"]=r.coolTarget;o["whirlpoolSec"]=r.whirlpoolSec;o["whirlpoolMax"]=r.whirlpoolMax;
  auto adds=o.createNestedArray("additions");for(const auto& x:r.additions){auto b=adds.createNestedObject();b["name"]=x.name;b["remainingSec"]=x.remainingSec;}
}
bool recipe(JsonObjectConst o,Recipe& r){
  if(o["schema"]!=schema||!o["name"].is<const char*>()||!o["description"].is<const char*>()||!o["steps"].is<JsonArrayConst>()||!o["additions"].is<JsonArrayConst>()||!o["mashout"].is<bool>())return false;
  Recipe candidate;
#define RF(x) if(!o[#x].is<float>())return false;candidate.x=o[#x].as<float>();
  RF(liters) RF(strike) RF(boilReference) RF(boilPower) RF(coolTarget) RF(whirlpoolMax)
#undef RF
  if(!o["boilSec"].is<uint32_t>()||!o["whirlpoolSec"].is<uint32_t>()||o["steps"].size()>10||o["additions"].size()>6)return false;
  candidate.name=o["name"].as<std::string>();candidate.description=o["description"].as<std::string>();candidate.mashout=o["mashout"];candidate.boilSec=o["boilSec"];candidate.whirlpoolSec=o["whirlpoolSec"];
  candidate.steps.clear();for(JsonObjectConst item:o["steps"].as<JsonArrayConst>()){Step s;if(!step(item,s))return false;candidate.steps.push_back(s);}
  if(!step(o["mashoutStep"],candidate.mashoutStep))return false;
  candidate.additions.clear();for(JsonObjectConst item:o["additions"].as<JsonArrayConst>()){if(!item["name"].is<const char*>()||!item["remainingSec"].is<uint32_t>())return false;candidate.additions.push_back({item["name"].as<std::string>(),item["remainingSec"].as<uint32_t>()});}
  if(!valid(candidate))return false;r=std::move(candidate);return true;
}
// Explicit fields: no raw struct/padding persisted and no remote commissioning flag.
#define CONFIG_FLOATS(X) X(kp) X(ki) X(tolerance) X(maxTemp) X(pumpMax) X(maxRate) X(boilPlateauBand)
#define CONFIG_INTS(X) X(windowMs) X(stableSec) X(heatTimeoutSec) X(staleMs) X(pumpMinMs) X(primeOnSec) X(primeOffSec) X(primeCycles) X(flowGraceMs) X(boilStableSec)
#define CONFIG_BOOLS(X) X(countOutOfBand) X(heatWithoutPump) X(requireLevel) X(requireFlow) X(autoBoil) X(sound)
void config(JsonObject o,const Config& c){
  o["schema"]=schema;
#define PUT(x) o[#x]=c.x;
  CONFIG_FLOATS(PUT) CONFIG_INTS(PUT) CONFIG_BOOLS(PUT)
#undef PUT
  o["lcdAddress"]=c.lcdAddress;
  for(unsigned i=0;i<1;i++){auto s=o.createNestedObject("sensor");s["offset"]=c.offset[i];auto a=s.createNestedArray("rom");for(auto b:c.rom[i])a.add(b);auto h=o.createNestedObject("heater");h["watts"]=c.watts[i];h["enabled"]=c.enabled[i];}
}
bool config(JsonObjectConst o,Config& c){
  if(o["schema"]!=schema)return false;Config candidate=c;
#define GETF(x) if(!o[#x].is<float>())return false;candidate.x=o[#x].as<float>();
#define GETI(x) if(!o[#x].is<uint32_t>())return false;candidate.x=o[#x].as<uint32_t>();
#define GETB(x) if(!o[#x].is<bool>())return false;candidate.x=o[#x].as<bool>();
  CONFIG_FLOATS(GETF) CONFIG_INTS(GETI) CONFIG_BOOLS(GETB)
#undef GETF
#undef GETI
#undef GETB
  if(!o["lcdAddress"].is<uint8_t>())return false;
  candidate.lcdAddress=o["lcdAddress"];
  for(unsigned i=0;i<1;i++){auto s=o["sensor"];auto h=o["heater"];
    if(!s["offset"].is<float>()||!s["rom"].is<JsonArrayConst>()||s["rom"].size()!=8||!h["watts"].is<float>()||!h["enabled"].is<bool>())return false;
    candidate.offset[i]=s["offset"];candidate.watts[i]=h["watts"];candidate.enabled[i]=h["enabled"];
    for(unsigned j=0;j<8;j++){if(!s["rom"][j].is<uint8_t>())return false;candidate.rom[i][j]=s["rom"][j];}
  }if(!valid(candidate))return false;c=candidate;return true;
}
void checkpoint(JsonObject o,const Checkpoint& p){o["schema"]=schema;o["state"]=int(p.state);o["resumeState"]=int(p.resumeState);o["step"]=p.step;o["effective"]=p.effective;o["elapsed"]=p.elapsed;o["fired"]=p.fired;o["acknowledged"]=p.acknowledged;recipe(o.createNestedObject("recipe"),p.recipe);}
bool checkpoint(JsonObjectConst o,Checkpoint& p){
  if(o["schema"]!=schema||!o["state"].is<unsigned>()||!o["resumeState"].is<unsigned>()||o["state"].as<unsigned>()>22||o["resumeState"].as<unsigned>()>22||!o["effective"].is<uint64_t>()||!o["elapsed"].is<uint64_t>()||!o["step"].is<uint32_t>()||!o["fired"].is<uint32_t>()||!o["acknowledged"].is<uint32_t>())return false;
  p.state=State(o["state"].as<unsigned>());p.resumeState=State(o["resumeState"].as<unsigned>());p.effective=o["effective"];p.elapsed=o["elapsed"];p.step=o["step"];p.fired=o["fired"];p.acknowledged=o["acknowledged"];return recipe(o["recipe"],p.recipe);
}
String encode(const App& app){DynamicJsonDocument d(40000);d["schema"]=schema;d["selected"]=app.selected;config(d.createNestedObject("config"),app.controller.config);auto a=d.createNestedArray("recipes");for(const auto& r:app.recipes)recipe(a.createNestedObject(),r);checkpoint(d.createNestedObject("checkpoint"),app.controller.checkpoint());String out;if(!d.overflowed())serializeJson(d,out);return out;}
bool decode(const String& text,App& app){
  DynamicJsonDocument d(40000);if(deserializeJson(d,text)||d["schema"]!=schema||!d["selected"].is<unsigned>()||!d["recipes"].is<JsonArray>()||d["recipes"].size()<1||d["recipes"].size()>8)return false;
  Config c;std::vector<Recipe> rs;Checkpoint cp;
  if(!config(d["config"],c)||!checkpoint(d["checkpoint"],cp))return false;
  for(JsonObjectConst item:d["recipes"].as<JsonArrayConst>()){Recipe r;if(!recipe(item,r))return false;rs.push_back(r);}
  unsigned selected=d["selected"];if(selected>=rs.size())return false;
  Controller validation(true);if(!validation.restore(cp))return false;
  app.controller.config=c;app.recipes=std::move(rs);app.selected=selected;return app.controller.restore(cp);
}
String snapshot(const App& app,Ms now){
  DynamicJsonDocument d(45000);auto& c=app.controller;d["revision"]=c.revision;d["state"]=label(c.state);d["fault"]=label(c.fault);d["target"]=c.target;d["remainingMs"]=c.remaining();d["effectiveMs"]=c.effective;d["elapsedMs"]=c.elapsed;d["pump"]=c.output.pump;d["requestedWatts"]=c.output.power.requestedWatts;d["allowedWatts"]=c.output.power.allowedWatts;d["selected"]=app.selected;d["localEditing"]=app.editing();d["message"]=app.message;
  d["outputsEnabled"]=bool(CERVEJINO_ENABLE_OUTPUTS&&!CERVEJINO_SIM);d["simulation"]=bool(CERVEJINO_SIM);d["flowVerified"]=c.config.requireFlow&&c.inputs.flowOk;
  auto t=d.createNestedArray("temperatures");for(auto s:c.inputs.t){auto v=t.createNestedObject();bool fresh=s.valid&&now>=s.at&&now-s.at<=c.config.staleMs;v["valid"]=fresh;if(fresh){v["value"]=s.filtered;v["raw"]=s.raw;}v["ageMs"]=now>=s.at?now-s.at:0;}
  auto h=d.createNestedArray("heaters");for(unsigned i=0;i<1;i++){auto v=h.createNestedObject();v["duty"]=c.output.power.duty[i]*100;v["on"]=c.output.heater[i];}
  auto a=d.createNestedArray("recipes");for(const auto& r:app.recipes)recipe(a.createNestedObject(),r);config(d.createNestedObject("config"),c.config);
  auto logs=d.createNestedArray("events");for(const auto& e:c.events)logs.add(e);
  auto adds=d.createNestedArray("additions");for(unsigned i=0;i<c.recipe.additions.size();i++){auto v=adds.createNestedObject();v["name"]=c.recipe.additions[i].name;v["remainingSec"]=c.recipe.additions[i].remainingSec;v["fired"]=bool(c.fired&(1u<<i));v["acknowledged"]=bool(c.acknowledged&(1u<<i));}
  auto lcd=d.createNestedArray("lcd");for(const auto& row:app.screen(now))lcd.add(row);String out;if(!d.overflowed())serializeJson(d,out);return out;
}
}
