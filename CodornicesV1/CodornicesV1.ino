


//Libreria para ESP8266
#define EEPROM_SIZE 12

//Librerias reloj
#include <Wire.h> // Library for I2C communication
#include <SPI.h>  // not used here, but needed to prevent a RTClib compile error
#include "RTClib.h"


//Librerias WiFi
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

//Librerias DNS
#include <ESP8266mDNS.h>
#include <LEAmDNS.h>
#include <LEAmDNS_lwIPdefs.h>
#include <LEAmDNS_Priv.h>

//Librerias Servidor
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

ESP8266WiFiMulti wifiMulti;

//Para incluir otra pestaña
#include "datos.h"

//Salidas de focos
int pinAmarillo = D0;
int pinAzul = D1;
int pinVerde = D7;
int pinBlanco = D8;

int focos = 1;

//Variables controladoras
int amarillo = 0;
int azul = 0;
int verde = 0;
int blanco = 1;
int tiempo = 1;
int posicion [4]= {4,4,4,3};
int tiempoFoco[4] = {0,0,0,1};
int tiempoIndividual = 0;
int hora = 1080;

//Variable para página web
String Estado_Foco;

//Variable de espera
const uint32_t TiempoEsperaWiFi = 5000;

//Variable para servidor
AsyncWebServer servidor(80);


//Metodo para ejecutar acciones en la página web
String processor (const String& var) {

  //Datos Generales
  if (var == "IP") {
    String aux1=WiFi.localIP().toString();
    Estado_Foco = " " + aux1;
  }

  if (var == "WIFI") {
    String aux1=WiFi.SSID();
    Estado_Foco = " " + aux1;
  }

  //Información de focos activos
  if (var == "AMARILLO") {

    if (amarillo == 1) {
      Estado_Foco = "ACTIVO";
    }
    else {
      Estado_Foco = "";
    }
  }

  if (var == "AZUL") {

    if (azul == 1) {
      Estado_Foco = "ACTIVO";
    }
    else {
      Estado_Foco = "";
    }
  }

  if (var == "VERDE") {

    if (verde == 1) {
      Estado_Foco = "ACTIVO";
    }
    else {
      Estado_Foco = "";
    }
  }

  if (var == "BLANCO") {

    if (blanco == 1) {
      Estado_Foco = "ACTIVO";
    }
    else {
      Estado_Foco = "";
    }
  }


  //Información de horas de prendido de focos
  if (var == "TIEMPO_PRENDIDO") {

    if(tiempo == 60){
    Estado_Foco = "ACTIVO UNA HORA DE PRENDIDO";
    }

    if(tiempo == 120){
    Estado_Foco = "ACTIVO DOS HORAS DE PRENDIDO";
    }

    if(tiempo == 180){
    Estado_Foco = "ACTIVO TRES HORAS DE PRENDIDO";
    }

    if(tiempo == 240){
    Estado_Foco = "ACTIVO CUATRO HORAS DE PRENDIDO";
    }

  }

  return Estado_Foco;
}


//Instancias de Reloj
RTC_DS3231 RTC;
DateTime ahora;



void setup() {
  //Puerto serie
  Serial.begin(9600);

//Inicialización del Reloj
  Wire.begin(D3, D4);                         // Iniciar el I2C [SDA = D3, SCL = D4], la frecuencia del reloj por default es 100kHz
  RTC.begin();                               // Inicio del RTC

//Busqueda de RTC
  if (! RTC.begin()) {
    Serial.println("No encuentra el RTC");
    Serial.flush();
    while (1) delay(10);
  }

 if (RTC.lostPower()) {
    Serial.println("RTC perdio energía, fijaremos el tiempo!");
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
   
  }

 RTC.disable32K();

  //Puertos en salida
  pinMode(pinAmarillo, OUTPUT);
  pinMode(pinAzul, OUTPUT);
  pinMode(pinVerde, OUTPUT);
  pinMode(pinBlanco, OUTPUT);

  
  //Conexión WiFi
  Serial.println("\nInciando multi WiFi");
  wifiMulti.addAP(ssid1, password1);
  // wifiMulti.addAP(ssid2, password2);
  wifiMulti.addAP(ssid3, password3);
  wifiMulti.addAP(ssid4, password4);


  //Tipo de WiFi, modo estación
  WiFi.mode(WIFI_STA);
  Serial.print("Conectando a WiFi ..");
  while (wifiMulti.run(TiempoEsperaWiFi) != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println(".. Conectado");
  Serial.println("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.println("ID: ");
  Serial.println(WiFi.localIP());

  //Configuración DNS
  if (!MDNS.begin("codornices")) {
    Serial.println("Error en la configuracion DNS!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS configurado");
  Serial.println("NOMBRE: codornices.local");

  //Cargar pagina web
  if (!SPIFFS.begin()) {
    Serial.println("Ha ocurrido un error mientras se monta el SPIFFS");
    return;
  }

  //Ingresar a la página
  servidor.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  //Para solicitar el CSS
  servidor.on("/estilos.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/estilos.css", "text/css");
  });



  //Cuando pulsamos el botón encendido AMARILLO
  servidor.on("/Amarillo=ON", HTTP_GET, [] (AsyncWebServerRequest * request) {
    amarillo = 1;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  //Cuando pulsamos el botón apagado
  servidor.on("/Amarillo=OFF", HTTP_GET, [] (AsyncWebServerRequest * request) {
    amarillo = 0;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });


  //Cuando pulsamos el botón encendido AZUL
  servidor.on("/Azul=ON", HTTP_GET, [] (AsyncWebServerRequest * request) {
    azul = 1;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  //Cuando pulsamos el botón apagado
  servidor.on("/Azul=OFF", HTTP_GET, [] (AsyncWebServerRequest * request) {
    azul = 0;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });



  //Cuando pulsamos el botón encendido VERDE
  servidor.on("/Verde=ON", HTTP_GET, [] (AsyncWebServerRequest * request) {
    verde = 1;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  //Cuando pulsamos el botón apagado
  servidor.on("/Verde=OFF", HTTP_GET, [] (AsyncWebServerRequest * request) {
    verde = 0;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });



  //Cuando pulsamos el botón encendido BLANCO
  servidor.on("/Blanco=ON", HTTP_GET, [] (AsyncWebServerRequest * request) {
    blanco = 1;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  //Cuando pulsamos el botón apagado
  servidor.on("/Blanco=OFF", HTTP_GET, [] (AsyncWebServerRequest * request) {
    blanco = 0;
     TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });



  //Cuando pulsamos el botón encendido TIEMPO
  servidor.on("/Tiempo=una", HTTP_GET, [] (AsyncWebServerRequest * request) {
    //Transforma las horas a minutos
    tiempo = 1 * 60;
    TIEMPO();
    //Llamado a método para la suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  servidor.on("/Tiempo=dos", HTTP_GET, [] (AsyncWebServerRequest * request) {
    //Transforma las horas a minutos
    tiempo = 2 * 60;
    TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();

    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  servidor.on("/Tiempo=tres", HTTP_GET, [] (AsyncWebServerRequest * request) {
    //Transforma las horas a minutos
    tiempo = 3 * 60;
    TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  servidor.on("/Tiempo=cuatro", HTTP_GET, [] (AsyncWebServerRequest * request) {
    //Transforma las horas a minutos
    tiempo = 4 * 60;
    TIEMPO();
    //Suma del tiempo de cada foco al tiempo actual
    tFoco();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });


 //RTC.adjust(DateTime(2022,9,24,18,00,00));  // Actualizción de fecha y hora manual para pruebas
// RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Actualizción de fecha y hora del sistema en el DS3231

 
  
  //Iniciamos servidor y DNS
  servidor.begin();
  MDNS.addService("http", "tcp", 80);


}



void loop() {

  //Código de prueba de focos sin reloj
  MDNS.update();


  // Tomar la fecha y hora actúal del RTC
  ahora = RTC.now();

  Serial.print(ahora.year(), DEC);
  Serial.print('/');
  Serial.print(ahora.month(), DEC);
  Serial.print('/');
  Serial.print(ahora.day(), DEC);
  Serial.print(' ');
  Serial.print(ahora.hour(), DEC);
  Serial.print(':');
  Serial.print(ahora.minute(), DEC);
  Serial.print(':');
  Serial.print(ahora.second(), DEC);
  Serial.println();

  Serial.print("Amarillo: ");
  Serial.println(amarillo);
  Serial.print("Azul: ");
  Serial.println(azul);
  Serial.print("Verde: ");
  Serial.println(verde);
  Serial.print("Blanco: ");
  Serial.println(blanco);
  Serial.println(" ");

  Serial.print("Salida Amarillo: ");
  Serial.println(digitalRead(pinAmarillo));
  Serial.print("Salida Azul: ");
  Serial.println(digitalRead(pinAzul));
  Serial.print("Salida Verde: ");
  Serial.println(digitalRead(pinVerde));
  Serial.print("Salida Blanco: ");
  Serial.println(digitalRead(pinBlanco));
  Serial.println(" ");

  Serial.print("Tiempo Individual: ");
  Serial.println(tiempoIndividual);
  
  Serial.print("Tiempo de prendido de cada foco: ");
  Serial.println(tiempoFoco[0]);
  Serial.println(tiempoFoco[1]);
  Serial.println(tiempoFoco[2]);
  Serial.println(tiempoFoco[3]);
  Serial.println(" ");

  Serial.println("Posición: ");
  Serial.println(posicion[0]);
  Serial.println(posicion[1]);
  Serial.println(posicion[2]);
  Serial.println(posicion[3]);
  Serial.println(" ");


  
  //Calculo del tiempo en minutos de prendido de cada foco
    focos = amarillo + azul + verde + blanco;
   

  //Función para calcular el tiempo individual y validación de focos
   TIEMPO();
   tFoco();

   
  //Llamado a la función para transformar la hora actual a minutos
  hora = tiempoMinutos(ahora.hour(), ahora.minute());

  delay(5000); // cinco segundos

 


  //Validación para que funcionen los focos solo entre 18:00 a 22:00
  if (ahora.hour() >= 18 && ahora.hour() < 22) {

  
    // Prende al amarillo primero transformando las 18 horas a minutos es 1080 minutos
    if ((hora > 1080 && hora <= tiempoFoco[0]) && posicion[0] == 0) {
      Serial.println("Entro 0");
      digitalWrite(pinAmarillo, HIGH);

    } else if ((hora > tiempoFoco[0] && posicion[0] == 0) || amarillo == 0){
      digitalWrite(pinAmarillo, LOW);

    }

    // Prende al azul segundo
    if ((hora > tiempoFoco[0] && hora <= tiempoFoco[1]) && posicion[1] == 1) {
      Serial.println("Entro 1");
      digitalWrite(pinAzul, HIGH);

    } else if ((hora >= tiempoFoco[1] && posicion[1] == 1) || (azul == 0)) {
      digitalWrite(pinAzul, LOW);
      
    }

    // Prende al verde tercero
    if ((hora > tiempoFoco[1] && hora <= tiempoFoco[2]) && posicion[2] == 2) {
      Serial.println("Entro 2");
      digitalWrite(pinVerde, HIGH);

    } else if ((hora > tiempoFoco[2] && posicion[2] == 2) || verde == 0){
      digitalWrite(pinVerde, LOW);

    }

    // Prende al blanco cuarto
    if ((hora > tiempoFoco[2] && hora <= tiempoFoco[3]) && posicion[3] == 3) {
      Serial.println("Entro 3");
      digitalWrite(pinBlanco, HIGH);

    } else if ((hora > tiempoFoco[3] && posicion[3] == 3) || (blanco == 0)) {
      digitalWrite(pinBlanco, LOW);

    }
  } else if (ahora.hour() > 6 && ahora.hour() < 7) {
    digitalWrite(pinBlanco, HIGH);

  } else {

    digitalWrite(pinAmarillo, LOW);
    digitalWrite(pinAzul, LOW);
    digitalWrite(pinVerde, LOW);
    digitalWrite(pinBlanco, LOW);

  }

}


//Método de cálculo de tiempo real en minutos
int tiempoMinutos(int h, int m) {
  h = h * 60;
  m = h + m;
  Serial.print("Tiempo recorriendo: ");
  Serial.println(m);
  return (m);
}


//Método para fijar el tiempo de cada foco
void tFoco() {

int aux = 1080;
  if(amarillo == 1){
     posicion[0] = 0;
     tiempoFoco[0] = tiempoIndividual + aux;
     aux = tiempoFoco[0];
  }else {
     posicion[0] = 4;
     tiempoFoco[0] = 0;
     aux = 1080;
  }

  if(azul == 1){
     posicion[1] = 1;
     tiempoFoco[1] = tiempoIndividual + aux;
     aux = tiempoFoco[1];
  }else {
     posicion[1] = 4;
     tiempoFoco[1] = 0;
     aux = 1080;
  }
  
  if(verde == 1){
     posicion[2] = 2;
     tiempoFoco[2] = tiempoIndividual + aux;
     aux = tiempoFoco[2];
  }else {
     posicion[2] = 4;
     tiempoFoco[2] = 0;
     aux = 1080;
  }
  
  if(blanco == 1){
     posicion[3] = 3;
     tiempoFoco[3] = tiempoIndividual + aux;
     aux = tiempoFoco[3];
  }else {
     posicion[3] = 4;
     tiempoFoco[3] = 0;
     aux = 1080;
  }
  
}


//Validación de luces para evitar división para cero
void TIEMPO(){

   if(amarillo == 0 && azul == 0 && verde == 0 && blanco == 0){
     for(int i=0 ; i<=3; i++){
     digitalWrite(pinAmarillo, HIGH);
      delay(1000);
     digitalWrite(pinAmarillo, LOW);
      delay(100);
     digitalWrite(pinAzul, HIGH);
      delay(1000);
     digitalWrite(pinAzul, LOW);
      delay(100);
     digitalWrite(pinVerde, HIGH);
      delay(1000);
     digitalWrite(pinVerde, LOW);
      delay(100);
     digitalWrite(pinBlanco, HIGH);
      delay(1000);
     digitalWrite(pinBlanco, LOW);
      delay(100);
    }
    blanco = 1;
    posicion[3] = 3;
   }else{
    tiempoIndividual = tiempo / focos;
   }
  
}
