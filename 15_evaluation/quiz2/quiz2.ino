static const int led_pin1 = 12;
static const int led_pin2 = 14;
static const int led_pin3 = 27;

void task_led1(void *parameter)
{
    while (1)
    {
        digitalWrite(led_pin1, HIGH);
        vTaskDelay(500);
        digitalWrite(led_pin1, LOW);
        vTaskDelay(500);
    }
}

void task_led2(void *parameter)
{
        digitalWrite(led_pin2, HIGH);
        vTaskDelay(250);
        digitalWrite(led_pin2, LOW);
        vTaskDelay(250);
}

void task_led3(void *parameter)
{
    while (1)
    {
        digitalWrite(led_pin3, HIGH);
        vTaskDelay(1000);
        digitalWrite(led_pin3, LOW);
        vTaskDelay(1000);
    }
}

void setup()
{
    int app_cpu = 0;

    pinMode(led_pin1, OUTPUT);
    pinMode(led_pin2, OUTPUT);    
    pinMode(led_pin3, OUTPUT);

    delay(500);

    app_cpu = xPortGetCoreID();
    printf("app_cpu is %d (%s core)\n", app_cpu, app_cpu > 0 ? "Dual" : "Single");
    
    xTaskCreatePinnedToCore( 
        task_led1,         
        "LED1",          
        1024,            
        NULL,            
        1,               
        NULL,            
        app_cpu);        

    xTaskCreatePinnedToCore( 
        task_led2,         
        "LED2",          
        1024,            
        NULL,            
        1,               
        NULL,            
        app_cpu);        

    xTaskCreatePinnedToCore( 
        task_led3,         
        "LED3",          
        1024,            
        NULL,            
        1,               
        NULL,            
        app_cpu);        

}

void loop()
{
    vTaskDelay(1000);
}