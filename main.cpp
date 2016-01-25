#include <stdio.h>

#include "StmSystem.h"
#include "CommandInterpreter.h"
#include "Commands.h"
#include "Power.h"

static StmSystem sys;

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


    CommandInterpreter interpreter(sys.mDebug);
    interpreter.add(new CmdHelp());
    interpreter.add(new CmdRead());
    interpreter.add(new CmdWrite());
    Gpio* gpio[] = { &sys.mGpioA, &sys.mGpioB, &sys.mGpioC, &sys.mGpioD, &sys.mGpioE, &sys.mGpioF, &sys.mGpioG, &sys.mGpioH, &sys.mGpioI };
    interpreter.add(new CmdPin(gpio, sizeof(gpio) / sizeof(gpio[0])));
    InterruptController::Line timer5Irq(sys.mNvic, StmSystem::InterruptIndex::TIM5);
    Timer timer5(StmSystem::BaseAddress::TIM5, ClockControl::ClockSpeed::APB1);
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

