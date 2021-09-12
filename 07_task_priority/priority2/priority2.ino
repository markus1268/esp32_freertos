#if CONFIG_FREERTOS_UNICORE
static BaseType_t app_cpu = 0;
#else
static BaseType_t app_cpu = 1;
#endif

void task1(void *argp)
{

    printf("Task1 executing, priority %u.\n",
           (unsigned)uxTaskPriorityGet(nullptr));
    vTaskDelete(nullptr);
}

void setup()
{
    BaseType_t rc;
    TaskHandle_t h1;

    app_cpu = xPortGetCoreID();

    delay(2000); // Allow USB init time

    printf("\ntaskcreate2.ino:\n");
    printf("loopTask priority is %u.\n",
           (unsigned)uxTaskPriorityGet(nullptr));

    rc = xTaskCreatePinnedToCore(
        task1,
        "task1",
        2000, // Stack size
        nullptr,
        0,      // Priority
        &h1,    // Task handle
        app_cpu // CPU
    );
    assert(rc == pdPASS);

    printf("Task1 created.\n");

    vTaskSuspend(h1);
    vTaskPrioritySet(h1, 3);

    printf("Zzzz... 3 secs\n");
    delay(3000);

    vTaskResume(h1);
}

// Not used:
void loop()
{
    vTaskDelete(nullptr);
}