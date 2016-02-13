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
    sys.configAlternate("C3,D0-1,D8-10,D14-15,E0-1,E7-15,F0-5,F11-15,G0-1,G4-5,G8,G15,H3,H5", Gpio::AltFunc::FMC, Gpio::Speed::Fast, Gpio::Pull::Up);
    MemoryController mem(F7System::BaseAddress::FMC_CONTROL);
    /*
    FMC_Bank5_6->SDCR[0] = 0x000019E0;
    burst, halfclock, cas latency 3, 4 banks, 32 bit
    FMC_Bank5_6->SDTR[0] = 0x01116361;
    TRCD=1, TRP=1, TWR=1, TRC=6, TRAS=3, TXSR=6, TMRD=1
    */

    MemoryController::SdRam ram(MemoryController::SdRam::Bank::Bank1,
                                MemoryController::SdRam::CasLatency::Cycle2,
                                MemoryController::SdRam::DataBus::Width16,
                                MemoryController::SdRam::RowAddress::Bits12,
                                MemoryController::SdRam::ColumnAddress::Bits8,
                                MemoryController::SdRam::BankCount::Four);
    ram.setTiming(2, 2, 2, 7, 4, 7, 2);
    mem.sdRamConfig(&ram, nullptr, MemoryController::SdRamClock::OneHalf);

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
    uint32_t* buffer = (uint32_t*)F7System::BaseAddress::FMC_BANK5;
    uint32_t* p = buffer;

    //for (int i = 0; i < 256; ++i) lcd.setPaletteEntry(LcdController::Layer::Layer0, i, i, i, 0);

    for (int y = 0; y < 272; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int i = (int)sqrtf(x*x + y*y) * 6;
            uint8_t r = 0, g = 0, b = 0;
            uint8_t m = i;
            if (i < 256)
            {
                r = i;
            }
            else if (i < 512)
            {
                r = 255;
                g = m;
            }
            else if (i < 512+256)
            {
                r = 255 - m;
                g = 255;
            }
            else if (i < 1024)
            {
                g = 255;
                b = m;
            }
            else if (i < 1024+256)
            {
                g = 255 - m;
                b = 255;
            }
            else if (i < 1024+512)
            {
                r = m;
                b = 255;
            }
            else if (i < 1024+512+256)
            {
                r = 255-m;
                b = 255-m;
            }
            *p++ = (255 << 24) | (r << 16) | (g << 8) | b;
        }
    }
    LcdController::LayerConfig layer(LcdController::Layer::Layer0, LcdController::PixelFormat::ARGB8888, 0, 0, width, 272, buffer);
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

