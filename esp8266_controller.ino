#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Hash.h>

ESP8266WebServer server(1000);

#define RELAY1 D1
#define RELAY2 D2
#define RELAY3 D5

String passHash;
String session;

void loadPass(){
  char b[65];
  for(int i=0;i<64;i++) b[i]=EEPROM.read(i);
  b[64]=0;
  passHash = String(b);
}

bool auth(){
  return server.hasHeader("Cookie") &&
         server.header("Cookie").indexOf(session)>=0;
}

void setup(){
  pinMode(RELAY1,OUTPUT);
  pinMode(RELAY2,OUTPUT);
  pinMode(RELAY3,OUTPUT);
  digitalWrite(RELAY1,HIGH);
  digitalWrite(RELAY2,HIGH);
  digitalWrite(RELAY3,HIGH);

  EEPROM.begin(128);
  loadPass();

  WiFi.begin("SSID","PASSWORD");
  while(WiFi.status()!=WL_CONNECTED) delay(500);

  server.on("/auth",HTTP_GET,[](){
    auth()?server.send(200):server.send(401);
  });

  server.on("/login",HTTP_POST,[](){
    if(server.arg("plain")==passHash){
      session=sha1(String(millis()));
      server.sendHeader("Set-Cookie","SID="+session+"; HttpOnly");
      server.send(200);
    } else server.send(403);
  });

  server.on("/passwd",HTTP_POST,[](){
    if(!auth()){server.send(403);return;}
    String b=server.arg("plain");
    int s=b.indexOf(':');
    if(b.substring(0,s)!=passHash){server.send(403);return;}
    passHash=b.substring(s+1);
    for(int i=0;i<64;i++)
      EEPROM.write(i,i<passHash.length()?passHash[i]:0);
    EEPROM.commit();
    server.send(200);
  });

  auto relay=[&](int p){
    if(!auth()){server.send(403);return;}
    digitalWrite(p,LOW); delay(500); digitalWrite(p,HIGH);
    server.send(200);
  };

  server.on("/r1",[&](){relay(RELAY1);});
  server.on("/r2",[&](){relay(RELAY2);});
  server.on("/r3",[&](){relay(RELAY3);});

  server.begin();
}

void loop(){
  server.handleClient();
}
