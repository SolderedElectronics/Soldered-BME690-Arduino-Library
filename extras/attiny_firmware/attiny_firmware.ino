/**
 * @file attiny_firmware.ino
 * @brief Fill in sensor specific code.
 * @author Soldered Electronics
 */

#include "qwiic.h"
#include <Wire.h>

int addr = DEFAULT_ADDRESS;

void setup()
{
    initDefault();
    addr = getI2CAddress();

    Wire.begin(addr);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop()
{
}


void receiveEvent(int howMany)
{
    while (1 < Wire.available())
    {
        char c = Wire.read();
    }

    char c = Wire.read();
}

void requestEvent()
{
    int n = 5;

    char a[n];
    Wire.write(a, n);
}
