// You'll likely need this on vanilla FreeRTOS
//#include <semphr.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const char command[] = "rms";          // Command
static const uint16_t timer_divider = 2;      // Divide 80 MHz by this
static const uint64_t timer_max_count = 2500; // 16kHz sample rate
static const uint32_t cli_delay = 10;         // ms delay
static const float adc_voltage = 3.3;         // Max ADC voltage
static const uint16_t adc_max = 4095;         // Max ADC value (12-bit)
static const uint8_t pwm_ch = 0;              // PWM channel
enum
{
    BUF_LEN = 1600
}; // Number of elements in sample buffer
enum
{
    MSG_LEN = 100
}; // Max characters in message body
enum
{
    MSG_QUEUE_LEN = 5
}; // Number of slots in message queue
enum
{
    CMD_BUF_LEN = 255
}; // Number of characters in command buffer

// Pins
static const int adc_pin = A0;
static const int led_pin = 15;

// Message struct to wrap strings for queue
typedef struct Message
{
    char body[MSG_LEN];
} Message;

// Globals
static hw_timer_t *timer = NULL;
static TaskHandle_t processing_task = NULL;
static SemaphoreHandle_t sem_done_reading = NULL;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t msg_queue;
static volatile uint16_t buf_0[BUF_LEN];     // One buffer in the pair
static volatile uint16_t buf_1[BUF_LEN];     // The other buffer in the pair
static volatile uint16_t *write_to = buf_0;  // Double buffer write pointer
static volatile uint16_t *read_from = buf_1; // Double buffer read pointer
static volatile uint8_t buf_overrun = 0;     // Double buffer overrun flag
static float adc_rms;

//*****************************************************************************
// Functions that can be called from anywhere (in this file)

// Swap the write_to and read_from pointers in the double buffer
// Only ISR calls this at the moment, so no need to make it thread-safe
void IRAM_ATTR swap()
{
    volatile uint16_t *temp_ptr = write_to;
    write_to = read_from;
    read_from = temp_ptr;
}

//*****************************************************************************
// Interrupt Service Routines (ISRs)

// This function executes when timer reaches max (and resets)
void IRAM_ATTR onTimer()
{

    static uint16_t idx = 0;
    BaseType_t task_woken = pdFALSE;

    // If buffer is not overrun, read ADC to next buffer element. If buffer is
    // overrun, drop the sample.
    if ((idx < BUF_LEN) && (buf_overrun == 0))
    {
        write_to[idx] = analogRead(adc_pin);
        idx++;
    }

    // Check if the buffer is full
    if (idx >= BUF_LEN)
    {

        // If reading is not done, set overrun flag. We don't need to set this
        // as a critical section, as nothing can interrupt and change either value.
        if (xSemaphoreTakeFromISR(sem_done_reading, &task_woken) == pdFALSE)
        {
            buf_overrun = 1;
        }

        // Only swap buffers and notify task if overrun flag is cleared
        if (buf_overrun == 0)
        {

            // Reset index and swap buffer pointers
            idx = 0;
            swap();

            // A task notification works like a binary semaphore but is faster
            vTaskNotifyGiveFromISR(processing_task, &task_woken);
        }
    }

    // Exit from ISR (Vanilla FreeRTOS)
    //portYIELD_FROM_ISR(task_woken);

    // Exit from ISR (ESP-IDF)
    if (task_woken)
    {
        portYIELD_FROM_ISR();
    }
}

//*****************************************************************************
// Tasks

// Serial terminal task
void doCLI(void *parameters)
{

    Message rcv_msg;
    char c;
    char cmd_buf[CMD_BUF_LEN];
    uint8_t idx = 0;
    uint8_t cmd_len = strlen(command);

    // Clear whole buffer
    memset(cmd_buf, 0, CMD_BUF_LEN);

    // Loop forever
    while (1)
    {

        // Look for any error messages that need to be printed
        if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTRUE)
        {
            Serial.println(rcv_msg.body);
        }

        // Read characters from serial
        if (Serial.available() > 0)
        {
            c = Serial.read();

            // Store received character to buffer if not over buffer limit
            if (idx < CMD_BUF_LEN - 1)
            {
                cmd_buf[idx] = c;
                idx++;
            }

            // Print newline and check input on 'enter'
            if ((c == '\n') || (c == '\r'))
            {

                // Print newline to terminal
                Serial.print("\r\n");

                // Print average value if command given is "avg"
                cmd_buf[idx - 1] = '\0';
                if (strcmp(cmd_buf, command) == 0)
                {
                    Serial.print("RMS Voltage: ");
                    Serial.println(adc_rms);
                }

                // Reset receive buffer and index counter
                memset(cmd_buf, 0, CMD_BUF_LEN);
                idx = 0;

                // Otherwise, echo character back to serial terminal
            }
            else
            {
                Serial.print(c);
            }
        }

        // Don't hog the CPU. Yield to other tasks for a while
        vTaskDelay(cli_delay / portTICK_PERIOD_MS);
    }
}

// Wait for semaphore and calculate average of ADC values
void calcRMS(void *parameters)
{

    Message msg;
    float val;
    float avg;
    float rms;
    float brightness;

    // Loop forever, wait for semaphore, and print value
    while (1)
    {

        // Wait for notification from ISR (similar to binary semaphore)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Calculate average (as floating point value)
        avg = 0.0;
        for (int i = 0; i < BUF_LEN; i++)
        {
            avg += (float)read_from[i];
            //vTaskDelay(105 / portTICK_PERIOD_MS); // Uncomment to test overrun flag
        }
        avg /= BUF_LEN;

        // Convert average to voltage
        avg = (avg * adc_voltage) / (float)adc_max;

        // Calculate volts-RMS value (filter out DC component)
        rms = 0.0;
        for (int i = 0; i < BUF_LEN; i++)
        {
            val = ((float)read_from[i] * adc_voltage) / (float)adc_max;
            rms += powf((val - avg), 2);
        }
        rms = sqrtf(rms / BUF_LEN);

        // Udate LED brightness
        brightness = (rms * UINT16_MAX) / adc_voltage;
        ledcWrite(pwm_ch, brightness);

        // Updating the shared float may or may not take multiple isntructions, so
        // we protect it with a mutex or critical section. The ESP-IDF critical
        // section is the easiest for this application.
        portENTER_CRITICAL(&spinlock);
        adc_rms = rms;
        portEXIT_CRITICAL(&spinlock);

        // If we took too long to process, buffer writing will have overrun. So,
        // we send a message to be printed out to the serial terminal.
        if (buf_overrun == 1)
        {
            strcpy(msg.body, "Error: Buffer overrun. Samples have been dropped.");
            xQueueSend(msg_queue, (void *)&msg, 10);
        }

        // Clearing the overrun flag and giving the "done reading" semaphore must
        // be done together without being interrupted.
        portENTER_CRITICAL(&spinlock);
        buf_overrun = 0;
        xSemaphoreGive(sem_done_reading);
        portEXIT_CRITICAL(&spinlock);
    }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup()
{

    // Configure PWM pin
    pinMode(led_pin, OUTPUT);
    ledcAttachPin(led_pin, pwm_ch); // Assign pin to PWM channel 0
    ledcSetup(pwm_ch, 4000, 16);    // channel 0, 12kHz, 16-bit resolution

    // Configure Serial
    Serial.begin(115200);

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Sample and Process Demo---");

    // Create semaphore before it is used (in task or ISR)
    sem_done_reading = xSemaphoreCreateBinary();

    // Force reboot if we can't create the semaphore
    if (sem_done_reading == NULL)
    {
        Serial.println("Could not create one or more semaphores");
        ESP.restart();
    }

    // We want the done reading semaphore to initialize to 1
    xSemaphoreGive(sem_done_reading);

    // Create message queue before it is used
    msg_queue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message));

    // Start task to handle command line interface events. Let's set it at a
    // higher priority but only run it once every 20 ms.
    xTaskCreatePinnedToCore(doCLI,
                            "Do CLI",
                            2048,
                            NULL,
                            2,
                            NULL,
                            app_cpu);

    // Start task to calculate average. Save handle for use with notifications.
    xTaskCreatePinnedToCore(calcRMS,
                            "Calculate RMS",
                            2048,
                            NULL,
                            1,
                            &processing_task,
                            app_cpu);

    // Start a timer to run ISR every 100 ms
    timer = timerBegin(0, timer_divider, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, timer_max_count, true);
    timerAlarmEnable(timer);

    // Delete "setup and loop" task
    vTaskDelete(NULL);
}

void loop()
{
    // Execution should never get here
}