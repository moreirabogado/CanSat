// <Wire.h> se usa para comunicación I2C para el sensor de presión (no hace falta asignar pines)
// SCL (Serial Clock) : A5
// SDA (Serial Data)  : A4
// <SFE_BMP180.h> que permite utilizar el sensor <> debe estar en libraries
// <SPI.h> Comunicación SPI para el SD (no hace falta asignar pines)
// SCK (Serial Clock)         : D13
// MISO (Master In Slave Out) : D12
// MOSI (Master Out Slave In) : D11
// SS (Slave Select)          : D10

#include <Wire.h>
#include <SFE_BMP180.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>

SFE_BMP180 bmp;
Servo myservo;

// Variables para el BMP
double Po,T,P;
float A=0,Amax=0;

// Variables para los estados
byte estado=0;
unsigned int i=0;
unsigned long previousMillisSD = 0; // will store last time SD was updated  //long intervalSD = 100; //no se puso para ahorrar espacio
unsigned long previousMillisServo = 0;  // will store last time SD was updated  //long intervalServo = 2000; //no se puso para ahorrar espacio

// Asignación de pines
//const int chipSelect = 10; //Solamente srive para verificar si la SD está presente //no se puso para ahorrar espacio
//const int pinBuzzer = 9; //no se puso para ahorrar espacio

void setup()
{
  myservo.attach(2);
  iniciarBMP();
  SD.begin(10);
}

void loop()
{
  actualizarPresion();
  A = 0.6*A + 0.4*bmp.altitude(P,Po); //Calcular altura con respecto al punto de referencia
  if(Amax<A) Amax=A;

  switch(estado)
  {
    case 0:
      if(millis() - previousMillisSD > 50)
      {
        previousMillisSD = millis();
        guardarSD();
      }
      if( A>2)
      {
        estado=1;
      }
      break;
    case 1:
      guardarSD();
      if( ( A<(Amax-0.5) or A>30.0) and A>5.0 )
      {
        previousMillisServo = millis();
        myservo.attach(3);
        estado=2;
      }
      break;
    case 2:
      guardarSD();
      myservo.write(250);
      if(millis() - previousMillisServo > 1000)
      {
        myservo.attach(2);
      }
      if(A<=0)
      {
        myservo.attach(2);
        estado=3;
      }
      break;
    case 3:
      guardarSD();
      delay(1000);
      actualizarPresion();
      A = 0.6*A + 0.4*bmp.altitude(P,Po);
      guardarSD();
      tone(9,2000);
      delay(500);
      actualizarPresion();
      A = 0.6*A + 0.4*bmp.altitude(P,Po);
      guardarSD();
      tone(9,1500);
      delay(500);
      break;
  }
}

void iniciarBMP()
{
  if (bmp.begin())
  {
    actualizarPresion();
    Po=P;
  }
}

void actualizarPresion()
{
  char status;
  status = bmp.startTemperature(); //Inicio de lectura de temperatura
  if (status)
  {
    delay(status); //Pausa para que finalice la lectura
    status = bmp.getTemperature(T);  //Obtener la temperatura
    if (status)
    {
      status = bmp.startPressure(3); //Inicio lectura de presión
      if (status)
      {
        delay(status); //Pausa para que finalice la lectura
        status = bmp.getPressure(P,T); //Obtener la presión
        if(status)
        {
          return;
        }
      }
    }
  }
}

void guardarSD()
{
  String dataString = "";
  i++;
  dataString = String(i);
  dataString += ",";
  dataString += String(previousMillisSD/1000.0);
  dataString += ",";
  dataString += String(estado);
  dataString += ",";
  dataString += String(P);
  dataString += "," ;
  dataString +=String(T);
  dataString +=",";
  dataString +=String(A);
          
  File dataFile = SD.open("ejemplo.txt", FILE_WRITE);
  if (dataFile)
  {
    dataFile.println(dataString);
    dataFile.close();
  }
}
