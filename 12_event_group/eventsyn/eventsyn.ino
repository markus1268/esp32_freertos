#if CONFIG_FREERTOS_UNICORE
static BaseType_t app_cpu = 0;
#else
static BaseType_t app_cpu = 1;
#endif

#define GPIO_LED1 12
#define GPIO_LED2 14
#define GPIO_LED3 27

// define three event flag bit variable
#define TASK1_BIT (1UL << 0UL) // zero shift for bit0
#define TASK2_BIT (1UL << 1UL) // 1 shift for flag  bit 1
#define TASK3_BIT (1UL << 2UL) // 2 shift for flag bit 2
#define ALL_SYN_BIT (TASK1_BIT | TASK2_BIT | TASK3_BIT)

//  declare a event grounp handler variable
EventGroupHandle_t xEventGroup;

static TaskHandle_t htask1;

static void task1(void *arg)
{
    for (;;)
    {
        delay(1000);
        xEventGroupSync(xEventGroup, TASK1_BIT, ALL_SYN_BIT, portMAX_DELAY );
        digitalWrite(GPIO_LED1, digitalRead(GPIO_LED1) ^ HIGH);
        printf("  task1() notified this task.\n");
    }
}

static void task2(void *arg)
{
    for (;;)
    {
        delay(2000);
        xEventGroupSync(xEventGroup, TASK2_BIT, ALL_SYN_BIT, portMAX_DELAY );
        digitalWrite(GPIO_LED2, digitalRead(GPIO_LED2) ^ HIGH);
        printf("  task2() notified this task.\n");
    }
}

void setup()
{
    //int app_cpu = 0;
    BaseType_t rc;

    app_cpu = xPortGetCoreID();
    pinMode(GPIO_LED1, OUTPUT);
    digitalWrite(GPIO_LED1, LOW);
    pinMode(GPIO_LED2, OUTPUT);
    digitalWrite(GPIO_LED2, LOW);
    pinMode(GPIO_LED3, OUTPUT);
    digitalWrite(GPIO_LED3, LOW);

    delay(2000); // Allow USB to connect
    printf("eventsyn.ino:\n");
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

}

void loop()
{
    delay(3000);
    xEventGroupSync(xEventGroup, TASK3_BIT, ALL_SYN_BIT, portMAX_DELAY );
    digitalWrite(GPIO_LED3, digitalRead(GPIO_LED3) ^ HIGH);
    printf("  loop() notified this task.\n");
}