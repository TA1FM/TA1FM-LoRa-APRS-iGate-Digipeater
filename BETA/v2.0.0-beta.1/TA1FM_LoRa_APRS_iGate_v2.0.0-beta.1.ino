/*
  TA1FM LoRa APRS iGate & Digi V2.0.0 beta.1 (DEV7.2 source)
  Heltec Wireless Stick Lite V3 (ESP32-S3 + SX1262), SSD1306 / SH1106 / OFF on Wire1
  RadioLib 7.8.1, Adafruit SSD1306, Adafruit SH110X, Adafruit GFX
  APRS-IS iGate + optional backup RF digipeater. Own position beacon to APRS-IS in iGate mode; RF in backup Digi mode.
  AUTO DIGI OFF BY DEFAULT. TX requires explicit web opt-in.
  Pinout: Ricardo Guzman WSL V3 board_pinout.h (provided by user).
*/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <time.h>
#include <esp_system.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include <TinyGPS++.h>  // Arduino Library Manager: TinyGPSPlus
#include <Adafruit_BME280.h> // Arduino Library Manager: Adafruit BME280 Library

// === EDIT THESE TWO BEFORE FIRST UPLOAD ===
// 8+ characters, change them for your own device.
// Copy TA1FM_secrets.example.h to TA1FM_secrets.h before compiling.
#include "TA1FM_secrets.h"
static const char* DEFAULT_AP_PASSWORD = TA1FM_AP_PASSWORD;
// Experimental TOCALL: NOT a registered device identity.
// Replace only after an identifier is formally allocated.
static const char* OWN_TOCALL = "APZTFM";
static const char* PANEL_PASSWORD = TA1FM_PANEL_PASSWORD;
static const char* PANEL_USER = "admin";

static constexpr int PIN_SCK=9, PIN_MISO=11, PIN_MOSI=10, PIN_CS=8;
static constexpr int PIN_RST=12, PIN_DIO1=14, PIN_BUSY=13;
static constexpr int PIN_SDA=41, PIN_SCL=42, PIN_VEXT=36;
static constexpr int GPS_RX=47, GPS_TX=48;
static constexpr uint32_t GPS_BAUD=9600;
static constexpr int BATTERY_PIN=1, ADC_CTRL_PIN=37;
static constexpr int ADC_CTRL_ON_STATE=LOW;
static constexpr float BATTERY_DIVIDER_RATIO=4.9f; // WSL V3: 390k + 100k; multimetre ile dogrulayin.
static constexpr uint32_t BATTERY_SAMPLE_INTERVAL_MS=30000UL;
static constexpr float LORA_FREQ=433.775f, LORA_BW=125.0f;
static constexpr uint8_t LORA_SF=12, LORA_CR=5, LORA_SYNC=0x12;
static constexpr size_t MAX_FRAME=255;

SX1262 radio = new Module(PIN_CS, PIN_DIO1, PIN_RST, PIN_BUSY);
Adafruit_SSD1306 oledSsd(128,64,&Wire1,-1);
Adafruit_SH1106G oledSh(128,64,&Wire1,-1);
Adafruit_GFX* oledDisplay=nullptr; // Secili OLED'in ortak cizim arayuzu.
WebServer server(80);
Preferences prefs;
WiFiClient aprs;
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;
Adafruit_BME280 bme;
bool bmeDetected=false, bmeFresh=false;
uint8_t bmeAddress=0;
float bmeTemp=NAN,bmeHum=NAN,bmePressure=NAN;
uint32_t bmeLastProbe=0,bmeLastRead=0,tlmLastIs=0,tlmLastRf=0,tlmRfAttempt=0;
uint16_t tlmSequence=0;
uint8_t tlmDescriptionStage=0;
String extraWifiSsid[4],extraWifiPass[4];
struct AprsMessage {String from,to,text,id,channel;uint32_t at=0;};
String bridgeRfPending="";uint32_t bridgeRfQueuedAt=0,bridgeRfLastAttempt=0;
uint32_t bridgeSeenHashes[24]={0},bridgeSeenAt[24]={0};uint8_t bridgeSeenPtr=0;
uint32_t bridgeLastFilter=0;bool bridgeFilterDirty=false;
AprsMessage messageLog[16];uint8_t messageHead=0,messageCount=0;
String outboundDest,outboundText,outboundId,outboundCall;
uint8_t outboundMode=0,outboundRfTries=0,outboundIsTries=0;
bool outboundAcked=false;
uint32_t outboundRfLast=0,outboundIsLast=0;
String ackRfPending,ackIsPending;
uint32_t ackRfAt=0,ackIsAt=0,ackRfAttempt=0;
uint16_t nextMessageNumber=1;
uint8_t wifiCandidate=0;

uint32_t gpsCharsSeen=0;
uint32_t gpsLastFreshAt=0;
uint32_t gpsLastPoll=0;
uint32_t trackerLastRf=0,trackerLastIs=0,trackerRfAttempts=0;
float lastTrackerRfCourse=NAN,lastTrackerIsCourse=NAN;
uint32_t trackerRfSent=0,trackerIsSent=0,trackerRfFailures=0;
uint32_t trackerStatusLastRf=0,trackerStatusLastIs=0,trackerStatusRfAttempt=0;
uint32_t trackerStatusRfSent=0,trackerStatusIsSent=0;


struct Config {
  String call, ssid, wifiPassword, host, passcode, apPassword;
  uint16_t port;
  double latitude, longitude;
  uint16_t beaconMinutes;
  bool beaconEnabled;
  String symbolTable, symbolCode;
  String commentText, statusText;
  uint16_t statusMinutes;
  bool statusEnabled;
  bool autoDigi, wide1, wide2, aprsIsEnabled;
  String rfBeaconPath; // DIRECT, WIDE1-1, WIDE2-1, WIDE2-2
  uint8_t probeMinutes;
  uint8_t txPowerDbm;
  uint8_t oledType; // 0=SSD1306, 1=SH1106, 2=Kapali
  bool beaconBattery; // APRS comment sonuna Vbat bilgisi ekle.
  uint16_t oledSleepSeconds; // 0=zamanlama kapali (sürekli acik)
  float batteryCalibration; // Volt cinsinden artis/azalis; varsayilan 0.00V
  // Tracker DEV2: 0=off, 1=LoRa, 2=APRS-IS, 3=both. OFF by default.
  uint8_t trackerMode;
  String trackerCall, trackerTable, trackerSymbol, trackerComment;
  uint16_t trackerMinutes;
  bool trackerSmart;
  uint16_t trackerSlowKmh, trackerFastKmh, trackerSlowSeconds, trackerFastSeconds;
  // DEV4: independent tracker status and optional mobile iGate coordinates.
  bool trackerStatusEnabled=false, igateUseGps=false;
  uint8_t trackerStatusMode=0;  // 0=off, 1=RF, 2=APRS-IS, 3=both
  uint16_t trackerStatusMinutes=30;
  String trackerStatusText="TA1FM Mobile Tracker";
  bool messagingEnabled=false;
  bool messageBridgeEnabled=false; // opt-in: APRS-IS to known RF stations only
  bool telemetryEnabled=false;
  uint8_t telemetryMode=0; // 0 off, 1 RF, 2 APRS-IS, 3 both
  uint16_t telemetryMinutes=30;
  bool trackerTurnEnabled=false;
  uint16_t trackerTurnDegrees=45;
  uint16_t trackerTurnMinSeconds=30;

};
Config cfg;
float batteryVoltage=NAN;
uint32_t lastBatterySample=0;
bool batterySampled=false;
bool radioOk=false, oledOk=false, aprsVerified=false, loginSent=false;
bool oledSleeping=false;
uint32_t lastDisplayRxActivity=0;
String adminSaltHex, adminHashHex; // NVS'de sadece salt + SHA256 dogrulayicisi tutulur.
uint8_t securityFailures=0;
uint32_t securityBlockUntil=0;

String hexEncode(const uint8_t* b,size_t n){
  const char* digits="0123456789abcdef"; String out;out.reserve(n*2);
  for(size_t i=0;i<n;i++){out+=(char)digits[b[i]>>4];out+=(char)digits[b[i]&15];}
  return out;
}
String adminDigest(const String& saltHex,const String& password){
  // Salt'tan sonra parola; standart SHA-256, salt rastgele 128-bit.
  String input=saltHex+":"+password;
  uint8_t hash[32];
  mbedtls_sha256((const unsigned char*)input.c_str(),input.length(),hash,0);
  return hexEncode(hash,sizeof(hash));
}
bool constantEqual(const String& a,const String& b){
  if(a.length()!=b.length())return false;
  uint8_t diff=0;
  for(size_t i=0;i<a.length();i++)diff|=(uint8_t)(a[i]^b[i]);
  return diff==0;
}
bool checkAdminPassword(const String& password){
  if(adminSaltHex.length()==32 && adminHashHex.length()==64)
    return constantEqual(adminHashHex,adminDigest(adminSaltHex,password));
  return constantEqual(password,String(PANEL_PASSWORD)); // Ilk kurulum / eski surum.
}
void storeAdminPassword(const String& password){
  uint8_t salt[16];esp_fill_random(salt,sizeof(salt));
  String sh=hexEncode(salt,sizeof(salt));
  String hh=adminDigest(sh,password);
  prefs.putString("admsalt",sh);
  prefs.putString("admhash",hh);
  adminSaltHex=sh;adminHashHex=hh;
}
// HTTP Basic: sadece yetkili kullanici ve hash dogrulama.
bool validAdminAuthHeader(){
  String h=server.header("Authorization");
  if(!h.startsWith("Basic ") || h.length()>250)return false;
  String enc=h.substring(6);
  uint8_t out[160];size_t olen=0;
  if(mbedtls_base64_decode(out,sizeof(out)-1,&olen,(const unsigned char*)enc.c_str(),enc.length())!=0)return false;
  out[olen]=0;
  String credentials((const char*)out);
  int sep=credentials.indexOf(':');
  if(sep<1)return false;
  String user=credentials.substring(0,sep),pass=credentials.substring(sep+1);
  return constantEqual(user,String(PANEL_USER)) && checkAdminPassword(pass);
}

void displayPower(bool on){
  if(!oledOk || !oledDisplay || oledSleeping==!on)return;
  if(cfg.oledType==0)oledSsd.ssd1306_command(on?0xAF:0xAE);
  else oledSh.oled_command(on?0xAF:0xAE);
  oledSleeping=!on;
}
void onValidRxDisplay(){
  lastDisplayRxActivity=millis();
  if(oledSleeping)displayPower(true);
}
void maintainDisplaySleep(){
  if(!oledOk || cfg.oledType==2)return;
  if(cfg.oledSleepSeconds==0){if(oledSleeping)displayPower(true);return;}
  if((uint32_t)(millis()-lastDisplayRxActivity)>=((uint32_t)cfg.oledSleepSeconds*1000UL))
    displayPower(false);
}


String batteryLabel(){
  if(!isfinite(batteryVoltage)) return "---";
  return String(batteryVoltage,2)+"V";
}

// Batarya bolucusu sadece ADC okumasi suresince acik kalir.
// Olcumde 2.5-4.5 V araligi: Li-ion pil bagli degilse yaniltici veri gondermeyelim.
void sampleBattery(){
  digitalWrite(ADC_CTRL_PIN,ADC_CTRL_ON_STATE);
  delayMicroseconds(3000);
  uint32_t sum=0;
  for(int i=0;i<16;i++) sum+=(uint32_t)analogReadMilliVolts(BATTERY_PIN);
  digitalWrite(ADC_CTRL_PIN,!ADC_CTRL_ON_STATE);
  float raw=(sum/16.0f)*(BATTERY_DIVIDER_RATIO/1000.0f);
  float calibrated=raw+cfg.batteryCalibration;
  batteryVoltage=(isfinite(raw) && raw>=2.5f && raw<=4.5f &&
                  isfinite(calibrated) && calibrated>=2.5f && calibrated<=4.5f)?calibrated:NAN;
  lastBatterySample=millis();
  batterySampled=true;
}
void maintainBattery(){
  if(!batterySampled || (uint32_t)(millis()-lastBatterySample)>=BATTERY_SAMPLE_INTERVAL_MS) sampleBattery();
}

// APRS konum comment limitini korur; status mesaji degismez.
String beaconComment(){
  String comment=cfg.commentText;
  if(!cfg.beaconBattery || !isfinite(batteryVoltage))return comment;
  String suffix="Vbat="+String(batteryVoltage,2)+"V";
  if(suffix.length()>43)return comment;
  const size_t maxBase=43-suffix.length()-1;
  if(comment.length()>maxBase)comment.remove(maxBase);
  comment.trim();
  return comment.isEmpty()?suffix:comment+" "+suffix;
}

volatile bool rxFlag=false;
void IRAM_ATTR onRadioRx() { rxFlag=true; }

bool apEnabled=false;
String lastStation="---", aprsLine="";
uint32_t countRx=0, countValid=0, countSent=0, countDup=0, countDropped=0;
uint32_t lastScreen=0, lastWifiAttempt=0, lastAprsAttempt=0, disconnectedSince=0, lastKeepalive=0;
uint32_t aprsConnectStarted=0, lastBeacon=0;
bool beaconDueOnConnect=true;
uint32_t countBeacon=0, countStatus=0;
uint32_t lastStatus=0;
bool statusDueOnConnect=true;
uint32_t lastRxAt=0, countRejected=0;
float lastRssi=0, lastSnr=0;
// Auto Digi state: APRS-IS service health, NOT just Wi-Fi association.
bool digiMode=false;
uint32_t bootToken=0, restartAt=0;
bool bootRfPending=true;
RTC_DATA_ATTR uint8_t rapidBootCount=0;
static constexpr uint32_t BOOT_RF_WAIT_MS=10000UL;
String manualRfResult="", manualIsResult="";
bool otaFailed=false, otaStarted=false, otaSuccessful=false;
String otaError="";
uint32_t aprsDownFrom=0, lastDigiProbe=0;
uint32_t countDigiTx=0, countDigiBusy=0, countDigiLimit=0, countDigiFailed=0;
uint32_t lastDigiTx=0, txWindowStart=0;
// RF beacon has its own schedule but shares TX cooldown and hourly quota with relay TX.
uint32_t lastRfBeacon=0, lastRfBeaconAttempt=0, countRfBeacon=0, countRfBeaconFail=0;
bool rfBeaconDue=false;
// Web requests only schedule work; radio/TC P operations stay in loop().
bool manualRfPending=false, manualIsPending=false;
uint32_t lastManualRfRequest=0;
uint16_t txWindowCount=0;
// Only one pending relay, avoids uncontrolled packet accumulation when busy.
String digiPending="";
uint32_t digiPendingAt=0;
static constexpr uint32_t DIGI_AFTER_MS=60000UL;
static constexpr uint32_t TX_GAP_MS=30000UL;
static constexpr uint16_t TX_MAX_PER_HOUR=20;
// Web panelinden ayarlanir; varsayilan 10 dBm, bu kart icin ust sinir 21 dBm.


struct RxRecord { String station, packet, action; float rssi=0,snr=0; uint32_t receivedAt=0; };
RxRecord rxRecords[15];
uint8_t rxHead=0,rxCount=0;
struct EventRecord { String message; uint32_t at=0; };
EventRecord events[24];
uint8_t eventHead=0,eventCount=0;
void addEvent(const String& msg){events[eventHead].message=msg;events[eventHead].at=millis();eventHead=(eventHead+1)%24;if(eventCount<24)eventCount++;Serial.println("[OLAY] "+msg);}
void recordRx(const String& call,const String& packet,float rssi,float snr,const String& action){
  RxRecord &r=rxRecords[rxHead];r.station=call;r.packet=packet;r.rssi=rssi;r.snr=snr;r.action=action;r.receivedAt=millis();
  rxHead=(rxHead+1)%15;if(rxCount<15)rxCount++;
}
void updateRecentAction(const String& packet,const String& action){
  for(int i=0;i<rxCount;i++){
    uint8_t idx=(rxHead+15-1-i)%15;
    if(rxRecords[idx].packet==packet){rxRecords[idx].action=action;break;}
  }
}
String jsonEscape(const String& s){
  String r="\"";
  for(size_t i=0;i<s.length();i++){
    unsigned char c=s[i];
    if(c=='\"'||c=='\\'){r+='\\';r+=(char)c;}
    else if(c=='\n')r+="\\n";
    else if(c=='\r')r+="\\r";
    else if(c=='\t')r+="\\t";
    else if(c<32){char b[7];snprintf(b,sizeof(b),"\\u%04X",c);r+=b;}
    else r+=(char)c;
  }
  return r+"\"";
}
String rxClock(uint32_t at){
  // NTP time is optional; records fall back to device uptime.
  time_t t=time(nullptr);
  if(t>1700000000){t-=(millis()-at)/1000;struct tm tmv;localtime_r(&t,&tmv);char b[24];strftime(b,sizeof(b),"%H:%M:%S",&tmv);return String(b);}
  uint32_t secs=at/1000;char b[24];snprintf(b,sizeof(b),"+%02lu:%02lu:%02lu",(unsigned long)(secs/3600),(unsigned long)((secs/60)%60),(unsigned long)(secs%60));return String(b);
}
struct Pending { uint8_t line[300]; uint16_t len=0; };
static Pending queueData[12];
uint8_t queueHead=0, queueTail=0, queueCount=0;
struct Seen { uint32_t hash=0, time=0; };
Seen seen[24];
uint8_t seenPtr=0;

String htmlEscape(const String& s) {
  String r;
  for(size_t i=0;i<s.length();i++) {
    char c=s[i];
    if(c=='&') r+="&amp;";
    else if(c=='<') r+="&lt;";
    else if(c=='>') r+="&gt;";
    else if(c=='\"') r+="&quot;";
    else if(c=='\'') r+="&#39;";
    else r+=c;
  }
  return r;
}

bool validAprsText(const String& value, size_t limit) {
  if(value.length()>limit) return false;
  for(size_t i=0;i<value.length();i++) {
    uint8_t c=(uint8_t)value[i];
    if(c<32 || c>126 || c=='|' || c=='~') return false;
  }
  return true;
}

void loadConfig(){
  prefs.begin("derman-aprs",false);
  cfg.call=prefs.getString("call","TA1XXX-10");
  cfg.apPassword=prefs.getString("apkey",DEFAULT_AP_PASSWORD);
  if(cfg.apPassword.length()<8 || cfg.apPassword.length()>63)cfg.apPassword=DEFAULT_AP_PASSWORD;
  cfg.ssid=prefs.getString("ssid","");
  cfg.wifiPassword=prefs.getString("wifi","");
  cfg.host=prefs.getString("host","euro.aprs2.net");
  cfg.passcode=prefs.getString("pass","");
  cfg.port=prefs.getUShort("port",14580);
  cfg.latitude=prefs.getDouble("lat",0);
  cfg.longitude=prefs.getDouble("lon",0);
  cfg.beaconMinutes=prefs.getUShort("bmin",30);
  if(cfg.beaconMinutes<10 || cfg.beaconMinutes>120)cfg.beaconMinutes=30;
  cfg.beaconEnabled=prefs.getBool("bena",false);
  cfg.symbolTable=prefs.getString("symtab","L");
  cfg.symbolCode=prefs.getString("symcode","&");
  if(cfg.symbolTable.length()!=1 || cfg.symbolCode.length()!=1){cfg.symbolTable="L";cfg.symbolCode="&";}
  cfg.commentText=prefs.getString("comment","TA1FM LoRa iGate");
  cfg.statusText=prefs.getString("status","TA1FM LoRa APRS iGate &Digi");
  cfg.statusEnabled=prefs.getBool("stena",true);
  cfg.statusMinutes=prefs.getUShort("stmin",60);
  if(cfg.statusMinutes<15 || cfg.statusMinutes>240) cfg.statusMinutes=60;
  // Defensive cleanup of any previously stored invalid data.
  if(cfg.commentText.length()>43) cfg.commentText.remove(43);
  if(cfg.statusText.length()>62) cfg.statusText.remove(62);
  cfg.autoDigi=prefs.getBool("autodigi",false);
  cfg.aprsIsEnabled=prefs.getBool("isenabled",true);
  cfg.wide1=prefs.getBool("wide1",true);
  cfg.wide2=prefs.getBool("wide2",false);
  cfg.rfBeaconPath=prefs.getString("rfpath","WIDE1-1");
  if(cfg.rfBeaconPath!="DIRECT" && cfg.rfBeaconPath!="WIDE1-1" && cfg.rfBeaconPath!="WIDE2-1" && cfg.rfBeaconPath!="WIDE2-2") cfg.rfBeaconPath="WIDE1-1";
  cfg.txPowerDbm=prefs.getUChar("txpwr",10);
  if(cfg.txPowerDbm<10 || cfg.txPowerDbm>21) cfg.txPowerDbm=10;
  cfg.oledType=prefs.getUChar("oledtype",0);
  if(cfg.oledType>2)cfg.oledType=0;
  cfg.beaconBattery=prefs.getBool("batbeacon",false);
  cfg.batteryCalibration=prefs.getFloat("batcal",0.0f);
  if(!isfinite(cfg.batteryCalibration) || cfg.batteryCalibration < -1.0f || cfg.batteryCalibration > 1.0f) cfg.batteryCalibration=0.0f;
  cfg.oledSleepSeconds=prefs.getUShort("oledsleep",0);
  if(cfg.oledSleepSeconds!=0 && cfg.oledSleepSeconds!=15 && cfg.oledSleepSeconds!=30 &&
     cfg.oledSleepSeconds!=60 && cfg.oledSleepSeconds!=300 && cfg.oledSleepSeconds!=600)
     cfg.oledSleepSeconds=0;
  cfg.messagingEnabled=prefs.getBool("msgena",false);cfg.messageBridgeEnabled=prefs.getBool("msgbridge",false);
  cfg.telemetryEnabled=prefs.getBool("tlmena",false);
  cfg.telemetryMode=prefs.getUChar("tlmmode",0);if(cfg.telemetryMode>3)cfg.telemetryMode=0;
  cfg.telemetryMinutes=prefs.getUShort("tlmmin",30);if(cfg.telemetryMinutes<15 || cfg.telemetryMinutes>240)cfg.telemetryMinutes=30;
  cfg.trackerTurnEnabled=prefs.getBool("trkturn",false);
  cfg.trackerTurnDegrees=prefs.getUShort("turnang",45);
  cfg.trackerTurnMinSeconds=prefs.getUShort("turnsec",30);
  if(cfg.trackerTurnDegrees<15 || cfg.trackerTurnDegrees>150)cfg.trackerTurnDegrees=45;
  if(cfg.trackerTurnMinSeconds<15 || cfg.trackerTurnMinSeconds>300)cfg.trackerTurnMinSeconds=30;
  for(int i=0;i<4;i++){extraWifiSsid[i]=prefs.getString(("w"+String(i)+"s").c_str(),"");extraWifiPass[i]=prefs.getString(("w"+String(i)+"p").c_str(),"");}
  cfg.trackerMode=prefs.getUChar("trkmode",0);
  if(cfg.trackerMode>3)cfg.trackerMode=0;
  cfg.trackerCall=prefs.getString("trkcall",""); // Set distinct SSID explicitly in panel
  cfg.trackerTable=prefs.getString("trktab","/");
  cfg.trackerSymbol=prefs.getString("trksym",">");
  cfg.trackerComment=prefs.getString("trkcomm","TA1FM LoRa Tracker");
  cfg.trackerMinutes=prefs.getUShort("trkmin",15);
  if(cfg.trackerMinutes<1 || cfg.trackerMinutes>120)cfg.trackerMinutes=15;
  cfg.trackerSmart=prefs.getBool("trksmart",false);
  cfg.trackerSlowKmh=prefs.getUShort("trkslowv",5);
  cfg.trackerFastKmh=prefs.getUShort("trkfastv",80);
  cfg.trackerSlowSeconds=prefs.getUShort("trkslowt",900);
  cfg.trackerFastSeconds=prefs.getUShort("trkfastt",60);
  if(cfg.trackerFastKmh<=cfg.trackerSlowKmh || cfg.trackerFastKmh>200)cfg.trackerFastKmh=80;
  if(cfg.trackerSlowKmh>=cfg.trackerFastKmh)cfg.trackerSlowKmh=5;
  if(cfg.trackerSlowSeconds<30 || cfg.trackerSlowSeconds>7200)cfg.trackerSlowSeconds=900;
  if(cfg.trackerFastSeconds<15 || cfg.trackerFastSeconds>1800)cfg.trackerFastSeconds=60;
  cfg.trackerStatusEnabled=prefs.getBool("tstena",false);
  cfg.trackerStatusMode=prefs.getUChar("tstmode",0);
  if(cfg.trackerStatusMode>3)cfg.trackerStatusMode=0;
  cfg.trackerStatusMinutes=prefs.getUShort("tstmin",30);
  if(cfg.trackerStatusMinutes<1 || cfg.trackerStatusMinutes>240)cfg.trackerStatusMinutes=30;
  cfg.trackerStatusText=prefs.getString("tsttext","TA1FM Mobile Tracker");
  if(!validAprsText(cfg.trackerStatusText,62))cfg.trackerStatusText="TA1FM Mobile Tracker";
  cfg.igateUseGps=prefs.getBool("igategps",false);
  adminSaltHex=prefs.getString("admsalt","");
  adminHashHex=prefs.getString("admhash","");
  cfg.probeMinutes=prefs.getUChar("probemin",10);
  if(cfg.probeMinutes!=1 && cfg.probeMinutes!=5 && cfg.probeMinutes!=10 && cfg.probeMinutes!=15)cfg.probeMinutes=10;

}

bool validCall(String call){
  call.trim(); call.toUpperCase();
  int dash=call.indexOf('-');
  String base=dash<0 ? call:call.substring(0,dash);
  if(base.length()<3 || base.length()>6) return false;
  for(size_t i=0;i<base.length();i++) if(!isalnum((unsigned char)base[i])) return false;
  if(dash>=0){
    String ssid=call.substring(dash+1);
    if(ssid.isEmpty() || ssid.length()>2) return false;
    for(size_t i=0;i<ssid.length();i++) if(ssid[i]<'0'||ssid[i]>'9') return false;
    if(ssid.toInt()>15) return false;
  }
  return true;
}

void screenUpdate(){
  if(!oledOk || !oledDisplay || oledSleeping)return;
  Adafruit_GFX &d=*oledDisplay;
  if(cfg.oledType==0)oledSsd.clearDisplay();
  else oledSh.clearDisplay();
  d.setTextColor(1);d.setTextWrap(false);
  // Cagri isareti solda; pil voltaji sagda (2x fontu pil alani icin daraltiyoruz).
  d.setTextSize(1);d.setCursor(0,1);
  d.print(cfg.call.substring(0,12));
  d.setCursor(96,1);d.print(batteryLabel());
  d.drawLine(0,18,127,18,1);
  d.setCursor(0,23);d.print("WiFi: ");d.print(WiFi.status()==WL_CONNECTED?"OK":"YOK");
  d.setCursor(65,23);d.print("IS: ");
  d.print(cfg.aprsIsEnabled && WiFi.status()==WL_CONNECTED && aprs.connected() && aprsVerified?"OK":"YOK");
  d.setCursor(0,35);d.print("IP: ");
  if(WiFi.status()==WL_CONNECTED)d.print(WiFi.localIP().toString());
  else if(apEnabled)d.print(WiFi.softAPIP().toString());
  else d.print("---");
  d.setCursor(0,49);d.print("SON RX: ");d.print(lastStation.substring(0,15));
  if(cfg.oledType==0)oledSsd.display();
  else oledSh.display();
}

void beginDisplay(){
  oledOk=false;oledDisplay=nullptr;oledSleeping=false;lastDisplayRxActivity=millis();
  Wire1.begin(PIN_SDA,PIN_SCL);Wire1.setClock(100000);Wire1.setTimeOut(50);
  if(cfg.oledType==2){Serial.println("[OLED] Panelden KAPALI");return;}
  if(cfg.oledType==0){
    oledOk=oledSsd.begin(SSD1306_SWITCHCAPVCC,0x3C);
    if(oledOk)oledDisplay=&oledSsd;
  }else{
    oledOk=oledSh.begin(0x3C,true);
    if(oledOk)oledDisplay=&oledSh;
  }
  Serial.printf("[OLED] %s: %s\n",cfg.oledType==0?"SSD1306":"SH1106",oledOk?"HAZIR":"BASLATILAMADI");
}

void startAp(){
  if(apEnabled) return;
  if(WiFi.status()==WL_CONNECTED) return;
  WiFi.mode(WIFI_AP_STA);
  apEnabled=WiFi.softAP("TA1FM-APRS-SETUP",cfg.apPassword.c_str());
  if(apEnabled) Serial.printf("Kurulum AP: TA1FM-APRS-SETUP / IP: %s\n", WiFi.softAPIP().toString().c_str());
}
void stopAp(){
  if(!apEnabled) return;
  WiFi.softAPdisconnect(true); apEnabled=false;
  WiFi.mode(WIFI_STA);
}
String wifiName(uint8_t n){return n==0?cfg.ssid:extraWifiSsid[n-1];}
String wifiSecret(uint8_t n){return n==0?cfg.wifiPassword:extraWifiPass[n-1];}
void wifiTry(uint8_t n){
  String name=wifiName(n);if(name.isEmpty())return;
  wifiCandidate=n;WiFi.disconnect(false,false);
  WiFi.begin(name.c_str(),wifiSecret(n).c_str());
  lastWifiAttempt=millis();
  Serial.printf("[WIFI] Aday %u: %s\n",(unsigned)n,name.c_str());
}
void wifiTryNext(){
  for(uint8_t i=1;i<=5;i++){uint8_t n=(wifiCandidate+i)%5;if(wifiName(n).length()){wifiTry(n);return;}}
}
void beginWifi(){
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if(cfg.ssid.length()) {
    wifiTry(0); disconnectedSince=millis();
  } else {wifiCandidate=4;wifiTryNext();if(WiFi.status()!=WL_CONNECTED)startAp();}
}
void maintainWifi(){
  if(WiFi.status()==WL_CONNECTED){
    disconnectedSince=0;
    if(apEnabled) stopAp();
    return;
  }
  if(disconnectedSince==0) disconnectedSince=millis();
  bool anyWifi=cfg.ssid.length();for(int i=0;i<4;i++)anyWifi|=extraWifiSsid[i].length()>0;
  if(!anyWifi){startAp();return;}
  if(millis()-lastWifiAttempt>=15000){
    lastWifiAttempt=millis();
    wifiTryNext();
  }
  if(millis()-disconnectedSince>=90000) startAp();
}

bool accessAllowed(){
  if(securityBlockUntil && (int32_t)(millis()-securityBlockUntil)<0){
    server.send(429,"text/plain; charset=utf-8","Cok fazla hatali giris. Biraz sonra tekrar deneyin.");return false;
  }
  if(validAdminAuthHeader()){securityFailures=0;securityBlockUntil=0;return true;}
  if(++securityFailures>=10){securityFailures=0;securityBlockUntil=millis()+30000UL;}
  server.requestAuthentication();return false;
}

String pageHeader(const String& title){
  String p=R"HTML(<!doctype html><html lang="tr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>TA1FM APRS</title><style>
  body{font:14px system-ui,Arial;background:#101924;color:#e9f0f6;margin:0;padding:10px 16px 24px;line-height:1.5}
  main{max-width:900px;margin:auto}.card{background:#192534;border:1px solid #29394a;border-radius:12px;padding:17px 19px;margin:12px 0;box-shadow:0 3px 12px #0001}
  h1{font-size:23px}h2{font-size:18px}a{color:#7dd3fc}label{display:block;margin:12px 0 5px}
  input{box-sizing:border-box;width:100%;padding:9px 11px;font-size:14px;border:1px solid #3a4b5c;border-radius:7px;background:#101d2b;color:#eef7fc;outline:none}input:focus,select:focus{border-color:#4ea8ca}input[type=checkbox]{width:auto;accent-color:#36b4c9}
  button{cursor:pointer;background:#168aab;border:1px solid #2599b7;border-radius:7px;color:#fff;padding:9px 14px;margin-top:12px;font-weight:600}button:hover{background:#209eba}
  .muted{color:#acbacb}.ok{color:#74e4a2}.bad{color:#ffbb7a}small{display:block;margin:8px 0}
  .topbar{position:sticky;top:0;z-index:15;background:#101924f5;backdrop-filter:blur(8px);padding:12px 0 10px;border-bottom:1px solid #324b5d;margin-bottom:13px}
  .brand{font-weight:700;font-size:17px;margin-bottom:8px;color:#f3fbff;letter-spacing:.2px}
  .navitems{display:flex;gap:5px;flex-wrap:wrap}
  .navitem{display:inline-flex;align-items:center;justify-content:center;gap:4px;text-decoration:none;color:#b5c8d5;background:#192737;border:1px solid #324456;border-radius:7px;padding:7px 9px;font-size:12px;font-weight:550;white-space:nowrap}
  .navitem:hover{background:#2b4154;border-color:#63bfe8}.navitem.active{background:#087ba0;border-color:#53cafa;color:#fff}
  .actions{display:flex;flex-wrap:wrap;align-items:center;gap:9px;margin-top:14px}
  .actions button{margin-top:0;flex:1 1 150px}.actions button.secondary{background:#274761}
  .actions button:disabled{background:#37434b;color:#a8b5c1;opacity:.65;cursor:not-allowed}
  @media(max-width:500px){.navitem{flex:1 1 110px}.brand{font-size:18px}}
  .links a{display:inline-block;margin:5px 12px 5px 0}.notice{position:fixed;right:18px;top:18px;z-index:20;max-width:350px;border:1px solid #32566b;border-radius:10px;padding:14px;background:#183249;color:#fff;box-shadow:0 8px 25px #0009;display:none}
  .notice.error{background:#5b2830}.notice.success{background:#194a3b}.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
  select{box-sizing:border-box;width:100%;padding:9px 11px;background:#101d2b;color:#eef7fc;border:1px solid #3a4b5c;border-radius:7px}
  table{border-collapse:collapse;width:100%;font-size:13px}td,th{padding:9px 6px;border-bottom:1px solid #385064;text-align:left}
  td.info{word-break:break-all}.subtle{font-size:13px;color:#a8bfd0}
  .panel-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px;align-items:start;margin-top:14px}
  .panel-grid .card{margin:0;min-width:0}.panel-grid h2{font-size:16px;margin:0 0 10px}
  .panel-grid input{padding:8px 10px}.panel-grid label{font-size:13px;margin-top:9px}
  .panel-grid button{padding:8px 12px}.panel-grid .subtle{font-size:12px}
  .sys-heading{font-weight:650;font-size:19px;color:#ecf9ff;margin:15px 0 5px}
  .sys-tag{font-size:11px;color:#81c8d7;border:1px solid #285a65;padding:3px 8px;border-radius:20px}
  .system-choices{display:flex;flex-direction:column;gap:7px}.system-choices label{display:flex;gap:10px;align-items:flex-start;padding:9px;border:1px solid #385065;border-radius:8px;margin:0}
  .system-choices input{width:auto;flex:none;margin-top:3px}.system-choices small{margin:0}
  .system-col{display:flex;flex-direction:column;gap:14px;min-width:0}
  /* Sistem sayfasinda iki sutunun alt kenarlarini esitle. */
  .system-layout{align-items:stretch}
  .system-layout>.system-col:first-child>.card:last-child{flex:1}
  .quick-links{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px;margin-top:12px}
  .quick-links a{display:flex;align-items:center;justify-content:center;min-height:40px;padding:8px 5px;box-sizing:border-box;border:1px solid #356279;border-radius:8px;background:#143449;color:#e4faff;text-decoration:none;font-size:13px;font-weight:600;text-align:center}
  .quick-links a:hover{background:#126788;border-color:#59cbe6}
  .quick-links a.primary{background:#1287a5;border-color:#37b8db;color:white}
  .quick-links a.full{grid-column:1/-1}
  .system-status{display:grid;grid-template-columns:1fr 1fr;gap:8px}.system-status div{padding:8px;border:1px solid #34495c;border-radius:8px;font-size:12px;overflow-wrap:anywhere}
  @media(max-width:720px){.panel-grid{grid-template-columns:1fr}.system-status{grid-template-columns:1fr 1fr}}
  </style></head><body><div id='notice' class='notice' role='status' aria-live='polite'></div><main>)HTML";
  p+=R"JS(<script>
let toastTimer;
function toast(msg,type='success',ms=4500){const x=document.getElementById('notice');x.textContent=msg;x.className='notice '+type;x.style.display='block';clearTimeout(toastTimer);if(ms)toastTimer=setTimeout(()=>{x.style.display='none'},ms);}
async function postAction(url){
  if(url==='/beacon/rf'&&!confirm('Gercek RF yayini yapilacak. Onayliyor musunuz?'))return;
  try{const r=await fetch(url,{method:'POST',cache:'no-store'});const t=await r.text();
    toast(t,r.ok?'success':'error',r.ok?0:9000);if(!r.ok)return;
    const key=url.endsWith('/rf')?'rf':'is';
    for(let i=0;i<35;i++){
      await new Promise(resolve=>setTimeout(resolve,1000));
      const a=await fetch('/api/action-result?'+Date.now(),{cache:'no-store'}),x=await a.json();
      if(x[key]==='OK'){toast('Beacon basariyla gonderildi.','success');return;}
      if(x[key]==='HATA'){toast('Beacon gonderilemedi. RF kanal veya baglanti kontrol edin.','error',9000);return;}
    }
    toast('Beacon hala bekliyor veya zaman asimi. Seri monitor / olaylara bakin.','error',9000);
  }catch(e){toast('Cihaza erisilemiyor: '+e,'error',9000)}
}
async function checkReboot(oldToken){
  toast('Cihaz yeniden baslatiliyor / baglanti kontrol ediliyor...','success',0);
  for(let i=0;i<90;i++){
    await new Promise(r=>setTimeout(r,1000));
    try{const r=await fetch('/health?'+Date.now(),{cache:'no-store'});
      if(!r.ok)continue;
      const x=await r.json();
      if(x.boot!==oldToken){sessionStorage.setItem('ta1fm_notice','Yeniden baglandi - sistem hazir.');location.reload();return;}
    }catch(e){}
  }
  toast('Yeniden baglanti saglanamadi. Wi-Fi / IP degismisse yeni adresten acin.','error',0);
}
window.addEventListener('DOMContentLoaded',()=>{const m=sessionStorage.getItem('ta1fm_notice');if(m){sessionStorage.removeItem('ta1fm_notice');toast(m)}});
</script>)JS";
  String active="/";
  if(title.startsWith("iGate&Digi")) active="/settings";
  else if(title.startsWith("Son 15 RX")) active="/rx";
  else if(title.startsWith("Son olaylar")) active="/events";
  else if(title.startsWith("OTA")) active="/ota";
  else if(title.startsWith("GPS /")) active="/gps";
  else if(title.startsWith("Tracker /")) active="/tracker";
  else if(title.startsWith("Telemetri /")) active="/telemetry";
  else if(title.startsWith("WiFi /")) active="/wifi";
  else if(title.startsWith("Mesajlasma /")) active="/messages";
  else if(title.startsWith("Sistem /")) active="/system";
  else if(title.startsWith("Yedek /")) active="/backup";
  p+="<header class='topbar'><div class='brand'>&#128225; TA1FM LoRa APRS iGate & Digi</div><nav class='navitems' aria-label='Sayfa menusu'>";
  for(const char* path : {"/","/settings","/tracker","/messages","/telemetry","/wifi","/system","/backup"}){
    String dest=path;
    String label=dest=="/"?"&#8962; Ana Panel":dest=="/settings"?"&#128225; iGate&amp;Digi":dest=="/tracker"?"&#9678; Tracker":dest=="/messages"?"&#9993; Mesajlar":dest=="/telemetry"?"&#9638; Telemetri":dest=="/wifi"?"&#128246; Wi-Fi":dest=="/system"?"&#9881; Sistem":"&#128190; Yedek";
    p+="<a class='navitem"+String(active==dest?" active":"")+"' href='"+dest+"'"+String(active==dest?" aria-current='page'":"")+">"+label+"</a>";
  }
  p+="</nav></header><p class='muted'>"+htmlEscape(title)+"</p>";
  return p;
}
// GPS polling is bounded, so incoming LoRa packets keep priority.
void pollGPS(){
  uint16_t budget=128;
  while(budget-- && gpsSerial.available()){
    char c=(char)gpsSerial.read();
    ++gpsCharsSeen;
    gps.encode(c);
  }
  if(gps.location.isUpdated() && gps.location.isValid())gpsLastFreshAt=millis();
}
bool gpsFixFresh(){
  return gps.location.isValid() && gps.location.age()<10000UL;
}
bool gpsFixFresh();
// Tracker settings are isolated from legacy /settings save and its backup format.
void handleTrackerPage(){
  if(!accessAllowed())return;
  String p=pageHeader("Tracker / GPS Beacon");
  p+="<div class='card'><h2>GPS Tracker</h2><p>iGate ve Digi ayarlari bagimsizdir. Tracker varsayilan olarak kapalidir.</p>";
  p+="<form method='POST' action='/tracker/save'>";
  p+="<label>Beacon kanali</label><select name='mode'>";
  const char* options[]={"Kapali","LoRa RF","APRS-IS","Ikisi de"};
  for(int i=0;i<4;i++)p+="<option value='"+String(i)+"' "+String(cfg.trackerMode==i?"selected":"")+">"+options[i]+"</option>";
  p+="</select>";
  p+="<label>Tracker cagri isareti (iGate'den farkli SSID)</label><input name='call' maxlength='9' value='"+htmlEscape(cfg.trackerCall)+"' placeholder='Ornek: TA1FM-11'>";
  p+="<label>APRS sembol tablosu (/ veya \\)</label><input name='table' maxlength='1' value='"+htmlEscape(cfg.trackerTable)+"'>";
  p+="<label>APRS sembol karakteri (arac: &gt;)</label><input name='symbol' maxlength='1' value='"+htmlEscape(cfg.trackerSymbol)+"'>";
  p+="<label>Comment (ASCII, max 43)</label><input name='comment' maxlength='43' value='"+htmlEscape(cfg.trackerComment)+"'>";
  p+="<label>Sabit beacon araligi (dk: 1-120)</label><input type='number' name='minutes' min='1' max='120' value='"+String(cfg.trackerMinutes)+"'>";
  p+="<label><input type='checkbox' style='width:auto' name='smart' value='1' "+String(cfg.trackerSmart?"checked":"")+"> Hiza gore SmartBeaconing</label>";
  p+="<label>Yavas hiz esigi (km/sa)</label><input type='number' name='slowv' min='0' max='100' value='"+String(cfg.trackerSlowKmh)+"'>";
  p+="<label>Yuksek hiz esigi (km/sa)</label><input type='number' name='fastv' min='1' max='200' value='"+String(cfg.trackerFastKmh)+"'>";
  p+="<label>Yavas hiz beacon araligi (sn)</label><input type='number' name='slowt' min='30' max='7200' value='"+String(cfg.trackerSlowSeconds)+"'>";
  p+="<label>Yuksek hiz beacon araligi (sn)</label><input type='number' name='fastt' min='15' max='1800' value='"+String(cfg.trackerFastSeconds)+"'>";
  p+="<h3>Donus algilama (SmartBeaconing)</h3>";
  p+="<label><input type='checkbox' style='width:auto' name='turnon' value='1' "+String(cfg.trackerTurnEnabled?"checked":"")+"> Yon degisiminde beacon iste</label>";
  p+="<label>Donus acisi (15-150 derece)</label><input type='number' name='turnangle' min='15' max='150' value='"+String(cfg.trackerTurnDegrees)+"'>";
  p+="<label>Donus sonrasi en kisa sure (15-300 sn)</label><input type='number' name='turnsec' min='15' max='300' value='"+String(cfg.trackerTurnMinSeconds)+"'>";
  p+="<h3>Tracker Status (ayri paket)</h3>";
  p+="<label><input type='checkbox' style='width:auto' name='statuson' value='1' "+String(cfg.trackerStatusEnabled?"checked":"")+"> Status aktif</label>";
  p+="<label>Status metni (ASCII, max 62)</label><input name='statustext' maxlength='62' value='"+htmlEscape(cfg.trackerStatusText)+"'>";
  p+="<label>Status gonderim yolu</label><select name='statusmode'>";
  const char* smodes[]={"Kapali","LoRa RF","APRS-IS","Ikisi de"};
  for(int i=0;i<4;i++)p+="<option value='"+String(i)+"' "+String(cfg.trackerStatusMode==i?"selected":"")+">"+smodes[i]+"</option>";
  p+="</select><label>Status araligi (dk, 1-240)</label><input type='number' name='statusmin' min='1' max='240' value='"+String(cfg.trackerStatusMinutes)+"'>";
  p+="<button type='submit'>Tracker Ayarlarini Kaydet</button></form>";
  p+="<small>GPS FIX kaybolursa tracker yayin yapmaz. LoRa yayinlari mevcut CAD, TX limitleri ve OCP korumalari ile sinirlanir. APRS-IS icin internet ve dogrulanmis baglanti gerekir. SmartBeaconing hiza ve secime bagli GPS yon degisimine gore calisir.</small>";
  p+="<p>GPS: <b>"+String(gpsFixFresh()?"FIX OK":"FIX YOK")+"</b> | RF TX: "+String(trackerRfSent)+" | IS TX: "+String(trackerIsSent)+" | RF Hata: "+String(trackerRfFailures)+"</p></div>";
  p+="</main></body></html>";server.send(200,"text/html; charset=utf-8",p);
}
void handleTrackerSave(){
  if(!accessAllowed())return;
  int mode=server.arg("mode").toInt();
  String call=server.arg("call");call.trim();call.toUpperCase();
  String tab=server.arg("table"),sym=server.arg("symbol"),comm=server.arg("comment");
  int mins=server.arg("minutes").toInt(),sv=server.arg("slowv").toInt(),fv=server.arg("fastv").toInt();
  int st=server.arg("slowt").toInt(),ft=server.arg("fastt").toInt();
  bool smart=server.hasArg("smart");
  bool turnEnabled=server.hasArg("turnon");
  int turnDegrees=server.arg("turnangle").toInt(),turnSeconds=server.arg("turnsec").toInt();
  bool statusOn=server.hasArg("statuson");
  int statusMode=server.arg("statusmode").toInt();
  int statusMin=server.arg("statusmin").toInt();
  String statusText=server.arg("statustext");
  if(!server.hasArg("mode") || mode<0 || mode>3 ||
     (mode && (!validCall(call) || call==cfg.call)) ||
     (call.length() && !validCall(call)) ||
     tab.length()!=1 || (tab!="/" && tab!="\\") ||
     sym.length()!=1 || sym[0]<33 || sym[0]>126 || sym[0]=='<' ||
     !validAprsText(comm,43) || mins<1 || mins>120 ||
     statusMode<0 || statusMode>3 || statusMin<1 || statusMin>240 ||
     !validAprsText(statusText,62) ||
     turnDegrees<15 || turnDegrees>150 || turnSeconds<15 || turnSeconds>300 ||
     sv<0 || sv>100 || fv<=sv || fv>200 || st<30 || st>7200 || ft<15 || ft>1800 || ft>st){
    server.send(400,"text/plain; charset=utf-8","Tracker ayarlari gecersiz. Cagri isareti iGate'den farkli olmali.");return;
  }
  cfg.trackerTurnEnabled=turnEnabled;cfg.trackerTurnDegrees=turnDegrees;cfg.trackerTurnMinSeconds=turnSeconds;
  prefs.putBool("trkturn",turnEnabled);prefs.putUShort("turnang",turnDegrees);prefs.putUShort("turnsec",turnSeconds);
  cfg.trackerMode=mode;cfg.trackerCall=call;cfg.trackerTable=tab;cfg.trackerSymbol=sym;
  cfg.trackerComment=comm;cfg.trackerMinutes=mins;cfg.trackerSmart=smart;
  cfg.trackerStatusEnabled=statusOn;cfg.trackerStatusMode=statusMode;
  cfg.trackerStatusMinutes=statusMin;cfg.trackerStatusText=statusText;
  prefs.putBool("tstena",statusOn);prefs.putUChar("tstmode",statusMode);
  prefs.putUShort("tstmin",statusMin);prefs.putString("tsttext",statusText);
  cfg.trackerSlowKmh=sv;cfg.trackerFastKmh=fv;cfg.trackerSlowSeconds=st;cfg.trackerFastSeconds=ft;
  prefs.putUChar("trkmode",mode);prefs.putString("trkcall",call);
  prefs.putString("trktab",tab);prefs.putString("trksym",sym);prefs.putString("trkcomm",comm);
  prefs.putUShort("trkmin",mins);prefs.putBool("trksmart",smart);
  prefs.putUShort("trkslowv",sv);prefs.putUShort("trkfastv",fv);
  prefs.putUShort("trkslowt",st);prefs.putUShort("trkfastt",ft);
  trackerLastRf=millis();trackerLastIs=millis();
  trackerStatusLastRf=millis();trackerStatusLastIs=millis();
  server.sendHeader("Location","/tracker");server.send(303,"text/plain","Kaydedildi");
}

String gpsJson(){
  String j=String("{\"connected\":")+(gpsCharsSeen?"true":"false")+
    ",\"fix\":"+(gpsFixFresh()?"true":"false")+
    ",\"satellites\":"+String(gps.satellites.isValid()?gps.satellites.value():0)+
    ",\"age_ms\":"+String(gps.location.isValid()?gps.location.age():0)+
    ",\"lat\":"+String(gpsFixFresh()?gps.location.lat():0,6)+
    ",\"lon\":"+String(gpsFixFresh()?gps.location.lng():0,6)+
    ",\"speed_kmh\":"+String(gps.speed.isValid()?gps.speed.kmph():0,1)+
    ",\"course_deg\":"+String(gps.course.isValid()?gps.course.deg():0,1)+
    ",\"chars\":"+String(gpsCharsSeen)+"}";
  return j;
}
void handleGpsPage(){
  if(!accessAllowed())return;
  String p=pageHeader("GPS / GNSS durumu");
  p+=R"GPS(<div class='card'><h2>GPS / GNSS</h2><p class='muted'>RX GPIO47 / TX GPIO48 - UART 9600 baud</p>
  <p id='gpsstate'>GPS okunuyor...</p><table><tbody>
  <tr><td>Enlem</td><td id='gpslat'>---</td></tr><tr><td>Boylam</td><td id='gpslon'>---</td></tr>
  <tr><td>Hiz</td><td id='gpsspeed'>---</td></tr><tr><td>Yon</td><td id='gpscourse'>---</td></tr>
  <tr><td>Uydu</td><td id='gpssat'>---</td></tr></tbody></table>
  <small>GPS tracker ayarlari icin Tracker sekmesini acin. Sabit iGate beacon konumu degistirilmez.</small></div>
  <script>async function updateGps(){try{const r=await fetch('/api/gps',{cache:'no-store'});if(!r.ok)return;
  const x=await r.json();document.getElementById('gpsstate').textContent=x.fix?'GPS FIX OK':x.connected?'GPS bagli / FIX bekleniyor':'GPS verisi yok';
  document.getElementById('gpslat').textContent=x.fix?x.lat:'---';document.getElementById('gpslon').textContent=x.fix?x.lon:'---';
  document.getElementById('gpsspeed').textContent=x.speed_kmh+' km/sa';document.getElementById('gpscourse').textContent=x.course_deg+' derece';
  document.getElementById('gpssat').textContent=x.satellites;}catch(e){}}
  updateGps();setInterval(updateGps,2000);</script>)GPS";
  p+="</main></body></html>";server.send(200,"text/html; charset=utf-8",p);
}
void handleHome(){
  String p=pageHeader("V2.0.0 DEV2 — 433.775 MHz / SF12 / BW125 / RX");
  p+="<div class='card'><h2>Canli Durum</h2>";
  p+="<p>Istasyon: <b>"+htmlEscape(cfg.call)+"</b></p>";
  p+="<p>Wi-Fi: <b>"+String(WiFi.status()==WL_CONNECTED?"Bagli":"Bagli degil")+"</b></p>";
  p+="<p>Pil voltaji: <b>"+batteryLabel()+"</b> | Kalibrasyon: <b>"+String(cfg.batteryCalibration,2)+" V</b> | OLED: <b>"+String(cfg.oledType==0?"SSD1306":cfg.oledType==1?"SH1106":"KAPALI")+"</b></p>";
  p+="<p>Beacon'da Vbat: <b>"+String(cfg.beaconBattery?"ACIK":"KAPALI")+"</b> | OLED zamanlama: "+String(cfg.oledSleepSeconds?String(cfg.oledSleepSeconds)+" sn":"Kapali (surekli acik)")+"</p>";
  p+="<p>IP: <b>"+String(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():(apEnabled?WiFi.softAPIP().toString():"---"))+"</b></p>";
  p+="<p>APRS-IS: <b id='isState'>"+String(!cfg.aprsIsEnabled?"MANUEL KAPALI":(aprsVerified?"DOGRULANDI":"BAGLI DEGIL"))+"</b></p>";
  p+="<div class='actions' aria-label='Haberlesme kontrolleri'>";
  p+="<button id='isToggleBtn' type='button' onclick='toggleIs()'>APRS-IS Ac / Kapat</button>";
  p+="<button id='isBeaconBtn' class='secondary' type='button' onclick=\"postAction('/beacon/is')\" "+String(cfg.aprsIsEnabled && aprsVerified?"":"disabled title='APRS-IS bagli degil'")+">APRS-IS Beacon Gonder</button>";
  p+="<button class='secondary' type='button' onclick=\"postAction('/beacon/rf')\">RF Beacon Gonder (TX)</button>";
  p+="</div><small>Manuel RF gonderimde kanal ve TX limitleri korunur. APRS-IS kapaliysa internet beacon butonu pasiftir.</small>";
  p+=R"JS(<script>
async function refreshIsControl(){try{const r=await fetch('/api/is-state?'+Date.now(),{cache:'no-store'});if(!r.ok)return;const s=await r.json();document.getElementById('isBeaconBtn').disabled=!(s.enabled&&s.verified);document.getElementById('isState').textContent=!s.enabled?'MANUEL KAPALI':s.verified?'DOGRULANDI':'BAGLI DEGIL';}catch(e){}}
setInterval(refreshIsControl,5000);
async function toggleIs(){
 if(!confirm('APRS-IS durumunu degistirmek istiyor musunuz? Wi-Fi bagli kalacak.'))return;
 try{const r=await fetch('/api/toggle-is',{method:'POST'});const t=await r.text();toast(t,r.ok?'success':'error',6500);if(r.ok){document.getElementById('isState').textContent=t;document.getElementById('isBeaconBtn').disabled=true;}}catch(e){toast(e.message,'error',10000)}
}
</script>)JS";
  p+="<p>Calisma suresi: "+String(millis()/60000UL)+" dk | WiFi RSSI: "+String(WiFi.status()==WL_CONNECTED?String(WiFi.RSSI())+" dBm":"---")+" | Son restart: "+String((int)esp_reset_reason())+"</p>";
  p+="<p>Son duyulan: <b>"+htmlEscape(lastStation)+"</b></p>";
  p+="<p>Son RF alimi: <b>"+String(lastRxAt ? String((millis()-lastRxAt)/1000)+" sn once" : "Henuz yok")+"</b>";
  if(lastRxAt) p+=" | RSSI: "+String(lastRssi,1)+" dBm | SNR: "+String(lastSnr,1)+" dB";
  p+="</p>";
  p+="<p>LoRa paket: "+String(countRx)+" | APRS: "+String(countValid)+" | Aktarilan: "+String(countSent)+"</p>";
  p+="<p>DIGI tekrar: "+String(countDup)+" | APRS disi/gecersiz: "+String(countRejected)+" | Kuyruk kaybi: "+String(countDropped)+"</p>";
  p+="<p>Konum beacon: <b>"+String(cfg.beaconEnabled?"ACIK":"KAPALI")+"</b> | Gonderilen: "+String(countBeacon)+" | Aralik: "+String(cfg.beaconMinutes)+" dk | Sembol: <b>"+htmlEscape(cfg.symbolTable+cfg.symbolCode)+"</b></p>";
  p+="<p>Konum yorumu (gonderilen): "+htmlEscape(beaconComment())+"</p>";
  p+="<p>Status: <b>"+String(cfg.statusEnabled?"ACIK":"KAPALI")+"</b> | Gonderilen: "+String(countStatus)+" | Aralik: "+String(cfg.statusMinutes)+" dk</p>";
  p+="<p>Durum mesaji: "+htmlEscape(cfg.statusText)+"</p>";
  p+="<p>Auto Digipeater: <b>"+String(cfg.autoDigi?"ACIK":"KAPALI")+"</b> | Aktif mod: <b>"+String(digiMode?"DIGIPEATER":"iGATE/RX")+"</b></p>";
  p+="<p>TX gucu: <b>"+String(cfg.txPowerDbm)+" dBm</b> (RF Beacon ve Digipeater)</p>";
  p+="<p>WIDE1: "+String(cfg.wide1?"ACIK":"KAPALI")+" | WIDE2: "+String(cfg.wide2?"ACIK":"KAPALI")+" | RF Beacon Path: <b>"+htmlEscape(cfg.rfBeaconPath)+"</b> | Kontrol: "+String(cfg.probeMinutes)+" dk</p>";
  p+="<p>Digi TX: "+String(countDigiTx)+" | RF Beacon TX: "+String(countRfBeacon)+" | RF Beacon hata: "+String(countRfBeaconFail)+"</p>";
  p+="<p>RF Mesgul: "+String(countDigiBusy)+" | Saatlik limit: "+String(countDigiLimit)+" | Digi TX hata: "+String(countDigiFailed)+"</p>";
  p+="<p class='muted'>Otomatik Digi TX APRS-IS 60 sn kesikse veya manuel kapaliysa calisir. Manuel RF TX onay ve kanal kontrolune tabidir.</p>";
  p+="</div>";
  p+=R"JS(<script>
async function restartNow(){
 if(!confirm('TA1FM cihazi yeniden baslatilacak. Emin misiniz?'))return;
 try{const a=await fetch('/health',{cache:'no-store'});if(!a.ok)throw Error('Yetki veya baglanti hatasi');const old=(await a.json()).boot;
 const r=await fetch('/restart',{method:'POST',cache:'no-store'});const t=await r.text();if(!r.ok)throw Error(t);
 checkReboot(old);}catch(e){toast('Restart hatasi: '+e.message,'error',9000)}
}
</script>)JS";
  p+="<small>Durumu guncellemek icin sayfayi yenileyin.</small></main></body></html>";
  server.send(200,"text/html; charset=utf-8",p);
}
void handleSettings(){
  if(!accessAllowed())return;
  String p=pageHeader("iGate&Digi / Ayarlar");
  p+="<div class='card'><h3>Mobil iGate konumu</h3><form method='POST' action='/igate/gps/save'>";
  p+="<label><input type='checkbox' name='igateGps' value='1' style='width:auto' "+String(cfg.igateUseGps?"checked":"")+"> iGate beacon konumunu GPS ile guncelle</label>";
  p+="<small>GPS FIX yoksa iGate konum beacon'i gonderilmez; eski konum kullanilmaz.</small><button type='submit'>Konum ayarini kaydet</button></form></div>";
  p+="<div class='card'><form id='settingsForm' method='POST' action='/save' onsubmit='return saveSettings(event)'>";
  p+="<h2>OLED ekran ve pil voltaji</h2>";
  p+="<label>OLED ekran tipi</label><select name='oledtype'>";
  for(int i=0;i<3;i++){
    String v=i==0?"SSD1306":i==1?"SH1106":"OLED Kapali";
    p+="<option value='"+String(i)+"' "+String(cfg.oledType==i?"selected":"")+">"+v+"</option>";
  }
  p+="</select><small>OLED secimi kaydedildikten sonra yeniden baslatilarak uygulanir. Ekran yoksa iGate calismaya devam eder.</small>";
  p+="<label>OLED otomatik uyku - son gecerli RX paketinden sonra</label><select name='oledsleep'>";
  for(int secs : {0,15,30,60,300,600}){
    String caption=secs==0?"Zamanlama Kapali (ekran surekli acik)":secs==300?"5 dakika":secs==600?"10 dakika":String(secs)+" saniye";
    p+="<option value='"+String(secs)+"' "+String(cfg.oledSleepSeconds==secs?"selected":"")+">"+caption+"</option>";
  }
  p+="</select><small>Sure dolunca OLED uyur; yeni gecerli APRS RX paketinde uyanir. iGate/Digi devam eder. OLED Kapali ayri secenektir.</small>";
  p+="<p>Pil voltaji: <b>"+batteryLabel()+"</b> (30 saniyede bir olculur)</p>";
  p+="<label>Pil Voltaji Kalibrasyonu (+/- Volt)</label>";
  p+="<input name='batcal' type='number' step='0.01' min='-1.00' max='1.00' required value='"+String(cfg.batteryCalibration,2)+"'>";
  p+="<small>Multimetrede 4.13V, ekranda 3.93V ise +0.20 girin. Eksi deger voltaji dusurur. Kaydedilince OLED / Web / APRS Vbat ortak duzeltilir.</small>";
  p+="<label>iGate cagri isareti (SSID dahil)</label><input name='call' maxlength='9' required value='"+htmlEscape(cfg.call)+"'>";
  p+="<p><a href='/wifi'>Wi-Fi aglarini yonet (ana ag + ek aglar)</a></p>";
  p+="<label>APRS-IS sunucusu</label><input name='host' maxlength='80' required value='"+htmlEscape(cfg.host)+"'>";
  p+="<label>APRS-IS port</label><input name='port' type='number' min='1' max='65535' value='"+String(cfg.port)+"'>";
  p+="<label>APRS-IS passcode</label><input name='pass' type='text' maxlength='8' value='"+htmlEscape(cfg.passcode)+"'>";
  p+="<h2>APRS-IS Kontrol</h2>";
  p+="<label><input style='width:auto' type='checkbox' name='isenabled' value='1' "+String(cfg.aprsIsEnabled?"checked":"")+"> APRS-IS baglantisi acik (Wi-Fi bagimsiz)</label>";
  p+="<small>Kapali secilirse Wi-Fi/web kalir, Auto Digi etkinse hemen DIGI moduna gecer. Acilinca APRS-IS'e yeniden baglanir.</small>";
  p+="<h2>iGate Konumu (APRS.fi)</h2>";
  p+="<label>Enlem (ondalik derece, kuzey + / guney -)</label><input name='lat' type='number' step='any' min='-90' max='90' value='"+String(cfg.latitude,6)+"'>";
  p+="<label>Boylam (ondalik derece, dogu + / bati -)</label><input name='lon' type='number' step='any' min='-180' max='180' value='"+String(cfg.longitude,6)+"'>";
  p+="<h2>Harita sembolu</h2>";
  p+="<label>Hazir semboller</label><select id='preset' style='width:100%;padding:11px;background:#0d1722;color:#fff' onchange='pickSymbol()'>";
  p+="<option value=''>Ozel / mevcut kod</option><option value='L&'>LoRa iGate (L&amp;)</option>";
  p+="<option value='R&'>RX-only iGate (R&amp;)</option><option value='L#'>LoRa Digipeater (L#)</option>";
  p+="</select>";
  p+="<label>Symbol Table / Overlay (tek karakter)</label><input id='stab' name='stab' maxlength='1' required value='"+htmlEscape(cfg.symbolTable)+"'>";
  p+="<label>Symbol Code (tek karakter)</label><input id='scode' name='scode' maxlength='1' required value='"+htmlEscape(cfg.symbolCode)+"'>";
  p+="<small>LoRa iGate icin varsayilan L&amp;. Sembolun haritadaki rengi aprs.fi tarafindan belirlenir.</small>";
  p+="<script>function pickSymbol(){let v=document.getElementById('preset').value;if(v.length===2){document.getElementById('stab').value=v[0];document.getElementById('scode').value=v[1]}}</script>";
  p+="<label>Beacon araligi (dakika, 10-120)</label><input name='bmin' type='number' min='10' max='120' value='"+String(cfg.beaconMinutes)+"'>";
  p+="<label><input style='width:auto' type='checkbox' name='bena' value='1' "+String(cfg.beaconEnabled?"checked":"")+"> Konumumu APRS-IS'e gonder</label>";
  p+="<small>GPS gerekmez. iGate modunda APRS-IS konum beacon; Auto Digi ACIK ve Digi modu aktifken ayni beacon RF ile yayina verilir. Koordinatlar bos/0,0 ise etkinlestirilemez.</small>";
  p+="<small>APRS-IS aktarimi icin dogrulanmis amatör telsiz cagri isareti ve uygun passcode gereklidir.</small>";
  p+="<h2>APRS Comment &amp; Status</h2>";
  p+="<label>Comment Text (konum paketine eklenir, en fazla 43 karakter)</label>";
  p+="<input name='comment' maxlength='43' value='"+htmlEscape(cfg.commentText)+"'>";
  p+="<label><input style='width:auto' type='checkbox' name='batbeacon' value='1' "+String(cfg.beaconBattery?"checked":"")+"> Beacon comment alanina pil voltaji ekle (Vbat=3.92V)</label>";
  p+="<small>Acikken yorumun sonuna olculen voltaj eklenir. Toplam 43 karakteri asmamak icin yorum gerekirse kisaltilir. Olcum gecersizse Vbat eklenmez. RF ve APRS-IS konum Beacon'lari icin gecerlidir.</small>";
  p+="<label>Status Message (ayri APRS paketi, en fazla 62 karakter)</label>";
  p+="<input name='status' maxlength='62' value='"+htmlEscape(cfg.statusText)+"'>";
  p+="<label><input style='width:auto' type='checkbox' name='stena' value='1' "+String(cfg.statusEnabled?"checked":"")+"> Status mesajini APRS-IS'e gonder</label>";
  p+="<label>Status gonderim araligi (dakika, 15-240)</label>";
  p+="<input name='stmin' type='number' min='15' max='240' value='"+String(cfg.statusMinutes)+"'>";
  p+="<small>Metinlerde APRS uyumlulugu icin ASCII karakter kullanin (Turkce ozel karakter kullanmayin).</small>";

  p+="<h2>Auto Digipeater (yedek calisma)</h2>";
  p+="<label>LoRa TX gucu (10-21 dBm)</label><select name='txpwr' style='width:100%;padding:11px;background:#0d1722;color:white'>";
  for(int val : {10,14,17,20,21}) p+="<option value='"+String(val)+"' "+String(cfg.txPowerDbm==val?"selected":"")+">"+String(val)+" dBm</option>";
  p+="</select><small>Bu ayar RF Beacon ve Digipeater yayinlarina uygulanir. 21 dBm en yuksek secenektir; yasal EIRP sinirlarini, anten kazancini ve kablo kaybini kontrol edin.</small>";
  p+="<label><input style='width:auto' type='checkbox' name='autodigi' value='1' "+String(cfg.autoDigi?"checked":"")+"> Auto Digipeater ACIK (RF TX izni)</label>";
  p+="<label><input style='width:auto' type='checkbox' name='wide1' value='1' "+String(cfg.wide1?"checked":"")+"> WIDE1-1 paketlerini tekrarla</label>";
  p+="<label><input style='width:auto' type='checkbox' name='wide2' value='1' "+String(cfg.wide2?"checked":"")+"> WIDE2-1 / WIDE2-2 paketlerini tekrarla</label>";
  p+="<label>Kendi RF Beacon Path</label><select name='rfpath' style='width:100%;padding:11px;background:#0d1722;color:white'>";
  for(const char* choice : {"DIRECT","WIDE1-1","WIDE2-1","WIDE2-2"}){
    String v=choice;
    p+="<option value='"+v+"' "+String(cfg.rfBeaconPath==v?"selected":"")+">"+v+"</option>";
  }
  p+="</select><small>Yalniz kendi RF konum beacon'ina uygulanir; APRS-IS beacon ve Digipeater paketleri etkilenmez. WIDE2-2 en fazla iki tekrar istegidir, garanti degildir.</small>";
  p+="<label>Digipeater modundayken APRS-IS kontrol araligi</label><select name='probemin' style='width:100%;padding:11px;background:#0d1722;color:white'>";
  for(int val : {1,5,10,15}) p+="<option value='"+String(val)+"' "+String(cfg.probeMinutes==val?"selected":"")+">"+String(val)+" dakika</option>";
  p+="</select>";
  p+="<small>Internet/APRS-IS 60 sn kesilirse veya APRS-IS manuel kapatilirsa Digi modu. Digi RF beacon, konum beacon ayari aciksa etkin olur. Tekrarlama + RF beacon: en az 30 sn ara, toplam en fazla 20 TX/saat. TX gucu yukaridan ayarlanir. APRS-IS dogrulaninca iGate'e doner.</small>";
  p+="<small>Varsayilan KAPALI. RF yayin izniniz, anteniniz ve yerel kurallar uygun degilse acmayin. Web paneli testleri tek basina TX uyumlulugunu kanitlamaz.</small>";
  p+="<button type='submit'>Ayarlari Kaydet</button> ";
  p+="<p class='subtle'>Tum ayarlarin yedegi icin <a href='/backup'>Yedek</a> sekmesini kullanin.</p>";
  p+="</form></div>";
  p+=R"JS(<script>
async function saveSettings(event){event.preventDefault();const f=document.getElementById('settingsForm');if(!f.reportValidity())return false;let old=0;try{let h=await fetch('/health',{cache:'no-store'});old=(await h.json()).boot;const r=await fetch('/save',{method:'POST',body:new FormData(f)});const msg=await r.text();if(!r.ok){toast(msg,'error',10000);return false;}toast(msg,'success',0);checkReboot(old);}catch(e){toast('Kayit hatasi: '+e.message,'error',10000);}return false;}
</script>)JS";
  p+="</main></body></html>";
  server.send(200,"text/html; charset=utf-8",p);
}
void handleSave(){
  if(!accessAllowed())return;
  String call=server.arg("call"); call.trim();call.toUpperCase();
  String ssid=cfg.ssid,host=server.arg("host");host.trim();
  String portText=server.arg("port"),newPass=server.arg("pass"),newWifi="";
  int port=portText.toInt();
  String latText=server.arg("lat"),lonText=server.arg("lon");
  double lat=latText.toDouble(),lon=lonText.toDouble();
  int bmin=server.arg("bmin").toInt();
  bool bena=server.hasArg("bena");
  String stab=server.arg("stab"),scode=server.arg("scode");
  String comment=server.arg("comment"),status=server.arg("status");
  int stmin=server.arg("stmin").toInt();
  bool stena=server.hasArg("stena");
  int oledtype=server.arg("oledtype").toInt();
  int oledsleep=server.arg("oledsleep").toInt();
  bool batbeacon=server.hasArg("batbeacon");
  String calText=server.arg("batcal"); calText.trim();
  char* calEnd=nullptr;
  float batcal=strtof(calText.c_str(),&calEnd);
  if(!server.hasArg("batcal") || calText.isEmpty() || !calEnd || *calEnd!='\0' ||
     !isfinite(batcal) || batcal < -1.0f || batcal > 1.0f){
    server.send(400,"text/plain; charset=utf-8","Pil kalibrasyonu -1.00 ile +1.00 V arasinda olmali.");return;
  }
  if(!server.hasArg("oledtype") || oledtype<0 || oledtype>2){
    server.send(400,"text/plain; charset=utf-8","OLED tipi gecersiz.");return;
  }
  if(!server.hasArg("oledsleep") ||
     (oledsleep!=0 && oledsleep!=15 && oledsleep!=30 && oledsleep!=60 && oledsleep!=300 && oledsleep!=600)){
    server.send(400,"text/plain; charset=utf-8","OLED uyku suresi gecersiz.");return;
  }
  bool autodigi=server.hasArg("autodigi");
  bool isenabled=server.hasArg("isenabled");
  bool wide1=server.hasArg("wide1"),wide2=server.hasArg("wide2");
  String rfpath=server.arg("rfpath");
  if(rfpath!="DIRECT" && rfpath!="WIDE1-1" && rfpath!="WIDE2-1" && rfpath!="WIDE2-2"){
    server.send(400,"text/plain; charset=utf-8","RF Beacon path gecersiz.");return;
  }
  int txpwr=server.arg("txpwr").toInt();
  if(txpwr!=10 && txpwr!=14 && txpwr!=17 && txpwr!=20 && txpwr!=21){
    server.send(400,"text/plain; charset=utf-8","TX gucu secimi gecersiz.");return;
  }
  int probemin=server.arg("probemin").toInt();
  if(probemin!=1 && probemin!=5 && probemin!=10 && probemin!=15){
    server.send(400,"text/plain; charset=utf-8","Baglanti kontrol araligi gecersiz.");return;
  }
  if(!validAprsText(comment,43) || !validAprsText(status,62) ||
     stmin<15 || stmin>240 || (stena && status.isEmpty())) {
    server.send(400,"text/plain; charset=utf-8","Comment/Status gecersiz. ASCII karakter kullanin; | ve ~ kullanmayin.");return;
  }
  // '/' primary, '\\' alternate or A-Z/0-9 overlay.
  bool validTable=(stab.length()==1 && (stab=="/" || stab=="\\" || (stab[0]>='A' && stab[0]<='Z') || (stab[0]>='0' && stab[0]<='9')));
  bool validSymbol=(scode.length()==1 && scode[0]>=33 && scode[0]<=126 && scode[0]!='<' && scode[0]!='>');
  if(!validTable || !validSymbol){server.send(400,"text/plain; charset=utf-8","APRS sembol kodu gecersiz.");return;}

  if(bmin<10 || bmin>120 || !isfinite(lat) || !isfinite(lon) || lat < -90 || lat > 90 || lon < -180 || lon > 180 || (bena && ((lat==0 && lon==0) || latText.isEmpty() || lonText.isEmpty()))){
    server.send(400,"text/plain; charset=utf-8","Konum veya beacon araligi gecersiz.");return;
  }
  if(!validCall(call) || ssid.length()>32 || host.isEmpty() || host.length()>80 || host.indexOf(' ')>=0 || port<1 || port>65535 || newWifi.length()>63 || newPass.length()>8){
    server.send(400,"text/plain; charset=utf-8","Gecersiz ayar! Cagri isareti / sunucu / port bilgilerini kontrol edin.");return;
  }
  prefs.putString("call",call); prefs.putString("ssid",ssid);
  prefs.putString("host",host); prefs.putUShort("port",port);
  prefs.putDouble("lat",lat);prefs.putDouble("lon",lon);
  prefs.putUShort("bmin",(uint16_t)bmin);prefs.putBool("bena",bena);
  prefs.putString("symtab",stab);prefs.putString("symcode",scode);
  prefs.putString("comment",comment);prefs.putString("status",status);
  prefs.putUChar("oledtype",(uint8_t)oledtype);
  prefs.putUShort("oledsleep",(uint16_t)oledsleep);
  prefs.putBool("batbeacon",batbeacon);
  prefs.putFloat("batcal",batcal);
  prefs.putBool("stena",stena);prefs.putUShort("stmin",(uint16_t)stmin);
  prefs.putBool("isenabled",isenabled);
  prefs.putBool("autodigi",autodigi);prefs.putBool("wide1",wide1);
  prefs.putBool("wide2",wide2);prefs.putString("rfpath",rfpath);prefs.putUChar("probemin",(uint8_t)probemin);
  prefs.putUChar("txpwr",(uint8_t)txpwr);
  if(newWifi.length()) prefs.putString("wifi",newWifi);
  if(newPass.length()) prefs.putString("pass",newPass);
  server.send(200,"text/plain; charset=utf-8","Ayarlar kaydedildi. Cihaz yeniden baslatiliyor...");
  restartAt=millis()+1200;
}

// /security: PIN ve admin parola islemleri ana ayarlar sayfasindan ayridir.
void handleSecurity(){
  if(!accessAllowed())return;
  String p=pageHeader("Guvenlik - Yonetici ve Kurulum AP Sifreleri");
  p+="<div class='card'><h2>Yonetici Sifresini Degistir</h2>";
  p+="<p class='muted'>Kullanici adi: <b>admin</b> (degistirilemez)</p>";
  p+="<form method='POST' action='/security/admin' onsubmit='return securityPost(event,this)'>";
  p+="<label>Mevcut yonetici sifresi</label><input type='password' name='oldpass' autocomplete='current-password' required>";
  p+="<label>Yeni sifre (en az 10 karakter)</label><input type='password' name='newpass' minlength='10' maxlength='64' autocomplete='new-password' required>";
  p+="<label>Yeni sifre tekrar</label><input type='password' name='newpass2' minlength='10' maxlength='64' autocomplete='new-password' required>";
  p+="<button type='submit'>Yonetici Sifresini Degistir</button>";
  p+="<small>Sifre NVS icinde tuzlanmis SHA-256 dogrulayicisi olarak saklanir. JSON yedegine eklenmez. HTTP Basic sifreyi agda sifrelemez; paneli internete acmayin.</small></form></div>";
  p+="<div class='card'><h2>Kurulum AP Sifresi</h2>";
  p+="<form method='POST' action='/security/ap' onsubmit='return securityPost(event,this)'>";
  p+="<label>Yeni AP sifresi (8-63 karakter)</label><input type='password' name='apkey' minlength='8' maxlength='63' autocomplete='new-password' required>";
  p+="<label>Yeni AP sifresi tekrar</label><input type='password' name='apkey2' minlength='8' maxlength='63' autocomplete='new-password' required>";
  p+="<button type='submit'>AP Sifresini Degistir</button>";
  p+="<small>AP: TA1FM-APRS-SETUP. Kayittan sonra cihaz yeniden baslar. AP sifresi ayar yedegine eklenmez.</small></form></div>";
  p+=R"JS(<script>
async function securityPost(ev,form){
 ev.preventDefault();if(!form.reportValidity())return false;
 if(!confirm('Guvenlik ayari degistirilecek. Devam edilsin mi?'))return false;
 try{const r=await fetch(form.action,{method:'POST',body:new FormData(form),cache:'no-store'});
 const m=await r.text();toast(m,r.ok?'success':'error',9000);
 if(r.ok){form.reset();if(form.action.endsWith('/admin'))toast('Sifre degisti. Tarayicida yeni sifreyle giris yapin.', 'success',0);}
 }catch(e){toast('Islem basarisiz: '+e.message,'error',9000)}return false;
}
</script>)JS";
  p+="</main></body></html>";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"text/html; charset=utf-8",p);
}
void handleAdminPasswordChange(){
  if(!accessAllowed())return;
  String oldp=server.arg("oldpass"),p1=server.arg("newpass"),p2=server.arg("newpass2");
  if(!checkAdminPassword(oldp)){server.send(403,"text/plain; charset=utf-8","Mevcut sifre yanlis.");return;}
  if(p1!=p2 || p1.length()<10 || p1.length()>64 || p1==oldp){
    server.send(400,"text/plain; charset=utf-8","Yeni sifreler ayni olmali, 10-64 karakter ve oncekinden farkli olmali.");return;
  }
  storeAdminPassword(p1);
  addEvent("Yonetici sifresi degistirildi");
  server.send(200,"text/plain; charset=utf-8","Yonetici sifresi degistirildi. Yeni sifre ile giris yapin.");
}
void handleApPasswordChange(){
  if(!accessAllowed())return;
  String a=server.arg("apkey"),b=server.arg("apkey2");
  if(a!=b || a.length()<8 || a.length()>63){
    server.send(400,"text/plain; charset=utf-8","AP sifresi 8-63 karakter ve iki alanda ayni olmali.");return;
  }
  prefs.putString("apkey",a);
  addEvent("Kurulum AP sifresi degistirildi");
  server.send(200,"text/plain; charset=utf-8","AP sifresi kaydedildi. Cihaz yeniden baslatiliyor.");
  restartAt=millis()+1800;
}
void handleRestart(){
  if(!accessAllowed())return;
  addEvent("Web panelinden manuel restart istendi");
  server.send(200,"text/plain; charset=utf-8","Cihaz yeniden baslatiliyor.");
  restartAt=millis()+1500;
}

bool beaconCoordinatesValid(){
  return cfg.beaconEnabled && !(cfg.latitude==0 && cfg.longitude==0) &&
         validCall(cfg.call) && validAprsText(cfg.commentText,43);
}
void handleManualIs(){
  if(!accessAllowed())return;
  if(!beaconCoordinatesValid()){
    server.send(409,"text/plain; charset=utf-8","Konum beacon kapali veya koordinat gecersiz.");return;
  }
  if(!cfg.aprsIsEnabled || !aprsVerified || !aprs.connected() || WiFi.status()!=WL_CONNECTED){
    server.send(503,"text/plain; charset=utf-8","APRS-IS baglantisi dogrulanmamis.");return;
  }
  if(manualIsPending){server.send(409,"text/plain; charset=utf-8","APRS-IS beacon istegi zaten bekliyor.");return;}
  manualIsPending=true;
  manualIsResult="SIRADA";
  server.send(202,"text/plain; charset=utf-8","APRS-IS beacon istegi siraya alindi.");
}
void handleManualRf(){
  if(!accessAllowed())return;
  if(!cfg.autoDigi || !radioOk){
    server.send(403,"text/plain; charset=utf-8","RF TX icin Auto Digipeater ACIK ve radyo hazir olmali.");return;
  }
  if(!beaconCoordinatesValid()){
    server.send(409,"text/plain; charset=utf-8","Konum beacon kapali veya koordinat gecersiz.");return;
  }
  if(manualRfPending){server.send(409,"text/plain; charset=utf-8","RF beacon istegi zaten bekliyor.");return;}
  if(lastManualRfRequest && (uint32_t)(millis()-lastManualRfRequest)<TX_GAP_MS){
    server.send(429,"text/plain; charset=utf-8","Yeni manuel RF istegi icin en az 30 sn bekleyin.");return;
  }
  lastManualRfRequest=millis();
  manualRfPending=true;
  manualRfResult="SIRADA";
  server.send(202,"text/plain; charset=utf-8","RF beacon istegi siraya alindi. TX sonucu durum ekraninda kontrol edilebilir.");
}

uint32_t fnv1a(const String& s){
  uint32_t h=2166136261u;
  for(size_t i=0;i<s.length();i++){h^=(uint8_t)s[i];h*=16777619u;}
  return h;
}
// Digipeater-only duplicate check. RX iGate MUST forward every eligible RF reception.
bool seenRecently(const String& packet){
  uint32_t h=fnv1a(packet), now=millis();
  for(auto &item:seen) if(item.hash==h && now-item.time<30000u) return true;
  seen[seenPtr]={h,now};seenPtr=(seenPtr+1)%24;
  return false;
}
// Length-tagged binary queue: do not truncate at 0x00 or convert entire APRS info to UTF-8.
bool enqueueBinary(const uint8_t* bytes,size_t length){
  if(queueCount>=12 || length==0 || length>sizeof(queueData[0].line)){countDropped++;return false;}
  Pending &p=queueData[queueTail]; memcpy(p.line,bytes,length);p.len=(uint16_t)length;
  queueTail=(queueTail+1)%12;queueCount++;return true;
}
// Full packet data starts after the three LoRa APRS prefix octets.
// Never normalize trailing spaces or decode arbitrary APRS information bytes.
bool enqueueGateFrame(const uint8_t* data,size_t frameEnd,size_t colon,const String& gateCall){
  String prefix;
  prefix.reserve(colon+gateCall.length()+8);
  for(size_t i=3;i<3+colon;i++)prefix+=(char)data[i];
  prefix+=",qAO,"+gateCall+":";
  const size_t infoStart=3+colon+1;
  const size_t infoSize=frameEnd-infoStart;
  size_t total=prefix.length()+infoSize;
  if(total>sizeof(queueData[0].line)){countDropped++;return false;}
  uint8_t out[300];memcpy(out,prefix.c_str(),prefix.length());
  memcpy(out+prefix.length(),data+infoStart,infoSize);
  return enqueueBinary(out,total);
}

// LoRa APRS format: 3C FF 01 + TNC2 SRC>DEST[,PATH]:INFO.
// Parse only the ASCII address header; preserve the original info bytes verbatim.
// A C-string conversion of the info field would corrupt NUL and binary Mic-E packets.
bool decodeAprs(const uint8_t* data,size_t len,String& source,String& packet,
                size_t& frameEnd,size_t& colonOffset){
  if(len<10 || data[0]!=0x3c || data[1]!=0xff || data[2]!=0x01)return false;
  frameEnd=len;
  for(size_t i=3;i<len;i++)if(data[i]=='\r'||data[i]=='\n'){frameEnd=i;break;}
  if(frameEnd<=3)return false;
  size_t gt=0, colon=0;
  for(size_t i=3;i<frameEnd;i++){
    if(data[i]==':' && colon==0){colon=i;break;}
    if(data[i]=='>' && gt==0)gt=i;
    if(data[i]<33 || data[i]>126)return false; // Address header only.
  }
  if(gt<6 || colon<=gt+1 || colon>=frameEnd-1 || colon-gt>105)return false;
  source="";for(size_t i=3;i<gt;i++)source+=(char)data[i];
  source.toUpperCase();if(!validCall(source))return false;
  String header="";for(size_t i=gt+1;i<colon;i++)header+=(char)data[i];
  String uc=header;uc.toUpperCase();
  // Match complete path components, never substrings in unrelated callsigns.
  String path=","+uc+",";
  if(path.indexOf(",TCPIP,")>=0 || path.indexOf(",TCPIP*,")>=0 ||
     path.indexOf(",TCPXX,")>=0 || path.indexOf(",TCPXX*,")>=0 ||
     path.indexOf(",NOGATE,")>=0 || path.indexOf(",NOGATE*,")>=0 ||
     path.indexOf(",RFONLY,")>=0 || path.indexOf(",RFONLY*,")>=0)return false;
  // Already gated/q-constructed packets must not be accepted from RF.
  int at=uc.indexOf(',');
  while(at>=0){
    int next=uc.indexOf(',',at+1);
    String part=uc.substring(at+1,next<0?uc.length():next);
    if(part.length()>=3 && part[0]=='Q' && part[1]=='A')return false;
    at=next;
  }
  // Do not gate generic APRS queries or Internet-originated third-party frames.
  if(data[colon+1]=='?')return false;
  if(data[colon+1]=='}'){
    // Third-party gateway selection/unwrap requires its own complete parser.
    // Fail closed rather than create an APRS-IS -> RF -> APRS-IS loop.
    return false;
  }
  colonOffset=colon-3;
  // Only for logs/GUI and DIGI eligibility: replace binary/control bytes in display.
  packet="";packet.reserve(frameEnd-3);
  for(size_t i=3;i<frameEnd;i++){
    uint8_t c=data[i];
    packet+=(char)((c>=32 && c<=126)?c:'.');
  }
  return true;
}

void checkDigiEligibility(const String& packet);
void manageDigiTx();
void updateHybridMode();

void examineIncomingAprs(const String& packet,const String& channel);
void readRadio(){
  if(!rxFlag || !radioOk) return;
  rxFlag=false;
  size_t n=radio.getPacketLength();
  uint8_t data[MAX_FRAME+1]={0};
  int state=(n>0 && n<=MAX_FRAME)? radio.readData(data,n) : -1;
  if(state==RADIOLIB_ERR_NONE){
    countRx++;lastRxAt=millis();
    float rssi=radio.getRSSI(),snr=radio.getSNR();lastRssi=rssi;lastSnr=snr;
    String source,packet;
    Serial.printf("[RX] %u byte RSSI %.1f SNR %.1f\n",(unsigned)n,rssi,snr);
    size_t frameEnd=0, colonOffset=0;
    if(decodeAprs(data,n,source,packet,frameEnd,colonOffset)){
      countValid++;lastStation=source;
      onValidRxDisplay();
      recordRx(source,packet,rssi,snr,"RX");
      Serial.println("[APRS] "+packet);
      examineIncomingAprs(packet,"RF");
      if(digiMode && cfg.autoDigi){
        // DIGI still constructs TX using a String. Until binary-safe RF DIGI is
        // implemented, refuse binary info rather than transmitting a modified copy.
        bool printableInfo=true;
        for(size_t i=3+colonOffset+1;i<frameEnd;i++){
          if(data[i]<32 || data[i]>126){printableInfo=false;break;}
        }
        if(!printableInfo){
          countRejected++;updateRecentAction(packet,"DIGI BINARY ATLANDI");
        }else if(seenRecently(packet)){
          countDup++;updateRecentAction(packet,"TEKRAR");Serial.println("[DIGI] Duplicate (30 sn)");
        }else checkDigiEligibility(packet);
      } else if(WiFi.status()==WL_CONNECTED && aprsVerified && aprs.connected()){
        // RX iGate: do not suppress duplicates; APRS-IS can use all RF reception reports.
        if(enqueueGateFrame(data,frameEnd,colonOffset,cfg.call))
          updateRecentAction(packet,"IS SIRADA");
      } else countDropped++;
    } else {
      countRejected++;
      Serial.println("[APRS] Gecersiz veya desteklenmeyen paket; HEX:");
      for(size_t i=0;i<n;i++){Serial.printf("%02X ",data[i]);if(i%24==23)Serial.println();}
      Serial.println();
    }
  } else Serial.printf("[RX] Paket hatasi: %d\n",state);
  int rc=radio.startReceive();
  if(rc!=RADIOLIB_ERR_NONE) Serial.printf("[RX] Tekrar baslatma hatasi: %d\n",rc);
}


// APRS path manipulation: only first still-unused WIDE alias, no third-party/Internet paths.
// WIDE1-1 -> OURCALL*, WIDE2-2 -> OURCALL*,WIDE2-1, WIDE2-1 -> OURCALL*.
// Never relay own source or a packet that has already traversed our station.
void checkDigiEligibility(const String& packet){
  if(!cfg.autoDigi || !digiMode || !digiPending.isEmpty()) return;
  int gt=packet.indexOf('>'), colon=packet.indexOf(':');
  if(gt<3 || colon<gt+2 || colon+1>=packet.length())return;
  String source=packet.substring(0,gt);source.toUpperCase();
  if(source==cfg.call || packet[colon+1]=='}')return;
  String addr=packet.substring(gt+1,colon);
  String u=addr;u.toUpperCase();
  if(u.indexOf("TCPIP")>=0 || u.indexOf("TCPXX")>=0 || u.indexOf(",Q")>=0)return;
  int firstComma=addr.indexOf(',');
  if(firstComma<0)return;
  String dest=addr.substring(0,firstComma);
  if(dest.isEmpty())return;
  String prefix=source+">"+dest;
  String suffix="";
  bool matched=false;
  bool foundUnused=false;
  int i=firstComma+1;
  while(i<=addr.length()){
    int j=addr.indexOf(',',i);
    if(j<0)j=addr.length();
    String token=addr.substring(i,j);
    String upper=token;upper.toUpperCase();
    if(token.isEmpty() || token.length()>13)return;
    if(upper==cfg.call || upper==cfg.call+"*")return;
    if(!matched && upper.endsWith("*")){
      // Already repeated by another station: preserve used hops.
      suffix+=","+token;
    } else if(!matched && !foundUnused){
      foundUnused=true;
      if(cfg.wide1 && upper=="WIDE1-1"){
        suffix+=","+cfg.call+"*";
        matched=true;
      } else if(cfg.wide2 && (upper=="WIDE2-1" || upper=="WIDE2-2")){
        suffix+=","+cfg.call+"*";
        if(upper=="WIDE2-2")suffix+=",WIDE2-1";
        matched=true;
      } else {
        // Only the first UNUSED hop may be digipeated; no skipping unsupported paths.
        return;
      }
    } else suffix+=","+token;
    if(j==addr.length())break;
    i=j+1;
  }
  if(!matched)return;
  String output=prefix+suffix+packet.substring(colon);
  // 3-byte LoRa APRS header + max 252 bytes TNC2 text.
  if(output.length()>252)return;
  updateRecentAction(packet,"DIGI SIRADA");
  digiPending=output;
  digiPendingAt=millis();
  Serial.println("[DIGI] Aday: "+digiPending);
}

// TX DIAGNOSTIC: apply the same power/OCP order used by Ricardo's working SX1262 setup.
// This does not measure radiated RF power; compare with the wattmeter.
bool prepareTxPower(const char* operation){
  int16_t p=radio.setOutputPower((int8_t)cfg.txPowerDbm);
  int16_t c=radio.setCurrentLimit(140.0f);
  Serial.printf("[TX KONTROL] %s: istenen=%u dBm, POWER=%d, OCP=%d, limit=%.1f mA\n",
                operation,(unsigned)cfg.txPowerDbm,p,c,radio.getCurrentLimit());
  return p==RADIOLIB_ERR_NONE && c==RADIOLIB_ERR_NONE;
}

void manageDigiTx(){
  if(!radioOk || !cfg.autoDigi || !digiMode || digiPending.isEmpty())return;
  uint32_t now=millis();
  if((uint32_t)(now-digiPendingAt)>15000u){digiPending="";return;}
  if(txWindowStart==0 || (uint32_t)(now-txWindowStart)>=3600000u){txWindowStart=now;txWindowCount=0;}
  if(txWindowCount>=TX_MAX_PER_HOUR){countDigiLimit++;digiPending="";return;}
  if(lastDigiTx && (uint32_t)(now-lastDigiTx)<TX_GAP_MS)return;
  // Wait a short, bounded window after hearing packet; re-check RF before TX.
  if((uint32_t)(now-digiPendingAt)<1200u)return;
  if(!prepareTxPower("DIGI")){
    countDigiFailed++;digiPending="";
    radio.startReceive();
    return;
  }
  noInterrupts(); rxFlag=false; interrupts();
  int16_t ch=radio.scanChannel(); // LoRa CAD: check for a detected preamble.
  if(ch!=RADIOLIB_CHANNEL_FREE){
    countDigiBusy++;
    if(ch!=RADIOLIB_LORA_DETECTED)Serial.printf("[DIGI] CAD hata: %d\n",ch);
    // The channel may be occupied. Drop this packet rather than congest RF.
    digiPending="";
    radio.startReceive();
    return;
  }
  uint8_t data[255];
  data[0]=0x3C;data[1]=0xFF;data[2]=0x01;
  memcpy(data+3,digiPending.c_str(),digiPending.length());
  size_t n=digiPending.length()+3;
  // Blocking RF TX: RX is unavailable for the packet airtime.
  int16_t result=radio.transmit(data,n);
  lastDigiTx=millis();txWindowCount++;
  if(result==RADIOLIB_ERR_NONE){countDigiTx++;Serial.println("[DIGI] RF TX OK: "+digiPending);addEvent("DIGI TX OK: "+digiPending.substring(0,70));
    int gt=digiPending.indexOf('>'), colon=digiPending.indexOf(':');
    String src=digiPending.substring(0,gt), info=colon>=0?digiPending.substring(colon):"";
    for(int i=0;i<rxCount;i++){int idx=(rxHead+15-1-i)%15;if(rxRecords[idx].station==src && rxRecords[idx].packet.endsWith(info)){rxRecords[idx].action="DIGI OK";break;}}
  }
  else {countDigiFailed++;addEvent("DIGI TX HATA: "+String(result));Serial.printf("[DIGI] TX HATA: %d\n",result);}
  digiPending="";
  noInterrupts();rxFlag=false;interrupts();
  int16_t r=radio.startReceive();
  if(r!=RADIOLIB_ERR_NONE)Serial.printf("[DIGI] RX yeniden baslatma hata: %d\n",r);
}

void updateHybridMode(){
  uint32_t now=millis();
  bool healthy=(cfg.aprsIsEnabled && WiFi.status()==WL_CONNECTED && aprs.connected() && aprsVerified);
  if(healthy){
    aprsDownFrom=0;
    if(digiMode){
      digiMode=false;digiPending="";rfBeaconDue=false;
      Serial.println("[HIBRIT] APRS-IS dogrulandi -> iGate moduna donuldu");
    }
    return;
  }
  if(aprsDownFrom==0)aprsDownFrom=now?now:1;
  if(!cfg.autoDigi){digiMode=false;digiPending="";rfBeaconDue=false;return;}
  if(!digiMode && (!cfg.aprsIsEnabled || (uint32_t)(now-aprsDownFrom)>=DIGI_AFTER_MS)){
    digiMode=true;digiPending="";
    rfBeaconDue=true;lastRfBeaconAttempt=0;
    lastDigiProbe=now;
    // Drop stale queued APRS-IS frames at transition.
    queueCount=queueHead=queueTail=0;
    addEvent(cfg.aprsIsEnabled?"APRS-IS 60 sn yok -> DIGI aktif":"APRS-IS manuel kapali -> DIGI aktif");
  }
}

void disconnectAprs(){
  if(manualIsPending)manualIsResult="HATA";manualIsPending=false;
  aprs.stop();aprsVerified=false;loginSent=false;aprsLine="";aprsConnectStarted=0;
  // Never replay stale RF receptions after network disconnection/reconnection.
  queueCount=queueHead=queueTail=0;
}

// Convert signed decimal degree to the uncompressed APRS ddmm.mmN / dddmm.mmE notation.
String coordAprs(double value,bool latitude){
  char hemi=(value>=0 ? (latitude?'N':'E') : (latitude?'S':'W'));
  double a=fabs(value);
  int deg=(int)a;
  long minuteHundredths=lround((a-deg)*6000.0);
  if(minuteHundredths>=6000){deg++;minuteHundredths=0;}
  char buf[16];
  if(latitude) snprintf(buf,sizeof(buf),"%02d%02ld.%02ld%c",deg,minuteHundredths/100,minuteHundredths%100,hemi);
  else snprintf(buf,sizeof(buf),"%03d%02ld.%02ld%c",deg,minuteHundredths/100,minuteHundredths%100,hemi);
  return String(buf);
}

// DEV4: Do not publish stale iGate coordinates when mobile GPS mode is selected.
bool currentIgateCoordinates(double &lat,double &lon){
  if(cfg.igateUseGps){
    if(!gpsFixFresh())return false;
    lat=gps.location.lat();lon=gps.location.lng();
    return true;
  }
  lat=cfg.latitude;lon=cfg.longitude;
  return !(lat==0 && lon==0);
}
void handleIgateGpsSave(){
  if(!accessAllowed())return;
  cfg.igateUseGps=server.hasArg("igateGps");
  prefs.putBool("igategps",cfg.igateUseGps);
  server.sendHeader("Location","/settings");server.send(303,"text/plain","OK");
}

// Locally-originated beacon: TCPIP* only, never RF digipeater path/q construct.
void sendOwnBeacon(){
  if(!cfg.aprsIsEnabled || !cfg.beaconEnabled || !aprsVerified || !aprs.connected())return;
  double lat=0,lon=0;
  if(!currentIgateCoordinates(lat,lon) || !validCall(cfg.call))return;
  uint32_t now=millis();
  if(!manualIsPending && !beaconDueOnConnect && (uint32_t)(now-lastBeacon)<(uint32_t)cfg.beaconMinutes*60000UL)return;
  String line=cfg.call+">"+OWN_TOCALL+",TCPIP*:!"+coordAprs(lat,true)+cfg.symbolTable+coordAprs(lon,false)+cfg.symbolCode+beaconComment();
  size_t n=aprs.print(line+"\r\n");
  if(n==line.length()+2){
    countBeacon++;lastBeacon=now;beaconDueOnConnect=false;manualIsPending=false;manualIsResult="OK";
    Serial.println("[BEACON] APRS-IS TCP gonderildi: "+line);
  }else {manualIsResult="HATA";disconnectAprs();}
}

// RF position beacon only in active backup Digi mode. Never send TCPIP*/qAO on RF.
// The frame uses the same LoRa-APRS 3-byte header as received/digipeated frames.
void manageRfBeacon(){
  if(!radioOk || !cfg.autoDigi || (!digiMode && !manualRfPending && !bootRfPending) || !cfg.beaconEnabled) return;
  double lat=0,lon=0;
  if(!currentIgateCoordinates(lat,lon) || !validCall(cfg.call)) return;
  if(!validAprsText(cfg.commentText,43)) return;
  if(bootRfPending && rapidBootCount>=3)bootRfPending=false;
  // Prioritize a pending tracker relay over our own beacon.
  if(!digiPending.isEmpty()) return;
  uint32_t now=millis();
  uint32_t interval=(uint32_t)cfg.beaconMinutes*60000UL;
  if(bootRfPending && now<BOOT_RF_WAIT_MS) return;
  if(!manualRfPending && !rfBeaconDue && !bootRfPending && (uint32_t)(now-lastRfBeacon)<interval) return;
  // Retry slowly after busy channel or failed TX; avoid tight repeated CAD attempts.
  if(lastRfBeaconAttempt && (uint32_t)(now-lastRfBeaconAttempt)<30000UL) return;
  if(lastDigiTx && (uint32_t)(now-lastDigiTx)<TX_GAP_MS) return;
  if(txWindowStart==0 || (uint32_t)(now-txWindowStart)>=3600000UL){
    txWindowStart=now;txWindowCount=0;
  }
  if(txWindowCount>=TX_MAX_PER_HOUR){
    if(!lastRfBeaconAttempt || (uint32_t)(now-lastRfBeaconAttempt)>=30000UL){
      countDigiLimit++;lastRfBeaconAttempt=now;
    }
    return;
  }
  String line=cfg.call+">"+OWN_TOCALL+(cfg.rfBeaconPath=="DIRECT" ? "" : ","+cfg.rfBeaconPath)+":!"+coordAprs(lat,true)+cfg.symbolTable+
              coordAprs(lon,false)+cfg.symbolCode+beaconComment();
  if(line.length()>252){countRfBeaconFail++;lastRfBeaconAttempt=now;manualRfPending=false;return;}
  lastRfBeaconAttempt=now;
  if(!prepareTxPower("RF BEACON")){
    countRfBeaconFail++;manualRfPending=false;manualRfResult="HATA";
    radio.startReceive();
    return;
  }
  noInterrupts();rxFlag=false;interrupts();
  int16_t cad=radio.scanChannel();
  if(cad!=RADIOLIB_CHANNEL_FREE){
    countDigiBusy++;
    Serial.printf("[RF BEACON] Kanal mesgul / CAD: %d\n",cad);
    radio.startReceive();
    return;
  }
  uint8_t frame[255];
  frame[0]=0x3C;frame[1]=0xFF;frame[2]=0x01;
  memcpy(frame+3,line.c_str(),line.length());
  int16_t rc=radio.transmit(frame,line.length()+3);
  lastDigiTx=millis();
  txWindowCount++;
  if(rc==RADIOLIB_ERR_NONE){
    countRfBeacon++;lastRfBeacon=lastDigiTx;rfBeaconDue=false;manualRfPending=false;bootRfPending=false;manualRfResult="OK";
    Serial.println("[RF BEACON] TX OK: "+line);
  }else{
    countRfBeaconFail++;manualRfPending=false;manualRfResult="HATA";
    Serial.printf("[RF BEACON] TX HATA: %d\n",rc);
  }
  noInterrupts();rxFlag=false;interrupts();
  int16_t rx=radio.startReceive();
  if(rx!=RADIOLIB_ERR_NONE) Serial.printf("[RF BEACON] RX yeniden baslatma hata: %d\n",rx);
}

// DEV2 tracker TX: separate schedule/identity from fixed iGate beacons.
// GPS must be fresh; no stale coordinate replay.
uint32_t trackerPeriodMs(){
  if(!cfg.trackerSmart || !gps.speed.isValid() || gps.speed.age()>10000UL)
    return (uint32_t)cfg.trackerMinutes*60000UL;
  float v=gps.speed.kmph();
  uint32_t sec=cfg.trackerSlowSeconds;
  if(v>=cfg.trackerFastKmh)sec=cfg.trackerFastSeconds;
  else if(v>cfg.trackerSlowKmh){
    float factor=(v-cfg.trackerSlowKmh)/(float)(cfg.trackerFastKmh-cfg.trackerSlowKmh);
    sec=(uint32_t)(cfg.trackerSlowSeconds+factor*(int32_t)(cfg.trackerFastSeconds-cfg.trackerSlowSeconds));
  }
  return sec*1000UL;
}
bool trackerTurnDue(bool rf){
  if(!cfg.trackerSmart || !cfg.trackerTurnEnabled || !gps.course.isValid() || gps.course.age()>10000UL ||
     !gps.speed.isValid() || gps.speed.age()>10000UL || gps.speed.kmph()<5.0f)return false;
  float last=rf?lastTrackerRfCourse:lastTrackerIsCourse;
  uint32_t lastAt=rf?trackerLastRf:trackerLastIs;
  if(!isfinite(last)||lastAt==0||(uint32_t)(millis()-lastAt)<(uint32_t)cfg.trackerTurnMinSeconds*1000UL)return false;
  float diff=fabsf(gps.course.deg()-last);if(diff>180.0f)diff=360.0f-diff;
  return diff>=cfg.trackerTurnDegrees;
}
bool trackerReady(){
  return cfg.trackerMode && gpsFixFresh() && validCall(cfg.trackerCall) && cfg.trackerCall!=cfg.call;
}
String trackerPosition(){
  if(!trackerReady())return "";
  return cfg.trackerCall+">"+OWN_TOCALL+":!"+coordAprs(gps.location.lat(),true)+
    cfg.trackerTable+coordAprs(gps.location.lng(),false)+cfg.trackerSymbol+cfg.trackerComment;
}
void manageTrackerIs(){
  if(!(cfg.trackerMode&2) || !trackerReady() || !cfg.aprsIsEnabled || !aprsVerified || !aprs.connected())return;
  uint32_t now=millis();
  if((uint32_t)(now-trackerLastIs)<trackerPeriodMs() && !trackerTurnDue(false))return;
  String pos=trackerPosition();
  int colon=pos.indexOf(':');if(colon<0)return;
  String line=pos.substring(0,colon)+",TCPIP*"+pos.substring(colon);
  if(aprs.print(line+"\r\n")==line.length()+2){trackerLastIs=now;lastTrackerIsCourse=gps.course.isValid()?gps.course.deg():NAN;trackerIsSent++;Serial.println("[TRACKER IS] "+line);}
  else disconnectAprs();
}
void manageTrackerRf(){
  if(!(cfg.trackerMode&1) || !radioOk || !trackerReady())return;
  uint32_t now=millis();
  if((uint32_t)(now-trackerLastRf)<trackerPeriodMs() && !trackerTurnDue(true))return;
  if(trackerRfAttempts && (uint32_t)(now-trackerRfAttempts)<30000UL)return;
  if(!digiPending.isEmpty() || (lastDigiTx && (uint32_t)(now-lastDigiTx)<TX_GAP_MS))return;
  if(txWindowStart==0 || (uint32_t)(now-txWindowStart)>=3600000UL){txWindowStart=now;txWindowCount=0;}
  if(txWindowCount>=TX_MAX_PER_HOUR)return;
  String line=trackerPosition();
  int colon=line.indexOf(':');if(colon<0)return;
  line=line.substring(0,colon)+(cfg.rfBeaconPath=="DIRECT"?"":","+cfg.rfBeaconPath)+line.substring(colon);
  if(line.length()>252){trackerRfFailures++;trackerRfAttempts=now;return;}
  trackerRfAttempts=now;
  if(!prepareTxPower("TRACKER")){trackerRfFailures++;radio.startReceive();return;}
  noInterrupts();rxFlag=false;interrupts();
  int16_t cad=radio.scanChannel();
  if(cad!=RADIOLIB_CHANNEL_FREE){radio.startReceive();return;}
  uint8_t frame[255]={0x3C,0xFF,0x01};
  memcpy(frame+3,line.c_str(),line.length());
  int16_t rc=radio.transmit(frame,line.length()+3);
  lastDigiTx=millis();txWindowCount++;
  if(rc==RADIOLIB_ERR_NONE){trackerLastRf=lastDigiTx;lastTrackerRfCourse=gps.course.isValid()?gps.course.deg():NAN;trackerRfSent++;Serial.println("[TRACKER RF] "+line);}
  else{trackerRfFailures++;Serial.printf("[TRACKER RF] TX HATA: %d\n",rc);}
  noInterrupts();rxFlag=false;interrupts();
  radio.startReceive();
}

// DEV4 independent tracker status. No GPS FIX needed for status packets.
String trackerStatusLine(){
  if(!cfg.trackerStatusEnabled || cfg.trackerStatusMode==0 ||
     !validCall(cfg.trackerCall) || cfg.trackerCall==cfg.call ||
     !validAprsText(cfg.trackerStatusText,62) || cfg.trackerStatusText.isEmpty())return "";
  return cfg.trackerCall+">"+OWN_TOCALL+":>"+cfg.trackerStatusText;
}
void manageTrackerStatusIs(){
  if(!(cfg.trackerStatusMode&2) || !cfg.aprsIsEnabled || !aprsVerified || !aprs.connected())return;
  uint32_t now=millis();
  if((uint32_t)(now-trackerStatusLastIs)<(uint32_t)cfg.trackerStatusMinutes*60000UL)return;
  String line=trackerStatusLine();if(line.isEmpty())return;
  int colon=line.indexOf(':');line=line.substring(0,colon)+",TCPIP*"+line.substring(colon);
  if(aprs.print(line+"\r\n")==line.length()+2){trackerStatusLastIs=now;trackerStatusIsSent++;}
  else disconnectAprs();
}
void manageTrackerStatusRf(){
  if(!(cfg.trackerStatusMode&1) || !radioOk || !cfg.trackerStatusEnabled)return;
  uint32_t now=millis();
  if((uint32_t)(now-trackerStatusLastRf)<(uint32_t)cfg.trackerStatusMinutes*60000UL)return;
  if(trackerStatusRfAttempt && (uint32_t)(now-trackerStatusRfAttempt)<30000UL)return;
  if(!digiPending.isEmpty() || (lastDigiTx && (uint32_t)(now-lastDigiTx)<TX_GAP_MS))return;
  if(txWindowStart==0 || (uint32_t)(now-txWindowStart)>=3600000UL){txWindowStart=now;txWindowCount=0;}
  if(txWindowCount>=TX_MAX_PER_HOUR)return;
  String line=trackerStatusLine();if(line.isEmpty())return;
  int colon=line.indexOf(':');
  line=line.substring(0,colon)+(cfg.rfBeaconPath=="DIRECT"?"":","+cfg.rfBeaconPath)+line.substring(colon);
  if(line.length()>252)return;
  trackerStatusRfAttempt=now;
  if(!prepareTxPower("TRACKER STATUS")){radio.startReceive();return;}
  noInterrupts();rxFlag=false;interrupts();
  if(radio.scanChannel()!=RADIOLIB_CHANNEL_FREE){radio.startReceive();return;}
  uint8_t frame[255]={0x3C,0xFF,0x01};
  memcpy(frame+3,line.c_str(),line.length());
  int16_t rc=radio.transmit(frame,line.length()+3);
  lastDigiTx=millis();txWindowCount++;
  if(rc==RADIOLIB_ERR_NONE){trackerStatusLastRf=lastDigiTx;trackerStatusRfSent++;Serial.println("[TRACKER STATUS RF] "+line);}
  noInterrupts();rxFlag=false;interrupts();radio.startReceive();
}

// A status report is a separate APRS-IS packet, not part of the position beacon.
void sendOwnStatus(){
  if(!cfg.statusEnabled || cfg.statusText.isEmpty() || !aprsVerified || !aprs.connected()) return;
  if(!validAprsText(cfg.statusText,62) || !validCall(cfg.call)) return;
  uint32_t now=millis();
  if(!statusDueOnConnect && (uint32_t)(now-lastStatus)<(uint32_t)cfg.statusMinutes*60000UL) return;
  String line=cfg.call+">"+OWN_TOCALL+",TCPIP*:>"+cfg.statusText;
  size_t n=aprs.print(line+"\r\n");
  if(n==line.length()+2){
    countStatus++;lastStatus=now;statusDueOnConnect=false;
    Serial.println("[STATUS] APRS-IS TCP gonderildi: "+line);
  } else disconnectAprs();
}

void parseAprsResponse(const String& line){
  Serial.println("[APRS-IS] "+line);
  String low=line;low.toLowerCase();
  if(low.startsWith("# logresp ")){
    if(low.indexOf("unverified")>=0){Serial.println("[APRS-IS] Passcode kabul edilmedi");disconnectAprs();return;}
    if(low.indexOf("verified")>=0){aprsVerified=true;beaconDueOnConnect=true;statusDueOnConnect=true;Serial.println("[APRS-IS] GIRIS DOGRULANDI");}
  }
}
void manageTelemetryIs();
void manageMessagingIs();
void refreshBridgeFilter();
void maintainAprs(){
  if(!cfg.aprsIsEnabled || WiFi.status()!=WL_CONNECTED || cfg.passcode.isEmpty() || !validCall(cfg.call)){
    if(aprs.connected()) disconnectAprs();
    return;
  }
  if(!aprs.connected()){
    aprsVerified=false;loginSent=false;
    // APRS-IS link down: discard old queued data, never send delayed positions later.
    queueCount=queueHead=queueTail=0;
    if(digiMode){
      if((uint32_t)(millis()-lastDigiProbe)<(uint32_t)cfg.probeMinutes*60000UL)return;
      lastDigiProbe=millis();
    } else if((uint32_t)(millis()-lastAprsAttempt)<15000u) return;
    lastAprsAttempt=millis();
    Serial.printf("[APRS-IS] %s:%d baglaniyor...\n",cfg.host.c_str(),cfg.port);
    // TCP connection can block briefly; 1200 ms budget to limit RX interference.
    if(!aprs.connect(cfg.host.c_str(),cfg.port,1200)){
      Serial.println("[APRS-IS] Baglanti basarisiz");return;
    }
    aprs.setNoDelay(true);
    aprsConnectStarted=millis();
    aprsLine="";
    String login="user "+cfg.call+" pass "+cfg.passcode+" vers TA1FM-LoRa-iGate DEV5";
    if(cfg.messagingEnabled){login+=" filter g/"+cfg.call;if(validCall(cfg.trackerCall))login+="/"+cfg.trackerCall;}
    login+="\r\n";
    aprs.print(login);loginSent=true;
    Serial.println("[APRS-IS] Giris istendi");
  }
  // Read only a bounded amount per loop; never block waiting for a line.
  int budget=256;
  while(aprs.available() && budget-- >0){
    char c=aprs.read();
    if(c=='\n'){
      aprsLine.trim();
      if(aprsLine.startsWith("#")) parseAprsResponse(aprsLine);
      else examineIncomingAprs(aprsLine,"IS");
      aprsLine="";
    } else if(c!='\r') {
      if(aprsLine.length()<255) aprsLine+=c;
      else aprsLine="";
    }
  }
  if(!aprsVerified){
    if(aprsConnectStarted && millis()-aprsConnectStarted>10000){
      Serial.println("[APRS-IS] Login dogrulama zaman asimi");disconnectAprs();
    }
    return;
  }
  sendOwnBeacon();
  manageTrackerIs();
  manageTrackerStatusIs();
  manageTelemetryIs();
  manageMessagingIs();
  refreshBridgeFilter();
  if(aprsVerified && aprs.connected()) sendOwnStatus();
  // Send at most one queued frame per loop; TCP write success is not server acceptance.
  if(!digiMode && queueCount && aprs.connected()){
    Pending &p=queueData[queueHead];
    // Binary-safe TCP: send the full byte sequence, including embedded 0x00.
    size_t wrote=aprs.write(p.line,p.len);
    const uint8_t crlf[2]={'\r','\n'};
    if(wrote==p.len && aprs.write(crlf,2)==2){
      Serial.printf("[APRS-IS] TCP gonderildi: %u byte (binary-safe)\n",(unsigned)p.len);
      queueHead=(queueHead+1)%12;queueCount--;countSent++;
    }else disconnectAprs();
  }
}


void handleActionResult(){
  if(!accessAllowed())return;
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json","{\"rf\":"+jsonEscape(manualRfResult)+",\"is\":"+jsonEscape(manualIsResult)+"}");
}
void handleIsToggle(){
  if(!accessAllowed())return;
  cfg.aprsIsEnabled=!cfg.aprsIsEnabled;
  prefs.putBool("isenabled",cfg.aprsIsEnabled);
  if(!cfg.aprsIsEnabled){
    disconnectAprs();
    aprsDownFrom=millis();
    updateHybridMode(); // Immediate Digi when enabled, independent of Wi-Fi.
    addEvent("APRS-IS manuel KAPATILDI - Wi-Fi devam");
  }else{
    aprsDownFrom=millis();lastAprsAttempt=0;lastDigiProbe=0;
    // Stop digipeating while reconnecting; prevent RF/IS double mode.
    digiMode=false;digiPending="";rfBeaconDue=false;
    addEvent("APRS-IS manuel ACILDI - baglanti deneniyor");
  }
  server.send(200,"text/plain; charset=utf-8",cfg.aprsIsEnabled?"APRS-IS ACILIYOR":"APRS-IS MANUEL KAPALI");
}

// Read-only JSON endpoint. The page polls it every 15 seconds without reloading.
void handleRxJson(){
  if(!accessAllowed())return;
  String p="{\"count\":"+String(countRx)+",\"rows\":[";
  for(uint8_t i=0;i<rxCount;i++){
    int idx=(rxHead+15-1-i)%15;
    const RxRecord &r=rxRecords[idx];
    if(i)p+=",";
    p+="{\"time\":"+jsonEscape(rxClock(r.receivedAt))+
       ",\"call\":"+jsonEscape(r.station)+",\"rssi\":"+String(r.rssi,1)+
       ",\"snr\":"+String(r.snr,1)+",\"action\":"+jsonEscape(r.action)+
       ",\"packet\":"+jsonEscape(r.packet)+"}";
  }
  p+="]}";
  server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",p);
}
void handleRxPage(){
  if(!accessAllowed())return;
  String p=pageHeader("Son 15 RX Beacon - 15 saniyede bir guncellenir");
  p+="<div class='card'>";
  p+="<p id='summary'>Yukleniyor...</p><div style='overflow-x:auto'><table><thead><tr><th>Saat</th><th>Cagri</th><th>RSSI</th><th>SNR</th><th>Islem</th></tr></thead><tbody id='rows'></tbody></table></div>";
  p+="<p class='subtle'>Satira tiklayinca tam APRS paketi gorulur. Sayfa sabit kalir, veri 15 sn'de bir yenilenir.</p>";
  p+="<pre id='fullPacket' style='white-space:pre-wrap;word-break:break-all'></pre></div>";
  p+=R"JS(<script>
async function refreshRx(){try{
 const r=await fetch('/api/rx?'+Date.now(),{cache:'no-store'});if(!r.ok)throw Error(r.status);
 const d=await r.json(),b=document.getElementById('rows');b.replaceChildren();
 for(const item of d.rows){const tr=document.createElement('tr');for(const k of ['time','call','rssi','snr','action']){const td=document.createElement('td');td.textContent=item[k];tr.appendChild(td)}tr.style.cursor='pointer';tr.onclick=()=>document.getElementById('fullPacket').textContent=item.packet;b.appendChild(tr)}
 document.getElementById('summary').textContent='Toplam RX: '+d.count+' | Listelenen: '+d.rows.length+' | Guncelleme: '+new Date().toLocaleTimeString('tr-TR');
}catch(e){document.getElementById('summary').textContent='Baglanti hatasi: '+e.message}}
refreshRx();setInterval(refreshRx,15000);
</script>)JS";
  p+="</main></body></html>";server.send(200,"text/html; charset=utf-8",p);
}
void handleEvents(){
  if(!accessAllowed())return;
  if(server.hasArg("download")){
    String body="TA1FM V1.5.8 Olay Kayitlari (son 24 / RAM)\n";
    for(uint8_t i=0;i<eventCount;i++){int idx=(eventHead+24-eventCount+i)%24;body+=String(events[idx].at/1000)+"s | "+events[idx].message+"\n";}
    server.sendHeader("Content-Disposition","attachment; filename=TA1FM_olaylar.txt");server.send(200,"text/plain; charset=utf-8",body);return;
  }
  String p=pageHeader("Son olaylar - RAM kayitlari");
  p+="<div class='card'><p><a href='/events?download=1'>Olaylari Indir</a></p>";
  for(uint8_t i=0;i<eventCount;i++){int idx=(eventHead+24-1-i)%24;p+="<p><b>"+String(events[idx].at/1000)+"s</b> "+htmlEscape(events[idx].message)+"</p>";}
  p+="<small>Yeniden baslatilinca kayitlar silinir.</small></div></main></body></html>";
  server.send(200,"text/html; charset=utf-8",p);
}
void handleOtaPage(){
  if(!accessAllowed())return;
  String p=pageHeader("OTA Firmware Guncelleme - 4 MB Default / OTA bolumu gerekir");
  p+="<div class='card'>";
  p+="<p><b>UYARI:</b> Yalniz bu ESP32-S3 cihazina ait derlenmis .bin dosyasini yukleyin. ";
  p+="Mevcut partition OTA desteklemiyorsa islem baslatilamaz. USB ile kurtarma erisiminizi koruyun.</p>";
  p+="<form id='otaForm' onsubmit='return doOta(event)'><input type='file' id='bin' name='firmware' accept='.bin,application/octet-stream' required>";
  p+="<button type='submit'>BIN Dosyasini Yukle</button></form><p id='otaMsg'></p></div>";
  p+=R"JS(<script>
async function doOta(e){e.preventDefault();const f=document.getElementById('otaForm'),file=document.getElementById('bin').files[0];if(!file||!file.name.toLowerCase().endsWith('.bin')){toast('Yalniz .bin kabul edilir','error');return false;}
 if(!confirm('OTA baslasin mi? Islem sirasinda cihazin enerjisini kesmeyin.'))return false;
 let boot=0;try{boot=(await (await fetch('/health')).json()).boot}catch(e){}
 const b=document.querySelector('#otaForm button');b.disabled=true;toast('Yukleniyor, cihazin enerjisini kesmeyin...', 'success',0);
 try{const r=await fetch('/ota',{method:'POST',body:new FormData(f)});const msg=await r.text();
 if(!r.ok){toast(msg,'error',0);b.disabled=false;return false;}toast(msg,'success',0);checkReboot(boot);
 }catch(err){toast('OTA baglanti hatasi: '+err,'error',0);b.disabled=false;}return false;}
</script>)JS";
  p+="</main></body></html>";server.send(200,"text/html; charset=utf-8",p);
}
void handleOtaUpload(){
  if(!validAdminAuthHeader())return;
  HTTPUpload &u=server.upload();
  if(u.status==UPLOAD_FILE_START){
    otaFailed=false;otaStarted=false;otaSuccessful=false;otaError="";
    if(!u.filename.endsWith(".bin")){otaFailed=true;otaError="Dosya .bin olmali";return;}
    if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)){
      otaFailed=true;otaError="OTA bolumu yetersiz veya yok (partition kontrolu). Hata "+String(Update.getError());return;
    }
    otaStarted=true;
    addEvent("OTA yukleme basladi: "+u.filename);
  }else if(u.status==UPLOAD_FILE_WRITE){
    if(otaStarted&&!otaFailed){if(Update.write(u.buf,u.currentSize)!=u.currentSize){otaFailed=true;otaError="Flash yazma hatasi "+String(Update.getError());Update.abort();}}
  }else if(u.status==UPLOAD_FILE_END){
    if(otaStarted&&!otaFailed){if(!Update.end(true)){otaFailed=true;otaError="OTA bitirme hatasi "+String(Update.getError());}else otaSuccessful=true;}
    otaStarted=false;
  }else if(u.status==UPLOAD_FILE_ABORTED){
    if(otaStarted)Update.abort();otaStarted=false;otaFailed=true;otaError="OTA iptal edildi";
  }
}
void handleOtaFinish(){
  if(!accessAllowed())return;
  if(otaFailed || !otaSuccessful){
    server.send(400,"text/plain; charset=utf-8","OTA basarisiz: "+otaError+". Calisan yazilim korunuyor.");
    addEvent("OTA basarisiz: "+otaError);return;
  }
  addEvent("OTA tamamlandi; yeniden baslatiliyor");
  server.send(200,"text/plain; charset=utf-8","OTA tamamlandi. Cihaz yeniden baslatiliyor...");
  restartAt=millis()+1600;
}


// ---- BME280 / APRS TLM: never publish absent, stale or implausible sensor values. ----
void maintainBme(){
  uint32_t now=millis();
  if(!cfg.telemetryEnabled){bmeFresh=false;return;}
  if(!bmeDetected){
    if(bmeLastProbe && (uint32_t)(now-bmeLastProbe)<30000UL)return;
    bmeLastProbe=now;
    if(bme.begin(0x76,&Wire1)){bmeDetected=true;bmeAddress=0x76;}
    else if(bme.begin(0x77,&Wire1)){bmeDetected=true;bmeAddress=0x77;}
    else {bmeDetected=false;bmeAddress=0;}
    if(bmeDetected)addEvent("BME280 algilandi");
    else bmeFresh=false;
    return;
  }
  if((uint32_t)(now-bmeLastRead)<5000UL)return;
  bmeLastRead=now;
  // Re-check physical chip ID on each sample; cached readings must never be transmitted.
  Wire1.beginTransmission(bmeAddress);Wire1.write((uint8_t)0xD0);
  if(Wire1.endTransmission(false)!=0 || Wire1.requestFrom(bmeAddress,(uint8_t)1)!=(uint8_t)1 || Wire1.read()!=0x60){
    bmeFresh=false;bmeDetected=false;bmeLastProbe=now;
    addEvent("BME280 baglantisi kesildi; telemetri durduruldu");return;
  }
  float t=bme.readTemperature(),h=bme.readHumidity(),p=bme.readPressure()/100.0f;
  if(!isfinite(t)||!isfinite(h)||!isfinite(p)||t< -40||t>85||h<0||h>100||p<300||p>1100){
    bmeFresh=false;bmeDetected=false;bmeLastProbe=now;
    addEvent("BME280 olcum gecersiz; telemetri durduruldu");return;
  }
  bmeTemp=t;bmeHum=h;bmePressure=p;bmeFresh=true;
}
bool telemetryReady(){return cfg.telemetryEnabled && cfg.telemetryMode && bmeDetected && bmeFresh &&
   (uint32_t)(millis()-bmeLastRead)<12000UL && validCall(cfg.call);}
String tlmPacket(){
  // T# analog channels 0-255.  PARM/UNIT/EQNS describe scaling:
  // T=(A1/2)-40 C; RH=A2/2 percent; Pressure=(A3*4)+300 hPa.
  uint8_t t=(uint8_t)constrain(lroundf((bmeTemp+40)*2),0L,255L);
  uint8_t h=(uint8_t)constrain(lroundf(bmeHum*2),0L,255L);
  uint8_t p=(uint8_t)constrain(lroundf((bmePressure-300)/4),0L,255L);
  char buf[64];snprintf(buf,sizeof(buf),"T#%03u,%03u,%03u,%03u,000,000,00000000",tlmSequence%1000,t,h,p);
  return cfg.call+">"+OWN_TOCALL+":"+String(buf);
}
String tlmDefinition(){
  String dest=cfg.call;while(dest.length()<9)dest+=" ";
  String body;
  if(tlmDescriptionStage==0)body="PARM.Temp,Humidity,Pressure,Unused,Unused";
  else if(tlmDescriptionStage==1)body="UNIT.C,percent,hPa,-,-";
  else body="EQNS.0,0.5,-40,0,0.5,0,0,4,300,0,1,0,0,1,0";
  return cfg.call+">"+OWN_TOCALL+"::"+dest+":"+body;
}
void manageTelemetryIs(){
  if(!(cfg.telemetryMode&2)||!telemetryReady()||!cfg.aprsIsEnabled||!aprsVerified||!aprs.connected())return;
  uint32_t now=millis();
  if(tlmLastIs && (uint32_t)(now-tlmLastIs)<(uint32_t)cfg.telemetryMinutes*60000UL)return;
  String line;
  if(tlmDescriptionStage<3)line=tlmDefinition();else line=tlmPacket();
  int colon=line.indexOf(':');if(colon<0)return;
  line=line.substring(0,colon)+",TCPIP*"+line.substring(colon);
  if(aprs.print(line+"\r\n")==line.length()+2){
    if(tlmDescriptionStage<3)tlmDescriptionStage++;
    else {tlmSequence++;tlmLastIs=now;tlmDescriptionStage=(tlmSequence%12==0?0:3);}
    Serial.println("[TLM IS] "+line);
  }else disconnectAprs();
}
void manageTelemetryRf(){
  if(!(cfg.telemetryMode&1)||!radioOk||!telemetryReady())return;
  uint32_t now=millis();
  if(tlmLastRf && (uint32_t)(now-tlmLastRf)<(uint32_t)cfg.telemetryMinutes*60000UL)return;
  if(tlmRfAttempt && (uint32_t)(now-tlmRfAttempt)<30000UL)return;
  if(!digiPending.isEmpty()||(lastDigiTx&&(uint32_t)(now-lastDigiTx)<TX_GAP_MS))return;
  if(txWindowStart==0||(uint32_t)(now-txWindowStart)>=3600000UL){txWindowStart=now;txWindowCount=0;}
  if(txWindowCount>=TX_MAX_PER_HOUR)return;
  String line=tlmPacket();if(line.length()>252)return;
  tlmRfAttempt=now;
  if(!prepareTxPower("BME TLM")){radio.startReceive();return;}
  noInterrupts();rxFlag=false;interrupts();
  if(radio.scanChannel()!=RADIOLIB_CHANNEL_FREE){radio.startReceive();return;}
  uint8_t frame[255]={0x3c,0xff,0x01};memcpy(frame+3,line.c_str(),line.length());
  int16_t rc=radio.transmit(frame,line.length()+3);
  lastDigiTx=millis();txWindowCount++;
  if(rc==RADIOLIB_ERR_NONE){tlmLastRf=lastDigiTx;tlmSequence++;Serial.println("[TLM RF] "+line);}
  noInterrupts();rxFlag=false;interrupts();radio.startReceive();
}
void handleTelemetry(){
  if(!accessAllowed())return;
  String p=pageHeader("Telemetri / BME280");
  p+="<div class='card'><h2>BME280 Telemetri</h2>";
  p+="<p>Sensor: <b>"+String(bmeDetected&&bmeFresh?"OK":"YOK / GECERSIZ")+"</b></p>";
  if(bmeDetected&&bmeFresh)p+="<p>Sicaklik: "+String(bmeTemp,1)+" C | Nem: "+String(bmeHum,1)+" % | Basinc: "+String(bmePressure,1)+" hPa</p>";
  p+="<form method='POST' action='/telemetry/save'>";
  p+="<label><input type='checkbox' style='width:auto' name='enabled' value='1' "+String(cfg.telemetryEnabled?"checked":"")+"> BME280 telemetri aktif</label>";
  p+="<label>Gonderim kanali</label><select name='mode'>";
  const char* modes[]={"Kapali","LoRa RF","APRS-IS","Ikisi de"};
  for(int i=0;i<4;i++)p+="<option value='"+String(i)+"' "+String(cfg.telemetryMode==i?"selected":"")+">"+modes[i]+"</option>";
  p+="</select><label>Aralik (15-240 dakika)</label><input type='number' name='minutes' min='15' max='240' value='"+String(cfg.telemetryMinutes)+"'>";
  p+="<button type='submit'>Kaydet</button></form><small>Sensor bulunamaz veya okuma gecersizse hicbir telemetri paketi gonderilmez. WX degil APRS TLM kullanilir. RF TX limiti ortak korunur.</small></div></main></body></html>";
  server.send(200,"text/html; charset=utf-8",p);
}
void handleTelemetrySave(){
  if(!accessAllowed())return;
  int mode=server.arg("mode").toInt(),minutes=server.arg("minutes").toInt();
  if(mode<0||mode>3||minutes<15||minutes>240){server.send(400,"text/plain","Gecersiz telemetri ayari");return;}
  cfg.telemetryEnabled=server.hasArg("enabled");cfg.telemetryMode=mode;cfg.telemetryMinutes=minutes;
  prefs.putBool("tlmena",cfg.telemetryEnabled);prefs.putUChar("tlmmode",mode);prefs.putUShort("tlmmin",minutes);
  bmeDetected=false;bmeFresh=false;bmeLastProbe=0;tlmLastRf=millis();tlmLastIs=millis();tlmDescriptionStage=0;
  server.sendHeader("Location","/telemetry");server.send(303,"text/plain","Kaydedildi");
}
void handleWifiNetworks(){
  if(!accessAllowed())return;
  String p=pageHeader("WiFi / Coklu Aglar");
  p+="<div class='card'><h2>Kayitli Wi-Fi Aglar</h2><p>Ana ag ve ek aglar burada yonetilir. Kaydedildikten sonra baglanti yeniden denenir.</p>";
  p+="<form method='POST' action='/wifi/save'>";
  p+="<h3>Ana Wi-Fi (Oncelik 1)</h3><label>SSID</label><input name='primarySsid' maxlength='32' value='"+htmlEscape(cfg.ssid)+"'>";
  p+="<label>Yeni sifre (bos: mevcut korunur)</label><input type='password' name='primaryPass' maxlength='63' autocomplete='new-password'>";
  for(int i=0;i<4;i++){
    p+="<h3>Ek Ag "+String(i+1)+"</h3><label>SSID</label><input name='s"+String(i)+"' maxlength='32' value='"+htmlEscape(extraWifiSsid[i])+"'>";
    p+="<label>Sifre (bos birak: mevcut sifre korunsun)</label><input type='password' name='p"+String(i)+"' maxlength='63' value='' autocomplete='new-password'>";
    p+="<label><input type='checkbox' style='width:auto' name='d"+String(i)+"' value='1'> Agi sil</label>";
  }
  p+="<button type='submit'>Wi-Fi Aglarini Kaydet</button></form><small>Degisiklikler yeniden baslatmadan sonraki baglanti denemelerinde kullanilir.</small></div></main></body></html>";
  server.send(200,"text/html; charset=utf-8",p);
}
void handleWifiNetworksSave(){
  if(!accessAllowed())return;
  String primary=server.arg("primarySsid"),primaryPass=server.arg("primaryPass");primary.trim();
  if(primary.length()>32 || primaryPass.length()>63 || (primaryPass.length() && primaryPass.length()<8)){
    server.send(400,"text/plain","Ana Wi-Fi SSID/sifre gecersiz");return;
  }
  String ss[4],pw[4];
  for(int i=0;i<4;i++){
    ss[i]=server.arg("s"+String(i));ss[i].trim();pw[i]=server.arg("p"+String(i));
    if(server.hasArg("d"+String(i))){ss[i]="";pw[i]="";}
    if(ss[i].length()>32||pw[i].length()>63||(pw[i].length()&&pw[i].length()<8)){
      server.send(400,"text/plain","Gecersiz Wi-Fi agi/sifresi");return;
    }
  }
  if(primary!=cfg.ssid && primaryPass.isEmpty()){
    server.send(400,"text/plain","Ana Wi-Fi SSID degisirse yeni sifre gerekli");return;
  }
  cfg.ssid=primary; prefs.putString("ssid",primary);
  if(primaryPass.length()){cfg.wifiPassword=primaryPass;prefs.putString("wifi",primaryPass);}
  for(int i=0;i<4;i++){
    if(ss[i]!=extraWifiSsid[i] && pw[i].isEmpty())extraWifiPass[i]="";
    extraWifiSsid[i]=ss[i];if(pw[i].length()||ss[i].isEmpty())extraWifiPass[i]=pw[i];
    prefs.putString(("w"+String(i)+"s").c_str(),extraWifiSsid[i]);
    prefs.putString(("w"+String(i)+"p").c_str(),extraWifiPass[i]);
  }
  server.sendHeader("Location","/wifi");server.send(303,"text/plain","Kaydedildi");
}


// ---- APRS Message transport (RF / APRS-IS) ----
String padAddressee(String call){while(call.length()<9)call+=" ";return call;}
void rememberMessage(const String& src,const String& dst,const String& txt,const String& id,const String& channel){
  AprsMessage &m=messageLog[messageHead];m.from=src;m.to=dst;m.text=txt;m.id=id;m.channel=channel;m.at=millis();
  messageHead=(messageHead+1)%16;if(messageCount<16)messageCount++;
}
bool ownMessageAddress(String s){s.trim();s.toUpperCase();return s==cfg.call || (cfg.trackerCall.length() && s==cfg.trackerCall);}
String messageFrame(String from,String to,String body){return from+">"+OWN_TOCALL+"::"+padAddressee(to)+":"+body;}
bool sendMessageRf(const String& line);
// Check recently received LoRa station callsign, not an arbitrary Internet recipient.
bool bridgeKnownRf(const String& call){
  if(!validCall(call) || ownMessageAddress(call))return false;
  uint32_t now=millis();
  for(uint8_t i=0;i<rxCount;i++){
    const RxRecord &r=rxRecords[(rxHead+15-1-i)%15];
    if(r.station==call && (uint32_t)(now-r.receivedAt)<1800000UL)return true;
  }
  return false;
}
uint32_t bridgeHash(const String& frame){
  uint32_t h=2166136261UL;
  for(size_t i=0;i<frame.length();i++){h^=(uint8_t)frame[i];h*=16777619UL;}
  return h?h:1;
}
bool bridgeDuplicate(const String& frame){
  uint32_t h=bridgeHash(frame),now=millis();
  for(int i=0;i<24;i++)if(bridgeSeenHashes[i]==h && (uint32_t)(now-bridgeSeenAt[i])<600000UL)return true;
  bridgeSeenHashes[bridgeSeenPtr]=h;bridgeSeenAt[bridgeSeenPtr]=now;bridgeSeenPtr=(bridgeSeenPtr+1)%24;
  return false;
}
// APRS-IS is not an open RF relay: only direct ordinary message addressed to a
// recently heard RF station, with strict source/recipient and payload filtering.
void considerIsToRfBridge(const String& line,const String& from,const String& to,const String& body){
  if(!cfg.messageBridgeEnabled || !cfg.messagingEnabled || !bridgeKnownRf(to))return;
  if(!validCall(from) || ownMessageAddress(from) || body.isEmpty() || body.length()>80 || !validAprsText(body,80))return;
  if(!bridgeRfPending.isEmpty())return; // do not create unbounded message queues
  String frame=messageFrame(from,to,body);
  if(frame.length()>240 || bridgeDuplicate(frame))return;
  bridgeRfPending=frame;bridgeRfQueuedAt=millis();bridgeRfLastAttempt=0;
  addEvent("BRIDGE IS->RF sirada: "+from+" > "+to);
}
void manageMessageBridgeRf(){
  if(!cfg.messagingEnabled || !cfg.messageBridgeEnabled){bridgeRfPending="";return;}
  if(bridgeRfPending.isEmpty())return;
  uint32_t now=millis();
  if((uint32_t)(now-bridgeRfQueuedAt)>120000UL){bridgeRfPending="";addEvent("BRIDGE RF kuyruk suresi doldu");return;}
  if(bridgeRfLastAttempt && (uint32_t)(now-bridgeRfLastAttempt)<15000UL)return;
  bridgeRfLastAttempt=now;
  if(sendMessageRf(bridgeRfPending)){addEvent("BRIDGE IS->RF gonderildi");bridgeRfPending="";}
}
// APRS-IS g/ filter follows recently heard LoRa stations. This only requests
// traffic destined to those stations; messageBridgeEnabled=false adds none.
void refreshBridgeFilter(){
  if(!cfg.messagingEnabled || !cfg.messageBridgeEnabled || !aprsVerified || !aprs.connected())return;
  uint32_t now=millis();if(bridgeLastFilter && (uint32_t)(now-bridgeLastFilter)<60000UL)return;
  bridgeLastFilter=now;
  String f="# filter g/"+cfg.call;
  if(validCall(cfg.trackerCall) && cfg.trackerCall!=cfg.call)f+="/"+cfg.trackerCall;
  uint8_t n=0;
  for(uint8_t i=0;i<rxCount && n<8;i++){
    const RxRecord &r=rxRecords[(rxHead+15-1-i)%15];
    if(!bridgeKnownRf(r.station))continue;
    if(f.indexOf("/"+r.station)>=0)continue;
    f+="/"+r.station;n++;
  }
  aprs.print(f+"\r\n");
}
void examineIncomingAprs(const String& line,const String& channel){
  if(!cfg.messagingEnabled)return;
  int gt=line.indexOf('>'),colon=line.indexOf(':');if(gt<3||colon<gt+1)return;
  // Message: :DEST_____ :TEXT{ID ; no third-party parsing.
  if((int)line.length()<colon+12 || line[colon+1]!=':' || line[colon+11]!=':')return;
  String from=line.substring(0,gt);from.toUpperCase();if(!validCall(from))return;
  String to=line.substring(colon+2,colon+11);to.trim();to.toUpperCase();
  bool isOurs=ownMessageAddress(to);
  if(!isOurs && channel!="IS")return;
  String body=line.substring(colon+12);if(body.length()>80||body.isEmpty())return;
  if(!validAprsText(body,80))return;
  if(!isOurs){considerIsToRfBridge(line,from,to,body);return;}
  if(body.startsWith("ack")||body.startsWith("rej")){
    String id=body.substring(3);id.trim();
    if(from==outboundDest && id==outboundId){outboundAcked=true;addEvent("MSG ACK " + from + " kanal="+channel);}
    return;
  }
  String id="";int brace=body.lastIndexOf('{');
  if(brace>=0 && brace>=((int)body.length()-6)){id=body.substring(brace+1);body=body.substring(0,brace);}
  // Avoid duplicate display, but still ACK a genuine retry.
  bool duplicate=false;
  for(int i=0;i<messageCount;i++){
    AprsMessage &m=messageLog[(messageHead+16-1-i)%16];
    if(m.from==from && m.to==to && m.id==id && m.text==body &&
       (uint32_t)(millis()-m.at)<600000UL){duplicate=true;break;}
  }
  if(!duplicate){rememberMessage(from,to,body,id,channel);addEvent("MSG "+channel+" "+from+": "+body.substring(0,45));}
  if(id.length() && id.length()<=5){
    String ack=messageFrame(to,from,"ack"+id);
    if(channel=="RF")ackRfPending=ack;
    else ackIsPending=ack;
  }
}
// RF messages share the existing CAD, PA/OCP and global hourly TX protections.
bool sendMessageRf(const String& line){
  if(!cfg.messagingEnabled || !radioOk || line.isEmpty() || line.length()>252)return false;
  uint32_t now=millis();
  if(!digiPending.isEmpty() || (lastDigiTx && (uint32_t)(now-lastDigiTx)<TX_GAP_MS))return false;
  if(txWindowStart==0||(uint32_t)(now-txWindowStart)>=3600000UL){txWindowStart=now;txWindowCount=0;}
  if(txWindowCount>=TX_MAX_PER_HOUR)return false;
  if(!prepareTxPower("MESSAGE")){radio.startReceive();return false;}
  noInterrupts();rxFlag=false;interrupts();
  if(radio.scanChannel()!=RADIOLIB_CHANNEL_FREE){radio.startReceive();return false;}
  uint8_t data[255]={0x3c,0xff,0x01};memcpy(data+3,line.c_str(),line.length());
  int16_t rc=radio.transmit(data,line.length()+3);
  lastDigiTx=millis();txWindowCount++;
  noInterrupts();rxFlag=false;interrupts();radio.startReceive();
  if(rc==RADIOLIB_ERR_NONE){Serial.println("[MSG RF] "+line);return true;}
  addEvent("MSG RF TX HATA "+String(rc));return false;
}
void manageMessagingRf(){
  if(!cfg.messagingEnabled)return;
  uint32_t now=millis();
  if(ackRfPending.length() && (!ackRfAttempt||(uint32_t)(now-ackRfAttempt)>30000UL)){
    ackRfAttempt=now;
    if(sendMessageRf(ackRfPending))ackRfPending="";
    return;
  }
  if(!(outboundMode&1)||outboundAcked||outboundRfTries>=3||outboundDest.isEmpty())return;
  if(outboundRfLast && (uint32_t)(now-outboundRfLast)<60000UL)return;
  String line=messageFrame(outboundCall,outboundDest,outboundText+"{"+outboundId);
  if(sendMessageRf(line)){outboundRfLast=now;outboundRfTries++;}
}
void manageMessagingIs(){
  if(!cfg.messagingEnabled||!cfg.aprsIsEnabled||!aprsVerified||!aprs.connected())return;
  if(ackIsPending.length()){
    String line=ackIsPending;int colon=line.indexOf(':');
    if(colon>=0){line=line.substring(0,colon)+",TCPIP*"+line.substring(colon);
      if(aprs.print(line+"\r\n")==line.length()+2)ackIsPending="";
      else disconnectAprs();
    }
    return;
  }
  if(!(outboundMode&2)||outboundAcked||outboundIsTries>=3||outboundDest.isEmpty())return;
  uint32_t now=millis();if(outboundIsLast && (uint32_t)(now-outboundIsLast)<60000UL)return;
  String line=messageFrame(outboundCall,outboundDest,outboundText+"{"+outboundId);
  int colon=line.indexOf(':');if(colon<0)return;
  line=line.substring(0,colon)+",TCPIP*"+line.substring(colon);
  if(aprs.print(line+"\r\n")==line.length()+2){outboundIsTries++;outboundIsLast=now;Serial.println("[MSG IS] "+line);}
  else disconnectAprs();
}
void handleMessages(){
  if(!accessAllowed())return;
  String p=pageHeader("Mesajlasma / APRS");
  p+="<div class='card'><h2>APRS Mesajlasma</h2><form method='POST' action='/messages/config'>";
  p+="<label><input type='checkbox' name='enabled' style='width:auto' value='1' "+String(cfg.messagingEnabled?"checked":"")+"> Mesajlasma aktif</label>";
  p+="<label><input type='checkbox' name='bridge' style='width:auto' value='1' "+String(cfg.messageBridgeEnabled?"checked":"")+"> Iki yonlu mesaj koprusu (RF -> IS / IS -> bilinen RF)</label>";
  p+="<small>IS -> RF yalniz son 30 dakikada LoRa'dan duyulmus istasyonlara, TX korumalarina tabidir. Varsayilan kapali.</small><button type='submit'>Kaydet</button></form>";
  p+="<form method='POST' action='/messages/send'><label>Hedef cagri isareti</label><input name='to' maxlength='9' placeholder='TA1FM-8' required>";
  p+="<label>Mesaj (en fazla 60 karakter)</label><input name='msg' maxlength='60' required>";
  p+="<label>Gonderim kanali</label><select name='mode'><option value='1'>LoRa RF</option><option value='2'>APRS-IS</option><option value='3'>Ikisi de</option></select>";
  p+="<button type='submit'>Mesaji Gonder</button></form>";
  p+="<p>Son gonderim: "+htmlEscape(outboundDest)+" | ACK: "+String(outboundAcked?"Alindi":"Bekleniyor / yok")+" | RF deneme: "+String(outboundRfTries)+" | IS deneme: "+String(outboundIsTries)+"</p>";
  p+="<h3>Gelen ve Gonderilen Mesajlar (RAM)</h3><div id='messageFeed'>Yukleniyor...</div>";
  p+=R"JS(<script>
async function refreshMessages(){
 try{const r=await fetch('/api/messages',{cache:'no-store'});if(!r.ok)return;
   const data=await r.json();const box=document.getElementById('messageFeed');if(!box)return;
   box.replaceChildren();
   const table=document.createElement('table');
   const head=document.createElement('thead');const hr=document.createElement('tr');
   for(const label of ['Saat','Yon / Kanal','Kaynak','Hedef','Mesaj','Durum']){const th=document.createElement('th');th.textContent=label;hr.appendChild(th)}
   head.appendChild(hr);table.appendChild(head);
   const body=document.createElement('tbody');
   for(const m of data.messages){const tr=document.createElement('tr');for(const val of [m.time,m.channel,m.from,m.to,m.text,m.status]){const td=document.createElement('td');td.textContent=val;tr.appendChild(td)}body.appendChild(tr)}
   table.appendChild(body);box.replaceChildren(table);
 }catch(e){} }
refreshMessages();setInterval(refreshMessages,3000);
</script>)JS";
  p+="<small>Sayfa 3 saniyede bir guncellenir. Siraya alindi / RF veya IS denendi, teslim onayi degildir. Otomatik mesaj koprusu yalniz izin verilen hedeflere acilir.</small></div></main></body></html>";
  server.send(200,"text/html; charset=utf-8",p);
}
void handleMessagesApi(){
  if(!accessAllowed())return;
  String out="{\"messages\":[";
  for(int i=0;i<messageCount;i++){
    const AprsMessage &m=messageLog[(messageHead+16-1-i)%16];
    String status="Alindi";
    if(m.channel.startsWith("TX")){
      if(m.id==outboundId && m.to==outboundDest){
        if(outboundAcked)status="ACK alindi";
        else if((outboundMode&1 && outboundRfTries>=3) || (outboundMode&2 && outboundIsTries>=3))status="ACK yok / deneme bitti";
        else if(outboundRfTries||outboundIsTries)status="Gonderildi / ACK bekleniyor";
        else status="Sirada";
      } else status="Gecmis (ACK bilinmiyor)";
    }
    if(i)out+=",";
    out+="{\"time\":"+jsonEscape(rxClock(m.at))+",\"from\":"+jsonEscape(m.from)+",\"to\":"+jsonEscape(m.to)+",\"channel\":"+jsonEscape(m.channel)+",\"text\":"+jsonEscape(m.text)+",\"status\":"+jsonEscape(status)+"}";
  }
  out+="]}";server.sendHeader("Cache-Control","no-store");server.send(200,"application/json; charset=utf-8",out);
}
void handleMessagesConfig(){
  if(!accessAllowed())return;
  cfg.messagingEnabled=server.hasArg("enabled");cfg.messageBridgeEnabled=cfg.messagingEnabled && server.hasArg("bridge");
  prefs.putBool("msgena",cfg.messagingEnabled);prefs.putBool("msgbridge",cfg.messageBridgeEnabled);
  bridgeRfPending="";bridgeLastFilter=0;
  if(!cfg.messagingEnabled){ackRfPending="";ackIsPending="";outboundMode=0;}
  server.sendHeader("Location","/messages");server.send(303,"text/plain","Kaydedildi");
}
void handleMessageSend(){
  if(!accessAllowed())return;
  String to=server.arg("to");to.trim();to.toUpperCase();
  String m=server.arg("msg");int mode=server.arg("mode").toInt();
  if(!cfg.messagingEnabled || !validCall(to) || to==cfg.call || (cfg.trackerCall.length()&&to==cfg.trackerCall) ||
     m.isEmpty()||!validAprsText(m,60)||mode<1||mode>3){
    server.send(400,"text/plain","Mesaj ayarlari gecersiz / mesajlasma kapali");return;
  }
  outboundCall=cfg.call;outboundDest=to;outboundText=m;outboundMode=mode;
  outboundId=String(nextMessageNumber++%100);if(outboundId.length()==1)outboundId="0"+outboundId;
  outboundAcked=false;outboundRfTries=0;outboundIsTries=0;outboundRfLast=0;outboundIsLast=0;
  rememberMessage(outboundCall,to,m,outboundId,String("TX ")+(mode==1?"RF":mode==2?"IS":"RF+IS"));
  addEvent("MSG kuyruga alindi: "+to);
  server.sendHeader("Location","/messages");server.send(303,"text/plain","Siraya alindi");
}


// Profiles are conveniences, not mandatory modes. All individual settings remain editable.
void handleProfiles(){
  if(!accessAllowed())return;
  String p=pageHeader("Profil / Sabit Mobil");
  p+="<div class='card'><h2>Hazir Calisma Profilleri</h2><p>Profil secimi yalnizca belirtilen ayarlari degistirir; Digi, Mesaj, Telemetri, Wi-Fi ve RF limitleri korunur.</p>";
  p+="<form method='POST' action='/profiles/apply'><label>Profil</label><select name='preset'>";
  p+="<option value='fixed'>Sabit iGate: iGate sabit koordinat, Tracker kapali</option>";
  p+="<option value='mobileis'>Mobil iGate: iGate GPS, Tracker APRS-IS</option>";
  p+="<option value='mobileboth'>Mobil iGate: iGate GPS, Tracker LoRa + APRS-IS</option>";
  p+="</select><label><input type='checkbox' name='confirm' value='1' style='width:auto' required> Profil ayarlarini uygula</label>";
  p+="<button type='submit'>Profili Uygula</button></form>";
  p+="<small>Tracker icin ayri gecerli cagri isareti girilmelidir. SmartBeaconing ve diger tum ayarlar Tracker sekmesinde duzenlenebilir.</small></div></main></body></html>";
  server.send(200,"text/html; charset=utf-8",p);
}
void handleProfileApply(){
  if(!accessAllowed())return;
  if(!server.hasArg("confirm")){server.send(400,"text/plain","Onay gerekli");return;}
  String preset=server.arg("preset");
  if(preset!="fixed" && preset!="mobileis" && preset!="mobileboth"){
    server.send(400,"text/plain","Gecersiz profil");return;
  }
  if(preset!="fixed" && (!validCall(cfg.trackerCall)||cfg.trackerCall==cfg.call)){
    server.send(400,"text/plain","Once Tracker sekmesinden ayri cagri isareti ayarlayin");return;
  }
  cfg.igateUseGps=preset!="fixed";
  cfg.trackerMode=preset=="fixed"?0:(preset=="mobileis"?2:3);
  prefs.putBool("igategps",cfg.igateUseGps);prefs.putUChar("trkmode",cfg.trackerMode);
  trackerLastRf=millis();trackerLastIs=millis();
  addEvent("Profil uygulandi: "+preset);
  server.sendHeader("Location","/system");server.send(303,"text/plain","Profil uygulandi");
}

// DEV7: comprehensive and versioned settings backup. Credentials in export are PLAINTEXT.
// Admin verifier is intentionally excluded; admin password is not reversible.
void handleBackupExport(){
  if(!accessAllowed())return;
  String j="{\"_format\":\"TA1FM-V2.0-DEV7-settings\",\"_version\":1,\"settings\":{";
  bool first=true;
  auto add=[&](const char* name,const String &value){
    if(!first)j+=",";first=false;
    j+=jsonEscape(String(name))+":"+jsonEscape(value);
  };

  add("call",cfg.call);
  add("ssid",cfg.ssid);
  add("wifi",cfg.wifiPassword);
  add("host",cfg.host);
  add("pass",cfg.passcode);
  add("port",String(cfg.port));
  add("lat",String(cfg.latitude, 8));
  add("lon",String(cfg.longitude, 8));
  add("bmin",String(cfg.beaconMinutes));
  add("bena",(cfg.beaconEnabled?"1":"0"));
  add("symtab",cfg.symbolTable);
  add("symcode",cfg.symbolCode);
  add("comment",cfg.commentText);
  add("status",cfg.statusText);
  add("stena",(cfg.statusEnabled?"1":"0"));
  add("stmin",String(cfg.statusMinutes));
  add("autodigi",(cfg.autoDigi?"1":"0"));
  add("isenabled",(cfg.aprsIsEnabled?"1":"0"));
  add("wide1",(cfg.wide1?"1":"0"));
  add("wide2",(cfg.wide2?"1":"0"));
  add("rfpath",cfg.rfBeaconPath);
  add("txpwr",String(cfg.txPowerDbm));
  add("oledtype",String(cfg.oledType));
  add("batbeacon",(cfg.beaconBattery?"1":"0"));
  add("batcal",String(cfg.batteryCalibration, 4));
  add("oledsleep",String(cfg.oledSleepSeconds));
  add("probemin",String(cfg.probeMinutes));
  add("msgena",(cfg.messagingEnabled?"1":"0"));
  add("msgbridge",(cfg.messageBridgeEnabled?"1":"0"));
  add("tlmena",(cfg.telemetryEnabled?"1":"0"));
  add("tlmmode",String(cfg.telemetryMode));
  add("tlmmin",String(cfg.telemetryMinutes));
  add("trkturn",(cfg.trackerTurnEnabled?"1":"0"));
  add("turnang",String(cfg.trackerTurnDegrees));
  add("turnsec",String(cfg.trackerTurnMinSeconds));
  add("trkmode",String(cfg.trackerMode));
  add("trkcall",cfg.trackerCall);
  add("trktab",cfg.trackerTable);
  add("trksym",cfg.trackerSymbol);
  add("trkcomm",cfg.trackerComment);
  add("trkmin",String(cfg.trackerMinutes));
  add("trksmart",(cfg.trackerSmart?"1":"0"));
  add("trkslowv",String(cfg.trackerSlowKmh));
  add("trkfastv",String(cfg.trackerFastKmh));
  add("trkslowt",String(cfg.trackerSlowSeconds));
  add("trkfastt",String(cfg.trackerFastSeconds));
  add("tstena",(cfg.trackerStatusEnabled?"1":"0"));
  add("tstmode",String(cfg.trackerStatusMode));
  add("tstmin",String(cfg.trackerStatusMinutes));
  add("tsttext",cfg.trackerStatusText);
  add("igategps",(cfg.igateUseGps?"1":"0"));
  add("apkey",cfg.apPassword);
  add("w0s",extraWifiSsid[0]);
  add("w0p",extraWifiPass[0]);
  add("w1s",extraWifiSsid[1]);
  add("w1p",extraWifiPass[1]);
  add("w2s",extraWifiSsid[2]);
  add("w2p",extraWifiPass[2]);
  add("w3s",extraWifiSsid[3]);
  add("w3p",extraWifiPass[3]);
  j+="}}";
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("Content-Disposition","attachment; filename=TA1FM_V2_TUM_AYARLAR.json");
  server.send(200,"application/json; charset=utf-8",j);
}

void handleBackupImport(){
  if(!accessAllowed())return;
  if(server.arg("_format")!="TA1FM-V2.0-DEV7-settings" || server.arg("_version")!="1"){
    server.send(400,"text/plain","Yedek surumu uyumsuz");return;
  }
  // Validate ALL values before writing ANY NVS keys.
  bool valid=true;
  String bad;
  auto validate=[&](const char* key,char kind,const String &v,bool rangeOk){
    if(!server.hasArg(key)){valid=false;bad=String("Eksik: ")+key;return;}
    if(kind=='B' && v!="0" && v!="1"){valid=false;bad=String("Bool: ")+key;return;}
    if(!rangeOk){valid=false;bad=String("Gecersiz: ")+key;return;}
  };
  long n=0;double d=0;
  auto parseInt=[&](const String& v)->bool{
    if(v.isEmpty())return false;
    for(size_t i=0;i<v.length();i++)if(v[i]<'0'||v[i]>'9')return false;
    n=v.toInt();return true;
  };
  auto parseDouble=[&](const String& v)->bool{
    if(v.isEmpty()||v.length()>28)return false;
    for(size_t i=0;i<v.length();i++)if(!((v[i]>='0'&&v[i]<='9')||v[i]=='.'||v[i]=='-'||v[i]=='+'))return false;
    d=v.toDouble();return isfinite(d);
  };

  if(valid){String v=server.arg("call");validate("call",'S',v,validCall(v));}
  if(valid){String v=server.arg("ssid");validate("ssid",'S',v,v.length()<=32);}
  if(valid){String v=server.arg("wifi");validate("wifi",'S',v,v.length()<=63);}
  if(valid){String v=server.arg("host");validate("host",'S',v,v.length()>0 && v.length()<=80 && v.indexOf(' ')<0);}
  if(valid){String v=server.arg("pass");validate("pass",'S',v,v.length()<=8);}
  if(valid){String v=server.arg("port");validate("port",'I',v,parseInt(v) && (n>=1 && n<=65535));}
  if(valid){String v=server.arg("lat");validate("lat",'F',v,parseDouble(v) && (d>=-90 && d<=90));}
  if(valid){String v=server.arg("lon");validate("lon",'F',v,parseDouble(v) && (d>=-180 && d<=180));}
  if(valid){String v=server.arg("bmin");validate("bmin",'I',v,parseInt(v) && (n>=10 && n<=120));}
  if(valid){String v=server.arg("bena");validate("bena",'B',v,true);}
  if(valid){String v=server.arg("symtab");validate("symtab",'S',v,v.length()==1 && (v=="/" || v=="\\" || isalnum((unsigned char)v[0])));}
  if(valid){String v=server.arg("symcode");validate("symcode",'S',v,v.length()==1 && v[0]>=33 && v[0]<=126 && v[0]!='<' && v[0]!='>');}
  if(valid){String v=server.arg("comment");validate("comment",'S',v,validAprsText(v,43));}
  if(valid){String v=server.arg("status");validate("status",'S',v,validAprsText(v,62));}
  if(valid){String v=server.arg("stena");validate("stena",'B',v,true);}
  if(valid){String v=server.arg("stmin");validate("stmin",'I',v,parseInt(v) && (n>=15 && n<=240));}
  if(valid){String v=server.arg("autodigi");validate("autodigi",'B',v,true);}
  if(valid){String v=server.arg("isenabled");validate("isenabled",'B',v,true);}
  if(valid){String v=server.arg("wide1");validate("wide1",'B',v,true);}
  if(valid){String v=server.arg("wide2");validate("wide2",'B',v,true);}
  if(valid){String v=server.arg("rfpath");validate("rfpath",'S',v,v=="DIRECT" || v=="WIDE1-1" || v=="WIDE2-1" || v=="WIDE2-2");}
  if(valid){String v=server.arg("txpwr");validate("txpwr",'I',v,parseInt(v) && (n==10 || n==14 || n==17 || n==20 || n==21));}
  if(valid){String v=server.arg("oledtype");validate("oledtype",'I',v,parseInt(v) && (n<=2));}
  if(valid){String v=server.arg("batbeacon");validate("batbeacon",'B',v,true);}
  if(valid){String v=server.arg("batcal");validate("batcal",'F',v,parseDouble(v) && (d>=-1 && d<=1));}
  if(valid){String v=server.arg("oledsleep");validate("oledsleep",'I',v,parseInt(v) && (n==0 || n==15 || n==30 || n==60 || n==300 || n==600));}
  if(valid){String v=server.arg("probemin");validate("probemin",'I',v,parseInt(v) && (n==1 || n==5 || n==10 || n==15));}
  if(valid){String v=server.arg("msgena");validate("msgena",'B',v,true);}
  if(valid){String v=server.arg("msgbridge");validate("msgbridge",'B',v,true);}
  if(valid){String v=server.arg("tlmena");validate("tlmena",'B',v,true);}
  if(valid){String v=server.arg("tlmmode");validate("tlmmode",'I',v,parseInt(v) && (n<=3));}
  if(valid){String v=server.arg("tlmmin");validate("tlmmin",'I',v,parseInt(v) && (n>=15 && n<=240));}
  if(valid){String v=server.arg("trkturn");validate("trkturn",'B',v,true);}
  if(valid){String v=server.arg("turnang");validate("turnang",'I',v,parseInt(v) && (n>=15 && n<=150));}
  if(valid){String v=server.arg("turnsec");validate("turnsec",'I',v,parseInt(v) && (n>=15 && n<=300));}
  if(valid){String v=server.arg("trkmode");validate("trkmode",'I',v,parseInt(v) && (n<=3));}
  if(valid){String v=server.arg("trkcall");validate("trkcall",'S',v,v.isEmpty() || validCall(v));}
  if(valid){String v=server.arg("trktab");validate("trktab",'S',v,v=="/" || v=="\\");}
  if(valid){String v=server.arg("trksym");validate("trksym",'S',v,v.length()==1 && v[0]>=33 && v[0]<=126 && v[0]!='<');}
  if(valid){String v=server.arg("trkcomm");validate("trkcomm",'S',v,validAprsText(v,43));}
  if(valid){String v=server.arg("trkmin");validate("trkmin",'I',v,parseInt(v) && (n>=1 && n<=120));}
  if(valid){String v=server.arg("trksmart");validate("trksmart",'B',v,true);}
  if(valid){String v=server.arg("trkslowv");validate("trkslowv",'I',v,parseInt(v) && (n<=100));}
  if(valid){String v=server.arg("trkfastv");validate("trkfastv",'I',v,parseInt(v) && (n>=1 && n<=200));}
  if(valid){String v=server.arg("trkslowt");validate("trkslowt",'I',v,parseInt(v) && (n>=30 && n<=7200));}
  if(valid){String v=server.arg("trkfastt");validate("trkfastt",'I',v,parseInt(v) && (n>=15 && n<=1800));}
  if(valid){String v=server.arg("tstena");validate("tstena",'B',v,true);}
  if(valid){String v=server.arg("tstmode");validate("tstmode",'I',v,parseInt(v) && (n<=3));}
  if(valid){String v=server.arg("tstmin");validate("tstmin",'I',v,parseInt(v) && (n>=1 && n<=240));}
  if(valid){String v=server.arg("tsttext");validate("tsttext",'S',v,validAprsText(v,62));}
  if(valid){String v=server.arg("igategps");validate("igategps",'B',v,true);}
  if(valid){String v=server.arg("apkey");validate("apkey",'S',v,v.length()>=8 && v.length()<=63);}
  if(valid){String v=server.arg("w0s");validate("w0s",'S',v,v.length()<=32);}
  if(valid){String v=server.arg("w0p");validate("w0p",'S',v,v.length()<=63);}
  if(valid){String v=server.arg("w1s");validate("w1s",'S',v,v.length()<=32);}
  if(valid){String v=server.arg("w1p");validate("w1p",'S',v,v.length()<=63);}
  if(valid){String v=server.arg("w2s");validate("w2s",'S',v,v.length()<=32);}
  if(valid){String v=server.arg("w2p");validate("w2p",'S',v,v.length()<=63);}
  if(valid){String v=server.arg("w3s");validate("w3s",'S',v,v.length()<=32);}
  if(valid){String v=server.arg("w3p");validate("w3p",'S',v,v.length()<=63);}
  if(valid){
    String trkCall=server.arg("trkcall"),mainCall=server.arg("call");
    int trkMode=server.arg("trkmode").toInt(),statusMode=server.arg("tstmode").toInt();
    if((trkMode || (server.arg("tstena")=="1" && statusMode)) && (trkCall.isEmpty() || trkCall==mainCall)){
      valid=false;bad="Tracker callsign eksik veya iGate ile ayni";
    }
    if(server.arg("trkfastv").toInt()<=server.arg("trkslowv").toInt()){
      valid=false;bad="Tracker hiz esikleri gecersiz";
    }
    if(server.arg("msgbridge")=="1" && server.arg("msgena")!="1"){
      valid=false;bad="Kopru acikken mesajlasma kapali olamaz";
    }
    if(server.arg("bena")=="1" && server.arg("igategps")!="1" &&
       server.arg("lat").toDouble()==0 && server.arg("lon").toDouble()==0){
      valid=false;bad="Sabit beacon icin koordinat gerekli";
    }
    for(int i=0;i<4;i++){
      String ss=server.arg("w"+String(i)+"s"),pw=server.arg("w"+String(i)+"p");
      if(ss.length()>0 && pw.length()>0 && pw.length()<8){valid=false;bad="Ek Wi-Fi sifresi gecersiz";break;}
    }
    String ss=server.arg("ssid"),pw=server.arg("wifi");
    if(ss.length()>0 && pw.length()>0 && pw.length()<8){valid=false;bad="Ana Wi-Fi sifresi gecersiz";}
  }
  if(!valid){server.send(400,"text/plain; charset=utf-8",bad);return;}
  // Reboot after successful import. Existing admin password/hash is preserved.

  prefs.putString("call",server.arg("call"));
  prefs.putString("ssid",server.arg("ssid"));
  prefs.putString("wifi",server.arg("wifi"));
  prefs.putString("host",server.arg("host"));
  prefs.putString("pass",server.arg("pass"));
  prefs.putUShort("port",(uint16_t)server.arg("port").toInt());
  prefs.putDouble("lat",server.arg("lat").toDouble());
  prefs.putDouble("lon",server.arg("lon").toDouble());
  prefs.putUShort("bmin",(uint16_t)server.arg("bmin").toInt());
  prefs.putBool("bena",server.arg("bena")=="1");
  prefs.putString("symtab",server.arg("symtab"));
  prefs.putString("symcode",server.arg("symcode"));
  prefs.putString("comment",server.arg("comment"));
  prefs.putString("status",server.arg("status"));
  prefs.putBool("stena",server.arg("stena")=="1");
  prefs.putUShort("stmin",(uint16_t)server.arg("stmin").toInt());
  prefs.putBool("autodigi",server.arg("autodigi")=="1");
  prefs.putBool("isenabled",server.arg("isenabled")=="1");
  prefs.putBool("wide1",server.arg("wide1")=="1");
  prefs.putBool("wide2",server.arg("wide2")=="1");
  prefs.putString("rfpath",server.arg("rfpath"));
  prefs.putUChar("txpwr",(uint8_t)server.arg("txpwr").toInt());
  prefs.putUChar("oledtype",(uint8_t)server.arg("oledtype").toInt());
  prefs.putBool("batbeacon",server.arg("batbeacon")=="1");
  prefs.putFloat("batcal",server.arg("batcal").toDouble());
  prefs.putUShort("oledsleep",(uint16_t)server.arg("oledsleep").toInt());
  prefs.putUChar("probemin",(uint8_t)server.arg("probemin").toInt());
  prefs.putBool("msgena",server.arg("msgena")=="1");
  prefs.putBool("msgbridge",server.arg("msgbridge")=="1");
  prefs.putBool("tlmena",server.arg("tlmena")=="1");
  prefs.putUChar("tlmmode",(uint8_t)server.arg("tlmmode").toInt());
  prefs.putUShort("tlmmin",(uint16_t)server.arg("tlmmin").toInt());
  prefs.putBool("trkturn",server.arg("trkturn")=="1");
  prefs.putUShort("turnang",(uint16_t)server.arg("turnang").toInt());
  prefs.putUShort("turnsec",(uint16_t)server.arg("turnsec").toInt());
  prefs.putUChar("trkmode",(uint8_t)server.arg("trkmode").toInt());
  prefs.putString("trkcall",server.arg("trkcall"));
  prefs.putString("trktab",server.arg("trktab"));
  prefs.putString("trksym",server.arg("trksym"));
  prefs.putString("trkcomm",server.arg("trkcomm"));
  prefs.putUShort("trkmin",(uint16_t)server.arg("trkmin").toInt());
  prefs.putBool("trksmart",server.arg("trksmart")=="1");
  prefs.putUShort("trkslowv",(uint16_t)server.arg("trkslowv").toInt());
  prefs.putUShort("trkfastv",(uint16_t)server.arg("trkfastv").toInt());
  prefs.putUShort("trkslowt",(uint16_t)server.arg("trkslowt").toInt());
  prefs.putUShort("trkfastt",(uint16_t)server.arg("trkfastt").toInt());
  prefs.putBool("tstena",server.arg("tstena")=="1");
  prefs.putUChar("tstmode",(uint8_t)server.arg("tstmode").toInt());
  prefs.putUShort("tstmin",(uint16_t)server.arg("tstmin").toInt());
  prefs.putString("tsttext",server.arg("tsttext"));
  prefs.putBool("igategps",server.arg("igategps")=="1");
  prefs.putString("apkey",server.arg("apkey"));
  prefs.putString("w0s",server.arg("w0s"));
  prefs.putString("w0p",server.arg("w0p"));
  prefs.putString("w1s",server.arg("w1s"));
  prefs.putString("w1p",server.arg("w1p"));
  prefs.putString("w2s",server.arg("w2s"));
  prefs.putString("w2p",server.arg("w2p"));
  prefs.putString("w3s",server.arg("w3s"));
  prefs.putString("w3p",server.arg("w3p"));
  addEvent("DEV7 tam ayarlar yedegi geri yuklendi (admin parola korundu)");
  server.send(200,"text/plain; charset=utf-8","Tum ayarlar geri yuklendi. Cihaz yeniden baslatiliyor.");
  restartAt=millis()+1800;
}

void handleBackupPage(){
  if(!accessAllowed())return;
  String p=pageHeader("Yedek / Ayarlari Indir - Geri Yukle");
  p+="<div class='panel-grid'><section class='card'><h2>&#8681; Tum Ayarlari Yedekle</h2>";
  p+="<p class='subtle'>iGate, Digi, Tracker, SmartBeaconing, Status, BME280 telemetri, mesajlasma ve kopru, 5 Wi-Fi agi, OLED, profillerin kullandigi konum ve mod ayarlari dahil.</p>";
  p+="<button type='button' onclick='backupAll()'>Ayarlari Yedekle (.json)</button></section>";
  p+="<section class='card'><h2>&#8679; Tum Ayarlari Geri Yukle</h2>";
  p+="<input id='restoreFile' type='file' accept='.json,application/json'>";
  p+="<button type='button' onclick='restoreAll()'>Geri Yukle ve Yeniden Baslat</button>";
  p+="<p class='subtle'>Dosya alindiginda butun alanlar kontrol edilir. Onaydan sonra ayarlar kaydedilir ve cihaz yeniden baslatilir.</p></section></div>";
  p+="<div class='card'><h2>&#9888; Guvenlik</h2><p>Yedek JSON dosyasinda Wi-Fi, APRS-IS passcode ve kurulum AP sifresi <b>acik metin</b> bulunur. Dosyayi gizli saklayin. Yonetici sifresi/hash yedeklenmez ve geri yuklemede degismez.</p>";
  p+="<p>Eski V1.5.x dosyalari yeni tam yedek biciminde degildir; bu sayfaya dogrudan geri yuklenemez.</p></div>";
  p+=R"JS(<script>
async function backupAll(){
 try{const r=await fetch('/backup/export',{cache:'no-store'});if(!r.ok)throw Error(await r.text());
 const obj=await r.json();const b=new Blob([JSON.stringify(obj,null,2)],{type:'application/json'});
 const a=document.createElement('a');a.href=URL.createObjectURL(b);a.download='TA1FM_V2_TUM_AYARLAR.json';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);
 toast('Tum kayitli ayarlar indirildi. Sifreler acik metindir.');
 }catch(e){toast('Yedekleme hatasi: '+e.message,'error',9000)}
}
async function restoreAll(){
 try{
 const input=document.getElementById('restoreFile');const f=input.files[0];
 if(!f||f.size>24000)throw Error('Gecerli ve 24 KB altinda JSON secin');
 const obj=JSON.parse(await f.text());
 if(obj._format!=='TA1FM-V2.0-DEV7-settings'||obj._version!==1||typeof obj.settings!=='object'||!obj.settings)throw Error('Desteklenmeyen yedek surumu');
 if(!confirm('BUTUN ayarlar degistirilecek. Wi-Fi adresiniz ve AP sifreniz degisebilir. Devam edilsin mi?'))return;
 const params=new URLSearchParams();params.set('_format',obj._format);params.set('_version',String(obj._version));
 for(const [key,val] of Object.entries(obj.settings)){
   if(typeof val!=='string')throw Error('Gecersiz deger: '+key);
   params.set(key,val);
 }
 const h=await fetch('/health',{cache:'no-store'});if(!h.ok)throw Error('Baglanti hatasi');const old=(await h.json()).boot;
 const r=await fetch('/backup/import',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:params.toString(),cache:'no-store'});
 const msg=await r.text();if(!r.ok)throw Error(msg);toast(msg,'success',0);checkReboot(old);
 }catch(e){toast('Geri yukleme hatasi: '+e.message,'error',10000)}
}
</script>)JS";
  p+="</main></body></html>";
  server.sendHeader("Cache-Control","no-store");server.send(200,"text/html; charset=utf-8",p);
}

void handleSystem(){
  if(!accessAllowed())return;
  String p=pageHeader("Sistem / Yonetim");
  p+="<div class='sys-heading'>&#9881; Sistem <span class='sys-tag'>TA1FM iGate</span></div>";
  p+="<div class='panel-grid system-layout'><div class='system-col'>";
  p+="<section class='card'><h2>&#128274; Guvenlik</h2><p class='subtle'>Panel girisi ve kurulum AP sifresi.</p>";
  p+="<p class='subtle'>Panel kullanici adi: <b>admin</b></p>";
  p+="<form method='POST' action='/security/admin' onsubmit='return sysSecurityPost(event,this)'>";
  p+="<label>Mevcut yonetici sifresi</label><input type='password' autocomplete='current-password' name='oldpass' required>";
  p+="<label>Yeni yonetici sifresi</label><input type='password' autocomplete='new-password' name='newpass' minlength='10' maxlength='64' required>";
  p+="<label>Yeni sifre tekrar</label><input type='password' autocomplete='new-password' name='newpass2' minlength='10' maxlength='64' required>";
  p+="<button type='submit'>Panel Sifresini Degistir</button></form>";
  p+="<form method='POST' action='/security/ap' onsubmit='return sysSecurityPost(event,this)'>";
  p+="<label>Yeni Kurulum AP sifresi</label><input type='password' autocomplete='new-password' name='apkey' minlength='8' maxlength='63' required>";
  p+="<label>Yeni AP sifresi tekrar</label><input type='password' autocomplete='new-password' name='apkey2' minlength='8' maxlength='63' required>";
  p+="<button type='submit'>AP Sifresini Degistir</button></form></section>";
  p+="<section class='card'><h2>&#9211; Sistem Islemleri</h2><p class='subtle'>Yeniden baslatma ve yazilim yonetimi.</p>";
  p+="<button type='button' onclick='sysRestart()'>Cihazi Yeniden Baslat</button>";
  p+="<p class='subtle'>Guncelleme, tanilama ve yedekleme baglantilari Hızlı Erişim kartinda.</p></section>";
  p+="</div><div class='system-col'>";
  p+="<section class='card'><h2>&#9783; Profiller</h2><p class='subtle'>Sabit veya mobil iGate ayarlarini hizla uygula.</p>";
  p+="<form method='POST' action='/profiles/apply'><div class='system-choices'>";
  p+="<label><input type='radio' name='preset' value='fixed' "+String(!cfg.igateUseGps?"checked":"")+"><span><b>Sabit iGate</b><small>iGate sabit konumda, Tracker kapali</small></span></label>";
  p+="<label><input type='radio' name='preset' value='mobileis' "+String(cfg.igateUseGps && cfg.trackerMode==2?"checked":"")+"><span><b>Mobil iGate - APRS-IS</b><small>GPS konumu, Tracker internet uzerinden</small></span></label>";
  p+="<label><input type='radio' name='preset' value='mobileboth' "+String(cfg.igateUseGps && cfg.trackerMode==3?"checked":"")+"><span><b>Mobil iGate - LoRa + APRS-IS</b><small>GPS konumu, iki kanalda Tracker</small></span></label>";
  p+="</div><label><input type='checkbox' name='confirm' value='1' required> Profil degisikligini onayliyorum</label>";
  p+="<button type='submit'>Profili Uygula</button></form><small>Mesaj, BME280, Digi ve Wi-Fi ayarlari etkilenmez.</small></section>";
  p+="<section class='card'><h2>&#9889; Hizli Erisim</h2><p class='subtle'>Guncelleme, tanilama ve yedek islemlerine tek dokunusla ulasin.</p>";
  p+="<div class='quick-links'>";
  p+="<a class='primary full' href='/ota'>&#8679; OTA Firmware Guncelle</a>";
  p+="<a href='/gps'>&#9678; GPS Durumu</a>";
  p+="<a href='/rx'>&#8646; Son RX</a>";
  p+="<a href='/events'>&#9776; Olaylar</a>";
  p+="<a href='/backup'>&#128190; Yedek</a>";
  p+="</div></section>";
  p+="<section class='card'><h2>&#9635; Durum</h2><div class='system-status'>";
  p+="<div>Firmware<br><b>V2.0.0 DEV7</b></div>";
  p+="<div>Wi-Fi<br><b>"+String(WiFi.status()==WL_CONNECTED?htmlEscape(WiFi.SSID()):"Bagli degil")+"</b></div>";
  p+="<div>APRS-IS<br><b>"+String(aprsVerified?"Dogrulandi":"Bagli degil")+"</b></div>";
  p+="<div>Calisma suresi<br><b>"+String(millis()/60000UL)+" dk</b></div>";
  p+="</div><p class='subtle'>Tum ayarlari saklamak veya geri almak icin <a href='/backup'>Yedek</a> sekmesini kullanin.</p></section>";
  p+="</div></div>";
  p+=R"JS(<script>
async function sysSecurityPost(e,form){e.preventDefault();if(!form.reportValidity())return false;
 if(!confirm('Guvenlik ayarini degistirmek istiyor musunuz?'))return false;
 try{const r=await fetch(form.action,{method:'POST',body:new FormData(form),cache:'no-store'});const t=await r.text();toast(t,r.ok?'success':'error',9000);if(r.ok)form.reset();}
 catch(err){toast('Hata: '+err.message,'error',9000)}return false;
}
async function sysRestart(){
 if(!confirm('Cihaz yeniden baslatilacak. Emin misiniz?'))return;
 try{const h=await fetch('/health',{cache:'no-store'});const old=(await h.json()).boot;
 const r=await fetch('/restart',{method:'POST'});if(!r.ok)throw Error(await r.text());checkReboot(old);}
 catch(e){toast('Restart hatasi: '+e.message,'error',9000)}
}
</script>)JS";
  p+="</main></body></html>";
  server.sendHeader("Cache-Control","no-store");server.send(200,"text/html; charset=utf-8",p);
}

void setup(){
  Serial.begin(115200);delay(400);
  Serial.println("\n=== TA1FM LoRa APRS iGate V2.0.0 DEV7 ===");
  loadConfig();
  bootToken=esp_random();
  // RTC survives ESP.restart but not a real power-off. Limit startup TX in reboot loops.
  if(esp_reset_reason()==ESP_RST_POWERON)rapidBootCount=0;
  if(rapidBootCount<255)rapidBootCount++;
  if(rapidBootCount>=3){bootRfPending=false;Serial.println("[BOOT] Tekrarlayan restart: baslangic RF beacon engellendi");}
  addEvent("Basladi, reset nedeni="+String((int)esp_reset_reason()));
  pinMode(PIN_VEXT,OUTPUT); digitalWrite(PIN_VEXT,LOW);delay(100);
  // ADC'de sadece pil gerilimini okurken olcum bolucusunu etkinlestir.
  pinMode(ADC_CTRL_PIN,OUTPUT);
  digitalWrite(ADC_CTRL_PIN,!ADC_CTRL_ON_STATE);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN,ADC_11db);
  sampleBattery();
  beginDisplay();
  // Wire1 must exist even when OLED is disabled (BME280 uses same bus).
  gpsSerial.begin(GPS_BAUD,SERIAL_8N1,GPS_RX,GPS_TX);
  Serial.printf("[GPS] UART basladi RX=%d TX=%d BAUD=%lu\n",GPS_RX,GPS_TX,(unsigned long)GPS_BAUD);
  SPI.begin(PIN_SCK,PIN_MISO,PIN_MOSI,PIN_CS);
  int rc=radio.begin(LORA_FREQ,LORA_BW,LORA_SF,LORA_CR,LORA_SYNC,10,8,1.8);
  if(rc==RADIOLIB_ERR_NONE){
    // Heltec SX1262: use Ricardo's verified 140 mA OCP setting.
    // This protects against premature PA limiting at elevated TX power;
    // it does NOT guarantee over-the-air packet reception by another station.
    // Ricardo: output power first, then OCP; keep DIO2 antenna control enabled.
    int16_t power=radio.setOutputPower((int8_t)cfg.txPowerDbm);
    int16_t ocp=radio.setCurrentLimit(140.0f);
    int16_t sw=radio.setDio2AsRfSwitch();
    int16_t crc=radio.setCRC(2);
    Serial.printf("[LoRa] OCP: %d (%.1f mA), RF switch: %d, CRC: %d\n",
                  ocp, radio.getCurrentLimit(),sw,crc);
    Serial.printf("[LoRa] TX gucu ayari: %u dBm (sonuc %d)\n",cfg.txPowerDbm,power);
    if(ocp!=RADIOLIB_ERR_NONE || sw!=RADIOLIB_ERR_NONE || crc!=RADIOLIB_ERR_NONE || power!=RADIOLIB_ERR_NONE){
      Serial.printf("[LoRa] RF setup error OCP/SW/CRC/PWR: %d/%d/%d/%d; TX kapali\n",ocp,sw,crc,power);
      cfg.autoDigi=false; // Runtime safety; saved preference unchanged.
    }
    radio.setDio1Action(onRadioRx);
    rc=radio.startReceive();
    radioOk=(rc==RADIOLIB_ERR_NONE);
  }
  Serial.printf("[LoRa] %s (kod %d)\n",radioOk?"SX1262 HAZIR":"HATA",rc);
  beginWifi();
  // Asynchronous SNTP; normal RF receive does not wait for a time server.
  configTime(3*3600,0,"pool.ntp.org","time.google.com");
  aprsDownFrom=millis();
  server.on("/",HTTP_GET,handleHome);
  server.on("/settings",HTTP_GET,handleSettings);
  server.on("/system",HTTP_GET,handleSystem);
  server.on("/backup",HTTP_GET,handleBackupPage);
  server.on("/backup/export",HTTP_GET,handleBackupExport);
  server.on("/backup/import",HTTP_POST,handleBackupImport);
  server.on("/gps",HTTP_GET,handleGpsPage);
  server.on("/tracker",HTTP_GET,handleTrackerPage);
  server.on("/telemetry",HTTP_GET,handleTelemetry);
  server.on("/telemetry/save",HTTP_POST,handleTelemetrySave);
  server.on("/profiles",HTTP_GET,handleProfiles);
  server.on("/profiles/apply",HTTP_POST,handleProfileApply);
  server.on("/messages",HTTP_GET,handleMessages);
  server.on("/api/messages",HTTP_GET,handleMessagesApi);
  server.on("/messages/config",HTTP_POST,handleMessagesConfig);
  server.on("/messages/send",HTTP_POST,handleMessageSend);
  server.on("/wifi",HTTP_GET,handleWifiNetworks);
  server.on("/wifi/save",HTTP_POST,handleWifiNetworksSave);
  server.on("/tracker/save",HTTP_POST,handleTrackerSave);
  server.on("/igate/gps/save",HTTP_POST,handleIgateGpsSave);
  server.on("/api/gps",HTTP_GET,[](){if(!accessAllowed())return;server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",gpsJson());});
  server.on("/save",HTTP_POST,handleSave);
  server.on("/security",HTTP_GET,handleSecurity);
  server.on("/security/admin",HTTP_POST,handleAdminPasswordChange);
  server.on("/security/ap",HTTP_POST,handleApPasswordChange);
  server.on("/restart",HTTP_POST,handleRestart);
  server.on("/beacon/is",HTTP_POST,handleManualIs);
  server.on("/beacon/rf",HTTP_POST,handleManualRf);
  server.on("/health",HTTP_GET,[](){if(!accessAllowed())return;server.sendHeader("Cache-Control","no-store");server.send(200,"application/json","{\"boot\":"+String(bootToken)+",\"uptime\":"+String(millis()/1000)+"}");});
  server.on("/api/is-state",HTTP_GET,[](){if(!accessAllowed())return;server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",String("{\"enabled\":")+(cfg.aprsIsEnabled?"true":"false")+",\"verified\":"+(aprsVerified?"true":"false")+"}");});
  server.on("/api/toggle-is",HTTP_POST,handleIsToggle);
  server.on("/api/action-result",HTTP_GET,handleActionResult);
  server.on("/api/rx",HTTP_GET,handleRxJson);
  server.on("/rx",HTTP_GET,handleRxPage);
  server.on("/events",HTTP_GET,handleEvents);
  server.on("/ota",HTTP_GET,handleOtaPage);
  server.on("/ota",HTTP_POST,handleOtaFinish,handleOtaUpload);
  const char* collectedHeaders[]={"Authorization"};
  server.collectHeaders(collectedHeaders,1);
  server.onNotFound([](){server.send(404,"text/plain","Sayfa bulunamadi");});
  server.begin();
  screenUpdate();
}

void loop(){
  // Priority to radio packets.
  readRadio();
  pollGPS();
  maintainBme();
  maintainBattery();
  maintainWifi();
  server.handleClient();
  if(restartAt && (int32_t)(millis()-restartAt)>=0)ESP.restart();
  if(millis()>120000UL && rapidBootCount!=0)rapidBootCount=0;
  readRadio();
  pollGPS();
  maintainAprs();
  updateHybridMode();
  readRadio();
  manageDigiTx();
  manageRfBeacon();
  manageTrackerRf();
  manageTrackerStatusRf();
  manageTelemetryRf();
  manageMessagingRf();
  manageMessageBridgeRf();
  maintainDisplaySleep();
  if(millis()-lastScreen>=1000){lastScreen=millis();screenUpdate();}
  delay(1);
}