#ifndef TOUCHCONTROLLER_H
#define TOUCHCONTROLLER_H

//#define DEBUG_TOUCH

#include "i2c.h"

class TouchController : public System::Event::Callback, public ExternalInterrupt::Callback
{
public:
    enum TouchEvent { PressDown, LiftUp, Contact, NoEvent };
    TouchController(I2C &i2c, uint8_t address);

    void configInterrupt(ExternalInterrupt::Line* line);

    void update();

private:
    void eventCallback(System::Event *event) override;
    void interruptCallback(InterruptController::Index index) override;

private:
    static const int MAX_TOUCH_POINT = 5;
    static const int GET_POS_DATA_LEN = 3 + 6 * MAX_TOUCH_POINT;
    System::Event mGetPosEvent;
    System::Event mConfigEvent;
    I2C& mI2C;
    ExternalInterrupt::Line* mLine;
    I2C::Transfer mGetPos;
    I2C::Transfer mConfig;
    uint8_t* mGetPosWriteData;
    uint8_t* mGetPosReadData;
    uint8_t* mConfigWriteData;
    bool mLastWasEmpty;
#ifdef DEBUG_TOUCH
    System::Event mReadAllEvent;
    I2C::Transfer mReadAll;
    uint8_t* mReadAllWriteData;
    uint8_t* mReadAllReadData;
#endif
};

#endif // TOUCHCONTROLLER_H
