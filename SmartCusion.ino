#include <ESP8266WiFi.h>

//巴法云服务器地址默认即可
#define TCP_SERVER_ADDR "bemfa.com"
//服务器端口//TCP创客云端口8344//TCP设备云端口8340
#define TCP_SERVER_PORT "8344"

/***********************传感器引脚设置************************/
const int HEATER = D0;
const int LM35 = A0;
const int PRESS = D1;
const int BUZZER = D2;
/***********************************************************/

/***********************WIFI及密钥配置************************/
String DEFAULT_STASSID = "CMCC-a7nn";
String DEFAULT_STAPSW = "r59kkj9f";
String UID = "1065cb19f6d3ced80e2d3a378f861f50";
String TOPIC = "SmartCusion";
String TOPIC2 = "SmartCusionController";
/***********************************************************/

unsigned long samplingTimeTick = 0;

unsigned long sitTimeTick = 0;

unsigned long maxSitTime = 30 * 60 * 1000;

unsigned int heaterStartTemp = 22;

int buzzerFreq = 1000;

int pressStatus = false;
//加热器状态状态
String heaterStatus = "off";
String heaterMode = "auto";

//led 控制函数
void turnOnHeater();
void turnOffHeater();

//设置上传速率2s（1s<=upDataTime<=60s）
#define upDataTime 5 * 1000

//最大字节数
#define MAX_PACKETSIZE 512

/***********************tcp客户端***************************/
WiFiClient TCPclient;
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;//心跳
unsigned long preTCPStartTick = 0;//连接
bool preTCPConnected = false;
/***********************************************************/

void doWiFiTick();
void startSTA();

void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);

float getTemperature() {
  float tempVal = (analogRead(LM35) * 4.88 / 10);
  //Serial.printf("temperature:%f C\n", tempVal);
  return tempVal;
}

void recvData() {
  if (TCPclient.available()) {//收数据
    char c =TCPclient.read();
    TcpClient_Buff += c;
    TcpClient_BuffIndex++;
    TcpClient_preTick = millis();
    
    if(TcpClient_BuffIndex >= MAX_PACKETSIZE - 1){
      TcpClient_BuffIndex = MAX_PACKETSIZE-2;
      TcpClient_preTick = TcpClient_preTick - 200;
    }
    preHeartTick = millis();
  }

  if((TcpClient_Buff.length() >= 1) && (millis() - TcpClient_preTick>=200)) {//data ready
    TCPclient.flush();
    Serial.println("Buff");
    Serial.println(TcpClient_Buff);

    if(TcpClient_Buff.indexOf("&msg=heater:on") >= 0) {   //如果检测到开灯指令
      turnOnHeater();
      heaterMode = "manual";
    }else if(TcpClient_Buff.indexOf("&msg=heater:off") >= 0) { //如果检测到关灯指令
      turnOffHeater();
      heaterMode = "manual";
    }else if(TcpClient_Buff.indexOf("&msg=heaterMode:auto") >= 0){
      heaterMode = "auto";
    }else if(TcpClient_Buff.indexOf("&msg=maxSitTime:") >= 0) {
      int start = TcpClient_Buff.lastIndexOf("&msg=maxSitTime:");
      Serial.println("startIndex");
      Serial.println(start);
      maxSitTime = TcpClient_Buff.substring(start + 1, start + 3).toInt() * 60 * 1000;
      Serial.println("maxSitTime");
      Serial.println(maxSitTime);
    }else if(TcpClient_Buff.indexOf("&msg=heaterStartTemp:") >= 0) {
      int start = TcpClient_Buff.lastIndexOf("&msg=heaterStartTemp:");
      heaterStartTemp = TcpClient_Buff.substring(start + 1, start + 3).toInt();
    }
    TcpClient_Buff="";//清空字符串，以便下次接收
    TcpClient_BuffIndex = 0;
  }

  
}

void uploadData() {
  if(millis() - preHeartTick >= upDataTime){
    preHeartTick = millis();
    float temperature = getTemperature();

    int sitTime = (millis() - sitTimeTick) / 1000;

    String upstr = "";
    upstr = "cmd=2&uid="+UID+"&topic="+TOPIC+"&msg="+temperature+"#"+sitTime+"#"+maxSitTime+"#"+heaterStatus+"#"+heaterMode+ "#" +heaterStartTemp+ "\r\n";
    sendtoTCPServer(upstr);
    upstr = "";
  }
}

/*
*发送数据到TCP服务器
*/
void sendtoTCPServer(String p){
  if (!TCPclient.connected()) {
    Serial.println("Client is not readly");
    return;
  }
  TCPclient.print(p);
  Serial.println("[Send to TCPServer]:");
  Serial.println(p);
}

/*
*初始化和服务器建立连接
*/
void startTCPClient(){
  if (TCPclient.connect(TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT))) {
    Serial.print("\nConnected to server:");
    Serial.printf("%s:%d\r\n",TCP_SERVER_ADDR,atoi(TCP_SERVER_PORT));
    String tcpTemp="";
    tcpTemp = "cmd=1&uid="+UID+"&topic="+TOPIC2+"\r\n";

    sendtoTCPServer(tcpTemp);
    preTCPConnected = true;
    preHeartTick = millis();
    TCPclient.setNoDelay(true);
  } else {
    Serial.print("Failed connected to server:");
    Serial.println(TCP_SERVER_ADDR);
    TCPclient.stop();
    preTCPConnected = false;
  }
  preTCPStartTick = millis();
}

/*
  *检查数据，发送数据
*/
void doTCPClientTick(){
 //检查是否断开，断开后重连
  if(WiFi.status() != WL_CONNECTED) return;

  if (!TCPclient.connected()) {//断开重连
    if (preTCPConnected == true) {
      preTCPConnected = false;
      preTCPStartTick = millis();
      Serial.println();
      Serial.println("TCP Client disconnected.");
      TCPclient.stop();
    } else if(millis() - preTCPStartTick > 1*1000) //重新连接
      startTCPClient();
  } else {
    recvData();

    uploadData();
  }
  
}


//打开灯泡
void turnOnHeater(){
  Serial.println("Turn on heater");
  digitalWrite(HEATER, LOW);
  heaterStatus = "on";
}

//关闭灯泡
void turnOffHeater(){
  Serial.println("Turn off heater");
  digitalWrite(HEATER, HIGH);
  heaterStatus = "off";
}

void startSTA(){
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(DEFAULT_STASSID, DEFAULT_STAPSW);
}

/*
  WiFiTick
  检查是否需要初始化WiFi
  检查WiFi是否连接上，若连接成功启动TCP Client
  控制指示灯
*/
void doWiFiTick(){
  static bool startSTAFlag = false;
  static bool taskStarted = false;
  static uint32_t lastWiFiCheckTick = 0;

  if (!startSTAFlag) {
    startSTAFlag = true;
    startSTA();
    Serial.printf("Heap size:%d\r\n", ESP.getFreeHeap());
  }

  //未连接1s重连
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiCheckTick > 1000) {
      lastWiFiCheckTick = millis();
    }
  } else {
    if (taskStarted == false) {
      taskStarted = true;
      Serial.print("\r\nGet IP Address: ");
      Serial.println(WiFi.localIP());
      startTCPClient();
    }
  }
}

// 初始化，相当于main 函数
void setup() {
  Serial.begin(115200);
  //初始化引脚为输出
  pinMode(LM35, INPUT);
  pinMode(PRESS, INPUT);  
  pinMode(BUZZER, OUTPUT);  
  pinMode(HEATER, OUTPUT); 
}

//循环
void loop() {
  doWiFiTick();
  doTCPClientTick();

  if (millis() - samplingTimeTick > 2000 && heaterMode == "auto") {
    float temp = getTemperature();
    if (temp < heaterStartTemp && heaterStatus == "off" ) {
      turnOnHeater();
    } else if (temp > heaterStartTemp + 2 && heaterStatus == "on") {
      turnOffHeater(); 
    }

    samplingTimeTick = millis();
  }
  

  bool isSetTime = false;
  if (pressStatus) {
    if (!isSetTime) {
      sitTimeTick = millis();
      isSetTime = true;
    }

    if (millis() - sitTimeTick > maxSitTime) {
      tone(BUZZER, buzzerFreq, 500);
    }
  } else {
    isSetTime = false;
  }
}
