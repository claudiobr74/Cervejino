#include "cervejino/ui.h"
#include <cstdio>
#include <algorithm>
namespace brew {
static std::string number(float x,int decimals=1){char b[32];std::snprintf(b,sizeof b,decimals?"%.1f":"%.0f",double(x));return b;}
static std::string timeText(Ms t){char b[24];std::snprintf(b,sizeof b,"%02u:%02u:%02u",unsigned(t/3600000),unsigned(t/60000%60),unsigned(t/1000%60));return b;}
Screen fit(Screen s){for(auto& row:s){row.resize(std::min<size_t>(row.size(),20));row.append(20-row.size(),' ');}return s;}
Press Button::update(bool pressed,Ms now,bool repeatAllowed){
  if(pressed!=raw_){raw_=pressed;changed_=now;}
  if(now-changed_<35)return Press::None;
  if(!pressed&&!armed_){armed_=true;stable_=false;return Press::None;}
  if(!armed_)return Press::None;
  if(pressed!=stable_){stable_=pressed;if(pressed){down_=repeat_=now;emitted_=false;if(!repeatAllowed){emitted_=true;return Press::Short;}}else if(!emitted_)return Press::Short;}
  if(pressed&&repeatAllowed&&now-down_>=600&&now-repeat_>=150){repeat_=now;emitted_=true;return Press::Repeat;}
  if(pressed&&!repeatAllowed&&now-down_>=1200&&now-repeat_==now-down_){repeat_=now;return Press::Long;}
  return Press::None;
}
void App::numeric(const std::string& n,float& v,float lo,float hi,float step){fields_.push_back({n,[&v]{return number(v);},[&v,lo,hi,step](int d){v=clamp(v+d*step,lo,hi);}});}
void App::integer(const std::string& n,uint32_t& v,uint32_t lo,uint32_t hi,uint32_t step){fields_.push_back({n,[&v]{return std::to_string(v);},[&v,lo,hi,step](int d){int64_t z=int64_t(v)+int64_t(d)*step;v=uint32_t(std::max<int64_t>(lo,std::min<int64_t>(hi,z)));}});}
void App::boolean(const std::string& n,bool& v){fields_.push_back({n,[&v]{return v?"Sim":"Nao";},[&v](int){v=!v;}});}
void App::stepFields(Step& s,const std::string& p){numeric(p+" temperatura",s.target,1,100,0.5);integer(p+" tempo (s)",s.seconds,1,86400,60);numeric(p+" potencia %",s.cap,0,100,5);numeric(p+" rampa C/min",s.ramp,0,10,0.1);boolean(p+" bomba",s.pump.enabled);integer(p+" bomba ON s",s.pump.onSec,1,86400,5);integer(p+" bomba OFF s",s.pump.offSec,0,86400,5);}
void App::recipeFields(){
  fields_.clear();save_=Save::Recipe;page_=Page::Fields;cursor_=0;editing_=false;
  auto& r=draftRecipe_;
  fields_.push_back({"Nome: posicao",[this]{return std::to_string(charAt_+1);},[this](int d){charAt_=(charAt_+24+d)%24;}});
  fields_.push_back({"Nome: caractere",[this]{return draftRecipe_.name;},[this](int d){static const std::string chars=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";auto& n=draftRecipe_.name;n.resize(24,' ');size_t pos=chars.find(n[charAt_]);if(pos==std::string::npos)pos=0;n[charAt_]=chars[(pos+chars.size()+d)%chars.size()];}});
  numeric("Volume litros",r.liters,1,1000,1);numeric("Adicionar graos C",r.strike,1,95,0.5);
  fields_.push_back({"Numero de etapas",[this]{return std::to_string(draftRecipe_.steps.size());},[this](int d){auto n=draftRecipe_.steps.size();if(d>0&&n<10)draftRecipe_.steps.push_back(Step{});if(d<0&&n>1)draftRecipe_.steps.pop_back();}});
  for(unsigned i=0;i<r.steps.size();i++)stepFields(r.steps[i],"E"+std::to_string(i+1));
  boolean("Usar mash-out",r.mashout);stepFields(r.mashoutStep,"MO");
  numeric("Fervura referencia C",r.boilReference,80,104,0.5);numeric("Fervura potencia %",r.boilPower,0,100,5);integer("Fervura tempo s",r.boilSec,1,86400,60);
  integer("Whirlpool tempo s",r.whirlpoolSec,0,86400,60);numeric("Whirlpool max C",r.whirlpoolMax,1,100,0.5);numeric("Resfriamento alvo C",r.coolTarget,1,90,0.5);
  fields_.push_back({"Numero de adicoes",[this]{return std::to_string(draftRecipe_.additions.size());},[this](int d){auto& a=draftRecipe_.additions;if(d>0&&a.size()<6)a.push_back({"Adicao "+std::to_string(a.size()+1),0});if(d<0&&!a.empty())a.pop_back();}});
  for(unsigned i=0;i<r.additions.size();i++)integer("Adicao "+std::to_string(i+1)+" restante s",r.additions[i].remainingSec,0,r.boilSec,60);
}
void App::configFields(bool calibration){
  fields_.clear();save_=Save::Config;page_=Page::Fields;cursor_=0;editing_=false;auto& c=draftConfig_;
  if(calibration){numeric("Offset sensor C",c.offset[0],-10,10,0.1);return;}
  numeric("Resistencia watts",c.watts[0],1,20000,100);
  integer("Janela PWM ms",c.windowMs,1000,10000,500);numeric("PI Kp",c.kp,0,100,0.5);numeric("PI Ki",c.ki,0,5,0.01);
  numeric("Tolerancia C",c.tolerance,0.1,5,0.1);integer("Estabilidade s",c.stableSec,1,600);integer("Timeout aquecer s",c.heatTimeoutSec,60,86400,60);
  boolean("Contar fora da faixa",c.countOutOfBand);boolean("Aquecer sem bomba",c.heatWithoutPump);numeric("Limite termico C",c.maxTemp,80,115,0.5);numeric("Bomba max C",c.pumpMax,20,105,0.5);
  boolean("Som avisos",c.sound);boolean("Fervura automatica",c.autoBoil);integer("Fervura estavel s",c.boilStableSec,10,600,10);numeric("Fervura faixa C",c.boilPlateauBand,0.1,2,0.1);numeric("Variacao max C/s",c.maxRate,0.1,10,0.1);
  integer("Leitura vencida ms",c.staleMs,1000,10000,250);integer("Bomba repouso ms",c.pumpMinMs,1000,60000,1000);
  integer("Escorva ciclos",c.primeCycles,0,10);integer("Escorva ON s",c.primeOnSec,1,60);integer("Escorva OFF s",c.primeOffSec,1,60);
  boolean("Exigir nivel",c.requireLevel);boolean("Exigir fluxo",c.requireFlow);integer("Prazo fluxo ms",c.flowGraceMs,1000,30000,1000);
  fields_.push_back({"Endereco LCD I2C",[&c]{char b[8];std::snprintf(b,sizeof b,"0x%02X",c.lcdAddress);return std::string(b);},[&c](int d){c.lcdAddress=uint8_t(clamp(c.lcdAddress+d,8,119));}});
}
void App::manualFields(){fields_.clear();save_=Save::Manual;page_=Page::Fields;cursor_=0;editing_=false;boolean("Aquecimento ligado",draftManual_.heating);boolean("Controle por potencia",draftManual_.byPower);numeric("Temperatura alvo C",draftManual_.target,1,100,0.5);numeric("Potencia %",draftManual_.power,0,100,5);boolean("Bomba ligada",draftManual_.pump);integer("Temporizador s",draftManual_.seconds,1,86400,60);}
void App::confirm(const std::string& t,std::function<void()> fn){question_=t;yes_=std::move(fn);returnPage_=page_;page_=Page::Confirm;editing_=false;}
void App::key(Key k,Press p,Ms now){
  if(p==Press::None||p==Press::Long)return;
  if(p==Press::Repeat&&!(editing_&&(k==Key::Up||k==Key::Down)))return;
  now_=now;auto& c=controller;
  if(c.state==State::Fault){if(k==Key::Ok&&p==Press::Short)c.command(Command::Ack,now,true);return;}
  if(c.state==State::Recovery){if(k==Key::Ok){if(!c.command(Command::Recover,now,true))message="Verifique sensores";}else if(k==Key::Back)c.command(Command::Discard,now,true);return;}
  if(page_==Page::Confirm){if(k==Key::Back){page_=returnPage_;return;}if(k==Key::Ok){auto action=yes_;page_=returnPage_;action();}return;}
  if((c.active()||page_==Page::Run)&&page_!=Page::Fields){page_=Page::Run;
    if(k==Key::Up){runPage_=(runPage_+5)%6;return;}if(k==Key::Down){runPage_=(runPage_+1)%6;return;}
    if(k==Key::Back){if(c.state!=State::Paused)c.command(Command::Pause,now);else confirm("Cancelar brassagem?",[this]{controller.command(Command::Cancel,now_,true);home();});return;}
    if(k==Key::Ok){
      if(runPage_==4&&c.active()){
        if(c.state!=State::Paused)c.command(Command::Pause,now);
        auto resume=c.checkpoint().resumeState;
        if(resume==State::Manual){draftManual_=c.manual;manualFields();save_=Save::ActiveManual;}
        else{
          draftRecipe_=c.recipe;fields_.clear();cursor_=0;editing_=false;page_=Page::Fields;save_=Save::ActiveRecipe;
          if(resume==State::Mash&&c.stepIndex<draftRecipe_.steps.size()){auto& s=draftRecipe_.steps[c.stepIndex];numeric("Etapa alvo C",s.target,1,100,0.5);integer("Etapa tempo total s",s.seconds,1,86400,60);numeric("Etapa potencia %",s.cap,0,100,5);}
          else if(resume==State::MashOut){numeric("Mashout alvo C",draftRecipe_.mashoutStep.target,1,100,0.5);integer("Mashout total s",draftRecipe_.mashoutStep.seconds,1,86400,60);}
          else if(resume==State::Boil){integer("Fervura total s",draftRecipe_.boilSec,1,86400,60);numeric("Fervura potencia %",draftRecipe_.boilPower,0,100,5);}
          else{page_=Page::Run;message="Sem ajuste nesta fase";}
        }return;
      }
      if(c.state==State::Paused)confirm("Retomar processo?",[this]{controller.command(Command::Resume,now_,true);});
      else if((c.fired&~c.acknowledged)!=0)c.command(Command::AckAddition,now,true);
      else if(c.idle())home();
      else c.command(Command::Confirm,now,true);
    }return;
  }
  if(page_==Page::Fields){
    if(editing_){if(k==Key::Up||k==Key::Down){fields_[cursor_].change(k==Key::Up?-1:1);return;}
      editing_=false;
      if(save_==Save::Recipe){unsigned old=cursor_;recipeFields();cursor_=std::min<unsigned>(old,fields_.size());}return;
    }
    if(k==Key::Back){confirm("Descartar alteracoes?",[this]{home();});return;}
    if(k==Key::Up){if(cursor_)cursor_--;return;}if(k==Key::Down){if(cursor_<fields_.size())cursor_++;return;}
    if(k==Key::Ok&&cursor_<fields_.size()){editing_=true;return;}
    if(k==Key::Ok)confirm(save_==Save::Manual?"Iniciar modo manual?":"Salvar alteracoes?",[this]{
      if(save_==Save::ActiveRecipe){if(!valid(draftRecipe_)){message="Ajuste invalido";return;}controller.recipe=draftRecipe_;controller.revision++;controller.log("Etapa ajustada local");page_=Page::Run;}
      else if(save_==Save::ActiveManual){controller.manual=draftManual_;controller.revision++;controller.log("Manual ajustado");page_=Page::Run;}
      else if(save_==Save::Recipe){while(!draftRecipe_.name.empty()&&draftRecipe_.name.back()==' ')draftRecipe_.name.pop_back();if(!valid(draftRecipe_)){message="Receita invalida";return;}if(recipeDraftNew_)recipes.push_back(draftRecipe_);else recipes[editRecipe_]=draftRecipe_;saveRequested=true;controller.revision++;home();}
      else if(save_==Save::Config){if(!valid(draftConfig_)){message="Config invalida";return;}controller.config=draftConfig_;saveRequested=true;controller.revision++;home();}
      else{controller.manual=draftManual_;if(controller.command(Command::ManualStart,now_,true))page_=Page::Run;else message="Inicio bloqueado";}
    });return;
  }
  if(page_==Page::Recipes){unsigned n=recipes.size()+1;if(k==Key::Back){home();return;}if(k==Key::Up)cursor_=(cursor_+n-1)%n;if(k==Key::Down)cursor_=(cursor_+1)%n;
    if(k==Key::Ok){if(cursor_==recipes.size()){if(recipes.size()>=8){message="Maximo 8 receitas";return;}draftRecipe_=Recipe{};draftRecipe_.name="Receita "+std::to_string(recipes.size()+1);recipeDraftNew_=true;recipeFields();}else{selected=editRecipe_=cursor_;cursor_=0;page_=Page::RecipeActions;}}return;}
  if(page_==Page::RecipeActions){if(k==Key::Back){home();return;}if(k==Key::Up)cursor_=(cursor_+3)%4;if(k==Key::Down)cursor_=(cursor_+1)%4;if(k!=Key::Ok)return;
    if(cursor_==0){message="Receita selecionada";home();}
    else if(cursor_==1){draftRecipe_=recipes[editRecipe_];recipeDraftNew_=false;recipeFields();}
    else if(cursor_==2&&recipes.size()<8){recipes.push_back(recipes[editRecipe_]);recipes.back().name=(recipes.back().name.substr(0,18)+" copia");saveRequested=true;controller.revision++;home();}
    else if(cursor_==3)confirm("Excluir receita?",[this]{if(recipes.size()>1){recipes.erase(recipes.begin()+editRecipe_);selected=0;saveRequested=true;controller.revision++;home();}else message="Manter uma receita";});return;
  }
  if(page_==Page::Diagnostics){if(k==Key::Back)home();else if(k==Key::Ok)confirm("Vincular sensor?",[this]{bindRequested=true;home();});return;}
  if(page_==Page::Wifi){if(k==Key::Back)home();else if(k==Key::Ok){wifi=!wifi;if(wifiToggle)wifiToggle(wifi);}return;}
  if(k==Key::Up)cursor_=(cursor_+6)%7;if(k==Key::Down)cursor_=(cursor_+1)%7;
  if(k==Key::Ok)switch(cursor_){
    case 0:confirm("Iniciar brassagem?",[this]{controller.recipe=recipes[selected];if(controller.command(Command::Start,now_,true))page_=Page::Run;else message="Inicio bloqueado";});break;
    case 1:page_=Page::Recipes;cursor_=selected;break;
    case 2:draftManual_=controller.manual;manualFields();break;
    case 3:draftConfig_=c.config;configFields();break;
    case 4:draftConfig_=c.config;configFields(true);break;
    case 5:page_=Page::Diagnostics;break;
    case 6:page_=Page::Wifi;break;
  }
}
Screen App::screen(Ms now)const{
  const auto& c=controller;auto temp=[&](unsigned i){auto t=c.inputs.t[i];return t.valid&&now>=t.at&&now-t.at<=c.config.staleMs?number(t.filtered):"ERRO";};
  if(c.state==State::Fault)return fit({"FALHA BLOQUEANTE",label(c.fault),"Saidas desligadas","              OK ack"});
  if(c.state==State::Recovery)return fit({"Energia interrompida","Verifique a panela","Retomar so se seguro","Sair          OK sim"});
  if(page_==Page::Confirm)return fit({question_,"Confirmar acao?","", "Nao           OK sim"});
  if((c.active()||page_==Page::Run)&&page_!=Page::Fields){std::string foot=c.state==State::Paused?"Sair  <    >  Retoma":"Pausa <    >  OK";
    if(runPage_==0){auto s=c.currentStep();std::string phase=label(c.state);if(s)phase=c.effective?"Contando patamar":std::fabs(c.inputs.t[0].filtered-c.target)<=c.config.tolerance?"Estabilizando":"Aquecendo mostura";return fit({phase,"T:"+temp(0)+" Alvo:"+number(c.target),"Resta "+timeText(c.remaining()),foot});}
    if(runPage_==1)return fit({"Temperatura","T:"+temp(0),"Alvo:"+number(c.target),foot});
    if(runPage_==2)return fit({"Aquecimento: "+number(c.output.power.duty[0]*100,0)+"%",c.output.pump?"Bomba ON (comando)":"Bomba OFF", "Req/Aut W "+number(c.output.power.requestedWatts,0)+"/"+number(c.output.power.allowedWatts,0),foot});
    if(runPage_==4)return fit({"Ajustar fase atual","OK pausa e edita","Retomar apos salvar",foot});
    if(runPage_==5){const char* help="Siga as instrucoes";if(c.state==State::Precheck)help="Confirme agua cheia";else if(c.state==State::AddGrain||c.state==State::ConfirmGrain)help="Malte, telas, trava";else if(c.state==State::RemoveBasket)help="Retire cesto e drene";else if(c.state==State::Boil)help="Ferver sem tampa";else if(c.state==State::Cooling)help="Resfriamento externo";return fit({"Ajuda da etapa",help,"OK confirma etapa",foot});}
    for(unsigned i=0;i<c.recipe.additions.size();i++)if((c.fired&~c.acknowledged)&(1u<<i))return fit({"ADICIONAR AGORA",c.recipe.additions[i].name,"OK reconhece avisos",foot});
    Ms next=UINT64_MAX;std::string name="Sem proxima adicao";for(unsigned i=0;i<c.recipe.additions.size();i++)if(!(c.fired&(1u<<i))){Ms when=Ms(c.recipe.additions[i].remainingSec)*1000;Ms d=c.remaining()>when?c.remaining()-when:0;if(d<next){next=d;name=c.recipe.additions[i].name;}}
    return fit({"Proximo evento",name,next==UINT64_MAX?"":timeText(next),foot});
  }
  if(page_==Page::Fields){if(cursor_==fields_.size())return fit({"Finalizar edicao",save_==Save::Manual?"Iniciar manual":"Salvar","", "Volta -    +    OK"});const auto& f=fields_[cursor_];return fit({f.name,f.read(),editing_?"Editando (- / +)":"OK para editar","Volta -    +    OK"});}
  if(page_==Page::Recipes)return fit({"Receitas",cursor_<recipes.size()?recipes[cursor_].name:"+ Nova receita",message,"Volta ^    v    OK"});
  if(page_==Page::RecipeActions){static const char* a[]={"Selecionar","Editar","Duplicar","Excluir"};return fit({recipes[editRecipe_].name,a[cursor_],message,"Volta ^    v    OK"});}
  if(page_==Page::Diagnostics)return fit({"Temperatura:"+temp(0),c.config.commissioned?"Hardware configurado":"Saidas bloqueadas", "OK vincula sensores","Volta          OK"});
  if(page_==Page::Wifi)return fit({"WiFi complementar",wifi?"AP Cervejino ativo":"Desligado",wifiInfo,"Volta       Liga/Off"});
  static const char* menu[]={"Iniciar brassagem","Receitas","Modo manual","Configuracao","Calibracao","Diagnostico","WiFi"};
  return fit({"CERVEJINO",menu[cursor_],message.empty()?recipes[selected].name:message,"      ^    v    OK"});
}
}
