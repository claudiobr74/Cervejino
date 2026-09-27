#include "cervejino/storage.h"
#include <LittleFS.h>
namespace storage {
struct Header {uint32_t magic,version,sequence,length,checksum;};
static uint32_t sequence=0;static int active=-1;static bool mounted=false;
uint32_t crc(const uint8_t* data,size_t n){uint32_t c=0xffffffff;for(size_t i=0;i<n;i++){c^=data[i];for(int j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320:0);}return ~c;}
static bool read(int slot,String& text,uint32_t& seq){auto f=LittleFS.open(slot?"/state1.dat":"/state0.dat","r");if(!f)return false;Header h{};if(f.read(reinterpret_cast<uint8_t*>(&h),sizeof h)!=sizeof h||h.magic!=0x4352564a||h.version!=1||h.length>40000||f.size()!=sizeof h+h.length)return false;text=f.readString();seq=h.sequence;return text.length()==h.length&&crc(reinterpret_cast<const uint8_t*>(text.c_str()),text.length())==h.checksum;}
Result load(String& text){
  // Never format automatically: a mount failure must not erase recovery/config.
  mounted=LittleFS.begin(false);if(!mounted)return Result::Corrupt;
  String a,b;uint32_t sa=0,sb=0;bool va=read(0,a,sa),vb=read(1,b,sb);
  if(!va&&!vb)return LittleFS.exists("/state0.dat")||LittleFS.exists("/state1.dat")?Result::Corrupt:Result::Empty;
  bool pickB=vb&&(!va||int32_t(sb-sa)>0);active=pickB?1:0;sequence=pickB?sb:sa;text=pickB?b:a;return Result::Ok;
}
bool save(const String& text){
  if(!mounted||text.isEmpty()||text.length()>40000)return false;
  int slot=active==0?1:0;Header h{0x4352564a,1,sequence+1,uint32_t(text.length()),crc(reinterpret_cast<const uint8_t*>(text.c_str()),text.length())};
  auto f=LittleFS.open(slot?"/state1.dat":"/state0.dat","w");if(!f)return false;
  bool ok=f.write(reinterpret_cast<const uint8_t*>(&h),sizeof h)==sizeof h&&f.write(reinterpret_cast<const uint8_t*>(text.c_str()),text.length())==text.length();f.flush();f.close();
  String check;uint32_t seq;if(!ok||!read(slot,check,seq)||check!=text)return false;active=slot;sequence=h.sequence;return true;
}
}
