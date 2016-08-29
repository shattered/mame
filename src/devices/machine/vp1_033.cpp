// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	K1801VP1-033 multifunction device:
	- "FDIC" mode -- DEC RX11 floppy interface
	- "PIC" mode -- 16-bit parallel interface
	- "BPIC" mode -- 8-bit parallel interface

	CSR bits in "PIC" mode:
	- 15 (R) REQB -- interrupt request B
	- 07 (R) REQA -- interrupt request A
	- 06 (RW) IEA -- interrupt enable A
	- 05 (RW) IEB -- interrupt enable B
	- 01 (RW) CSR1 -- output pin
	- 00 (RW) CSR0 -- output pin

***************************************************************************/

#include "vp1_033.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VP1_033 = &device_creator<vp1_033_device>;



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


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vp1_033_device - constructor
//-------------------------------------------------

vp1_033_device::vp1_033_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, VP1_033, "K1801VP1-033", tag, owner, clock, "vp1_033", __FILE__),
	device_serial_interface(mconfig, *this),
	device_z80daisy_interface(mconfig, *this),
	m_write_pic_csr0(*this),
	m_write_pic_csr1(*this),
	m_write_pic_out(*this),
	m_vec_a(0),
	m_vec_b(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vp1_033_device::device_start()
{
	// resolve callbacks
	m_write_pic_csr0.resolve_safe();
	m_write_pic_csr1.resolve_safe();
	m_write_pic_out.resolve_safe();

	// save state
	save_item(NAME(m_csr));
	save_item(NAME(m_rbuf));
	save_item(NAME(m_tbuf));

	m_csr = 0;
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void vp1_033_device::device_reset()
{
	DBG_LOG(1,"VP1-033 Reset", ("vec_a %03o vec_b %03o\n", m_vec_a, m_vec_b));

	m_rbuf = m_tbuf = 0;
	m_csr &= (PICCSR_WR ^ PICCSR_RD);

	m_write_pic_csr0(0);
	m_write_pic_csr1(0);
}

int vp1_033_device::z80daisy_irq_state()
{
	if (m_reqa == ASSERT_LINE || m_reqb == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int vp1_033_device::z80daisy_irq_ack()
{
	int vec = -1;

	if (m_reqb == ASSERT_LINE) {
		m_reqb = CLEAR_LINE;
		vec = m_vec_b;
	} else if (m_reqa == ASSERT_LINE) {
		m_reqa = CLEAR_LINE;
		vec = m_vec_a;
	}

	if (vec > 0)
		DBG_LOG(1,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

void vp1_033_device::z80daisy_irq_reti()
{
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ16_MEMBER( vp1_033_device::read )
{
	UINT16 data = 0;

	switch (offset & 0x03)
	{
	case 0:
		data = m_csr & PICCSR_RD;
		break;

	case 1:
		data = m_tbuf;
		break;

	case 2:
		data = m_rbuf;
		break;
	}

	DBG_LOG(2,"VP1-033 PIC R", ("%d == %06o\n", offset, data));

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE16_MEMBER( vp1_033_device::write )
{
	DBG_LOG(2,"VP1-033 PIC W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	switch (offset & 0x03)
	{
	case 0:
#if 0
        if ((data & CSR_IE) == 0) {
			M_WRITE_RXRDY(CLEAR_LINE);
		} else if ((m_csr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			M_WRITE_RXRDY(ASSERT_LINE);
		}
#endif
		m_csr = ((m_csr & ~PICCSR_WR) | (data & PICCSR_WR));
		m_write_pic_csr0(BIT(m_csr, 0));
		m_write_pic_csr1(BIT(m_csr, 1));
		break;

	case 1:
		COMBINE_DATA(&m_tbuf);
		m_write_pic_out(data, mem_mask);
		break;
	}
}

