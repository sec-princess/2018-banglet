/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/*********************************************************************
 * Thank you, Adafruit Industries for the examples!
 * 
 * This is the sketch that makes the Banglet work
 * It came from https://github.com/pdxbadgers/2018-banglet/
 * There are other sketches there so if you would like to flash the
 * other ones to see what they do or try to hack on your banglet, feel
 * free to clone the project and follow the instructions
 * 
 * This code is released under the MIT license and is provided as is
 * with no warranties or guarantees or assurances, so use at your own
 * risk. That means, don't blame us if your banglet catches fire.
 * 
 * This is meant to be used with either the Adafruit Bluefruit LE app
 * or the Serial Bluetooth Terminal. It takes advantage of Nordic's
 * proprietary UART service, allowing you to interact with the banglet
 * using a 'shell'. You can find these apps at the Android play store
 * (I am not sure about iOS but I am guessing you can find an equivalent)
 * 
 * Once you connect to the banglet, you can list all the commands by sending
 * '/list' through the terminal.
 * 
 * Happy Hacking!
 ************************************************************************/

#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>

/*
 * Global
 */

// BLE Service
BLEUart bleuart;


// List of commands
// Add command name here and make sure the MAX_COMM value matches the number of commands
const int MAX_COMM_LEN = 10;
const int MAX_COMM = 5;
const char commands[MAX_COMM][MAX_COMM_LEN] = {"list", "rainbow", "patriot", "off", "scan"};


// BT device scan
uint seen=0;
const byte MAX_MACS = 100;
uint8_t* seen_macs[MAX_MACS]; 


// neopixel
#define N_LEDS 10
#define PIN    A0
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);


byte neopix_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


/*
 * Initial setup
 */
void setup()
{
  // setup serial
  Serial.begin(115200);
  Serial.println("Welcome to your Banglet!");
  Serial.println("---------------------------\n");

  // Config the peripheral connection with maximum bandwidth 
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName(strcat("Banglet-", getMcuUniqueID()));
  Bluefruit.setConnectCallback(connect_callback);
  Bluefruit.setDisconnectCallback(disconnect_callback);

  // Start Central Scan
  Bluefruit.setConnLedInterval(250);
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.start(0);

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Set up and start advertising
  startAdv();

  Serial.println("Please use the Adafruit Bluefruit app or the Serial Bluetooth Terminal app to connect to the Banglet");
  Serial.println("Once connected, send /list for a list of commands you can send");
}

// advertise peripheral BLEUART service
void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

// find mac addresses
int find_mac(uint8_t* mac)
{
  for(int i=0;i<seen;++i)
  {
    if(0==memcmp(mac,seen_macs[i],6))
    {
      return i;
    }
  }
  seen_macs[seen]=mac;
  seen++;
}

// scan callback function which prints out mac
// addresses in a readable way
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  uint8_t* new_thing = new uint8_t[6]; 
  memcpy(new_thing,report->peer_addr.addr,6);

  int thing = find_mac(new_thing);
  
  Serial.println();
  for ( int i = 0 ; i < seen; ++i)
  {
    Serial.printBufferReverse(seen_macs[i], 6, ':');
    Serial.println();
  }
  
  Serial.printf("device: %d  ",thing);
  Serial.printf("num seen: %d ",seen);

  // Check if advertising contain BleUart service
  if ( Bluefruit.Scanner.checkReportForUuid(report, BLEUART_UUID_SERVICE) )
  {
    Serial.println("BLE UART service detected");
  }

  Serial.println();
}


/*
 * Loop function
 * Read bytes send from UART app and do stuff with it
 */
void loop()
{
  // Forward data from HW Serial to BLEUART
  // This doesn't actually work but seems to be needed to initialize the buffer correctly
  while (Serial.available())
  {
    // Delay to wait for enough input, since we have a limited transmission buffer
    delay(2);

    uint8_t buf[64];
    int count = Serial.readBytes(buf, sizeof(buf));
    bleuart.write( buf, count );
  }

  // Forward from BLEUART to HW Serial
  while ( bleuart.available() )
  {
    uint8_t rx_buf[64];
    int rx_size = bleuart.read(rx_buf, sizeof(rx_buf));
    parseCommand(rx_buf, rx_size);
  }

  // Request CPU to enter low-power mode until an event/interrupt occurs
  waitForEvent();
}

// UART service connect callback
void connect_callback(uint16_t conn_handle)
{
  char central_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
  //listCommands();
}

// UART service disconnect callback
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.println("Disconnected");
}

/*
 * Main Banglet functions that parses the send messages and does things with it
 */
void parseCommand(uint8_t *buf, int bufsize)
{
  //for(int i=0; i<bufsize; i++){
  //  Serial.write(buf[i]); 
  //}
  if(buf[0] == '/'){
    int index = getCommand(buf);
    if(index != -1)
    {
      doOption(index);
    }else{
      bleuart.write("Invalid option\n");
    }
  }else{
    bleuart.write("Send /list for list of commands. All commands start with /\n");
  }
}

// Return the index of the commands user entered
// If not there return -1
int getCommand(uint8_t *buf)
{
  int buf_end = 1;
  while(buf[buf_end] != '\n')
  {
   // Serial.println((char)buf[buf_end]);
    buf_end++;
  }
  // for some reason there are some undetermined characters at the end
  // so shortend the length by this amount which is was found out through
  // experimentation - yes I don't know what is going on here
  buf_end = buf_end - 2;
  //Serial.println(buf_end);
  
  //go through each command
  for(int i=0; i<MAX_COMM; i++)
  {
    //Serial.println(commands[i]);
    //Serial.println((char *)buf);
    //check if the buffer command is the same
    if(strncmp(commands[i], (char *)++buf, buf_end) == 0)
    {
      return i;
    }else{
      --buf;
    }
  }

  return -1;
}

// Switch between functions
void doOption(int index)
{
  if(index == 0) listCommands();
  if(index == 1) rainbow();
  if(index == 2) patriot();
  if(index == 3) off();
  if(index == 4) scan();
}


/*
 * All the things Banglet can do
 * You can either hack these or add your function here
 */

// list all commands
void listCommands()
{
  bleuart.write("List of commands:\n");
  for(int i=0; i<MAX_COMM; i++)
  {
    bleuart.write(commands[i]);
    bleuart.write('\n');
  }
}

// red, white and blue lights
void patriot()
{
  
  bleuart.write("USA! USA!\n");
  // light up strip with red, white and blue
  int counter=0;
  uint32_t red=strip.Color(255,0,0);
  uint32_t white=strip.Color(255,255,255);
  uint32_t blue=strip.Color(0,0,255);
  bleuart.flush();
  while(bleuart.peek() == -1)
  {
    for (int i = 0; i < strip.numPixels(); i+=3)
    {
      strip.setPixelColor((i+counter)%15,red);
      strip.setPixelColor((i+counter+1)%15,white);
      strip.setPixelColor((i+counter+2)%15,blue);
    }

    counter = (counter+1)%3;
  
    strip.show();
    delay(200);
  }

}

// rainbow lights
void rainbow()
{
  bleuart.write("Ooo! Rainbow!\n");
  bleuart.flush();
  while(bleuart.peek() == -1) rainbow(10, 1);
}

void rainbow(uint8_t wait, int rainbowLoops)
{
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for(int k = 0 ; k < rainbowLoops ; k ++){
    
    for(int j=0; j<256; j++) { // 5 cycles of all colors on wheel

      for(int i=0; i< strip.numPixels(); i++) {

        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);

        redVal = red(wheelVal);
        greenVal = green(wheelVal);
        blueVal = blue(wheelVal);

        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );

      }

        strip.show();
        delay(wait);
    }
  
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}

// turn the lights off if they get annoying
void off()
{
  bleuart.write("Turning off the lights\n");
  for(int i=0; i<strip.numPixels(); i++)
  {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
  
}

// scan close by BT devices
void scan()
{
  bleuart.write("Sneaky!\n");
  bleuart.flush();
  while(bleuart.peek() == -1) btscan();
}

void btscan()
{
  
  delay(1000);
  
  uint i = 0;

  // light up the LEDs
  int first=0;
  if(seen>12)first=seen-12;
  
  for ( i = first ; i < seen; ++i)
  {
    //uint32_t color = strip.Color(seen_addrs[i][0],seen_addrs[i][1],seen_addrs[i][2]);
    uint32_t color = strip.Color(seen_macs[i][5],seen_macs[i][4],seen_macs[i][3]);
    strip.setPixelColor(i-first, color);
  }

  strip.show();
}