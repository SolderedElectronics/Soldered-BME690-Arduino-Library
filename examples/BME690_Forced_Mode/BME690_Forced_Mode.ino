/**
 **************************************************
 *
 * @file        BME690_Forced_Mode.ino
 * @brief       Reads temperature, pressure, humidity and gas resistance from
 *              the BME690 sensor in forced mode, one measurement at a time.
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

    // Start the sensor on the default I2C address (0x76).
    // Use sensor.begin(BME69X_I2C_ADDR_HIGH) if the address jumper is soldered.
    sensor.begin();

    if (sensor.checkStatus() == BME69X_ERROR)
    {
        Serial.print("BME690 initialization failed: ");
        Serial.println(sensor.statusString());
        while (1)
            ;
    }

    // Oversampling for temperature, pressure and humidity.
    sensor.setTPH();

    // IIR filter for the temperature and pressure readings.
    sensor.setFilter(BME69X_FILTER_SIZE_3);

    // Heat the gas sensor plate to 300 degrees Celsius for 100 milliseconds.
    sensor.setHeaterProf(300, 100);

    Serial.println("Timestamp(ms), Temperature(C), Pressure(Pa), Humidity(%), Gas resistance(Ohm), Status");
}

void loop()
{
    bme69xData data;

    // Trigger a single measurement.
    sensor.setOpMode(BME69X_FORCED_MODE);

    // Wait for the measurement to finish, the heater duration included.
    delayMicroseconds(sensor.getMeasDur());

    if (sensor.fetchData())
    {
        sensor.getData(data);

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
        Serial.println(data.status, HEX);
    }

    delay(1000);
}
