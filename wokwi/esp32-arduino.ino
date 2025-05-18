//220585R sathsara h.m.w.c.

#include <Adafruit_GFX.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <ESP32Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Define constants for the pins
#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 35
#define PB_DOWN 33
#define DHTPIN 12
#define LED_2 2
#define LDR1 A0
#define LDR2 A3
#define SERVMO 18

float GAMMA = 0.75;
const float RL10 = 50;
float MIN_ANGLE = 30;
float lux = 0;
String luxName;
int Angle_Offset = 30;
float Controlling_Factor = 0.75;

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     19800  // 5 hours 30 minutes = 19800 seconds
#define UTC_OFFSET_DST 0

char tempAr[6];
char lightAr[6];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

bool isScheduledON = false;
unsigned long scheduledOnTime;

// Define global variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
long utc_offset = 19800;  // Global variable, default to 5h30m (IST)

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int num_alarms = 2;
int alarm_hours[] = {12,12};
int alarm_minutes[] = {52,54};
bool alarm_triggered[] = {false, false};

// Buzzer tone parameters
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 348;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 7;
String modes[] = {"1 - Set Time Zone", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Disable Alarm","5 - View Alarms","6 - Delete Alarm","7 - Set Time"};

// Declare Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

//servo moter initializing position 
int pos = 0;
Servo servo; 

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for serial monitor

  // Initialize display first
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while (1);  // Halt if display fails
  }
  display.display();
  delay(1000);

  // Show welcome message
  display.clearDisplay();
  print_line("Welcome to Medibox!", 10, 20, 2);
  display.display();
  delay(2000);

  // Initialize WiFi
  setupWifi();

  // Initialize time configuration
  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);
  
  // Initialize NTP client with correct offset
  timeClient.begin();
  timeClient.setTimeOffset(utc_offset);
  timeClient.update();

  // Initialize MQTT
  setupMqtt();

  // Initialize DHT sensor
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  // Set pin modes
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);
  servo.attach(SERVMO, 500, 2400);
}

void loop() {
  if(!mqttClient.connected()){
    connectToBrocker();
  }
  mqttClient.loop();

  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW){
    delay(200);
    go_to_menu();
  }
  
  check_temp();
  updateTemperature();
  updateLightIntensity();
  rotateMotor();
  
  Serial.println(tempAr);
  Serial.println(lightAr);
  mqttClient.publish("CSE-ADMIN-TEMP", tempAr);
  mqttClient.publish("CSE-ADMIN-LIGHT-INTENSITY", lightAr);
  
  checkSchedule();
  delay(500);
}

void setupWifi() {
  Serial.println("Connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WiFi", 0, 0, 2);
    display.display();
    
  }

  display.clearDisplay();
  print_line("Connected to WiFi", 0, 0, 2);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.display();
  delay(1000);
}

void updateTemperature(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature,2).toCharArray(tempAr,6);
}

void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void connectToBrocker(){
  while(!mqttClient.connected()){
    Serial.print("Attempting MQTT connection ...");
    if(mqttClient.connect("ESP32-123456789")){
      Serial.println("connected");
      mqttClient.subscribe("CSE-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("CSE-ADMIN-SCH-ON");
      mqttClient.subscribe("CSE-ADMIN-ANGLE-OFFSET");
      mqttClient.subscribe("CSE-ADMIN-CONTROLLING_FACTOR");
    }
    else{
      Serial.print("failed");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void receiveCallback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length + 1]; // +1 for null terminator
  for(int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  payloadCharAr[length] = '\0'; // Add null terminator

  Serial.println();

  if (strcmp(topic,"CSE-ADMIN-MAIN-ON-OFF")==0){  
      buzzerOn(payloadCharAr[0]=='1');
  }
  else if (strcmp(topic,"CSE-ADMIN-SCH-ON")==0){
      if (payloadCharAr[0]=='N'){
        isScheduledON = false;
      }else if(payloadCharAr[0]=='1'){
        isScheduledON = true;
        scheduledOnTime = atol(payloadCharAr);
      }
  }
  else if(strcmp(topic, "CSE-ADMIN-ANGLE-OFFSET") == 0){
    Angle_Offset = atoi(payloadCharAr);
    Serial.println(Angle_Offset);
  }
  else if(strcmp(topic, "CSE-ADMIN-CONTROLLING_FACTOR") == 0){
    Controlling_Factor = atof(payloadCharAr);
    Serial.println(Controlling_Factor);
  }
}

unsigned long getTime() {
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule(){
  if(isScheduledON){
    unsigned long currentTime = getTime();
    if(currentTime > scheduledOnTime){
      buzzerOn(true);
      isScheduledON = false;
      mqttClient.publish("CSE-ADMIN-MAIN-ON-OFF-ESP","1");
      mqttClient.publish("CSE-ADMIN-SCH-ESP-ON","0");
      Serial.println("Scheduled ON");
    }else{
      buzzerOn(false);
    }
  }
}

void buzzerOn(bool on){
  if(on){
    int duration = 1000; // milliseconds

    // Play a short melody when turned on
    for(int i = 0; i < n_notes; i++) {
      tone(BUZZER, notes[i]);
      delay(duration);
      noTone(BUZZER);
      delay(50); // short pause
    }
  } else {
    noTone(BUZZER); // Stop the buzzer
  }
}

void updateLightIntensity() {
  int analogValue1 = analogRead(LDR1); //INDOOR LDR
  int analogValue2 = analogRead(LDR2); //OUTDOOR LDR
  
  // Calculate light intensity for LDR1
  float voltage1 = analogValue1 / 4095.0 * 3.3; // ESP32 has 12-bit ADC (0-4095) and 3.3V reference
  float resistance1 = 2000.0 * voltage1 / (1.0 - voltage1 / 3.3);
  float lux1_raw = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance1, (1 / GAMMA));
  
  // Calculate light intensity for LDR2
  float voltage2 = analogValue2 / 4095.0 * 3.3; // ESP32 has 12-bit ADC (0-4095) and 3.3V reference
  float resistance2 = 2000.0 * voltage2 / (1.0 - voltage2 / 3.3);
  float lux2_raw = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance2, (1 / GAMMA));
  
  // Normalize lux values to range 0-1
  // Using predefined minimum and maximum possible lux values for your sensor setup
  const float MIN_LUX = 0.0;                                             // Minimum possible lux value
  const float MAX_LUX = pow(RL10 * 1e3 * pow(10, GAMMA) / 100.0, (1 / GAMMA)); // Assuming 100 ohms as minimum resistance
  
  // Normalize to 0-1 range
  float lux1 = constrain((lux1_raw - MIN_LUX) / (MAX_LUX - MIN_LUX), 0.0, 1.0);
  float lux2 = constrain((lux2_raw - MIN_LUX) / (MAX_LUX - MIN_LUX), 0.0, 1.0);
  
  // Determine the highest intensity
  lux = max(lux1, lux2); // Update global lux variable
  String(lux, 2).toCharArray(lightAr, 6);
  
  Serial.print("Light intensity : ");
  Serial.println(lux);
   
  // Output the highest intensity source
  String highestLDR;
  if (lux1 > lux2) {
    highestLDR = "INDOOR"; // LDR1 has higher intensity
  } else {
    highestLDR = "OUTDOOR"; // LDR2 has higher intensity
  }
  mqttClient.publish("Highest_LDR", highestLDR.c_str());
}

void rotateMotor(){
  // Calculate position using the controlling factor and offset
  pos = Angle_Offset + (180 - Angle_Offset) * lux * Controlling_Factor;
  // Constrain the servo position to valid range
  pos = constrain(pos, Angle_Offset, 180);
  Serial.print("Servo position: ");
  Serial.println(pos);
  servo.write(pos);
}

// Function to print a message at a given position
void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
}

// Function to print the current time on the display using NTPClient
void print_time_now() {
  display.clearDisplay();
  
  timeClient.update();
  
  // Get formatted time string (HH:MM:SS)
  String timeStr = formatTime(timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
  
  // Get formatted date string using epoch time
  String dateStr = formatDate(timeClient.getEpochTime());
  
  // Get day of week from NTPClient
  String dayStr = getWeekDay(timeClient.getDay());

  // Display time (large font)
  print_line(timeStr, 15, 5, 2);  // Centered time display
  
  // Display date (medium font)
  print_line(dateStr, 25, 30, 1);  // Centered date display
  
  // Display day of week (small font)
  print_line(dayStr, 35, 45, 1);   // Centered day display

  // Add a separator line
  display.drawLine(0, 25, SCREEN_WIDTH, 25, SSD1306_WHITE);

  display.display();  // Refresh screen after updating text
}

// Helper function to format time
String formatTime(int h, int m, int s) {
  String hourStr = (h < 10) ? "0" + String(h) : String(h);
  String minStr = (m < 10) ? "0" + String(m) : String(m);
  String secStr = (s < 10) ? "0" + String(s) : String(s);
  return hourStr + ":" + minStr + ":" + secStr;
}

// Helper function to get day of week
String getWeekDay(int day) {
  String weekDays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  return weekDays[day];
}

// Helper function to format date from epoch time
String formatDate(unsigned long epochTime) {
  time_t rawtime = epochTime;
  struct tm * timeinfo;
  char buffer[20];
  
  timeinfo = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%b %d, %Y", timeinfo);
  return String(buffer);
}

// Function to update time
void update_time() {
  timeClient.update();
  
  // Update global time variables
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds = timeClient.getSeconds();
  
  // Get day from epoch time
  time_t rawtime = timeClient.getEpochTime();
  struct tm * timeinfo;
  timeinfo = localtime(&rawtime);
  days = timeinfo->tm_mday;
}

// Function to update time and check if alarm needs to be triggered
void update_time_with_check_alarm(){
  update_time();
  print_time_now();
  delay(1000);

  if(alarm_enabled){
    for (int i = 0; i < num_alarms; i++){
      // Check if current time matches alarm time within a 1-minute window
      if(!alarm_triggered[i] && 
         hours == alarm_hours[i] && 
         abs(minutes - alarm_minutes[i]) <= 1) {
        ring_alarm(i);
        alarm_triggered[i] = true;
      }
      
      // Reset alarm_triggered at midnight
      if(hours == 0 && minutes == 0 && seconds < 5) {
        alarm_triggered[i] = false;
      }
    }
  }
}

// Function to trigger the alarm and play a tone until cancelled
void ring_alarm(int alarm_idx) {
  display.clearDisplay();
  print_line("Medicine Time!", 20, 20, 2);
  update_time();
  display.display(); 

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  //Buzzer ring alarm until user press cancel button
  while (!break_happened && digitalRead(PB_CANCEL) == HIGH && digitalRead(PB_OK) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      } else if (digitalRead(PB_OK) == LOW) {
          delay(200);
          
          // Calculate snooze time (5 minutes ahead)
          int new_minutes = minutes + 5;  
          int new_hours = hours;

          if (new_minutes >= 60) {
              new_minutes -= 60;
              new_hours = (new_hours + 1) % 24;
          }

          alarm_minutes[alarm_idx] = new_minutes;
          alarm_hours[alarm_idx] = new_hours;
          alarm_triggered[alarm_idx] = false;  // Reset the trigger
          

          display.clearDisplay();
          print_line("Snoozed 5 min", 10, 20, 2);  // Updated message
          display.display();
          delay(1000);
          break_happened = true;
          break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  noTone(BUZZER); // Ensure buzzer is turned off
  digitalWrite(LED_1, LOW);
  display.clearDisplay();  // Clear the display after stopping the alarm
  display.display();  // Refresh the screen
}

int wait_for_button_press(){
  while(true){
    if (digitalRead(PB_UP) == LOW){
      delay(200);
      return PB_UP;
    }
    if (digitalRead(PB_DOWN) == LOW){
      delay(200);
      return PB_DOWN;
    }
    if (digitalRead(PB_OK) == LOW){
      delay(200);
      return PB_OK;
    }
    if (digitalRead(PB_CANCEL) == LOW){
      delay(200);
      return PB_CANCEL;
    }
    update_time();
  }
}

void go_to_menu(){
  while (digitalRead(PB_CANCEL) == HIGH){
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);
    display.display();  // Ensure the text appears on the screen
    update_time();

    int pressed = wait_for_button_press();

    if (pressed == PB_UP){
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }
    else if (pressed == PB_DOWN){
      delay(200);
      current_mode -= 1;
      if (current_mode < 0){
        current_mode = max_modes - 1;
      }
    }
    else if (pressed == PB_OK){
      delay(200);
      Serial.println(current_mode);
      run_mode(current_mode);
    }
    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
}

void set_time(){
  int temp_hour = hours;
  while(true){
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    display.display();
    
    int pressed = wait_for_button_press();

    if (pressed == PB_UP){
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }
    else if (pressed == PB_DOWN){
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0){
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK){
      delay(200);
      hours = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
  
  int temp_minutes = minutes;
  while(true){
    display.clearDisplay();
    print_line("Enter minutes: " + String(temp_minutes), 0, 0, 2);
    display.display();
    
    int pressed = wait_for_button_press();

    if (pressed == PB_UP){
      delay(200);
      temp_minutes += 1;
      temp_minutes = temp_minutes % 60;
    }
    else if (pressed == PB_DOWN){
      delay(200);
      temp_minutes -= 1;
      if (temp_minutes < 0){
        temp_minutes = 59;
      }
    }
    else if (pressed == PB_OK){
      delay(200);
      minutes = temp_minutes;
      break;
    }
    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
  
  display.clearDisplay();
  print_line("Time is set", 0, 0, 2);
  display.display();
  delay(1000);
  
  // After setting manual time, adjust the NTP client
  // This is a simplification - in real implementation, you would adjust system time or offset
  timeClient.setTimeOffset(utc_offset);
  timeClient.update();
}

void set_alarm(int alarm){
  int temp_hour = alarm_hours[alarm];
  while(true){
    display.clearDisplay();
    print_line("Enter alarm hour: " + String(temp_hour), 0, 0, 2);
    display.display();
    update_time();

    int pressed = wait_for_button_press();

    if (pressed == PB_UP){
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }
    else if (pressed == PB_DOWN){
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0){
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK){
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
  
  int temp_minutes = alarm_minutes[alarm];
  while(true){
    display.clearDisplay();
    print_line("Enter alarm minutes: " + String(temp_minutes), 0, 0, 2);
    display.display();
    
    int pressed = wait_for_button_press();

    if (pressed == PB_UP){
      delay(200);
      temp_minutes += 1;
      temp_minutes = temp_minutes % 60;
    }
    else if (pressed == PB_DOWN){
      delay(200);
      temp_minutes -= 1;
      if (temp_minutes < 0){
        temp_minutes = 59;
      }
    }
    else if (pressed == PB_OK){
      delay(200);
      alarm_minutes[alarm] = temp_minutes;
      break;
    }
    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
  
  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  display.display();
  delay(1000);
  
  // Reset the alarm_triggered for this alarm
  alarm_triggered[alarm] = false;
  // Ensure alarm is enabled
  alarm_enabled = true;
}

void view_alarms() {
  display.clearDisplay();
  print_line("Active Alarms:", 0, 0, 1);
  update_time();
  
  for (int i = 0; i < num_alarms; i++) {
    // Format time with leading zeros
    String alarmTime = (alarm_hours[i] < 10 ? "0" : "") + String(alarm_hours[i]) + ":" + 
                      (alarm_minutes[i] < 10 ? "0" : "") + String(alarm_minutes[i]);
    
    String alarmStatus = alarm_enabled ? "ON" : "OFF";
    String alarm_text = "Alarm " + String(i+1) + ": " + alarmTime + " " + alarmStatus;
    
    print_line(alarm_text, 0, 15 + (i * 15), 1);
  }
  
  display.display();
  
  // Wait for any button press to exit
  while (digitalRead(PB_CANCEL) == HIGH && digitalRead(PB_OK) == HIGH) {
    delay(100);
  }
  delay(200); // Debounce
}

void delete_alarm() {
  if (num_alarms <= 0) {
    display.clearDisplay();
    print_line("No alarms to delete", 0, 20, 1);
    display.display();
    delay(2000);
    return;
  }
  
  int selected_alarm = 0;
  while (true) {
    display.clearDisplay();
    print_line("Delete Alarm:", 0, 0, 1);
    print_line("> Alarm " + String(selected_alarm + 1) + ": " + 
               String(alarm_hours[selected_alarm]) + ":" + 
               (alarm_minutes[selected_alarm] < 10 ? "0" : "") + 
               String(alarm_minutes[selected_alarm]), 0, 20, 1);
    display.display();

    int pressed = wait_for_button_press();

    if (pressed == PB_UP || pressed == PB_DOWN) {
      selected_alarm = (selected_alarm + 1) % num_alarms;
    } else if (pressed == PB_OK) {
      // Shift remaining alarms
      for (int i = selected_alarm; i < num_alarms - 1; i++) {
        alarm_hours[i] = alarm_hours[i + 1];
        alarm_minutes[i] = alarm_minutes[i + 1];
        alarm_triggered[i] = alarm_triggered[i + 1];
      }

      num_alarms--; // Reduce total number of alarms

      display.clearDisplay();
      print_line("Alarm deleted", 10, 20, 1);
      display.display();
      delay(2000);
      break;
    } else if (pressed == PB_CANCEL) {
      break;
    }
  }
}

void set_time_zone() {
  long temp_offset = utc_offset / 3600;  // Convert to hours for simplicity
  while (true) {
    display.clearDisplay();
    print_line("UTC Offset (h): " + String(temp_offset), 0, 0, 2);
    display.display();

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      temp_offset = (temp_offset + 1) % 24;  // Max 24-hour offset
    } else if (pressed == PB_DOWN) {
      temp_offset = (temp_offset - 1);
      if (temp_offset < -12) temp_offset = 12;  // Min -12-hour offset
    } else if (pressed == PB_OK) {
      utc_offset = temp_offset * 3600;  // Convert back to seconds
      
      // Update time configuration with new offset
      configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);
      
      // Update NTP client with new offset
      timeClient.setTimeOffset(utc_offset);
      timeClient.update();
      
      display.clearDisplay();
      print_line("Time Zone Set", 0, 0, 2);
      display.display();
      delay(1000);
      break;
    } else if (pressed == PB_CANCEL) {
      break;
    }
  }
}

void run_mode(int mode){
  if (mode == 0){
    set_time_zone();
  }
  else if (mode == 1 || mode == 2){
    set_alarm(mode - 1);
  }
  else if (mode == 3){
    alarm_enabled = !alarm_enabled; // Toggle alarm state
    display.clearDisplay();
    print_line(alarm_enabled ? "Alarms Enabled" : "Alarms Disabled", 0, 20, 1);
    display.display();
    delay(1000);
  }
  else if (mode == 4){
    view_alarms();
  }
  else if (mode == 5){
    delete_alarm();
  }
  else if (mode == 6){
    set_time();
  }
}

void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  
  bool temp_warning = false;
  bool humid_warning = false;
  
  String temp_msg = "Temp: ";
  String humid_msg = "Humid: ";

  // Check temperature status
  if (data.temperature < 24) {
    temp_warning = true;
    temp_msg += String(data.temperature, 1) + "C LOW!";
  } else if (data.temperature > 32) {
    temp_warning = true;
    temp_msg += String(data.temperature, 1) + "C HIGH!";
  } 

  // Check humidity status
  if (data.humidity < 65) {
    humid_warning = true;
    humid_msg += String(data.humidity, 1) + "% LOW!";
  } else if (data.humidity > 80) {
    humid_warning = true;
    humid_msg += String(data.humidity, 1) + "% HIGH!";
  } 

  // If there's a warning, display it
  if (temp_warning || humid_warning) {
    display.clearDisplay();
    print_line("WARNING!", 20, 10, 2);
    
    if (temp_warning){
      print_line(temp_msg, 10, 30, 1);
    }
    if(humid_warning){
      print_line(humid_msg, 10, 40, 1);
    }
    display.display();

    if (temp_warning || humid_warning) {
      digitalWrite(LED_2, HIGH);
      delay(500);
      digitalWrite(LED_2, LOW);
    }
  }
}