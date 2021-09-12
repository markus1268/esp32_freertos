#include <TM1637Display.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define CLK 18                   //Set the CLK pin connection to the display
#define DIO 23                   //Set the DIO pin connection to the display
TM1637Display display(CLK, DIO); //set up the 4-Digit Display.

// Settings
static const uint8_t msg_queue_len = 5;

// Globals
static QueueHandle_t msg_queue;

//*****************************************************************************
// Tasks

// Task: wait for item on queue and print it
void printMessages(void *parameters)
{

    int item;

    display.setBrightness(3);
    display.clear();

    // Loop forever
    while (1)
    {

        // See if there's a message in the queue (do not block)
        if (xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE)
        {
            Serial.println(item);
            display.showNumberDec(item, true);
        }
        //Serial.println(item);

        // Wait before trying again
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup()
{

    // Configure Serial
    Serial.begin(115200);

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Queue Demo---");

    // Create queue
    msg_queue = xQueueCreate(msg_queue_len, sizeof(int));

    // Start print task
    xTaskCreatePinnedToCore(printMessages,
                            "Print Messages",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);
}

void loop()
{

    static int num = 0;

    // Try to add item to queue for 10 ticks, fail if queue is full
    if (xQueueSend(msg_queue, (void *)&num, 10) != pdTRUE)
    {
        Serial.println("Queue full");
    }
    num++;

    // Wait before trying again
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}