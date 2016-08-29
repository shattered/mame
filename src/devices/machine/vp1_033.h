// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	K1801VP1-033 multifunction device

***************************************************************************/

#pragma once

#ifndef __VP1_033__
#define __VP1_033__

#include "emu.h"

#include "includes/pdp11.h"


#define PICCSR_REQB     0100000                         /* dataset int, RO */
#define PICCSR_REQA     0000200                         /* ring, RO */
#define PICCSR_IEA      0000100                         /* DSI ie, RW */
#define PICCSR_IEB      0000040                         /* sec xmt, RWNI */
#define PICCSR_CSR1     0000002                         /* RTS, RW */
#define PICCSR_CSR0     0000001                         /* DTR, RW */
#define PICCSR_RD       (PICCSR_REQB|PICCSR_REQA|PICCSR_IEA|PICCSR_IEB|PICCSR_CSR1|PICCSR_CSR0)
#define PICCSR_WR       (PICCSR_IEA|PICCSR_IEB|PICCSR_CSR1|PICCSR_CSR0)


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_VP1_033_VEC_A(_vec) \
	vp1_033_device::static_set_vec_a(*device, _vec);

#define MCFG_VP1_033_VEC_B(_vec) \
	vp1_033_device::static_set_vec_b(*device, _vec);

#define MCFG_VP1_033_PIC_CSR0_HANDLER(_write) \
	devcb = &vp1_033_device::set_pic_csr0_callback(*device, DEVCB_##_write);

#define MCFG_VP1_033_PIC_CSR1_HANDLER(_write) \
	devcb = &vp1_033_device::set_pic_csr1_callback(*device, DEVCB_##_write);

#define MCFG_VP1_033_PIC_OUT_HANDLER(_write) \
	devcb = &vp1_033_device::set_pic_out_callback(*device, DEVCB_##_write);



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> vp1_033_device

class vp1_033_device : public device_t,
					public device_serial_interface,
					public device_z80daisy_interface
{
public:
	// construction/destruction
	vp1_033_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	static void static_set_vec_a(device_t &device, int vec) { downcast<vp1_033_device &>(device).m_vec_a = vec; }
	static void static_set_vec_b(device_t &device, int vec) { downcast<vp1_033_device &>(device).m_vec_b = vec; }

	template<class _Object> static devcb_base &set_pic_csr0_callback(device_t &device, _Object object) { return downcast<vp1_033_device &>(device).m_write_pic_csr0.set_callback(object); }
	template<class _Object> static devcb_base &set_pic_csr1_callback(device_t &device, _Object object) { return downcast<vp1_033_device &>(device).m_write_pic_csr1.set_callback(object); }
	template<class _Object> static devcb_base &set_pic_out_callback(device_t &device, _Object object) { return downcast<vp1_033_device &>(device).m_write_pic_out.set_callback(object); }

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

private:
	devcb_write_line m_write_pic_csr0;
	devcb_write_line m_write_pic_csr1;
	devcb_write16 m_write_pic_out;

	line_state m_reqa;
	line_state m_reqb;

	int m_vec_a;
	int m_vec_b;

	UINT16 m_csr;
	UINT16 m_rbuf;
	UINT16 m_tbuf;
};


// device type definition
extern const device_type VP1_033;


#endif
