/**
 **************************************************
 *
 * @file        BME690_Parallel_Mode.ino
 * @brief       Runs the BME690 in parallel mode, where the gas sensor sweeps
 *              through a heater profile while temperature, pressure and
 *              humidity are measured continuously.
 *
 *              Connect the breakout board to the I2C pins of your board, or
 *              use a Qwiic cable.
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Josip Šimun Kuči @ Soldered.com
 ***************************************************/

#include "BME690-SOLDERED.h"

// New data, gas measurement valid and heater stable, all at once.
#define BME690_VALID_DATA (BME69X_NEW_DATA_MSK | BME69X_GASM_VALID_MSK | BME69X_HEAT_STAB_MSK)

BME690 sensor;

// Heater temperature profile in degrees Celsius.
uint16_t heaterTemp[10] = {320, 100, 100, 100, 200, 200, 200, 320, 320, 320};

// Multipliers of the shared heater duration, one per profile step.
uint16_t heaterMul[10] = {5, 2, 10, 30, 5, 5, 5, 5, 5, 5};

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

    // The shared heating duration is the total measurement duration minus the
    // time needed for the temperature, pressure and humidity measurement.
    uint16_t sharedHeatrDur = 140 - (sensor.getMeasDur(BME69X_PARALLEL_MODE) / 1000);

    sensor.setHeaterProf(heaterTemp, heaterMul, sharedHeatrDur, 10);
    sensor.setOpMode(BME69X_PARALLEL_MODE);

    Serial.println("Timestamp(ms), Temperature(C), Pressure(Pa), Humidity(%), Gas resistance(Ohm), Status, Gas "
                   "index, Meas index");
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

            // Skip the fields which hold no valid measurement.
            if (data.status == BME690_VALID_DATA)
            {
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
                Serial.print(data.gas_index);
                Serial.print(", ");
                Serial.println(data.meas_index);
            }
        } while (nFieldsLeft);
    }
}
