#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define SDA_PIN 21
#define SCL_PIN 22
#define RELAY_PIN 26
#define DHT_PIN 4


#define LOW_PLUS 14
#define LOW_MINUS 27
#define HIGH_PLUS 13
#define HIGH_MINUS 12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Preferences prefs;

int humidity = 0;
int temperature = 0; 


float lowThreshold;
float oldLowThreshold;
float highThreshold;
float oldHighThreshold;

bool relayState = false;
bool saveIt = false;
int counter = 80;

void setup()
{
//Serial.begin(9600);//115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
      for(;;) delay(1000);
    }
    pinMode(LOW_PLUS, INPUT_PULLUP);
    pinMode(LOW_MINUS, INPUT_PULLUP);
    pinMode(HIGH_PLUS, INPUT_PULLUP);
    pinMode(HIGH_MINUS, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    
    digitalWrite(RELAY_PIN, HIGH);
    pinMode(DHT_PIN, INPUT_PULLUP);
    
    prefs.begin("thresholds"); 
    //prefs.clear();
    //prefs.remove("low");
    //prefs.remove("high");     
    lowThreshold = prefs.getFloat("low", 50.0);
    highThreshold = prefs.getFloat("high", 60.0);   
    oldLowThreshold = lowThreshold;
    oldHighThreshold = highThreshold;
    int hall_value = hallRead(); 
   float cpu_temp = temperatureRead();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Display hardware-defined memory sizes
//  display.println("ESP32 DevKit V1");
  display.print("CPU: ");
  display.print(ESP.getCpuFreqMHz());
  display.println(" MHz");
  display.print("Flash: ");
  display.print(ESP.getFlashChipSize() / (1024 * 1024));
  display.println(" MB");
  display.print("Hall Sensor: ");
  display.print(hall_value);
  display.println(" uT");
  display.print("CPU Temp: ");
  display.print(cpu_temp, 1); 
  display.println(" C");
  display.println("Booting...");
  display.display();
}

void readDHT()
{
    uint8_t data[5] = {0, 0, 0, 0, 0};
    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);
    digitalWrite(DHT_PIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHT_PIN, INPUT_PULLUP);
    while (digitalRead(DHT_PIN) == HIGH); 
    while (digitalRead(DHT_PIN) == LOW); 
    while (digitalRead(DHT_PIN) == HIGH); 
    for(int i = 0; i < 40; i++)
    { 
        while (digitalRead(DHT_PIN) == LOW); 
        unsigned long start = micros(); 
        while (digitalRead(DHT_PIN) == HIGH); 
        if(micros() - start > 40) data[i / 8] |= (1 << (7 - (i % 8))); 
    }
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    { 
        humidity = data[0]; 
        temperature = data[2]; 
    } else { humidity = 0; temperature = 0; }
}


void saveMyPrefs()
{
    char info[16];
    
    if(oldLowThreshold != lowThreshold)
    { 
        oldLowThreshold = lowThreshold;
        prefs.putFloat("low", lowThreshold);
    }
    if(oldHighThreshold != highThreshold)
    { 
        oldHighThreshold = highThreshold;
        prefs.putFloat("high", highThreshold);
    }
    sprintf(info, "%02.0f,%02.0f", lowThreshold, highThreshold); 
    
    display.clearDisplay();
display.fillRect(0, 0, 127, 63, SSD1306_WHITE);
    display.setTextColor(BLACK,WHITE);
    display.setTextSize(4);
    display.setCursor(4, 20);
    display.println(info);
    display.display();
}

void updateDisplayInfo()
{
    char h[3];
    char t[3];
    char thL[3];
    char thH[3];
    char cpu[5];
float cpu_temp =  temperatureRead();
    display.clearDisplay();
    sprintf(h, "%02d", humidity);
    sprintf(t, "%02d", temperature);
    sprintf(thL, "%02.0f", lowThreshold);
    sprintf(thH, "%02.0f", highThreshold);
    sprintf(cpu, "%02.1f", cpu_temp);
    display.setTextColor(WHITE);
    display.setTextSize(3);
//
    display.setCursor(0, 0);
    display.print("H:");
    display.setCursor(38,0);
    display.println(h);
    display.setCursor(82,0);
    display.print("%");
//
    display.setCursor(0, 24);
    display.print("T:");
    display.setCursor(38,24);
    display.println(t);
    display.setCursor(74,20);
    display.setTextSize(2);
    display.print("o");
   display.setCursor(85,24);
    display.setTextSize(3);
    display.print("C");
//
    display.setTextSize(2);
    display.setCursor(2, 49);
    display.drawLine(0, 48, 25, 48, SSD1306_WHITE); 
    display.fillRect(0, 48, 2, 16, SSD1306_WHITE);
    display.setTextColor(BLACK,WHITE);
    display.print(thL);
    display.setTextColor(WHITE);
    display.setCursor(26, 53);
    display.print("~");
    display.setTextColor(BLACK,WHITE);
    display.fillRect(36, 48, 2, 16, SSD1306_WHITE);
    display.drawLine(36, 48, 61, 48, SSD1306_WHITE); 
    display.setCursor(38, 49);
    display.print(thH);
    display.setTextColor(WHITE);
    display.setCursor(80, 49);
    display.print(cpu);
    display.display();
}
void loop()
{
    delay(200);
   
    int sum = 0;
    if(digitalRead(LOW_PLUS)  == LOW ) { sum |= 0x1; }
    if(digitalRead(LOW_MINUS) == LOW ) { sum |= 0x2; }
    if(digitalRead(HIGH_PLUS) == LOW ) { sum |= 0x4; }
    if(digitalRead(HIGH_MINUS) == LOW) { sum |= 0x8; }
   
    if(sum == 0) //no inputs
    {
        counter++;
        if(counter > 100)
        {
            counter = 0;
            if(saveIt == true)
            {
                saveMyPrefs();
                saveIt = false;
                delay(4000);
            }
            else
            {
                readDHT();
                if(humidity > highThreshold && !relayState)
                { 
                    digitalWrite(RELAY_PIN, LOW); 
                    relayState = true; 
                } 
                else if(humidity < lowThreshold && relayState)
                { 
                    digitalWrite(RELAY_PIN, HIGH); 
                    relayState = false; 
                }
           }
           updateDisplayInfo(); 
        }
    }
    else //handle inputs
    {
        counter = 80; //reset it,let a small amount of time before saving it
        switch(sum)
        {
           case 1: //LOW_PLUS
           {
               if(lowThreshold < highThreshold)
               {
                   lowThreshold += 1.0;
                   saveIt = true;
               }
           }break;
           case 2: //LOW_MINUS
           {
               if(lowThreshold > 0)
               { 
                   lowThreshold -= 1.0;
                   saveIt = true;
               }
           }break;
           case 4: //HIGH_PLUS
           {
               if(highThreshold < 100)
               {
                   highThreshold += 1.0; 
                   saveIt = true;
               }
           }break;
           case 8: //HIGH_MINUS
           {
               if(highThreshold > lowThreshold)
               {
                   highThreshold -= 1.0;
                   saveIt = true;
               }
           
           }break;
           //any combination,multiple button pressed
           // LOW_PLUS   |= 0x1
           // LOW_MINUS  |= 0x2
           // HIGH_PLUS  |= 0x4
           // HIGH_MINUS |= 0x8
           // EXAMPLE
           //case 5: //LOW_PLUS + HIGH_PLUS
           //case 6: //LOW_MINUS + HIGH_PLUS
           //case 7: //LOW_MINUS + HIGH_PLUS + LOW_PLUS
           default:
             break;
        }
        updateDisplayInfo();
    }
}

