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

static const System::BaseAddress GPIO_ARRAY[] = {
    F7System::BaseAddress::GPIOA,
    F7System::BaseAddress::GPIOB,
    F7System::BaseAddress::GPIOC,
    F7System::BaseAddress::GPIOD,
    F7System::BaseAddress::GPIOE,
    F7System::BaseAddress::GPIOF,
    F7System::BaseAddress::GPIOG,
    F7System::BaseAddress::GPIOH,
    F7System::BaseAddress::GPIOI,
    F7System::BaseAddress::GPIOJ,
    F7System::BaseAddress::GPIOK
};

F7System::F7System() :
    System(BaseAddress::SCB, GPIO_ARRAY, sizeof(GPIO_ARRAY) / sizeof(GPIO_ARRAY[0]), BaseAddress::RCC),
    mExtI(BaseAddress::EXTI, 23),
    mNvic(BaseAddress::NVIC, 82),
    mSysTick(BaseAddress::STK, clockControl()),
    mSysCfg(BaseAddress::SYSCFG),
    mDma1(BaseAddress::DMA1),
    mDma2(BaseAddress::DMA2),
    //    mUsart1(BaseAddress::USART1, clockControl(), ClockControl::Clock::APB2),
    //    mUsart2(BaseAddress::USART2, clockControl(), ClockControl::ClockSpeed::APB1, 512, 32),
    //    mUsart3(BaseAddress::USART3, clockControl(), ClockControl::Clock::APB1),
    //    mUart4(BaseAddress::UART4, clockControl(), ClockControl::Clock::APB1),
    //    mUart5(BaseAddress::UART5, clockControl(), ClockControl::Clock::APB1),
    mUsart6(BaseAddress::USART6, clockControl(), ClockControl::ClockSpeed::APB2, 512, 32),
    mDebug(mUsart6),
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
    clockControl()->resetClock();
}

void F7System::init()
{
    clockControl()->setSystemClock(216000000);
    clockControl()->enable(ClockControl::Function::Usart6);
    clockControl()->enable(ClockControl::Function::Dma2);

    //mDebug.config(115200);
    mDebug.config(921600);//, Serial::Parity::Odd, Serial::WordLength::Nine);
    mDebug.configStream(new Dma::Stream(mDma2, Dma::Stream::StreamIndex::Stream7, Dma::Stream::ChannelIndex::Channel5,
                                  new InterruptController::Line(mNvic, InterruptIndex::DMA2_Stream7)),
                  new Dma::Stream(mDma2, Dma::Stream::StreamIndex::Stream2, Dma::Stream::ChannelIndex::Channel5,
                                  new InterruptController::Line(mNvic, InterruptIndex::DMA2_Stream2)),
                  new InterruptController::Line(mNvic, InterruptIndex::USART6));
    mNvic.setPriotity(InterruptIndex::DMA2_Stream7, InterruptController::Priority::Lowest);
    mNvic.setPriotity(InterruptIndex::DMA2_Stream2, InterruptController::Priority::Low);
    mDebug.enable(Device::All);

    // USART2 TX
    gpio('C')->configAlternate(Gpio::Index::Pin6, Gpio::AltFunc::USART6);
    // USART2 RX
    gpio('C')->configAlternate(Gpio::Index::Pin7, Gpio::AltFunc::USART6, Gpio::Speed::Medium, Gpio::Pull::Down);

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

