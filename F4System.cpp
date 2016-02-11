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

#include "F4System.h"

#include <cstdio>

static const System::BaseAddress GPIO_ARRAY[] = {
    F4System::BaseAddress::GPIOA,
    F4System::BaseAddress::GPIOB,
    F4System::BaseAddress::GPIOC,
    F4System::BaseAddress::GPIOD,
    F4System::BaseAddress::GPIOE,
    F4System::BaseAddress::GPIOF,
    F4System::BaseAddress::GPIOG,
    F4System::BaseAddress::GPIOH,
    F4System::BaseAddress::GPIOI
};

F4System::F4System() :
    System(BaseAddress::SCB, GPIO_ARRAY, sizeof(GPIO_ARRAY) / sizeof(GPIO_ARRAY[0]), BaseAddress::RCC),
    mExtI(BaseAddress::EXTI, 23),
    mNvic(BaseAddress::NVIC, 82),
    mSysTick(BaseAddress::STK, clockControl()),
    mSysCfg(BaseAddress::SYSCFG),
    mDma1(BaseAddress::DMA1),
    mDma2(BaseAddress::DMA2),
    //    mUsart1(BaseAddress::USART1, &mRcc, ClockControl::Clock::APB2),
    mUsart2(BaseAddress::USART2, clockControl(), ClockControl::ClockSpeed::APB1, 512, 32),
    //    mUsart3(BaseAddress::USART3, &mRcc, ClockControl::Clock::APB1),
    //    mUart4(BaseAddress::UART4, &mRcc, ClockControl::Clock::APB1),
    //    mUart5(BaseAddress::UART5, &mRcc, ClockControl::Clock::APB1),
    //    mUsart6(BaseAddress::USART6, &mRcc, ClockControl::Clock::APB2),
    mDebug(mUsart2),
    mSpi1(BaseAddress::SPI1, clockControl(), ClockControl::ClockSpeed::APB2),
    mSpi2(BaseAddress::SPI2, clockControl(), ClockControl::ClockSpeed::APB1),
    mSpi3(BaseAddress::SPI3, clockControl(), ClockControl::ClockSpeed::APB1),
    mI2C1(BaseAddress::I2C1, clockControl(), ClockControl::ClockSpeed::APB1),
    mI2C2(BaseAddress::I2C2, clockControl(), ClockControl::ClockSpeed::APB1),
    mI2C3(BaseAddress::I2C3, clockControl(), ClockControl::ClockSpeed::APB1),
    mFlash(BaseAddress::FLASH, *clockControl(), Flash::AccessSize::x32),
    mFpu(BaseAddress::FPU),
    mIWdg(BaseAddress::IWDG)
{
    init();
}

F4System::~F4System()
{
}

void F4System::handleTrap(System::TrapIndex index, unsigned int* stackPointer)
{
    mDebug.configDma(nullptr, nullptr);
    mDebug.configInterrupt(nullptr);
    System::handleTrap(index, stackPointer);
    // wait for last byte to be written
    for (int i = 0; i < 10000; ++i);
    clockControl()->resetClock();
}

void F4System::init()
{
    clockControl()->setSystemClock(168000000);
    clockControl()->enable(ClockControl::Function::Usart2);
    clockControl()->enable(ClockControl::Function::GpioA);
    clockControl()->enable(ClockControl::Function::Dma1);

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
    gpio('A')->configAlternate(Gpio::Index::Pin2, Gpio::AltFunc::USART2);
    // USART2 RX
    gpio('A')->configAlternate(Gpio::Index::Pin3, Gpio::AltFunc::USART2, Gpio::Speed::Medium, Gpio::Pull::Down);

    mFlash.set(Flash::Feature::InstructionCache, true);
    mFlash.set(Flash::Feature::DataCache, true);
    // Be careful with prefetch, it does nasty things...
    //mFlash.set(Flash::Feature::Prefetch, true);
    mFpu.enable(FpuControl::AccessPrivileges::Full);

}

void F4System::consoleRead(char *msg, unsigned int len)
{
    mDebug.read(msg, len);
}

void F4System::consoleWrite(const char *msg, unsigned int len)
{
    mDebug.write(msg, len);
}

