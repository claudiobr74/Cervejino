#pragma once
#include "model.h"
#include <functional>
namespace brew {
enum class Key { Back, Up, Down, Ok };
enum class Press { None, Short, Long, Repeat };
class Button {
  bool raw_=true, stable_=true, armed_=false, emitted_=false;
  Ms changed_=0, down_=0, repeat_=0;
public:
  Press update(bool pressed,Ms now,bool repeatAllowed);
};
using Screen=std::array<std::string,4>;
struct Field {
  std::string name;
  std::function<std::string()> read;
  std::function<void(int)> change;
};
class App {
public:
  Controller controller;
  std::vector<Recipe> recipes{Recipe{}};
  unsigned selected=0;
  bool saveRequested=false, bindRequested=false, wifi=false;
  std::string message, wifiInfo="WiFi desligado";
  std::function<void(bool)> wifiToggle;
  explicit App(bool sim=false):controller(sim){}
  void key(Key k,Press p,Ms now);
  Screen screen(Ms now) const;
  bool repeatAllowed()const{return editing_;}
  bool editing()const{return page_==Page::Fields||page_==Page::Recipes;}
  void home(){page_=Page::Home;cursor_=0;editing_=false;}
  void setMessage(const std::string& s){message=s;}
private:
  enum class Page { Home, Recipes, RecipeActions, Fields, Run, Confirm, Diagnostics, Wifi };
  enum class Save { None, Recipe, Config, Manual, ActiveRecipe, ActiveManual };
  Page page_=Page::Home, returnPage_=Page::Home;
  Save save_=Save::None;
  Ms now_=0;
  unsigned cursor_=0, runPage_=0, editRecipe_=0, charAt_=0;
  bool editing_=false, recipeDraftNew_=false;
  Recipe draftRecipe_;
  Config draftConfig_;
  Manual draftManual_;
  std::vector<Field> fields_;
  std::string question_;
  std::function<void()> yes_;
  void confirm(const std::string& text,std::function<void()> yes);
  void recipeFields();
  void configFields(bool calibration=false);
  void manualFields();
  void numeric(const std::string& n,float& v,float lo,float hi,float step);
  void integer(const std::string& n,uint32_t& v,uint32_t lo,uint32_t hi,uint32_t step=1);
  void boolean(const std::string& n,bool& v);
  void stepFields(Step& s,const std::string& prefix);
};
Screen fit(Screen s);
}
