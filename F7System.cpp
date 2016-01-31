/*
 * (c) 2012 Thomas Wihl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "F7System.h"

#include <cstdio>

F7System::F7System() :
    System(BaseAddress::SCB),
    mGpioA(BaseAddress::GPIOA),
    mGpioB(BaseAddress::GPIOB),
    mGpioC(BaseAddress::GPIOC),
    mGpioD(BaseAddress::GPIOD),
    mGpioE(BaseAddress::GPIOE),
    mGpioF(BaseAddress::GPIOF),
    mGpioG(BaseAddress::GPIOG),
    mGpioH(BaseAddress::GPIOH),
    mGpioI(BaseAddress::GPIOI),
    mRcc(BaseAddress::RCC),
    mExtI(BaseAddress::EXTI, 23),
    mNvic(BaseAddress::NVIC, 82),
    mSysTick(BaseAddress::STK, &mRcc),
    mSysCfg(BaseAddress::SYSCFG),
    mDma1(BaseAddress::DMA1),
    mDma2(BaseAddress::DMA2),
    //    mUsart1(BaseAddress::USART1, &mRcc, ClockControl::Clock::APB2),
    //    mUsart2(BaseAddress::USART2, &mRcc, ClockControl::ClockSpeed::APB1, 512, 32),
    //    mUsart3(BaseAddress::USART3, &mRcc, ClockControl::Clock::APB1),
    //    mUart4(BaseAddress::UART4, &mRcc, ClockControl::Clock::APB1),
    //    mUart5(BaseAddress::UART5, &mRcc, ClockControl::Clock::APB1),
    mUsart6(BaseAddress::USART6, &mRcc, ClockControl::ClockSpeed::APB2, 512, 32),
    mDebug(mUsart6),
    mSpi1(BaseAddress::SPI1, &mRcc, ClockControl::ClockSpeed::APB2),
    mSpi2(BaseAddress::SPI2, &mRcc, ClockControl::ClockSpeed::APB1),
    mSpi3(BaseAddress::SPI3, &mRcc, ClockControl::ClockSpeed::APB1),
    mI2C1(BaseAddress::I2C1, &mRcc, ClockControl::ClockSpeed::APB1),
    mI2C2(BaseAddress::I2C2, &mRcc, ClockControl::ClockSpeed::APB1),
    mI2C3(BaseAddress::I2C3, &mRcc, ClockControl::ClockSpeed::APB1),
    mFlash(BaseAddress::FLASH, mRcc, Flash::AccessSize::x32),
    mFpu(BaseAddress::FPU),
    mIWdg(BaseAddress::IWDG)
{
    init();
}

F7System::~F7System()
{
}

void F7System::handleTrap(System::TrapIndex index, unsigned int* stackPointer)
{
    mDebug.configDma(nullptr, nullptr);
    mDebug.configInterrupt(nullptr);
    System::handleTrap(index, stackPointer);
    // wait for last byte to be written
    for (int i = 0; i < 10000; ++i);
    mRcc.resetClock();
}

void F7System::init()
{
    mRcc.setSystemClock(216000000);
    mRcc.enable(ClockControl::Function::Usart6);
    mRcc.enable(ClockControl::Function::GpioC);
    mRcc.enable(ClockControl::Function::Dma2);

    mDebug.config(115200);
    //mDebug.config(921600);//, Serial::Parity::Odd, Serial::WordLength::Nine);
    mDebug.configStream(new Dma::Stream(mDma2, Dma::Stream::StreamIndex::Stream7, Dma::Stream::ChannelIndex::Channel5,
                                  new InterruptController::Line(mNvic, InterruptIndex::DMA2_Stream7)),
                  new Dma::Stream(mDma2, Dma::Stream::StreamIndex::Stream2, Dma::Stream::ChannelIndex::Channel5,
                                  new InterruptController::Line(mNvic, InterruptIndex::DMA2_Stream2)),
                  new InterruptController::Line(mNvic, InterruptIndex::USART6));
    mNvic.setPriotity(InterruptIndex::DMA2_Stream7, InterruptController::Priority::Lowest);
    mNvic.setPriotity(InterruptIndex::DMA2_Stream2, InterruptController::Priority::Low);
    mDebug.enable(Device::All);

    // USART2 TX
    mGpioC.configOutput(Gpio::Index::Pin6, Gpio::OutputType::PushPull, Gpio::Pull::None, Gpio::Speed::Low);
    mGpioC.setAlternate(Gpio::Index::Pin6, Gpio::AltFunc::USART6);
    // USART2 RX
    mGpioC.configInput(Gpio::Index::Pin7, Gpio::Pull::Down);
    mGpioC.setAlternate(Gpio::Index::Pin7, Gpio::AltFunc::USART6);

    mFlash.set(Flash::Feature::InstructionCache, true);
    mFlash.set(Flash::Feature::DataCache, true);
    // Be careful with prefetch, it does nasty things...
    //mFlash.set(Flash::Feature::Prefetch, true);
    mFpu.enable(FpuControl::AccessPrivileges::Full);

}

void F7System::consoleRead(char *msg, unsigned int len)
{
    mDebug.read(msg, len);
}

void F7System::consoleWrite(const char *msg, unsigned int len)
{
    mDebug.write(msg, len);
}

void F7System::printInfo()
{
    updateBogoMips();
    ClockControl::Reset::Reason rr = mRcc.resetReason();
    std::printf("\n\n\nRESET: ");
    if (rr & ClockControl::Reset::LowPower) printf("LOW POWER   ");
    if (rr & ClockControl::Reset::WindowWatchdog) printf("WINDOW WATCHDOG   ");
    if (rr & ClockControl::Reset::IndependentWatchdog) printf("INDEPENDENT WATCHDOG   ");
    if (rr & ClockControl::Reset::Software) printf("SOFTWARE RESET   ");
    if (rr & ClockControl::Reset::PowerOn) printf("POWER ON   ");
    if (rr & ClockControl::Reset::Pin) printf("PIN RESET   ");
    if (rr & ClockControl::Reset::BrownOut) printf("BROWN OUT   ");
    std::printf("\n");
    std::printf("CLOCK   : System = %luMHz, AHB = %luMHz, APB1 = %luMHz, APB2 = %luMHz\n",
                mRcc.clock(ClockControl::ClockSpeed::System) / 1000000,
                mRcc.clock(ClockControl::ClockSpeed::AHB) / 1000000,
                mRcc.clock(ClockControl::ClockSpeed::APB1) / 1000000,
                mRcc.clock(ClockControl::ClockSpeed::APB2) / 1000000);
    std::printf("BOGOMIPS: %lu.%lu\n", bogoMips() / 1000000, bogoMips() % 1000000);
    std::printf("RAM     : %luk heap free, %luk heap used, %luk bss used, %lik data used.\n", (memFree() + 512) / 1024, (memUsed() + 512) / 1024, (memBssUsed() + 512) / 1024, (memDataUsed() + 512) / 1024);
    std::printf("STACK   : %luk free, %luk used, %luk max used.\n", (stackFree() + 512) / 1024, (stackUsed() + 512) / 1024, (stackMaxUsed() + 512) / 1024);
}
