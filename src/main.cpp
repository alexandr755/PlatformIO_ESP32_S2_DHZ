
#include <Arduino.h>
#include <UniversalTelegramBot.h> // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <Ticker.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <GyverTimer.h> // подключаем библиотеку
#include "Wire.h"
#include <Adafruit_AHTX0.h>
#include "INA226.h"
#include "GyverTM1637.h"
#define LED 15  // 8-ый выходной цифровой контакт LED
#define WIFI_SSID "TP-LINK_17"
#define WIFI_PASS "rob17rob17"
#define WIFI_SSID2 "Wi-Fi-Rab"
#define WIFI_PASS2 "kamsk1975"
///TM display
#define CLK 2
#define DIO 1
GyverTM1637 disp(CLK, DIO);
uint32_t Now, clocktimer;
boolean flag;
//
// активация датчика AHT10
Adafruit_AHTX0 aht;
Adafruit_Sensor *aht_humidity, *aht_temp;
// активация датчика INA226
INA226 INA(0x40);
float BusVoltage;
//
GTimer myTimer(MS);     // создать миллисекундный таймер
// put function declarations here:

const unsigned int configSTACK = 40960;
TaskHandle_t sshHandle = NULL;
QueueHandle_t queue_1;

///////////
//IPAddress local_IP(192, 168, 17, 16);
//IPAddress gateway(192, 168, 17, 1);
//IPAddress subnet(255, 255, 255, 0);
//IPAddress primaryDNS(8, 8, 8, 8);
//IPAddress secondaryDNS(8, 8, 4, 4);

// Create a WiFiMulti object
WiFiMulti WiFiMulti;

//void sshTask(void* pvParameter);
void getVoltsCurrent();
void getTemPHum();
void check_AHT10andINA226()
{
  /// проверка есть ли датчик temp@him
  Serial.println("Adafruit AHT10/AHT20 test!");
  if (!aht.begin())
  {
    Serial.println("Failed to find AHT10/AHT20 chip");
    while (1)
    {
      delay(10);
    }
  }
  Serial.println("AHT10/AHT20 Found!");
  aht_temp = aht.getTemperatureSensor();
  aht_temp->printSensorDetails();

  aht_humidity = aht.getHumiditySensor();
  aht_humidity->printSensorDetails();
    /// проверка есть ли датчик INA226
  Serial.print("SDA connect to - ");
  Serial.println(SDA);
  Serial.print("SCL connect to - ");
  Serial.println(SCL);
  Serial.println(__FILE__);
  Serial.print("INA226_LIB_VERSION: ");
  Serial.println(INA226_LIB_VERSION);
  Wire.begin();

  if (!INA.begin())
  {
    Serial.println("could not connect. Fix and Reboot");
  }
  INA.setMaxCurrentShunt(1, 0.002);
  INA.setBusVoltageConversionTime(7);
  
  //////

}


void fadeBlink() {
  // пишем HELL
  disp.displayByte(_H, _E, _L, _L);

  Now = millis();
  while (millis () - Now < 3000) {    // 3 секунды
    for (int i = 7; i > 0; i--) {
      disp.brightness(i);   // меняем яркость
      delay(40);
    }
    for (int i = 0; i < 8; i++) {
      disp.brightness(i);   // меняем яркость
      delay(40);
    }
  }
}

void showFloat(GyverTM1637 &disp, float num) {
  int integer = abs((int)num);
  int decimal = abs((int)(num * 10) % 10);
  
  if (num < 0) disp.display(0, 10);  // Минус
  else disp.display(0, integer / 10);
  
  disp.display(1, integer % 10);
  disp.display(2, decimal);
  disp.point(1);
}

void Task1code( void *pvParameters) 
{
  pinMode(LED, OUTPUT); 
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  while(1) { //infinite loop
  digitalWrite(LED, HIGH);
  vTaskDelay( 500 / portTICK_PERIOD_MS );
  digitalWrite(LED, LOW);
  vTaskDelay( 500 / portTICK_PERIOD_MS );
   };
  // Сюда мы не должны добраться никогда. Но если "что-то пошло не так" - нужно всё-таки удалить задачу из памяти
  vTaskDelete(NULL);
} 



// Функция задачи 1
void task_print_temp(void *pvParameters)
{
  float temp_celsius = 0;
  // Организуем бесконечный цикл
  while(1) {
    // Выводим сообщение в терминал
    Serial.println("Task_print_temp");
    if (xQueueReceive(queue_1, &temp_celsius, portMAX_DELAY) == pdPASS) {
    Serial.print("Temp from queue :");
    Serial.println(temp_celsius);
    } 
    vTaskDelay( 1000 / portTICK_PERIOD_MS ); // ждать одну секунду
    // Пауза 10 секунд
    //vTaskDelay(pdMS_TO_TICKS(10000)); //100cek = 100000 / 1s = 1000ms 
  };
  // Сюда мы не должны добраться никогда. Но если "что-то пошло не так" - нужно всё-таки удалить задачу из памяти
  vTaskDelete(NULL);
}

// Функция чтение датчика температуры встроенного
void task_read_temp(void *pvParameters)
{
  // Организуем бесконечный цикл
  while(1) {
    // Выводим сообщение в терминал
  Serial.print("READ chip temp: ");
  float temp_celsius = temperatureRead();
  Serial.print("Temp onBoard ");
  Serial.print(temp_celsius);
  Serial.println("°C");  
    Serial.println();
    //ESP_LOGI("TASK11", "Task 1 executed");
    //Serial.print("READ_temp_task running on core ");
    //Serial.println(xPortGetCoreID());
    // Пауза 10 секунд
    //vTaskDelay(pdMS_TO_TICKS(10000)); //100cek = 100000 / 1s = 1000ms
    xQueueSend(queue_1, &temp_celsius, portMAX_DELAY);
    vTaskDelay( 50000 / portTICK_PERIOD_MS ); 
  };
  // Сюда мы не должны добраться никогда. Но если "что-то пошло не так" - нужно всё-таки удалить задачу из памяти
  vTaskDelete(NULL);
}

void Read_AHT10_INA226( void *pvParameters) 
{
  Serial.print("Task1Read_AHT10_INA226 ");
    while(1) { //infinite loop
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
      BusVoltage = INA.getBusVoltage() - (INA.getBusVoltage() / 100 * 5.1);
      //if (temp.temperature > 23 || humidity.relative_humidity > 60 || BusVoltage < 21)
      Serial.print("Temp: ");
      Serial.print(temp.temperature);
      Serial.print(" C");
      Serial.print("\t\t");
      Serial.print("Humidity: ");
      Serial.print(humidity.relative_humidity);
      Serial.println(" \%");
      Serial.print("BusVoltage = ");
      Serial.println(BusVoltage);
      showFloat(disp,temp.temperature);  // Вывод дробных чисел trmperature
      disp.displayByte(3, _t);
      delay(3000);
      showFloat(disp,humidity.relative_humidity);  // Вывод дробных чисел trmperature
      disp.displayByte(3, _H);
      delay(3000);
      showFloat(disp,BusVoltage);  // Вывод дробных чисел trmperature
      disp.displayByte(3, _U);
      //disp.displayByte(2, _b);
      //disp.displayByte(2, _U);	
      //disp.displayInt((int)temp.temperature);
   vTaskDelay( 11000 / portTICK_PERIOD_MS );
   };
  // Сюда мы не должны добраться никогда. Но если "что-то пошло не так" - нужно всё-таки удалить задачу из памяти
  vTaskDelete(NULL);
} 

void setup() {
  Wire.begin();
  check_AHT10andINA226();
    ////// WIFI Connect
  delay(10);

  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);
  WiFiMulti.addAP(WIFI_SSID2, WIFI_PASS2);

  Serial.print("\n\nWaiting for WiFi... ");

  // WIFI Connection, Reboot after 30 attempts
  uint8_t not_connected_counter = 0;
  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
    not_connected_counter++;
    if (not_connected_counter > 30)
    {
      Serial.println("Resetting due to Wifi not connecting...");
      ESP.restart();
    }
  }

  if (WiFi.SSID() == WIFI_SSID)
  {
    // WiFi.config should after WiFi.begin or wifiMulti.addAP
    Serial.println("\nDHCP IP Configuration");
    //WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  }
  if (WiFi.SSID() == WIFI_SSID2)
  {
    // WiFi.config should after WiFi.begin or wifiMulti.addAP
    Serial.println("\nDHCP IP Configuration");
    // WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("TEst is finished222");
  ///
  // put your setup code here, to run once:
  Serial.println("Setup has finished");
  Serial.print("setup() function running on core: ");
  Serial.println(xPortGetCoreID());
  queue_1 = xQueueCreate(4, sizeof(float));
  if (queue_1 == NULL) {
  Serial.println("Queue can not be created");
  }

  xTaskCreate(task_print_temp,"task_print_temp",10000,NULL,3,NULL);
   delay(500);
  xTaskCreate(Task1code,"TaskLED_BLINK",10000,NULL,1,NULL);
  //xTaskCreatePinnedToCore(Task1code, "Task2", 10000, NULL, 0, NULL,  0);
   delay(500);
  xTaskCreate(Read_AHT10_INA226,"Read_AHT10_INA226",10000,NULL,5,NULL);
   delay(500);
  ///SSH_task_start
  ///xTaskCreatePinnedToCore(sshTask, "ssh-connect", configSTACK, NULL,(tskIDLE_PRIORITY + 3), &sshHandle,portNUM_PROCESSORS); 
  ///delay(500);
  /// READ_temp
  xTaskCreatePinnedToCore(task_read_temp, "task_read_temp", 10000, NULL,(tskIDLE_PRIORITY + 1), NULL,portNUM_PROCESSORS); 
  delay(500);
  //// Init display
  disp.clear();
  disp.brightness(7);  // яркость, 0 - 7 (минимум - максимум)
  ///
  myTimer.setInterval(1200000);    // 300000 настроить интервал 1 мин = 60000ms 5мин
}

void loop() {
  // Empty. Things are done in Tasks.
  // put your main code here, to run repeatedly:
  //Serial.println("Next step");
  //Serial.println("in LOOp ");
  //delay(2000);
  //Serial.println(xPortGetCoreID());
  //delay(1000);
 
  if (myTimer.isReady())
  { // Timer is complite
  //ints();
  //scrolls();
  //fadeBlink();
  getVoltsCurrent();
  getTemPHum();

  }
 
}

void getTemPHum()
{
  //  /* Get a new normalized sensor event */
  sensors_event_t humidity;
  sensors_event_t temp;
  aht_humidity->getEvent(&humidity);
  aht_temp->getEvent(&temp);

  Serial.print("\t\tTemperature ");
  Serial.print(temp.temperature);
  Serial.println(" deg C");

  /* Display the results (humidity is measured in % relative humidity (% rH) */
  Serial.print("\t\tHumidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println(" % rH");
  Serial.print("\t\tTemperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degrees C");

  delay(100);

  /*//   serial plotter friendly format
  Serial.print(temp.temperature);
  Serial.print(",");

  Serial.print(humidity.relative_humidity);

  Serial.println();
  delay(10);
  */
}

void getVoltsCurrent()
{
  Serial.println("\nBUS\tSHUNT\tCURRENT\tPOWER");
  for (int i = 0; i < 1; i++)
  {
    Serial.print(INA.getBusVoltage(), 3);
    Serial.print("\t");
    Serial.print(INA.getShuntVoltage_mV(), 3);
    Serial.print("\t");
    Serial.print(INA.getCurrent_mA(), 3);
    Serial.print("\t");
    Serial.print(INA.getPower_mW(), 3);
    Serial.println();
    delay(1000);
  }
}

/* add comment
void sshTask(void* pvParameter) {
  SSH ssh{};

  // Pick one of the ssh authentication methods to connect
  Serial.println("SSH Connecting to server...");

  // With password (in server side create password for the user and allow
  // password authentication in /etc/ssh/sshd_config)
  ssh.connectWithPassword("192.168.3.185", "test","kamsk@#64");

  // With public key
  // ssh.connectWithKey("101.102.103.104", "ubuntu","/spiffs/key1.pub","/spiffs/key1");
  // With public key, encrypted private key
  // ssh.connectWithKey("192.168.1.200", "hpirila","/spiffs/key2.pub","/spiffs/key2","MyPassPhrase");

  if (ssh.isConnected) {
    Serial.println("SSH is connected!\n");

    Serial.println("Lets create a test file in server");
    ssh.sendCommand("echo \"This is a test file for ESP32-S Arduino SSH wrapper class\" > testFile1");

    Serial.println("Copying testFile1 from server to ESP32 spiffs file system");
    ssh.scpGetFile("testFile1", "/spiffs/testFile1");

    Serial.println("Copying testFile1 from ESP32 back to server with new name testFile2");
    ssh.scpPutFile("/spiffs/testFile1", "testFile2");

    Serial.println("Compare testFile1 and testFile2 and print the result to result.txt\n");
    Serial.println("Login to server and cat result.txt");
    Serial.println("It should say testFile1 and testFile2 are identical.\n");
    ssh.sendCommand("diff -s testFile1 testFile2 > result.txt");

    // ssh.scpGetFile("key1", "/spiffs/key1");
    // ssh.scpGetFile("key1.pub", "/spiffs/key1.pub");
    // ssh.scpGetFile("key2", "/spiffs/key2");
    // ssh.scpGetFile("key2.pub", "/spiffs/key2.pub");

  } else {
    Serial.println("SSH connection failed.");
  }

  Serial.println("Close ssh connection");
  ssh.end();
  Serial.println("Kill ssh task");
  vTaskDelete(NULL);
}
*/