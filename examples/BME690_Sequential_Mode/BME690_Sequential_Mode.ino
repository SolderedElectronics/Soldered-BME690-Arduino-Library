/**
 **************************************************
 *
 * @file        BME690_Sequential_Mode.ino
 * @brief       Runs the BME690 in sequential mode, where the sensor steps
 *              through a heater profile on its own, sleeping between the
 *              measurements.
 *
 *              Connect the breakout board to the I2C pins of your board, or
 *              use a Qwiic cable.
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Josip Šimun Kuči @ Soldered.com
 ***************************************************/

#include "BME690-SOLDERED.h"

BME690 sensor;

// Heater temperature profile in degrees Celsius.
uint16_t heaterTemp[10] = {320, 100, 100, 100, 200, 200, 200, 320, 320, 320};

// Heating duration profile in milliseconds.
uint16_t heaterDur[10] = {150, 150, 150, 150, 150, 150, 150, 150, 150, 150};

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

    sensor.setTPH();

    // Sleep duration between two measurements of the profile.
    sensor.setSeqSleep(BME69X_ODR_250_MS);

    sensor.setHeaterProf(heaterTemp, heaterDur, 10);
    sensor.setOpMode(BME69X_SEQUENTIAL_MODE);

    Serial.println("Timestamp(ms), Temperature(C), Pressure(Pa), Humidity(%), Gas resistance(Ohm), Status, Gas index");
}

void loop()
{
    bme69xData data;
    uint8_t nFieldsLeft = 0;

    if (sensor.fetchData())
    {
        do
        {
            nFieldsLeft = sensor.getData(data);

            Serial.print(millis());
            Serial.print(", ");
            Serial.print(data.temperature);
            Serial.print(", ");
            Serial.print(data.pressure);
            Serial.print(", ");
            Serial.print(data.humidity);
            Serial.print(", ");
            Serial.print(data.gas_resistance);
            Serial.print(", 0x");
            Serial.print(data.status, HEX);
            Serial.print(", ");
            Serial.println(data.gas_index);
        } while (nFieldsLeft);
    }

    delay(100);
}
