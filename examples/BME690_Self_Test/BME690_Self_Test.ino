/**
 **************************************************
 *
 * @file        BME690_Self_Test.ino
 * @brief       Runs the built-in self test of the BME690 sensor and prints
 *              the result. The sensor is left in sleep mode afterwards.
 *
 *              Connect the breakout board to the I2C pins of your board, or
 *              use a Qwiic cable.
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Josip Šimun Kuči @ Soldered.com
 ***************************************************/

#include "BME690-SOLDERED.h"

BME690 sensor;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    sensor.begin();

    if (sensor.checkStatus() == BME69X_ERROR)
    {
        Serial.print("BME690 initialization failed: ");
        Serial.println(sensor.statusString());
        while (1)
            ;
    }

    Serial.print("Unique ID: 0x");
    Serial.println(sensor.getUniqueId(), HEX);

    Serial.println("Running the self test, this takes a few seconds...");

    if (sensor.selfTest() == BME69X_OK)
    {
        Serial.println("Self test passed.");
    }
    else
    {
        Serial.print("Self test failed: ");
        Serial.println(sensor.statusString());
    }
}

void loop()
{
}
