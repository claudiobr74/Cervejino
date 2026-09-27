#pragma once
#include <Arduino.h>
#include <Preferences.h>
namespace storage {
enum class Result { Empty, Ok, Corrupt };
Result load(String& text);
bool save(const String& text);
uint32_t crc(const uint8_t* bytes,size_t len);
}
