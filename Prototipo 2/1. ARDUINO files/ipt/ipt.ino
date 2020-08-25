#include <Wire.h>
// Se usa para comunicación I2C para el sensor de presión (no hace falta asignar pines)
// SDA (Serial Data)  : 2
// SCL (Serial Clock) : 3

#include <SPI.h>
// Comunicación SPI para el SD (no hace falta asignar pines)
// SS (Slave Select)          : 10 (este si se debe asignar - permite controlar al esclavo)
// SCK (Serial Clock)         : 15
// MISO (Master In Slave Out) : 14
// MOSI (Master Out Slave In) : 16


// <SFE_BMP180.h> que permite utilizar el sensor <> debe estar en libraries
#include <SFE_BMP180.h>
#include <SD.h>
#include <Servo.h>

SFE_BMP180 bmp;
Servo motor;

// Variables para el BMP
double Po=0, T, P, A=0, Amax=0;

// Variables para los estados
byte estado = 0;

// Asignación de pines
//const int pinBuzzer = 9; //no se puso para ahorrar espacio

void setup()
{
  motor.attach(6);  //pin del motor
  motor.write(90);  //posicion cerrada
  pinMode(9,OUTPUT); //buzzer
  SD.begin(10);   //seleccionar al esclavo
  
  bmp.begin();
  delay(3000);
  for(int i=0;i<100;i++)
  {
    actualizarBMP();
    Po = Po + P/100.0;
  }
  analogWrite(9,255);
  delay(100);
  analogWrite(9,0);
}

void loop()
{
  actualizarBMP();
  guardarSD();
  
  switch (estado)
  {
    case 0:
      if ( A > 2.0 )
      {
        estado = 1;
      }
      break;
      
    case 1:
      if ( ( A < (Amax - 1.0) and A > 10.0) )
      {
        estado = 2;
      }
      break;
    
    case 2:
      motor.write(0);
      
      if (A <= 0)
      {
        estado = 3;
      }
      break;
    
    case 3:
      analogWrite(9,millis()%256);

      if ( A > 2 )
      {
        motor.attach(5);
        analogWrite(9,0);
      }
      break;

    case 4:

  }
}

void actualizarBMP()
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
        status = bmp.getPressure(P, T); //Obtener la presión
        if (status)
        {
          A = 0.6*bmp.altitude(P, Po) + 0.4*A ;
          if (A > Amax)
          {
            Amax = A;
          }
          return;
        }
      }
    }
  }
}

void guardarSD()
{
    String dataString = String(millis()/1000.0) + ";" + String(estado) + ";" + String(T) + ";" + String(P) + ";" + String(A);

    File dataFile = SD.open("datos.txt", FILE_WRITE);
    if (dataFile)
    {
      dataFile.println(dataString);
      dataFile.close();
    }
}
