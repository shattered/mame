// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	UKNC ROM cartridge

***************************************************************************/

#include "uknc_rom.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type UKNC_ROM = &device_creator<uknc_rom_device>;



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
//  uknc_rom_device - constructor
//-------------------------------------------------

uknc_rom_device::uknc_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, UKNC_ROM, "UKNC_ROM", tag, owner, clock, "uknc_rom", __FILE__),
	device_image_interface(mconfig, *this),
	device_qbus_card_interface(mconfig, *this), m_base(nullptr)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void uknc_rom_device::device_start()
{
	set_qbus_device();

	// save state
	save_item(NAME(m_slot));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void uknc_rom_device::device_reset()
{
	DBG_LOG(1,"UKNC_ROM Reset", ("in progress\n"));
}


void uknc_rom_device::ce0_ce3_w(int data)
{
	DBG_LOG(1,"UKNC_ROM CEx", ("<- %x\n", data));

	switch (data)
	{
		case 2:
		case 4:
		case 6:
			m_qbus->install_device(0100000, 0117777, read16_delegate(FUNC(uknc_rom_device::read),this), 
				write16_delegate(FUNC(uknc_rom_device::write),this));
			m_slot = ((data >> 1) & 3) - 1;
			break;

		default:
			m_slot = -1;
			break;
	}
}


image_init_result uknc_rom_device::call_load()
{
	device_image_interface* image = this;
	UINT64 size = image->length();

	DBG_LOG(1,"call_load", ("filename %s size %d\n", image->filename(), size));

	m_base = std::make_unique<UINT8[]>(24576);
	if(size <= 24576)
	{
		image->fread(m_base.get(),size);
		return image_init_result::PASS;
	}
	else
	{
		return image_init_result::FAIL;
	}
}

void uknc_rom_device::call_unload()
{
	m_base = nullptr;
}


READ16_MEMBER( uknc_rom_device::read )
{
	UINT16 data = 0;

	if (m_slot >= 0) {
		data  = m_base[m_slot*8192 + (offset << 1)];
		data |= m_base[m_slot*8192 + (offset << 1) + 1] << 8;
	}

	DBG_LOG(2,"UKNC_ROM R", ("%d == %04x (%06o)\n", offset, data, data));

	return data;
}

WRITE16_MEMBER( uknc_rom_device::write )
{
	DBG_LOG(2,"UKNC_ROM W", ("%d <- %04x (%06o)\n", offset, data, data));
}
