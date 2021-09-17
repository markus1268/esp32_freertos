//#include "freertos/stream_buffer.h"
//#include "freertos/message_buffer.h"
#include "freertos/ringbuf.h"

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Settings
static const uint32_t task_0_delay = 1000; // Time (ms) Task 0 blocks itself
static const uint32_t task_1_delay = 1000;

//static StreamBufferHandle_t rxStream = NULL;
//static MessageBufferHandle_t rxMessage = NULL;
static RingbufHandle_t rb_handle;

// Globals

//*****************************************************************************
// Tasks

void doTaskA(void *parameters)
{
    UBaseType_t res;
    uint8_t ucArrayToSend[] = {'0', '1', '2', '3'};
    char *pcStringToSend = "Hello StreamBuffer";
    const TickType_t x100ms = pdMS_TO_TICKS(100);

    // Do forever
    while (1)
    {
        res = xRingbufferSend(rb_handle, ucArrayToSend, sizeof(ucArrayToSend), x100ms);
        if (res != pdTRUE)
        {
            // The call to xStreamBufferSend() times out before there was enough
            // space in the buffer for the data to be written, but it did
            // successfully write xBytesSent bytes.
        }
        res = xRingbufferSend(rb_handle, (void *)pcStringToSend, strlen(pcStringToSend), 0);
        if (res != pdTRUE)
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
    size_t item_size;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    // Do forever
    while (1)
    {
        //Receive an item from no-split ring buffer
        char *item = (char *)xRingbufferReceive(rb_handle, &item_size, pdMS_TO_TICKS(1000));

        //Check received item
        if (item != NULL) {
            //Print item
            for (int i = 0; i < item_size; i++) {
                //printf("%c", item[i]);
                Serial.print(item[i]);
                Serial.print(' ');
            }
            //printf("\n");
            Serial.println();
            //Return Item
            vRingbufferReturnItem(rb_handle, (void *)item);
        } else {
            //Failed to receive item
            Serial.println("Failed to receive item");
        }

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
    Serial.println("---FreeRTOS Ring Buffer Demo---");

    rb_handle = xRingbufferCreate(1028, RINGBUF_TYPE_NOSPLIT);

    xTaskCreatePinnedToCore(doTaskA,
                            "Task A",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

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