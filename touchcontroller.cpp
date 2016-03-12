#include "stdio.h"

#include "touchcontroller.h"
/* No IRQ
 0: 00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
10: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
20: ff ff 01 36 e5 0e 82 00 1b 7d 37 0b 74 7e 24 00
30: 49 7e 01 ff ff ff ff ff ff ff ff ff ff ff ff 00
40: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80: 17 5c 0f 0f 0f a0 01 1e 09 1e 00 00 00 00 00 00
90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
a0: 00 05 0c 14 01 01 12 01 51 01 03 00 00 00 00 0c
b0: 00 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00
c0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
d0: 00 1f 00 00 00 00 00 00 00 00 00 00 00 00 00 00
e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
f0: 00 00 00 00 00 00 00 00 00 00 00 00 01 40 1f

Constant IRQ
 0: 00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
10: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
20: ff ff 01 36 e5 0e 82 00 1b 7d 37 0b 74 7e 24 00
30: 49 7e 01 ff ff ff ff ff ff ff ff ff ff ff ff 00
40: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80: 17 5c 0f 0f 0f a0 01 02 09 1e 00 00 00 00 00 00
90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
a0: 00 05 0c 14 01 01 12 01 51 01 03 00 00 00 00 0c
b0: 00 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00
c0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
d0: 00 1f 00 00 00 00 00 00 00 00 00 00 00 00 00 00
e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
f0: 00 00 00 00 00 00 00 00 00 00 00 00 01 40 1f

*/


/*   0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
 0: 00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
10: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
20: ff ff 01 36 e5 0e 82 00 1b 7d 37 0b 74 7e 24 00
30: 49 7e 01 ff ff ff ff ff ff ff ff ff ff ff ff 00
40: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80: 17 5c 0f 0f 0f a0 01 1e 09 1e 00 00 00 00 00 00
90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
a0: 00 05 0c 14 01 01 12 01 51 01 03 00 00 00 00 0c
b0: 00 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00
c0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
d0: 00 1f 00 00 00 00 00 00 00 00 00 00 00 00 00 00
e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
f0: 00 00 00 00
*/

TouchController::TouchController(I2C& i2c, uint8_t address) :
    mGetPosEvent(*this),
    mConfigEvent(*this),
    mI2C(i2c),
    mLine(nullptr),
    mLastWasEmpty(false)
  #ifdef DEBUG_TOUCH
    ,mReadAllEvent(*this)
  #endif
{
    memset(&mGetPos, 0, sizeof(mGetPos));
    mGetPos.mAddress = address;
    mGetPos.mEvent = &mGetPosEvent;
    mGetPos.mWriteData = mGetPosWriteData = new uint8_t[1];
    mGetPos.mReadData = mGetPosReadData = new uint8_t[GET_POS_DATA_LEN];
    mGetPosWriteData[0] = 2;
    mGetPos.mWriteLength = 1;
    mGetPos.mReadLength = GET_POS_DATA_LEN;

#ifdef DEBUG_TOUCH
    memset(&mReadAll, 0, sizeof(mReadAll));
    mReadAll.mAddress = address;
    mReadAll.mEvent = &mReadAllEvent;
    mReadAll.mWriteData = mReadAllWriteData = new uint8_t[1];
    mReadAll.mReadData = mReadAllReadData = new uint8_t[255];
    mReadAllWriteData[0] = 0;
    mReadAll.mWriteLength = 1;
    mReadAll.mReadLength = 255;
    System::instance()->printInfo();
    mI2C.transfer(&mReadAll);
#endif


//    memset(&mConfig, 0, sizeof(mConfig));
//    mConfig.mAddress = address;
//    mConfig.mEvent = &mConfigEvent;
//    mConfig.mWriteData = mConfigWriteData = new uint8_t[2];
//    mConfigWriteData[0] = 0x87;
//    mConfigWriteData[1] = 2;
//    mConfig.mWriteLength = 2;
//    mI2C.transfer(&mConfig);
//#ifdef DEBUG_TOUCH
//    mI2C.transfer(&mReadAll);
//#endif

}

void TouchController::configInterrupt(ExternalInterrupt::Line *line)
{
    mLine = line;
    line->setCallback(this);
    line->enable(ExternalInterrupt::Trigger::Falling);
}

void TouchController::update()
{

}

void TouchController::eventCallback(System::Event *event)
{
#ifdef DEBUG_TOUCH
    if (event == &mReadAllEvent)
    {
        for (unsigned i = 0; i < mReadAll.mReadLength; ++i)
        {
            if ((i % 16) == 0) printf("\n%2x:", i);
            printf(" %02x", mReadAll.mReadData[i]);
        }
        printf("\n");
    }
#endif
    if (event == &mGetPosEvent)
    {
        static uint64_t lastUs = 0;
        static const int SKIP = 0;
        int count = mGetPosReadData[SKIP] & 0x0f;
        if (count > MAX_TOUCH_POINT) return;
        if (count == 0)
        {
            mLastWasEmpty = true;
            return;
        }
        int x[MAX_TOUCH_POINT], y[MAX_TOUCH_POINT], event[MAX_TOUCH_POINT], weight[MAX_TOUCH_POINT], id[MAX_TOUCH_POINT];
        uint64_t us = System::instance()->ns() / 1000;
        uint64_t delta = us - lastUs;
        printf("%llums %i: ", delta / 1000, count);
        lastUs = us;
        for (int i = 0; i < count; ++i)
        {
            x[i] = ((mGetPosReadData[SKIP + 3 + i * 6] & 0x0f) << 8) |  mGetPosReadData[SKIP + 4 + i * 6];
            y[i] = ((mGetPosReadData[SKIP + 1 + i * 6] & 0x0f) << 8) |  mGetPosReadData[SKIP + 2 + i * 6];
            weight[i] = mGetPosReadData[7];
            event[i] = static_cast<TouchEvent>(mGetPosReadData[SKIP + 1 + i * 6] >> 6);
            id[i] = static_cast<TouchEvent>(mGetPosReadData[SKIP + 3 + i * 6] >> 4);
            if (event[i] != NoEvent) printf("(%i: %4ix%4i, e=%i, w=%i), ", id[i], x[i], y[i], event[i], weight[i]);
        }
        printf("\n");
    }
}

void TouchController::interruptCallback(InterruptController::Index /*index*/)
{
    if (!mLastWasEmpty) mI2C.transfer(&mGetPos);
    else mLastWasEmpty = false;
//#ifdef DEBUG_TOUCH
//    mI2C.transfer(&mReadAll);
//#endif
}
