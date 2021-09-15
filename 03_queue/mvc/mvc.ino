// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define LED1 12
#define LED2 14
#define LED3 27

// Settings
static const uint8_t buf_len = 255;     // Size of buffer to look for command
static const char command[] = "temp ";  // Note the space!
static const int input_queue_len = 5;   // Size of delay_queue
static const int display_queue_len = 5; // Size of msg_queue
static const uint8_t blink_max = 100;   // Num times to blink before message

// Pins (change this if your Arduino board does not have LED_BUILTIN defined)
//static const int led_pin = LED_BUILTIN;

// Message struct: used to wrap strings (not necessary, but it's useful to see
// how to use structs here)
typedef struct DispMsg
{
    int led_no;
    int delay_val;
} DispMsg;

// Globals
static QueueHandle_t input_queue;
static QueueHandle_t display_queue;

//*****************************************************************************
// Tasks

void input_task(void *parameters)
{

    char c;
    char buf[buf_len];
    uint8_t idx = 0;
    uint8_t cmd_len = strlen(command);
    int temperature;

    // Clear whole buffer
    memset(buf, 0, buf_len);

    // Loop forever
    while (1)
    {

        // Read characters from serial
        if (Serial.available() > 0)
        {
            c = Serial.read();

            // Store received character to buffer if not over buffer limit
            if (idx < buf_len - 1)
            {
                buf[idx] = c;
                idx++;
            }

            // Print newline and check input on 'enter'
            if ((c == '\n') || (c == '\r'))
            {

                // Print newline to terminal
                Serial.print("\r\n");

                // Check if the first 6 characters are "delay "
                if (memcmp(buf, command, cmd_len) == 0)
                {

                    // Convert last part to positive integer (negative int crashes)
                    char *tail = buf + cmd_len;
                    temperature = atoi(tail);
                    temperature = abs(temperature);

                    // Send integer to other task via queue
                    if (xQueueSend(input_queue, (void *)&temperature, 10) != pdTRUE)
                    {
                        Serial.println("ERROR: Could not put item on delay queue.");
                    }
                }

                // Reset receive buffer and index counter
                memset(buf, 0, buf_len);
                idx = 0;

                // Otherwise, echo character back to serial terminal
            }
            else
            {
                Serial.print(c);
            }
        }
    }
}

void process_task(void *parameters)
{
    DispMsg disp_msg;
    int temperature;

    while (1)
    {
        if (xQueueReceive(input_queue, (void *)&temperature, 0) == pdTRUE)
        {
            if (temperature < 20)
            {
                disp_msg.led_no = 1;
                disp_msg.delay_val = 100;
            }
            else if (temperature <= 30)
            {
                disp_msg.led_no = 2;
                disp_msg.delay_val = 500;
            }
            else
            {
                disp_msg.led_no = 3;
                disp_msg.delay_val = 250;
            }
            xQueueSend(display_queue, (void *)&disp_msg, 10);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void display_task(void *parameters)
{
    DispMsg disp_msg;
    int led_no = 0;
    int delay_time = 100;

    // Set up pin
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, LOW);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, LOW);
    pinMode(LED3, OUTPUT);
    digitalWrite(LED3, LOW);

    // Loop forever
    while (1)
    {

        // See if there's a message in the queue (do not block)
        if (xQueueReceive(display_queue, (void *)&disp_msg, 0) == pdTRUE)
        {
            led_no = disp_msg.led_no;
            digitalWrite(LED1, LOW);
            digitalWrite(LED2, LOW);
            digitalWrite(LED3, LOW);
            delay_time = disp_msg.delay_val;

        }

        // Blink
        if (led_no == 1)
        {
            digitalWrite(LED1, digitalRead(LED1) ^ HIGH);
        }
        if (led_no == 2)
        {
            digitalWrite(LED2, digitalRead(LED2) ^ HIGH);
        }
        if (led_no == 3)
        {
            digitalWrite(LED3, digitalRead(LED3) ^ HIGH);
        }
        vTaskDelay(delay_time / portTICK_PERIOD_MS);
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
    Serial.println("---FreeRTOS Queue MVC Demo---");
    Serial.println("Enter the command 'temp xx' where xx is temperature value ");

    // Create queues
    input_queue = xQueueCreate(input_queue_len, sizeof(int));
    display_queue = xQueueCreate(display_queue_len, sizeof(DispMsg));

    xTaskCreatePinnedToCore(input_task,
                            "Input",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

    xTaskCreatePinnedToCore(process_task,
                            "Processing",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

    xTaskCreatePinnedToCore(display_task,
                            "Display",
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