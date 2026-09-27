#include "cervejino/hardware.h"
#include "cervejino/serialization.h"
#include "cervejino/storage.h"
#include "cervejino/web.h"
#include <WiFi.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <atomic>
#include <freertos/queue.h>

using namespace brew;
static App app(CERVEJINO_SIM);
static hw::Sensors sensors;
static hw::Lcd lcd;
static Inputs input;
static Button buttons[4];
static QueueHandle_t saves;
static std::atomic<bool> storageFailed{false},stalled{false};
static std::atomic<bool> storageBusy{false};
static std::atomic<uint32_t> heartbeat{0};
static bool startup=true;
static Ms lastSave=0,lastPublish=0,lastUi=0,lastSim=0;
static uint32_t persistedRevision=0,alarmRevision=0;
static float simulatedTemp=25;
static Ms nowMs(){return uint64_t(esp_timer_get_time())/1000;}

static void storageTask(void*){
  for(;;){String* text=nullptr;if(xQueueReceive(saves,&text,portMAX_DELAY)==pdTRUE){storageBusy=true;if(!storage::save(*text))storageFailed=true;delete text;storageBusy=false;}}
}
static void guardTask(void*){
  for(;;){if(uint32_t(millis()-heartbeat.load())>300){hw::off();stalled=true;}vTaskDelay(pdMS_TO_TICKS(20));}
}
static bool applyRequest(JsonObjectConst o,Ms now,String& why){
  auto& c=app.controller;
  std::string cmd=o["cmd"].as<std::string>();bool confirmed=o["confirmed"].as<bool>();
  if(web::maintenance){if(cmd=="ota_cancel"&&confirmed){web::maintenance=false;why="Manutencao encerrada";return true;}why="Modo de atualizacao ativo";return false;}
  // Emergency stop/pause is never rejected solely because a display revision changed.
  if(cmd=="pause"){why="Pausa";return c.command(Command::Pause,now);}
  if(cmd=="cancel"){c.command(Command::Pause,now);why="Cancelado";return c.command(Command::Cancel,now,confirmed);}
  if(!confirmed){why="Confirmacao obrigatoria";return false;}
  if(o["revision"].as<uint32_t>()!=c.revision||app.editing()){why="Estado mudou ou edicao local em curso; recarregue";return false;}
  if(cmd=="start"){c.recipe=app.recipes[app.selected];why="Iniciar";return c.command(Command::Start,now,true);}
  struct Entry{const char* name;Command value;};
  const Entry entries[]={{"confirm",Command::Confirm},{"resume",Command::Resume},{"ack",Command::Ack},{"recover",Command::Recover},{"discard",Command::Discard},{"skip",Command::Skip},{"ack_addition",Command::AckAddition},{"manual_stop",Command::ManualStop}};
  for(auto e:entries)if(cmd==e.name){why=String(e.name);return c.command(e.value,now,true);}
  if(!c.idle()){why="Pare o processo antes de editar";return false;}
  if(cmd=="ota_prepare"){
    if(storageBusy||uxQueueMessagesWaiting(saves)||app.saveRequested){why="Aguarde a gravacao em memoria";return false;}
    hw::off();web::maintenance=true;c.revision++;why="Saidas bloqueadas; envie firmware.bin";return true;
  }
  if(cmd=="manual_start"){
    auto m=o["manual"];
    if(!m["byPower"].is<bool>()||!m["pump"].is<bool>()||!m["target"].is<float>()||!m["power"].is<float>()||!m["seconds"].is<uint32_t>()){why="Parametros manuais invalidos";return false;}
    c.manual.byPower=m["byPower"];c.manual.pump=m["pump"];c.manual.target=m["target"];c.manual.power=m["power"];c.manual.seconds=m["seconds"];c.manual.heating=true;
    why="Manual";return c.command(Command::ManualStart,now,true);
  }
  if(cmd=="select"||cmd=="recipe_delete"){
    if(!o["index"].is<unsigned>()||o["index"].as<unsigned>()>=app.recipes.size())return false;unsigned i=o["index"];
    if(cmd=="select")app.selected=i;else{if(app.recipes.size()==1){why="Manter uma receita";return false;}app.recipes.erase(app.recipes.begin()+i);app.selected=0;}
  }else if(cmd=="recipe_save"){
    if(!o["index"].is<int>())return false;int i=o["index"];Recipe r;
    if(!codec::recipe(o["recipe"],r)||i< -1||(i>=0&&unsigned(i)>=app.recipes.size())||(i==-1&&app.recipes.size()>=8)){why="Receita ou indice invalido";return false;}
    if(i==-1)app.recipes.push_back(r);else app.recipes[i]=r;
  }else if(cmd=="config_save"){
    Config candidate=c.config;if(!codec::config(o["config"],candidate)){why="Configuracao invalida";return false;}c.config=candidate;
  }else if(cmd=="station"){
    if(!o["ssid"].is<const char*>()||!o["password"].is<const char*>())return false;
    String ssid=o["ssid"].as<String>(),pass=o["password"].as<String>();if(ssid.isEmpty()||ssid.length()>32||pass.length()>63)return false;
    WiFi.persistent(false);WiFi.begin(ssid.c_str(),pass.c_str());c.revision++;why="Conexao solicitada; acompanhe o IP no Serial";return true;
  }else{why="Comando desconhecido";return false;}
  c.revision++;app.saveRequested=true;c.log("Alteracao via web: "+cmd);why="Alteracao aplicada em RAM; persistencia pendente";return true;
}
void setup(){
  hw::initOutputs();Serial.begin(115200);
  for(int p:board::buttons)pinMode(p,INPUT_PULLUP);
  // NC safety contacts to GND; broken wire reads HIGH (unsafe).
  for(int p:{board::emergency,board::level,board::flow})pinMode(p,INPUT_PULLUP);
  pinMode(board::buzzer,OUTPUT);digitalWrite(board::buzzer,LOW);
  String stored;auto result=storage::load(stored);
  if(result==storage::Result::Corrupt||(result==storage::Result::Ok&&!codec::decode(stored,app)))storageFailed=true;
  if(result==storage::Result::Empty){
    app.recipes.clear();Recipe r;r.name="Demo classica";r.strike=38;r.steps={{52,70,0,600,{true,600,30}},{63,70,0,2100,{true,600,30}},{73,70,0,2100,{true,600,30}}};r.mashoutStep.target=78;app.recipes.push_back(r);
    r.name="Demo infusao unica";r.strike=65;r.steps={{65,70,0,3600,{true,600,30}}};app.recipes.push_back(r);
    r.name="Demo bancada";r.strike=30;r.steps={{32,50,0,60,{true,10,3}}};r.mashout=false;r.boilSec=60;r.additions.clear();app.recipes.push_back(r);app.saveRequested=true;
  }
  app.controller.config.commissioned=bool(CERVEJINO_ENABLE_OUTPUTS&&!CERVEJINO_SIM);
  lcd.begin(app.controller.config.lcdAddress);lcd.set(fit({"CERVEJINO","Inicializando...","Saidas desligadas",""}));
  saves=xQueueCreate(1,sizeof(String*));xTaskCreatePinnedToCore(storageTask,"storage",6144,nullptr,1,nullptr,0);
  web::begin();app.wifiToggle=[](bool on){web::enabled=on;app.wifiInfo=on?std::string("Senha ")+web::password().c_str():"WiFi desligado";};
  heartbeat=millis();xTaskCreatePinnedToCore(guardTask,"output-guard",2048,nullptr,3,nullptr,1);
  esp_task_wdt_init(3,true);esp_task_wdt_add(nullptr);
  Serial.println("Cervejino: GPIOs de potencia bloqueados nos perfis sim/bench.");
}
void loop(){
  Ms now=nowMs();heartbeat=millis();esp_task_wdt_reset();
  if(lcd.address()!=app.controller.config.lcdAddress&&app.controller.idle())lcd.begin(app.controller.config.lcdAddress);
  lcd.tick(now);
  if(web::maintenance){
    hw::off();lcd.set(fit({"Atualizacao WiFi","Saidas desligadas","Envie firmware.bin","Cancelar pelo web"}));
    web::Request* r=nullptr;if(web::take(r)){DynamicJsonDocument d(18000);String why;bool ok=false;uint32_t id=0;if(!deserializeJson(d,r->json)){id=d["id"];ok=applyRequest(d.as<JsonObjectConst>(),now,why);}web::result(id,ok,why);delete r;}
    vTaskDelay(pdMS_TO_TICKS(5));return;
  }
#if CERVEJINO_SIM
  float dt=lastSim?(now-lastSim)/1000.f:0;lastSim=now;
  simulatedTemp+=((app.controller.output.heater[0]?0.10f:0.f)-0.0005f*(simulatedTemp-25))*dt;
  simulatedTemp=clamp(simulatedTemp,20,99);input.t[0]={simulatedTemp,simulatedTemp,now,true};
  input.emergencyOk=input.levelOk=input.flowOk=true;input.lcdOk=true;
#else
  sensors.tick(now,app.controller.config,input);input.lcdOk=lcd.healthy();input.emergencyOk=digitalRead(board::emergency)==LOW;input.levelOk=digitalRead(board::level)==LOW;input.flowOk=digitalRead(board::flow)==LOW;
#endif
  if(startup){
#if CERVEJINO_SIM
    startup=false;
#else
    if(lcd.ready()&&input.t[0].valid)startup=false;else if(now>10000)startup=false;
#endif
    if(startup){hw::off();vTaskDelay(pdMS_TO_TICKS(5));return;}
  }
  if(storageFailed)app.controller.trip(Fault::Persistence);
  if(stalled)app.controller.trip(Fault::ControlStall);
  app.controller.tick(now,input);
  for(unsigned i=0;i<4;i++){auto press=buttons[i].update(digitalRead(board::buttons[i])==LOW,now,app.repeatAllowed()&&(i==1||i==2));if(press!=Press::None)app.key(Key(i),press,now);}
  if(app.bindRequested){app.bindRequested=false;if(app.controller.idle()){
#if CERVEJINO_SIM
    app.message="Sensor simulado";
#else
    hw::off();bool ok=sensors.bind(app.controller.config);app.message=ok?"Sensor vinculado":"Conecte 1 sensor";if(ok){app.saveRequested=true;app.controller.revision++;}
#endif
  }}
  web::Request* req=nullptr;if(web::take(req)){DynamicJsonDocument d(18000);String why="Comando invalido";bool ok=false;uint32_t id=0;if(!deserializeJson(d,req->json)){id=d["id"];ok=applyRequest(d.as<JsonObjectConst>(),now,why);}web::result(id,ok,why);delete req;}
  // Single owner writes output state. Guard task can only force outputs OFF.
  if(storageFailed||stalled)hw::off();else hw::apply(app.controller.output);
  if(now-lastUi>=100){lastUi=now;lcd.set(app.screen(now));}
  if(now-lastPublish>=1000){lastPublish=now;web::publish(codec::snapshot(app,now));}
  uint32_t rev=app.controller.revision;
  bool due=(app.saveRequested||(rev!=persistedRevision&&now-lastSave>=2000)||(app.controller.active()&&now-lastSave>=30000));
  if(due&&!storageFailed&&uxQueueSpacesAvailable(saves)){
    auto text=new String(codec::encode(app));if(text->isEmpty()){delete text;storageFailed=true;}else if(xQueueSend(saves,&text,0)==pdTRUE){lastSave=now;persistedRevision=rev;app.saveRequested=false;}else delete text;
  }
  static Ms beepUntil=0;
  if(rev!=alarmRevision){alarmRevision=rev;auto s=app.controller.state;if(app.controller.fault!=Fault::None||s==State::AddGrain||s==State::MashDone||s==State::EndBoil||(app.controller.fired&~app.controller.acknowledged))beepUntil=now+1000;}
  digitalWrite(board::buzzer,app.controller.config.sound&&now<beepUntil?HIGH:LOW);
  vTaskDelay(pdMS_TO_TICKS(5));
}
