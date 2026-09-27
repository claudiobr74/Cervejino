#pragma once
#include <Arduino.h>
#include <atomic>
namespace web {
struct Request {String json;};
extern std::atomic<bool> enabled;
extern std::atomic<bool> maintenance;
void begin();
void publish(const String& snapshot);
bool take(Request*& request);
void result(uint32_t id,bool applied,const String& message);
String password();
}
