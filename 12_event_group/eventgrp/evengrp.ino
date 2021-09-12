#if CONFIG_FREERTOS_UNICORE
static BaseType_t app_cpu = 0;
#else
static BaseType_t app_cpu = 1;
#endif

#define GPIO_LED 12
#define GPIO_LED2 14
#define GPIO_LED3 27

// define three event flag bit variable
#define TASK1_BIT (1UL << 0UL) // zero shift for bit0
#define TASK2_BIT (1UL << 1UL) // 1 shift for flag  bit 1
#define TASK3_BIT (1UL << 2UL) // 2 shift for flag bit 2

//  declare a event grounp handler variable
EventGroupHandle_t xEventGroup;

static TaskHandle_t htask1;
// Settings
static const uint16_t timer_divider = 80; // Count at 1 MHz
static const uint64_t timer_max_count = 1000000;
static hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer()
{
    BaseType_t task_woken = pdFALSE;

    xEventGroupSetBitsFromISR(xEventGroup, TASK3_BIT, &task_woken);

    if (task_woken)
    {
        portYIELD_FROM_ISR();
    }
}

static void task1(void *arg)
{
    const EventBits_t xBitsToWaitFor = (TASK1_BIT | TASK2_BIT | TASK3_BIT);
    EventBits_t xEventGroupValue;

    for (;;)
    {
        xEventGroupValue = xEventGroupWaitBits(xEventGroup, xBitsToWaitFor, pdTRUE, pdTRUE, portMAX_DELAY);
        printf("eventgrp: %d\n", xEventGroupValue);
        if ((xEventGroupValue & TASK1_BIT) != 0)
        {
            digitalWrite(GPIO_LED, digitalRead(GPIO_LED) ^ HIGH);
            printf("  loop() notified this task.\n");
        }
        if ((xEventGroupValue & TASK2_BIT) != 0)
        {
            digitalWrite(GPIO_LED2, digitalRead(GPIO_LED2) ^ HIGH);
            printf("  task2() notified this task.\n");
        }
        if ((xEventGroupValue & TASK3_BIT) != 0)
        {
            digitalWrite(GPIO_LED3, digitalRead(GPIO_LED3) ^ HIGH);
            printf("  timer intr notified this task.\n");
        }
    }
}

static void task2(void *arg)
{
    //unsigned count;

    //for (;; count += 100u)
    for (;;)
    {
        //delay(500 + count);
        delay(3000);
        xEventGroupSetBits(xEventGroup, TASK2_BIT);
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
    printf("evengrp.ino:\n");
    xEventGroup = xEventGroupCreate();

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
    delay(2000);
    xEventGroupSetBits(xEventGroup, TASK1_BIT);
}