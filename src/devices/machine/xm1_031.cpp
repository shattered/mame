// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	KR1515XM1-031 multifunction device

***************************************************************************/

#include "xm1_031.h"

#include "machine/kb_7007.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type XM1_031_KBD = &device_creator<xm1_031_kbd_device>;
const device_type XM1_031_TIMER = &device_creator<xm1_031_timer_device>;


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor xm1_031_kbd_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( mc7007_keyboard );
}



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VERBOSE_DBG 2       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-24s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  xm1_031_kbd_device - constructor
//-------------------------------------------------

xm1_031_kbd_device::xm1_031_kbd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, XM1_031_KBD, "K1801XM1-031 Kbd", tag, owner, clock, "xm1_031_kbd", __FILE__)
	, device_z80daisy_interface(mconfig, *this)
	, m_kbd(*this, "Y%u", 4)
	, m_out_virq(*this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void xm1_031_kbd_device::device_start()
{
	// resolve callbacks
	m_out_virq.resolve_safe();

	// save state
	save_item(NAME(m_csr));
	save_item(NAME(m_buf));

	m_timer = timer_alloc(0);
	m_timer->adjust(attotime::from_hz(50), 0, attotime::from_hz(50));

	m_keys = std::make_unique<bool[]>(128);

	m_buf = 0;
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void xm1_031_kbd_device::device_reset()
{
	DBG_LOG(1,"XM1-031 Reset", ("in progress\n"));

	m_csr = 0;
}

int xm1_031_kbd_device::z80daisy_irq_state()
{
	if (m_rxrdy == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int xm1_031_kbd_device::z80daisy_irq_ack()
{
	int vec = -1;

	if (m_rxrdy == ASSERT_LINE) {
		m_rxrdy = CLEAR_LINE;
		vec = 0300;
	} 

	if (vec > 0)
		DBG_LOG(1,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

void xm1_031_kbd_device::z80daisy_irq_reti()
{
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ16_MEMBER( xm1_031_kbd_device::read )
{
	UINT16 data = 0;

	switch (offset)
	{
	case 0:
		data = m_csr & KBDCSR_RD;
		break;

	case 1:
		data = m_buf;
		m_csr &= ~CSR_DONE;
		clear_virq(m_out_virq, m_csr, CSR_IE, m_rxrdy);
		break;
	}

	DBG_LOG(2,"XM1-031 Kbd R", ("%d == %06o\n", offset, data));

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE16_MEMBER( xm1_031_kbd_device::write )
{
	DBG_LOG(2,"XM1-031 Kbd W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	switch (offset & 0x03)
	{
	case 0:
        if ((data & CSR_IE) == 0) {
			clear_virq(m_out_virq, 1, 1, m_rxrdy);
		} else if ((m_csr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			raise_virq(m_out_virq, 1, 1, m_rxrdy);
		}
		m_csr = ((m_csr & ~KBDCSR_WR) | (data & KBDCSR_WR));
		break;
	}
}


void xm1_031_kbd_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	if (m_csr & CSR_DONE)
		return;

	for (int row = 0; row < 12; row++) {
		UINT8 bits = m_kbd[row]->read();

		if (bits) {
			int bit = 0;

			while (bits >>= 1) { ++bit; }
			m_buf = (bit << 4) + row + 4;

			if (!m_keys[m_buf]) {
				m_keys[m_buf] = TRUE;
				m_csr |= CSR_DONE;
				raise_virq(m_out_virq, m_csr, CSR_IE, m_rxrdy);
				DBG_LOG(2,"XM1-031 Kbd", ("press: row Y%d col X%d code %06o\n", row + 4, bit, m_buf));
				return;
			}
		} else {
			for (int bit = 0; bit < 8; bit++) {
				int key = (bit << 4) + row + 4;
				if (m_keys[key]) {
					m_keys[key] = FALSE;
					m_buf = (key & 15) | 0x80;
					m_csr |= CSR_DONE;
					raise_virq(m_out_virq, m_csr, CSR_IE, m_rxrdy);
					DBG_LOG(2,"XM1-031 Kbd", ("release: row Y%d col X%d code %06o\n", row + 4, bit, m_buf));
					return;
				}
			}
		}
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  xm1_031_timer_device - constructor
//-------------------------------------------------

xm1_031_timer_device::xm1_031_timer_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, XM1_031_TIMER, "K1801XM1-031 Timer", tag, owner, clock, "xm1_031_timer", __FILE__)
	, device_z80daisy_interface(mconfig, *this)
	, m_out_virq(*this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void xm1_031_timer_device::device_start()
{
	// resolve callbacks
	m_out_virq.resolve_safe();

	// save state
	save_item(NAME(m_csr));
	save_item(NAME(m_buf));
	save_item(NAME(m_value));
	save_item(NAME(m_counter));

	m_timer = timer_alloc(0);
	m_timer->adjust(attotime::never, 0, attotime::never);

	m_buf = m_csr = m_value = m_counter = 0;
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void xm1_031_timer_device::device_reset()
{
	DBG_LOG(1,"XM1-031 Reset", ("in progress\n"));

	m_csr &= ~(TMRCSR_MODE|TMRCSR_EVNT);
}

int xm1_031_timer_device::z80daisy_irq_state()
{
	if (m_rxrdy == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int xm1_031_timer_device::z80daisy_irq_ack()
{
	int vec = -1;

	if (m_rxrdy == ASSERT_LINE) {
		m_rxrdy = CLEAR_LINE;
		vec = 0304;
	} 

	if (vec > 0)
		DBG_LOG(1,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

void xm1_031_timer_device::z80daisy_irq_reti()
{
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ16_MEMBER( xm1_031_timer_device::read )
{
	UINT16 data = 0;

	switch (offset)
	{
	case 0:
		data = m_csr & TMRCSR_RD;
		m_csr &= ~TMRCSR_OVF;
		break;

	case 2:
		data = m_value & TMRBUF_RD;
		m_csr &= ~CSR_DONE;
		clear_virq(m_out_virq, m_csr, CSR_IE, m_rxrdy);
		break;
	}

	DBG_LOG(2,"XM1-031 Timer R", ("%d == %06o\n", offset, data));

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE16_MEMBER( xm1_031_timer_device::write )
{
	DBG_LOG(2,"XM1-031 Timer W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	switch (offset & 0x03)
	{
	case 0:
		if (BIT(data, 0)) {
			int period = (data >> 1) & 3;
			m_timer->adjust(attotime::from_usec(2 << period), 0, attotime::from_usec(2 << period));
			DBG_LOG(2,"XM1-031 Timer", ("enabled, period %d usec, counter value %d\n", 2 << period, m_counter));
		} else {
			m_timer->adjust(attotime::never, 0, attotime::never);
			DBG_LOG(2,"XM1-031 Timer", ("disabled\n"));
		}
        if ((data & CSR_IE) == 0) {
			clear_virq(m_out_virq, 1, 1, m_rxrdy);
		} else if ((m_csr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			raise_virq(m_out_virq, 1, 1, m_rxrdy);
		}
		m_csr = ((m_csr & ~TMRCSR_WR) | (data & TMRCSR_WR));
		break;

	case 1:
		m_buf = data & TMRBUF_WR;
		if (!BIT(m_csr, 0))
			m_value = m_counter = m_buf;
		break;
	}
}

/*
 * While enabled and having not yet reached zero (DONE=0), timer counts down and copies current count to value
 * register. After reaching zero, it stops updating value register and:
 * - if DONE is not yet set, sets it and optionally raises VIRQ;
 * - it DONE is set, sets OVF.
 * in either case, it reloads counter from buffer and starts counting down again.
 * DONE is cleared on read of value register.
 * OVF is cleared on read of CSR.
 */

void xm1_031_timer_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	if (m_counter) {
		m_counter--;

		if (m_counter == 0) {
			if (m_csr & CSR_DONE) {
				m_csr |= TMRCSR_OVF;
			} 
			m_csr |= CSR_DONE;
			raise_virq(m_out_virq, m_csr, CSR_IE, m_rxrdy);
			m_value = m_counter = m_buf;
		} else {
			m_value = m_counter;
		}
	}
}
