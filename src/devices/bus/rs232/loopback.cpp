// license:BSD-3-Clause
// copyright-holders:smf

#include "loopback.h"

const device_type RS232_LOOPBACK = &device_creator<rs232_loopback_device>;

rs232_loopback_device::rs232_loopback_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, RS232_LOOPBACK, "RS232 Loopback", tag, owner, clock, "rs232_loopback", __FILE__),
	device_rs232_port_interface(mconfig, *this),
	m_flow(*this, "FLOW_CONTROL")
{
}

static INPUT_PORTS_START(rs232_loopback)
	PORT_START("FLOW_CONTROL")
	PORT_CONFNAME(0x01, 0x00, "Flow Control")
	PORT_CONFSETTING(0x00, "Off")
	PORT_CONFSETTING(0x01, "On")
INPUT_PORTS_END

ioport_constructor rs232_loopback_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(rs232_loopback);
}

void rs232_loopback_device::device_start()
{
	{
		output_ri(0);
		output_cts(0);
		output_dsr(0);
		output_dcd(0);
	}
}

WRITE_LINE_MEMBER( rs232_loopback_device::input_txd )
{
	if (started())
	{
		output_rxd(state);
	}
}

WRITE_LINE_MEMBER( rs232_loopback_device::input_rts )
{
	if (started())
	{
		if (m_flow->read()) {
			output_ri(state);
			output_cts(state);
		} else {
			output_ri(0);
			output_cts(0);
		}
	}
}

WRITE_LINE_MEMBER( rs232_loopback_device::input_dtr )
{
	if (started())
	{
		if (m_flow->read()) {
			output_dsr(state);
			output_dcd(state);
		} else {
			output_dsr(0);
			output_dcd(0);
		}
	}
}
