#include <DS1302.h>               // DS1302 리얼타임클럭 헤더
#include <Servo.h>                // 서보모터 헤더
#include <LiquidCrystal.h>        // LCD 헤더
#include <SoftwareSerial.h>       // Bluetooth 헤더

// DS1302 핀번호
#define rtc_rst 8           
#define rtc_dat 9
#define rtc_clk 13

// Bluetooth 핀번호
#define bt_tx 7             
#define bt_rx 6

// LCD 핀번호
#define lcd_rs 12            
#define lcd_e A2
#define lcd_d4 5
#define lcd_d5 4
#define lcd_d6 3
#define lcd_d7 2

// 서보모터 핀번호
#define mt_pin 10

// LCD 세로, 가로 길이
#define lcd_h 2             
#define lcd_w 16

// 푸시버튼 핀번호
#define opbtn_pin A0        
#define clbtn_pin A1

// 수동부저 핀번호(pwm)
#define spk_pin 11    

// 통신속도
#define boardrate 9600

// 기기 객체 생성 : 서보모터, DS1302, Bluetooth, LCD
Servo servo;
DS1302 rtc(rtc_rst, rtc_dat, rtc_clk);
SoftwareSerial bluetooth(bt_tx, bt_rx);              
LiquidCrystal lcd(lcd_rs, lcd_e, lcd_d4, lcd_d5, lcd_d6, lcd_d7);

// 전역 변수 선언
String lcdStr = "";                     // Bluetooth 수신용 문자열 변수
String strTime = "";
unsigned long money = 0xfffffffe;       // 사용가능 금액 변수
int angle;                              // 서보모터 각 변수
bool writeMoney = false;                 // 계좌 작성 여부 변수
int opbtn, clbtn;                       // 잠금해제, 수납여부 버튼 변수
int chattering1 = 0;                    // 채터링용 변수(1.잠금해제용 2.수납여부용)
int chattering2 = 0;
bool pushBtn1 = false;                  // 잠금해제 버튼 푸시 여부
bool pushBtn2 = false;                  // 수납여부 버튼 푸시 여부
unsigned long opTime, opTime_w, crTime; // 케이스 연 시각 변수                     
bool firstOpen = false;                 // 처음 개봉 여부 변수
bool firstMoney = false;                // 처음 계좌 정보 입력 여부 변수
String strMoney = "";                   // 블루투스로 받은 문자열 금액
bool isOpen = false;                    // 개봉 여부 변수

void setup() {
  Serial.begin(boardrate);        // 시리얼 통신 속도 9600 설정
  bluetooth.begin(boardrate);     // 블루투스 통신 속도 9600 설정
  lcd.begin(lcd_w, lcd_h);        // lcd 가로 세로 설정
  servo.attach(mt_pin);           // 서보모터 핀 설정
  pinMode(opbtn_pin, INPUT);      // 잠금장치 스위치 버튼 핀 설정
  pinMode(clbtn_pin, INPUT);      // 카드 수납 여부 스위치 버튼 핀 설정
  pinMode(spk_pin, OUTPUT);       // 수동부저 핀 설정
  angle = 60;
  servo.write(angle);
  Serial.println("44");
  
// DS1302 초기 설정 : 처음 업로드 후 주석 처리 후 재업로드 ----------------

  rtc.halt(false);                // 동작 모드로 설정
  rtc.writeProtect(false);        // 시간 변경이 가능하도록 설정
  
  rtc.setDOW(3);                  // 수요일 설정
  rtc.setTime(23, 55, 10);        // 시간(시, 분, 초) 설정(24시간 형식)
  rtc.setDate(23, 10, 2019);      // 2019년 10월 23일로 설정

// ----------------------------------------------------------------

}

void loop() {
  
  if (firstOpen == false && bluetooth.available() > 0) {        // 처음 블루투스로 핸드폰을 통해 케이스 개봉
    delay(50);
    
    angle = 10;
    servo.write(angle);
    
    isOpen = true;
    
    while (bluetooth.available() > 0)
      bluetooth.read();
    
    firstOpen = true;

    opTime = millis();
    opTime_w = millis();
  }
  
  if (firstMoney == false && bluetooth.available() > 0) {  // 처음 사용가능 금액 입력
    delay(50);

    strMoney = "";
    while (bluetooth.available() > 0) {                         // 블루투스로 금액 문자열 수신
      char fMoney = bluetooth.read();
      strMoney += fMoney;
    }
    strMoney.trim();                                            // 공백 문자 제거
    money = strMoney.toInt();                                   // String 문자열을 long 타입의 범위에서 정수로 변환
    
    lcd.setCursor(0,1);                                         // 사용가능 금액 LCD로 출력 전 2번째 줄 초기화
    lcd.print("                ");
 
    firstMoney = true;
    writeMoney = true;
  }
  

// 잠금장치 해제 버튼 클릭 시 pushbtn1 = true 값 출력, 잠금 해제 기능, chattering 구현
  if (firstOpen == true && digitalRead(opbtn_pin) == 1 && pushBtn1 == false)
    chattering1++;
  else
    chattering1 = 0;

  if (chattering1 > 10) {
    pushBtn1 = true;
    chattering1 = 0;
  }
  else
    pushBtn1 = false;

  if (pushBtn1 == true && money > 0) {                   // 사용한도 금액이 0보다 클 경우 모터를 조정해 케이스를 열어줌
    angle = 10;
    servo.write(angle);
      
    writeMoney = false;

    opTime = millis();
    opTime_w = millis();

    Serial.println(opTime);          // 테스트
  }

  crTime = millis();                         // 현재 시각
  int t = crTime-opTime_w;                     // 열리고 지난 시간 저장(1/1000초 단위)  

// 열려있거나 지출내역 혹은 처음 사용가능 금액을 적지 않은 경고음이 10초마다 울림
  if ((isOpen == true || writeMoney == false || firstMoney == false) && t >= 10000 && firstOpen == true) {   
    tone(spk_pin, 500);
    delay(1000);
    noTone(spk_pin);      // ???
    opTime_w = crTime;           // 열리고 지난 시간 0초로 초기화
  }

// 수납 시 수납 버튼이 눌리면서 pushbtn2 = true 값 출력, 잠금 조치, chattering 구현
  if (digitalRead(clbtn_pin) == 1 && pushBtn2 == false)
    chattering2++;
  else
    chattering2 = 0;

  if (chattering2 > 10) {
    pushBtn2 = true;
    chattering2 = 0;
  }
  else
    pushBtn2 = false;

  if (pushBtn2 == true && crTime-opTime >= 5000) {
    angle = 60;
    servo.write(angle);
    
    lcd.setCursor(0,1);               // LCD 2째줄 초기화
    lcd.print("                ");

    isOpen = false;
  }

// 지출 내역 입력
  if (firstMoney == true && writeMoney == false && bluetooth.available() > 0) {
    delay(50);
    
    strMoney = "";
    while (bluetooth.available()) {
      char rMoney = bluetooth.read();
      strMoney += rMoney;
    }
    strMoney.trim();
    money = strMoney.toInt();
    
    lcd.setCursor(0,1);               // LCD 2째줄 초기화
    lcd.print("                ");
    
    writeMoney = true;
  }

// 사용 가능 금액 LCD 출력
  if (money == 0xfffffffe) {
    lcd.setCursor(0,0);
    lcd.print("Input your");
    lcd.setCursor(0,1);
    lcd.print("spendable money");
  }
  else {
    lcd.setCursor(0,0);
    lcd.print("Spendable money");
    lcd.setCursor(0,1);
    lcd.print(money);
  }
}
