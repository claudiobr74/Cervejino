#pragma once
#include "ui.h"
#include <ArduinoJson.h>
namespace codec {
constexpr unsigned schema=1;
void recipe(JsonObject o,const brew::Recipe& r);
bool recipe(JsonObjectConst o,brew::Recipe& r);
void config(JsonObject o,const brew::Config& c);
bool config(JsonObjectConst o,brew::Config& c);
void checkpoint(JsonObject o,const brew::Checkpoint& p);
bool checkpoint(JsonObjectConst o,brew::Checkpoint& p);
String encode(const brew::App& app);
bool decode(const String& text,brew::App& app);
String snapshot(const brew::App& app,brew::Ms now);
}
