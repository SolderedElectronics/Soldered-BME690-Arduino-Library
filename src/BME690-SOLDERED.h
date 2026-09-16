/**
 **************************************************
 *
 * @file        BME690-SOLDERED.h
 * @brief       Header file for the Soldered BME690 environmental sensor
 *              breakout board library. All of the logic is contained within
 *              the libs/ folder.
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Original Author: Bosch Sensortec GmbH
 *              Modifications by: Josip Šimun Kuči @ Soldered.com
 * @date        Last modified: 2026-09-14
 ***************************************************/

#ifndef BME690_SOLDERED_H
#define BME690_SOLDERED_H

#include "Arduino.h"
#include "Wire.h"
#include "libs/Bosch-BME69x-Library/bme69x/bme69x_defs.h"
#include "libs/Bosch-BME69x-Library/bme69xLibrary.h"

/**
 * @brief Class for the Soldered BME690 breakout board, I2C only.
 */
class BME690 : public Bme69x
{
  public:
    /**
     * @brief Class constructor.
     */
    BME690() : Bme69x() { }

    /**
     * @brief Initializes the sensor on the default Wire bus.
     *
     * @param address I2C address of the sensor, BME69X_I2C_ADDR_LOW by default.
     * @param idleTask Delay or idle function.
     */
    void begin(uint8_t address = BME69X_I2C_ADDR_LOW, bme69x_delay_us_fptr_t idleTask = bme69xDelayUs)
    {
        Wire.begin();
        Bme69x::begin(address, Wire, idleTask);
    }

    /**
     * @brief Initializes the sensor on a given Wire bus.
     *
     * The bus has to be started by the sketch beforehand.
     *
     * @param i2c The TwoWire object to communicate over.
     * @param address I2C address of the sensor, BME69X_I2C_ADDR_LOW by default.
     * @param idleTask Delay or idle function.
     */
    void begin(TwoWire &i2c, uint8_t address = BME69X_I2C_ADDR_LOW, bme69x_delay_us_fptr_t idleTask = bme69xDelayUs)
    {
        Bme69x::begin(address, i2c, idleTask);
    }
};

/**
 * @brief Data structure holding one measured field of the BME690.
 */
class BME690Data : public bme69xData
{
};

#endif // BME690_SOLDERED_H
