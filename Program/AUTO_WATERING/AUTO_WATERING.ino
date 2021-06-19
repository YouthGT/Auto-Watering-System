//#define BLINKER_WIFI_AUTO
#define BLINKER_PRINT Serial//串口输出
#define U8X8_USE_PINS

#define BLINKER_WIFI
#define BLINKER_ESP_SMARTCONFIG //使用smartconfig配网
#define BLINKER_MIOT_SENSOR //作为温度传感器接入小爱

#include <SPI.h>         //显示屏SPI传输库
#include <U8g2lib.h>     //显示屏的库
#include <Blinker.h>     //Blinker官方库
#define DHTPIN 15        //对应D8针脚接入DHT11
#define DHTTYPE DHT11    //选择传感器类型 DHT11
#include <DHT.h>         //温度传感器运行库 自用DHT11                    
DHT dht(DHTPIN, DHTTYPE);

uint32_t read_time = 0;
float temp_read,humi_read; //定义获取DHT11的数据
int sensorPin = A0;  //设置模拟口A0为信号输入端，土壤传感器 esp8266只有一个模拟输入
int sensorValue = 0; //初始化土壤模拟信号的变量
float precentsensorValue = 0; //初始化湿度变量
int waterPumpPin = D2; //继电器 水泵控制 
int sliderValue = 40;    //土壤湿度阈值 通过土壤传感器
int sliderValue1 = 20;   //空气温度阈值 通过气温传感器
bool manualFlag = false; //手动控制默认关闭

U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, D5 /*clock*/, D6 /*data*/, D7 /*cs*/, D3 /*dc*/, D4 /*reset*/);//定义OLED的引脚

BlinkerNumber NumSoil("num-soil"); //Blinke上自定义显示土壤湿度
BlinkerNumber Numtem("num-tem");  //Blinke上自定义显示空气温度
BlinkerNumber Numhum("num-hum");  //Blinke上自定义显示空气湿度

BlinkerButton BtnManual("btn-manual"); //手动浇花按键

BlinkerSlider Slider1("ran-1"); //设置滑块控制浇水土壤湿度阈值
BlinkerSlider Slider2("ran-2"); //设置滑块控制浇水空气温度阈值

BlinkerText Tex1("tex-1"); //文本1手动浇水状态
BlinkerText Tex2("tex-2"); //文本2自动浇水状态
BlinkerText Tex3("tex-3"); //文本3 时间信息

//char auth[] = "7d29d0baddb6";
//char type[] = "arduino";

char auth[] = "7d29d0baddb6"; //key
char ssid[] = "156";         //wifi name
char pswd[] = "651651651";    //password

void slider1_callback(int32_t value)//土壤湿度滑块
{
  sliderValue = value;
  BLINKER_LOG("get slider value: ", value);
}

void slider2_callback(int32_t value)//空气湿度滑块
{
  sliderValue1 = value;
  BLINKER_LOG("get slider2 value: ", value);
}

void heartbeat() //心跳包回调函数
{
  if (digitalRead(waterPumpPin) == HIGH)
    Tex2.print("暂停浇水");
  else if (digitalRead(waterPumpPin) == LOW)
    Tex2.print("正在浇水");
  Tex3.print(rts());
  Slider1.print(sliderValue);
  Slider2.print(sliderValue1);
  NumSoil.print(precentsensorValue);
  Numtem.print(temp_read);
  Numhum.print(humi_read);
}

void BtnManual_callback(const String &state) //手动浇花按键回调函数
{
  BLINKER_LOG("get button state: ", state);
  if (state == "on")
  {
    manualFlag = true;
    digitalWrite(waterPumpPin, LOW); //开启继电器
    Tex1.print("正在浇水");
    BtnManual.print("on");
  }
  if (state == "off")
  {
    manualFlag = false;
    digitalWrite(waterPumpPin, HIGH); //关掉继电器
    Tex1.print("暂停浇水");
    BtnManual.print("off");
  }
}
void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);

    Blinker.vibrate();
    
    uint32_t BlinkerTime = millis();
    
    Blinker.print("millis", BlinkerTime);
    
}
void dataStorage()//数据存储
{
    Blinker.dataStorage("cha-temp", temp_read);
    Blinker.dataStorage("cha-humi", humi_read);
    Blinker.dataStorage("cha-hum",precentsensorValue);
}
 
String rts()//计算运行时间
    {
    int rt = Blinker.runTime();
    int r,e,f,s;
    String fh;
    Blinker.delay(100);
    if(rt >= 86400)//天数
    {r = rt / 86400;
     e = rt / 3600 - r*24;
     f = rt / 60 - r*1440 - e*60;
     s = rt - r*86400 - e*3600 - f*60;}
    else if(rt >= 3600)
    {r = 0;
     e = rt / 3600;
     f = rt / 60 - e*60;
     s = rt - e*3600 - f*60;}
    else
    {r = 0;
     e = 0;
     f = rt / 60;
     s = rt - f*60;}
     
    //BLINKER_LOG(r," ",e," ",f," ",s);//输出数据测试
    
    if(f==0 & e==0 & r==0)
    {fh = String("")+ s +"秒";}
    else if(r == 0 & e == 0 )
    {fh = String("")+ f + "分" + s +"秒";}
    else if(r == 0)
    {fh = String("")+ e + "时" + f + "分" + s +"秒"; }
    else
    {fh = String("")+ r + "天" + e + "时" + f + "分" + s +"秒";}

    return(fh);
    }

void miotQuery(int32_t queryCode) //接入小爱模块


{
    BLINKER_LOG("MIOT Query codes: ", queryCode);

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_HUMI_NUMBER : //给小爱同学返回湿度数据
            BLINKER_LOG("MIOT Query HUMI");
            BlinkerMIOT.humi(humi_read); //湿度值
            BlinkerMIOT.print();
            break;
        case BLINKER_CMD_QUERY_TEMP_NUMBER : //给小爱同学返回温度数据
            BLINKER_LOG("MIOT Query TEMP");
            BlinkerMIOT.temp(temp_read);  //温度值
            BlinkerMIOT.print();
            break;
        default :                     //默认在传感器没有返回数值的时候播报的数值
            BlinkerMIOT.temp(0);
            BlinkerMIOT.humi(0);
            BlinkerMIOT.print();
            break;
    }
}
void oledDisplay()
{

  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_helvR10_te);
    u8g2.setCursor(0, 13);
    u8g2.print("AWS for YouthGT");
    u8g2.setCursor(0, 27);
    u8g2.print("soil humi: " + String(precentsensorValue) + "%");
    u8g2.setCursor(0, 41);
    if (digitalRead(waterPumpPin) == HIGH)
      u8g2.print("state: pausing");
    else if (digitalRead(waterPumpPin) == LOW)
      u8g2.print("state: flowering");

    u8g2.drawLine(0, 46, 128, 46);

    u8g2.setCursor(0, 63);
    u8g2.print("Time:" + String(Blinker.hour()) + ":" + String(Blinker.minute()));
    u8g2.setCursor(72, 63);
    u8g2.print("Thre:" + String(sliderValue));
  } while (u8g2.nextPage());
}

void u8g2Init()
{
  u8g2.begin();
  u8g2.setFlipMode(0);
  u8g2.clearBuffer();
  u8g2.enableUTF8Print();
}

void setup()
{
  Serial.begin(115200); //初始化串口
  BLINKER_DEBUG.stream(BLINKER_PRINT);

  // 初始化有LED的IO
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  //关闭指示灯
  pinMode(waterPumpPin, OUTPUT);    //灌溉水泵
  digitalWrite(waterPumpPin, HIGH); //关闭水泵

  u8g2Init(); //初始化OLED
  dht.begin();//初始化DHT11温度传感器

  Blinker.begin(auth); // 初始化blinker

  BlinkerMIOT.attachQuery(miotQuery);//初始化接入小爱查询端口

  Blinker.attachHeartbeat(heartbeat); //心跳包初始化

  Blinker.attachDataStorage(dataStorage);//附加数据存储


  Blinker.setTimezone(8.0); //获取NTP时间

  BtnManual.attach(BtnManual_callback); //初始化手动浇水按键

  Slider1.attach(slider1_callback);        //初始化土壤湿度阈值滑块
  Slider2.attach(slider2_callback);       //初始化空气温度阈值滑块
}
void loop()
{
  Blinker.run();
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  sensorValue = analogRead(sensorPin); //读取土壤湿度数据
  precentsensorValue = float((-(sensorValue - 866)) / 234.0 * 100);//计算土壤湿度
  //Serial.println(precentsensorValue);
  //Serial.println(manualFlag);
        humi_read = h;
        temp_read = t;

  
if (manualFlag = false)                  //手动控制标志位false的时，启动自动浇花
  {
    if ((precentsensorValue < sliderValue)&&(temp_read < sliderValue1)) //水不足并且温度不高于阈值 则补水
      digitalWrite(waterPumpPin, LOW);    //开启继电器
    else                                  //认为水多 不需要启动
      digitalWrite(waterPumpPin, HIGH);   //关掉继电器
  }

  oledDisplay();

  Blinker.delay(1200);//1.2S刷新一次
}
