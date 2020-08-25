/**************************************************************************************
  SUB-SISTEMA DE TELEMETRIA (SST) - ARDUINO PRO MINI
  Esclavo SPI: Recibe los datos del Maestro y los envia a la Estación Terrena

  FUNCIONES
  Recibe cada parte del mensaje por SPI hasta que se llene el buffer
  Luego convierte los datos en caracteres y envia por Software Serial
  ___________________________________________________________________________________
  | --SPI--                                 | --UART--                                |
  | SCK  (serial clock)  : D13              | RX (SoftWare)     : D2                  |
  | MISO (master in)     : D12              | TX (SoftWare)     : D3                  |
  | MOSI (master out)    : D11              |                                         |
  | SS   (select slave)  : D10 slave enable |                                         |
  |_________________________________________|_________________________________________|

  Lucas Moreira
  José Moreira
  27-may-2020
  05-jun-2020                       // probando con 16MHz sin maquina de estados solo recibe
  05-jun-2020                       // probar con 8Mhz
  05-jun-2020                       // cambiar por dtostrf
  12-ago-2020                       // cambie todo por string
  13-ago-2020                       // use muchos print y una variable char pequeña para cada dato
***************************************************************************************/
#define buffLength 13               // tamaño de buffer

#include <SPI.h>                    // libreria para la comunicacion SPI
#include <NeoSWSerial.h>            // https://github.com/SlashDevin/NeoSWSerial

NeoSWSerial ss( 2, 3 );             // RX TX -> tratar de usar hardware 115200(default) del lora

volatile uint8_t pos;

union {
  float f;
  byte b[4];
} buff[buffLength];
/* 
 *  Time
 *  Temperatura Presion Altura
 *  Yaw Pitch Roll
 *  Latitud Longitud Altura2
 *  Voltaje Corriente Potencia
 */

uint32_t *t = (uint32_t*) (&buff[0].f);
float tiempo;
char ctemp[10] = "";
const char inicio[]="AT+SEND=1,79";
const char coma =',';

void setup()
{
  delay(1000);
  Serial.begin(115200);             // Serial para el Monitor
  ss.begin(9600);                   // Serial para el LORA

  ss.println(F("AT+PARAMETER=7,9,4,5"));// LORA config 

  pinMode(SCK, INPUT);
  pinMode(MISO, OUTPUT);            // Configurar MISO como salida esclavo (slave out)
  pinMode(MOSI, INPUT);
  pinMode(SS, INPUT);               // Slave Select como entrada

  SPCR |= _BV(SPE);                 // Activa el bus SPI en modo esclavo
  SPCR |= _BV(SPIE);                // Activa las interrupciones en el bus SPI
}
                                    // https://www.arduino.cc/en/pmwiki.php?n=Reference/Static
                                    // https://www.microchip.com/forums/m1031522.aspx

ISR (SPI_STC_vect)
{
  buff[pos / 4].b[pos % 4] = SPDR;
  pos++;
}

void loop(void)
{
  if ( digitalRead(SS) == HIGH and pos != 0 and ss.available() > 0)    // transmite mientras que el transfer no este ocupado ( y pos sea el maximo o sea este completo? )
  {
    ss.print(inicio);
    
    tiempo = float(*t)/1000.0;    // en segundos
    
    dtostrf(tiempo ,5,1,ctemp); // time ddd,d       ->5,1   +5    = 5
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[1].f ,4,1,ctemp);    // temp dd,d        ->4,1   +4 +1 = 10
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[2].f ,6,0,ctemp);  // pres dddddd      ->6,0   +6 +1 = 17
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[3].f ,5,1,ctemp); // altu ddd,d       ->5,1   +5 +1 = 23
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[4].f ,3,0,ctemp); // angu ddd         ->3,0   +3 +1 = 27
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[5].f ,3,0,ctemp); // angu ddd         ->3,0   +3 +1 = 31
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[6].f ,3,0,ctemp); // angu ddd         ->3,0   +3 +1 = 35
    ss.print(coma);
    ss.print(ctemp);

    //ss.println();
    //ss.print("AT+SEND=1,44,B");
    
    
    dtostrf(buff[7].f ,10,6,ctemp); // lati -dd,dddddd ->10,6  +10+1 = 41  11
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[8].f ,10,6,ctemp); // long -dd,dddddd ->10,6  +10+1 = 52  22
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[9].f ,5,1,ctemp); // altu ddd,d       ->5,1   +5 +1 = 58  27
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[10].f ,5,2,ctemp); // volt dd,dd      ->5,2   +5 +1 = 64  33
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[11].f,4,0,ctemp); // curr dddd        ->4,0   +4 +1 = 69  38
    ss.print(coma);
    ss.print(ctemp);
    
    dtostrf(buff[12].f,4,0,ctemp); // powe dddd        ->4,0   +4 +1 = 74  43
    ss.print(coma);
    ss.print(ctemp);
    
    ss.println();


    pos = 0;                          // reinicia contador para proximo transfer
  }
}
