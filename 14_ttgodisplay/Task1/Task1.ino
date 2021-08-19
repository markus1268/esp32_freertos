#include <TFT_eSPI.h>
 
TaskHandle_t Handle_aTask;
TaskHandle_t Handle_bTask;
TaskHandle_t Handle_monitorTask;
 
TFT_eSPI tft = TFT_eSPI(135, 240);
TFT_eSprite img = TFT_eSprite(&tft);
TFT_eSprite img2 = TFT_eSprite(&tft);
 
static void LCD_TASK_1 (void* pvParameters) {
    Serial.println("Thread A: Started");
    img.createSprite(100, 100);
    img.fillSprite(tft.color565(229,58,64)); // RED
    img.setTextSize(8);
    img.setTextColor(TFT_WHITE);
    for(int i = 0; i < 100; i++) {
        img.drawNumber(i, 10, 25);
        img.pushSprite(30, 70);
        img.fillSprite(tft.color565(229,58,64));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(i== 99) i = 0;
    }
}
 
static void LCD_TASK_2 (void* pvParameters) {
    Serial.println("Thread B: Started");
    img2.createSprite(100, 100);
    img2.fillSprite(tft.color565(48,179,222)); // BLUE
    img2.setTextSize(8);
    img2.setTextColor(TFT_WHITE);
    for(int x = 99; x >= 0; x--) {
        img2.drawNumber(x, 10, 25);
        img2.pushSprite(190, 70);
        img2.fillSprite(tft.color565(48,179,222));
        vTaskDelay(500 / portTICK_PERIOD_MS);
        if(x== 0) x = 99;
    }
}
 
void taskMonitor(void* pvParameters) {
    int x;
    int measurement;
 
    Serial.println("Task Monitor: Started");
 
    // run this task a few times before exiting forever
    for (x = 0; x < 10; ++x) {
 
        Serial.println("");
        Serial.println("******************************");
        Serial.println("[Stacks Free Bytes Remaining] ");
 
        measurement = uxTaskGetStackHighWaterMark(Handle_aTask);
        Serial.print("Thread A: ");
        Serial.println(measurement);
 
        measurement = uxTaskGetStackHighWaterMark(Handle_bTask);
        Serial.print("Thread B: ");
        Serial.println(measurement);
 
        measurement = uxTaskGetStackHighWaterMark(Handle_monitorTask);
        Serial.print("Monitor Stack: ");
        Serial.println(measurement);
 
        Serial.println("******************************");
 
        vTaskDelay(10000 / portTICK_PERIOD_MS); // print every 10 seconds
    }
 
    // delete ourselves.
    // Have to call this or the system crashes when you reach the end bracket and then get scheduled.
    Serial.println("Task Monitor: Deleting");
    vTaskDelete(NULL);
}
 
void setup() {
 
    Serial.begin(115200);
 
    //vNopDelayMS(1000); // prevents usb driver crash on startup, do not omit this
    //while (!Serial) ;  // Wait for Serial terminal to open port before starting program
 
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(tft.color565(9,7,7)); // BLACK background
    tft.setTextColor(tft.color565(239,220,5)); // YELLOW text
    tft.setTextSize(2);
    tft.drawString("Thread A", 30, 50);
    tft.drawString("Thread B", 190, 50);
 
    Serial.println("");
    Serial.println("******************************");
    Serial.println("        Program start         ");
    Serial.println("******************************");
 
    // Create the threads that will be managed by the rtos
    // Sets the stack size and priority of each task
    // Also initializes a handler pointer to each task, which are important to communicate with and retrieve info from tasks
    xTaskCreate(LCD_TASK_1,     "Task A",       1024, NULL, tskIDLE_PRIORITY + 3, &Handle_aTask);
    xTaskCreate(LCD_TASK_2,     "Task B",       1024, NULL, tskIDLE_PRIORITY + 2, &Handle_bTask);
    xTaskCreate(taskMonitor, "Task Monitor",    1024, NULL, tskIDLE_PRIORITY + 1, &Handle_monitorTask);
 
    // Start the RTOS, this function will never return and will schedule the tasks.
    vTaskStartScheduler();
}
 
void loop() {
    //NOTHING
}
