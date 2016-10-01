// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	Elektronika MS 1201 series of single-board computers.  These serve as
	base for DVK series of desktops.

	1201.01	1801VM1 CPU, max 56 KB user memory
	1201.02	1801VM2 CPU, -//-
	1201.03	1801VM3 CPU, max 248 KB user memory, has MMU
	1201.04	1801VM3 CPU, max 1 MB user memory, has MMU, optional 1801VM4 FPU

	to verify:
	- pass 791401..04 tests cleanly with no visual artifacts ('k proho')

	to do:
	- install slot devices into bankdev0 map, not maincpu
	. implement DX interface
	- implement LP interface
	- make DX and LP optional (1201.02-01 and 1201.02-02 variants)

	- convert ksm and kcgd to devices for rs232 slot
	- remove extra screen from kgd
	- complete kcgd (mouse, 182 firmware etc.)
	- ksm: baud rate selection
	- what does counter register in kgd do?

	- DL11: baud rate selection
	- DL11: noise after RESET (seen in ZRXB test)
	- PC11: implement punch
	- qbus: DMA
	- various: alternate CSR/vectors

	- multiple units in vhd
	- any kind of TOY clock

	- implement MX device
	- implement MY device (uses DMA)
	- implement KTlK device (4-, 6-port serial mux)

	- implement RM device (CSR 167740, MS3404.04)
	- implement MT device
	- implement RK device
	- implement DY device

****************************************************************************/

#include "emu.h"

#include "bus/qbus/qbus.h"
#include "bus/qbus/mpi_cards.h"
#include "bus/rs232/rs232.h"
#include "cpu/t11/t11.h"
#include "machine/bankdev.h"
#include "machine/clock.h"
#include "machine/dl11.h"
#include "machine/ram.h"
#include "softlist.h"

#define VERBOSE_DBG 1       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-24s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)


class ms1201_state : public driver_device
{
public:
	ms1201_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_dl11(*this, "dl11"),
		m_rs232(*this, "rs232"),
		m_bankdev(*this, "bankdev0")
	{ }

	virtual void machine_start() override;
	virtual void machine_reset() override;

	enum
	{
		TIMER_ID_PCLK
	};
	TIMER_DEVICE_CALLBACK_MEMBER(pclk_timer);

	DECLARE_READ16_MEMBER(dip_r);
	DECLARE_WRITE16_MEMBER(odt_w);

private:
	int m_odt;

protected:
	required_device<cpu_device> m_maincpu;
	required_device<dl11_device> m_dl11;
	required_device<rs232_port_device> m_rs232;
	required_device<address_map_bank_device> m_bankdev;
};

static INPUT_PORTS_START( ms1201 )
	PORT_START("SA0")
	PORT_DIPNAME(0x01, 0x00, "ODT (HALT) mode")
	PORT_DIPSETTING(0x01, DEF_STR(Yes) )
	PORT_DIPSETTING(0x00, DEF_STR(No) )
	PORT_DIPNAME(0x02, 0x00, "Timer interrupt")
	PORT_DIPSETTING(0x02, DEF_STR(Yes) )
	PORT_DIPSETTING(0x00, DEF_STR(No) )

	PORT_START("SA1")
	PORT_DIPNAME(0x0f, 0x01, "Boot mode")
	PORT_DIPSETTING(0x00, "0 - start at vector 24" )
	PORT_DIPSETTING(0x01, "1 - start in ODT" )
	PORT_DIPSETTING(0x02, "2 - boot DX: or DW:,MY:,MX:" )
	PORT_DIPSETTING(0x03, "3 - start at 0140000" )
	// rom 279 only
	PORT_DIPSETTING(0x04, "4 - start in user ROM" )
	PORT_DIPSETTING(0x05, "5 - run tests once (T0)" )
	PORT_DIPSETTING(0x06, "6 - start at 0173000" )
	PORT_DIPSETTING(0x07, "7 - run tests continuously (T7)" )
INPUT_PORTS_END

static ADDRESS_MAP_START( ms1201_banked_map, AS_PROGRAM, 16, ms1201_state )
// USER mode
	AM_RANGE (0000000, 0157777) AM_RAM
//	AM_RANGE (0177170, 0177173) AM_NOP	// VP1-033 in RX11 floppy controller mode (device DX:)
	AM_RANGE (0177514, 0177517) AM_NOP	// VP1-033 in LP11 parallel port mode (device LP:)
	AM_RANGE (0177560, 0177567) AM_DEVREADWRITE("dl11", dl11_device, read, write)
// HALT mode
	AM_RANGE (0340000, 0357777) AM_ROM	AM_REGION("maincpu", 0)
	AM_RANGE (0370000, 0377777) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ms1201_mem, AS_PROGRAM, 16, ms1201_state )
	AM_RANGE (0000000, 0177777) AM_DEVREADWRITE("bankdev0", address_map_bank_device, read16, write16)
ADDRESS_MAP_END

READ16_MEMBER(ms1201_state::dip_r)
{
	return ioport("SA1")->read();
}

WRITE16_MEMBER(ms1201_state::odt_w)
{
	DBG_LOG(2,"ODT", ("%d -> %d -- %s\n", m_odt, data, data?"HALT":"USER"));

	if (m_odt != data) {
		m_bankdev->set_bank(data & 1);
		m_odt = data;
	}
}

TIMER_DEVICE_CALLBACK_MEMBER(ms1201_state::pclk_timer)
{
	if (BIT(ioport("SA0")->read(),0))
		m_maincpu->set_input_line(INPUT_LINE_1801HALT, ASSERT_LINE);
	else
		m_maincpu->set_input_line(INPUT_LINE_1801HALT, CLEAR_LINE);

	if (BIT(ioport("SA0")->read(),1))
		m_maincpu->set_input_line(INPUT_LINE_EVNT, ASSERT_LINE);
}

static const z80_daisy_config ms1201_daisy_chain[] =
{
	{ "dl11" },
	{ "qbus" },
	{ nullptr }
};

// machine

void ms1201_state::machine_start()
{
	logerror("MS1201: machine_start()\n");

	m_odt = 0;

	m_bankdev->space(AS_PROGRAM).set_trap_unmap(TRUE);
	m_bankdev->space(AS_PROGRAM).set_trap_line(INPUT_LINE_BUSERR);
}

void ms1201_state::machine_reset()
{
	logerror("MS1201: machine_reset()\n");
}

static MACHINE_CONFIG_START( ms1201, ms1201_state )
	MCFG_CPU_ADD("maincpu", K1801VM2, XTAL_8MHz/2)
	MCFG_Z80_DAISY_CHAIN(ms1201_daisy_chain)
	MCFG_CPU_PROGRAM_MAP(ms1201_mem)
	MCFG_T11_INITIAL_MODE(0140000)
	MCFG_T11_FEATURES(T11FEAT_BUSERR)
	MCFG_K1801VM2_BANKSWITCH_CB(WRITE16(ms1201_state, odt_w))
	MCFG_K1801VM2_DIPSWITCH_CB(READ16(ms1201_state, dip_r))

	MCFG_DEVICE_ADD("bankdev0", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(ms1201_banked_map)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_BIG)
	MCFG_ADDRESS_MAP_BANK_ADDRBUS_WIDTH(17)
	MCFG_ADDRESS_MAP_BANK_DATABUS_WIDTH(16)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0200000)

	MCFG_TIMER_DRIVER_ADD_PERIODIC("pclk", ms1201_state, pclk_timer, attotime::from_hz(50))

	MCFG_DEVICE_ADD("qbus", QBUS, 0)
	MCFG_QBUS_CPU(":maincpu")
	MCFG_QBUS_OUT_BIRQ4_CB(INPUTLINE("maincpu", INPUT_LINE_VIRQ))
	// slot 0 is taken by ms1201 itself
	MCFG_QBUS_SLOT_ADD("qbus", "qbus1", 0, mpi_cards, "pc11")
	MCFG_QBUS_SLOT_ADD("qbus", "qbus2", 0, mpi_cards, "vhd")
	MCFG_QBUS_SLOT_ADD("qbus", "qbus3", 0, mpi_cards, nullptr)
	MCFG_QBUS_SLOT_ADD("qbus", "qbus4", 0, mpi_cards, nullptr)

	// serial connection to host
	MCFG_DEVICE_ADD("dl11", DL11, XTAL_4_608MHz)
	MCFG_DL11_RXC(9600)
	MCFG_DL11_TXC(9600)
	MCFG_DL11_RXVEC(060)
	MCFG_DL11_TXVEC(064)
	MCFG_DL11_TXD_HANDLER(DEVWRITELINE("rs232", rs232_port_device, write_txd))
	MCFG_DL11_RTS_HANDLER(DEVWRITELINE("rs232", rs232_port_device, write_rts))
	MCFG_DL11_RXRDY_HANDLER(INPUTLINE("maincpu", INPUT_LINE_VIRQ))
	MCFG_DL11_TXRDY_HANDLER(INPUTLINE("maincpu", INPUT_LINE_VIRQ))

	MCFG_RS232_PORT_ADD("rs232", default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("dl11", dl11_device, rx_w))

//	MCFG_SOFTWARE_LIST_ADD("flop_list","ms1201_flop")
//	MCFG_SOFTWARE_LIST_ADD("ptap_list","ms1201_ptap")
MACHINE_CONFIG_END


ROM_START( ms1201 )
	ROM_REGION16_LE(020000, "maincpu", ROMREGION_ERASE00)
	ROM_DEFAULT_BIOS("055")
	ROM_SYSTEM_BIOS(0, "055", "mask 055 (198x)")
	ROMX_LOAD("055.dat", 0, 020000, CRC(11c28e48) SHA1(a328ca6de54630cd81659977fd2a0e7f0ad168fd), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "279", "mask 279 (1991)")
	ROMX_LOAD("279.dat", 0, 020000, CRC(26932af0) SHA1(e2d863085fc818ebcb08dcccbc5470106dd8eb33), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME      PARENT  COMPAT   MACHINE    INPUT    INIT                      COMPANY     FULLNAME       FLAGS */
COMP( 1985, ms1201,   0,      0,       ms1201,    ms1201,  driver_device,     0,     "USSR",     "MS 1201.02",  MACHINE_NO_SOUND_HW)

