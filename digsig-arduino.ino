#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

//#define DEBUGGING

#define PIN 6
#define visina_matrice 32 // Duzina jedne matrice
#define sirina_matrice 8 // Sirina jedne matrice
#define br_matrica 4 // Ukupan broj matrica
Adafruit_NeoPixel matrix = Adafruit_NeoPixel((visina_matrice*sirina_matrice)*br_matrica, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoMatrix matrix_2 = Adafruit_NeoMatrix(32, 8, 8,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 3); // IP address, may need to change depending on network
byte gateway[] = { 192, 168, 1, 1 };                   // internet access via router
byte subnet[] = { 255, 255, 255, 0 };                  //subnet mask
EthernetServer server(80);  // create a server at port 80
File webFile;

String HTTP_req;

int trenutniZnak = 1024;
byte sirina_slike = 32;
byte visina_slike = 32;
byte red=0;
int k;
int boja=0;
int piksel = 0;

int x  = matrix_2.width();
String ime = "Digital Signalizer";

void setup()
{
    // disable Ethernet chip and enable SD
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    
    Serial.begin(9600);       // for debugging
    
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");

    // disable SD chip and enable Ethernet
    digitalWrite(4, HIGH);
    digitalWrite(10, LOW);
    
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    delay(100);
    server.begin();           // start to listen for clients
    delay(100);
    matrix.begin();
    matrix.setBrightness(5);
    matrix_2.begin();
    matrix_2.setTextWrap(false);
    matrix_2.setBrightness(30);
    matrix_2.setTextColor(matrix.Color(255, 255, 0));

    PromijeniZnak(trenutniZnak);
}

/*
 * ========================================
 * ================ LOOP ==================
 * ========================================
 */

void loop()
{
  //ProcitajSerijskiPort();
  PromijeniBrightness();
  
  EthernetClient client = server.available();  // try to get client

  if (client) {  // got client?
      boolean currentLineIsBlank = true;
      while (client.connected()) {
          if (client.available()) {   // client data available to read
              char c = client.read(); // read 1 byte (character) from client
              HTTP_req += c;
              
              // print HTTP request character to serial monitor
              //Serial.print(c);
              
              // last line of client request is blank and ends with \n
              // respond to client only after last line received
              if (c == '\n' && currentLineIsBlank) {
                
                  // open requested web page file
                  if (HTTP_req.indexOf("GET / ") > -1
                               || HTTP_req.indexOf("GET /index.htm") > -1) {
                      
                      client.println("HTTP/1.1 200 OK");
                      client.println("Content-Type: text/html");
                      client.println("Connnection: close");
                      client.println();
                      webFile = SD.open("index.htm");        // open web page file
                      if (webFile) {
                          while(webFile.available()) {
                              client.write(webFile.read()); // send web page to client
                          }
                          webFile.close();
                      }
                      
                      client.println("<img id='trenutni-img' src='?load=" + String(trenutniZnak) + ".bmp'>");
                      
                      webFile = SD.open("index2.htm");        // open web page file
                      if (webFile)
                      {
                          while(webFile.available()) {
                              client.write(webFile.read()); // send web page to client
                          }
                          webFile.close();
                      }
                  }


                  
                  else if (HTTP_req.indexOf("GET /?load=") > -1)
                  {
                    int i=11;
                    String tekst = "";
                    while (HTTP_req[i] != ' ')
                    {
                      tekst += HTTP_req[i];
                      i++;
                    }
                    
                    int pozicijaTacke = tekst.indexOf(".");
                    String ekstenzija = tekst.substring(pozicijaTacke+1);
                    String temp;
                    if (ekstenzija.length() > 3)
                      client.println("Ekstenzija ne valja: " + tekst);
                    else
                    {
                      if (pozicijaTacke > 8)
                      {
                        //Serial.println("Staro ime fajla " + tekst);
                        tekst = tekst.substring(0, pozicijaTacke);
                        //Serial.println("Novo ime fajla " + tekst);
                        temp = tekst.substring(0, 6);
                        //Serial.println("Temp novi = " + temp);
                        temp += "~1.";
                        temp += ekstenzija;
                        //Serial.println("Temp = " + temp);
                      }
                      else
                      {
                        temp = tekst;
                      }
                    
                      temp = "media/" + temp;
                      //Serial.println(temp);
                      webFile = SD.open(temp);
                      if (webFile) {
                          client.println("HTTP/1.1 200 OK");
                          client.println();
                      }
                    }
                  }


                  
                  else if (HTTP_req.indexOf("GET /?znak=") > -1)
                  {
                    client.println("HTTP/1.1 200 OK");
                    client.println();
                    int i=11;
                    String tekst = "";
                    while (HTTP_req[i] != ' ')
                    {
                      tekst += HTTP_req[i];
                      i++;
                    }

                    ime = HTTP_req.substring(HTTP_req.indexOf("&ime=") + 5, HTTP_req.indexOf("&nocache="));
                    ime.replace("%20", " "); // razmak
                    ime.replace("%C4%8D", "^"); // č
                    ime.replace("%C4%87", "_"); // ć
                    ime.replace("%C5%A1", "]"); // š
                    ime.replace("%C5%BE", "["); // ž
                    matrix_2.fillScreen(10);
                    matrix_2.setCursor(0, 0);
                    matrix_2.print(F("******"));
                    matrix_2.show();
                    x=matrix_2.width();
                    
                    trenutniZnak = tekst.toInt();
                    if (PromijeniZnak(trenutniZnak))
                    {
                      client.println("Znak uspjesno promijenjen! Novi znak je ID=" + String(trenutniZnak));
                    }
                    else
                      client.println("Znak ID=" + String(trenutniZnak) + " nije bilo moguce ucitati!");
                  }


                  
                  if (webFile) {
                      while(webFile.available()) {
                          client.write(webFile.read()); // send web page to client
                      }
                      webFile.close();
                  }
                  HTTP_req = "";
                  break;
              }


              
              // every line of text received from the client ends with \r\n
              if (c == '\n') {
                  // last character on line of received text
                  // starting new line with next character read
                  currentLineIsBlank = true;
              } 
              else if (c != '\r') {
                  // a text character was received from client
                  currentLineIsBlank = false;
              }
          } // end if (client.available())
      } // end while (client.connected())
      delay(1);      // give the web browser time to receive the data
      client.stop(); // close the connection
  } // end if (client)
  matrix.show();
  
  matrix_2.fillScreen(10);
  matrix_2.setCursor(x, 0);
  matrix_2.print(ime);
  int dokle = ime.length()*7;
  if(--x < -dokle) {
    x = matrix_2.width();
  }
  matrix_2.show();
  delay(10);
}

int bright = 0;
void PromijeniBrightness()
{

  bright = analogRead(0);
  switch(bright)
  {
    case 64 ... 127:
      matrix.setBrightness(10);
      #if defined DEBUGGING
        Serial.println("10");
      #endif
      break;
    case 128 ... 255:
      matrix.setBrightness(20);
      #if defined DEBUGGING
        Serial.println("20");
      #endif
      break;
    case 256 ... 511:
      matrix.setBrightness(30);
      #if defined DEBUGGING
        Serial.println("30");
      #endif
      break;
    case 512 ... 767:
      matrix.setBrightness(40);
      #if defined DEBUGGING
        Serial.println("40");
      #endif
      break;
    case 768 ... 1023:
      matrix.setBrightness(50);
      #if defined DEBUGGING
        Serial.println("50");
      #endif
      break;
    default:
      matrix.setBrightness(5);
      #if defined DEBUGGING
        Serial.println("5");
      #endif
  }
  #if defined DEBUGGING
    Serial.println(bright);
  #endif
  
  /*
  bright = analogRead(0) / 8;
  matrix.setBrightness(bright);
  */
  
}


void ProcitajSerijskiPort()
{
  if (Serial.available() > 0)
  {
    String serialRead;
    while (Serial.available() > 0)
    {
      serialRead += Serial.read();
    }
    PromijeniZnak(serialRead.toInt());
    Serial.println(serialRead);
  }
}

bool PromijeniZnak(int znak)
{
  String lokacija = "znakovi/" + String(znak) + ".txt";
  //Serial.println(lokacija);
  webFile = SD.open(lokacija);
  int broj = 0;
  if (webFile) {
      //Serial.println("Ucitan znak " + lokacija);
      String cifre;
      byte bojePiksela[3];
      
      for (int brojacI = 0; brojacI < visina_slike*sirina_slike; brojacI++)
      {
        for (int brojacJ = 0; brojacJ < 3; brojacJ++)
        {
          cifre = "";
          while (webFile.peek() != 44)
          {
            cifre += char(webFile.read());
            //Serial.println("Ucitane cifre = " + cifre + ", broj = " + String(broj));
          }
          webFile.read();
          webFile.read();
          
          bojePiksela[brojacJ] = byte(cifre.toInt());
          //Serial.println("Broj = " + String(broj) + ", slika[broj] = " + String(slika[broj]));
        }
        matrix.setPixelColor(brojacI, matrix.Color(bojePiksela[0], bojePiksela[1], bojePiksela[2]));
      }
      matrix.show();
      return true;
  }
  else
    return false;
}
