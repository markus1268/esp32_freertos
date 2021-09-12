#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static SemaphoreHandle_t one_Hz_sem;
static QueueHandle_t data_q_handle;
static QueueSetHandle_t data_or_sem_qs_handle;

void producer_1Hz_sem(void *p)
{
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xSemaphoreGive(one_Hz_sem);
    }
}

void producer_data(void *p)
{
    int x = 0;

    while (1)
    {
        ++x;
        xQueueSend(data_q_handle, &x, 0);
        vTaskDelay((rand() % 1000) + 100);
    }
}

void processor(void *p)
{
    int samples[10];
    int count = 0;

    while (1)
    {
        QueueSetMemberHandle_t who_unblocked = xQueueSelectFromSet(data_or_sem_qs_handle, 2000);
        if (who_unblocked == one_Hz_sem) {
            if (xSemaphoreTake(one_Hz_sem, 0)) {
                float avg = 0;
                for (int i = 0; i < count; i++) {
                    avg += samples[i];
                }
                avg /= count;
                count = 0;
                Serial.print("One Hz Average of Samples = ");
                Serial.println(avg);
            }
            else {
                Serial.println("Should never happen");
            }

        }
        else if (who_unblocked == data_q_handle) {
            int x;
            if (xQueueReceive(data_q_handle, &x, 0)) {
                samples[count++] = x;
                Serial.print("Retrieve: ");
                Serial.println(x);
            }
            else {
                Serial.println("Should never happen");
            }
        }
        else {
            Serial.println("Invalid case");
        }
    }
}

void setup()
{

    // Configure Serial
    Serial.begin(115200);

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Queue Sets Demo---");

    one_Hz_sem = xSemaphoreCreateBinary();
    data_q_handle = xQueueCreate(10, sizeof(int));
    data_or_sem_qs_handle = xQueueCreateSet(11);
    xQueueAddToSet(one_Hz_sem, data_or_sem_qs_handle);
    xQueueAddToSet(data_q_handle, data_or_sem_qs_handle);

    // Start print task
    xTaskCreatePinnedToCore(producer_1Hz_sem,
                            "Producer Semaphore",
                            2048,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

    xTaskCreatePinnedToCore(producer_data,
                            "Producer Data",
                            2048,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

    xTaskCreatePinnedToCore(processor,
                            "Processor",
                            2048,
                            NULL,
                            2,
                            NULL,
                            app_cpu);
}

void loop()
{

    // Wait before trying again
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}