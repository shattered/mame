// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	K1801VP1-120 -- 3 parallel channels (2 bidir, 1 uni)

***************************************************************************/

#include "vp1_120.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VP1_120 = &device_creator<vp1_120_device>;
const device_type VP1_120_SUB = &device_creator<vp1_120_sub_device>;



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


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vp1_120_device - constructor
//-------------------------------------------------

vp1_120_device::vp1_120_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, VP1_120, "K1801VP1-120", tag, owner, clock, "vp1_120", __FILE__)
	, device_z80daisy_interface(mconfig, *this)
	, m_out_virq(*this)
	, m_out_channel0(*this)
	, m_out_channel1(*this)
	, m_out_channel2(*this)
	, m_ack_channel0(*this)
	, m_ack_channel1(*this)
	, m_out_reset(*this)
{
}

vp1_120_sub_device::vp1_120_sub_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, VP1_120_SUB, "K1801VP1-120 sub", tag, owner, clock, "vp1_120_sub", __FILE__)
	, device_z80daisy_interface(mconfig, *this)
	, m_out_virq(*this)
	, m_out_channel0(*this)
	, m_out_channel1(*this)
	, m_ack_channel0(*this)
	, m_ack_channel1(*this)
	, m_ack_channel2(*this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vp1_120_device::device_start()
{
	// resolve callbacks
	m_out_virq.resolve_safe();
	m_out_channel0.resolve_safe();
	m_out_channel1.resolve_safe();
	m_out_channel2.resolve_safe();
	m_ack_channel0.resolve_safe();
	m_ack_channel1.resolve_safe();
	m_out_reset.resolve_safe();

	// save state
	save_item(NAME(m_channel[0].rcsr));
	save_item(NAME(m_channel[0].rbuf));
	save_item(NAME(m_channel[0].tcsr));
	save_item(NAME(m_channel[0].tbuf));
	save_item(NAME(m_channel[1].rcsr));
	save_item(NAME(m_channel[1].rbuf));
	save_item(NAME(m_channel[1].tcsr));
	save_item(NAME(m_channel[1].tbuf));
	save_item(NAME(m_channel[2].tcsr));
	save_item(NAME(m_channel[2].tbuf));

	memset(m_channel, 0, sizeof(m_channel));
	m_channel[1].channel = 1;
	m_channel[2].channel = 2;
}

void vp1_120_sub_device::device_start()
{
	// resolve callbacks
	m_out_virq.resolve_safe();
	m_out_channel0.resolve_safe();
	m_out_channel1.resolve_safe();
	m_ack_channel0.resolve_safe();
	m_ack_channel1.resolve_safe();
	m_ack_channel2.resolve_safe();

	// save state
	save_item(NAME(m_channel[0].rbuf));
	save_item(NAME(m_channel[0].tbuf));
	save_item(NAME(m_channel[1].rbuf));
	save_item(NAME(m_channel[1].tbuf));
	save_item(NAME(m_channel[2].rbuf));
	save_item(NAME(m_rcsr));
	save_item(NAME(m_tcsr));

	m_reset = CLEAR_LINE;
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void vp1_120_device::device_reset()
{
	DBG_LOG(1,"VP1-120 Reset", ("in progress\n"));

	m_channel[0].tcsr = m_channel[1].tcsr = m_channel[2].tcsr = CSR_DONE;
	m_channel[0].rcsr = m_channel[1].rcsr = m_channel[2].rcsr = 0;

	m_out_reset(ASSERT_LINE);
}

void vp1_120_sub_device::device_reset()
{
	DBG_LOG(1,"VP1-120 sub Reset", ("in progress\n"));

	memset(m_channel, 0, sizeof(m_channel));

	m_rcsr = 0;
	m_tcsr = PPTCSR_1DONE|PPTCSR_0DONE;
}

int vp1_120_device::z80daisy_irq_state()
{
	if ((m_channel[0].rxrdy|m_channel[1].rxrdy|m_channel[0].txrdy|m_channel[1].txrdy|m_channel[2].txrdy) == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int vp1_120_sub_device::z80daisy_irq_state()
{
	if ((m_channel[0].rxrdy|m_channel[1].rxrdy|m_channel[2].rxrdy|m_channel[0].txrdy|m_channel[1].txrdy|m_reset) == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int vp1_120_device::z80daisy_irq_ack()
{
	int vec = -1;

	if (m_channel[0].rxrdy == ASSERT_LINE) {
		m_channel[0].rxrdy = CLEAR_LINE;
		vec = 060;
	} else if (m_channel[0].txrdy == ASSERT_LINE) {
		m_channel[0].txrdy = CLEAR_LINE;
		vec = 064;
	} else if (m_channel[1].rxrdy == ASSERT_LINE) {
		m_channel[1].rxrdy = CLEAR_LINE;
		vec = 0460;
	} else if (m_channel[1].txrdy == ASSERT_LINE) {
		m_channel[1].txrdy = CLEAR_LINE;
		vec = 0464;
	} else if (m_channel[2].txrdy == ASSERT_LINE) {
		m_channel[2].txrdy = CLEAR_LINE;
		vec = 0467;
	}

	if (vec > 0)
		DBG_LOG(1,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

int vp1_120_sub_device::z80daisy_irq_ack()
{
	int vec = -1;

	if (m_reset == ASSERT_LINE) {
		m_reset = CLEAR_LINE;
		vec = 0314;
	} else if (m_channel[0].rxrdy == ASSERT_LINE) {
		m_channel[0].rxrdy = CLEAR_LINE;
		vec = 0320;
	} else if (m_channel[0].txrdy == ASSERT_LINE) {
		m_channel[0].txrdy = CLEAR_LINE;
		vec = 0324;
	} else if (m_channel[1].rxrdy == ASSERT_LINE) {
		m_channel[1].rxrdy = CLEAR_LINE;
		vec = 0330;
	} else if (m_channel[1].txrdy == ASSERT_LINE) {
		m_channel[1].txrdy = CLEAR_LINE;
		vec = 0334;
	} else if (m_channel[2].rxrdy == ASSERT_LINE) {
		m_channel[2].rxrdy = CLEAR_LINE;
		vec = 0340;
	}

	if (vec > 0)
		DBG_LOG(1,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

void vp1_120_device::z80daisy_irq_reti()
{
}

void vp1_120_sub_device::z80daisy_irq_reti()
{
}


READ16_MEMBER( vp1_120_device::read0 )
{
	UINT16 data = channel_read(&m_channel[0], offset);

	DBG_LOG(2,"Channel 0 R", ("%d == %06o\n", offset, data));

	return data;
}

READ16_MEMBER( vp1_120_device::read1 )
{
	UINT16 data = channel_read(&m_channel[1], offset);

	DBG_LOG(2,"Channel 1 R", ("%d == %06o\n", offset, data));

	return data;
}

READ16_MEMBER( vp1_120_device::read2 )
{
	UINT16 data = channel_read(&m_channel[2], offset);

	DBG_LOG(2,"Channel 2 R", ("%d == %06o\n", offset, data));

	return data;
}

WRITE16_MEMBER( vp1_120_device::write0 )
{
	DBG_LOG(2,"Channel 0 W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	channel_write(&m_channel[0], offset, data, mem_mask);
}

WRITE16_MEMBER( vp1_120_device::write1 )
{
	DBG_LOG(2,"Channel 1 W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	channel_write(&m_channel[1], offset, data, mem_mask);
}

WRITE16_MEMBER( vp1_120_device::write2 )
{
	DBG_LOG(2,"Channel 2 W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	channel_write(&m_channel[2], offset, data, mem_mask);
}


UINT16 vp1_120_device::channel_read(channel_t *channel, offs_t offset)
{
	UINT16 data = 0;

	switch (offset & 0x03)
	{
	case REGISTER_RCSR:
		data = channel->rcsr & SRCCSR_RD;
		break;

	case REGISTER_RBUF:
		data = channel->rbuf;
		channel->rcsr &= ~CSR_DONE;
		clear_virq(m_out_virq, channel->rcsr, CSR_IE, channel->rxrdy);
		switch (channel->channel)
		{
		case 0:	m_ack_channel0(ASSERT_LINE); break;
		case 1:	m_ack_channel1(ASSERT_LINE); break;
		}
		break;

	case REGISTER_TCSR:
		data = channel->tcsr & SRCCSR_RD;
		break;
	}

	return data;
}

void vp1_120_device::channel_write(channel_t *channel, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	switch (offset & 0x03)
	{
	case REGISTER_RCSR:
        if ((data & CSR_IE) == 0) {
			clear_virq(m_out_virq, 1, 1, channel->rxrdy);
		} else if ((channel->rcsr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			raise_virq(m_out_virq, 1, 1, channel->rxrdy);
		}
		channel->rcsr = ((channel->rcsr & ~SRCCSR_WR) | (data & SRCCSR_WR));
		break;

	case REGISTER_RBUF:
		// unmapped
		break;

	case REGISTER_TCSR:
        if ((data & CSR_IE) == 0) {
			clear_virq(m_out_virq, 1, 1, channel->txrdy);
        } else if ((channel->tcsr & (CSR_DONE + CSR_IE)) == CSR_DONE) {
			raise_virq(m_out_virq, 1, 1, channel->txrdy);
		}
		channel->tcsr = ((channel->tcsr & ~SRCCSR_WR) | (data & SRCCSR_WR));
		break;

	case REGISTER_TBUF:
		channel->tcsr &= ~CSR_DONE;
		DBG_LOG(1,"Chan Tx", ("ch %d tbuf %06o '%c' tcsr %06o\n", 
			channel->channel, data, ((data&127) < 32 ? 32 : (data&127)), channel->tcsr));
		clear_virq(m_out_virq, channel->tcsr, CSR_IE, channel->txrdy);
		switch (channel->channel)
		{
		case 0:	m_out_channel0(data); break;
		case 1:	m_out_channel1(data); break;
		case 2:	m_out_channel2(data); break;
		}
		break;
	}
}


READ16_MEMBER( vp1_120_sub_device::read )
{
	UINT16 data = 0;

	switch (offset) {
		case 0:	// ch0 data
			data = m_channel[0].rbuf;
			m_rcsr &= ~PPRCSR_0RDY;
			clear_virq(m_out_virq, m_rcsr, PPRCSR_0IE, m_channel[0].rxrdy);
			m_ack_channel0(ASSERT_LINE);
			break;

		case 1:	// ch1 data
			data = m_channel[1].rbuf;
			m_rcsr &= ~PPRCSR_1RDY;
			clear_virq(m_out_virq, m_rcsr, PPRCSR_1IE, m_channel[1].rxrdy);
			m_ack_channel1(ASSERT_LINE);
			break;

		case 2:	// ch2 data
			data = m_channel[2].rbuf;
			m_rcsr &= ~PPRCSR_2RDY;
			clear_virq(m_out_virq, m_rcsr, PPRCSR_2IE, m_channel[2].rxrdy);
			m_ack_channel2(ASSERT_LINE);
			break;

		case 3:
			data = m_rcsr & PPRCSR_RD;
			break;

		case 7:
			data = m_tcsr & PPTCSR_RD;
			break;
	}

	DBG_LOG(2,"Subchan R", ("%d == %06o\n", offset, data));

	return data;
}

WRITE16_MEMBER( vp1_120_sub_device::write )
{
	DBG_LOG(2,"Subchan W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	switch (offset) {
		case 3:
			if ((data & PPRCSR_0IE) == 0) {
				clear_virq(m_out_virq, 1, 1, m_channel[0].rxrdy);
			} else if ((m_rcsr & (PPRCSR_0RDY + PPRCSR_0IE)) == PPRCSR_0RDY) {
				m_channel[0].rxrdy = (ASSERT_LINE);
			}
			if ((data & PPRCSR_1IE) == 0) {
				clear_virq(m_out_virq, 1, 1, m_channel[1].rxrdy);
			} else if ((m_rcsr & (PPRCSR_1RDY + PPRCSR_1IE)) == PPRCSR_1RDY) {
				m_channel[1].rxrdy = (ASSERT_LINE);
			}
			if ((data & PPRCSR_2IE) == 0) {
				clear_virq(m_out_virq, 1, 1, m_channel[2].rxrdy);
			} else if ((m_rcsr & (PPRCSR_2RDY + PPRCSR_2IE)) == PPRCSR_2RDY) {
				m_channel[2].rxrdy = (ASSERT_LINE);
			}
			m_rcsr = ((m_rcsr & ~PPRCSR_WR) | (data & PPRCSR_WR));
			break;

		case 4:	// ch0 data
			m_out_channel0(data);
			clear_virq(m_out_virq, m_tcsr, PPTCSR_0IE, m_channel[0].txrdy);
			break;

		case 5:	// ch1 data
			m_out_channel1(data);
			clear_virq(m_out_virq, m_tcsr, PPTCSR_1IE, m_channel[1].txrdy);
			break;

		case 7:
			if ((data & PPTCSR_0IE) == 0) {
				clear_virq(m_out_virq, 1, 1, m_channel[0].txrdy);
			} else if ((m_tcsr & (PPTCSR_0DONE + PPTCSR_0IE)) == PPTCSR_0DONE) {
				raise_virq(m_out_virq, 1, 1, m_channel[0].txrdy);
			}
			if ((data & PPTCSR_1IE) == 0) {
				clear_virq(m_out_virq, 1, 1, m_channel[1].txrdy);
			} else if ((m_tcsr & (PPTCSR_1DONE + PPTCSR_1IE)) == PPTCSR_1DONE) {
				raise_virq(m_out_virq, 1, 1, m_channel[1].txrdy);
			}
			m_tcsr = ((m_tcsr & ~PPTCSR_WR) | (data & PPTCSR_WR));
			break;
	}

}
