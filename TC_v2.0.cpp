// Termocontrol v2.0
// Two sensors

#include <OneWire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM2.h>
#include <Bounce2.h>

#define KEY_UP 4
#define KEY_DN 5
#define KEY_OK 3
#define DS_PIN 2
#define R1_PIN 13
#define R2_PIN 12
#define R3_PIN 11

// Create objects

LiquidCrystal_I2C lcd(0x27,16,2);
OneWire ds(DS_PIN);
Bounce keyUP = Bounce();
Bounce keyDN = Bounce();
Bounce keyOK = Bounce();

byte water_sens[8] = {0x28,0xFF,0x53,0x81,0xA4,0x15,0x03,0x5C};
byte room_sens[8] = {0x28,0xFF,0x8F,0xDA,0xA4,0x15,0x03,0x6F};


float target_water_temp = 60.0;
float current_water_temp = 0;
float target_room_temp = 23.0;
float current_room_temp = 0;
float water_gisterezis = 5.0;
float room_gisterezis = 5.0;
uint8_t heater_power[] = {0,1,2};
char* power_name[] = {"Low","Med","High"};
uint8_t currentPower = 0;
boolean disp_scrl = 0;
uint8_t mode = 0;
uint8_t sp = 2;

unsigned long SENS_prevMillis = 0;        // Многозадачность
const long SENS_interval = 5000;
unsigned long DISP_SCRL_prevMillis = 0;       
const long DISP_SCRL_interval = 6000;

float GetTemp(byte *sensor){
  byte data[12];
  ds.reset();
  ds.select(sensor);
  ds.write(0x44, 1);
  delay(50); 
  ds.reset();
  ds.select(sensor);
  ds.write(0xBE);

  for (byte i = 0; i < 10; i++){ 
    data[i] = ds.read();
  }
  int tmp=(data[1] << 8) | data[0];
  float temp =  (float)tmp / 16.0;
  return temp;
}

void disp(boolean state){
  if(!state){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Water temp:");
    lcd.print(current_water_temp,1);
    lcd.print("C");
    lcd.setCursor(0,1);
    lcd.print("Room temp:");
    lcd.print(current_room_temp,1);
    lcd.print("C");
  }
  else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Target WT:");
    lcd.print(target_water_temp,1);
    lcd.print("C");
    lcd.setCursor(0,1);
    lcd.print("Power: ");
    lcd.print(power_name[currentPower]);
  }  
}

void menu(){
  if(mode==0) disp(disp_scrl);
  if(mode==1) ChangeWaterTarget();
  if(mode==2) ChangeWaterHysteresis();
  if(mode==3) ChangeRoomTarget();
  if(mode==4) ChangeRoomHysteresis();
  if(mode==5) changePower(); 
  if(mode==6) saveParam(); 
}

void ChangeWaterTarget(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Water target temp C:");
  lcd.setCursor(0,1);
  lcd.print(target_water_temp,1);
}

void ChangeWaterHysteresis(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Water hysteresis C:");
  lcd.setCursor(0,1);
  lcd.print(water_gisterezis,1); 
}

void ChangeRoomTarget(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Room target temp C:");
  lcd.setCursor(0,1);
  lcd.print(target_room_temp,1);
}

void ChangeRoomHysteresis(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Room hysteresis C:");
  lcd.setCursor(0,1);
  lcd.print(room_gisterezis,1); 
}

void changePower(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Power:");
  lcd.setCursor(0,1);
  lcd.print(power_name[currentPower]); 
}

void saveParam(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Save param?");
  lcd.setCursor(0,1);
  lcd.print("Up-Yes  Down-No");
  switch(sp){
    case 0:
    mode=0;
    sp=2;
    break;
    case 1:
    EEPROM_write(8,target_water_temp);
    delay(5);
    EEPROM_write(16,water_gisterezis);
    delay(5);
    EEPROM_write(24,target_room_temp);
    delay(5);
    EEPROM_write(32,room_gisterezis);
    delay(5);
    EEPROM_write(2,currentPower);
    delay(5);
    lcd.setCursor(0,0);
    lcd.print("Param saved");
    delay(2000);
    mode=0;
    sp=2;
    break;
    case 2:
    mode=6;
    sp=2;
    break;
  }
}


void relay(uint8_t power_mode){
  float minTarg = target_water_temp - water_gisterezis;
  float minRoomTarg = target_room_temp - room_gisterezis;

  switch(power_mode){
    case 0:
    if(current_water_temp >= target_water_temp && current_room_temp >= target_room_temp) digitalWrite(R1_PIN,HIGH);
    if(current_water_temp <= minTarg) digitalWrite(R1_PIN,LOW);
    break;

    case 1:
    if(current_water_temp >= target_water_temp && current_room_temp >= target_room_temp){
      digitalWrite(R1_PIN,HIGH);
      digitalWrite(R2_PIN,HIGH);
    }
    else if(current_water_temp <= minTarg){
      digitalWrite(R1_PIN,LOW);
      digitalWrite(R2_PIN,LOW);
    }
    break;

    case 2:
    if(current_water_temp >= target_water_temp && current_room_temp >= target_room_temp){
      digitalWrite(R1_PIN,HIGH);
      digitalWrite(R2_PIN,HIGH);
      digitalWrite(R3_PIN,HIGH);
    }
    else if(current_water_temp <= minTarg){
      digitalWrite(R1_PIN,LOW);
      digitalWrite(R2_PIN,LOW);
      digitalWrite(R3_PIN,LOW);
    }
    break;
  }
  
}

void setup() { 
  lcd.init();
  lcd.backlight();

  keyUP.attach(KEY_UP); 
  keyUP.interval(5);
  keyDN.attach(KEY_DN); 
  keyDN.interval(5);
  keyOK.attach(KEY_OK); 
  keyOK.interval(5);
 
  pinMode(R1_PIN, OUTPUT);
  pinMode(R2_PIN, OUTPUT);
  pinMode(R3_PIN, OUTPUT);
  digitalWrite(R1_PIN, HIGH);
  digitalWrite(R2_PIN, HIGH);
  digitalWrite(R3_PIN, HIGH);
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_DN, INPUT_PULLUP);
  pinMode(KEY_OK, INPUT_PULLUP);  

  delay(5);
  EEPROM_read_mem(8, &target_water_temp, sizeof(target_water_temp));
  delay(5);
  EEPROM_read_mem(16, &water_gisterezis, sizeof(water_gisterezis));
  delay(5);
  EEPROM_read_mem(24, &target_room_temp, sizeof(target_room_temp));
  delay(5);
  EEPROM_read_mem(32, &room_gisterezis, sizeof(room_gisterezis));
  delay(5);
  EEPROM_read_mem(2, &currentPower, sizeof(currentPower));
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("TermoContriol");      
  lcd.setCursor(0,1);
  lcd.print("v2.0 by Alex");  
  delay (2000);  
}

void loop() {
  unsigned long currentMillis = millis();

  keyUP.update();
  keyDN.update();
  keyOK.update();

  if (currentMillis - SENS_prevMillis >= SENS_interval){
    SENS_prevMillis = currentMillis;
    current_water_temp=GetTemp(water_sens);
    delay(10);
    current_room_temp=GetTemp(room_sens);    
  }  
  if (currentMillis - DISP_SCRL_prevMillis >= DISP_SCRL_interval){
    DISP_SCRL_prevMillis = currentMillis;
    disp_scrl=!disp_scrl;
  } 

  if(keyOK.fell()) mode++;

  if(mode==1){
    if(keyUP.read() == 0) target_water_temp+=0.5;
    if(keyDN.read() == 0) target_water_temp-=0.5;
  }
  if(mode==2){
    if(keyUP.read() == 0) water_gisterezis+=0.5;
    if(keyDN.read() == 0) water_gisterezis-=0.5;
  }
  if(mode==3){
    if(keyUP.read() == 0) target_room_temp+=0.5;
    if(keyDN.read() == 0) target_room_temp-=0.5;
  }
  if(mode==4){
    if(keyUP.read() == 0) room_gisterezis+=0.5;
    if(keyDN.read() == 0) room_gisterezis-=0.5;
  }
  if(mode==5){
    if(keyUP.fell() && currentPower!=2) currentPower++;
    if(keyDN.fell() && currentPower!=0) currentPower--;
  }
  if(mode==6){
    if(keyUP.fell()) sp=1;
    if(keyDN.fell()) sp=0;
  }

  menu();

  relay(heater_power[currentPower]);

  delay(50);
}