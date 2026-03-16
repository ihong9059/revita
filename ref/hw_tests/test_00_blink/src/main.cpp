#include <Arduino.h>

// UART2 pins (P0.16 TX, P0.15 RX)
#define UART2_TX 16
#define UART2_RX 15

void setup() {
    Serial1.setPins(UART2_RX, UART2_TX);
    Serial1.begin(115200);
    delay(500);
}

void loop() {
    Serial1.println("Hello from RAK4631!");
    delay(1000);
}
