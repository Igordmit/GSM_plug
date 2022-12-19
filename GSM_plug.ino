#include <SoftwareSerial.h>
#include <EEPROM.h>

SoftwareSerial mySerial(5, 6);

String phoneNumber = "";
String trustedNumber1 = "";
String trustedNumber2 = "";
String serviceNumber = "7**********";

unsigned long timerCheckConnection = 0;
unsigned long timerReset = 0;
unsigned long timerCRAG = 0;
unsigned long timerWork = 0;
unsigned long currWork = 0;
unsigned long result = 0;

bool connected = true;
bool waitingCRAG = false;
bool loseModule = false;
bool timerEnable = true;

void setup()
{
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  pinMode(7, OUTPUT);
  pinMode(4, INPUT);  

  Serial.begin(19200);                   
  mySerial.begin(19200);                
  Serial.println("Initializing..."); 
  timerCheckConnection = millis();

  delay(5000);
  sendATCommand("AT+CMGF=1");     

  trustedNumber1 = getEEPROMPhone(0);
  trustedNumber2 = getEEPROMPhone(11);
  getWorkTime();
}
 
void loop()
{
  updateSerial();
  checkConnection();
  checkReset();
  checkModule();
  checkWorkTime();
}
 
void updateSerial()
{
  while (Serial.available()) 
  {
    mySerial.println(Serial.readString());
  }

  while(mySerial.available()) 
  {
    String SMSText = "";
    String signalFromSIM = mySerial.readString();
    
    if(signalFromSIM.indexOf("CMGR") != -1){
      if(checkPhoneNumber(signalFromSIM)){
        SMSText = getSMSText(signalFromSIM);
        Serial.println(SMSText);
        executeSMSCommand(SMSText);
      }
      sendATCommand("AT+CMGDA=\"DEL ALL\"");

    }else if(signalFromSIM.indexOf("CMTI") != -1){
      
      String SMSNumber = "";
      SMSNumber = getSMSNumber(signalFromSIM);
      sendATCommand("AT+CMGR=" + SMSNumber);

    } else if (signalFromSIM.indexOf("CREG") != -1){
      if(loseModule){
        sendATCommand("AT+CMGF=1");
        loseModule = false;
      }
      waitingCRAG = false;
      if(!checkCREGStatus(signalFromSIM)){
        digitalWrite(13, HIGH);
        digitalWrite(7, LOW);
        if(connected) {
          timerReset = millis();
        }
        connected = false;
      } else {
        digitalWrite(7, HIGH);
        connected = true;
      }

    }else{
      Serial.println(signalFromSIM);
    }
  }
}

String getSMSNumber(String message){
  int simbolPos;
  simbolPos = message.lastIndexOf(',');
  return message.substring(simbolPos+1);
}

void sendATCommand(String command){
  mySerial.println(command);
}

String getSMSText(String allText){
  int simbolPos;
  String SMSText = "";
  simbolPos = allText.lastIndexOf('"');
  SMSText = allText.substring(simbolPos+1);

  SMSText = SMSText.substring(SMSText.indexOf('\n') + 1);
  SMSText = SMSText.substring(0, SMSText.indexOf('\r'));
  return SMSText;
}

void executeSMSCommand(String SMSText){
  if(SMSText == "1"){
    digitalWrite(13, LOW);
    currWork = timerWork + millis();
    Serial.println(currWork);
  } else if(SMSText == "0") {
    digitalWrite(13, HIGH);
  } else if(SMSText == "2"){
    sendInfo();
  } else if(SMSText.indexOf("N=") != -1){
    String number = "";
    String memoryPosition = SMSText.substring(0, 1);
    number = numberFromSMS(SMSText);
    if(number != ""){
      if(memoryPosition == "1"){
        setEEPROMNumberWrite(number,0);
        trustedNumber1 = getEEPROMPhone(0);
      } 
      else{
        setEEPROMNumberWrite(number,11);
        trustedNumber2 = getEEPROMPhone(11); 
      }
    }
  } else if(SMSText.indexOf("T=") != -1){
      setWorkTime(SMSText);
      getWorkTime();
      Serial.println(timerWork);
  }
}

bool checkPhoneNumber(String SMSText){
  int simbolPos;
  simbolPos = SMSText.indexOf('+', 20);
  phoneNumber = SMSText.substring(simbolPos+1, SMSText.indexOf('"', simbolPos));
  return (phoneNumber == trustedNumber1 || phoneNumber == trustedNumber2 || phoneNumber == serviceNumber);
}

bool checkCREGStatus(String SMSText){
  int simbolPos;
  simbolPos = SMSText.indexOf(',');
  if (SMSText.substring(simbolPos+1, simbolPos+2) == "1"){
    return true;
  }
  return false;
}

void sendInfo(){
  String text = "";
  if(digitalRead(4)){
    text ="Status=off\n";
  }else{
    text ="Status=on\n";
  }
  sendSMS(text + "1N="+ trustedNumber1 + "\n2N="+ trustedNumber2 + "\nTimer=" + String(timerWork/60/1000));
}

String getEEPROMPhone(int seek){
  String tempTrustedNumber = "";
  tempTrustedNumber += (char)EEPROM.read(4 + seek);
  tempTrustedNumber += (char)EEPROM.read(5 + seek);
  tempTrustedNumber += (char)EEPROM.read(6 + seek);
  tempTrustedNumber += (char)EEPROM.read(7 + seek);
  tempTrustedNumber += (char)EEPROM.read(8 + seek);
  tempTrustedNumber += (char)EEPROM.read(9 + seek);
  tempTrustedNumber += (char)EEPROM.read(10 + seek);
  tempTrustedNumber += (char)EEPROM.read(11 + seek);
  tempTrustedNumber += (char)EEPROM.read(12 + seek);
  tempTrustedNumber += (char)EEPROM.read(13 + seek);
  tempTrustedNumber += (char)EEPROM.read(14 + seek);
  return tempTrustedNumber;
}

void setEEPROMNumberWrite(String newPhoneNumber, int seek){
  EEPROM.write(4 + seek,newPhoneNumber[0]);
  EEPROM.write(5 + seek,newPhoneNumber[1]);
  EEPROM.write(6 + seek,newPhoneNumber[2]);
  EEPROM.write(7 + seek,newPhoneNumber[3]);
  EEPROM.write(8 + seek,newPhoneNumber[4]);
  EEPROM.write(9 + seek,newPhoneNumber[5]);
  EEPROM.write(10 + seek,newPhoneNumber[6]);
  EEPROM.write(11 + seek,newPhoneNumber[7]);
  EEPROM.write(12 + seek,newPhoneNumber[8]);
  EEPROM.write(13 + seek,newPhoneNumber[9]);
  EEPROM.write(14 + seek,newPhoneNumber[10]);
}

String numberFromSMS(String text){
  String newNumber = "";
  if(text.indexOf("N=")==1){
    newNumber = text.substring(3);
    return newNumber;
  }
  return "";
}

void resetModule(){
  sendATCommand("AT+CFUN=1,1");
  delay(5000);
  sendATCommand("AT+CMGF=1");
}

unsigned long getTimeInterval(unsigned long timer, unsigned long currTime){
  if(currTime > timer){
    return currTime - timer;
  }
}

void checkConnection(){
  if(millis() - timerCheckConnection > 15000){
    Serial.println("--");
    waitingCRAG = true;
    timerCRAG = millis();
    sendATCommand("AT+CREG?");
    timerCheckConnection = millis();
  }
}

void checkReset(){
  if((!connected) && ((millis() - timerReset) > 60000)){
    timerReset = millis();
    resetModule(); 
  }
}

void checkModule(){
  if(waitingCRAG && ((millis() - timerCRAG) > 5000)){
    digitalWrite(13, HIGH);
    digitalWrite(7, LOW);
    waitingCRAG = false;
    loseModule = true;
  }
}

void checkWorkTime(){
  if(timerEnable && millis()>currWork && !digitalRead(4)){
    digitalWrite(13, HIGH);
  }
}

void sendSMS(String text){
  sendATCommand("AT+CMGS=\"+"+phoneNumber+"\"");
  delay(100);
  sendATCommand(text);
  delay(100);
  mySerial.println((char)26); 
  delay(100);
  mySerial.println();
  delay(4000);
}

void getWorkTime(){
  String txt = "";
  txt += (char)EEPROM.read(1);
  txt += (char)EEPROM.read(2);
  txt += (char)EEPROM.read(3);
  timerWork = txt.toInt() * 60 * 1000;
  Serial.println(timerWork);
}

void setWorkTime(String text){
  Serial.println(text);
  String time = text.substring(2,5);
  Serial.println(time);
  EEPROM.write(1,time[0]);
  EEPROM.write(2,time[1]);
  EEPROM.write(3,time[2]);
}