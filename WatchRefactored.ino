#include <TFT_eSPI.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <time.h>
#include <esp_adc_cal.h>
#include <driver/adc.h>
#include <map>
#include "wifi_logo_16x16.h"
#include "josh_hutcherson_240x135.h"
#include "prigozhin_angery_240x135.h"
#include "fahrzeug_240x135.h"
#include "epstein_240x135.h"
#include "hawking_240x135.h"

// -------------- BASIC DEFINES --------------


#define WIDTH 240
#define HEIGTH 135

#define BUTTON1PIN 35
#define BUTTON2PIN 0

#define WIFI_COUNT 5
#define NUM_PAGES 8

#define MEME_COUNT 4

// -------------- BASIC STRUCTURES --------------

const String wifi_information[WIFI_COUNT][2] = {
    {"Vodafone-1B63", "yHh5wn84rxY2PRry"},
    {"Radisek", "Radisek1721975"},
    {"WGT-HOSTE", "gpdcgpdc"},
    {"WGT-UCITELE", "pieRRe 0"},
    {"CDWiFi", ""},
};

struct ButtonState {
    bool pressed = false;
    bool long_press = false;
    unsigned long press_time = 0;
    unsigned long long_press_last_time = 0;
    unsigned long long_press_delay = 500;
};

struct WeatherData {
    String long_url = "https://wttr.in/Tábor?T&d";
    String short_url = "https://wttr.in/Tábor?format=\"%t+%C\"";
    String short_info;
    String long_info;
};

struct PongState {
    int paddle_height = 23;
    int paddle_width = 3;
    int paddle1_x = 3;
    int paddle1_y = 60;
    int paddle1_vel = -2;
    int paddle2_x = 236;
    int paddle2_y = 60;
    int paddle2_vel = -2;

    int player1_score = 0;
    int player2_score = 0;

    int ball_x = 120;
    int ball_y = 67;
    int ball_size = 3;
    int ball_vel_x = 1;
    int ball_vel_y = 1;
};

struct Page {
    String name;
    void (*displayFunction)(Page * page);
    void (*interactFunction)(Page * page);
    bool interact1;
    bool interact2;
    int scroll;
};

// -------------- FUNCTION PROTOTYPES --------------

void reset_sprite();
void setupDisplay();
void getWeatherData(WeatherData * data);
void reset_buttons(ButtonState * button1, ButtonState * button2);
void handle_buttons(ButtonState * button1, ButtonState * button2);
void setupPins();
void setupWifi();
void nextPage();
void prevPage();

void displayHomePage(Page * page);
void displayDebugPage(Page * page);
void displayWeatherPage(Page * page);
void displayNetworksPage(Page * page);
void displayPongPage(Page * page);
void displayMemePage(Page * page);
void displayBrightnessPage(Page * page);
void displayCheatPage(Page * page);

void genericInteract(Page * page);
void interactHomePage(Page * page);
void interactWeatherPage(Page * page);
void interactNetworksPage(Page * page);
void interactPongPage(Page * page);
void interactBrightnessPage(Page * page);
void interactCheatPage(Page * page);


//void getBatteryVoltage();

// -------------- GLOBAL VARIABLES --------------

int selected_page = 0;
ButtonState button1 = {};
ButtonState button2 = {};
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
WeatherData w_data = {};
PongState pong_state = {};
tm timeinfo = {};

int brightness = 100;

int meme_index = 0;

int settings_index = 0;

String wifi_info_string = "";

// whole lorem ipsum text
String cheat = "lorem ipsum dolor sit amet, consectetur adipiscing elit,sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";

const unsigned short * memes[MEME_COUNT] = {
    josh_hutcherson_240x135,
    prigozhin_angery_240x135,
    epstein_240x135,
    hawking_240x135
};

//unsigned long battery_measure_last_time = 0;
//unsigned long battery_measure_timer = 5000;
//float battery_voltage = 0;

// -------------- FUNCTION DEFINITIONS --------------

// resets the buffer, filling it with black
// and setting the cursor position to (0,0)
void reset_sprite() {
    sprite.fillSprite(sprite.color565(0, 0, 0));
    sprite.setCursor(0,0);
}

// sets the display up, initializing all the
// tft things. also sets up the buffer
void setupDisplay() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    sprite.createSprite(WIDTH, HEIGTH);
    sprite.setTextDatum(3);
    sprite.setSwapBytes(true);

    reset_sprite();
}

// gets the current weather data from wttr.in
// the data gets put into a WeatherData struct
void getWeatherData(WeatherData * data) {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    HTTPClient http;
    http.setUserAgent("curl/8.5.0");
    http.addHeader("Accept", "text/plain");
    http.begin(data->short_url);
    int response_code = http.GET();
    if (response_code > 0) {
        data->short_info = http.getString();
        data->short_info.replace("°", String((char)247));
        data->short_info.replace("\"", "");
    } else {
        data->short_info = "Error";
    }

    http.end();

    http.setUserAgent("curl/8.5.0");
    http.addHeader("Accept", "text/plain");
    http.begin(data->long_url);
    response_code = http.GET();
    if (response_code > 0) {
        data->long_info = http.getString();
        data->long_info.replace("°", String((char)247));
        data->long_info.replace("á", String((char)134));
    } else {
        data->long_info = "Error";
    }
}

// Puts all of the button states to false
// remember to call this min once after every
// tick of the program
void reset_buttons(ButtonState * button1, ButtonState * button2) {
    button1->pressed = false;
    button2->pressed = false;
    button1->long_press = false;
    button2->long_press = false;
}

// handles the button inputs, abstracting all of the
// bullshit away and putting the states into the
// ButtonState structs
void handle_buttons(ButtonState * button1, ButtonState * button2) {
    unsigned long currentTime = millis();

    if (digitalRead(BUTTON1PIN) == LOW && button1->press_time == 0) {
        button1->press_time = 1;
        button1->long_press_last_time = currentTime;
    }

    if (digitalRead(BUTTON1PIN) == LOW) {
        button1->press_time += currentTime - button1->long_press_last_time;
        button1->long_press_last_time = currentTime;
    }

    if (digitalRead(BUTTON1PIN) == HIGH && button1->press_time > 0) {
        if (button1->press_time >= button1->long_press_delay) {
            button1->long_press = true;
        } else {
            button1->pressed = true;
        }
        button1->press_time = 0;
        button1->long_press_last_time = 0;
    }



    if (digitalRead(BUTTON2PIN) == LOW && button2->press_time == 0) {
        button2->press_time = 1;
        button2->long_press_last_time = currentTime;
    }

    if (digitalRead(BUTTON2PIN) == LOW) {
        button2->press_time += currentTime - button2->long_press_last_time;
        button2->long_press_last_time = currentTime;
    }

    if (digitalRead(BUTTON2PIN) == HIGH && button2->press_time > 0) {
        if (button2->press_time >= button2->long_press_delay) {
            button2->long_press = true;
        } else {
            button2->pressed = true;
        }

        button2->press_time = 0;
        button2->long_press_last_time = 0;
    }
}

// sets the pins to the correct modes
void setupPins() {
    //esp_adc_cal_characteristics_t adc_chars;
    //esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC_ATTEN_DB_2_5, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
    //pinMode(14, OUTPUT);
    pinMode(BUTTON1PIN, INPUT);
    pinMode(BUTTON2PIN, INPUT);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, LOW);
}

void genericInteract(Page * page) {
    handle_buttons(&button1, &button2);

    if (button1.pressed) {
        nextPage();
    }

    if (button2.pressed) {
        prevPage();
    }

    reset_buttons(&button1, &button2);
}

void displayCheatPage(Page * page) {
    reset_sprite();

    sprite.setTextColor(sprite.color565(255, 0, 0));
    sprite.setCursor(0, 18+page->scroll);
    sprite.setTextSize(1);

    sprite.println(cheat);

    sprite.setCursor(0, 0);
    sprite.setTextColor(sprite.color565(255, 0, 0), sprite.color565(0, 0, 0));
    sprite.setTextSize(2);
    sprite.println("Nothing suspicious:");
    

    if (page->interact1) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 0, 0));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 0, 0));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 0, 0));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 0, 0));
    }

    sprite.pushSprite(0, 0);
}

void interactCheatPage(Page * page) {
    handle_buttons(&button1, &button2);
    if (button1.long_press) {
        page->interact1 = !page->interact1;
    }

    if (page->interact1) {
        if (button1.pressed) {
            page->scroll -= 5;
        }

        if (button2.pressed) {
            page->scroll += 5;
        }
    } else {
        if (button1.pressed) {
            nextPage();
        }

        if (button2.pressed) {
            prevPage();
        }
    }
    reset_buttons(&button1, &button2);
}

void displayDebugPage(Page * page) {
    reset_sprite();
    sprite.setTextSize(2);
    sprite.setTextColor(sprite.color565(255, 0, 0));
    sprite.println("Debug:");
    sprite.setTextSize(1);

    if (digitalRead(BUTTON1PIN) == LOW) {
        sprite.println("Button 1: pressed");
    } else {
        sprite.println("Button 1: released");
    }
    sprite.print("Button 1 press time: ");
    sprite.println(button1.press_time);


    if (digitalRead(BUTTON2PIN) == LOW) {
        sprite.println("Button 2: pressed");
    } else {
        sprite.println("Button 2: released");
    }
    sprite.print("Button 2 press time: ");
    sprite.println(button2.press_time);

    sprite.print("Brightness: ");
    sprite.println(brightness);

    sprite.print("WiFi local IP: ");
    sprite.println(WiFi.localIP());

    static std::map<wl_status_t, String> statusMap = {
        {WL_NO_SHIELD, "No Shield"},
        {WL_IDLE_STATUS, "Idle"},
        {WL_SCAN_COMPLETED, "Scan Completed"},
        {WL_CONNECTED, "Connected"},
        {WL_CONNECT_FAILED, "Connect Failed"},
        {WL_CONNECTION_LOST, "Connection Lost"},
        {WL_DISCONNECTED, "Disconnected"}
    };
    sprite.print("Wifi status:");
    sprite.println(statusMap.find(WiFi.status())->second);

    sprite.print("Milis: ");
    sprite.println(millis());


    sprite.pushSprite(0,0);
}

void displayHomePage(Page * page) {
    getLocalTime(&timeinfo);
    reset_sprite();

    sprite.setTextColor(sprite.color565(255, 0, 0));

    sprite.setCursor(65,0);
    sprite.setTextSize(2);
    sprite.print(timeinfo.tm_mday);
    sprite.print(".");
    sprite.print(timeinfo.tm_mon + 1);
    sprite.print(". ");
    sprite.println(timeinfo.tm_year + 1900);

    sprite.setTextSize(5);

    sprite.print(&timeinfo, "%H");
    sprite.print(":");
    sprite.print(&timeinfo, "%M");
    sprite.print(":");
    sprite.println(&timeinfo, "%S");

    sprite.setTextSize(1);
    sprite.println();
    sprite.setTextSize(2);
    sprite.println(w_data.short_info);

    sprite.setTextSize(1);
    sprite.println();
    sprite.setTextSize(2);
    sprite.print("Brightness: ");
    sprite.println(brightness);

    //sprite.print("Battery: ");
    //sprite.print(battery_voltage);
    //sprite.println("V");

    if (digitalRead(BUTTON1PIN) == LOW && button1.press_time > button1.long_press_delay) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 255, 255));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 255, 255));
    }

    sprite.pushSprite(0,0);
}

void interactHomePage(Page * page) {
    handle_buttons(&button1, &button2);
    //getBatteryVoltage();

    if (button1.pressed) {
        nextPage();
    }

    if (button2.pressed) {
        prevPage();
    }

    if (button1.long_press) {
        int tmp = brightness;
        brightness = 0;
        ledcSetup(0, 5000, 8); // 0-15, 5000, 8
        ledcAttachPin(TFT_BL, 0); // TFT_BL, 0 - 15
        ledcWrite(0, brightness); // 0-15, 0-255 (with 8 bit resolution)
        esp_light_sleep_start();
        brightness = tmp;
        ledcSetup(0, 5000, 8); // 0-15, 5000, 8
        ledcAttachPin(TFT_BL, 0); // TFT_BL, 0 - 15
        ledcWrite(0, brightness); // 0-15, 0-255 (with 8 bit resolution)
        delay(250);
        //prevPage();
    }

    reset_buttons(&button1, &button2);
}

void displayMemePage(Page * page) {
    reset_sprite();
    sprite.pushImage(0, 0, 240, 135, memes[meme_index]);

    if (page->interact1) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 0, 0));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 0, 0));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 0, 0));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 0, 0));
    }

    if (digitalRead(BUTTON1PIN) == LOW && button1.press_time > button1.long_press_delay) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 255, 255));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 255, 255));
    }

    sprite.pushSprite(0,0);
}

void interactMemePage(Page * page) {
    handle_buttons(&button1, &button2);

    if (button1.long_press) {
        page->interact1 = !page->interact1;
    }

    if (page->interact1) {
        if (button1.pressed) {
            if (meme_index + 1 > MEME_COUNT - 1) {
                meme_index = 0;
            } else {
                meme_index += 1;
            }
        }


        if (button2.pressed) {
            if (meme_index-1 < 0) {
                meme_index = MEME_COUNT-1;
            } else {
                meme_index -= 1;
            }
        }
    } else {
        if (button1.pressed) {
            nextPage();
        }
        if (button2.pressed) {
            prevPage();
        }
    }

    reset_buttons(&button1, &button2);
}

void displayNetworksPage(Page * page) {
    reset_sprite();

    sprite.setCursor(0, 16+page->scroll);
    sprite.setTextSize(1);
    sprite.println("");
    sprite.println(wifi_info_string);

    sprite.setCursor(24,0);
    sprite.fillRect(0, 0, 240, 16, sprite.color565(0, 0, 0));
    sprite.setTextSize(2);
    sprite.setTextColor(sprite.color565(255, 0, 0));
    sprite.println("Wifi networks:");
    sprite.drawLine(24, 16, 184, 16, sprite.color565(255, 0, 0));
    sprite.pushImage(0, 0, 16, 16, wifi_logo_16x16);

    if (page->interact2) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 255, 255));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 255, 255));
    }

    if (digitalRead(BUTTON1PIN) == LOW && button1.press_time > button1.long_press_delay) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 255, 255));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 255, 255));
    }

    sprite.pushSprite(0,0);
}

void interactNetworksPage(Page * page) {
    handle_buttons(&button1, &button2);
    
    if (!page->interact2) {
        if (button1.pressed) {
            nextPage();
        }

        if (button2.pressed) {
            prevPage();
        }

        if (button1.long_press) {
            wifi_info_string = "";

            int n = WiFi.scanNetworks();
            if (n == 0) {
                wifi_info_string = "no networks found";
            } else {
                wifi_info_string += String(n);
                wifi_info_string += " networks found\n";
                for (int i = 0; i < n; ++i) {
                    wifi_info_string += String(i + 1);
                    wifi_info_string += ": ";
                    wifi_info_string += WiFi.SSID(i);
                    wifi_info_string += " (";
                    wifi_info_string += WiFi.RSSI(i);
                    wifi_info_string += ")";
                    wifi_info_string += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*";
                    wifi_info_string += "\n";
                    delay(10);
                }
            }
        }
    } else {
        if (button1.pressed) {
            page->scroll -= 5;
        }

        if (button2.pressed) {
            page->scroll += 5;
        }
    }

    if (button2.long_press) {
        page->interact2 = !page->interact2;
    }

    reset_buttons(&button1, &button2);
}

void displayPongPage(Page * page) {
    reset_sprite();

    if (page->interact1) {
        // draw the center line
        sprite.setTextSize(2);
        sprite.drawLine(120, 0, 120, 135, sprite.color565(120, 0, 0));
        // draw the scores
        sprite.drawString(String(pong_state.player1_score), 80, 8);
        sprite.drawString(String(pong_state.player2_score), 140, 8);

        // draw the paddles
        sprite.fillRect(pong_state.paddle1_x, pong_state.paddle1_y, pong_state.paddle_width, pong_state.paddle_height, sprite.color565(255, 0, 0));
        sprite.fillRect(pong_state.paddle2_x, pong_state.paddle2_y, pong_state.paddle_width, pong_state.paddle_height, sprite.color565(255, 0, 0));

        // draw the ball
        sprite.fillCircle(pong_state.ball_x, pong_state.ball_y, pong_state.ball_size, sprite.color565(255, 0, 0));
    } else {
        sprite.setTextSize(8);
        sprite.setTextColor(sprite.color565(255, 0, 0));
        sprite.setCursor(40, 40);
        sprite.print("PONG");
    }

    sprite.pushSprite(0,0);
}

void interactPongPage(Page * page) {
    handle_buttons(&button1, &button2);

    if (digitalRead(BUTTON1PIN) == LOW && digitalRead(BUTTON2PIN) == LOW && button1.press_time > 2000 && button2.press_time > 2000) {
        pong_state = {};
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 255, 255));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 255, 255));
        sprite.pushSprite(0,0);
        delay(500);
        page->interact1 = !page->interact1;
    }

    if (page->interact1) {
        // first handle the game logic
        // move the ball
        
        // check if the ball hits the paddles
        if ((pong_state.ball_x + pong_state.ball_size > pong_state.paddle2_x && pong_state.ball_y >= pong_state.paddle2_y && pong_state.ball_y <= pong_state.paddle2_y + pong_state.paddle_height) || ((pong_state.ball_x - pong_state.ball_size < pong_state.paddle1_x && pong_state.ball_y >= pong_state.paddle1_y && pong_state.ball_y <= pong_state.paddle1_y + pong_state.paddle_height))) {
            pong_state.ball_vel_x *= -1;
        }

        if (pong_state.ball_x + pong_state.ball_size + pong_state.ball_vel_x > 239) {
            pong_state.player1_score += 1;
            pong_state.ball_x = 120;
            pong_state.ball_vel_x *= -1;
            delay(100);
        } else if (pong_state.ball_x + pong_state.ball_size + pong_state.ball_vel_x < 0) {
            pong_state.player2_score += 1;
            pong_state.ball_x = 120;
            pong_state.ball_vel_x *= -1;
            delay(100);
        } else {
            pong_state.ball_x += pong_state.ball_vel_x;
        }

        if (pong_state.ball_y + pong_state.ball_size + pong_state.ball_vel_y > 134 || pong_state.ball_y + pong_state.ball_size + pong_state.ball_vel_y < 0) {
            pong_state.ball_vel_y *= -1;
        } else {
            pong_state.ball_y += pong_state.ball_vel_y;
        }

        // move the paddles
        if (pong_state.paddle1_y + pong_state.paddle1_vel < 0) {
            pong_state.paddle1_y = 0;
        } else if (pong_state.paddle1_y + pong_state.paddle_height + pong_state.paddle1_vel > 134) {
            pong_state.paddle1_y = 134 - pong_state.paddle_height;
        } else {
            pong_state.paddle1_y += pong_state.paddle1_vel;
        }

        if (pong_state.paddle2_y + pong_state.paddle2_vel < 0) {
            pong_state.paddle2_y = 0;
        } else if (pong_state.paddle2_y + pong_state.paddle_height + pong_state.paddle2_vel > 134) {
            pong_state.paddle2_y = 134 - pong_state.paddle_height;
        } else {
            pong_state.paddle2_y += pong_state.paddle2_vel;
        }

        if (button1.pressed) {
            pong_state.paddle1_vel *= -1;
        }

        if (button2.pressed) {
            pong_state.paddle2_vel *= -1;
        }

        
    } else {
        if (button1.pressed) {
            nextPage();
        }

        if (button2.pressed) {
            prevPage();
        }
    }

    reset_buttons(&button1, &button2);
}

void displayBrightnessPage(Page * page) {
    reset_sprite();
    sprite.setCursor(0, 0);
    sprite.setTextColor(sprite.color565(255, 0, 0));

    sprite.setTextSize(3);
    sprite.println("Brightness");
    sprite.println(brightness);

    if (page->interact1) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 0, 0));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 0, 0));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 0, 0));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 0, 0));
    }
    
    if (digitalRead(BUTTON1PIN) == LOW && button1.press_time > button1.long_press_delay) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 255, 255));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 255, 255));
    }

    sprite.pushSprite(0, 0);
}

void interactBrightnessPage(Page * page) {
    handle_buttons(&button1, &button2);
    if (button1.long_press) {
        page->interact1 = !page->interact1;
    }

    if (page->interact1) {
        if (button1.pressed) {
            brightness += 10;
            if (brightness > 255) {
                brightness = 255;
            }
        }

        if (button2.pressed) {
            brightness -= 10;
            if (brightness < 0) {
                brightness = 0;
            }
        }
    } else {
        if (button1.pressed) {
            nextPage();
        }

        if (button2.pressed) {
            prevPage();
        }
    }
    
    reset_buttons(&button1, &button2);
}

// displays a menu to select a WiFi network from
// a saved array to connect to. this function
// should be called only once
void setupWifi() {
    int selected_wifi = 0;

    while (WiFi.status() != WL_CONNECTED) {
        reset_sprite();
        handle_buttons(&button1, &button2);
        sprite.setTextColor(sprite.color565(255, 0, 0));
        sprite.setTextSize(2);
        sprite.println("Select a network:");
        sprite.drawLine(0, 18, 190, 18, sprite.color565(255, 0, 0));
        sprite.setTextSize(1);
        sprite.println();
        sprite.setTextSize(2);

        for (int i = 0; i < WIFI_COUNT; i++) {
            if (i == selected_wifi) {
                sprite.setTextColor(sprite.color565(255, 255, 255));
                sprite.print("> ");
                sprite.print(wifi_information[i][0]);
                sprite.println(" <");
            } else {
                sprite.setTextColor(sprite.color565(255, 0, 0));
                sprite.print("  ");
                sprite.println(wifi_information[i][0]);
            }
        }

        if (button1.long_press) {
            reset_sprite();
            sprite.setTextColor(sprite.color565(255, 0, 0));
            sprite.setTextSize(1);
            sprite.print("[Connecting to : ");
            sprite.print(wifi_information[selected_wifi][0]);
            sprite.println("]");

            WiFi.begin(wifi_information[selected_wifi][0].c_str(), wifi_information[selected_wifi][1].c_str());
            while (WiFi.status() != WL_CONNECTED) {
                sprite.print(".");
                sprite.pushSprite(0,0);
                delay(500);
            }
            
            sprite.println();
            sprite.print("[Connected to : ");
            sprite.print(wifi_information[selected_wifi][0]);
            sprite.println("]");
            sprite.pushSprite(0,0);
            delay(1000);
        }

        if (button1.pressed) {
            if (selected_wifi == 0) {
                selected_wifi = WIFI_COUNT - 1;
            } else {
                selected_wifi -= 1;
            }
        }

        if (button2.pressed) {
            if (selected_wifi == WIFI_COUNT - 1) {
                selected_wifi = 0;
            } else {
                selected_wifi += 1;
            }
        }

        reset_buttons(&button1, &button2);
        sprite.pushSprite(0,0);
    }
}

void displayWeatherPage(Page * page) {
    reset_sprite();
    sprite.setTextSize(1);
    sprite.setCursor(0,0);
    sprite.setTextColor(sprite.color565(255, 0, 0));
    sprite.println(w_data.long_info);
    sprite.fillRect(0, 60, 240, 135, sprite.color565(0, 0, 0));

    if (digitalRead(BUTTON1PIN) == LOW && button1.press_time > button1.long_press_delay) {
        sprite.drawLine(0, 0, 240, 0, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 0, 0, 135, sprite.color565(255, 255, 255));
        sprite.drawLine(0, 134, 240, 134, sprite.color565(255, 255, 255));
        sprite.drawLine(239, 0, 239, 135, sprite.color565(255, 255, 255));
    }

    sprite.pushSprite(0,0);
}

void interactWeatherPage(Page * page) {
    handle_buttons(&button1, &button2);

    if (button1.pressed) {
        nextPage();
    }

    if (button2.pressed) {
        prevPage();
    }

    if (button1.long_press) {
        getWeatherData(&w_data);
    }

    reset_buttons(&button1, &button2);
}

// ------------------------- PAGE DEFINITIONS -----------------------


Page pages[NUM_PAGES] = {
    {"Home", displayHomePage, interactHomePage, false, false, 0},
    {"Debug", displayDebugPage, genericInteract, false, false, 0},
    {"Weather", displayWeatherPage, interactWeatherPage, false, false, 0},
    {"Networks", displayNetworksPage, interactNetworksPage, false, false, 0},
    {"Memes", displayMemePage, interactMemePage, false, false, 0},
    {"Pong", displayPongPage, interactPongPage, false, false, 0},
    {"Brightness", displayBrightnessPage, interactBrightnessPage, false, false, 0},
    {"Cheat", displayCheatPage, interactCheatPage, false, false, 0},
};

void nextPage() {
    if (selected_page + 1 > NUM_PAGES - 1) {
        selected_page = 0;
    } else {
        selected_page += 1;
    }
}

void prevPage() {
    if (selected_page - 1 < 0) {
        selected_page = NUM_PAGES - 1;
    } else {
        selected_page -= 1;
    }
}


// esp32 setup function, this gets run only once
void setup () {
    setupPins();
    Serial.begin(9600);
    Serial.println("Starting...");
    setupDisplay();
    setupWifi();
    sprite.setTextColor(sprite.color565(255, 0, 0));
    sprite.println("getting weather data...");
    sprite.pushSprite(0,0);
    getWeatherData(&w_data);
    sprite.println("configuring time using ntp...");
    sprite.pushSprite(0,0);
    configTime(3600, 0, "pool.ntp.org");
}

// esp32 loop function
void loop() {
    ledcSetup(0, 5000, 8); // 0-15, 5000, 8
    ledcAttachPin(TFT_BL, 0); // TFT_BL, 0 - 15
    ledcWrite(0, brightness); // 0-15, 0-255 (with 8 bit resolution)
    pages[selected_page].displayFunction(&pages[selected_page]);
    pages[selected_page].interactFunction(&pages[selected_page]);
}
