/**
 **************************************************
 *
 * @file        bme69xLibrary.cpp
 * @brief       Arduino, I2C-only, camelCase wrapper around the Bosch BME69x
 *              Sensor API. The Bosch API itself lives, unmodified, in the
 *              bme69x/ subfolder.
 *
 * @copyright   BSD-3-Clause
 * @authors     Original Author: Bosch Sensortec GmbH
 *              Modifications by: Josip Šimun Kuči @ Soldered.com
 * @date        Last modified: 2026-09-14
 ***************************************************/

#include "bme69xLibrary.h"

#include <string.h>

/* Maximum transaction size, field size 17 x 3. */
#define BME69X_MAX_READ_LENGTH 51

#ifdef ARDUINO_ARCH_ESP32
#define BME69X_I2C_BUFFER_SIZE I2C_BUFFER_LENGTH
#endif

#ifdef ARDUINO_ARCH_MBED
/* Assuming all MBED implementations of Wire have 256 byte sized buffers. */
#define BME69X_I2C_BUFFER_SIZE 256
#endif

#ifdef ARDUINO_ARCH_ESP8266
#define BME69X_I2C_BUFFER_SIZE BUFFER_LENGTH
#endif

#ifdef ARDUINO_ARCH_AVR
#define BME69X_I2C_BUFFER_SIZE BUFFER_LENGTH
#endif

#ifdef ARDUINO_ARCH_NRF52
#define BME69X_I2C_BUFFER_SIZE SERIAL_BUFFER_SIZE
#endif

#ifdef ARDUINO_ARCH_SAMD
/* Assuming all Arduino's and Adafruit's SAMD implementations of Wire have 256 byte sized buffers. */
#define BME69X_I2C_BUFFER_SIZE 256
#endif

#ifdef ARDUINO_ARCH_SAM
#define BME69X_I2C_BUFFER_SIZE BUFFER_LENGTH
#endif

/* Optimistically assume support for at least 64 byte reads. */
#ifndef BME69X_I2C_BUFFER_SIZE
#define BME69X_I2C_BUFFER_SIZE 64
#endif

#if BME69X_MAX_READ_LENGTH > BME69X_I2C_BUFFER_SIZE
#warning "Wire read requires a larger buffer size, parallel and sequential mode reads may fail."
#endif

Bme69x::Bme69x(void)
{
    m_comm.wireObj = nullptr;
    m_comm.i2cAddr = 0;
    status = BME69X_OK;
    memset(&m_bme6, 0, sizeof(m_bme6));
    memset(&m_conf, 0, sizeof(m_conf));
    memset(&m_heatrConf, 0, sizeof(m_heatrConf));
    memset(m_sensorData, 0, sizeof(m_sensorData));
    m_bme6.amb_temp = 25; // Typical room temperature in degrees Celsius.
    m_nFields = 0;
    m_iFields = 0;
    m_lastOpMode = BME69X_SLEEP_MODE;
}

void Bme69x::begin(bme69x_read_fptr_t read, bme69x_write_fptr_t write, bme69x_delay_us_fptr_t idleTask, void *intfPtr)
{
    m_bme6.intf = BME69X_I2C_INTF;
    m_bme6.read = read;
    m_bme6.write = write;
    m_bme6.delay_us = idleTask;
    m_bme6.intf_ptr = intfPtr;
    m_bme6.amb_temp = 25;

    status = bme69x_init(&m_bme6);
}

void Bme69x::begin(uint8_t i2cAddr, TwoWire &i2c, bme69x_delay_us_fptr_t idleTask)
{
    m_comm.i2cAddr = i2cAddr;
    m_comm.wireObj = &i2c;
    m_bme6.intf = BME69X_I2C_INTF;
    m_bme6.read = bme69xI2cRead;
    m_bme6.write = bme69xI2cWrite;
    m_bme6.delay_us = idleTask;
    m_bme6.intf_ptr = &m_comm;
    m_bme6.amb_temp = 25;

    status = bme69x_init(&m_bme6);
}

uint8_t Bme69x::readReg(uint8_t regAddr)
{
    uint8_t regData;
    readReg(regAddr, &regData, 1);
    return regData;
}

void Bme69x::readReg(uint8_t regAddr, uint8_t *regData, uint32_t length)
{
    status = bme69x_get_regs(regAddr, regData, length, &m_bme6);
}

void Bme69x::writeReg(uint8_t regAddr, uint8_t regData)
{
    status = bme69x_set_regs(&regAddr, &regData, 1, &m_bme6);
}

void Bme69x::writeReg(uint8_t *regAddr, const uint8_t *regData, uint32_t length)
{
    status = bme69x_set_regs(regAddr, regData, length, &m_bme6);
}

void Bme69x::softReset(void)
{
    status = bme69x_soft_reset(&m_bme6);
}

void Bme69x::setAmbientTemp(int8_t temp)
{
    m_bme6.amb_temp = temp;
}

uint32_t Bme69x::getMeasDur(uint8_t opMode)
{
    if (opMode == BME69X_SLEEP_MODE)
        opMode = m_lastOpMode;

    return bme69x_get_meas_dur(opMode, &m_conf, &m_bme6);
}

void Bme69x::setOpMode(uint8_t opMode)
{
    status = bme69x_set_op_mode(opMode, &m_bme6);
    if ((status == BME69X_OK) && (opMode != BME69X_SLEEP_MODE))
        m_lastOpMode = opMode;
}

uint8_t Bme69x::getOpMode(void)
{
    uint8_t opMode;
    status = bme69x_get_op_mode(&opMode, &m_bme6);
    return opMode;
}

void Bme69x::getTPH(uint8_t &osHum, uint8_t &osTemp, uint8_t &osPres)
{
    status = bme69x_get_conf(&m_conf, &m_bme6);

    if (status == BME69X_OK) {
        osHum = m_conf.os_hum;
        osTemp = m_conf.os_temp;
        osPres = m_conf.os_pres;
    }
}

void Bme69x::setTPH(uint8_t osTemp, uint8_t osPres, uint8_t osHum)
{
    status = bme69x_get_conf(&m_conf, &m_bme6);

    if (status == BME69X_OK) {
        m_conf.os_hum = osHum;
        m_conf.os_temp = osTemp;
        m_conf.os_pres = osPres;

        status = bme69x_set_conf(&m_conf, &m_bme6);
    }
}

uint8_t Bme69x::getFilter(void)
{
    status = bme69x_get_conf(&m_conf, &m_bme6);

    return m_conf.filter;
}

void Bme69x::setFilter(uint8_t filter)
{
    status = bme69x_get_conf(&m_conf, &m_bme6);

    if (status == BME69X_OK) {
        m_conf.filter = filter;

        status = bme69x_set_conf(&m_conf, &m_bme6);
    }
}

uint8_t Bme69x::getSeqSleep(void)
{
    status = bme69x_get_conf(&m_conf, &m_bme6);

    return m_conf.odr;
}

void Bme69x::setSeqSleep(uint8_t odr)
{
    status = bme69x_get_conf(&m_conf, &m_bme6);

    if (status == BME69X_OK) {
        m_conf.odr = odr;

        status = bme69x_set_conf(&m_conf, &m_bme6);
    }
}

void Bme69x::setHeaterProf(uint16_t temp, uint16_t dur)
{
    m_heatrConf.enable = BME69X_ENABLE;
    m_heatrConf.heatr_temp = temp;
    m_heatrConf.heatr_dur = dur;

    status = bme69x_set_heatr_conf(BME69X_FORCED_MODE, &m_heatrConf, &m_bme6);
}

void Bme69x::setHeaterProf(uint16_t *temp, uint16_t *dur, uint8_t profileLen)
{
    m_heatrConf.enable = BME69X_ENABLE;
    m_heatrConf.heatr_temp_prof = temp;
    m_heatrConf.heatr_dur_prof = dur;
    m_heatrConf.profile_len = profileLen;

    status = bme69x_set_heatr_conf(BME69X_SEQUENTIAL_MODE, &m_heatrConf, &m_bme6);
}

void Bme69x::setHeaterProf(uint16_t *temp, uint16_t *mul, uint16_t sharedHeatrDur, uint8_t profileLen)
{
    m_heatrConf.enable = BME69X_ENABLE;
    m_heatrConf.heatr_temp_prof = temp;
    m_heatrConf.heatr_dur_prof = mul;
    m_heatrConf.shared_heatr_dur = sharedHeatrDur;
    m_heatrConf.profile_len = profileLen;

    status = bme69x_set_heatr_conf(BME69X_PARALLEL_MODE, &m_heatrConf, &m_bme6);
}

uint8_t Bme69x::fetchData(void)
{
    m_nFields = 0;
    status = bme69x_get_data(m_lastOpMode, m_sensorData, &m_nFields, &m_bme6);
    m_iFields = 0;

    return m_nFields;
}

uint8_t Bme69x::getData(bme69xData &data)
{
    if (m_lastOpMode == BME69X_FORCED_MODE) {
        data = m_sensorData[0];
    } else {
        if (m_nFields) {
            /* m_iFields spans from 0-2 while m_nFields spans from 0-3,
             * where 0 means that there is no new data.
             */
            data = m_sensorData[m_iFields];
            m_iFields++;

            /* Limit reading continuously to the last field read. */
            if (m_iFields >= m_nFields) {
                m_iFields = m_nFields - 1;
                return 0;
            }

            /* Indicate if there is something left to read. */
            return m_nFields - m_iFields;
        }
    }

    return 0;
}

bme69xData *Bme69x::getAllData(void)
{
    return m_sensorData;
}

const bme69xHeatrConf &Bme69x::getHeaterConfiguration(void)
{
    return m_heatrConf;
}

uint32_t Bme69x::getUniqueId(void)
{
    uint8_t idRegs[4];
    uint32_t uid;
    readReg(BME69X_REG_UNIQUE_ID, idRegs, 4);

    uint32_t id1 = ((uint32_t)idRegs[3] + ((uint32_t)idRegs[2] << 8)) & 0x7fff;
    uid = (id1 << 16) + (((uint32_t)idRegs[1]) << 8) + (uint32_t)idRegs[0];

    return uid;
}

int8_t Bme69x::selfTest(void)
{
    status = bme69x_selftest_check(&m_bme6);

    return status;
}

BME69X_INTF_RET_TYPE Bme69x::intfError(void)
{
    return m_bme6.intf_rslt;
}

int8_t Bme69x::checkStatus(void)
{
    if (status < BME69X_OK)
        return BME69X_ERROR;

    if (status > BME69X_OK)
        return BME69X_WARNING;

    return BME69X_OK;
}

String Bme69x::statusString(void)
{
    String ret = "";
    switch (status) {
    case BME69X_OK:
        /* Don't return a text for OK. */
        break;
    case BME69X_E_NULL_PTR:
        ret = "Null pointer";
        break;
    case BME69X_E_COM_FAIL:
        ret = "Communication failure";
        break;
    case BME69X_E_DEV_NOT_FOUND:
        ret = "Sensor not found";
        break;
    case BME69X_E_INVALID_LENGTH:
        ret = "Invalid length";
        break;
    case BME69X_E_SELF_TEST:
        ret = "Self test failed";
        break;
    case BME69X_W_DEFINE_OP_MODE:
        ret = "Set the operation mode";
        break;
    case BME69X_W_NO_NEW_DATA:
        ret = "No new data";
        break;
    case BME69X_W_DEFINE_SHD_HEATR_DUR:
        ret = "Set the shared heater duration";
        break;
    default:
        ret = "Undefined error code";
    }

    return ret;
}

void bme69xDelayUs(uint32_t periodUs, void *intfPtr)
{
    (void)intfPtr;
    delayMicroseconds(periodUs);
}

int8_t bme69xI2cWrite(uint8_t regAddr, const uint8_t *regData, uint32_t length, void *intfPtr)
{
    uint32_t i;
    int8_t rslt = BME69X_OK;
    bme69xCommT *comm = nullptr;

    if (length + 1 > BME69X_I2C_BUFFER_SIZE)
        return BME69X_E_COM_FAIL;

    if (intfPtr == nullptr)
        return BME69X_E_NULL_PTR;

    comm = (bme69xCommT *)intfPtr;
    if (comm->wireObj == nullptr)
        return BME69X_E_NULL_PTR;

    comm->wireObj->beginTransmission(comm->i2cAddr);
    comm->wireObj->write(regAddr);
    for (i = 0; i < length; i++) {
        comm->wireObj->write(regData[i]);
    }
    if (comm->wireObj->endTransmission())
        rslt = BME69X_E_COM_FAIL;

    return rslt;
}

int8_t bme69xI2cRead(uint8_t regAddr, uint8_t *regData, uint32_t length, void *intfPtr)
{
    uint32_t i;
    int8_t rslt = BME69X_OK;
    bme69xCommT *comm = nullptr;

    if (length > BME69X_I2C_BUFFER_SIZE)
        return BME69X_E_COM_FAIL;

    if (intfPtr == nullptr)
        return BME69X_E_NULL_PTR;

    comm = (bme69xCommT *)intfPtr;
    if (comm->wireObj == nullptr)
        return BME69X_E_NULL_PTR;

    comm->wireObj->beginTransmission(comm->i2cAddr);
    comm->wireObj->write(regAddr);
    if (comm->wireObj->endTransmission())
        return BME69X_E_COM_FAIL;

    comm->wireObj->requestFrom((int)comm->i2cAddr, (int)length);
    for (i = 0; (i < length) && comm->wireObj->available(); i++) {
        regData[i] = comm->wireObj->read();
    }

    return rslt;
}
