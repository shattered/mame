// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

    Hewlett-Packard 95LX palmtop ("Jaguar")

	NEC V20 CPU @ 5.37 MHz
	HP F1000-80054 "Hopper" ASIC
		display controller
		keyboard controler
		UART
		interrupt controller
		interval timer
		real time clock
		A/D converter (4 channels)
		D/A converter (tone generator)
		PCMCIA controller
    512KB or 1MB RAM
    1MB BIOS ROM
	MDA-compatible LCD display, 16x40 chars, 128x240 pixels

    To do:
    - lots

    Useful links:
    - https://hermocom.com/hplx/view-all-hp-palmtop-articles/41-95lx
	- http://web.archive.org/web/20071012040320/http://www.daniel-hertrich.de/download/95lx_devguide.zip

	To ignore broken rtc:

	bp FD276,1,{do aw=w@(ds0*10+c4);go}

***************************************************************************/


#include "emu.h"

#include "machine/mc146818.h"
#include "machine/am9517a.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "bus/isa/isa.h"
#include "bus/isa/isa_cards.h"
#include "bus/pc_kbd/keyboards.h"
#include "cpu/nec/nec.h"
#include "machine/bankdev.h"
#include "machine/ram.h"
#include "softlist.h"


#define VERBOSE (LOG_GENERAL)
//#define LOG_OUTPUT_FUNC printf
#include "logmacro.h"


class hp95lx_state : public driver_device
{
public:
	hp95lx_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_bankdev_c000(*this, "bankdev_c000")
		, m_bankdev_d000(*this, "bankdev_d000")
		, m_bankdev_e000(*this, "bankdev_e000")
		, m_bankdev_e400(*this, "bankdev_e400")
		, m_bankdev_e800(*this, "bankdev_e800")
		, m_bankdev_ec00(*this, "bankdev_ec00")
		, m_ram(*this, RAM_TAG)
		, m_pic8259(*this, "pic8259")
		, m_pit8253(*this, "pit8253")
		, m_dma8237(*this, "dma8237")
		, m_rtc(*this, "rtc")
		{ }

	DECLARE_MACHINE_RESET(hp95lx);
	void init_hp95lx();

	DECLARE_WRITE8_MEMBER(romdos_bank_w);
	DECLARE_READ8_MEMBER(romdos_bank_r);

	DECLARE_WRITE8_MEMBER(bram_w);
	DECLARE_READ8_MEMBER(bram_r);

	DECLARE_WRITE8_MEMBER(d300_w);
	DECLARE_READ8_MEMBER(d300_r);
	DECLARE_WRITE8_MEMBER(e300_w);
	DECLARE_READ8_MEMBER(e300_r);
	DECLARE_WRITE8_MEMBER(f300_w);
	DECLARE_READ8_MEMBER(f300_r);

	void hp95lx(machine_config &config);
	void hp95lx_io(address_map &map);
	void hp95lx_map(address_map &map);
	void hp95lx_romdos(address_map &map);

protected:
	required_device<cpu_device> m_maincpu;
	required_device<address_map_bank_device> m_bankdev_c000;
	required_device<address_map_bank_device> m_bankdev_d000;
	required_device<address_map_bank_device> m_bankdev_e000;
	required_device<address_map_bank_device> m_bankdev_e400;
	required_device<address_map_bank_device> m_bankdev_e800;
	required_device<address_map_bank_device> m_bankdev_ec00;
	required_device<ram_device> m_ram;
	required_device<pic8259_device> m_pic8259;
	required_device<pit8253_device> m_pit8253;
	required_device<am9517a_device> m_dma8237;
	required_device<mc146818_device> m_rtc;

private:
	u8 m_mapper[32];
};


void hp95lx_state::init_hp95lx()
{
	if(!m_ram->started())
		throw device_missing_dependencies();
	m_maincpu->space(AS_PROGRAM).install_ram(0, m_ram->size() - 1, m_ram->pointer());
}

MACHINE_RESET_MEMBER(hp95lx_state, hp95lx)
{
}


WRITE8_MEMBER(hp95lx_state::d300_w)
{
	LOG("IO %04x <- %02x\n", 0xd300 + offset, data);

	switch (offset)
	{
	case 6:
	case 7:
//		m_rtc->write(space, offset - 6, data);
		break;
	}
}

READ8_MEMBER(hp95lx_state::d300_r)
{
	uint8_t data = 0;

	switch (offset)
	{
	case 0: // b0 = warm start flag?
//		data = 0x1;
		break;

	case 5:
		data = 0x2;
		break;

	case 6:
	case 7:
//		data = m_rtc->read(space, offset - 6);
		break;
	}

	LOG("IO %04x == %02x\n", 0xd300 + offset, data);

	return data;
}

WRITE8_MEMBER(hp95lx_state::e300_w)
{
	LOG("IO %04x <- %02x\n", 0xe300 + offset, data);

	switch (offset)
	{
	case 1: // RS-232 power
		break;

	case 13: // reset?
		break;

	case 14: // keyboard output?
	case 15:
		break;
	}
}

READ8_MEMBER(hp95lx_state::e300_r)
{
	uint8_t data = 0;

	switch (offset)
	{
	case 4: // Card Detect Register
		break;

	case 7:
		data = 0x20;
		break;

	case 9:
	case 14: // keyboard status?
	case 15:
		return data;
		break;
	}

	LOG("IO %04x == %02x\n", 0xe300 + offset, data);

	return data;
}

WRITE8_MEMBER(hp95lx_state::f300_w)
{
	LOG("IO %04x <- %02x\n", 0xf300 + offset, data);

	if (offset >= sizeof(m_mapper)) return;

	m_mapper[offset] = data;

	switch (offset)
	{
	case 0x11:	// E0000 16K
		if (data == 0)
		{
			LOG("MAPPER E0000 <- %05X\n", (m_mapper[offset - 1] & 127) * 0x2000);
			m_bankdev_e000->set_bank((m_mapper[offset - 1] >> 1) & 63);
		}
		break;

	case 0x13:	// E4000 16K
		if (data == 0)
		{
			LOG("MAPPER E4000 <- %05X\n", (m_mapper[offset - 1] & 127) * 0x2000);
			m_bankdev_e400->set_bank((m_mapper[offset - 1] >> 1) & 63);
		}
		break;

	case 0x15:	// E8000 16K
		if (data == 0)
		{
			LOG("MAPPER E8000 <- %05X\n", (m_mapper[offset - 1] & 127) * 0x2000);
			m_bankdev_e800->set_bank((m_mapper[offset - 1] >> 1) & 63);
		}
		break;

	case 0x17:	// EC000 16K
		if (data == 0)
		{
			LOG("MAPPER EC000 <- %05X\n", (m_mapper[offset - 1] & 127) * 0x2000);
			m_bankdev_ec00->set_bank((m_mapper[offset - 1] >> 1) & 63);
		}
		break;

	case 0x18:	// C0000 64K
		if ((data & 0x87) == 0x80)
		{
			LOG("MAPPER C0000 <- %X0000\n", (data >> 3) & 15);
			m_bankdev_c000->set_bank((data >> 3) & 15);
		}
		break;

	case 0x19:	// D0000 64K
		if ((data & 0x87) == 0x80)
		{
			LOG("MAPPER D0000 <- %X0000\n", (data >> 3) & 15);
			m_bankdev_d000->set_bank((data >> 3) & 15);
		}
		break;
	}
}

READ8_MEMBER(hp95lx_state::f300_r)
{
	uint8_t data = 0;

	switch (offset)
	{
	}

	LOG("IO %04x == %02x\n", 0xf300 + offset, data);

	return data;
}


void hp95lx_state::hp95lx_romdos(address_map &map)
{
	map(0, 0xfffff).rom().region("romdos", 0);
}

void hp95lx_state::hp95lx_map(address_map &map)
{
	map.unmap_value_high();
	map(0xa0000, 0xaffff).rom().region("romdos", 0xe0000); // OS functions (DOS, COMMAND.COM)
	map(0xc0000, 0xcffff).rw(m_bankdev_c000, FUNC(address_map_bank_device::read8), FUNC(address_map_bank_device::write8));
	map(0xd0000, 0xdffff).rw(m_bankdev_d000, FUNC(address_map_bank_device::read8), FUNC(address_map_bank_device::write8));
	map(0xe0000, 0xe3fff).rw(m_bankdev_e000, FUNC(address_map_bank_device::read8), FUNC(address_map_bank_device::write8));
	map(0xe4000, 0xe7fff).rw(m_bankdev_e400, FUNC(address_map_bank_device::read8), FUNC(address_map_bank_device::write8));
	map(0xe8000, 0xebfff).rw(m_bankdev_e800, FUNC(address_map_bank_device::read8), FUNC(address_map_bank_device::write8));
	map(0xec000, 0xeffff).rw(m_bankdev_ec00, FUNC(address_map_bank_device::read8), FUNC(address_map_bank_device::write8));
	map(0xf0000, 0xfffff).rom().region("romdos", 0xf0000); // BIOS ROM and SysMgr
}

void hp95lx_state::hp95lx_io(address_map &map)
{
	map.unmap_value_low();
	map(0x0000, 0x000f).rw("dma8237", FUNC(am9517a_device::read), FUNC(am9517a_device::write));
	map(0x0020, 0x002f).rw("pic8259", FUNC(pic8259_device::read), FUNC(pic8259_device::write));
	map(0x0040, 0x004f).rw("pit8253", FUNC(pit8253_device::read), FUNC(pit8253_device::write));
	map(0x0070, 0x007f).rw("rtc", FUNC(mc146818_device::read), FUNC(mc146818_device::write));
	map(0xd300, 0xd30f).rw(FUNC(hp95lx_state::d300_r), FUNC(hp95lx_state::d300_w));
	map(0xe300, 0xe30f).rw(FUNC(hp95lx_state::e300_r), FUNC(hp95lx_state::e300_w));
	map(0xf300, 0xf31f).rw(FUNC(hp95lx_state::f300_r), FUNC(hp95lx_state::f300_w));
}


MACHINE_CONFIG_START(hp95lx_state::hp95lx)
	MCFG_DEVICE_ADD("maincpu", V20, XTAL(5'370'000))
	MCFG_DEVICE_PROGRAM_MAP(hp95lx_map)
	MCFG_DEVICE_IO_MAP(hp95lx_io)
	MCFG_DEVICE_IRQ_ACKNOWLEDGE_DEVICE("pic8259", pic8259_device, inta_cb)

	MCFG_DEVICE_ADD("bankdev_c000", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(hp95lx_romdos)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_LITTLE)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(20)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x10000)

	MCFG_DEVICE_ADD("bankdev_d000", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(hp95lx_romdos)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_LITTLE)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(20)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x10000)

	MCFG_DEVICE_ADD("bankdev_e000", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(hp95lx_romdos)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_LITTLE)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(20)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x4000)

	MCFG_DEVICE_ADD("bankdev_e400", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(hp95lx_romdos)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_LITTLE)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(20)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x4000)

	MCFG_DEVICE_ADD("bankdev_e800", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(hp95lx_romdos)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_LITTLE)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(20)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x4000)

	MCFG_DEVICE_ADD("bankdev_ec00", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(hp95lx_romdos)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_LITTLE)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(20)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x4000)

	MCFG_DEVICE_ADD("pit8253", PIT8253, 0)
//	MCFG_PIT8253_CLK0(XTAL(14'318'181)/12.0) /* heartbeat IRQ */
//	MCFG_PIT8253_OUT0_HANDLER(WRITELINE("pic8259", pic8259_device, ir0_w))
//	MCFG_PIT8253_CLK0(XTAL(14'318'181)/12.0) /* misc IRQ */
//	MCFG_PIT8253_OUT0_HANDLER(WRITELINE("pic8259", pic8259_device, ir2_w))

	MCFG_DEVICE_ADD("dma8237", AM9517A, XTAL(14'318'181)/3.0)

	MCFG_DEVICE_ADD("pic8259", PIC8259, 0)
	MCFG_PIC8259_OUT_INT_CB(INPUTLINE("maincpu", 0))

	MCFG_DEVICE_ADD("rtc", MC146818, 32.768_kHz_XTAL)
	MCFG_MC146818_IRQ_HANDLER(WRITELINE("pic8259", pic8259_device, ir7_w))
	MCFG_MC146818_CENTURY_INDEX(0x32)

	MCFG_MACHINE_RESET_OVERRIDE(hp95lx_state, hp95lx)

	MCFG_DEVICE_ADD("isa", ISA8, 0)
	MCFG_ISA8_CPU(":maincpu")

	MCFG_DEVICE_ADD("board0", ISA8_SLOT, 0, "isa", pc_isa8_cards, "mda", true)

	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("512K")
MACHINE_CONFIG_END


ROM_START( hp95lx )
	ROM_REGION16_LE(0x100000, "romdos", 0)
	ROM_LOAD("firmware.bin", 0, 0x100000, CRC(18121c48) SHA1(c3bfb45cbbf4f57ae67fb5659da40f371d8e5c54))

#if 0
	ROM_FILL(0x22166,1,'t')
	ROM_FILL(0x22167,1,'f')
	ROM_FILL(0x22168,1,' ')
	ROM_FILL(0x22169,1,0xd)
	ROM_FILL(0x2216a,1,0xa)
#endif

#if 1
	ROM_FILL(0x22166,1,'d')
	ROM_FILL(0x22167,1,'i')
	ROM_FILL(0x22168,1,'r')
	ROM_FILL(0x22169,1,0xd)
	ROM_FILL(0x2216a,1,0xa)
#endif
ROM_END


//    YEAR  NAME      PARENT   COMPAT  MACHINE   INPUT  CLASS           INIT           COMPANY            FULLNAME   FLAGS
COMP( 1991, hp95lx,   0,       0,      hp95lx,   0,     hp95lx_state,   init_hp95lx,   "Hewlett-Packard", "HP 95LX",   MACHINE_IS_SKELETON )
