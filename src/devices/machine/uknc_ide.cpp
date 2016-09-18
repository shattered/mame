// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

  ISA 16 bit IDE controller

***************************************************************************/

#include "emu.h"

#include "uknc_ide.h"
#include "machine/ataintf.h"
#include "imagedev/harddriv.h"


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


READ16_MEMBER(uknc_ide_device::read )
{
	UINT16 data = 0;

	data = m_ide->read_cs0(space, 7-offset, mem_mask);

	DBG_LOG(2,"IDE R", ("%x == %04x (%04x)\n", 0x1f0+7-offset, data, data ^ 0177777));

	if (offset == 7)
		return data;
	else
		return (~data) & 255;
}

WRITE16_MEMBER(uknc_ide_device::write )
{
	DBG_LOG(2,"IDE W", ("%x <- %04x (%04x) & %06o%s\n", 0x1f0+7-offset, data ^ 0177777, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	if (offset == 7)
		m_ide->write_cs0(space, 7-offset, data, mem_mask);
	else
		m_ide->write_cs0(space, 7-offset, (~data) & 255, 0xff);
}

static MACHINE_CONFIG_FRAGMENT( uknc_ide )
	MCFG_ATA_INTERFACE_ADD("ide", ata_devices, "hdd", nullptr, false)
MACHINE_CONFIG_END


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type UKNC_IDE = &device_creator<uknc_ide_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor uknc_ide_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( uknc_ide );
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  uknc_ide_device - constructor
//-------------------------------------------------

uknc_ide_device::uknc_ide_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
		: device_t(mconfig, UKNC_IDE, "IDE Fixed Drive Adapter", tag, owner, clock, "uknc_ide", __FILE__),
		m_ide(*this, "ide")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void uknc_ide_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void uknc_ide_device::device_reset()
{
}

void uknc_ide_device::ce0_ce3_w(int data)
{
	switch (data)
	{
		case 2:
		case 4:
		case 6:
			if (m_slot < 0) {
//				m_qbus->install_device(0100000, 0117777, *this, &uknc_ide_device::map);
				m_qbus->install_device(0100000, 0117777, 
					read16_delegate(FUNC(address_map_bank_device::read16),subdevice<address_map_bank_device>("bankdev")), 
					write16_delegate(FUNC(address_map_bank_device::write16),subdevice<address_map_bank_device>("bankdev")));
			}
			m_slot = (data >> 1) & 3;
			m_bankdev->set_bank(m_slot);
			break;

		default:
			m_slot = -1;
			break;
	}

	DBG_LOG(1,"UKNC_IDE CEx", ("<- %x (slot %d)\n", data, m_slot));
}
