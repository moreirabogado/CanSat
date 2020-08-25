/**************************************************************************************
SUB-SITEMA DE COMPUTO (CCS) - SPARKFUN SAMD21 MINI
Maestro SPI: debe ser el computador de bordo que calcule y transfiera los datos al 
esclavo para que este envie por telemetría.

FUNCIONES
enviar mensajes como si fuera monitor al lora
poner un caracter al comienzo como indentificador de cada funcion y otro para el final
siempre escuchar por señales del monitor ?
 _____________________________________________________________________________________
| --SPI--                                 | --I2C--                                   |
| SCK  (serial clock)  : D13              | SCL (serial clock)  : D21                 |
| MISO (master in)     : D12              | SDA (serial data)   : D20                 |
| MOSI (master out)    : D11              |                                           |
| SS1  (select slave1) : D10 telemetria   | --UART--                                  |
| SS2  (select slave2) : D7  uSD          | SerialUSB           : USBport Monitor     |
| SS3  (select slave3) : D8  mision       | Serial1 RX          : D0 gps receiver     |
| SS4  (select slave4) : D9  potencia     | Serial1 TX          : D1 gps transmitter  |
|_________________________________________|___________________________________________| 

Lucas Moreira
José Moreira
01-jun-2020
05-jun-2020     // cambie el funcionamiento del tiempo de muestreo y funciono SSP
07-jun-2020     // cambie micros() por millis()
11-ago-2020     // agregue una variable mas al comienzo del buffer
***************************************************************************************/
#define SS1 10        // SST
#define SS2 7         // SD
#define SS3 8         // SSM
#define SS4 9         // SSP

#define buffLength 13               // cantidad de variables en el buffer de datos
#define FILE_BASE_NAME "Data"       // nombre base del archivo en SD
#define SAMPLE_INTERVAL_MS 100      // tiempo de muestreo en Milisegundos

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>        // https://github.com/adafruit/Adafruit_BMP280_Library
#include <SparkFunMPU9250-DMP.h>    // https://github.com/sparkfun/SparkFun_MPU-9250-DMP_Arduino_Library
#include <NMEAGPS.h>                // https://github.com/SlashDevin/NeoGPS
#include <SdFat.h>                  // https://github.com/greiman/SdFat

Adafruit_BMP280 bmp;                // objeto de la clase Adafruit_BMP280
MPU9250_DMP imu;                    // objeto de la clase MPU9250_DMP
NMEAGPS  gps;                       // decodifica los datos recibidos
gps_fix  fix;                       // estructura que almacena las variables
SdFat sd;                           // sistema de archivos tipo FAT16
SdFile file;                        // archivo para la tarjeta SD

SPISettings spiConfigA(2000000, MSBFIRST, SPI_MODE0);   // 2 MHz default for ATMEGA328P 3.3V 8MHz
SPISettings spiConfigB(4000000, MSBFIRST, SPI_MODE0);   // 4 MHz default for ATMEGA328P 5V 16MHz
SPISettings spiConfigC(12000000, MSBFIRST, SPI_MODE0);  // 12 MHz default for ATSAMD21 3.3v 48MHz

char fileName[] = FILE_BASE_NAME "00.csv";  // nombre completo del archivo
float pressureRef;                          // presion al inicio de la mision, referencia
uint32_t tiempoCargar;                      // tiempo en millisegundos (aprox 70 min)
float *tempo = (float*) (&tiempoCargar);    // variable para cargar en el buffer de tipo puntero a flotante
union
{
  float f;
  byte b[4];
}buff[buffLength];                          // buffer de datos (#variables)

void setup()
{
  delay(5000);
  SerialUSB.begin(9600);                // Monitor Serial
  Serial1.begin(9600);                  // GPS Serial
  
  // SPI settings recommendation        -> https://www.arduino.cc/en/Tutorial/SPITransaction
  // Multiple SPI chip select question  -> https://forum.arduino.cc/index.php?topic=103448.0
  pinMode(SS1, OUTPUT);
  pinMode(SS2, OUTPUT);
  pinMode(SS3, OUTPUT);
  pinMode(SS4, OUTPUT);
  digitalWrite(SS1, HIGH);
  digitalWrite(SS2, HIGH);
  digitalWrite(SS3, HIGH);
  digitalWrite(SS4, HIGH);
  SPI.begin();                          // SPI MODO MAESTRO. Outputs: SCK(low),MOSI(low),SS(high)


  while (!bmp.begin(BMP280_ADDRESS_ALT))
  {
    delay(500);
    SerialUSB.println(F("BMP BEGIN ERROR"));
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // Operating Mode
                  Adafruit_BMP280::SAMPLING_X2,     // Temperature Oversampling
                  Adafruit_BMP280::SAMPLING_X16,    // Pressure Oversampling
                  Adafruit_BMP280::FILTER_X16,      // Filtering
                  Adafruit_BMP280::STANDBY_MS_63);  // Standby Time
                  
  while (imu.begin() != INV_SUCCESS)
  {
    delay(500);
    SerialUSB.println(F("IMU BEGIN ERROR"));
  }
  imu.dmpBegin( DMP_FEATURE_6X_LP_QUAT |            // 6-axis (accel/gyro) quaternion calculation
                DMP_FEATURE_GYRO_CAL,               // Gyroscope calibration (0's out after 8 seconds of no motion)
                10);                                // Requested sample rate in Hz

  pressureRef = bmp.readPressure();                 // Referencia de la presion en Pa

  tiempoCargar = millis()/SAMPLE_INTERVAL_MS;       // Saber cuantos multiplos del tiempo de muestreo ya ocurrieron
  tiempoCargar = tiempoCargar + 1;                  // para comienzar en el proximo multiplo del tiempo de carga
  tiempoCargar = tiempoCargar * SAMPLE_INTERVAL_MS; // escalar a millisegundos (multiplo del tiempo de muestreo)
  
  renameFile();          // Verifica si ya hay un archivo con el mismo Nombre
  writeHeader();         // Escribe la cabecera para la hoja de datos
}

void loop()
{
  tiempoCargar = tiempoCargar + SAMPLE_INTERVAL_MS; // Tiempo para la proxima carga
  do
  {
    obtener_gps();                                  // hace pasar el tiempo, hasta que el tiempo actual 
  } while (millis() < tiempoCargar);                // llegue el tiempo de carga

  buff[0].f = *tempo;                               // Carga el tiempo en el buffer

  obtener_bmp();
  obtener_imu();
  obtener_gps();
  obtener_bat();
  enviar_sst();
  logData();

  SerialUSB.print("Time:");           SerialUSB.print(tiempoCargar);
  SerialUSB.print(", Temp:");         SerialUSB.print(buff[1].f);
  SerialUSB.print(", Pres:");         SerialUSB.print(buff[2].f);
  SerialUSB.print(", Alti:");         SerialUSB.print(buff[3].f);
  SerialUSB.print(", yaw:");          SerialUSB.print(buff[4].f);
  SerialUSB.print(", pit:");          SerialUSB.print(buff[5].f);
  SerialUSB.print(", rol:");          SerialUSB.print(buff[6].f);
  SerialUSB.print(", Latitude: ");    SerialUSB.print(buff[7].f,6);
  SerialUSB.print(", Longitude:");    SerialUSB.print(buff[8].f,6);
  SerialUSB.print(", Altitude: ");    SerialUSB.print(buff[9].f);
  SerialUSB.print(", volt:");         SerialUSB.print(buff[10].f);
  SerialUSB.print(", curr:");         SerialUSB.print(buff[11].f);
  SerialUSB.print(", pote:");         SerialUSB.print(buff[12].f);
  SerialUSB.println();
}


void obtener_bmp()
{
  buff[1].f = bmp.readTemperature();                // temperature in degress celcius   // Temperature resolution 0.01°C // -40°C to +85°C
  buff[2].f = bmp.readPressure();                   // Pressutre in SI units for Pascal // Relative accuracy ±0.12 hPa or ±12 Pa
  buff[3].f = bmp.readAltitude(pressureRef/100.0);  // Supplied sea level hPa as a reference. // Relative accuracy ±1m
}

/*  In aeronautical and aerospace engineering intrinsic rotations around these axes are often called Euler angles.
 *  
 *  Normal axis, or yaw axis — an axis drawn from top to bottom.
 *  Transverse axis, or pitch axis — an axis running from the pilot's left to right in piloted aircraft
 *  Longitudinal axis, or roll axis — an axis drawn through the body of the vehicle from tail to nose.
 *  https://en.wikipedia.org/wiki/Aircraft_principal_axes#/media/File:Yaw_Axis_Corrected.svg
 */
 void obtener_imu()
{
  if (imu.fifoAvailable())                          // Compute euler angles based on most recently read qw, qx, qy, and qz
  {                                                 // results are presented in degrees 
    if (imu.dmpUpdateFifo()==INV_SUCCESS)           // variables roll, pitch, and yaw will be updated
    {
      imu.computeEulerAngles();
      buff[4].f = imu.yaw;                          // 0 to 180 right ; 0 to -180 (360 to 180) left   // rumbo o guiñada
      buff[5].f = imu.pitch;                        // 0 to 90 up ; 0 to -90 90 to 180 ? down         // cabeceo
      buff[6].f = imu.roll;                         // 0 to 180 roll right ; 0 to -180 roll left      // balanceo o alabeo
    }
  }
}


void obtener_gps()
{
  while (gps.available( Serial1 ))
  {
    fix = gps.read();

    if (fix.valid.location)
    {
      buff[7].f = fix.latitude();     // paralelos    -25.326803  // -90 polo sur ; 90 polo norte
      buff[8].f = fix.longitude();    // meridianos   -57.516277  // -180 hemisferio oeste ; 180 hemisferio este
    }

    if (fix.valid.altitude)
    {
      buff[9].f = fix.altitude();     // altitud a nivel del mar? cuanto considera su presion atmosferica? o su referencia
    }
  }
}

void obtener_bat()
{
  SPI.beginTransaction(spiConfigA);           // Inicia SPI con configuracion A
  digitalWrite(SS4, LOW);                     // Habilita esclavo SPI en SSP
  
  SPI.transfer(0x00);                         // Request
  delayMicroseconds(40);
  
  for(uint8_t i=0 ; i<12 ; i++)               // cantidad de caracteres que recibe de SSP
  {
    buff[10+i/4].b[i%4] = SPI.transfer(0x00); // Receive
    delayMicroseconds(40);                    // http://www.gammon.com.au/spi
  }

  digitalWrite(SS4, HIGH);                    // Deshabilita esclavo SPI en SSP
  SPI.endTransaction();                       // Termina transmision SPI
}

/*
#define R_REQ   0    
#define R_POS   1
#define R_DAT   2
#define R_REC   4*/

/*  
 *     uint8_t i = 0;
  uint8_t response;
 *   while ( i < buffLength*4 )
  {
    
    SerialUSB.println(response);
    if ( response == R_POS )
    {
      response = SPI.transfer( i );
      delayMicroseconds(40);
    }
    else if ( response == R_DAT )
    {
      response = SPI.transfer ( buff[i/4].b[i%4] );
      delayMicroseconds(40);
      i++;
    }
    else
    {
      SPI.transfer( R_REQ );
      delaymilliseconds(40);
      response = SPI.transfer( R_REC );
      delaymilliseconds(40);
    }
    
  }*/

void enviar_sst()
{
  SPI.beginTransaction(spiConfigA);           // Inicia SPI con configuracion A
  digitalWrite(SS1, LOW);                     // Habilita esclavo SPI en SST

  for (uint8_t i = 0 ; i < buffLength*4 ; i++)
  {
    SPI.transfer(buff[i/4].b[i%4]);           // Send
    delayMicroseconds(40);
  }
  
  digitalWrite(SS1, HIGH);                    // Deshabilita esclavo SPI en SST
  SPI.endTransaction();                       // Termina transmision SPI
}

void initSD()
{
  while ( !sd.begin( SS2, spiConfigC ) )      // Inicia SPI con configuracion B
  {
    delay(500);
    SerialUSB.println(F("SD init: ERROR"));
    SerialUSB.println();
  }
  SerialUSB.println(F("SD init: OK"));
  digitalWrite(SS2, LOW);                     // Habilita esclavo SPI en SD
}

void openFile()
{
  if (!file.open(fileName, O_WRONLY | O_CREAT | O_AT_END ))     // ( WRITE ONLY | Crear si no Existe | apuntar al final del archivo )
  {
    SerialUSB.println(F("File OPen: ERROR"));
  }
  else
  {
    SerialUSB.println(F("File Open: OK"));
  }
}

void closeFile()
{
  if (!file.sync() || file.getWriteError())
  {
    SerialUSB.println("Write: ERROR");
  }
  else
  {
    SerialUSB.println("Write: OK");
  }
  
  if (!file.close())
  {
    SerialUSB.println(F("Close File: ERROR"));
  }
  else
  {
    SerialUSB.println(F("Close File: OK"));
  }
}

void endSD()
{
  digitalWrite(SS2, HIGH);                    // Deshabilita esclavo SPI en SD
  SPI.endTransaction();                       // Termina transmision SPI
  SerialUSB.println(F("SD end: OK"));
}

void renameFile()                             // renombra el archivo si ya existe con el mismo nombre
{
  initSD();
  
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  
  while (sd.exists(fileName))
  {
    if (fileName[BASE_NAME_SIZE + 1] != '9')
    {
      fileName[BASE_NAME_SIZE + 1]++;
      SerialUSB.print("Rename: OK ->");
      SerialUSB.println(fileName);
    }
    else if (fileName[BASE_NAME_SIZE] != '9')
    {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
      SerialUSB.print("Rename: OK ->");
      SerialUSB.println(fileName);
    }
    else
    {
      SerialUSB.println("Rename: ERROR");
    }
  }
  SerialUSB.println(fileName);

  endSD();
}

void writeHeader()
{
  initSD();
  openFile();
  
  file.print(F("millisegundos"));
  file.print(F(",Temperatura(*C)"));
  file.print(F(",Presion(hPa)"));
  file.print(F(",Altura(m)"));
  file.print(F(",psi(yaw)"));
  file.print(F(",theta(pitch)"));
  file.print(F(",phi(roll)"));
  file.print(F(",latitud"));
  file.print(F(",longitud"));
  file.print(F(",elevacion"));
  file.print(F(",voltaje"));
  file.print(F(",corriente"));
  file.print(F(",potencia"));
  file.println();
  
  closeFile();
  endSD();
}

void logData()      // imprime los 4 BYTES de cada FLOAT separados con espacio (modif: imprime los caracteres)
{
  initSD();
  openFile();

  //file.print("tiempo"); 
  file.print(tiempoCargar);
  for (uint8_t i = 1; i < buffLength ; i++)   // comienza en uno porque carga el tiempo que esta en uint32_t
  {
    file.write(',');
    char _bf[sizeof(String(buff[i].f))];                            // flotante a string
    String(buff[i].f).toCharArray(_bf,sizeof(String(buff[i].f)));   // string a char*
    file.print(F(_bf));                                             // esta funcion no admite String
  }
  file.println();
  
  closeFile();
  endSD();
}
