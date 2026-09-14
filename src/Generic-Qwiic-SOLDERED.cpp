/**
 * @file Generic-Qwiic-SOLDERED.cpp
 * @brief Example functions to overload in base class.
 * @author Soldered Electronics
 */


#include "Generic-Qwiic-SOLDERED.h"


Sensor::Sensor(int _pin)
{
    pin = _pin;
    native = 1;
}

void Sensor::initializeNative()
{
    pinMode(pin, INPUT);
}
