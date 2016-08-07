// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	DEC PC11 paper tape reader and punch controller (punch not implemented)

***************************************************************************/

#include "pc11.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type PC11 = &device_creator<pc11_device>;



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

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


enum
{
	REGISTER_RCSR = 0,
	REGISTER_RBUF,
	REGISTER_TCSR,
	REGISTER_TBUF
};

const char* pc11_regnames[] = {
	"RCSR", "RBUF", "TCSR", "TBUF"
};

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  pc11_device - constructor
//-------------------------------------------------

pc11_device::pc11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, PC11, "PC11", tag, owner, clock, "pc11", __FILE__),
	device_image_interface(mconfig, *this),
	device_z80daisy_interface(mconfig, *this),
	m_write_rxrdy(*this),
	m_write_txrdy(*this),
	m_rxvec(0),
	m_txvec(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void pc11_device::device_start()
{
	// resolve callbacks
	m_write_rxrdy.resolve_safe();
	m_write_txrdy.resolve_safe();

	// save state
	save_item(NAME(m_rcsr));
	save_item(NAME(m_rbuf));
	save_item(NAME(m_tcsr));
	save_item(NAME(m_tbuf));

	// about 300 cps
	emu_timer *timer = timer_alloc();
	timer->adjust(attotime::from_usec(333), 0, attotime::from_usec(333));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void pc11_device::device_reset()
{
	DBG_LOG(1,"PC11 Reset", ("rxvec %03o txvec %03o\n", m_rxvec, m_txvec));

	m_rcsr = m_rbuf = m_tbuf = 0;
	m_tcsr = CSR_DONE;
	m_rxrdy = m_txrdy = CLEAR_LINE;

	m_write_rxrdy(m_rxrdy);
	m_write_txrdy(m_txrdy);
}

int pc11_device::z80daisy_irq_state()
{
	if (m_rxrdy == ASSERT_LINE || m_txrdy == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int pc11_device::z80daisy_irq_ack()
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
		DBG_LOG(1,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

void pc11_device::z80daisy_irq_reti()
{
}


image_init_result pc11_device::call_load()
{
	/* reader unit */
	m_fd = this;
	m_rcsr &= ~(CSR_ERR|CSR_BUSY);

	DBG_LOG(1,"call_load", ("filename %s is_open %d\n", m_fd->filename(), m_fd->is_open()));

	return image_init_result::PASS;
}

void pc11_device::call_unload()
{
	/* reader unit */
	DBG_LOG(1,"call_unload", ("filename %s is_open %d\n", m_fd->filename(), m_fd->is_open()));
	m_fd = nullptr;
	m_rcsr |= CSR_ERR;
}



//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ16_MEMBER( pc11_device::read )
{
	UINT16 data = 0;

	switch (offset & 0x03)
	{
	case REGISTER_RCSR:
		data = m_rcsr & PTRCSR_IMP;
		break;

	case REGISTER_RBUF:
		data = m_rbuf & 0377;
		m_rcsr &= ~CSR_DONE;
		clear_virq(m_write_rxrdy, m_rcsr, CSR_IE, m_rxrdy);
		break;
	}

	DBG_LOG(2,"PC11 R", ("%d == %06o\n", offset, data));

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE16_MEMBER( pc11_device::write )
{
	DBG_LOG(2,"PC11 W", ("%d <- %06o\n", offset, data));

	switch (offset & 0x03)
	{
	case REGISTER_RCSR:
        if ((data & CSR_IE) == 0) {
			clear_virq(m_write_rxrdy, 1, 1, m_rxrdy);
		} else if ((m_rcsr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			raise_virq(m_write_rxrdy, 1, 1, m_rxrdy);
		}
		if (data & CSR_GO) {
			m_rcsr = (m_rcsr & ~CSR_DONE) | CSR_BUSY;
			clear_virq(m_write_rxrdy, m_rcsr, CSR_IE, m_rxrdy);
		}
		m_rcsr = ((m_rcsr & ~PTRCSR_WR) | (data & PTRCSR_WR));
		break;

	case REGISTER_RBUF:
		break;
	}
}


//-------------------------------------------------
//  rxrdy_r - receiver ready
//-------------------------------------------------

READ_LINE_MEMBER( pc11_device::rxrdy_r )
{
	return ((m_rcsr & (CSR_DONE|CSR_IE)) == (CSR_DONE|CSR_IE)) ? ASSERT_LINE : CLEAR_LINE;
}


//-------------------------------------------------
//  txemt_r - transmitter empty
//-------------------------------------------------

READ_LINE_MEMBER( pc11_device::txemt_r )
{
	return ((m_tcsr & (CSR_DONE|CSR_IE)) == (CSR_DONE|CSR_IE)) ? ASSERT_LINE : CLEAR_LINE;
}

void pc11_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	UINT8 reply;

	if (!(m_rcsr & CSR_BUSY))
		return;

	DBG_LOG(2,"PC11 Timer", ("rcsr %06o id %d param %d m_fd %p\n", m_rcsr, id, param, m_fd));

	m_rcsr = (m_rcsr | CSR_ERR) & ~CSR_BUSY;
	if (m_fd && m_fd->exists() && (m_fd->fread(&reply, 1) == 1)) {
		m_rcsr = (m_rcsr | CSR_DONE) & ~CSR_ERR;
		m_rbuf = reply;
	}
	raise_virq(m_write_rxrdy, m_rcsr, CSR_IE, m_rxrdy);
}
