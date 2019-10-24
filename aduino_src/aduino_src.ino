#include <Servo.h>                // 서보모터 헤더
#include <LiquidCrystal.h>        // LCD 헤더
#include <SoftwareSerial.h>       // Bluetooth 헤더
#include "Otto_sounds.h"          // 오토봇 사운드 헤더

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

// 기기 객체 생성 : 서보모터, Bluetooth, LCD
Servo servo;
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
unsigned long opTime, opTime_w, crTime, pTime; // 시각 처리 변수                    
bool firstOpen = false;                 // 처음 개봉 여부 변수
bool firstMoney = false;                // 처음 계좌 정보 입력 여부 변수
String strMoney = "";                   // 블루투스로 받은 문자열 금액
bool isOpen = false;                    // 개봉 여부 변수

// 오토봇 효과음 변수, 함수 ---------------------------------------------------------
int pinBuzzer = spk_pin;
int whatSong = 0;

void _tone (float noteFrequency, long noteDuration, int silentDuration){

  if(silentDuration==0)
    silentDuration=1;

  tone(pinBuzzer, noteFrequency, noteDuration);
  delay(noteDuration);
  delay(silentDuration);     
}

void bendTones (float initFrequency, float finalFrequency, float prop, long noteDuration, int silentDuration){

  //Examples:
  //  bendTones (880, 2093, 1.02, 18, 1);
  //  bendTones (note_A5, note_C7, 1.02, 18, 0);

  if(silentDuration==0){silentDuration=1;}

  if(initFrequency < finalFrequency)
  {
      for (int i=initFrequency; i<finalFrequency; i=i*prop) {
          _tone(i, noteDuration, silentDuration);
      }

  } else{

      for (int i=initFrequency; i>finalFrequency; i=i/prop) {
          _tone(i, noteDuration, silentDuration);
      }
  }
}
void sing(int songName){
  switch(songName){

    case S_connection:
      _tone(note_E5,50,30);
      _tone(note_E6,55,25);
      _tone(note_A6,60,10);
    break;

    case S_disconnection:
      _tone(note_E5,50,30);
      _tone(note_A6,55,25);
      _tone(note_E6,50,10);
    break;

    case S_buttonPushed:
      bendTones (note_E6, note_G6, 1.03, 20, 2);
      delay(30);
      bendTones (note_E6, note_D7, 1.04, 10, 2);
    break;

    case S_mode1:
      bendTones (note_E6, note_A6, 1.02, 30, 10);  //1318.51 to 1760
    break;

    case S_mode2:
      bendTones (note_G6, note_D7, 1.03, 30, 10);  //1567.98 to 2349.32
    break;

    case S_mode3:
      _tone(note_E6,50,100); //D6
      _tone(note_G6,50,80);  //E6
      _tone(note_D7,300,0);  //G6
    break;

    case S_surprise:
      bendTones(800, 2150, 1.02, 10, 1);
      bendTones(2149, 800, 1.03, 7, 1);
    break;

    case S_OhOoh:
      bendTones(880, 2000, 1.04, 8, 3); //A5 = 880
      delay(200);

      for (int i=880; i<2000; i=i*1.04) {
           _tone(note_B5,5,10);
      }
    break;

    case S_OhOoh2:
      bendTones(1880, 3000, 1.03, 8, 3);
      delay(200);

      for (int i=1880; i<3000; i=i*1.03) {
          _tone(note_C6,10,10);
      }
    break;

    case S_cuddly:
      bendTones(700, 900, 1.03, 16, 4);
      bendTones(899, 650, 1.01, 18, 7);
    break;

    case S_sleeping:
      bendTones(100, 500, 1.04, 10, 10);
      delay(500);
      bendTones(400, 100, 1.04, 10, 1);
    break;

    case S_happy:
      bendTones(1500, 2500, 1.05, 20, 8);
      bendTones(2499, 1500, 1.05, 25, 8);
    break;

    case S_superHappy:
      bendTones(2000, 6000, 1.05, 8, 3);
      delay(50);
      bendTones(5999, 2000, 1.05, 13, 2);
    break;

    case S_happy_short:
      bendTones(1500, 2000, 1.05, 15, 8);
      delay(100);
      bendTones(1900, 2500, 1.05, 10, 8);
    break;

    case S_sad:
      bendTones(880, 669, 1.02, 20, 200);
    break;

    case S_confused:
      bendTones(1000, 1700, 1.03, 8, 2); 
      bendTones(1699, 500, 1.04, 8, 3);
      bendTones(1000, 1700, 1.05, 9, 10);
    break;

    case S_fart1:
      bendTones(1600, 3000, 1.02, 2, 15);
    break;

    case S_fart2:
      bendTones(2000, 6000, 1.02, 2, 20);
    break;

    case S_fart3:
      bendTones(1600, 4000, 1.02, 2, 20);
      bendTones(4000, 3000, 1.02, 2, 20);
    break;

  }
}

int playSounds[] = {
  S_connection,
  S_disconnection,
  S_buttonPushed,
  S_mode1,
  S_mode2,
  S_mode3,
  S_surprise,
  S_OhOoh,
  S_OhOoh2,  
  S_cuddly,
  S_sleeping,
  S_happy,
  S_superHappy,
  S_happy_short,
  S_sad,
  S_confused,
  S_fart1,
  S_fart2,
  S_fart3,
};

//-------------------------------------------------------------------------------

void setup() {
  Serial.begin(boardrate);        // 시리얼 통신 속도 9600 설정
  bluetooth.begin(boardrate);     // 블루투스 통신 속도 9600 설정
  lcd.begin(lcd_w, lcd_h);        // lcd 가로 세로 설정
  servo.attach(mt_pin);           // 서보모터 핀 설정
  pinMode(opbtn_pin, INPUT);      // 잠금장치 스위치 버튼 핀 설정
  pinMode(clbtn_pin, INPUT);      // 카드 수납 여부 스위치 버튼 핀 설정
  pinMode(spk_pin, OUTPUT);       // 수동부저 핀 설정
  angle = 0;
  servo.write(angle);
  sing(playSounds[5]);
  pTime = millis();
}

void loop() {
  
// 처음 블루투스로 핸드폰을 통해 케이스 개봉
  if (firstOpen == false && bluetooth.available() > 0) {
    delay(50);
    
    angle = 90;
    servo.write(angle);
    
    isOpen = true;
    sing(playSounds[1]);      // 열 때 효과음
    
    while (bluetooth.available() > 0)
      bluetooth.read();
    
    firstOpen = true;

    opTime = millis();
    opTime_w = millis();
  }

// 처음 사용가능 금액 입력
  if (firstMoney == false && bluetooth.available() > 0) {
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

  if (chattering1 > 5) {
    pushBtn1 = true;
    chattering1 = 0;
  }
  else
    pushBtn1 = false;

  if (isOpen == false && pushBtn1 == true && money > 0 && writeMoney == true) {
    angle = 90;
    servo.write(angle);
      
    writeMoney = false;
    isOpen = true;
    sing(playSounds[1]);      // 열 때 효과음

    opTime = millis();
    opTime_w = millis();
  }
  else if (isOpen == false && pushBtn1 == true && money == 0 && writeMoney == true)  {
    crTime = millis();
    if (crTime-pTime > 5000) {
      if (whatSong == 0) {
        sing(playSounds[7]);
        whatSong++;
        pTime = crTime;
      }
      else if (whatSong == 1) {
        sing(playSounds[14]);
        whatSong++;
        pTime = crTime;
      }
      else {
        sing(playSounds[8]);
        whatSong = 0;
        pTime = crTime;
      }
    }
  }

  crTime = millis();                           // 현재 시각
  int t = crTime-opTime_w;                     // 열리고 지난 시간 저장(1/1000초 단위)  

// 열려있거나 지출내역 혹은 처음 사용가능 금액을 적지 않은 경고음이 10초마다 울림
  if ((isOpen == true || writeMoney == false || firstMoney == false) && t >= 12000 && firstOpen == true) {   
    sing(playSounds[11]);      // 경고음
    sing(playSounds[11]);      // 경고음
    opTime_w = crTime;           // 열리고 지난 시간 0초로 초기화
  }

// 수납 시 수납 버튼이 눌리면서 pushbtn2 = true 값 출력, 잠금 조치, chattering 구현
  if (digitalRead(clbtn_pin) == 1 && pushBtn2 == false)
    chattering2++;
  else
    chattering2 = 0;

  if (chattering2 > 5) {
    pushBtn2 = true;
    chattering2 = 0;
  }
  else
    pushBtn2 = false;

  if (pushBtn2 == true && crTime-opTime >= 5000) {
    angle = 0;
    servo.write(angle);
    
    lcd.setCursor(0,1);               // LCD 2째줄 초기화
    lcd.print("                ");

    isOpen = false;
    sing(playSounds[0]);      // 닫을 때 효과음
    opTime = millis();
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

// 지출 가능 금액이 0원이 되어 케이스가 잠겼을 시 앱을 통해 잔액 설정을 다시 해 열 수 있음
  if (money == 0 && bluetooth.available() > 0) {
    delay(50);
    
    strMoney = "";
    while (bluetooth.available() > 0) {
      char sMoney = bluetooth.read();
      strMoney += sMoney;
    }
    strMoney.trim();
    money = strMoney.toInt();
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
