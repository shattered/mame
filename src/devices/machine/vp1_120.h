// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	K1801VP1-120 -- 3 parallel channels (2 bidir, 1 uni)

***************************************************************************/

#pragma once

#ifndef __VP1_120__
#define __VP1_120__

#include "emu.h"

#include "includes/pdp11.h"


#define SRCCSR_RD       (CSR_IE|CSR_DONE)
#define SRCCSR_WR       CSR_IE

#define PPTCSR_1DONE    0000020
#define PPTCSR_0DONE    0000010
#define PPTCSR_0UNMAP   0000004
#define PPTCSR_1IE      0000002
#define PPTCSR_0IE      0000001
#define PPTCSR_RD       (PPTCSR_1DONE|PPTCSR_0DONE|PPTCSR_0UNMAP|PPTCSR_1IE|PPTCSR_0IE)
#define PPTCSR_WR		(PPTCSR_0UNMAP|PPTCSR_1IE|PPTCSR_0IE)

#define PPRCSR_RIE      0000100
#define PPRCSR_2RDY     0000040
#define PPRCSR_1RDY     0000020
#define PPRCSR_0RDY     0000010
#define PPRCSR_2IE      0000004
#define PPRCSR_1IE      0000002
#define PPRCSR_0IE      0000001
#define PPRCSR_RD       (PPRCSR_RIE|PPRCSR_2RDY|PPRCSR_1RDY|PPRCSR_0RDY|PPRCSR_2IE|PPRCSR_1IE|PPRCSR_0IE)
#define PPRCSR_WR		(PPRCSR_RIE|PPRCSR_2IE|PPRCSR_1IE|PPRCSR_0IE)


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/


#define MCFG_VP1_120_VIRQ_HANDLER(_write) \
	devcb = &vp1_120_device::set_virq_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_CHANNEL0_OUT_HANDLER(_write) \
	devcb = &vp1_120_device::set_channel0_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_CHANNEL1_OUT_HANDLER(_write) \
	devcb = &vp1_120_device::set_channel1_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_CHANNEL2_OUT_HANDLER(_write) \
	devcb = &vp1_120_device::set_channel2_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_CHANNEL0_ACK_HANDLER(_write) \
	devcb = &vp1_120_device::set_channel0_ack_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_CHANNEL1_ACK_HANDLER(_write) \
	devcb = &vp1_120_device::set_channel1_ack_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_RESET_HANDLER(_write) \
	devcb = &vp1_120_device::set_reset_callback(*device, DEVCB_##_write);


#define MCFG_VP1_120_SUB_VIRQ_HANDLER(_write) \
	devcb = &vp1_120_sub_device::set_virq_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_SUB_CHANNEL0_OUT_HANDLER(_write) \
	devcb = &vp1_120_sub_device::set_channel0_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_SUB_CHANNEL1_OUT_HANDLER(_write) \
	devcb = &vp1_120_sub_device::set_channel1_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_SUB_CHANNEL0_ACK_HANDLER(_write) \
	devcb = &vp1_120_sub_device::set_channel0_ack_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_SUB_CHANNEL1_ACK_HANDLER(_write) \
	devcb = &vp1_120_sub_device::set_channel1_ack_callback(*device, DEVCB_##_write);

#define MCFG_VP1_120_SUB_CHANNEL2_ACK_HANDLER(_write) \
	devcb = &vp1_120_sub_device::set_channel2_ack_callback(*device, DEVCB_##_write);


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct {
	int channel;
	UINT16 rcsr;
	UINT16 rbuf;
	UINT16 tcsr;
	UINT16 tbuf;
	line_state rxrdy;
	line_state txrdy;
} channel_t;

typedef struct {
	UINT16 rbuf;
	UINT16 tbuf;
	line_state rxrdy;
	line_state txrdy;
} subchannel_t;


// ======================> vp1_120_device

class vp1_120_device : public device_t,
					public device_z80daisy_interface
{
public:
	// construction/destruction
	vp1_120_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _Object> static devcb_base &set_virq_callback(device_t &device, _Object object) { return downcast<vp1_120_device &>(device).m_out_virq.set_callback(object); }
	template<class _Object> static devcb_base &set_channel0_callback(device_t &device, _Object object) { return downcast<vp1_120_device &>(device).m_out_channel0.set_callback(object); }
	template<class _Object> static devcb_base &set_channel1_callback(device_t &device, _Object object) { return downcast<vp1_120_device &>(device).m_out_channel1.set_callback(object); }
	template<class _Object> static devcb_base &set_channel2_callback(device_t &device, _Object object) { return downcast<vp1_120_device &>(device).m_out_channel2.set_callback(object); }
	template<class _Object> static devcb_base &set_channel0_ack_callback(device_t &device, _Object object) { return downcast<vp1_120_device &>(device).m_ack_channel0.set_callback(object); }
	template<class _Object> static devcb_base &set_channel1_ack_callback(device_t &device, _Object object) { return downcast<vp1_120_device &>(device).m_ack_channel1.set_callback(object); }
	template<class _Object> static devcb_base &set_reset_callback(device_t &device, _Object object) { return downcast<vp1_120_device &>(device).m_out_reset.set_callback(object); }

	DECLARE_READ16_MEMBER( read0 );
	DECLARE_READ16_MEMBER( read1 );
	DECLARE_READ16_MEMBER( read2 );
	DECLARE_WRITE16_MEMBER( write0 );
	DECLARE_WRITE16_MEMBER( write1 );
	DECLARE_WRITE16_MEMBER( write2 );

	DECLARE_WRITE16_MEMBER( write_channel0 ) { m_channel[0].rbuf = data; m_channel[0].rcsr |= CSR_DONE; raise_virq(m_out_virq, m_channel[0].rcsr, CSR_IE, m_channel[0].rxrdy); }
	DECLARE_WRITE16_MEMBER( write_channel1 ) { m_channel[1].rbuf = data; m_channel[1].rcsr |= CSR_DONE; raise_virq(m_out_virq, m_channel[1].rcsr, CSR_IE, m_channel[1].rxrdy); }
	DECLARE_WRITE_LINE_MEMBER( ack_channel0 ) { m_channel[0].tcsr |= CSR_DONE; raise_virq(m_out_virq, m_channel[0].tcsr, CSR_IE, m_channel[0].txrdy); }
	DECLARE_WRITE_LINE_MEMBER( ack_channel1 ) { m_channel[1].tcsr |= CSR_DONE; raise_virq(m_out_virq, m_channel[1].tcsr, CSR_IE, m_channel[1].txrdy); }
	DECLARE_WRITE_LINE_MEMBER( ack_channel2 ) { m_channel[2].tcsr |= CSR_DONE; raise_virq(m_out_virq, m_channel[2].tcsr, CSR_IE, m_channel[2].txrdy); }

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

	UINT16 channel_read(channel_t *channel, offs_t offset);
	void channel_write(channel_t *channel, offs_t offset, UINT16 data, UINT16 mem_mask);

private:
	devcb_write_line m_out_virq;
	devcb_write16 m_out_channel0;
	devcb_write16 m_out_channel1;
	devcb_write16 m_out_channel2;
	devcb_write_line m_ack_channel0;
	devcb_write_line m_ack_channel1;
	devcb_write_line m_out_reset;

	channel_t m_channel[3];
};


class vp1_120_sub_device : public device_t,
					public device_z80daisy_interface
{
public:
	// construction/destruction
	vp1_120_sub_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _Object> static devcb_base &set_virq_callback(device_t &device, _Object object) { return downcast<vp1_120_sub_device &>(device).m_out_virq.set_callback(object); }
	template<class _Object> static devcb_base &set_channel0_callback(device_t &device, _Object object) { return downcast<vp1_120_sub_device &>(device).m_out_channel0.set_callback(object); }
	template<class _Object> static devcb_base &set_channel1_callback(device_t &device, _Object object) { return downcast<vp1_120_sub_device &>(device).m_out_channel1.set_callback(object); }
	template<class _Object> static devcb_base &set_channel0_ack_callback(device_t &device, _Object object) { return downcast<vp1_120_sub_device &>(device).m_ack_channel0.set_callback(object); }
	template<class _Object> static devcb_base &set_channel1_ack_callback(device_t &device, _Object object) { return downcast<vp1_120_sub_device &>(device).m_ack_channel1.set_callback(object); }
	template<class _Object> static devcb_base &set_channel2_ack_callback(device_t &device, _Object object) { return downcast<vp1_120_sub_device &>(device).m_ack_channel2.set_callback(object); }

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

	DECLARE_WRITE16_MEMBER( write_channel0 ) { logerror("ch 0 rbuf %06o rcsr %06o\n", data, m_rcsr); m_channel[0].rbuf = data; m_rcsr |= PPRCSR_0RDY; raise_virq (m_out_virq, m_rcsr, PPRCSR_0IE, m_channel[0].rxrdy); }
	DECLARE_WRITE16_MEMBER( write_channel1 ) { logerror("ch 1 rbuf %06o rcsr %06o\n", data, m_rcsr); m_channel[1].rbuf = data; m_rcsr |= PPRCSR_1RDY; raise_virq (m_out_virq, m_rcsr, PPRCSR_1IE, m_channel[1].rxrdy); }
	DECLARE_WRITE16_MEMBER( write_channel2 ) { logerror("ch 2 rbuf %06o rcsr %06o\n", data, m_rcsr); m_channel[2].rbuf = data; m_rcsr |= PPRCSR_2RDY; raise_virq (m_out_virq, m_rcsr, PPRCSR_2IE, m_channel[2].rxrdy); }
	DECLARE_WRITE_LINE_MEMBER( ack_channel0 ) { m_tcsr |= PPTCSR_0DONE; raise_virq (m_out_virq, m_tcsr, PPTCSR_0IE, m_channel[0].txrdy); }
	DECLARE_WRITE_LINE_MEMBER( ack_channel1 ) { m_tcsr |= PPTCSR_1DONE; raise_virq (m_out_virq, m_tcsr, PPTCSR_1IE, m_channel[1].txrdy); }
	DECLARE_WRITE_LINE_MEMBER( write_reset ) { raise_virq(m_out_virq, m_rcsr, PPRCSR_RIE, m_reset); }

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

private:
	devcb_write_line m_out_virq;
	devcb_write16 m_out_channel0;
	devcb_write16 m_out_channel1;
	devcb_write_line m_ack_channel0;
	devcb_write_line m_ack_channel1;
	devcb_write_line m_ack_channel2;

	line_state m_reset;

	subchannel_t m_channel[3];

	UINT16 m_rcsr;
	UINT16 m_tcsr;
};


// device type definition
extern const device_type VP1_120;
extern const device_type VP1_120_SUB;


#endif
