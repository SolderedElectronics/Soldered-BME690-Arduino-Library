/**
 **************************************************
 *
 * @file        bme69xLibrary.h
 * @brief       Arduino, I2C-only, camelCase wrapper around the Bosch BME69x
 *              Sensor API. The Bosch API itself lives, unmodified, in the
 *              bme69x/ subfolder.
 *
 * @copyright   BSD-3-Clause
 * @authors     Original Author: Bosch Sensortec GmbH
 *              Modifications by: Josip Šimun Kuči @ Soldered.com
 * @date        Last modified: 2026-09-14
 ***************************************************/

#ifndef BME69X_LIBRARY_H
#define BME69X_LIBRARY_H

#include "Arduino.h"
#include "Wire.h"
#include "bme69x/bme69x.h"
#include <string.h>

#define BME69X_ERROR   INT8_C(-1)
#define BME69X_WARNING INT8_C(1)

/**
 * @brief Datatype working as an I2C interface descriptor.
 */
typedef struct
{
    TwoWire *wireObj; ///< The TwoWire object used for the transfers.
    uint8_t i2cAddr;  ///< The I2C address of the sensor.
} bme69xCommT;

/** Datatypes kept consistent with camel casing */
typedef struct bme69x_data bme69xData;
typedef struct bme69x_dev bme69xDev;
typedef struct bme69x_conf bme69xConf;
typedef struct bme69x_heatr_conf bme69xHeatrConf;

/**
 * @brief Function that implements the default microsecond delay callback.
 *
 * @param periodUs Duration of the delay in microseconds.
 * @param intfPtr Pointer to the interface descriptor.
 */
void bme69xDelayUs(uint32_t periodUs, void *intfPtr);

/**
 * @brief Function that implements the default I2C write transaction.
 *
 * @param regAddr Register address of the sensor.
 * @param regData Pointer to the data to be written to the sensor.
 * @param length Length of the transfer.
 * @param intfPtr Pointer to the interface descriptor.
 * @return 0 if successful, non-zero otherwise.
 */
int8_t bme69xI2cWrite(uint8_t regAddr, const uint8_t *regData, uint32_t length, void *intfPtr);

/**
 * @brief Function that implements the default I2C read transaction.
 *
 * @param regAddr Register address of the sensor.
 * @param regData Pointer to the buffer the data is read into.
 * @param length Length of the transfer.
 * @param intfPtr Pointer to the interface descriptor.
 * @return 0 if successful, non-zero otherwise.
 */
int8_t bme69xI2cRead(uint8_t regAddr, uint8_t *regData, uint32_t length, void *intfPtr);

/**
 * @brief Arduino class wrapping the Bosch BME69x Sensor API over I2C.
 */
class Bme69x
{
  public:
    /** Stores the BME69x Sensor API error code of the last executed call. */
    int8_t status;

    /**
     * @brief Class constructor, initializes all the internal structures.
     */
    Bme69x(void);

    /**
     * @brief Initializes the sensor with custom callbacks.
     *
     * @param read Read callback.
     * @param write Write callback.
     * @param idleTask Delay or idle function.
     * @param intfPtr Pointer to the interface descriptor.
     */
    void begin(bme69x_read_fptr_t read, bme69x_write_fptr_t write, bme69x_delay_us_fptr_t idleTask, void *intfPtr);

    /**
     * @brief Initializes the sensor using the Wire library.
     *
     * @param i2cAddr The I2C address the sensor is at.
     * @param i2c The TwoWire object to communicate over.
     * @param idleTask Delay or idle function.
     */
    void begin(uint8_t i2cAddr, TwoWire &i2c, bme69x_delay_us_fptr_t idleTask = bme69xDelayUs);

    /**
     * @brief Reads a single register.
     *
     * @param regAddr Register address.
     * @return Data at that register.
     */
    uint8_t readReg(uint8_t regAddr);

    /**
     * @brief Reads multiple registers.
     *
     * @param regAddr Start register address.
     * @param regData Pointer to the buffer the data is read into.
     * @param length Number of registers to read.
     */
    void readReg(uint8_t regAddr, uint8_t *regData, uint32_t length);

    /**
     * @brief Writes data to a single register.
     *
     * @param regAddr Register address.
     * @param regData Data for that register.
     */
    void writeReg(uint8_t regAddr, uint8_t regData);

    /**
     * @brief Writes multiple registers.
     *
     * @param regAddr Pointer to the register addresses.
     * @param regData Pointer to the data for those registers.
     * @param length Number of registers to write.
     */
    void writeReg(uint8_t *regAddr, const uint8_t *regData, uint32_t length);

    /**
     * @brief Triggers a soft reset of the sensor.
     */
    void softReset(void);

    /**
     * @brief Sets the ambient temperature used for a better heater configuration.
     *
     * @param temp Temperature in degrees Celsius, 25 by default.
     */
    void setAmbientTemp(int8_t temp = 25);

    /**
     * @brief Gets the measurement duration in microseconds.
     *
     * @param opMode Operation mode of the sensor, the last used one is taken if none is set.
     * @return Temperature, pressure and humidity measurement time in microseconds.
     */
    uint32_t getMeasDur(uint8_t opMode = BME69X_SLEEP_MODE);

    /**
     * @brief Sets the operation mode.
     *
     * @param opMode BME69X_SLEEP_MODE, BME69X_FORCED_MODE, BME69X_PARALLEL_MODE or BME69X_SEQUENTIAL_MODE.
     */
    void setOpMode(uint8_t opMode);

    /**
     * @brief Gets the operation mode.
     *
     * @return BME69X_SLEEP_MODE, BME69X_FORCED_MODE, BME69X_PARALLEL_MODE or BME69X_SEQUENTIAL_MODE.
     */
    uint8_t getOpMode(void);

    /**
     * @brief Gets the temperature, pressure and humidity over-sampling.
     *
     * @param osHum Humidity over-sampling, BME69X_OS_NONE to BME69X_OS_16X.
     * @param osTemp Temperature over-sampling, BME69X_OS_NONE to BME69X_OS_16X.
     * @param osPres Pressure over-sampling, BME69X_OS_NONE to BME69X_OS_16X.
     */
    void getTPH(uint8_t &osHum, uint8_t &osTemp, uint8_t &osPres);

    /**
     * @brief Sets the temperature, pressure and humidity over-sampling.
     *
     * Passing no arguments sets the defaults.
     *
     * @param osTemp Temperature over-sampling, BME69X_OS_NONE to BME69X_OS_16X.
     * @param osPres Pressure over-sampling, BME69X_OS_NONE to BME69X_OS_16X.
     * @param osHum Humidity over-sampling, BME69X_OS_NONE to BME69X_OS_16X.
     */
    void setTPH(uint8_t osTemp = BME69X_OS_2X, uint8_t osPres = BME69X_OS_16X, uint8_t osHum = BME69X_OS_1X);

    /**
     * @brief Gets the IIR filter configuration.
     *
     * @return BME69X_FILTER_OFF to BME69X_FILTER_SIZE_127.
     */
    uint8_t getFilter(void);

    /**
     * @brief Sets the IIR filter configuration.
     *
     * @param filter BME69X_FILTER_OFF to BME69X_FILTER_SIZE_127.
     */
    void setFilter(uint8_t filter = BME69X_FILTER_OFF);

    /**
     * @brief Gets the sleep duration used in sequential mode.
     *
     * @return BME69X_ODR_0_59_MS to BME69X_ODR_NONE.
     */
    uint8_t getSeqSleep(void);

    /**
     * @brief Sets the sleep duration used in sequential mode.
     *
     * @param odr BME69X_ODR_0_59_MS to BME69X_ODR_NONE.
     */
    void setSeqSleep(uint8_t odr = BME69X_ODR_0_59_MS);

    /**
     * @brief Sets the heater profile for forced mode.
     *
     * @param temp Heater temperature in degrees Celsius.
     * @param dur Heating duration in milliseconds.
     */
    void setHeaterProf(uint16_t temp, uint16_t dur);

    /**
     * @brief Sets the heater profile for sequential mode.
     *
     * @param temp Heater temperature profile in degrees Celsius.
     * @param dur Heating duration profile in milliseconds.
     * @param profileLen Length of the profile.
     */
    void setHeaterProf(uint16_t *temp, uint16_t *dur, uint8_t profileLen);

    /**
     * @brief Sets the heater profile for parallel mode.
     *
     * @param temp Heater temperature profile in degrees Celsius.
     * @param mul Profile of the number of repetitions.
     * @param sharedHeatrDur Shared heating duration in milliseconds.
     * @param profileLen Length of the profile.
     */
    void setHeaterProf(uint16_t *temp, uint16_t *mul, uint16_t sharedHeatrDur, uint8_t profileLen);

    /**
     * @brief Fetches the data from the sensor into the local buffer.
     *
     * @return Number of new data fields.
     */
    uint8_t fetchData(void);

    /**
     * @brief Gets a single data field out of the local buffer.
     *
     * @param data Structure the data is stored into.
     * @return Number of new fields remaining.
     */
    uint8_t getData(bme69xData &data);

    /**
     * @brief Gets the whole local data buffer.
     *
     * @return Pointer to the three sensor data fields.
     */
    bme69xData *getAllData(void);

    /**
     * @brief Gets the currently set heater configuration.
     *
     * @return Reference to the heater configuration.
     */
    const bme69xHeatrConf &getHeaterConfiguration(void);

    /**
     * @brief Retrieves the unique ID of the sensor.
     *
     * @return Unique ID.
     */
    uint32_t getUniqueId(void);

    /**
     * @brief Runs the built-in self test of the sensor.
     *
     * The sensor is left in sleep mode and has to be reconfigured afterwards.
     *
     * @return BME69X_OK if the self test passed, an error code otherwise.
     */
    int8_t selfTest(void);

    /**
     * @brief Gets the error code of the interface functions.
     *
     * @return Interface return code.
     */
    BME69X_INTF_RET_TYPE intfError(void);

    /**
     * @brief Checks whether an error or a warning has occurred.
     *
     * @return BME69X_ERROR if an error occurred, BME69X_WARNING if a warning occurred, BME69X_OK otherwise.
     */
    int8_t checkStatus(void);

    /**
     * @brief Gets a brief text description of the last error.
     *
     * @return String describing the status code.
     */
    String statusString(void);

  private:
    bme69xCommT m_comm;         ///< I2C interface descriptor handed to the Bosch API.
    bme69xDev m_bme6;           ///< Bosch API device structure.
    bme69xConf m_conf;          ///< Sensor configuration, over-sampling, filter and ODR.
    bme69xHeatrConf m_heatrConf;///< Gas heater configuration.
    bme69xData m_sensorData[3]; ///< Local buffer holding the last fetched data fields.
    uint8_t m_nFields;          ///< Number of new data fields in the local buffer.
    uint8_t m_iFields;          ///< Index of the next data field to be read out.
    uint8_t m_lastOpMode;       ///< Last operation mode which was set.
};

#endif // BME69X_LIBRARY_H
