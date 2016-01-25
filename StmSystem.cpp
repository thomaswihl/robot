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

#include "StmSystem.h"

#include <cstdio>

StmSystem::StmSystem() :
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
    mRcc(BaseAddress::RCC, 8000000),
    mExtI(BaseAddress::EXTI, 23),
    mNvic(BaseAddress::NVIC, 82),
    mSysTick(BaseAddress::STK, &mRcc),
    mSysCfg(BaseAddress::SYSCFG),
    mDma1(BaseAddress::DMA1),
    mDma2(BaseAddress::DMA2),
    //    mUsart1(BaseAddress::USART1, &mRcc, ClockControl::Clock::APB2),
    mUsart2(BaseAddress::USART2, &mRcc, ClockControl::ClockSpeed::APB1, 512, 32),
    //    mUsart3(BaseAddress::USART3, &mRcc, ClockControl::Clock::APB1),
    //    mUart4(BaseAddress::UART4, &mRcc, ClockControl::Clock::APB1),
    //    mUart5(BaseAddress::UART5, &mRcc, ClockControl::Clock::APB1),
    //    mUsart6(BaseAddress::USART6, &mRcc, ClockControl::Clock::APB2),
    mDebug(mUsart2),
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

StmSystem::~StmSystem()
{
}

void StmSystem::handleTrap(System::TrapIndex index, unsigned int* stackPointer)
{
    mDebug.configDma(nullptr, nullptr);
    mDebug.configInterrupt(nullptr);
    System::handleTrap(index, stackPointer);
    // wait for last byte to be written
    for (int i = 0; i < 10000; ++i);
    mRcc.resetClock();
}

void StmSystem::init()
{
    mRcc.setSystemClock(168000000);
    mRcc.enable(ClockControl::Function::Usart2);
    mRcc.enable(ClockControl::Function::GpioA);
    mRcc.enable(ClockControl::Function::Dma1);

    //    mDebug.config(9600);
    mDebug.config(921600);//, Serial::Parity::Odd, Serial::WordLength::Nine);
    mDebug.configStream(new Dma::Stream(mDma1, Dma::Stream::StreamIndex::Stream6, Dma::Stream::ChannelIndex::Channel4,
                                  new InterruptController::Line(mNvic, InterruptIndex::DMA1_Stream6)),
                  new Dma::Stream(mDma1, Dma::Stream::StreamIndex::Stream5, Dma::Stream::ChannelIndex::Channel4,
                                  new InterruptController::Line(mNvic, InterruptIndex::DMA1_Stream5)),
                  new InterruptController::Line(mNvic, InterruptIndex::USART2));
    mNvic.setPriotity(InterruptIndex::DMA1_Stream6, InterruptController::Priority::Lowest);
    mNvic.setPriotity(InterruptIndex::DMA1_Stream5, InterruptController::Priority::Low);
    mDebug.enable(Device::All);

    // USART2 TX
    mGpioA.configOutput(Gpio::Index::Pin2, Gpio::OutputType::PushPull, Gpio::Pull::None, Gpio::Speed::Low);
    mGpioA.setAlternate(Gpio::Index::Pin2, Gpio::AltFunc::USART2);
    // USART2 RX
    mGpioA.configInput(Gpio::Index::Pin3, Gpio::Pull::Down);
    mGpioA.setAlternate(Gpio::Index::Pin3, Gpio::AltFunc::USART2);

    mFlash.set(Flash::Feature::InstructionCache, true);
    mFlash.set(Flash::Feature::DataCache, true);
    // Be careful with prefetch, it does nasty things...
    //mFlash.set(Flash::Feature::Prefetch, true);
    mFpu.enable(FpuControl::AccessPrivileges::Full);

}

void StmSystem::consoleRead(char *msg, unsigned int len)
{
    mDebug.read(msg, len);
}

void StmSystem::consoleWrite(const char *msg, unsigned int len)
{
    mDebug.write(msg, len);
}

void StmSystem::printInfo()
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
