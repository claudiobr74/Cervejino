#include "cervejino/web.h"
#include "web_page.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
namespace web {
std::atomic<bool> enabled{false};
std::atomic<bool> maintenance{false};
static WebServer server(80);
static QueueHandle_t requests;
static SemaphoreHandle_t lock;
static String latest="{}",lastResult="{}",secret,csrf;
static bool running=false;
static uint32_t lastId=0;
static bool uploadOk=false;
static uint32_t rebootAt=0;
static bool auth(){if(!server.authenticate("cervejino",secret.c_str())){server.requestAuthentication();return false;}return true;}
String password(){return secret;}
void publish(const String& s){if(xSemaphoreTake(lock,0)==pdTRUE){latest=s;xSemaphoreGive(lock);}}
bool take(Request*& r){return xQueueReceive(requests,&r,0)==pdTRUE;}
void result(uint32_t id,bool applied,const String& message){DynamicJsonDocument d(512);d["id"]=id;d["applied"]=applied;d["message"]=message;String s;serializeJson(d,s);if(xSemaphoreTake(lock,pdMS_TO_TICKS(2))==pdTRUE){lastResult=s;xSemaphoreGive(lock);}}
static void task(void*){
  for(;;){if(enabled&&!running){WiFi.mode(WIFI_AP_STA);WiFi.softAP("Cervejino",secret.c_str());server.begin();running=true;}else if(!enabled&&running){server.stop();WiFi.disconnect(true);WiFi.softAPdisconnect(true);WiFi.mode(WIFI_OFF);running=false;}
    if(running)server.handleClient();if(rebootAt&&int32_t(millis()-rebootAt)>=0)ESP.restart();vTaskDelay(pdMS_TO_TICKS(5));}
}
void begin(){
  requests=xQueueCreate(3,sizeof(Request*));lock=xSemaphoreCreateMutex();char b[33];snprintf(b,sizeof b,"%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random());secret=String(b).substring(0,12);snprintf(b,sizeof b,"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());csrf=b;
  const char* headers[]={"X-CSRF-Token"};server.collectHeaders(headers,1);
  server.on("/",HTTP_GET,[]{if(!auth())return;String html=FPSTR(WEB_PAGE);html.replace("__CSRF__",csrf);server.sendHeader("Cache-Control","no-store");server.sendHeader("X-Frame-Options","DENY");server.send(200,"text/html; charset=utf-8",html);});
  server.on("/api/state",HTTP_GET,[]{if(!auth())return;String copy;if(xSemaphoreTake(lock,pdMS_TO_TICKS(20))==pdTRUE){copy=latest;xSemaphoreGive(lock);}server.sendHeader("Cache-Control","no-store");server.send(copy.length()?200:503,"application/json",copy.length()?copy:"{}");});
  server.on("/api/result",HTTP_GET,[]{if(!auth())return;String copy;if(xSemaphoreTake(lock,pdMS_TO_TICKS(20))==pdTRUE){copy=lastResult;xSemaphoreGive(lock);}server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",copy);});
  server.on("/api/command",HTTP_POST,[]{
    if(!auth())return;if(server.header("X-CSRF-Token")!=csrf){server.send(403,"application/json","{\"error\":\"CSRF\"}");return;}
    String body=server.arg("plain");if(body.length()>12000){server.send(413,"application/json","{\"error\":\"Limite 12 KB\"}");return;}
    DynamicJsonDocument d(18000);if(deserializeJson(d,body)||!d["id"].is<uint32_t>()||!d["revision"].is<uint32_t>()||!d["cmd"].is<const char*>()||!d["confirmed"].is<bool>()){server.send(400,"application/json","{\"error\":\"Comando invalido\"}");return;}
    uint32_t id=d["id"];if(id<=lastId){server.send(409,"application/json","{\"error\":\"ID repetido ou antigo; recarregue\"}");return;}
    auto r=new Request{body};if(xQueueSend(requests,&r,0)!=pdTRUE){delete r;server.send(503,"application/json","{\"error\":\"Fila ocupada\"}");return;}lastId=id;server.send(202,"application/json","{\"received\":true}");
  });
  server.on("/api/update",HTTP_POST,[]{
    if(!auth())return;
    bool ok=uploadOk&&!Update.hasError();
    server.send(ok?200:400,"application/json",ok?"{\"ok\":true,\"reboot\":true}":"{\"error\":\"Atualizacao recusada ou incompleta\"}");
    if(ok)rebootAt=millis()+1000;
  },[]{
    auto& upload=server.upload();
    if(upload.status==UPLOAD_FILE_START){
      uploadOk=maintenance&&server.authenticate("cervejino",secret.c_str())&&server.header("X-CSRF-Token")==csrf;
      if(uploadOk)uploadOk=Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH);
    }else if(upload.status==UPLOAD_FILE_WRITE){if(uploadOk&&Update.write(upload.buf,upload.currentSize)!=upload.currentSize)uploadOk=false;}
    else if(upload.status==UPLOAD_FILE_END){if(uploadOk)uploadOk=Update.end(true);else Update.abort();}
    else if(upload.status==UPLOAD_FILE_ABORTED){Update.abort();uploadOk=false;}
  });
  xTaskCreatePinnedToCore(task,"web",8192,nullptr,1,nullptr,0);
}
}
