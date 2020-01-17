#include <LiquidCrystal_I2C.h>
#define pin 7

LiquidCrystal_I2C lcd(0x27, 16,2);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(pin,OUTPUT);
  
  lcd.begin();
  lcd.backlight();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Lock!");
  
  lcd.clear();
  lcd.print("Door is locked!");
  
  digitalWrite(pin,HIGH);
  delay(2000);
  Serial.println("Unlock!");
  digitalWrite(pin, LOW);
  
  lcd.clear();
  lcd.print("Door is open!");
  
  delay(2000);
}
