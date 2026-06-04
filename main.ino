#define BLYNK_TEMPLATE_ID "TMPL6-8JF2omx"
#define BLYNK_TEMPLATE_NAME "Emdedding System"
#define BLYNK_AUTH_TOKEN "heHtrcU4EudnzvyRbtpJLWTbWZfomxVS"
#define SERVO_PIN 27
#define RAIN_SENSOR_PIN 33
#define UV_SENSOR_PIN 32
#define BUTTON_PIN 15
#define SDA_PIN_OF_RTC 22
#define SCL_PIN_OF_RTC 23
#define SDA_PIN_OF_GY30 5
#define SCL_PIN_OF_GY30 18
#define ADDRESS_OF_GY30 0x23
#define ADDRESS_OF_RTC 0x68
#define ANGLE_TO_CLOSE_DOOR 0
#define ANGLE_TO_OPEN_DOOR 160
// Đặt thời gian giữ nút để chuyển đổi mode là 10s, bật tắt hẹn giờ là 5s
const int HOLDING = 5000;
const int LONGHOLDING = 10000;
const int I2C_FREQUENCY = 40000;

#include "RTClib.h";
#include "ESP32Servo.h";
#include "BH1750.h";
#include "Wire.h";
#include "time.h";
#include "WiFi.h";
#include "WiFiClient.h";
#include "BlynkSimpleEsp32.h";
#include "soc/soc.h";
#include "soc/rtc_cntl_reg.h";

BH1750 GY30;
Servo myServo;
RTC_DS3231 rtc;
TwoWire I2C_one = TwoWire(0);
TwoWire I2C_two = TwoWire(1);
BlynkTimer timer;

int RAIN_INDEX_TO_CLOSE = 1500;
int UV_INDEX_TO_CLOSE = 6;
int LIGHT_INTENSITY_TO_CLOSE = 50000;

char ssid[] = "MSI 8491";
char pass[] = "12345678";
int hour_millis = 3600 * 1000;
int counter = millis();
int tmp = 0;
int hour_to_close = 19, minute_to_close = 0, hour_to_open = 7, minute_to_open = 0;
int mode = 0;
bool is_timer = false;
bool button_close_door = true;
bool is_close_door = true, network_status = false, getNTPTime = false;
// Lưu trạng thái nút bấm trước đó. Giá trị mặc định cho biến này là không bấm (false)
bool lastButtonState = false;
// Lưu thời gian nhấn nút để xác định xem là giữ nút hay là nhấn nút
int pressCounter = 0;
bool isConnectRTC = false, isConnectGY30 = false;


int read_rain_sensor(){
  int analog_data = analogRead(RAIN_SENSOR_PIN);

  return analog_data;
}

int read_light_sensor(){
  float light_index = GY30.readLightLevel(); 
  
  return light_index;
}

float read_uv_sensor(){
  int analogValue = analogRead(UV_SENSOR_PIN); 
  float sensorValue = (float) (5 * analogValue / 1024.0);
  float UV_index = sensorValue * 10;

  return UV_index;
}

int read_button_state(){
  int state = digitalRead(BUTTON_PIN);

  return state;
}

void open_door(){
  if(is_close_door){
    myServo.write(ANGLE_TO_OPEN_DOOR);
    Serial.println("Đang mở cửa");
    is_close_door = false;
    delay(1000);
  }
}

void close_door(){   
  if(!is_close_door){
    myServo.write(ANGLE_TO_CLOSE_DOOR);
    Serial.println("Đang đóng cửa");
    is_close_door = true;
    delay(1000);
  }
}

bool check_raining(){
  int n = read_rain_sensor();
  if (n <= RAIN_INDEX_TO_CLOSE){
    Serial.printf("It is raining: %d \n", n);
    return true;
  }
  return false;
}

bool check_uv(){
  float n = read_uv_sensor();
  if(n >= UV_INDEX_TO_CLOSE){
    Serial.printf("UV is high: %d \n", n);
    return true;
  }
  return false;
}

bool check_light_intensity(){
  int n = read_light_sensor();
  if(n >= LIGHT_INTENSITY_TO_CLOSE){
    Serial.printf("Light intensity is high: %d \n", n);
    return true;
  }
  return false;
}

bool check_time(){
  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();

  if(hour < hour_to_open || hour > hour_to_close){
    return true;
  }

  if(hour == hour_to_open && minute < minute_to_open) return true;

  if(hour == hour_to_close && minute >= minute_to_close) return true;  
  
  return false;
}

bool check_button(){
  if(read_button_state() == 1) return true;

  return false;
}

void resetRTCModule() {
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  getNTPTime = false;
  for(int i = 0; i <= 10; i++){
    if(now < 8 * 3600 * 2){
      delay(300);
      Serial.print(".");
      now = time(nullptr);
    } else {
      getNTPTime = true;
      break;
    }
  }
  DateTime timeVN(now + 7*3600);
  rtc.adjust(timeVN);
  if(getNTPTime == false){
    Serial.println("Không lấy được thời gian từ server, sử dụng thời gian ngẫu nhiên để đặt tạm cho module RTC");
    is_timer = false;
  } else {
    Serial.printf("Thời gian từ server: %d:%d:%d \n", rtc.now().hour(), rtc.now().minute(), rtc.now().second());
  }

  Serial.println();
  Serial.println("End Reset RTC Module");
}

BLYNK_WRITE(V4){
  int pinValue = param.asInt();
  button_close_door = pinValue;
}

BLYNK_WRITE(V5){
  int pinValue = param.asInt();
  hour_to_close = pinValue;
}

BLYNK_WRITE(V6){
  int pinValue = param.asInt();
  minute_to_close = pinValue;
}

BLYNK_WRITE(V8){
  int pinValue = param.asInt();
  hour_to_open = pinValue;
}

BLYNK_WRITE(V9){
  int pinValue = param.asInt();
  minute_to_open = pinValue;
}

BLYNK_WRITE(V11){
  int pinValue = param.asInt();
  is_timer = pinValue;
}

BLYNK_WRITE(V12){
  int pinValue = param.asInt();
  Serial.print("Đặt lại chế độ cho hệ thống: ");
  mode = pinValue;
}

BLYNK_WRITE(V14){
  int pinValue = param.asInt();

  UV_INDEX_TO_CLOSE = pinValue;
}

BLYNK_WRITE(V15){
  int pinValue = param.asInt();

  RAIN_INDEX_TO_CLOSE = 1400 + 100*pinValue;
}

void myTimerEvent(){
  Blynk.virtualWrite(V1, check_raining());
  Blynk.virtualWrite(V2, read_light_sensor());
  Blynk.virtualWrite(V3, read_uv_sensor());
  Blynk.virtualWrite(V7, is_close_door);
  Blynk.virtualWrite(V10, rtc.now().hour());
  Blynk.virtualWrite(V13, rtc.now().minute());

  Blynk.virtualWrite(V11, is_timer);
  Blynk.virtualWrite(V12, mode);
  Blynk.virtualWrite(V8, hour_to_open);
  Blynk.virtualWrite(V9, minute_to_open);
  Blynk.virtualWrite(V5, hour_to_close);
  Blynk.virtualWrite(V6, minute_to_close);
  Blynk.virtualWrite(V4, button_close_door);

  Blynk.virtualWrite(V14, UV_INDEX_TO_CLOSE);
  Blynk.virtualWrite(V15, (int) (RAIN_INDEX_TO_CLOSE - 1400)/100);
}

void setupWifi(){
  WiFi.begin(ssid, pass);

  for(int i = 0; i <= 10; i++){
    if (WiFi.status() != WL_CONNECTED) {
      delay(200); Serial.print(".");
      network_status = false;
    } else {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      network_status = true;
      break;
    }
  }
}

void checkServo(){
  Serial.println("Start check servo");
  for(int i = 0; i <= 180; i+=30){
    myServo.write(i);
    delay(1000);
  }
  Serial.println("End check servo");
}

void setup() {
  delay(2000);
  Serial.begin(9600);
  
  // Tắt chế độ phát hiện nguồn điện thấp
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.println("Start setup");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  myServo.attach(SERVO_PIN);
  checkServo();
  I2C_one.begin(SDA_PIN_OF_RTC, SCL_PIN_OF_RTC, I2C_FREQUENCY);
  I2C_two.begin(SDA_PIN_OF_GY30, SCL_PIN_OF_GY30, I2C_FREQUENCY);
  Serial.println("Done I2C one and two, servo");

  // Kết nối và đồng thời cập nhật trạng thái biến kiểm tra;
  isConnectRTC = rtc.begin(&I2C_one);

  // Kết nối GY30 và đồng thời cập nhật trạng thái biến kiểm tra
  isConnectGY30 = GY30.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &I2C_two);

  setupWifi();

  Blynk.config(BLYNK_AUTH_TOKEN);
  if(WiFi.status() == WL_CONNECTED){
    resetRTCModule();
    Blynk.connect(5000);
  }
  // Đặt khoảng thời gian gửi dữ liệu lên blynk là 1s / 1 lần gửi
  timer.setInterval(1000L, myTimerEvent);
}

void loop() {

  // Kiểm tra xem còn kết nối GY30 hay không, nếu còn mà trạng thái chưa kết nối thì cập nhật lại, nếu không còn thì cập nhật trạng thái thành false
  if(GY30.configure(BH1750::CONTINUOUS_HIGH_RES_MODE) == 1){
    if(isConnectGY30 == false){
      isConnectGY30 = GY30.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, ADDRESS_OF_GY30, &I2C_two);
    }
  } else {
    isConnectGY30 = false;
  }

  // Kiểm tra kết nối rtc
  isConnectRTC = rtc.begin(&I2C_one);

  // Kiểm tra trạng thái nút bấm
  // Nếu trạng thái trước là nhả nút và trạng thái hiện tại là nhấn nút thì bắt đầu counter
  // Nếu trạng thái trước là nhấn nút và trạng thái hiện tại là nhả nút thì kết thúc counter và kiểm tra thời gian nhấn
  if(check_button() == true && lastButtonState == false){
    Serial.println("Press button");
    pressCounter = millis();
    lastButtonState = true;
  } else if(check_button() == false && lastButtonState == true){
    Serial.println("Release button");
    lastButtonState = false;
    pressCounter = millis() - pressCounter;
    // Kiểm tra thời gian giữ nút, nếu là giữ 10s thì đổi mode, nếu là giữ 5s thì bật/tắt timer, còn nếu là nhấn đóng cửa thì chuyển đổi trạng thái nút đóng cửa
    // Muốn cửa đóng/mở khi nhấn thì phải chuyển sang mode thủ công. Ở chế độ tự động nút nhấn bị vô hiệu hóa.
    if(pressCounter >= HOLDING && pressCounter < LONGHOLDING){
      // Chuyển đổi mode
      is_timer = !is_timer;
      Serial.print("Change timer: ");
      Serial.println(is_timer);
    } else if (pressCounter >= LONGHOLDING){
      mode = 1 - mode;
      Serial.print("Change mode: ");
      if(mode == 1){
        Serial.println("Thủ công");
      } else {
        Serial.println("Tự động");
      }
    } else {
      button_close_door = !button_close_door;
      Serial.print("Chang button close door: ");
      Serial.println(button_close_door);
    }
  }

  // Mode tự động
  if(mode == 0){
    if(check_uv() || (isConnectGY30 && check_light_intensity()) || check_raining()){
      Serial.println("Thời tiết không an toàn");
      close_door();
    } else {
      // Nếu không kết nối được rtc thì đóng cửa - bảo đảm an ninh
      // Nếu có thì kiểm tra
      if(is_timer == true){
        if(isConnectRTC == true){
          if(check_time()){
            Serial.println("Chế độ hẹn giờ bật. Ngoài thời gian mở cửa");
            close_door();
          } else {
            Serial.println("Chế độ hẹn giờ bật. Trong thời gian mở cửa");
            open_door();
          }
        } else {
          Serial.println("Thời tiết an toàn nhưng RTC không có kết nối");
        }
      } else {
        Serial.println("Chế độ hẹn giờ tắt. Thời tiết an toàn");
        open_door();
      }
    }
  // Mode thủ công
  } else if(mode == 1){
    if(button_close_door == true){
      close_door();
    } else {
      open_door();
    }
  }

  // Nếu có kết nối wifi thì chạy blynk
  // Đồng thời kiểm tra xem có phải là có kết nối lại hay không, nếu là kết nối lại thì thục hiện thủ tục kết nối lại
  // Nếu kết nối 
  counter = millis() - tmp;
  if(WiFi.status() == WL_CONNECTED){
    if(network_status == false){
      network_status = true;
      timer.run();
      Blynk.connect(5000);
    }

    if(counter >= hour_millis){
      counter = 0; tmp = millis();
      resetRTCModule();
    }

    if(getNTPTime == false){
      resetRTCModule();
    }

    timer.run();
    Blynk.run(); 
  } else {
    if(network_status == true)  network_status = false;

    // Thử kết nối lại wifi
    setupWifi();
  }
}

