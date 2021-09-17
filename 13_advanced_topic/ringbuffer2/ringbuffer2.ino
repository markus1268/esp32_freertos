//#include "freertos/stream_buffer.h"
//#include "freertos/message_buffer.h"
#include "freertos/ringbuf.h"

#define LED1 12
#define LED2 14
#define LED3 27

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Settings
static const uint32_t task_0_delay = 250; // Time (ms) Task 0 blocks itself
static const uint32_t task_1_delay = 1000;

// Pins
//static const int pin_1 = 12; // LED pin

//static StreamBufferHandle_t rxStream = NULL;
//static MessageBufferHandle_t rxMessage = NULL;
static RingbufHandle_t rb_handle;

typedef struct LedMsg {
  int led_no;
  int led_state;
} LedMsg;

// Globals

//*****************************************************************************
// Tasks

void doTaskA(void *parameters)
{
    UBaseType_t res;
    LedMsg led_msg;
    static int led_no;
    static int led_state[] = {0, 0, 0};
    const TickType_t x100ms = pdMS_TO_TICKS(100);

    // Do forever
    while (1)
    {
        led_no = random(3);
        led_state[led_no] ^= 1;
        led_msg.led_no = led_no;
        led_msg.led_state = led_state[led_no];

        res = xRingbufferSend(rb_handle, &led_msg, sizeof(led_msg), x100ms);
        if (res != pdTRUE)
        {
            // The call to xStreamBufferSend() times out before there was enough
            // space in the buffer for the data to be written, but it did
            // successfully write xBytesSent bytes.
        }
        vTaskDelay(task_0_delay / portTICK_PERIOD_MS);
    }
}

void doTaskB(void *parameters)
{
    size_t item_size;
    LedMsg *pbuf;

    // Configure pin
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, LOW);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, LOW);
    pinMode(LED3, OUTPUT);
    digitalWrite(LED3, LOW);

    // Do forever
    while (1)
    {
        //Receive an item from no-split ring buffer
        LedMsg *item = (LedMsg *)xRingbufferReceive(rb_handle, &item_size, portMAX_DELAY);

        //Check received item
        if (item != NULL) {
            pbuf = item;
            //Serial.println(item_size);
            for (int i = 0; i < (item_size / sizeof(LedMsg)); i++) {
                Serial.print("Led No: ");
                Serial.print(pbuf->led_no);
                Serial.print(" Led State: ");
                Serial.print(pbuf->led_state);
                Serial.println();
                if (pbuf->led_no == 0)
                {
                    digitalWrite(LED1, pbuf->led_state);
                }
                if (pbuf->led_no == 1)
                {
                    digitalWrite(LED2, pbuf->led_state);
                }
                if (pbuf->led_no == 2)
                {
                    digitalWrite(LED3, pbuf->led_state);
                }
                pbuf = pbuf + sizeof(LedMsg);
            }
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