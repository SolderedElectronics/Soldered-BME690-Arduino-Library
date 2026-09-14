/**
 * @file Generic-Qwiic-SOLDERED.h
 * @brief Header file for sensor specific code.
 * @author Soldered Electronics
 */

#ifndef SENSOR_H
#define SENSOR_H

#include "Arduino.h"
#include "libs/Generic-Qwiic/qwiic.hpp"

class Sensor : public Qwiic
{
  public:
    /**
     * @brief                   Sensor specific native constructor.
     *
     * @param int _pin          Example parameter.
     */
    Sensor(int _pin);

  protected:
    /**
     * @brief                   Overloaded function for virtual in base class to initialize sensor specific.
     */
    void initializeNative();

  private:
    int pin;
};

#endif
