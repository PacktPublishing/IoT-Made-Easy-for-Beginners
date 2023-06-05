const byte led_gpio = 32; 

void setup() { 

  pinMode(led_gpio, OUTPUT); 

}

void loop() { 

  digitalWrite(led_gpio, HIGH);    

  delay(1000);                        

  digitalWrite(led_gpio, LOW);     

  delay(1000);                        

} 