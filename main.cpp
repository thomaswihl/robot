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
    sys.mRcc.enable(ClockControl::Function::GpioD);

    sys.mGpioD.configOutput(Gpio::Index::Pin12, Gpio::OutputType::PushPull);
    sys.mGpioD.configOutput(Gpio::Index::Pin13, Gpio::OutputType::PushPull);
    sys.mGpioD.configOutput(Gpio::Index::Pin14, Gpio::OutputType::PushPull);
    sys.mGpioD.configOutput(Gpio::Index::Pin15, Gpio::OutputType::PushPull);

    Gpio::Pin led0(sys.mGpioD, Gpio::Index::Pin12);
    Gpio::Pin led1(sys.mGpioD, Gpio::Index::Pin13);
    Gpio::Pin led2(sys.mGpioD, Gpio::Index::Pin14);
    Gpio::Pin led3(sys.mGpioD, Gpio::Index::Pin15);

    led0.set();
    sys.printInfo();

    // RK043FN48H LCD 480x272
    /*hLtdcHandler.Init.HorizontalSync = (RK043FN48H_HSYNC - 1);
    hLtdcHandler.Init.VerticalSync = (RK043FN48H_VSYNC - 1);
    hLtdcHandler.Init.AccumulatedHBP = (RK043FN48H_HSYNC + RK043FN48H_HBP - 1);
    hLtdcHandler.Init.AccumulatedVBP = (RK043FN48H_VSYNC + RK043FN48H_VBP - 1);
    hLtdcHandler.Init.AccumulatedActiveH = (RK043FN48H_HEIGHT + RK043FN48H_VSYNC + RK043FN48H_VBP - 1);
    hLtdcHandler.Init.AccumulatedActiveW = (RK043FN48H_WIDTH + RK043FN48H_HSYNC + RK043FN48H_HBP - 1);
    hLtdcHandler.Init.TotalHeigh = (RK043FN48H_HEIGHT + RK043FN48H_VSYNC + RK043FN48H_VBP + RK043FN48H_VFP - 1);
    hLtdcHandler.Init.TotalWidth = (RK043FN48H_WIDTH + RK043FN48H_HSYNC + RK043FN48H_HBP + RK043FN48H_HFP - 1);
    */

//#define  RK043FN48H_WIDTH    ((uint16_t)480)          /* LCD PIXEL WIDTH            */
//#define  RK043FN48H_HEIGHT   ((uint16_t)272)          /* LCD PIXEL HEIGHT           */
//#define  RK043FN48H_HSYNC            ((uint16_t)41)   /* Horizontal synchronization */
//#define  RK043FN48H_HBP              ((uint16_t)13)   /* Horizontal back porch      */
//#define  RK043FN48H_HFP              ((uint16_t)32)   /* Horizontal front porch     */
//#define  RK043FN48H_VSYNC            ((uint16_t)10)   /* Vertical synchronization   */
//#define  RK043FN48H_VBP              ((uint16_t)2)    /* Vertical back porch        */
//#define  RK043FN48H_VFP              ((uint16_t)2)    /* Vertical front porch       */
//#define  RK043FN48H_FREQUENCY_DIVIDER    5            /* LCD Frequency divider      */

    /* RK043FN48H LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */
//    periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
//    periph_clk_init_struct.PLLSAI.PLLSAIN = 192;
//    periph_clk_init_struct.PLLSAI.PLLSAIR = RK043FN48H_FREQUENCY_DIVIDER;
//    periph_clk_init_struct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
//    HAL_RCCEx_PeriphCLKConfig(&periph_clk_init_struct);

    sys.mRcc.enable(ClockControl::Function::GpioE);
    sys.mRcc.enable(ClockControl::Function::GpioG);
    sys.mRcc.enable(ClockControl::Function::GpioI);
    sys.mRcc.enable(ClockControl::Function::GpioJ);
    sys.mRcc.enable(ClockControl::Function::GpioK);
    sys.mRcc.setSaiClock(9000000);
    sys.mRcc.enable(ClockControl::Function::Ltdc);
    sys.mGpioE.configAlternate(Gpio::Index::Pin4, Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    sys.mGpioG.configAlternate(Gpio::Index::Pin12, Gpio::AltFunc::LCD_AF9, Gpio::Speed::Fast);
    for (int i = 9; i <= 10; ++i)
    {
        sys.mGpioI.configAlternate(static_cast<Gpio::Index>(i), Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    }
    Gpio::Pin lcdDisp(sys.mGpioI, Gpio::Index::Pin12);
    sys.mGpioI.configOutput(Gpio::Index::Pin12, Gpio::OutputType::PushPull, Gpio::Speed::Fast);
    for (int i = 13; i <= 15; ++i)
    {
        sys.mGpioI.configAlternate(static_cast<Gpio::Index>(i), Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    }
    for (int i = 0; i <= 11; ++i)
    {
        sys.mGpioJ.configAlternate(static_cast<Gpio::Index>(i), Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    }
    for (int i = 13; i <= 15; ++i)
    {
        sys.mGpioJ.configAlternate(static_cast<Gpio::Index>(i), Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    }
    for (int i = 0; i <= 2; ++i)
    {
        sys.mGpioK.configAlternate(static_cast<Gpio::Index>(i), Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    }
    Gpio::Pin lcdBacklight(sys.mGpioK, Gpio::Index::Pin3);
    sys.mGpioK.configOutput(Gpio::Index::Pin3, Gpio::OutputType::PushPull, Gpio::Speed::Fast);
    for (int i = 4; i <= 7; ++i)
    {
        sys.mGpioK.configAlternate(static_cast<Gpio::Index>(i), Gpio::AltFunc::LCD_AF14, Gpio::Speed::Fast);
    }
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
    Gpio* gpio[] = { &sys.mGpioA, &sys.mGpioB, &sys.mGpioC, &sys.mGpioD, &sys.mGpioE, &sys.mGpioF, &sys.mGpioG, &sys.mGpioH, &sys.mGpioI, &sys.mGpioJ, &sys.mGpioK };
    interpreter.add(new CmdPin(gpio, sizeof(gpio) / sizeof(gpio[0])));
    InterruptController::Line timer5Irq(sys.mNvic, F7System::InterruptIndex::TIM5);
    Timer timer5(F7System::BaseAddress::TIM5, ClockControl::ClockSpeed::APB1);
//    timer5.setInterrupt(Timer::InterruptType::CaptureCompare, &timer5Irq);
    interpreter.add(new CmdMeasureClock(sys.mRcc, timer5));
    interpreter.start();

    while (true)
    {
        if (sys.waitForEvent(event) && event != nullptr)
        {
            led1.set();
            event->callback();
            led1.reset();
        }

    }
    return 0;
}

