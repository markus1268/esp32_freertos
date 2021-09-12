// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

QueueHandle_t xMailbox;
TaskHandle_t TaskHandle_1; // handler for Task1
TaskHandle_t TaskHandle_2; // handler for Task2

void setup()
{
    // Configure Serial
    Serial.begin(115200);

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Mutex Solution---");
    Serial.println("Send something to start mailbox demo: ");

    // Wait for input from Serial
    while (Serial.available() <= 0)
        ;

    xMailbox = xQueueCreate(1, sizeof( int32_t));

    // Start task 1
    xTaskCreatePinnedToCore(vUpdateMailbox,
                            "Sender",
                            1024,
                            NULL,
                            1,
                            &TaskHandle_1,
                            app_cpu);

    xTaskCreatePinnedToCore(vReadMailbox,
                            "Receiver",
                            1024,
                            NULL,
                            1,
                            &TaskHandle_2,
                            app_cpu);

}

void loop()
{

    // Do nothing but allow yielding to lower-priority tasks
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void vUpdateMailbox(void *parameters)
{
    int32_t ulNewValue = 1;
    while (1)
    {
        xQueueOverwrite(xMailbox, &ulNewValue);
        Serial.println("Data written to mailbox");
        ulNewValue++;
        vTaskDelay(500);
    }
}

void vReadMailbox(void *parameters)
{
    int32_t value_received;
    while (1)
    {
        xQueuePeek(xMailbox, &value_received, portMAX_DELAY);
        Serial.print("Data Read from mailbox = ");
        Serial.println(value_received);
        vTaskDelay(100);
    }
}