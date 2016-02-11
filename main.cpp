#include <stdio.h>
#include <math.h>

#include "F7System.h"
#include "CommandInterpreter.h"
#include "Commands.h"
#include "Power.h"
#include "LcdController.h"

static F7System sys;

int main()
{
    System::Event* event;

//    sys.configOutput("d12-15");

//    Gpio::Pin led0(*sys.gpio('D'), Gpio::Index::Pin12);
//    Gpio::Pin led1(*sys.gpio('D'), Gpio::Index::Pin13);
//    Gpio::Pin led2(*sys.gpio('D'), Gpio::Index::Pin14);
//    Gpio::Pin led3(*sys.gpio('D'), Gpio::Index::Pin15);

//    led0.set();
    sys.printInfo();

    sys.clockControl()->setSaiClock(9000000);
    sys.clockControl()->enable(ClockControl::Function::Ltdc);
    sys.configAlternate("G12", Gpio::AltFunc::LCD_AF9, Gpio::Speed::Fast);
    sys.configAlternate("E4,I9-10,I13-15,J0-11,J13-15,K0-2,K4-7", Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    Gpio::ConfigurablePin lcdDisp(*sys.gpio('I'), Gpio::Index::Pin12);
    lcdDisp.configOutput(Gpio::OutputType::PushPull, Gpio::Speed::Fast);
    Gpio::ConfigurablePin lcdBacklight(*sys.gpio('K'), Gpio::Index::Pin3);
    lcdBacklight.configOutput(Gpio::OutputType::PushPull, Gpio::Speed::Fast);

    LcdController lcd(F7System::BaseAddress::LCD, 480, 272);
    //lcd.config(41, 2, 2, 10, 2, 2);
    lcd.config(41, 13, 32, 10, 2, 2);
    //lcd.configSignals(LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow);
    int width = 480;
    uint8_t* buffer = (uint8_t*)malloc(width*272*1);
    uint8_t* p = buffer;

    for (int i = 0; i < 256; ++i) lcd.setPaletteEntry(LcdController::Layer::Layer0, i, i, i, 0);

    for (int y = 0; y < 272; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
//            uint8_t r = 0, g = 0, b = 0;
//            if (x == y) r = g = b = 255;
//            uint8_t r = std::min(255, (int)sqrtf(x*x + y*y));
//            uint8_t g = 0;//std::min(255, (int)sqrtf((width - x)*(width - x) + y*y));
//            uint8_t b = 0;//std::min(255, (int)sqrtf(x*x + (272 - y)*(272 - y)));
            int r = (int)sqrtf(x*x + y*y);
            if (r > 255)
            {
                r -= 256;
                r = 255 - r;
            }
            if (r < 0)
            {
                r = -r;
            }
            *p++ = r;
        }
    }
    LcdController::LayerConfig layer(LcdController::Layer::Layer0, LcdController::PixelFormat::L8, 0, 0, width, 272, buffer);
    lcd.enable(layer);
    lcd.enable();
    lcdDisp.set();
    lcdBacklight.set();
    CommandInterpreter interpreter(sys.mDebug);
    interpreter.add(new CmdHelp());
    interpreter.add(new CmdRead());
    interpreter.add(new CmdWrite());
    interpreter.add(new CmdPin(sys.gpio(), sys.gpioCount()));

    InterruptController::Line timer5Irq(sys.mNvic, F7System::InterruptIndex::TIM5);
    Timer timer5(F7System::BaseAddress::TIM5, ClockControl::ClockSpeed::APB1);
//    timer5.setInterrupt(Timer::InterruptType::CaptureCompare, &timer5Irq);
    interpreter.add(new CmdMeasureClock(*sys.clockControl(), timer5));
    interpreter.start();

    while (true)
    {
        if (sys.waitForEvent(event) && event != nullptr)
        {
//            led1.set();
            event->callback();
//            led1.reset();
        }

    }
    return 0;
}

