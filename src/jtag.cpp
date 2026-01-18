/*
   https://github.com/gashtaan/sinowealth-8051-dumper

   Copyright (C) 2023, Michal Kovacik

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <util/delay.h>

#include "config.h"
#include "jtag.h"
#include "serial.h"

JTAG::JTAG()
{
	// Set all pins to Hi-Z/Input initially
	DDRD &= ~_BV(PIN_VREF);
	DDRD &= ~_BV(PIN_TDO);
	DDRD &= ~_BV(PIN_TDI);
	DDRD &= ~_BV(PIN_TMS);
	DDRD &= ~_BV(PIN_TCK);
}

void JTAG::connect()
{
	// Wait for Vref since we don't have reset pin - this is
	// an alternative that does not require power switch/relay.
	// TODO: Power cycle via high side switch/relay
	serialWrite("Waiting for Vref to get high - enable power to target manually:\r\n");
	while (!getBit(PIN_VREF)) {
		_delay_us(100);
	}
	serialWrite("Vref is now high - resuming\r\n");

	// Configure output pins after Vref check passes
	DDRD |= _BV(PIN_TDI);
	DDRD |= _BV(PIN_TMS);
	DDRD |= _BV(PIN_TCK);

	// Do not power the target via I/O leakage
	clrBit(PIN_TCK);
	clrBit(PIN_TDI);
	clrBit(PIN_TMS);

	setBit(PIN_TCK);
	setBit(PIN_TDI);
	setBit(PIN_TMS);

	_delay_us(500);

	clrBit(PIN_TCK);
	_delay_us(1);
	setBit(PIN_TCK);
	_delay_us(50);

	for (uint8_t n = 0; n < 165; ++n)
	{
		clrBit(PIN_TMS);
		_delay_us(2);
		setBit(PIN_TMS);
		_delay_us(2);
	}

	for (uint8_t n = 0; n < 105; ++n)
	{
		clrBit(PIN_TDI);
		_delay_us(2);
		setBit(PIN_TDI);
		_delay_us(2);
	}

	for (uint8_t n = 0; n < 90; ++n)
	{
		clrBit(PIN_TCK);
		_delay_us(2);
		setBit(PIN_TCK);
		_delay_us(2);
	}

	for (uint16_t n = 0; n < 25600; ++n)
	{
		clrBit(PIN_TMS);
		_delay_us(2);
		setBit(PIN_TMS);
		_delay_us(2);
	}

	_delay_us(8);

	clrBit(PIN_TMS);

	m_mode = Mode::ICP;
	startMode();

	for (uint16_t n = 0; n < 25600; ++n)
	{
		setBit(PIN_TCK);
		_delay_us(2);
		clrBit(PIN_TCK);
		_delay_us(2);
	}

	reset();
}

void JTAG::disconnect()
{
	// for debugging purposes it's convenient to leave connection in ICP mode as it will survive host reset/upload
	// (PIN_TCK must be held high in READY state, if it's set low during host reset/upload, target will disconnect)
	switchMode(Mode::ICP);
}

void JTAG::reset()
{
	if (m_mode == Mode::ERROR)
		return;

	if (m_mode == Mode::JTAG)
	{
		// reset JTAG state
		for (uint8_t n = 0; n < 35; ++n)
			nextState(1);

		setBit(PIN_TCK);

		clrBit(PIN_TMS);
	}
	else
	{
		setBit(PIN_TCK);

		setBit(PIN_TMS);
		_delay_us(2);
		clrBit(PIN_TMS);
		_delay_us(2);
	}

	m_mode = Mode::READY;
}

void JTAG::switchMode(Mode mode)
{
	if (m_mode == mode)
		return;

	if (m_mode != Mode::READY)
		reset();

	m_mode = mode;
	startMode();

	if (m_mode == Mode::ICP)
	{
		_delay_us(800);

		pingICP();
	}
	else if (m_mode == Mode::JTAG)
	{
		// reset JTAG state
		for (uint8_t n = 0; n < 8; ++n)
			nextState(1);

		sendInstruction(2);
		sendData<4>(4);

		sendInstruction(3);
		sendData<23>(0x403000);
		_delay_us(50);
		sendData<23>(0x402000);
		sendData<23>(0x400000);

		// most likely breakpoints initialization
		// SH68F881W works without it, but maybe for other chips it's mandatory
		{
			sendData<23>(0x630000);
			sendData<23>(0x670000);
			sendData<23>(0x6B0000);
			sendData<23>(0x6F0000);
			sendData<23>(0x730000);
			sendData<23>(0x770000);
			sendData<23>(0x7B0000);
			sendData<23>(0x7F0000);
		}

		sendInstruction(2);
		sendData<4>(1);

		sendInstruction(12);
	}
}

void JTAG::startMode() const
{
	clrBit(PIN_TCK);
	_delay_us(2);

	for (uint8_t m = 0x80; m; m >>= 1)
	{
		if (uint8_t(m_mode) & m)
			setBit(PIN_TDI);
		else
			clrBit(PIN_TDI);

		setBit(PIN_TCK);
		_delay_us(2);
		clrBit(PIN_TCK);
		_delay_us(2);
	}

	setBit(PIN_TCK);
	_delay_us(2);
	clrBit(PIN_TCK);
	_delay_us(2);

	setBit(PIN_TCK);
	_delay_us(2);
	clrBit(PIN_TCK);
	_delay_us(2);
}

bool JTAG::checkJTAG()
{
	uint16_t id = getID();
	return (id != 0x0000 && id != 0xFFFF);
}

bool JTAG::checkICP()
{
	switchMode(Mode::ICP);

	sendICPData(ICP_SET_IB_OFFSET_L);
	sendICPData(0x69);
	sendICPData(ICP_SET_IB_OFFSET_H);
	sendICPData(0xFF);

	sendICPData(ICP_GET_IB_OFFSET);
	auto b = receiveICPData();
	(void)receiveICPData();

	return (b == 0x69);
}

void JTAG::pingICP() const
{
	if (m_mode != Mode::ICP)
		return;

	sendICPData(ICP_PING);
	sendICPData(0xFF);
}

uint16_t JTAG::getID()
{
	switchMode(Mode::JTAG);

	sendInstruction(JTAG_IDCODE);
	return receiveData<16, uint16_t>();
}

bool JTAG::readFlashICP(uint8_t* buffer, uint8_t bufferSize, uint32_t address, bool customBlock)
{
	switchMode(Mode::ICP);

#if CHIP_TYPE != 1
	sendICPData(0x46);
	sendICPData(0xFE);
	sendICPData(0xFF);
#endif

	sendICPData(ICP_SET_IB_OFFSET_L);
	sendICPData(address & 0x000000FF);
	sendICPData(ICP_SET_IB_OFFSET_H);
	sendICPData((address & 0x0000FF00) >> 8);
#if CHIP_TYPE == 4 || CHIP_TYPE == 7
	sendICPData(ICP_SET_XPAGE);
	sendICPData((address & 0x00FF0000) >> 16);
#endif

	sendICPData(customBlock ? ICP_READ_CUSTOM_BLOCK : ICP_READ_FLASH);

	for (uint8_t n = 0; n < bufferSize; ++n)
		buffer[n] = receiveICPData();

	reset();

	return true;
}

bool JTAG::readFlashJTAG(uint8_t* buffer, uint8_t bufferSize, uint32_t address, bool customBlock)
{
	if (customBlock)
		return false;

	switchMode(Mode::JTAG);

#if CHIP_FLASH_SIZE > 65536
	uint8_t bank = address >> 15;
	if (bank > 0)
	{
		// banks 1-N are mapped to upper half of address space
		address &= 0x00007FFF;
		address |= 0x00008000;
	}

	// MOV PBANKLO, 0x55
	sendData<8>(reverseBits(0x75));
	sendData<8>(reverseBits(0xB7));
	sendData<8>(reverseBits(0x55));

	// MOV PBANK, bank
	sendData<8>(reverseBits(0x75));
	sendData<8>(reverseBits(0xB6));
	sendData<8>(reverseBits(bank));

	// NOPs
	sendData<8>(reverseBits(0x00));
	sendData<8>(reverseBits(0x00));
	sendData<8>(reverseBits(0x00));
	sendData<8>(reverseBits(0x00));
#endif

	sendInstruction(0);

	for (uint8_t n = 0; n < bufferSize + 1; ++n, ++address)
	{
		nextState(1); // Select-DR
		nextState(0); // Capture-DR
		nextState(0); // Shift-DR

		// send and receive data in single data shift
		for (uint16_t m = 0x8000; m; m >>= 1)
			nextState(0, address & m);

		// meaning of this sequence is unknown to me
		nextState(0, 0);
		nextState(0, 0);
		nextState(0, 0);
		nextState(0, 1);
		nextState(0, 0);
		nextState(0, 0);

		uint8_t data = 0;
		for (uint8_t n = 0; n < 7; ++n, data <<= 1)
			data |= nextState(0, 0);
		data |= nextState(1);

		nextState(1); // Update-DR
		nextState(0); // Idle
		nextState(0); // Idle? Needed, don't know why

		if (n > 0)
			// first data is garbage, next data is a byte read from previously shifted address
			buffer[n - 1] = data;
	}

	sendInstruction(12);

	return true;
}

void JTAG::sendICPData(uint8_t value)
{
	for (uint8_t m = 0x80; m; m >>= 1)
	{
		if (value & m)
			setBit(PIN_TDI);
		else
			clrBit(PIN_TDI);

		pulseClock();
	}

	pulseClock();

	clrBit(PIN_TDI);
}

uint8_t JTAG::receiveICPData()
{
	uint8_t value = 0;
	for (uint8_t m = 1; m; m <<= 1)
	{
		pulseClock();

		if (getBit(PIN_TDO))
			value |= m;
	}

	pulseClock();

	return value;
}

bool JTAG::nextState(bool tms)
{
	if (tms)
		setBit(PIN_TMS);
	else
		clrBit(PIN_TMS);

	setBit(PIN_TCK);
	_delay_us(2);

	bool b = getBit(PIN_TDO);

	clrBit(PIN_TCK);
	_delay_us(2);

	return b;
}

bool JTAG::nextState(bool tms, bool out)
{
	if (out)
		setBit(PIN_TDI);
	else
		clrBit(PIN_TDI);

	return nextState(tms);
}

void JTAG::pulseClock()
{
	_delay_us(1);
	setBit(PIN_TCK);
	_delay_us(1);
	clrBit(PIN_TCK);
}

void JTAG::pulseClocks(uint8_t count)
{
	while (count-- > 0)
		pulseClock();
}
