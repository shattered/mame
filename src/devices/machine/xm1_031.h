// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	KR1515XM1-031 multifunction device

***************************************************************************/

#pragma once

#ifndef __XM1_031__
#define __XM1_031__

#include "emu.h"

#include "includes/pdp11.h"


#define KBDCSR_RD       (CSR_IE|CSR_DONE)
#define KBDCSR_WR       (CSR_IE)

#define TMRCSR_MODE		0001
#define TMRCSR_FREQ		0006
#define TMRCSR_OVF		0010
#define TMRCSR_EIE		0020
#define TMRCSR_EVNT		0040
#define TMRCSR_RD       0377
#define TMRCSR_WR       (CSR_IE|TMRCSR_EIE|TMRCSR_FREQ|TMRCSR_MODE)

#define TMRBUF_RD		0007777
#define TMRBUF_WR		TMRBUF_RD


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/


#define MCFG_XM1_031_KBD_VIRQ_HANDLER(_write) \
	devcb = &xm1_031_kbd_device::set_virq_callback(*device, DEVCB_##_write);

#define MCFG_XM1_031_TIMER_VIRQ_HANDLER(_write) \
	devcb = &xm1_031_timer_device::set_virq_callback(*device, DEVCB_##_write);


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> xm1_031_kbd_device

class xm1_031_kbd_device : public device_t,
					public device_z80daisy_interface
{
public:
	// construction/destruction
	xm1_031_kbd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _Object> static devcb_base &set_virq_callback(device_t &device, _Object object) { return downcast<xm1_031_kbd_device &>(device).m_out_virq.set_callback(object); }

	virtual ioport_constructor device_input_ports() const override;

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

private:
	required_ioport_array<12> m_kbd;
	devcb_write_line m_out_virq;

	line_state m_rxrdy;
	UINT16 m_csr;
	UINT16 m_buf;
	std::unique_ptr<bool[]> m_keys;

	emu_timer *m_timer;
};


class xm1_031_timer_device : public device_t,
					public device_z80daisy_interface
{
public:
	// construction/destruction
	xm1_031_timer_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _Object> static devcb_base &set_virq_callback(device_t &device, _Object object) { return downcast<xm1_031_timer_device &>(device).m_out_virq.set_callback(object); }

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

private:
	devcb_write_line m_out_virq;

	line_state m_rxrdy;
	UINT16 m_csr;		// 177710
	UINT16 m_buf;		// 177712
	UINT16 m_value;		// 177714
	UINT16 m_counter;

	emu_timer *m_timer;
};


// device type definition
extern const device_type XM1_031_KBD;
extern const device_type XM1_031_TIMER;


#endif
