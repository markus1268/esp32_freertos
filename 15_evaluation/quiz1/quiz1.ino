static const int led_pin = LED_BUILTIN;

void hello_led(void *parameter)
{
    while (1)
    {
        digitalWrite(led_pin, HIGH);
        vTaskDelay(500);
        digitalWrite(led_pin, LOW);
        vTaskDelay(500);
    }
}

void setup()
{
    int app_cpu = 0;

    pinMode(led_pin, OUTPUT);
    delay(500);

    app_cpu = xPortGetCoreID();
    printf("app_cpu is %d (%s core)\n", app_cpu, app_cpu > 0 ? "Dual" : "Single");
    
    xTaskCreatePinnedToCore( 
        hello_led,         
        "Hello",          
        1024,             
        NULL,             
        0,                
        NULL,             
        app_cpu);         
}

void loop()
{

}