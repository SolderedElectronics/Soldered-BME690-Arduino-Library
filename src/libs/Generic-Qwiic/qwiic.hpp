/**
 * @file qwiic.hpp
 * @brief Basic functions for Qwiic libraries
 * @author Soldered Electronics
 */

#ifndef QWIIC_H
#define QWIIC_H

#include "Arduino.h"
#include "Wire.h"

#define ANALOG_READ_REG 0
#define DIGITAL_READ_REG 1

class Qwiic
{
public:
    /**
     * @brief       Main constructor for Qwiic version
     *
     */
    Qwiic()
    {
        native = false;
    }

    virtual ~Qwiic() {}

    /**
     * @brief       Initializes sensors on native or Qwiic on default address
     */
    void begin()
    {
        if (native)
            initializeNative();
        else
            begin(defaultAddress);
        beginDone = 1;
    }

    /**
     * @brief                  Initializes sensors on supplied i2c address
     *
     * @param uint8_t _address Custom Qwiic sensor address
     */
    void begin(uint8_t _address)
    {
        address = _address;

        Wire.begin();

        beginDone = 1;
    }

protected:
    bool native = false;
    bool beginDone = 0;

    virtual void initializeNative() = 0;

    int err;

    char address;
    const char defaultAddress = 0x30;

    /**
     * @brief                Private function to send a single byte to sensor
     *
     * @param  char regAddr  Address of register to access later
     *
     * @return int           Standard endTransmission error codes
     */
    int sendAddress(char regAddr)
    {
        Wire.beginTransmission(address);
        Wire.write(regAddr);

        return err = Wire.endTransmission();
    }

    /**
     * @brief           Private function to read n bytes over i2c
     *
     * @param  char a[] Array to read data to
     * @param  int n    Number of bytes to read
     *
     * @return int      Error code, always 0
     */
    int readData(char a[], int n)
    {
        Wire.requestFrom(address, n);
        Wire.readBytes(a, n);

        return 0;
    }

    /**
     * @brief                   Private function to send over i2c and then read n bytes
     *
     * @param char regAddr      Address of register to access data from
     * @param char a            Array to put data in
     * @param size_t n          Size of data to read
     *
     * @return int              0 if read successfuly, error code from endTransmission if not
     */
    int readRegister(char regAddr, char a[], size_t n)
    {
        if (sendAddress(regAddr))
            return err;

        if (readData(a, n))
            return err;

        return 0;
    }

    /**
     * @brief           Private function to write n bytes over i2c
     *
     * @param char a[]  Array to read data from
     * @param int n     Number of bytes to read
     *
     * @return int       Standard endTransmission error codes
     */
    int sendData(const uint8_t *a, int n)
    {
        Wire.beginTransmission(address);
        Wire.write(a, n);

        return err = Wire.endTransmission();
    }
};

#endif
