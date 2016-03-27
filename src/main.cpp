#include <Wire.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Adafruit_SSD1306.h>
#include <NeoPixelBus.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

// -------------------- CONFIGURATION --------------------

#define JSON_BUFF_SIZE 512

#define WIFI_TIMEOUT 30000

#define BADGE_TEAM_DEFAULT 1
#define BADGE_ID_DEFAULT 1
#define BADGE_TEAM_MAX 20
#define BADGE_ID_MAX 99

const char STR_WAIT      [] PROGMEM = "Please wait...";
const char STR_OTA_UPDATE[] PROGMEM = "OTA Update";
const char STR_OTA_DONE  [] PROGMEM = "Done. Rebooting...";

const char STR_FILE_WIFI   [] = "/wifi.json";
const char STR_FILE_CONF   [] = "/conf.json";
const char STR_NAME_DEFAULT[] = "Team 1-1";

const RgbColor COLOR_OFF     (  0,   0,   0);
const RgbColor COLOR_DEFAULT (  0,   0, 128);
const RgbColor COLOR_ECHO    ( 20, 150,   0);

const uint16_t  NET_PORT   = 11337;
const IPAddress NET_SERVER ( 10,  13,  37, 100);
const IPAddress NET_GROUP  (239,  13,  37,   1);

// -------------------- CONFIGURATION --------------------

typedef unsigned long ulong;

enum sigval {
    SIG_LOW  = -85,
    SIG_MED  = -65,
    SIG_HIGH = -55
};

typedef struct textscroll {
    int16_t length;
    int16_t offset;
} textscroll_t;

typedef struct badge {
    uint8_t      team;
    uint8_t      id;
    RgbColor     color;
    char         name[256];
    textscroll_t scroll;
} badge_t;

typedef struct event {
    ulong interval;
    void  (*callback)(void);
    ulong prev;
} event_t;

typedef struct flash {
    RgbColor* color;
    uint8_t   count;
} flash_t;

typedef void(*handler_f)(uint8_t cmd);

char moduleName[15];

bool setupMode = false;
bool connected = false;

int   i;
File  file;
uint  size;
char  name[256];
char  temp[2048];

badge_t badge;
flash_t flash;

Adafruit_SSD1306 oled(16);
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> pixel(8, 0);
WiFiUDP udp;

void otaStart() {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print(FPSTR(STR_OTA_UPDATE));
    oled.setCursor(0, 10);
    oled.print(FPSTR(STR_WAIT));
    oled.display();
}

void otaEnd() {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print(FPSTR(STR_OTA_UPDATE));
    oled.setCursor(0, 10);
    oled.print(FPSTR(STR_OTA_DONE));
    oled.display();
}

void enableSetupMode(const char* msg) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print(msg);
    oled.setCursor(0, 10);
    oled.print("Running Setup Mode");
    oled.display();
    delay(2000);
    setupMode = true;
}

void runSetupMode() {
    setupMode = true;
    
    // Disconnect from any existing network
    WiFi.disconnect();
    delay(10);

    // Switch to AP mode
    WiFi.mode(WIFI_AP);
    delay(10);
    WiFi.softAP(moduleName);

    // Render details
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print("AP: ");
    oled.print(moduleName);
    oled.setCursor(0, 10);
    oled.print(WiFi.softAPIP());
    oled.setCursor(0, 20);
    oled.print("Setup Mode");
    oled.display();
}

void updateName(const char* name) {
    // Either copy the name or the default name
    strncpy(
        badge.name, 
        (name != NULL) ? name : STR_NAME_DEFAULT,
        sizeof(badge.name)-2
    );

    // Make sure the string is terminated with a newline and a null character
    badge.name[strnlen(name, sizeof(badge.name)-2)  ] = '\n';
    badge.name[strnlen(name, sizeof(badge.name)-2)+1] = '\0';

    // Update the name rendering length
    int16_t ox, oy; uint16_t ow, oh;
    oled.setTextSize(2);
    oled.getTextBounds(badge.name, 0, 0, &ox, &oy, &ow, &oh);
    badge.scroll.length = -1 * ow;
    badge.scroll.offset = SSD1306_LCDWIDTH;
}

void updateColor(const RgbColor* color, uint8_t count) {
    flash.count = (count == 0) ? 0 : count * 2 + 1;
    flash.color = const_cast<RgbColor*>(color);
}

void saveConfig() {
    if((file = SPIFFS.open(STR_FILE_CONF, "w"))) {
        StaticJsonBuffer<JSON_BUFF_SIZE> buff;
        JsonObject& root = buff.createObject();
        
        root["team"] = badge.team;
        root["id"  ] = badge.id;
        JsonArray& color = root.createNestedArray("color");
        color.add(badge.color.R);
        color.add(badge.color.G);
        color.add(badge.color.B);

        // Trim off newline when saving the name
        size = strnlen(badge.name, sizeof(badge.name)-1) - 1;
        strncpy(name, badge.name, size);
        name[size] = '\0';
        root["name"] = name;
        
        size = root.prettyPrintTo(temp, sizeof(temp)-1);
        temp[size] = '\0';
        file.print(temp);
        file.close();
    }
}

void renderRSSI() {
    int rssi = WiFi.RSSI();
    oled.fillRect(114, 0, 14, 8, BLACK);

    if(rssi > SIG_LOW ) 
         oled.fillRect(114, 5, 4, 3, WHITE);
    else oled.drawRect(114, 5, 4, 3, WHITE);

    if(rssi > SIG_MED ) 
         oled.fillRect(119, 3, 4, 5, WHITE);
    else oled.drawRect(119, 3, 4, 5, WHITE);

    if(rssi > SIG_HIGH) 
         oled.fillRect(124, 0, 4, 8, WHITE);
    else oled.drawRect(124, 0, 4, 8, WHITE);
}

void renderName() {
    oled.fillRect(0, 10, 128, 22, BLACK);
    oled.setTextSize(2);
    oled.setCursor(badge.scroll.offset, 10);
    oled.print(badge.name);
    oled.display();

    badge.scroll.offset -= 3;
    if(badge.scroll.offset < badge.scroll.length) 
        badge.scroll.offset = SSD1306_LCDWIDTH;
}

void renderTeam() {
    oled.fillRect(80, 0, 30, 10, BLACK);
    oled.setTextSize(1);
    oled.setCursor(80, 0);
    oled.printf("%02d-%02d", badge.team, badge.id);
    oled.display();
}

void renderColor() {
    pixel.ClearTo((flash.count & 1) ? COLOR_OFF : *(flash.color));
    pixel.Show();
    if(flash.count >  0) flash.count--;
    if(flash.count == 0) flash.color = &(badge.color);
}

event_t event[] = {
    {  33, renderName , 0 },
    {  40, renderRSSI , 0 },
    {  40, renderTeam , 0 },
    { 250, renderColor, 0 }
};

void runEvents() {
    ulong curr = millis();
    for(i = 0; i < 4; ++i) {
        if((curr - event[i].prev) >= event[i].interval) {
            event[i].prev = curr;
            event[i].callback();
        }
    }
}

void handleTeamChange(uint8_t cmd) {
    uint8_t team = udp.read();
    uint8_t id   = udp.read();
    badge.team = constrain(team, 1, BADGE_TEAM_MAX);
    badge.id   = constrain(id,   1, BADGE_ID_MAX);
    saveConfig();
}

void handleColorChange(uint8_t cmd) {
    uint8_t r = udp.read();
    uint8_t g = udp.read();
    uint8_t b = udp.read();
    badge.color.R = constrain(r, 0, 255);
    badge.color.G = constrain(g, 0, 255);
    badge.color.B = constrain(b, 0, 255);
    saveConfig();
}

void handleTeamColorChange(uint8_t cmd) {
    uint8_t team = udp.read();
    team = constrain(team, 1, BADGE_TEAM_MAX);
    if(team == badge.team) handleColorChange(cmd);
}

void handleNameChange(uint8_t cmd) {
    uint8_t length = std::min(size, sizeof(badge.name)-2);
    udp.read(name, length);
    name[length] = '\0';
    updateName(name);
    saveConfig();
}

void handleEcho(uint8_t cmd) {
    if(badge.team > 8) return;
    udp.beginPacket(NET_SERVER, NET_PORT);
    udp.write(cmd);
    udp.write(badge.team);
    udp.write(badge.id);
    udp.endPacket();
    updateColor(&COLOR_ECHO, 2);
}

handler_f handler[] = {
    handleTeamChange,
    handleColorChange,
    handleTeamColorChange,
    handleNameChange,
    handleEcho
};

void handleRequests() {
    if(udp.parsePacket() > 0) {
        if((size = udp.available()) > 0) {
            uint8_t cmd = udp.read(); size--;
            if(cmd > 0 && cmd < 6) handler[cmd-1](cmd);
        }
        udp.flush();
    }
}

void setup() {
    sprintf(moduleName, "esp8266-%06x", ESP.getChipId());

    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();
    Serial.printf("--- %s ---\n", moduleName);

    // Initialize the oled
    oled.begin();
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setTextWrap(false);
    oled.display();

    // Initialize the neopixel
    pixel.Begin();
    pixel.Show();

    setupMode = false;
    connected = false;

    // Initialize the file system
    if(!SPIFFS.begin()) {
        enableSetupMode("SPIFFS mount failure");
    }

    // Set badge defaults
    badge.team  = BADGE_TEAM_DEFAULT;
    badge.id    = BADGE_ID_DEFAULT;
    badge.color = COLOR_DEFAULT;
    updateName(STR_NAME_DEFAULT);

    // Load the badge configuration file
    if(!setupMode && (file = SPIFFS.open(STR_FILE_CONF, "r"))) {
        size = file.readBytes(temp, file.size());
        if(size == file.size()) {
            
            // Parse the badge configuration
            temp[size] = '\0';
            StaticJsonBuffer<JSON_BUFF_SIZE> buff;
            JsonObject& root = buff.parseObject(temp);

            if(root.success()) {
                // Make sure things are in byte range
                // Also make sure that the default will be 1-1
                badge.team  = constrain(root["team"], 1, 255);
                badge.id    = constrain(root["id"]  , 1, 255);

                // Parse the badge color
                JsonArray& color = root["color"];
                if(color.success() && color.size() == 3) {
                    // Make sure each element is in range (0-255)
                    badge.color.R = constrain(color[0], 0, 255);
                    badge.color.G = constrain(color[1], 0, 255);
                    badge.color.B = constrain(color[2], 0, 255);
                }
                // Fall back to the default if the color isn't valid
                else badge.color = COLOR_DEFAULT;

                // Parse the badge name
                const char* name = root["name"];
                updateName(name);
            }
            else {
                enableSetupMode("Badge conf invalid");
            }
        } 
        else {
            enableSetupMode("Badge conf not found");
        }
        file.close();
    }

    // Set the badge color
    updateColor(&(badge.color), 0);
    pixel.ClearTo(badge.color);
    pixel.Show();

    // Try to connect to the wifi defined by the configuration
    if(!setupMode && (file = SPIFFS.open(STR_FILE_WIFI, "r"))) {
        size = file.readBytes(temp, file.size());
        if(size == file.size()) {

            // Parse the wifi configuration
            temp[size] = '\0';
            StaticJsonBuffer<JSON_BUFF_SIZE> buff;
            JsonObject& root = buff.parseObject(temp);

            if(root.success()) {

                // Extract the SSID and password
                // This assumes that the parser will return NULL when
                // a key is unset
                const char* ssid = root["ssid"];
                const char* pass = root["pass"];

                if(ssid != NULL) {
                    oled.clearDisplay();
                    oled.setTextSize(1);
                    oled.setCursor(0, 0);
                    oled.printf("SSID: %s", ssid);
                    oled.setCursor(0, 10);
                    oled.print("Connecting:");
                    oled.setCursor(0, 20);
                    oled.display();

                    // The ESP needs a little time to change modes
                    WiFi.mode(WIFI_STA);
                    delay(10);

                    // Try to connect for allotted time
                    WiFi.begin(ssid, pass);
                    ulong start = millis();
                    uint  count = 0;
                    while(!(connected = (WiFi.status() == WL_CONNECTED)) && 
                          (millis()-start) < WIFI_TIMEOUT) {
                        if((count = (count + 1) % 4) == 3) {
                            oled.print('.');
                            oled.display();
                        }
                        delay(500);
                    }

                    // If we're still not connected, use setup mode
                    if(!connected) enableSetupMode("Unable to connect");
                }
                else {
                    enableSetupMode("SSID unset");
                }
            }
            else {
                enableSetupMode("WiFi conf invalid");
            }
        }
        else {
            enableSetupMode("WiFi conf not found");
        }
        file.close();
    }

    if(connected) {
        // If we did connect, display the IP we got and the badge info
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setCursor(0, 0);
        oled.print(WiFi.localIP());
        oled.display();

        // Start the UDP server
        udp.beginMulticast(WiFi.localIP(), NET_GROUP, NET_PORT);
    }
    // If unable to connect to the configured station, use AP mode
    else {
        runSetupMode();
    }

    // Setup OTA updates
    ArduinoOTA.onStart(otaStart);
    ArduinoOTA.onEnd(otaEnd);
    ArduinoOTA.begin();
}

void loop() {
    // Handle OTA requests
    ArduinoOTA.handle();
    
    // Setup should only handle OTA requests
    if(setupMode) return yield();

    // Process all time-based events without blocking
    runEvents();

    // Handle incoming UDP requests
    handleRequests();

    // Let the ESP do any other background things
    yield();
}