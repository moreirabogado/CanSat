//________________________________________________________________________________________
import processing.serial.*;                                                               // libreria para comunicacion serial
import grafica.*;                                                                         // libreria para los graficos x-y

import de.fhpotsdam.unfolding.*;                                                          // librerias para el mapa. Referencia: Prof. Dr. Till Nagel. Mail: t.nagel@hs-mannheim.de
import de.fhpotsdam.unfolding.geo.*;
import de.fhpotsdam.unfolding.utils.*;
import de.fhpotsdam.unfolding.providers.*;
import de.fhpotsdam.unfolding.mapdisplay.MapDisplayFactory;

UnfoldingMap map;
Location locationFiuna = new Location(-25.329522, -57.522036);                            // comentar despues 
Location[] locations = new Location[6000];                                                // vector donde se guardaran las posiciones del GPS
int i = 0;                                                                                // contador para guardar las distintas posiciones del gps en un vector
int j = 0;                                                                                // contador para graficar los puntos sucesivos que fueron guardados en el vector


Serial myPort;  
//int cc=59;                                                                                // Posicion del caracter '\n' de los datos recibidos por serial  
String inputData; 
float address, bytes, time, temp, pres, alti, yaw, pitch, roll, lati, longi, elev, volt, curr, pote;
float bate;
float curr_ant=0;

public GPlot plot1, plot2, plot3, plot4;
public GPointsArray polygonPoints;
  
GPointsArray ponitsTemp = new GPointsArray(6000);                                            // array de puntos para los graficos (prueba)
GPointsArray ponitsPres = new GPointsArray(6000);
GPointsArray ponitsAlti = new GPointsArray(6000);
//________________________________________________________________________________________
public void setup() {
  
  size(1920, 1080, P3D);
  noStroke(); 
  
  printArray(Serial.list());
  myPort = new Serial(this, Serial.list()[0], 115200);   // Se abre el puerto que se esta usando con el baud rate especificado
  myPort.write("AT+PARAMETER=7,9,4,5");
  myPort.bufferUntil('\n'); 

  // Configuracion para el primer grafico
  plot1 = new GPlot(this);
  plot1.setPos(0, 120);
  plot1.setDim(500, 210);
  plot1.getXAxis().getAxisLabel().setText("Tiempo(s)");
  plot1.getYAxis().getAxisLabel().setText("Temperatura(°C)");
  plot1.setLineColor(color(150, 150, 255));

  // Configuracion para el segundo grafico
  plot2 = new GPlot(this);
  plot2.setPos(650, 120);
  plot2.setDim(500, 210);
  plot2.getXAxis().getAxisLabel().setText("Tiempo(s)");
  plot2.getYAxis().getAxisLabel().setText("Presion(hPa)");
  plot2.setLineColor(color(150, 150, 255));

  // Configuracion para el tercer grafico
  plot3 = new GPlot(this);
  plot3.setPos(1300, 120);
  plot3.setDim(500, 210);
  plot3.getXAxis().getAxisLabel().setText("Tiempo(s)");
  plot3.getYAxis().getAxisLabel().setText("Altitud(m)");
  plot3.setLineColor(color(150, 150, 255));

  // Configuracion de los eventos por mouse (zooming, panning, etc)
  plot1.activatePanning();
  plot1.activateZooming(1.1, CENTER, CENTER);
  plot1.activatePointLabels();
  plot2.activatePanning();
  plot2.activateZooming(1.1, CENTER, CENTER);
  plot2.activatePointLabels();
  plot3.activatePanning();
  plot3.activateZooming(1.1, CENTER, CENTER);
  plot3.activatePointLabels();
  
  // Configuracion para mostrar el mapa
  map = new UnfoldingMap(this, "map", 100, 500, 450, 450, true, false, new Microsoft.AerialProvider()); //new Google.GoogleMapProvider(): este mapa mejor, pero me funciono por un momento
  map.zoomToLevel(10); 
  map.panTo(locationFiuna);
  map.setZoomRange(3, 19); // prevent zooming too far out
  map.setPanningRestriction(locationFiuna, 50);
  MapUtils.createDefaultEventDispatcher(this, map);
  
  volt = 8.4;
  bate = 41.666666*volt-250;
}
//________________________________________________________________________________________
void serialEvent (Serial myPort)
{
  if ( myPort.available() > 0){ 
    inputData = myPort.readStringUntil('\n');
    println(inputData);
    if (inputData!=null && 1 == inputData.indexOf('R')){
      String [] list = splitTokens(inputData,",");
      //address = float(list[0]);
      //bytes = float(list[1]);
      time = float(list[2]); 
      temp = float(list[3]); //C
      pres = float(list[4]); //Pa
      alti = float(list[5]);
      yaw = float(list[6]);
      pitch = float(list[7]);
      roll = float(list[8]);
      lati = float(list[9])/10000;
      longi = float(list[10])/10000;
      elev = float(list[11]);
      volt = float(list[12]);
      curr = float(list[13]);// mA
      pote = float(list[14]);// mW
      
     //println(time,",",temp,",",pres,",",alti,",",yaw,",",pitch,",",roll,",",lati,",",longi,",",elev,",",volt,",",curr,",",pote);
  
    }
    
  }
  
}
//________________________________________________________________________________________
public void draw() {
  background(255);
  
  textSize(50); 
  fill(0, 102, 153);
  text("Estación Terrena - educanSAT", 600, 60);
  
  // Mapa
  textSize(20); 
  fill(0, 102, 153);
  text("Posición", 280, 480);
  map.draw();
  locations[i] = new Location(lati, longi);          
  i++;

  for (j=0; j<i; j++){                                                                      // se carga una posicion del vector en cada iteracion
    ScreenPosition pos = map.getScreenPosition(locations[j]);
    if (j==0){                                                                              // la posicion inicial se pinta en rojo y las demas en verde
      fill(255, 0, 0, 255);
    }else{
      fill(0, 255, 0, 255);
    }
    ellipse(pos.x, pos.y, 10, 10);
  }
  
  // Bateria
  if (abs(curr-curr_ant)<=curr)
  {
    bate = bate - curr*4273504e-12;
    curr_ant=curr;
  }
  if (bate > 41.666666*volt-250 && volt<8.4 && volt>6.0 )
  {
  bate = 41.666666*volt-250;
  }
  
  textSize(20); 
  fill(0, 102, 153);
  text("Batería", 1580, 480);
  fill(50);
  text(bate+"%", 1560, 540);
  stroke(126);                                                                              // cuadro de nivel de bateria
  line(1510,550 , 1710,550);
  line(1710,550 , 1710,600);
  line(1710,600 , 1510,600);
  line(1510,600 , 1510,550);
  fill(153);                                                                                
  quad(1510,550 , (1510+2*bate),550 , (1510+2*bate),600 , 1510,600);                        // barra de nivel de bateria 
  textSize(24); 
  fill(50);
  text("Voltaje(V):       " + volt, 1500, 700);
  text("Corriene(mA):  " + curr, 1500, 750);
  text("Potencia(mW):  " + pote, 1500, 800);
  
  //
  ponitsTemp.add(i, temp, "point " + i);                                                      // se agrega el valor actual de temperatura
  ponitsPres.add(i, pres, "point " + i);                                                      // se agrega el valor actual de presion
  ponitsAlti.add(i, alti, "point " + i);                                                      // se agrega el valor actual de altura
  
  plot1.setPoints(ponitsTemp);                                                                   
  plot2.setPoints(ponitsPres);
  plot3.setPoints(ponitsAlti);

  // Dibujar el primer grafico
  textSize(20); 
  fill(0, 102, 153);
  text("Temperatura", 250, 115);
  plot1.beginDraw();
  plot1.drawBackground();
  plot1.drawBox();
  plot1.drawXAxis();
  plot1.drawYAxis();
  plot1.drawTopAxis();
  plot1.drawRightAxis();
  plot1.drawTitle();
  plot2.drawGridLines(GPlot.BOTH);
  plot1.drawFilledContours(GPlot.HORIZONTAL, 0.05);
  plot1.drawLabels();
  plot1.endDraw();

  // Dibujar el segundo grafico 
  textSize(20); 
  fill(0, 102, 153);
  text("Presión", 930, 115);
  plot2.beginDraw();
  plot2.drawBackground();
  plot2.drawBox();
  plot2.drawXAxis();
  plot2.drawYAxis();
  plot2.drawTopAxis();
  plot2.drawRightAxis();
  plot2.drawTitle();
  plot2.drawGridLines(GPlot.BOTH);
  plot2.drawFilledContours(GPlot.HORIZONTAL, 0.05);
  plot2.drawLabels();
  plot2.endDraw();  
  //plot2.drawLines(); (para que se grafique linea nomas, no pintado)

  // Dibujar el tercer grafico
  textSize(20); 
  fill(0, 102, 153);
  text("Altitud", 1580, 115);
  plot3.beginDraw();
  plot3.drawBackground();
  plot3.drawBox();
  plot3.drawXAxis();
  plot3.drawYAxis();
  plot3.drawTopAxis();
  plot3.drawRightAxis();
  plot3.drawTitle();
  plot3.drawGridLines(GPlot.BOTH);
  plot3.drawFilledContours(GPlot.HORIZONTAL, 0.05);
  plot3.drawLabels();
  plot3.endDraw();
  
  // Dibujar el cilindro
  textSize(20); 
  fill(0, 102, 153);
  text("Orientación", 905, 480);
  fill(153);                                                                                // el plano en la base del cilindro
  quad((width/2)-100, (height/2)+370, (width/2)+100, (height/2)+370, (width/2)+250, (height/2)+420, (width/2)-250, (height/2)+420);
  lights();
  translate(width/2, height/2+50);
  rotateZ(-pitch*2*3.14/360);
  rotateX(roll*2*3.14/360);
  rotateY(yaw*2*3.14/360);
  noStroke();
  fill(100, 100, 100);
  dibujarCilindro(80, 80, 260, 64);                                                         // se llama a la funcion dibujar cilindro
}
//________________________________________________________________________________________
void dibujarCilindro(float topRadius, float bottomRadius, float tall, int sides)            // Referencia: http://laurence.com.ar/artes/comun/Processing_un_lenguaje_al%20alcance_de_todos.pdf
{
 float angle = 0;
 float angleIncrement = TWO_PI / sides;
 beginShape(QUAD_STRIP);
 for (int i = 0; i < sides + 1; ++i) {
   vertex(topRadius*cos(angle), 0, topRadius*sin(angle));
   vertex(bottomRadius*cos(angle), tall, bottomRadius*sin(angle));
   angle += angleIncrement;
 }
 endShape();
 //Si no es un cono, dibujar el circulo arriba
 if (topRadius != 0) {
   angle = 0;
   beginShape(TRIANGLE_FAN);
   //Punto Central
   vertex(0, 0, 0);
   for (int i = 0; i < sides + 1; i++) {
     vertex(topRadius * cos(angle), 0, topRadius *
     sin(angle));
     angle += angleIncrement;
   }
   endShape();
  }
  //Si no es un cono, dibujar el circulo abajo
  if (bottomRadius != 0) {
    angle = 0;
     beginShape(TRIANGLE_FAN);
     // Center point
     vertex(0, tall, 0);
     for (int i = 0; i < sides+1; i++) {
       vertex(bottomRadius * cos(angle), tall, bottomRadius *
       sin(angle));
       angle += angleIncrement;
     }
    endShape();
  }
  
  delay(50);
}
//________________________________________________________________________________________
