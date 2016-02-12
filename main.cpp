#include <stdio.h>
#include <math.h>

#include "F7System.h"
#include "CommandInterpreter.h"
#include "Commands.h"
#include "Power.h"
#include "LcdController.h"
#include "memorycontroller.h"

static F7System sys;

int main()
{
    System::Event* event;

    sys.printInfo();

    /* FMC: MT48LC4M32B2B5-6A (MICRON)
     * 128Mbit = 1Mx32x4 banks, 167MHz, 3-3-3, RCD 18ns, RP 18ns, CL 18ns, Refresh count 4k
     *
     * A0-11: F0-5,F12-15,G0-1
     * D0-15: D14-15,D0-1,E7-15,D8-10
     * SDNWE: H5
     * SDNRAS: F11
     * SDNCAS: G15
     * SDCLK: G8
     * BA0-1: G4-5
     * SDNE0: H3
     * SDCKE0: C3
     * NBL0-1: E0-1
     */
    sys.clockControl()->enable(ClockControl::Function::Fsmc);
    sys.configAlternate("C3,D0-1,D8-10,D14-15,E0-1,E7-15,F0-5,F11-15,G0-1,G4-5,G8,G15,H3,H5", Gpio::AltFunc::FMC, Gpio::Speed::Fast, Gpio::Pull::Down);
    MemoryController mem(F7System::BaseAddress::FMC_CONTROL);

    /* LCD display */
    sys.clockControl()->setSaiClock(9000000);
    sys.clockControl()->enable(ClockControl::Function::Ltdc);
    sys.configAlternate("G12", Gpio::AltFunc::LCD_AF9, Gpio::Speed::Fast);
    sys.configAlternate("E4,I9-10,I13-15,J0-11,J13-15,K0-2,K4-7", Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);

    Gpio::ConfigurablePin lcdDisp(*sys.gpio('I'), Gpio::Index::Pin12);
    lcdDisp.configOutput(Gpio::OutputType::PushPull, Gpio::Speed::Fast);
    Gpio::ConfigurablePin lcdBacklight(*sys.gpio('K'), Gpio::Index::Pin3);
    lcdBacklight.configOutput(Gpio::OutputType::PushPull, Gpio::Speed::Fast);

    LcdController lcd(F7System::BaseAddress::LCD, 480, 272);
    lcd.config(41, 2, 2, 10, 2, 2);
    //lcd.configSignals(LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow);
    int width = 480;
    uint8_t* buffer = (uint8_t*)malloc(width*272*1);
    uint8_t* p = buffer;

    for (int i = 0; i < 256; ++i) lcd.setPaletteEntry(LcdController::Layer::Layer0, i, i, i, 0);

    for (int y = 0; y < 272; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
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

    /* Command interpreter */
    CommandInterpreter interpreter(sys.mDebug);
    interpreter.add(new CmdHelp());
    interpreter.add(new CmdInfo(sys));
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
            event->callback();
        }

    }
    return 0;
}

