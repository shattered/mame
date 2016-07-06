// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	DEC DL11-type SLU

***************************************************************************/

#pragma once

#ifndef __DL11__
#define __DL11__

#include "emu.h"

#include "includes/pdp11.h"

/*
 * taken from PDP11/pdp11_dl.c in SIMH
 */

/* registers */

#define DLICSR_DSI      0100000                         /* dataset int, RO */
#define DLICSR_RNG      0040000                         /* ring, RO */
#define DLICSR_CTS      0020000                         /* CTS, RO */
#define DLICSR_CDT      0010000                         /* CDT, RO */
#define DLICSR_SEC      0002000                         /* sec rcv, RONI */
#define DLICSR_DSIE     0000040                         /* DSI ie, RW */
#define DLICSR_SECX     0000010                         /* sec xmt, RWNI */
#define DLICSR_RTS      0000004                         /* RTS, RW */
#define DLICSR_DTR      0000002                         /* DTR, RW */
#define DLICSR_RD       (CSR_DONE|CSR_IE)               /* DL11C */
#define DLICSR_WR       (CSR_IE)
#define DLICSR_RD_M     (DLICSR_DSI|DLICSR_RNG|DLICSR_CTS|DLICSR_CDT|DLICSR_SEC| \
                         CSR_DONE|CSR_IE|DLICSR_DSIE|DLICSR_SECX|DLICSR_RTS|DLICSR_DTR)
#define DLICSR_WR_M     (CSR_IE|DLICSR_DSIE|DLICSR_SECX|DLICSR_RTS|DLICSR_DTR)
#define DLIBUF_ERR      0100000
#define DLIBUF_OVR      0040000
#define DLIBUF_RBRK     0020000
#define DLIBUF_RD       (DLIBUF_ERR|DLIBUF_OVR|DLIBUF_RBRK|0377)
#define DLOCSR_MNT      0000004                         /* maint, RWNI */
#define DLOCSR_XBR      0000001                         /* xmit brk, RWNI */
#define DLOCSR_RD       (CSR_DONE|CSR_IE|DLOCSR_MNT|DLOCSR_XBR)
#define DLOCSR_WR       (CSR_IE|DLOCSR_MNT|DLOCSR_XBR)


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_DL11_RXC(_clock) \
	dl11_device::static_set_rxc(*device, _clock);

#define MCFG_DL11_TXC(_clock) \
	dl11_device::static_set_txc(*device, _clock);

#define MCFG_DL11_RXVEC(_vec) \
	dl11_device::static_set_rxvec(*device, _vec);

#define MCFG_DL11_TXVEC(_vec) \
	dl11_device::static_set_txvec(*device, _vec);

#define MCFG_DL11_TXD_HANDLER(_write) \
	devcb = &dl11_device::set_txd_callback(*device, DEVCB_##_write);

#define MCFG_DL11_RTS_HANDLER(_write) \
	devcb = &dl11_device::set_rts_callback(*device, DEVCB_##_write);

#define MCFG_DL11_RXRDY_HANDLER(_write) \
	devcb = &dl11_device::set_rxrdy_callback(*device, DEVCB_##_write);

#define MCFG_DL11_TXRDY_HANDLER(_write) \
	devcb = &dl11_device::set_txrdy_callback(*device, DEVCB_##_write);



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> dl11_device

class dl11_device : public device_t,
					public device_serial_interface,
					public device_z80daisy_interface
{
public:
	// construction/destruction
	dl11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual ioport_constructor device_input_ports() const override;

	static void static_set_rxc(device_t &device, int clock) { downcast<dl11_device &>(device).m_rxc = clock; }
	static void static_set_txc(device_t &device, int clock) { downcast<dl11_device &>(device).m_txc = clock; }
	static void static_set_rxvec(device_t &device, int vec) { downcast<dl11_device &>(device).m_rxvec = vec; }
	static void static_set_txvec(device_t &device, int vec) { downcast<dl11_device &>(device).m_txvec = vec; }

	template<class _Object> static devcb_base &set_txd_callback(device_t &device, _Object object) { return downcast<dl11_device &>(device).m_write_txd.set_callback(object); }
	template<class _Object> static devcb_base &set_rts_callback(device_t &device, _Object object) { return downcast<dl11_device &>(device).m_write_rts.set_callback(object); }
	template<class _Object> static devcb_base &set_rxrdy_callback(device_t &device, _Object object) { return downcast<dl11_device &>(device).m_write_rxrdy.set_callback(object); }
	template<class _Object> static devcb_base &set_txrdy_callback(device_t &device, _Object object) { return downcast<dl11_device &>(device).m_write_txrdy.set_callback(object); }

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

	DECLARE_READ_LINE_MEMBER( rxrdy_r );
	DECLARE_READ_LINE_MEMBER( txrdy_r );

	DECLARE_WRITE_LINE_MEMBER( rx_w ) { device_serial_interface::rx_w(state); }

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

	// device_serial_interface overrides
	virtual void tra_callback() override;
	virtual void tra_complete() override;
	virtual void rcv_complete() override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

private:
	devcb_write_line   m_write_txd;
	devcb_write_line   m_write_rts;
	devcb_write_line   m_write_rxrdy;
	devcb_write_line   m_write_txrdy;

	line_state	m_rxrdy;
	line_state	m_txrdy;

	int m_rxc;
	int m_txc;
	int m_rxvec;
	int m_txvec;

	UINT16 m_rcsr;
	UINT16 m_rbuf;
	UINT16 m_tcsr;
	UINT16 m_tbuf;

	const char* dl11_regnames[4];
};


// device type definition
extern const device_type DL11;


#endif
