// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/**********************************************************************

    Digital Equipment Corp. LSI-11 Bus (Qbus) emulation

**********************************************************************/

#include "qbus.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type QBUS_SLOT = &device_creator<qbus_slot_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  device_qbus_card_interface - constructor
//-------------------------------------------------

device_qbus_card_interface::device_qbus_card_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device)
//	, device_z80daisy_interface(mconfig, *this)
{
}

void device_qbus_card_interface::static_set_qbus(device_t &device, device_t *qbus_device)
{
	device_qbus_card_interface &qbus_card = dynamic_cast<device_qbus_card_interface &>(device);
	qbus_card.m_qbus_dev = qbus_device;
}

void device_qbus_card_interface::set_qbus_device()
{
	m_qbus = dynamic_cast<qbus_device *>(m_qbus_dev);
	m_qbus->add_qbus_card(this);
}


//-------------------------------------------------
//  qbus_slot_device - constructor
//-------------------------------------------------

qbus_slot_device::qbus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, QBUS_SLOT, "Qbus slot", tag, owner, clock, "qbus_slot", __FILE__)
	, device_slot_interface(mconfig, *this)
	, m_write_birq4(*this)
	, m_write_bdmr(*this)
{
}

void qbus_slot_device::static_set_qbus_slot(device_t &device, device_t *owner, const char *qbus_tag)
{
	qbus_slot_device &qbus_card = dynamic_cast<qbus_slot_device &>(device);
	qbus_card.m_owner = owner;
	qbus_card.m_qbus_tag = qbus_tag;
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void qbus_slot_device::device_start()
{
	device_qbus_card_interface *dev = dynamic_cast<device_qbus_card_interface *>(get_card_device());

	if (dev) device_qbus_card_interface::static_set_qbus(*dev,m_owner->subdevice(m_qbus_tag));

	// resolve callbacks
	m_write_birq4.resolve_safe();
	m_write_bdmr.resolve_safe();
}


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type QBUS = &device_creator<qbus_device>;

void qbus_device::static_set_cputag(device_t &device, const char *tag)
{
	qbus_device &qbus = downcast<qbus_device &>(device);
	qbus.m_cputag = tag;
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void qbus_device::device_config_complete()
{
	m_maincpu = subdevice<cpu_device>(m_cputag);
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  qbus_device - constructor
//-------------------------------------------------

qbus_device::qbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
		device_t(mconfig, QBUS, "Qbus", tag, owner, clock, "qbus", __FILE__),
		device_memory_interface(mconfig, *this),
		device_z80daisy_interface(mconfig, *this),
		m_program_config("QBUS 16-bit program", ENDIANNESS_LITTLE, 16, 24, 0, nullptr),
		m_out_birq4_cb(*this),
		m_out_birq5_cb(*this),
		m_out_birq6_cb(*this),
		m_out_birq7_cb(*this),
		m_out_bdmr_cb(*this),
		m_cputag(nullptr)
{
}

qbus_device::qbus_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source) :
		device_t(mconfig, type, name, tag, owner, clock, shortname, source),
		device_memory_interface(mconfig, *this),
		device_z80daisy_interface(mconfig, *this),
		m_program_config("QBUS 16-bit program", ENDIANNESS_LITTLE, 16, 24, 0, nullptr),
		m_out_birq4_cb(*this),
		m_out_birq5_cb(*this),
		m_out_birq6_cb(*this),
		m_out_birq7_cb(*this),
		m_out_bdmr_cb(*this),
		m_cputag(nullptr)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void qbus_device::device_start()
{
	// resolve callbacks
	m_out_birq4_cb.resolve_safe();
	m_out_birq5_cb.resolve_safe();
	m_out_birq6_cb.resolve_safe();
	m_out_birq7_cb.resolve_safe();
	m_out_bdmr_cb.resolve_safe();

	m_maincpu = subdevice<cpu_device>(m_cputag);

	m_prgspace = &m_maincpu->space(AS_PROGRAM);
	m_prgwidth = m_maincpu->space_config(AS_PROGRAM)->m_databus_width;
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void qbus_device::device_reset()
{
	device_qbus_card_interface *entry = m_device_list.first();

	while (entry)
	{
		entry->device_reset();
		entry = entry->next();
	}
}

void qbus_device::add_qbus_card(device_qbus_card_interface *card)
{
	m_device_list.append(*card);
}

int qbus_device::z80daisy_irq_state()
{
	int data = 0;
	device_qbus_card_interface *entry = m_device_list.first();

	while (entry)
	{
		data = entry->z80daisy_irq_state();
		if (data)
			return data;
		entry = entry->next();
	}

	return data;
}

int qbus_device::z80daisy_irq_ack()
{
	int vec = -1;
	device_qbus_card_interface *entry = m_device_list.first();

	while (entry)
	{
		vec = entry->z80daisy_irq_ack();
		if (vec > 0)
			return vec;
		entry = entry->next();
	}

	return vec;
}

void qbus_device::z80daisy_irq_reti()
{
}



void qbus_device::install_space(address_spacenum spacenum, offs_t start, offs_t end, read16_delegate rhandler, write16_delegate whandler)
{
	int buswidth;
	address_space *space;

	if (spacenum == AS_PROGRAM)
	{
		space = m_prgspace;
		buswidth = m_prgwidth;
	}
	else
	{
		fatalerror("Unknown space passed to qbus_device::install_space!\n");
	}

	switch(buswidth)
	{
		case 16:
			space->install_readwrite_handler(start, end, rhandler, whandler, 0xffff);
			break;
		default:
			fatalerror("QBUS: Bus width %d not supported\n", buswidth);
	}
}


void qbus_device::install_device(offs_t start, offs_t end, read16_delegate rhandler, write16_delegate whandler)
{
	install_space(AS_PROGRAM, start, end, rhandler, whandler);
}

// interrupt request from Qbus card
WRITE_LINE_MEMBER( qbus_device::birq4_w ) { m_out_birq4_cb(state); }
WRITE_LINE_MEMBER( qbus_device::birq5_w ) { m_out_birq5_cb(state); }
WRITE_LINE_MEMBER( qbus_device::birq6_w ) { m_out_birq6_cb(state); }
WRITE_LINE_MEMBER( qbus_device::birq7_w ) { m_out_birq7_cb(state); }

// dma request from Qbus card
WRITE_LINE_MEMBER( qbus_device::bdmr_w ) { m_out_bdmr_cb(state); }

