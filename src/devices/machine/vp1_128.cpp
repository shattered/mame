// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	K1801VP1-128 floppy controller, follow-up and compatible to VP1-097.

	CSR bits (write):
	- 10 REZ -- reserved (precompensation)
	- 09 WM -- write marker
	- 08 GOR -- start marker search
	- 07 ST -- step
	- 06 DIR -- step direction
	- 05 HS -- head select
	- 04 MSW -- motor on
	- 03 DS3 -- drive select 3
	- 02 DS3 -- drive select 2
	- 01 DS3 -- drive select 1
	- 00 DS3 -- drive select 0

	CSR bits (read):
	- 15 IND -- index
	- 14 CRC -- while reading: CRC valid, while writing: CRC write done
	- 07 TR -- data ready
	- 02 WRP -- write protected
	- 01 RDY -- ready
	- 00 TR0 -- track zero

***************************************************************************/

#include "vp1_128.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VP1_128 = &device_creator<vp1_128_device>;



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
//  vp1_128_device - constructor
//-------------------------------------------------

vp1_128_device::vp1_128_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, VP1_128, "K1801VP1-128", tag, owner, clock, "vp1_128", __FILE__),
	device_z80daisy_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vp1_128_device::device_start()
{
	// resolve callbacks

	// save state
	save_item(NAME(m_csr));
	save_item(NAME(m_rbuf));
	save_item(NAME(m_tbuf));

	m_csr = FLPCSR_RDY|FLPCSR_TR0;
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void vp1_128_device::device_reset()
{
	DBG_LOG(1,"VP1-128 Reset", ("in progress\n"));

	m_rbuf = m_tbuf = 0;
}

int vp1_128_device::z80daisy_irq_state()
{
	return 0;
}

int vp1_128_device::z80daisy_irq_ack()
{
	return -1;
}

void vp1_128_device::z80daisy_irq_reti()
{
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ16_MEMBER( vp1_128_device::read )
{
	UINT16 data = 0;

	switch (offset & 0x03)
	{
	case 0:
		data = m_csr & FLPCSR_RD;
		break;

	case 1:
		data = m_rbuf;
		break;
	}

	DBG_LOG(2,"VP1-128 Floppy R", ("%d == %06o\n", offset, data));

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE16_MEMBER( vp1_128_device::write )
{
	DBG_LOG(2,"VP1-128 Floppy W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));
}
