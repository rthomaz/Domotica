/*
  CeilingFanCore.h - Library for Led Light code.
  Created by Rodrigo Thomaz, June 4, 2017.
  Released into the public domain.
*/
#ifndef CeilingFanCore_h
#define CeilingFanCore_h

#include "Arduino.h"

class CeilingFanCore
{
  public:
    CeilingFanCore(int digitalOutPinFanDirectionForward, int digitalOutPinFanDirectionReverse, int analogOutPinFanSpeed, int addressLastSpeedFanDirectionForward, int addressLastSpeedFanDirectionReverse);
    String getFanDirection();
    byte setFanDirection(String value);
    byte getFanSpeed();
    void setFanSpeed(String fanDirection, byte value);
  private:
    int _digitalOutPinFanDirectionForward;      // Digital output pin that the Fan Direction Forward is attached to
    int _digitalOutPinFanDirectionReverse;      // Digital output pin that the Fan Direction Reverse is attached to
    int _analogOutPinFanSpeed;                  // Analog output pin that the Fan Speed is attached to    
    int _addressLastSpeedFanDirectionForward;   // Address of last speed of fan direction forward
    int _addressLastSpeedFanDirectionReverse;   // Address of default value of fan direction reverse
    byte getLastFanSpeed(String fanDirection);
    void setLastFanSpeed(String fanDirection, byte value);
};

#endif
