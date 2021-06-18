/* OLED
      arduino       OLED
      D5      >     MOSI
      D7      >     SCK
      D2      >     DC
      D8      >     CS 
      D3      >     RES
*/

//#define BLINKER_WIFI_AUTO
#define BLINKER_PRINT Serial
#define U8X8_USE_PINS

#define BLINKER_WIFI
#define BLINKER_ESP_SMARTCONFIG //使用smartconfig配网

#include <SPI.h>
#include <U8g2lib.h>
#include <Blinker.h>

int sensorPin = A0;  //设置模拟口A0为信号输入端，土壤传感器 esp8266只有一个模拟输入
int sensorValue = 0; //存放土壤模拟信号的变量
float precentsensorValue = 0;
int waterPumpPin = D4; //继电器 水泵控制 
int sliderValue = 40;
bool manualFlag = false;

U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, D5 /*clock*/, D6 /*data*/, D7 /*cs*/, D3 /*dc*/, D4 /*reset*/);

BlinkerNumber NumSoil("num-soil"); //土壤湿度的值

BlinkerButton BtnManual("btn-manual"); //手动浇花按键

BlinkerSlider Slider1("ran-1"); //浇花土壤湿度阈值

BlinkerText Tex1("tex-1"); //文本1
BlinkerText Tex2("tex-2"); //文本2

//char auth[] = "7d29d0baddb6";
//char type[] = "arduino";

char auth[] = "7d29d0baddb6"; //key
char ssid[] = "156";         //wifi name
char pswd[] = "651651651";    //password

void slider1_callback(int32_t value)
{
  sliderValue = value;
  BLINKER_LOG("get slider value: ", value);
}

void heartbeat() //心跳包回调函数
{
  if (digitalRead(waterPumpPin) == HIGH)
    Tex2.print("pausing");
  else if (digitalRead(waterPumpPin) == LOW)
    Tex2.print("flowering");

  Slider1.print(sliderValue);
  NumSoil.print(precentsensorValue);
}

void BtnManual_callback(const String &state) //手动浇花按键回调函数
{
  BLINKER_LOG("get button state: ", state);
  if (state == "on")
  {
    manualFlag = true;
    digitalWrite(waterPumpPin, LOW); //开启继电器
    Tex1.print("flowering");
    BtnManual.print("on");
  }
  if (state == "off")
  {
    manualFlag = false;
    digitalWrite(waterPumpPin, HIGH); //关掉继电器
    Tex1.print("pausing");
    BtnManual.print("off");
  }
}

void oledDisplay()
{

  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_helvR10_te);
    u8g2.setCursor(0, 13);
    u8g2.print("AFCS for YouthGT");
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

  //Blinker.begin(auth, type);
  //Blinker.begin(auth, ssid, pswd);
  Blinker.begin(auth); // 初始化blinker

  Blinker.attachHeartbeat(heartbeat); //心跳包初始化

  Blinker.setTimezone(8.0); //获取NTP时间

  BtnManual.attach(BtnManual_callback); //初始化按键

  Slider1.attach(slider1_callback);
}

void loop()
{

  Blinker.run();

  sensorValue = analogRead(sensorPin); //读取数据
  precentsensorValue = float((-(sensorValue - 866)) / 234.0 * 100);
  //Serial.println(precentsensorValue);
  //Serial.println(manualFlag);

  if (manualFlag != true) //手动控制标志位false的时候才会默认为自动
  {
    if (precentsensorValue < sliderValue) //认为水少 需要补水
      digitalWrite(waterPumpPin, LOW);    //开启继电器
    else                                  //认为水多 不需要启动
      digitalWrite(waterPumpPin, HIGH);   //关掉继电器
  }

  oledDisplay();

  Blinker.delay(1200);
}
