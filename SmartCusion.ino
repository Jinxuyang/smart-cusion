#include <SoftwareSerial.h>


/***********************WIFI及密钥配置************************/
String ssid ="CMCC-a7nn";       //WIFI名称，区分大小写，不支持5G
String password="r59kkj9f";  //WIFI密码，区分大小写
String uid ="1065cb19f6d3ced80e2d3a378f861f50";    //用户私钥，巴法云控制台获取
String myTopic ="SmartCusion";                      //用户主题，巴法云控制台创建
/***********************************************************/

/***********************传感器引脚设置************************/
const int HEATER = 2;
const int LM35 = A1;
const int PRESS = 123;
const int BUZZER = 123;
/***********************************************************/

/***********************esp-01设置************************/
SoftwareSerial mySerial(13, 12); // RX, TX  //初始化软串口,pin13接8266 TX，Pin12接8266 RX
#define KEEPALIVEATIME 30*1000     //心跳间隔，默认30秒发一次心跳
unsigned long preHeartTick = 0; //心跳时间
#define TIMEOUT 5000 //接收esp8266反馈的超时时间
String tcpLocalPort="";
/***********************************************************/

unsigned long sitTimeTick = 0;

unsigned long maxSitTime = 30 * 60 * 1000;

unsigned int heaterStartTemp = 22;

int buzzerFreq = 1000;

int pressStatus = false;


/*
 * 打开加热器函数
 */
void turnOnHeater(){
  Serial.println("Turn on heater");  //打印调试信息
  digitalWrite(HEATER,LOW); //改变引脚状态
}

/*
 * 关闭加热器函数
 */
void turnOffHeater(){
  Serial.println("Turn off heater");//打印调试信息
  digitalWrite(HEATER,HIGH);//改变引脚状态
}

/*
 * 获取当前温度
 */
float getTemp() {
  float tempVal = (analogRead(LM35) * 4.88 / 10);
  Serial.print("Temperature = ");
  Serial.print(tempVal);
  Serial.println(" Degree Celsius");
  return tempVal;
}

/*
 * 发送当前温度
 */
void sendTemp() {
  float temp = getTemp();
  mySerial.println("cmd=2&uid=" +uid+ "&topic=" +myTopic+ "&msg=temp:" +temp+ "\r\n");
  Serial.println("Send temp success");
}

void sendStartSitTime(unsigned long startSitTime) {
  mySerial.println("cmd=2&uid=" +uid+ "&topic=" +myTopic+ "&msg=startSitTime:" +startSitTime+ "\r\n");
  Serial.println("Send startSitTime success");
}

void sendEndSitTime(unsigned long endSitTime) {
  mySerial.println("cmd=2&uid=" +uid+ "&topic=" +myTopic+ "&msg=endSitTime:" +endSitTime+ "\r\n");
  Serial.println("Send endSitTime success");
}

/*
 * 重启函数，执行重启
 */
void(* resetFunc) (void) = 0;//重启函数

void subscribeServer () {
  SendCommand("AT+CIPSTART=\"TCP\",\"bemfa.com\",8344","OK");  // 连接巴法创客云服务器，IP: bemfa.com,端口：8344
  delay(200);
  Serial.println("Subscribe topic");//打印调试信息
  mySerial.println("AT+CIPSEND");  //进入透传模式，下面发的都会无条件传输
  delay(1000);
  mySerial.println("cmd=1&uid="+uid+"&topic="+myTopic+"\r\n");  //发送订阅指令
}

/*
 * 检查收到的信息 
 * 字符串匹配，匹配到开灯指令，进行开灯，匹配到关灯指令，进行关灯
 * 匹配到错误信息，进行重启启动(一般为网络情况的故障)
 */
void check_msg(String msg){
  if(msg.indexOf("&msg=heater:on") >= 0) {   //如果检测到开灯指令
    turnOnHeater();
  }else if(msg.indexOf("&msg=heater:off") >= 0) { //如果检测到关灯指令
    turnOffHeater();
  }else if(msg.indexOf("&msg=maxSitTime:") >= 0) {
    int start = msg.lastIndexOf("&msg=maxSitTime:");
    maxSitTime = msg.substring(start + 1, start + 3).toInt() * 60 * 1000;
  }else if(msg.indexOf("&msg=heaterStartTemp:") >= 0) {
    int start = msg.lastIndexOf("&msg=heaterStartTemp:");
    heaterStartTemp = msg.substring(start + 1, start + 3).toInt();
  }else if(msg.indexOf("CLOSED") >= 0){  //检测到断开服务器连接，重新连接
    subscribeServer(); //发送订阅指令
  }else if(msg.indexOf("ERROR") >= 0 || msg.indexOf("busy") >= 0){  //检测到错误反馈或者网络繁忙，重启arduino
    Serial.println("beginning restart");
    Serial.println(msg);
    resetFunc(); //重启函数，执行重启
  }
}

/*
 * 
 * 系统初始化
 * 初始化串口，并初始化WIFI
 */
void setup() {
  Serial.begin(9600);  //arduino波特率，仅用于查看调试信息，连接串口调试助手可以查看
  mySerial.begin(9600);  //设置软串口波特率，建议9600，如果波特率过大，接收的指令存在乱码的情况（115200实测会乱码）
  
  Serial.println("begin init");  //打印调试信息
  SendCommand("AT+RST","ready");  //重启WIFI模块
  delay(2000);
  SendCommand("AT+CWMODE=3","OK"); //设置路由器模式 1 station模式 2 AP路由器模式 3 station+AP混合模式
  SendCommand("AT+CWJAP=\"" +ssid+"\",\"" + password + "\"","OK"); //设置模块WIFI名称和WIFI密码
  delay(5000);
  SendCommand("AT+CIPMODE=1","OK");   //开启透明传输模式
  delay(1000);
  subscribeServer();

  pinMode(PRESS, INPUT);  
  pinMode(BUZZER, OUTPUT);  
  pinMode(HEATER, OUTPUT);  
}
 
void loop(){
  String IncomingString="";  //用于接收串口发来的数据
  boolean StringReady = false;  //接收到串口数据的标志
 
  while (mySerial.available()){//如果接收到esp8266的数据
    IncomingString=mySerial.readString(); //获取esp8266反馈的数据，及esp8266收到远程服务器发来的数据
    StringReady= true;
  }
  
  if (StringReady){ //如果有数据发来，检查接收到的数据
    Serial.println("Received String: " + IncomingString);  //串口打印显示收到的数据
    check_msg(IncomingString);  //调用检查数据函数，进行检查数据
  }

  if(millis() - preHeartTick >= KEEPALIVEATIME){//定时函数，用于保持心跳，30秒检测一次（现在时间减去上次时间是否大于或等于30s）
      preHeartTick = millis(); //获取现在时间，用于下次计算
      mySerial.print("+++");  //退出透传模式，以便检测连接
      delay(200);

      int connected_status = SendCommand("AT+CIPSTATUS","8344");  //检测是否订阅成功
      if(connected_status == true){  //未成功订阅
           subscribeServer();
      }else{  //如果没有断开连接，发送心跳指令
                mySerial.println("AT+CIPSEND"); //进入透传模式，下面发的都会无条件传输
                delay(200);
                Serial.println("--send ping");//打印调试信息
                mySerial.println("cmd=0&msg=keep\r\n");  //发送心跳
      }
  }

  if (getTemp() < heaterStartTemp) {
    digitalWrite(HEATER, LOW);
  } else if (getTemp() > heaterStartTemp + 2) {
    digitalWrite(HEATER, HIGH);  
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

/*
 * 发送AT指令函数
 * ****************************
 * cmd为需要发送的指令
 * ack是期待收到的结果
 * ****************************
 */
boolean SendCommand(String cmd, String ack){
  mySerial.println(cmd); // Send "AT+" command to module
  if (!echoFind(ack)) { // 如果超时或者错误响应
    return true; // 返回真
  } else {
    return false;
  }
}
/*
 * 检查收到的字符串
 * 配合SendCommand函数使用
 */
boolean echoFind(String keyword) {
 long deadline = millis() + TIMEOUT;
 while(millis() < deadline){
  if (mySerial.available()){
    String get_msg = mySerial.readString();
    Serial.println(get_msg);
     check_msg(get_msg);  //检查数据
     int keyword_index = get_msg.indexOf(keyword);//获取关键字所在字符串位置
     if(keyword_index >=0){//如果接收到的字符串有期待接收到的关键字
      if(keyword == "8344"){ //判断端口是否变化，如果变化，重新订阅
        if(tcpLocalPort ==""){ //如果本地端口为空
          tcpLocalPort = get_msg.substring(keyword_index+5,keyword_index+10); //获取本地tcp端口
        }else{
          if(tcpLocalPort != get_msg.substring(keyword_index+5,keyword_index+10)){ //如果本地端口变化
              tcpLocalPort = get_msg.substring(keyword_index+5,keyword_index+10);//获取本地tcp端口
              return false; //返回错误
            }
        }
      }
        return true;  //返回真
      }
   }
  }
 return false; // Timed out
}