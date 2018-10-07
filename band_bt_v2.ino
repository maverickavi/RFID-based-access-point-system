
#include <SoftwareSerial.h>
SoftwareSerial BTserial(6, A0); // RX | TX
// Connect the HC-05 TX to Arduino pin 2 RX. 
// Connect the HC-05 RX to Arduino pin 3 TX through a voltage divider.
 
char c = ' ';



#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <MFRC522.h>
#include <SPI.h>
#include <SD.h>

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
#define TFT_CS  3  // Chip select line for TFT display
#define TFT_RST  9  // Reset line for TFT (or see below...)
#define TFT_DC   8  // Data/command line for TFT

#define SD_CS    4  // Chip select line for SD card
#define SS_PIN 2
#define RST_PIN 5

 int flag = 0;
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[3]; 
//Use this reset pin for the shield!
//#define TFT_RST  0  // you can also connect this to the Arduino reset!
//#define 522_CS 2 //Chip select line for NFC Reader
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


#define  BLACK   0x0000

void setup(void) {
  Serial.begin(9600);
  pinMode(10, OUTPUT);
  pinMode(7, OUTPUT);
  //pinMode(TFT_CS, OUTPUT);
  digitalWrite(7,HIGH);

  tft.initR(INITR_BLACKTAB);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }
  Serial.println("OK!");

  // change the name here!
//  bmpDraw("1.bmp", 0, 0);
tft.fillScreen(BLACK);
  // wait 5 seconds
 
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

//  Serial.println(F("This code scan the MIFARE Classsic NUID."));
//  Serial.print(F("Using the following key:"));
 // printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

    BTserial.begin(38400);
    
    Serial.println(tft.getRotation(), DEC);

    tft.setRotation(tft.getRotation()+3);
 
}

void loop() {
  
  rfid.PCD_Init();
  if(flag!=0) btEvent();
  if ( ! rfid.PICC_IsNewCardPresent()&& (flag==1||flag==2||flag==3)){
    flag = 0;
//    bmpDraw("1.bmp", 0, 0);
tft.fillScreen(BLACK);
    return;
  }
  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial()){
    //Serial.println("hhh");
    return;
  }
  
//  Serial.println("out");
//  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  //Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
//    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if(rfid.uid.uidByte[0]==162&& (flag==0||flag==2||flag==3)){
      bmpDraw("1.bmp", 0, 0);
      flag = 1;
    }
  else if(rfid.uid.uidByte[0]==96 && (flag==0||flag==1||flag==3)){
      bmpDraw("2.bmp", 0, 0);
      flag = 2;
    }
  else if(rfid.uid.uidByte[0]==22 && (flag==0||flag==1||flag==2)){
      bmpDraw("3.bmp", 0, 0);
      flag = 3;
    }
  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
// uncomment these lines to draw bitmaps in different locations/rotations!
/*
  tft.fillScreen(ST7735_BLACK); // Clear display
  for(uint8_t i=0; i<4; i++)    // Draw 4 parrots
    bmpDraw("parrot.bmp", tft.width() / 4 * i, tft.height() / 4 * i);
  delay(1000);
  tft.setRotation(tft.getRotation() + 1); // Inc rotation 90 degrees
*/

}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20


void bmpDraw(char *filename, uint8_t x, uint8_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print("File not found");
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print("Image size: ");
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.Color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print("Loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println("BMP format not recognized.");
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}


void btEvent(){

  if (BTserial.available())
    {  
        c = BTserial.read();
        Serial.write(c);
        if(c=='c'){
          if(flag==1)bmpDraw("5.bmp", 0, 0);
          else if(flag==2)bmpDraw("7.bmp", 0, 0);
          delay(5000);
          if(flag==1)bmpDraw("1.bmp", 0, 0);
          if(flag==2)bmpDraw("2.bmp", 0, 0);
          if(flag==3)bmpDraw("3.bmp", 0, 0);
        }else if(c=='b'){
          if(flag==2)bmpDraw("6.bmp", 0, 0);
          else if(flag==3)bmpDraw("8.bmp", 0, 0);
          delay(5000);
          if(flag==1)bmpDraw("1.bmp", 0, 0);
          if(flag==2)bmpDraw("2.bmp", 0, 0);
          if(flag==3)bmpDraw("3.bmp", 0, 0);
        }else if(c=='a'){
          if(flag==1)bmpDraw("4.bmp", 0, 0);
          else if(flag==3)bmpDraw("9.bmp", 0, 0);
          delay(5000);
          if(flag==1)bmpDraw("1.bmp", 0, 0);
          if(flag==2)bmpDraw("2.bmp", 0, 0);
          if(flag==3)bmpDraw("3.bmp", 0, 0);
        }
        else if(c=='d'){
          if(flag==1)bmpDraw("10.bmp", 0, 0);
          else if(flag==2)bmpDraw("11.bmp", 0, 0);
          else if(flag==3)bmpDraw("12.bmp", 0, 0);
          delay(5000);
          if(flag==1)bmpDraw("1.bmp", 0, 0);
          if(flag==2)bmpDraw("2.bmp", 0, 0);
          if(flag==3)bmpDraw("3.bmp", 0, 0);
        }
        //delay(2000);
        c = ' ';
    }
  
}

