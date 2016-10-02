// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

  KNGMD floppy drive interface

***************************************************************************/

#include "emu.h"

#include "dvk_kngmd.h"


//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************


#define VERBOSE_DBG 2       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-16s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)


READ16_MEMBER(dvk_kngmd_device::read)
{
	UINT16 data = 0;

	switch (offset)
	{
		case 0:
			data = m_mxcs;
			if (m_floppy && !m_floppy->trk00_r())
				data |= MXCSR_TRK0;
			m_mxcs &= ~MXCSR_TIMER;
			break;
	}

	DBG_LOG(2,"MX R", ("%6o == %06o; state %d\n", 0177130+(offset<<1), data, m_state));

	return data;
}

WRITE16_MEMBER(dvk_kngmd_device::write)
{
	m_unit = MXCSR_GETDRIVE(data);
	m_floppy = m_floppies[m_unit];

	DBG_LOG(2,"MX W", ("%6o <- %06o & %06o%s; state %d unit %d cyl %d\n", 
		0177130+(offset<<1), data, mem_mask, mem_mask==0xffff?"":" WARNING", 
		m_state, m_unit, m_floppy ? m_floppy->get_cyl() : -1));

	switch (offset)
	{
		case 0:
			m_floppy->dir_w(BIT(data, 5));
			if (BIT(data, 4)) {
				m_floppy->stp_w(0);
				m_floppy->stp_w(1);
			}

			// motor on
			if (BIT(data, 6)) {
				m_floppy->mon_w(0);
				m_floppy->setup_index_pulse_cb(floppy_image_device::index_pulse_cb(FUNC(dvk_kngmd_device::index_callback), this));
			}
			if (BIT(data, 1)) {
				m_floppy->mon_w(1);
				m_floppy->setup_index_pulse_cb(floppy_image_device::index_pulse_cb());
			}

			if (BIT(data, 7)) {
				m_timer_2khz->adjust(attotime::from_hz(2000), 0, attotime::from_hz(2000));
			} else {
				m_timer_2khz->adjust(attotime::never, 0, attotime::never);
			}

			m_floppy->ss_w(BIT(data, 12));

			if (data & MXCSR_GO) {
				command(BIT(data, 13));
			}
			UPDATE_16BIT(&m_mxcs, data, MXCSR_WR);
			break;
	}
}

void dvk_kngmd_device::command(int command)
{
	DBG_LOG(1,"MX",("command %s drive %d\n", command?"WRITE":"READ", m_unit));
}

void dvk_kngmd_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	DBG_LOG(2,"MX Timer",("fired %d param %d m_state %d\n", id, param, m_state));

	switch (id)
	{
		case TIMER_ID_2KHZ:
			m_mxcs |= MXCSR_TIMER;
			break;
	}
}


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

FLOPPY_FORMATS_MEMBER( dvk_kngmd_device::floppy_formats )
    FLOPPY_HFE_FORMAT,
    FLOPPY_IPF_FORMAT,
    FLOPPY_MFI_FORMAT
FLOPPY_FORMATS_END0

static SLOT_INTERFACE_START( mx_floppies )
	SLOT_INTERFACE( "525dd", FLOPPY_525_DD )
SLOT_INTERFACE_END

static MACHINE_CONFIG_FRAGMENT( dvk_kngmd )
	MCFG_FLOPPY_DRIVE_ADD("0", mx_floppies, "525dd", dvk_kngmd_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("1", mx_floppies, "525dd", dvk_kngmd_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("2", mx_floppies, "525dd", dvk_kngmd_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("3", mx_floppies, "525dd", dvk_kngmd_device::floppy_formats)
MACHINE_CONFIG_END

const device_type DVK_KNGMD = &device_creator<dvk_kngmd_device>;


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  dvk_kngmd_device - constructor
//-------------------------------------------------

dvk_kngmd_device::dvk_kngmd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
		: device_t(mconfig, DVK_KNGMD, "MX Floppy Drive Adapter", tag, owner, clock, "dvk_kngmd", __FILE__)
		, device_qbus_card_interface(mconfig, *this)
{
}

void dvk_kngmd_device::index_callback(floppy_image_device *floppy, int state)
{
	DBG_LOG(2,"MX Index",("state %d\n", state));

	if (state)
		m_mxcs |= MXCSR_INDEX;
	else
		m_mxcs &= ~MXCSR_INDEX;
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dvk_kngmd_device::device_start()
{
	set_qbus_device();

	// save state
	save_item(NAME(m_mxcs));
	save_item(NAME(m_rbuf));
	save_item(NAME(m_wbuf));

	save_item(NAME(m_unit));
	save_item(NAME(m_state));
	save_item(NAME(m_seektime));
	save_item(NAME(m_waittime));

	m_timer_2khz = timer_alloc(TIMER_ID_2KHZ);
	m_timer_wait = timer_alloc(TIMER_ID_WAIT);

	m_seektime = 10; // ms, per EK-RX01-OP-001 page 1-13
	m_waittime = 80; // us, per EK-RX01-OP-001 page 4-1: 18 us for PDP-8 card

	m_qbus->install_device(0177130, 0177133, read16_delegate(FUNC(dvk_kngmd_device::read),this), 
		write16_delegate(FUNC(dvk_kngmd_device::write),this));

	m_mxcs = 0;

	m_floppies[0] = subdevice<floppy_connector>("0")->get_device();
	m_floppies[1] = subdevice<floppy_connector>("1")->get_device();
	m_floppies[2] = subdevice<floppy_connector>("2")->get_device();
	m_floppies[3] = subdevice<floppy_connector>("3")->get_device();
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void dvk_kngmd_device::device_reset()
{
	DBG_LOG(1,"MX",("Reset in progress\n"));

	for(auto & elem : m_floppies)
	{
		elem->set_rpm(300.);
	}

	m_rbuf = m_wbuf = 0;
	m_unit = 0;
	m_floppy = nullptr;
	m_state = 0;

	m_mxcs &= ~MXCSR_ERR;
	m_timer_2khz->adjust(attotime::never, 0, attotime::never);
	m_timer_wait->adjust(attotime::never, 0, attotime::never);
}


//-------------------------------------------------
//  device_mconfig_additions - return a pointer to
//  the device's machine fragment
//-------------------------------------------------

machine_config_constructor dvk_kngmd_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( dvk_kngmd );
}
