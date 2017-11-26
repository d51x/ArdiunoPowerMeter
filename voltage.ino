#include <EmonLib.h>
#include <MsTimer2.h>
#include <EEPROM2.h>

EnergyMonitor emon1;   

unsigned long interval = 1000; // интервал отправки данных с датчиков

const int pin_current = A0;  
const int pin_voltage = A1;  
const int pin_mercury = 2; 
unsigned long loopTime;
#define readbufsize 100 // максимальный размер буфера 
char *buf;
 
int sensorValue = 0; 
double current_div = 0.486;
double g = 111.1;

volatile unsigned int state = 3200;
volatile unsigned int blinkMin = 0;
volatile unsigned int tm = 0;
uint32_t kWh = 0;
unsigned int Wm = 0;

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  emon1.current(pin_voltage, g); 
Serial.begin(9600);

    while (!Serial) {
    //  ; // wait for serial port to connect. Needed for native USB port only
  } 

     //*Для синхронизации показаний электросчетчика с Ардуиной раскомментировать 2 строчки ниже
   //*в строку kWh = 0; вместо нуля подставить текущее значение счетчика
   //*загрузить скетч, потом закомментировать эти 2 строки и еще раз загрузить скетч
 
   //kWh = 0;  
   //EEPROM_write(1, kWh);

   //interval = 1000;
   //EEPROM_write(5, interval);
   EEPROM_read(5, interval);

   //current_div = 0.486;
   //EEPROM_write(10, current_div);
   EEPROM_read(10, current_div);

   pinMode(pin_mercury, INPUT_PULLUP); 

  EEPROM_read(1, kWh); //читаем показания электросчетчика из EEPROM
 
  attachInterrupt(0, blink, RISING);  // 0 - первое прерывание, pin2

  MsTimer2::set(60000, MsTimer);  
  MsTimer2::start();  
   buf = new char[readbufsize];
}

int rnd() {

 int i = random(100, 1000);
 Serial.println(i);
 return i; 
}
// the loop function runs over and over again forever
void loop() {

  //********************* выяисления *************************
  sensorValue = analogRead(pin_current);  
  int volt = int(sensorValue*current_div);          
  double Irms = emon1.calcIrms(2000);  // Calculate Irms only
  if ( Irms < 0.27 ) Irms = 0;
  //Serial.print(Irms*230.0);           // Apparent power
  double powerrms = Irms*volt;


//********************************** отправка данных ********************************
  unsigned long currentTime = millis();                           // считываем время, прошедшее с момента запуска программы
  if (currentTime >= (loopTime + interval)) {            // сравниваем текущий таймер с переменной loopTime

    Serial.print("electro/voltage ");
    Serial.println(volt, DEC);
delay(50);
    Serial.print("electro/current ");
    Serial.println(Irms, 2);  
delay(50);
    Serial.print("electro/power ");
    Serial.println(powerrms, 2);  
delay(50);

    Serial.print("electro/kwth ");
    Serial.println(kWh, DEC);  
delay(50);
              
    //Serial.print("electro/interval ");
    //Serial.println(interval, DEC); 
     
     //Serial.print("electro/dividerc ");
    //Serial.println(current_div, 3);     
     loopTime = currentTime;
  }


  //********************* прием данных *****************************
  // прием команд с парсингом с вЫключенной опцией "Disable send name topic" в ESP
  // с поддержкой atoi - может быть любое число
  int i = 0;
    while (Serial.available() && (i < readbufsize)) {
      buf[i++] = Serial.read();
      delay(2);
    }  

  if (i) { // в UART что-то пришло. обрабатываем
  char *pdata = NULL;
  char lwt[30];
  char lwt2[30];
  
  // пишем в топик email/arduino/gpioset 13 для установки 13 пина в единицу
  // не поддерживается mqtt flymon
  pdata = NULL;
      sprintf(lwt,"electro/interval ");
      sprintf(lwt2,"electro/dividerc ");
          pdata = (char *)strstr(buf,lwt);
   
        if (pdata != NULL) {
          pdata+= strlen(lwt);
          unsigned long intr  = atoi(pdata);
          if ( intr >= 1000) {
              interval = intr;
               EEPROM_write(5, interval);

          }
        }
        
        pdata = NULL; 
          pdata = (char *)strstr(buf,lwt);
          if (pdata != NULL) {
          pdata+= strlen(lwt);
          double divc  = atof(pdata);
          if ( divc > 0) {
              current_div = divc;
               EEPROM_write(10, current_div);

          }              
          }
        

  }
}

void MsTimer(){
  Wm = blinkMin;  
  blinkMin = 0;
}
 
void blink(){
  state--;
  if(state == 0 || state > 3200){ 
     kWh++;
     EEPROM_write(1, kWh); //Записываем текущее показание электросчетчика
     state = 3200; 
   } 
  blinkMin++;
}

