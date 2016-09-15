// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	DEC PC11 paper tape reader and punch controller

***************************************************************************/

#pragma once

#ifndef __PC11__
#define __PC11__

#include "emu.h"

#include "qbus.h"

#include "includes/pdp11.h"


#define PTRCSR_IMP      (CSR_ERR+CSR_BUSY+CSR_DONE+CSR_IE) /* paper tape reader */
#define PTRCSR_WR       (CSR_IE)
#define PTPCSR_IMP      (CSR_ERR + CSR_DONE + CSR_IE)   /* paper tape punch */
#define PTPCSR_WR       (CSR_IE)


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_PC11_RXVEC(_vec) \
	pc11_device::static_set_rxvec(*device, _vec);

#define MCFG_PC11_TXVEC(_vec) \
	pc11_device::static_set_txvec(*device, _vec);



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> pc11_device

class pc11_device : public device_t,
					public device_image_interface,
					public device_qbus_card_interface
{
public:
	// construction/destruction
	pc11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	static void static_set_rxvec(device_t &device, int vec) { downcast<pc11_device &>(device).m_rxvec = vec; }
	static void static_set_txvec(device_t &device, int vec) { downcast<pc11_device &>(device).m_txvec = vec; }

	// image-level overrides
	virtual iodevice_t image_type() const override { return IO_PUNCHTAPE; }

	virtual bool is_readable()  const override { return 1; }
	virtual bool is_writeable() const override { return 0; }
	virtual bool is_creatable() const override { return 0; }
	virtual bool must_be_loaded() const override { return 0; }
	virtual bool is_reset_on_load() const override { return 0; }
	virtual const char *image_interface() const override { return nullptr; }
	virtual const char *file_extensions() const override { return "bin,bim,lda"; }

	virtual image_init_result call_load() override;
	virtual void call_unload() override;

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
	// device-level overrides
	virtual void device_config_complete() override { update_names(); }
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

	// device_qbus_card_interface overrides
	virtual void biaki_w(int state) override { }

private:
	device_image_interface	*m_fd;

	line_state	m_rxrdy;
	line_state	m_txrdy;

	int m_rxvec;
	int m_txvec;

	UINT16 m_rcsr;
	UINT16 m_rbuf;
	UINT16 m_tcsr;
	UINT16 m_tbuf;

	const char* pc11_regnames[4];
};


// device type definition
extern const device_type DEC_PC11;


#endif
