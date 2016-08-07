// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	DEC DL11-type SLU (serial line unit)

	http://www.ibiblio.org/pub/academic/computer-science/history/pdp-11/hardware/micronotes/numerical/micronote33.txt

	Actually, a K1801VP1-065 (a follow-on to K1801VP1-035)

	RCSR bits:
	- 15 (R) 'parity error' (cleared on read of RBUF)
	- 12 (R) 'overrun' (cleared on read of RBUF)
	- 07 (R) 'receive done' (cleared on read of RBUF)
	- 06 (RW) 'interrupt enabled'
	- 00 (?) 'no stop bit detected' (065 only)

	TCSR bits:
	- 07 (R) 'transmit done' (cleared on read of RBUF)
	- 06 (RW) 'interrupt enabled'
	- 02 (RW) 'local loopback'
	- 00 (RW) 'send break'

	RBUF has no status bits in 15..08.

	Baud rate and character format are set via input leads, not
	programmatically.  2 stop bits are always sent and expected.

	NB1 NB0 = 0x: 5 bits
	NB1 NB0 = 10: 7 bits
	NB1 NB0 = 11: 8 bits

	NP PEV = 1x: no parity check
	NP PEV = 01: even
	NP PEV = 00: odd

	FR3..0 = 0000: 50 baud
	FR3..0 = 0001: 75
	FR3..0 = 0010: 100
	FR3..0 = 0011: 150
	FR3..0 = 0100: 200
	FR3..0 = 0101: 300
	FR3..0 = 0110: 600
	FR3..0 = 0111: 1200
	FR3..0 = 1000: 2400
	FR3..0 = 1001: 4800
	FR3..0 = 1010: 9600
	FR3..0 = 1011: 19200
	FR3..0 = 1100: 57600


***************************************************************************/

#include "dl11.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type DL11 = &device_creator<dl11_device>;



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VERBOSE_DBG 0       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-24s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)


enum
{
	REGISTER_RCSR = 0,
	REGISTER_RBUF,
	REGISTER_TCSR,
	REGISTER_TBUF
};

const char* dl11_regnames[] = {
	"RCSR", "RBUF", "TCSR", "TBUF"
};

/* Input ports */
static INPUT_PORTS_START( dl11_ports )
	PORT_START("SA1")
	PORT_DIPNAME(0x03, 0x03, "Character size")
	PORT_DIPSETTING(0x00, "5 bits")
	PORT_DIPSETTING(0x02, "7 bits")
	PORT_DIPSETTING(0x03, "8 bits")
	PORT_DIPNAME(0x0C, 0x08, "Parity check")
	PORT_DIPSETTING(0x08, "Off")
	PORT_DIPSETTING(0x04, "Even")
	PORT_DIPSETTING(0x00, "Odd")
	PORT_DIPNAME(0xF0, 0xA0, "Baud rate")
	PORT_DIPSETTING(0xC0, "57600")
	PORT_DIPSETTING(0xB0, "19200")
	PORT_DIPSETTING(0xA0, "9600")
	PORT_DIPSETTING(0x90, "4800")
	PORT_DIPSETTING(0x80, "2400")
	PORT_DIPSETTING(0x70, "1200")
	PORT_DIPSETTING(0x60, "600")
	PORT_DIPSETTING(0x50, "300")
	PORT_DIPSETTING(0x40, "200")
	PORT_DIPSETTING(0x30, "150")
	PORT_DIPSETTING(0x10, "75")
	PORT_DIPSETTING(0x00, "50")
INPUT_PORTS_END

ioport_constructor dl11_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( dl11_ports );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  dl11_device - constructor
//-------------------------------------------------

dl11_device::dl11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, DL11, "DL11", tag, owner, clock, "dl11", __FILE__),
	device_serial_interface(mconfig, *this),
	device_z80daisy_interface(mconfig, *this),
	m_write_txd(*this),
	m_write_rts(*this),
	m_write_rxrdy(*this),
	m_write_txrdy(*this),
	m_rxc(0),
	m_txc(0),
	m_rxvec(0),
	m_txvec(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dl11_device::device_start()
{
	// resolve callbacks
	m_write_txd.resolve_safe();
	m_write_rts.resolve_safe();
	m_write_rxrdy.resolve_safe();
	m_write_txrdy.resolve_safe();

	// save state
	save_item(NAME(m_rcsr));
	save_item(NAME(m_rbuf));
	save_item(NAME(m_tcsr));
	save_item(NAME(m_tbuf));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void dl11_device::device_reset()
{
	int dip;

	DBG_LOG(1,"DL11 Reset", ("rx %d tx %d rxvec %03o txvec %03o\n", m_rxc, m_txc, m_rxvec, m_txvec));

    dip = ioport("SA1")->read();

	set_data_frame(1 /* start bits */,
		(dip & 0x3) ? (((dip & 3) == 3) ? 8 : 7) : 5,
		(dip & 0xc) ? (((dip & 0xc) == 8) ? PARITY_NONE : PARITY_EVEN) : PARITY_ODD,
		STOP_BITS_2);

	// create the timers
	if (m_rxc > 0)
		set_rcv_rate(m_rxc);

	if (m_txc > 0)
		set_tra_rate(m_txc);

	m_rcsr = m_rbuf = m_tbuf = 0;
	m_tcsr = CSR_DONE;
	m_rxrdy = m_txrdy = CLEAR_LINE;

	m_write_txd(1);
	m_write_rts(0);
	m_write_rxrdy(m_rxrdy);
	m_write_txrdy(m_txrdy);
}

int dl11_device::z80daisy_irq_state()
{
	if (m_rxrdy == ASSERT_LINE || m_txrdy == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int dl11_device::z80daisy_irq_ack()
{
	int vec = -1;

	if (m_rxrdy == ASSERT_LINE) {
		m_rxrdy = CLEAR_LINE;
		vec = m_rxvec;
	} else if (m_txrdy == ASSERT_LINE) {
		m_txrdy = CLEAR_LINE;
		vec = m_txvec;
	}

	if (vec > 0)
		DBG_LOG(2,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

void dl11_device::z80daisy_irq_reti()
{
}


//-------------------------------------------------
//  tra_callback -
//-------------------------------------------------

void dl11_device::tra_callback()
{
	m_write_txd(transmit_register_get_data_bit());
}


//-------------------------------------------------
//  tra_complete -
//-------------------------------------------------

void dl11_device::tra_complete()
{
	m_tcsr |= CSR_DONE;
	raise_virq(m_write_txrdy, m_tcsr, CSR_IE, m_txrdy);
}


//-------------------------------------------------
//  rcv_complete -
//-------------------------------------------------

void dl11_device::rcv_complete()
{
	receive_register_extract();
	m_rbuf = get_received_char();
	DBG_LOG(2,"DL11 Rx", ("rbuf %06o '%c' rcsr %06o\n", m_rbuf, ((m_rbuf&127) < 32 ? 32 : (m_rbuf&127)), m_rcsr));
	m_rcsr |= CSR_DONE;
	raise_virq(m_write_rxrdy, m_rcsr, CSR_IE, m_rxrdy);
	m_write_rts(1);
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ16_MEMBER( dl11_device::read )
{
	UINT16 data = 0;

	switch (offset & 0x03)
	{
	case REGISTER_RCSR:
		data = m_rcsr & DLICSR_RD;
		break;

	case REGISTER_RBUF:
		data = m_rbuf;
		m_rcsr &= ~CSR_DONE;
		clear_virq(m_write_rxrdy, m_rcsr, CSR_IE, m_rxrdy);
		m_write_rts(0);
		break;

	case REGISTER_TCSR:
		data = m_tcsr & DLOCSR_RD;
		break;

	case REGISTER_TBUF:
		data = m_tbuf;
		break;
	}

	DBG_LOG(2,"DL11 R", ("%d == %06o\n", offset, data));

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE16_MEMBER( dl11_device::write )
{
	DBG_LOG(2,"DL11 W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	switch (offset & 0x03)
	{
	case REGISTER_RCSR:
        if ((data & CSR_IE) == 0) {
			clear_virq(m_write_rxrdy, 1, 1, m_rxrdy);
		} else if ((m_rcsr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			raise_virq(m_write_rxrdy, 1, 1, m_rxrdy);
		}
		m_rcsr = ((m_rcsr & ~DLICSR_WR) | (data & DLICSR_WR));
		break;

	case REGISTER_RBUF:
		break;

	case REGISTER_TCSR:
        if ((data & CSR_IE) == 0) {
			clear_virq(m_write_txrdy, 1, 1, m_txrdy);
        } else if ((m_tcsr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			raise_virq(m_write_txrdy, 1, 1, m_txrdy);
		}
		m_tcsr = ((m_tcsr & ~DLOCSR_WR) | (data & DLOCSR_WR));
		break;

	case REGISTER_TBUF:
		m_tbuf = data;
		m_tcsr &= ~CSR_DONE;
		DBG_LOG(2,"DL11 Tx", ("tbuf %06o '%c' tcsr %06o\n", m_tbuf, ((m_tbuf&127) < 32 ? 32 : (m_tbuf&127)), m_tcsr));
		clear_virq(m_write_txrdy, m_tcsr, CSR_IE, m_txrdy);
		transmit_register_setup(data & 0xff);
		break;
	}
}


//-------------------------------------------------
//  rxrdy_r - receiver ready
//-------------------------------------------------

READ_LINE_MEMBER( dl11_device::rxrdy_r )
{
	return ((m_rcsr & (CSR_DONE|CSR_IE)) == (CSR_DONE|CSR_IE)) ? ASSERT_LINE : CLEAR_LINE;
}


//-------------------------------------------------
//  txrdy_r - transmitter empty
//-------------------------------------------------

READ_LINE_MEMBER( dl11_device::txrdy_r )
{
	return ((m_tcsr & (CSR_DONE|CSR_IE)) == (CSR_DONE|CSR_IE)) ? ASSERT_LINE : CLEAR_LINE;
}


void dl11_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	device_serial_interface::device_timer(timer, id, param, ptr);
}
