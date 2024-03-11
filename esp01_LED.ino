#include <ESP_EEPROM.h>
#include <ESP8266WiFi.h>
 
const char* ssid = "GVPM_CS_2";//type your ssid
const char* password = "GVPM_CS_2023";//type your password
 
WiFiServer server(80);

#define releu 2
#define pir 1
#define buton 0


uint32_t timpDelayPir = 60000;
const uint32_t timpDelayButon = 120000;
uint32_t timpActivarePir = 0;
uint32_t timpActivareButon = 0;
bool pirActivat = false;
bool butonActivat = false;
bool cuplat = LOW;
bool decuplat = HIGH;

int timp = 0;  //valoare default a memoriei EEPROM

void scrieEEPROM(uint8_t );
uint8_t citesteEEPROM();
uint8_t value;


void setup(){
  pinMode(releu, OUTPUT);
  digitalWrite(releu, HIGH);

  pinMode(pir, INPUT_PULLUP);
  pinMode(buton, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  uint32_t timp1 = millis();    //temperizare inceput sa putem iesi din while
  while (WiFi.status() != WL_CONNECTED) {
    uint32_t timp2 = millis();    // timp curent din care scadem timpul de inceput
    delay(100);
    if(timp2 - timp1 > 15000) {
      WiFi.mode(WIFI_OFF);
      break;    //daca eu trecut 15 secunde si !wifi.begin(), iesi din while(fara WILLIs)
    }
  }
  if(WiFi.status() == WL_CONNECTED) server.begin();   //pornim serverul WEB doar daca este WILLIs 

//_______________________________________________________________________________________________________________________
  struct MyEEPROMStruct {
    uint8_t minut;
  } eepromVar1;
  EEPROM.begin(sizeof(MyEEPROMStruct));  //initializam memoria EEPROM la 255(xFF) (maxim 4.25 ore delay (255/60))
  if(EEPROM.percentUsed() >= 0) {
   EEPROM.get(0, eepromVar1);   //daca avem deja ceva scris in EEPROM, citeste acea valoare 
   timpDelayPir = eepromVar1.minut * 15000;   //AICI ASIGNAM TEMPORIZAREA PIR-ULUI increment de 15 secunde
}
  else scrieEEPROM(1);    //daca EEPROM este nescris , initializeaza cu:  1 min. Nu mai citesc valoarea pt. ca default timpDelayPir = 1 min, o citim data viitoare
  value = citesteEEPROM();
//_______________________________________________________________________________________________________________________
}

void loop(){
  if(!digitalRead(pir)) {         // GPIO1
    timpActivarePir = millis();   //timpul la care s-a detectat miscarea
    pirActivat = true;            //activare temporizare
  }
  TemporizareReleu();

  if(!digitalRead(buton)) {        //GPIO0
    if(!butonActivat) timpActivareButon = millis();  //daca deja am apasat butonul nu mai incrementez timpul de activare, 
    butonActivat = true;                             //din cauza modului de programare a timpului de delay
  }
  TemporizareButon();
  webServer(); 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////       //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////  /////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////  /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TemporizareReleu() {
  if(!pirActivat) return false;     //nu am vazut miscare , apeleaza doar loop();
  uint32_t acumP = millis();
  if(acumP - timpActivarePir >= timpDelayPir) {      //sunt inca in timpul de temporizare?
    if(!butonActivat) digitalWrite(releu, decuplat); //decuplam releul doar daca nu avem si temporizare data de buton
    pirActivat = false;             // am terminat cu temporizarea de pa PIR
    timpActivareButon = millis();   //incrementam timpul de activare butom atat timp cat se misca cineva
    timpActivarePir = 0;            // resetam momentul de activare
    return false;                   // returnam din functie, am terminat
  } 
  if(digitalRead(releu)) digitalWrite(releu, cuplat); //daca nu este cuplat releul...il cuplam
  return true;    //returnam cu releul cuplat, nu s-a terminat temporizarea
}

bool TemporizareButon() {
  if(!butonActivat) return false;
  uint32_t acumB = millis();

  if(acumB - timpActivareButon >= timpDelayButon) {   //procedura de incheiere a temporizarii
    digitalWrite(releu, decuplat);
    butonActivat = false;
    timpActivareButon = 0;
    return false;
  }                                                    ////-----------------------------////

  if(acumB - timpActivareButon >= 5000 && !digitalRead(buton)) ZaProsijar();   ///AICI TREBUIE SA APELEZ ZaProsijar() metoda de programarea timpului
  
  if(digitalRead(releu)) digitalWrite(releu, cuplat); //daca nu este cuplat releul...il cuplam...aici sunt in procedura de temporizare
  return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////       /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////    ////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////       /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*1 - 254 minute adica max ~4.2 ore*/
void scrieEEPROM(uint8_t minutul){
  struct MyEEPROMStruct {
    uint8_t minut;
  } eepromVar1;
  if(minutul > 0 && minutul < 254) {
   eepromVar1.minut = minutul;
   EEPROM.put(0, eepromVar1);   //valoarea de modificat in memoria eeprom
  }
  else {
    eepromVar1.minut = 1;
    EEPROM.put(0, eepromVar1); //daca nu avem o valorare valabila 0 - FF, default: 1 min
  }
  EEPROM.commit();    // write the data to EEPROM
  ESP.restart();
}

/*retuneaza valoarea (1 - 254) minute*/
uint8_t citesteEEPROM(){
  struct MyEEPROMStruct {
    uint8_t minut;
} eepromVar1;
  EEPROM.get(0, eepromVar1);
  return eepromVar1.minut;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////  ///  /////  ///  //////////////////////////////////////////////////////////////
////////////////////////////////////  ////  /  ///  ////////////////////////////////////////////////////////////////
/////////////////////////////////////  ///   ///  //////////////////////////////////////////////////////////////////
///////////////////////////////////////  /////  ////////////////////////////////////////////////////////////////////

void webServer(){

    // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
   
  while(!client.available()){
    delay(1);
  }
   
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  client.flush();
   
  // Match the request

  if (request.indexOf("/Increment") != -1) {
    value++;
    if(value > 254) value = 254;
  } 
  if (request.indexOf("/Decrement") != -1){
    value--;
	if(value < 1) value = 1;
    
  }
  if (request.indexOf("/SayOK") != -1){
  scrieEEPROM(value);
  } 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
   
  client.print("Secunde: ");
   
  client.print(String(citesteEEPROM()));  
  client.println("<br><br>");
  client.println("&emsp;<a href=\"/Decrement\">-</a>&emsp;"+String(value)+" &emsp;<a href=\"/Increment\">+</a><br><br>");
  client.println("&emsp;&emsp;<a href=\"/SayOK\"> OK </a><br>");
  client.println("</html>");
 
  delay(1); 
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////       //////////////////////////////////////////////////////////////
///////////////////////////////////////////////  ////  /////////////////////////////////////////////////////////////
///////////////////////////////////////////////  //  ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////  ///////////////////////////////////////////////////////////////////


/*Procedura de programare ar fi urmatoarea:
  - tin apasat butonul timp de minim 5 secunde
  - urmeaza 3 clipiri de 200ms
  - apas nutonul pentru a incrementa timpul
  - daca nu apas butonul pentru mai mult de 5 secunde resetez timpul la 15 sec(valoare default)
  - dupa numarul de secunde dorit (1 = 15sec), astept timpul de iesire din programare
  - dupa 5 secunde urmeaza un numar de clipiri egal cu numarul unitatilor programate  */
void ZaProsijar() {
  WiFi.mode(WIFI_OFF);
  delay(100);
  digitalWrite(releu, decuplat);    // decuplez ledurile pentru a confirma procedura de programare
  int32 ifZaProsijar = millis();
  int32 timptrecut = 0;
  uint8_t secunde = 1; // default 15 secunde, ajuta si la resetarea timpului de temporizare la un default: 15sec
  for(uint8_t m = 0; m < 3 ; m ++) {    //doua beepuri de 200ms ca sa stiu ca sunt in programare.
    digitalWrite(releu, cuplat);
    delay(200);
    digitalWrite(releu, decuplat);
    delay(200);
  }
  while(true){
    if(!digitalRead(buton)) {   //apas odata pe buton petru a incrementa timpul de temporizare(increment de 15 secunde)
      secunde ++;
      digitalWrite(releu, cuplat);
      delay(200); 
      digitalWrite(releu, decuplat);
      delay(200);
      ifZaProsijar = millis();    
    }
    timptrecut = millis();
    if((timptrecut - ifZaProsijar) > 5000 && digitalRead(buton)) break;
    yield(); //o zi si jumatate de batut capul, folosesc delay() sau yieald() din cauza watchdog reset...LA DRACU!!!!
  }
    if(secunde > 1) secunde --;   //din cauzA si anume ca pornesc de la secunde = 1
    for(uint8_t m = 0; m < secunde ; m ++) {    //doua beepuri de 200ms ca sa stiu ca sunt in programare.
      digitalWrite(releu, cuplat);
      delay(300);
      digitalWrite(releu, decuplat);
      delay(300);
    }
    scrieEEPROM(secunde);

}