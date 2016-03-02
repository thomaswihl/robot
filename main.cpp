#include <stdio.h>
#include <math.h>

#include "F7System.h"
#include "CommandInterpreter.h"
#include "Commands.h"
#include "Power.h"
#include "LcdController.h"
#include "memorycontroller.h"
#include "i2c.h"
#include "InterruptController.h"
//#include "bild.h"

static F7System sys;
static const int LCD_WIDTH = 480;
static const int LCD_HEIGHT = 272;

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
    sys.configAlternate("C3,D0-1,D8-10,D14-15,E0-1,E7-15,F0-5,F11-15,G0-1,G4-5,G8,G15,H3,H5", Gpio::AltFunc::FMC, Gpio::OutputType::PushPull, Gpio::Speed::Fast, Gpio::Pull::Up);
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
    sys.configAlternate("G12", Gpio::AltFunc::LCD_AF9, Gpio::OutputType::PushPull, Gpio::Speed::Fast);
    sys.configAlternate("E4,I9-10,I13-15,J0-11,J13-15,K0-2,K4-7", Gpio::AltFunc::LCD_AF14, Gpio::OutputType::PushPull, Gpio::Speed::Fast);

    Gpio::ConfigurablePin lcdDisp(*sys.gpio('I'), Gpio::Index::Pin12);
    lcdDisp.configOutput(Gpio::OutputType::PushPull, Gpio::Speed::Fast);
    Gpio::ConfigurablePin lcdBacklight(*sys.gpio('K'), Gpio::Index::Pin3);
    lcdBacklight.configOutput(Gpio::OutputType::PushPull, Gpio::Speed::Fast);

    LcdController lcd(F7System::BaseAddress::LCD, LCD_WIDTH, LCD_HEIGHT);
    lcd.config(41, 2, 2, 10, 2, 2);
    //lcd.configSignals(LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow, LcdController::Polarity::ActiveLow);
    uint32_t* buffer1 = (uint32_t*)F7System::BaseAddress::FMC_BANK5;
    uint32_t* buffer2 = buffer1 + LCD_WIDTH * LCD_HEIGHT;


    //for (int i = 0; i < 256; ++i) lcd.setPaletteEntry(LcdController::Layer::Layer0, i, i, i, 0);

//    const uint8_t* src = gimp_image.pixel_data;
//    uint32_t* p = buffer1;
//    for (unsigned y = 0; y < gimp_image.height; ++y)
//    {
//        for (unsigned x = 0; x < gimp_image.width; ++x)
//        {
//            *p++ = (src[3] << 24) | (src[0] << 16) | (src[1] << 8) | src[2];
//            src += 4;
//        }
//    }

    uint32_t* p = buffer1;
    for (unsigned y = 0; y < LCD_HEIGHT; ++y)
    {
        for (unsigned x = 0; x < LCD_WIDTH; ++x)
        {
            *p++ = (255 << 24) | (255 << 16) | (190 << 8) | 190;
        }
    }

    p = buffer2;
    int xcenter = LCD_WIDTH / 2;
    int ycenter = LCD_HEIGHT;
    for (int y = 0; y < LCD_HEIGHT; ++y)
    {
        for (int x = 0; x < LCD_WIDTH; ++x)
        {
            int i = (int)(sqrtf((xcenter - x)*(xcenter - x) / 4 + (ycenter - y)*(ycenter - y)) * 16.0f + 0.5f);
            uint8_t a = 0, r = 0, g = 0, b = 0;
            if (i > 1024 && i < 1024+1024+512+256)
            {
                uint8_t m = i;
                a = 255;
                i -= 1024;
                if (i < 256)
                {
                    a = i;
                    r = 255;
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
                    r = 255;
                    b = 255;
                    a = 255 - i;
                }
            }
            *p++ = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    LcdController::LayerConfig layer1(LcdController::Layer::Layer0, LcdController::PixelFormat::ARGB8888, 0, 0, LCD_WIDTH, LCD_HEIGHT, buffer1);
    LcdController::LayerConfig layer2(LcdController::Layer::Layer1, LcdController::PixelFormat::ARGB8888, 0, 0, LCD_WIDTH, LCD_HEIGHT, buffer2);
    layer2.setAlpha(0);
    lcd.enable(layer1);
    lcd.enable(layer2);
    lcd.enable();
    lcdDisp.set();
    lcdBacklight.set();

    layer2.setAlpha(255);
    lcd.enable(layer2);
//    for (int i = 0; i < 256; ++i)
//    {
//        layer2.setAlpha(i);
//        lcd.enable(layer2);
//        while (lcd.reloadActive())
//        { }
//    }

    sys.configAlternate("H7-8", Gpio::AltFunc::I2C3, Gpio::OutputType::OpenDrain);
    sys.clockControl()->enable(ClockControl::Function::I2C3);
    sys.clockControl()->enable(ClockControl::Function::Dma1);
    I2C i2c(F7System::BaseAddress::I2C3, sys.clockControl(), ClockControl::ClockSpeed::APB1);
    i2c.configDma(new Dma::Stream(sys.mDma1, Dma::Stream::StreamIndex::Stream4, Dma::Stream::ChannelIndex::Channel3,
                                  new InterruptController::Line(sys.mNvic, F7System::InterruptIndex::DMA1_Stream4)),
                  new Dma::Stream(sys.mDma1, Dma::Stream::StreamIndex::Stream2, Dma::Stream::ChannelIndex::Channel3,
                                  new InterruptController::Line(sys.mNvic, F7System::InterruptIndex::DMA1_Stream2)));
    i2c.configInterrupt(new InterruptController::Line(sys.mNvic, F7System::InterruptIndex::I2C3_EV),
                        new InterruptController::Line(sys.mNvic, F7System::InterruptIndex::I2C3_ER));
    // 0x40912732
    i2c.configMaster(100000, I2C::DutyCycle::Standard);
    I2C::Transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.mAddress = 0x70;
    const uint8_t tx[] = { 0x70 };
    tr.mWriteData = tx;
    tr.mWriteLength = 1;
    tr.mReadData = new uint8_t[0x1f];
    tr.mReadLength = 0x1f;
    i2c.transfer(&tr);

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

