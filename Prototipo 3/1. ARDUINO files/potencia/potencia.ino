/**************************************************************************************
SUB-SISTEMA DE POTENCIA (SSP) - ARDUINO PRO MINI
Esclavo SPI: Computa el nivel de bateria y envia al SSC

FUNCIONES
En cada solicitud válida del Maestro, se envia un caracter de la informción
Formtaro del mensaje a ser transmitido  <vBat corriente potencia> entre espacios -> ' '
 _____________________________________________________________________________________
| --SPI--                                 | --I2C--                                   |
| SCK  (serial clock)  : D13              | SCL (serial clock)  : D19/A5              |
| MISO (master in)     : D12              | SDA (serial data)   : D18/A4              |
| MOSI (master out)    : D11              |                                           |
| SS   (select slave)  : D10 slave enable |                                           |
|_________________________________________|___________________________________________|

Lucas Moreira
José Moreira
02-jun-2020   // 8MHz bootloader
***************************************************************************************/
#define buffLength 3              // nro de variables flotantes del buffer

#include <SPI.h>                  // http://www.gammon.com.au/spi
#include <Wire.h>
#include <Adafruit_INA219.h>      // https://github.com/adafruit/Adafruit_INA219

Adafruit_INA219 ina219;           // sensor de corriente

uint8_t pos;                      // https://forum.arduino.cc/index.php?topic=418692.msg2883195#msg2883195

union{
  float f;
  byte b[4];
}buff[buffLength];                // Voltaje, Potencia, Corriente

void setup(void) 
{
  Serial.begin(9600);             // Monitor Serial

  while ( !ina219.begin() )       // By default the initialization (32V, 2A)
  {
    delay(500);
    Serial.println("INA ERROR");
  }
  
  pinMode(SCK, INPUT);
  pinMode(MISO, OUTPUT);          // Configurar MISO como salida esclavo (slave out)
  pinMode(MOSI, INPUT);
  pinMode(SS, INPUT);             // Slave Select como entrada
  
  SPCR |= _BV(SPE);               // Activa el bus SPI en modo esclavo
  SPCR |= _BV(SPIE);              // Activa las interrupciones en el bus SPI
}

ISR (SPI_STC_vect) // Interrupt Service Routines // http://gammon.com.au/interrupts
{
  SPDR = buff[pos/4].b[pos%4];
  pos++;
}


void loop(void) 
{
  if ( digitalRead(SS) == HIGH )            // when it is not transmiting do the calculations
  {
    buff[0].f  = ina219.getBusVoltage_V();
    buff[1].f  = ina219.getCurrent_mA();
    buff[2].f  = ina219.getPower_mW();
    Serial.print("V:"); Serial.print(buff[0].f);
    Serial.print("I:"); Serial.print(buff[1].f);
    Serial.print("P:"); Serial.println(buff[2].f);
    pos = 0;
    delay(50);
  }
}
