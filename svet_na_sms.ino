#include <SoftwareSerial.h>
#include <EEPROM.h>//
#include "DHT.h"

// Порты подключения GPRS-модема
SoftwareSerial gprsSerial(7, 8);
DHT dht(4, DHT11);

// Порт Температурного датчика
int tempPin = 1;
int sen_1 = 11;
int sen_2 = 12;
int led = 8;  //объявление переменной целого типа, содержащей номер порта к которому мы подключили второй провод
char phone[13]; // переменная для хранения массива из 20 символов для номера телефона
int addr = 0;
bool ledflag = false;
String tel = "";
long t1a, t1b , t1c;

float temp_dht, humid_dht, temp_lm ,tempOut;
int s1, s2;
long time_pov_ms;
char buf[50];
int min_t;
bool debugflag, gprsMessflag=false;
int minute = 0;
boolean security=true;
int minute2 = 0;
boolean min_alert=false;
int minute3=0;
bool p_s1=false; 

//
String gprs_phonenumber, gprs_command, gprs_param1, gprs_param2 = "";

void setup()  //обязательная процедура setup, запускаемая в начале программы; объявление процедур начинается словом void

{
  // Включаем отладку
  debugflag = true;
  gprsMessflag = true;
  p_s1=false;
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);    // Подаем High на пин 9
  delay(3000);              // на 3 секунды
  digitalWrite(9, LOW);     // и отпускаем в Low. 
  delay(3000);             // Ждём 5 секунд для старта шилда
  dht.begin();

   // Отладка через последовательный порт
  if (debugflag)
    Serial.begin(9600);
 
  
  pinMode(led, OUTPUT); //объявление используемого порта, led - номер порта, второй аргумент - тип использования порта - на вход (INPUT) или на выход (OUTPUT)
  
  EEPROM.get(addr, phone); // считываем массив символов по адресу addr
  delay(150); 
  if (phone[1]+phone[2]+phone[3]+phone[4]+phone[5] >12){
    String telephon(phone);
    DebugText ("new tel set   ");  
    tel=telephon;
  }
  else {
    tel ="+79056897223";
  }
  
  DebugText(tel);
  
  min_t = EEPROM.read(31);
  //if((min_t>10)|| (min_t<2 ))
  //{
  //  EEPROM.write(31, 8);//min t
  //}

  
  gprs_init();
 
  
  t1c=0;
  t1b = 0;
  security = false;
 
  
  delay(500);
  SendMessage("Start");
  //gprs_sendmessage("+79056897223", "Start");
  
}
void loop() //обязательная процедура loop, запускаемая циклично после процедуры setup
{
  t1a = millis();
  if (t1a-t1b > 10000) 
  {
    t1b = t1a;
    Event10sec();   
  }
  // Minute timer
  if  (t1a-t1c > 59900)
  { 
    minute3++;
    minute2++;   
    minute++;
    t1c=t1a; 
  }
  // Day timer
  if (minute >=1440)
  { 
    minute = 0; 
    EventDay();
  }
  
  delay(1000);

}
// реакция на слишком низкую температуру 
void could(int t1_sensor_value, int t2_sensor_value, int min_value)
{ 
  if (min_alert)
 {  SendMessage("minute:"+String(minute2));
    if (minute2 >= 120)
    {
      SendMessage("warning t1=" + String(t1_sensor_value)+" t2="+String(t2_sensor_value)+" min_t="+String(min_value));
      minute2 = 0;
  } 
 }
  else 
  { 
    min_alert = true;
    minute2 = 0;
    SendMessage("warning t1=" + String(t1_sensor_value)+" t2="+String(t2_sensor_value)+" min_t="+String(min_value));
  }
  
}

// Timer one day
void EventDay ()
{
  SendMessage("temp on this day dat1: " +String(temp_dht)+ " dat2: " + String(temp_lm)+"OXP "+String(security));
  DebugText("OXP"+String(security));
}

// Timer 10 second
void Event10sec()
{
  ReadSensorsTemp();
  ReadSensorKeep();
  if (gprs_getMessage(true) == 1)
  {
    DebugText("Input sms:");
    DebugText(gprs_phonenumber);
    DebugText(gprs_command);
    DebugText(gprs_param1);
    DebugText(gprs_param2);
    if (gprs_param1.startsWith("New min_t"))
    {
        EEPROM.write(31, min_t);
        SendMessage("New min_t"+ String(min_t));
    }
    if (gprs_phonenumber.startsWith("Phone"))
    {
      EEPROM.write(32, phone);
      SendMessage("New phone number"+ String(phone));
    }
    if (gprs_command.startsWith("Temp"))
    {
      //DebugText("start eventday");
      EventDay();
    }
    if (gprs_command.startsWith("On"))
    {
      security = true;
      SendMessage("OXP on");
    } 
    if (gprs_command.startsWith("Off"))
    {
      security = false;
      SendMessage("OXP off");
    }
  }
}

//void Event2hour()
//{
  
 
//}

void ReadSensorsTemp()
{
    

    humid_dht = dht.readHumidity();
    temp_dht = dht.readTemperature();
    tempOut = analogRead(tempPin);
    temp_lm = tempOut * 0.48828125;

  
    if((temp_dht<min_t)|| (temp_lm<min_t))   
      could( temp_dht,temp_lm, min_t);
      
    else
      if (min_alert)
      {
        min_alert = false;
        SendMessage("Temp in normal. Temp=" +String(temp_dht));
      }
}

void ReadSensorKeep()
{

    s1=digitalRead(sen_1);
    s2=digitalRead(sen_2);
    DebugText("digitalRead 1 " +String(s1));;
    //SendMessage(String(s1));
    if ((s1==1) && (security))
      {
       AlertSecurity(s1, s2);
       p_s1=true;
       //DebugText(String(s1));
       //if (p_s1)
        // DebugText("p_s1=true");
         
      // else
        //DebugText("p_s1=false");
      }
    else      
    {
      //if (p_s1)
        //DebugText("p_s1=true1");
      //else
        //DebugText("p_s1=fals1");
      if (((p_s1)&&(s1==0))&&(security))
      {   
        p_s1=false;
        SendMessage("Everything in normal. Sen_1= " +String(s1));
      }
    }
}

void AlertSecurity( int s1,int s2)
{ if (security)
  {  
    if(minute3 >=3)
    {  
      SendMessage("Danger, thieves! sen1=" + String(s1));
      minute3=0;
    }
  }
  else
    {  
    minute3=0;
    SendMessage("Danger, thieves! sen1=" + String(s1));
    }
}


void SendMessage(String s)
{ 
  if (debugflag)
    Serial.println(s);
  
  if (gprsMessflag)
    gprs_sendmessage(tel, s);
}

// Отладочные сообщения
void DebugText(String s)
{ 
  if (debugflag)
    Serial.println(s);  
}

void gprs_init()
{
  DebugText ("gprs_init 38400");
  gprsSerial.begin(38400);
  delay(300);
  gprsSerial.println("AT+CLIP=1\r");  //включаем АОН
  delay(300);
  gprsSerial.print("AT+CMGF=1\r");//режим кодировки СМС - обычный (для англ.)
  delay(300);
  gprsSerial.print("AT+IFC=1, 1\r"); // конторль потока
  delay(300);
  gprsSerial.print("AT+CPBS=\"SM\"\r"); // включение адресной книги
  delay(300);
  gprsSerial.print("AT+CNMI=1,2,2,1,0\r");
  delay(3000);
  gprs_getMessage(false);

}


void gprs_sendmessage(String phonenumber, String s)
{
 // DebugText("before gprs_send " + phonenumber);
  gprsSerial.print("AT+CMGF=1\r");
  delay(150);    
  gprsSerial.print("AT + CMGS = \"");
  gprsSerial.print(phonenumber);
  gprsSerial.println("\"");
  delay(100);
  gprsSerial.print(s);
  gprsSerial.println((char)26);
  gprs_getMessage(false);
}

int gprs_getMessage(bool phonenumber_from_only)
{
  if (gprsSerial.available())
  {
    char bufGsm[64]; // buffer array for data recieve over serial port
    String inputGsmStr = ""; //входящая строка с gsm модема
    int countBufGsm = 0;
    //DebugText("available:");
    while (gprsSerial.available()) // reading data into char array
    {
      //DebugText("before read");
      bufGsm[countBufGsm++] = gprsSerial.read(); // writing data into array
      if (countBufGsm == 64) 
        break;
    }
    inputGsmStr += bufGsm;
    //DebugText(String(countBufGsm) + " SMS: " + inputGsmStr);
    // call clearBufferArray function to clear the storaged data from the array
    for (int i = 0; i < countBufGsm; i++)
      bufGsm[i] = NULL;
    
    
    if (phonenumber_from_only) 
    {
      gprs_phonenumber, gprs_command, gprs_param1, gprs_param2 = "";
      if (inputGsmStr.substring(2, 6) != "+CMT")
        return 0;
      else
      { 
        char space = 32;
        char cmd[64];
        int countCmd = 0;
        //DebugText("Structire: ");
        gprs_phonenumber = inputGsmStr.substring(9,21);
        //DebugText(gprs_phonenumber);
        gprs_command = inputGsmStr.substring(50, countBufGsm - 1);
        //DebugText(gprs_command);
        
        int countPoint = 1;
        
        for (int i = 50; i < countBufGsm; i ++)
        {
          if ((bufGsm[i] == char(32) ) || (bufGsm[i] == '\n'))
          {
            //DebugText(String(cmd));
            switch (countPoint){
              case 1: { gprs_command = cmd; break; }
              case 2: { gprs_param1 = cmd; break; }
              case 3: { gprs_param2 = cmd; break; }
            }
            countPoint++;
            for (int x = 0; x < countCmd; x++)
              cmd[x] = NULL;
            countCmd = 0;
          }
          else 
          {
            cmd[countCmd++] = bufGsm[i];
          }
        }
        //DebugText("! " +gprs_phonenumber + gprs_command+gprs_param1+gprs_param2);
        if (gprs_command!="")
          return 1;
      }
    }
    countBufGsm = 0; // set counter of while loop to zero    
    //DebugText("end get_gprs_message-----");
    
  }  
  
  return 0;
}
