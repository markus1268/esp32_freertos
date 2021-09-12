#if CONFIG_FREERTOS_UNICORE
static BaseType_t app_cpu = 0;
#else
static BaseType_t app_cpu = 1;
#endif

#define GPIO_LED 12
#define GPIO_LED2 14
#define GPIO_LED3 27

static TaskHandle_t htask1;
// Settings
static const uint16_t timer_divider = 80; // Count at 1 MHz
static const uint64_t timer_max_count = 1000000;
static hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer()
{
    BaseType_t task_woken = pdFALSE;
    
    xTaskNotifyFromISR(htask1, 0b0100, eSetBits, &task_woken);

    if (task_woken)
    {
        portYIELD_FROM_ISR();
    }
}

static void task1(void *arg)
{
    uint32_t rv;
    BaseType_t rc;

    for (;;)
    {
        rc = xTaskNotifyWait(0, 0b0111, &rv, portMAX_DELAY);
        printf("Task notified: rv=%u\n", unsigned(rv));
        if (rv & 0b0001) {
            digitalWrite(GPIO_LED, digitalRead(GPIO_LED) ^ HIGH);
            printf("  loop() notified this task.\n");
        }
        if (rv & 0b0010) {
            digitalWrite(GPIO_LED2, digitalRead(GPIO_LED2) ^ HIGH);
            printf("  task2() notified this task.\n");
        }
        if (rv & 0b0100) {
            digitalWrite(GPIO_LED3, digitalRead(GPIO_LED3) ^ HIGH);
            printf("  timer intr notified this task.\n");
        }
    }
}

static void task2(void *arg)
{
    unsigned count;
    BaseType_t rc;

    for (;; count += 100u)
    {
        delay(500 + count);
        rc = xTaskNotify(htask1, 0b0010, eSetBits);
        assert(rc == pdPASS);
    }
}

void setup()
{
    //int app_cpu = 0;
    BaseType_t rc;

    app_cpu = xPortGetCoreID();
    pinMode(GPIO_LED, OUTPUT);
    digitalWrite(GPIO_LED, LOW);
    pinMode(GPIO_LED2, OUTPUT);
    digitalWrite(GPIO_LED2, LOW);
    pinMode(GPIO_LED3, OUTPUT);
    digitalWrite(GPIO_LED3, LOW);

    delay(2000); // Allow USB to connect
    printf("tasknfy4.ino:\n");

    rc = xTaskCreatePinnedToCore(
        task1,   // Task function
        "task1", // Name
        3000,    // Stack size
        nullptr, // Parameters
        1,       // Priority
        &htask1, // handle
        app_cpu  // CPU
    );
    assert(rc == pdPASS);

    rc = xTaskCreatePinnedToCore(
        task2,   // Task function
        "task2", // Name
        3000,    // Stack size
        nullptr, // Parameters
        1,       // Priority
        nullptr, // no handle
        app_cpu  // CPU
    );
    assert(rc == pdPASS);

    // Create and start timer (num, divider, countUp)
    timer = timerBegin(0, timer_divider, true);

    // Provide ISR to timer (timer, function, edge)
    timerAttachInterrupt(timer, &onTimer, true);

    // At what count should ISR trigger (timer, count, autoreload)
    timerAlarmWrite(timer, timer_max_count, true);

    // Allow ISR to trigger
    timerAlarmEnable(timer);

}

void loop()
{
    BaseType_t rc;

    delay(500);
    rc = xTaskNotify(htask1, 0b0001, eSetBits);
    assert(rc == pdPASS);
}