//MediBox_Project
//Created by Liyanage.D.L.B.B 220362G

//Required Libraries
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include "time.h"

//Definitions
#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 19800
#define UTC_OFFSET_DST 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define BUZZER 18
#define LED_1 15
#define LED_2 2
#define CANCEL 34
#define UP 35
#define DOWN 32
#define OK 33
#define DHTPIN 12
#define DHTTYPE DHT22

// Object declarations
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);

//Variables

int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

bool alarm_enabled[] = {false,false, false};
int n_alarms = 3;
int alarm_hours[] = {0,0,0};
int alarm_minutes[] = {0,0,0};
bool alarm_triggered[] = {true, true, true};
int cancel_intr = 0;

int time_zone_minute[] = {0, 30, 45};

unsigned long timenow = 0;
unsigned long timeLast = 0;

int current_mode = 0;
int max_modes = 5;
String options[] = {" 1 : Set Time Zone" , " 2 : Set Alarm 1", " 3 : Remove Alarm 1", " 4 : Set Alarm 2", " 5 : Remove Alarm 2"};

int utc_hours = +5;
int utc_minutes = time_zone_minute[1];

// Function declarations
void IRAM_ATTR handleCANCEL();
void print_line_inverted(String text, int text_size, int row, int column);
void print_line(String text, int text_size, int row, int column);
void update_time();
void update_time_with_WIFI(int utc_hours, int utc_minutes, int sign);
void update_time_with_check_alarm();
void print_time_now(void);
void ring_alarm();
int  wait_for_button_press();
void go_to_menu(void);
void run_mode(int mode);
void set_time_zone();
void set_alarm(int alarm);
void check_temp(void);
void print_alarm(void);
void snooze_alarm(void);

volatile bool cancelPressed = false;
volatile bool OKPressed = false;


void IRAM_ATTR handleCANCEL() {
    if (cancel_intr == 0){
        cancelPressed = true;
    }
    else{
        cancelPressed =false;
    }
}
// setup function
void setup() {
    Serial.begin(9600);
    
    pinMode(BUZZER, OUTPUT);
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(CANCEL, INPUT);
    pinMode(UP, INPUT);
    pinMode(DOWN, INPUT);
    pinMode(OK, INPUT);
    
    attachInterrupt(digitalPinToInterrupt(CANCEL), handleCANCEL, FALLING);// attach interrupt to the cancel button
    
    // put your setup code here, to run once:
    Serial.println("MediBox is Initializing!");

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    
    delay(2000); 
    display.clearDisplay();

    WiFi.begin("Wokwi-GUEST", "", 6);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      display.clearDisplay();
      print_line("Connecting to WiFi", 1, 20, 0); // (String, text_size, cursor_row, cursor_column)
      display.display();
    }
    display.clearDisplay();
    print_line("Connected to WiFi", 1, 20, 0);
    display.display();
    
    
}

void loop() {

    cancel_intr = 0;

    display.clearDisplay();
    check_temp();
    update_time_with_check_alarm();
    display.display();

    if (cancelPressed){
        cancel_intr = 1;
        delay(100);
        go_to_menu();
        delay(100);
    }
}

// Function to print a line of text in a given text size and the given position
void print_line(String text, int text_size, int row, int column) {
    display.setTextSize(text_size);      
    display.setTextColor(SSD1306_WHITE); 
    display.setCursor(column, row);     
    display.println(text);
    
}

void print_line_inverted(String text, int text_size, int row, int column) {
    display.setTextSize(text_size);    
    display.setTextColor(SSD1306_BLACK,SSD1306_WHITE); 
    display.setCursor(column, row);      
    display.println(text);
}
    
// Function to automatically update the current time
void update_time(void) {
    timenow = millis() / 1000;  
    seconds = timenow - timeLast; 

    // If a minute has passed
    if (seconds >= 60) {
        seconds = 0;
        timeLast += 60;
        minutes += 1;
    }

    // If an hour has passed
    if (minutes == 60) {
        minutes = 0;
        hours += 1;
    }

    // If a day has passed
    if (hours == 24) {
        hours = 0;
        days += 1;

    // Enable the alarms again
    for (int i = 0; i < n_alarms; i++) {
        alarm_triggered[i] = false;
    }
  }
}

void update_time_with_WIFI(int utc_hours, int utc_minutes) {

    int utc_offset_seconds;

    if (utc_hours >= 0 ){
        utc_offset_seconds = (utc_hours * 3600) + (utc_minutes * 60);
    }
    else{
        utc_offset_seconds = -((utc_hours * 3600) + (utc_minutes * 60));
    }

    configTime(utc_offset_seconds, 0, "pool.ntp.org");

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    
    char day_str[8];
    char hour_str[8];
    char min_str[8];
    char sec_str[8];

    strftime(day_str, 8, "%d", &timeinfo);
    strftime(sec_str, 8, "%S", &timeinfo);
    strftime(hour_str, 8, "%H", &timeinfo);
    strftime(min_str, 8, "%M", &timeinfo);

    hours = atoi(hour_str);
    minutes = atoi(min_str);
    days = atoi(day_str);
    seconds = atoi(sec_str);
}

// Function to display the current time in DD:HH:MM:SS format
void print_time_now(void) {
    print_line("Day : ",1 , 0, 0);
    print_line(String(days), 1, 0, 40);
   
    print_line(String(hours), 2, 15, 15);
    print_line(":", 2, 15, 40);
    print_line(String(minutes), 2, 15, 55);
    print_line(":", 2, 15, 80);
    print_line(String(seconds), 2, 15, 95);
    


}

void print_alarm(void){
    if (alarm_enabled[0] == true){
        print_line("A_1:ON", 1, 40, 80);
    }
    else{
        print_line("A_1:OFF", 1, 40, 80);
    }

    if (alarm_enabled[1] == true){
        print_line("A_2:ON", 1, 50, 80);
    }
    else{
        print_line("A_2:OFF", 1, 50, 80);
    }
}

// Function to automatically update the current time while checking for alarms
void update_time_with_check_alarm() {

    update_time_with_WIFI(utc_hours, utc_minutes);
    print_time_now();
    print_alarm();
    

    for (int i = 0; i < n_alarms; i++) {

        if (alarm_enabled[i] == true && alarm_triggered[i] == false && hours == alarm_hours[i] && minutes == alarm_minutes[i]) {
            alarm_triggered[i] = true;
            if (i==2){
                Serial.println("snoozed");
                alarm_enabled[i] == false;  
            }
            ring_alarm();  // Call the ringing function
        }
    }
   
}

void ring_alarm() {
    cancel_intr = 0;

    display.clearDisplay();
    print_line("MEDICINE TIME!", 1, 20, 10);
    display.display();
    
    digitalWrite(LED_1, HIGH);

    while (true) {
        if (cancelPressed) {
            delay(200);
            break;
        }

        if (digitalRead(OK) == LOW) {
            delay(200);
            snooze_alarm();
            display.clearDisplay();
            print_line("Snoozed for 5 minutes", 1, 20, 2);
            display.display();
            delay(2000);
            break;
        }

        //siren 
        for (int freq = 440; freq <= 880; freq += 20) { 
            tone(BUZZER, freq);
            delay(50);
            if (cancelPressed || digitalRead(OK) == LOW) break;
        }
        
        for (int freq = 880; freq >= 440; freq -= 20) { 
            tone(BUZZER, freq);
            delay(50);
            if (cancelPressed || digitalRead(OK) == LOW) break;
        }
    }

    noTone(BUZZER);
    delay(200);
    digitalWrite(LED_1, LOW);
}

void snooze_alarm(void) {
    alarm_enabled[2] = true;
    alarm_triggered[2] = false;

    int temp_hours = hours;
    int temp_minutes = minutes + 5;  // Snooze for 5 minutes
    if (temp_minutes >= 60) {
        temp_hours += temp_minutes / 60;  // Increase hours if needed
        temp_minutes %= 60;  // Keep minutes within 0-59 range
    }

    temp_hours %= 24;  // Ensure hours stay within 0-23 range

    alarm_hours[2] = temp_hours;
    alarm_minutes[2] = temp_minutes;
}


// function to wait for button press in the menu
int wait_for_button_press() {
    while (true) {
        if (digitalRead(UP) == LOW) {
            delay(200);
            return UP;
        }

        else if (digitalRead(DOWN) == LOW) {
            delay(200);
            return DOWN;
        }

        else if (digitalRead(CANCEL) == LOW) {
            delay(200);
            return CANCEL;
        }

        else if (digitalRead(OK) == LOW) {
            delay(200);
            return OK;
        }

        update_time();
    }
}

// function to navigate through the menu
void go_to_menu(void) {
    while (true) {
        display.clearDisplay();
        if (current_mode == 0){
            print_line_inverted(options[0], 1, 0, 0);
            print_line(options[1], 1, 12, 0);
            print_line(options[2], 1, 24, 0);
            print_line(options[3], 1, 36, 0);
            print_line(options[4], 1, 48, 0);
            display.display();
        }

        else if (current_mode == 1){
            print_line(options[0], 1, 0, 0);
            print_line_inverted(options[1], 1, 12, 0);
            print_line(options[2], 1, 24, 0);
            print_line(options[3], 1, 36, 0);
            print_line(options[4], 1, 48, 0);
            display.display();
        }
        else if (current_mode == 2){
            print_line(options[0], 1, 0, 0);
            print_line(options[1], 1, 12, 0);
            print_line_inverted(options[2], 1, 24, 0);
            print_line(options[3], 1, 36, 0);
            print_line(options[4], 1, 48, 0);
            display.display();
        }
        else if (current_mode == 3){
            print_line(options[0], 1, 0, 0);
            print_line(options[1], 1, 12, 0);
            print_line(options[2], 1, 24, 0);
            print_line_inverted(options[3], 1, 36, 0);
            print_line(options[4], 1, 48, 0);
            display.display();
        }
        else{
            print_line(options[0], 1, 0, 0);
            print_line(options[1], 1, 12, 0);
            print_line(options[2], 1, 24, 0);
            print_line(options[3], 1, 36, 0);
            print_line_inverted(options[4], 1, 48, 0);
            display.display();
        }

        int pressed = wait_for_button_press();

        if (pressed == DOWN) {
            current_mode += 1;
            current_mode %= max_modes;
            delay(200);
        }

        else if (pressed == UP) {
            current_mode -= 1;
            if (current_mode < 0) {
                current_mode = max_modes - 1;
            }
            delay(200);
        }

        else if (pressed == OK) {
            Serial.println(current_mode);
            delay(200);
            run_mode(current_mode);
        }

        else{
           current_mode = 0;
           break;
        }
    }
}

void run_mode(int mode) {
    if (mode == 0) {
        set_time_zone();
    }

    else if (mode == 1 ) {
        set_alarm(0);
    }
    else if (mode == 3 ) {
        set_alarm(1);
    }

    else if (mode == 2) {
        alarm_hours[0] = 0 ;
        alarm_minutes[0] = 0 ;
        alarm_triggered[0] = true;
        alarm_enabled[0] = false;
        display.clearDisplay();
        print_line("Alarm 1 disabled " , 1, 20, 2);
        display.display();
        delay(1000);
    }
    else {
        alarm_hours[1] = 0 ;
        alarm_minutes[1] = 0 ;
        alarm_triggered[1] = true;
        alarm_enabled[1] = false;
        display.clearDisplay();
        print_line("Alarm 2 disabled " , 1, 20, 2);
        display.display();
        delay(1000);
    }
}

void set_time_zone() {
    int A=0;
    int j = 1;
   
    int temp_hour = utc_hours;
    int temp_minute = utc_minutes;
    
    while (true) {
        display.clearDisplay();
        print_line("Enter hour: " , 1, 0, 2);
        print_line(String(temp_hour), 2, 20, 20);
        print_line(":", 2, 20, 60);
        print_line(String(temp_minute), 2, 20, 75);
        display.display();


        int pressed = wait_for_button_press();
        if (pressed == UP) {
            delay(200);
            temp_hour += 1;
            if (temp_hour > 14){
                temp_hour = -12;
            }

        }

        else if (pressed == DOWN) {
            delay(200);
            temp_hour -= 1;
            if (temp_hour < -12) {
                temp_hour = 14;
            }
        }

        else if (pressed == OK) {
            delay(200);
            utc_hours = temp_hour;
            break;
        }

        else if (pressed == CANCEL) {
            delay(200);
            A = 1;
            break;
        }
    }

    if (A != 1){
        
        while (true) {
            display.clearDisplay();
            print_line("Enter minute: " , 1, 0, 2);
            print_line(String(temp_hour), 2, 20, 20);
            print_line(":", 2, 20, 60);
            print_line(String(temp_minute), 2, 20, 75);
            display.display();
    
            int pressed = wait_for_button_press();
            if (pressed == UP) {
                delay(200);
                j++;
                j = j % 3;
                }
    
            
    
            else if (pressed == DOWN) {
                delay(200);
                j--;

                if (j < 0) {
                    j = 2;
                }
            }
    
            else if (pressed == OK) {
                delay(200);
                utc_minutes = time_zone_minute[j];
                display.clearDisplay();
                print_line("Time Zone is set", 1, 20, 2);
                display.display();
                delay(1000);
                break;
            }
    
            else if (pressed == CANCEL) {
                delay(200);
                break;
            }
            temp_minute = time_zone_minute[j];
        }
}
}
void set_alarm(int alarm) {
    int A = 0 ;
    int temp_hour = alarm_hours[alarm];
    int temp_minute = alarm_minutes[alarm];
    while (true) {
        display.clearDisplay();
        print_line("Enter hour: " , 1, 0, 2);
        print_line(String(temp_hour), 2, 20, 35);
        print_line(":", 2, 20, 60);
        print_line(String(temp_minute), 2, 20, 75);
        display.display();

        
        int pressed = wait_for_button_press();
        if (pressed == UP) {
            delay(200);
            temp_hour += 1;
            temp_hour = temp_hour % 24;
        }
        
        else if (pressed == DOWN) {
            delay(200);
            temp_hour -= 1;
            if (temp_hour < 0) {
                temp_hour = 23;
            }
        }
        
        else if (pressed == OK) {
            delay(200);
            alarm_hours[alarm] = temp_hour;
            break;
        }
        
        else if (pressed == CANCEL) {
            A = 1;
            delay(200);
            break;
        }
    }
    
    if ( A != 1 ){
    while (true) {
        display.clearDisplay();
        print_line("Enter minute: " , 1, 0, 2);
        print_line(String(temp_hour), 2, 20, 35);
        print_line(":", 2, 20, 60);
        print_line(String(temp_minute), 2, 20, 75);
        display.display();

        
        int pressed = wait_for_button_press();
        if (pressed == UP) {
            delay(200);
            temp_minute += 1;
            temp_minute = temp_minute % 60;
        }
        
        else if (pressed == DOWN) {
            delay(200);
            temp_minute -= 1;
            if (temp_minute < 0) {
                temp_minute = 59;
            }
        }
        
        else if (pressed == OK) {
            delay(200);
            alarm_minutes[alarm] = temp_minute;
            alarm_enabled[alarm] = true;
            alarm_triggered[alarm] = false;
            
            display.clearDisplay();
            print_line("Alarm is set", 1, 20, 2);
            display.display();
            delay(1000);
            break;
        }
        
        else if (pressed == CANCEL) {
            delay(200);
            break;
        }
    }

}
}

// check_temp function
void check_temp(void) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity(); 

    bool all_good = true;

    display.setTextSize(1);

    if (temperature > 32) {
        all_good = false;
        digitalWrite(LED_2, HIGH);
        print_line("Temp:HIGH", 1, 40, 0);
    }
    else if (temperature < 24) {
        all_good = false;
        digitalWrite(LED_2, HIGH);
        print_line("Temp:LOW", 1, 40, 0);
    }
    if (humidity > 80) {
        all_good = false;
        digitalWrite(LED_2, HIGH);
        print_line("Hum :HIGH", 1, 50, 0);
    }
    else if (humidity < 65) {
        all_good = false;
        digitalWrite(LED_2, HIGH);
        print_line("Hum :LOW", 1, 50, 0);
    }
    if (all_good) {
        digitalWrite(LED_2, LOW);
    }
}