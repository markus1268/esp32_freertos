#include "freertos/stream_buffer.h"
#include "freertos/message_buffer.h"
//#include "freertos/ringbuf.h"

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Settings
static const uint32_t task_0_delay = 1000; // Time (ms) Task 0 blocks itself
static const uint32_t task_1_delay = 1000;

// Pins
static const int pin_1 = 12; // LED pin

#define NUM_BYTES 100
#define MIN_NUM_BYTES 2

//static StreamBufferHandle_t rxStream = NULL;
static MessageBufferHandle_t rxMessage = NULL;
//static RingbufHandle_t rb;

// Globals

//*****************************************************************************
// Tasks

void doTaskA(void *parameters)
{
    size_t xBytesSent;
    uint8_t ucArrayToSend[] = {'0', '1', '2', '3'};
    char *pcStringToSend = "Hello StreamBuffer";
    const TickType_t x100ms = pdMS_TO_TICKS(100);

    // Do forever
    while (1)
    {
        xBytesSent = xMessageBufferSend(rxMessage, (void *)ucArrayToSend, sizeof(ucArrayToSend), x100ms);
        if (xBytesSent != sizeof(ucArrayToSend))
        {
            // The call to xStreamBufferSend() times out before there was enough
            // space in the buffer for the data to be written, but it did
            // successfully write xBytesSent bytes.
        }
        xBytesSent = xMessageBufferSend(rxMessage, (void *)pcStringToSend, strlen(pcStringToSend), 0);
        if (xBytesSent != strlen(pcStringToSend))
        {
            // The entire string could not be added to the stream buffer because
            // there was not enough free space in the buffer, but xBytesSent bytes
            // were sent.  Could try again to send the remaining bytes.
        } // Yield processor for a while
        vTaskDelay(task_0_delay / portTICK_PERIOD_MS);
    }
}

void doTaskB(void *parameters)
{
    uint8_t ucRxData[25];
    size_t xReceivedBytes;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    // Configure pin
    //pinMode(pin_1, OUTPUT);

    // Do forever
    while (1)
    {
        xReceivedBytes = xMessageBufferReceive(rxMessage, (void *)ucRxData, sizeof(ucRxData), portMAX_DELAY);

        if (xReceivedBytes > 0)
        {
            // A ucRxData contains another xRecievedBytes bytes of data, which can
            // be processed here....
            for (int i=0; i < xReceivedBytes; i++)
            {
                Serial.print(ucRxData[i]);
                Serial.print(' ');
            }
            Serial.println();
        }
        // Toggle LED
        //digitalWrite(pin_1, !digitalRead(pin_1));
        //vTaskDelay(task_1_delay / portTICK_PERIOD_MS);
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
    Serial.println("---FreeRTOS Message Buffer Demo---");

    rxMessage = xMessageBufferCreate(NUM_BYTES);

    // Start Task 0 (in Core 0)
    xTaskCreatePinnedToCore(doTaskA,
                            "Task A",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

    // Start Task 1 (in Core 1)
    xTaskCreatePinnedToCore(doTaskB,
                            "Task B",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

    // Delete "setup and loop" task
    vTaskDelete(NULL);
}

void loop()
{
    // Execution should never get here
}