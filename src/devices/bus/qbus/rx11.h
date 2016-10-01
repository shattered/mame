// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
#pragma once

#ifndef __RX11_H__
#define __RX11_H__

#include "emu.h"

#include "qbus.h"

#include "includes/pdp11.h"
#include "imagedev/flopdrv.h"


#define RX_NUMTR        77                              /* tracks/disk */
#define RX_M_TRACK      0377
#define RX_NUMSC        26                              /* sectors/track */
#define RX_M_SECTOR     0177
#define RX_NUMBY        128                             /* bytes/sector */
#define RX_SIZE         (RX_NUMTR * RX_NUMSC * RX_NUMBY)        /* bytes/disk */
#define RX_NUMDR        2                               /* drives/controller */
#define RX_M_NUMDR      01
#define UNIT_V_WLK      (UNIT_V_UF)                     /* write locked */
#define UNIT_WLK        (1u << UNIT_V_UF)
#define UNIT_WPRT       (UNIT_WLK | UNIT_RO)            /* write protect */

#define RXCS_V_FUNC     1                               /* function */
#define RXCS_M_FUNC     7
#define  RXCS_FILL      0                               /* fill buffer */
#define  RXCS_EMPTY     1                               /* empty buffer */
#define  RXCS_WRITE     2                               /* write sector */
#define  RXCS_READ      3                               /* read sector */
#define  RXCS_RXES      5                               /* read status */
#define  RXCS_WRDEL     6                               /* write del data */
#define  RXCS_ECODE     7                               /* read error code */
#define RXCS_V_DRV      4                               /* drive select */
#define RXCS_V_DONE     5                               /* done */
#define RXCS_V_IE       6                               /* intr enable */
#define RXCS_V_TR       7                               /* xfer request */
#define RXCS_V_INIT     14                              /* init */
#define RXCS_V_ERR      15                              /* error */
#define RXCS_FUNC       (RXCS_M_FUNC << RXCS_V_FUNC)
#define RXCS_DRV        (1u << RXCS_V_DRV)
#define RXCS_DONE       (1u << RXCS_V_DONE)
#define RXCS_IE         (1u << RXCS_V_IE)
#define RXCS_TR         (1u << RXCS_V_TR)
#define RXCS_INIT       (1u << RXCS_V_INIT)
#define RXCS_ERR        (1u << RXCS_V_ERR)
#define RXCS_RD         (RXCS_DONE+RXCS_IE+RXCS_TR+RXCS_ERR)
#define RXCS_WR         (CSR_GO+RXCS_FUNC+RXCS_DRV+RXCS_IE+RXCS_INIT)
#define RXCS_GETFNC(x)  (((x) >> RXCS_V_FUNC) & RXCS_M_FUNC)

#define RXES_CRC        0001                            /* CRC error */
#define RXES_PAR        0002                            /* parity error */
#define RXES_ID         0004                            /* init done */
#define RXES_WLK        0010                            /* write protect */
#define RXES_DD         0100                            /* deleted data */
#define RXES_DRDY       0200                            /* drive ready */

#define RXEC_0HOME		0010	// Drive 0 failed to see home on Initialize.
#define RXEC_1HOME		0020	// Drive 1 failed to see home on Initialize.
#define RXEC_HOMEOUT	0030	// Found home when stepping out 10 tracks for INIT.
#define RXEC_TRACK77	0040	// Tried to access a track greater than 77.
#define RXEC_SEEKERR	0050	// Home was found before desired track was reached.
#define RXEC_SELFDIAG	0060	// Self-diagnostic error.
#define RXEC_SECTERR	0070	// Desired sector could not be found after looking at 52 headers (2 revolutions).
#define RXEC_WP			0100	// Write protect error.
#define RXEC_SEPCLOCK	0110	// More than 40us and no SEP clock seen.
#define RXEC_PREAMBLE	0120	// A preamble could not be found.
#define RXEC_IOMARK		0130	// Preamble found but no I/O mark foudn within allowable time span.
#define RXEC_CRCHEAD	0140	// CRC error on what we thought was a header.
#define RXEC_TRACKADR	0150	// The header track address of a good header does not compare with the desired track.
#define RXEC_IDAM		0160	// Too many tries for an IDAM (identifies header).
#define RXEC_DAM		0170	// Data AM not found in allotted time.
#define RXEC_CRCDATA	0200	// CRC error on reading the sector from the disk. No code appears in the ERREG.
#define RXEC_PARITY		0210	// All parity errors.


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> rx11_device

class rx11_device : public device_t,
					public device_qbus_card_interface
{
public:
	rx11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	READ16_MEMBER(read);
	WRITE16_MEMBER(write);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;
	virtual machine_config_constructor device_mconfig_additions() const override;

	// device_z80daisy_interface overrides
	virtual int z80daisy_irq_state() override;
	virtual int z80daisy_irq_ack() override;
	virtual void z80daisy_irq_reti() override;

	enum {
		TIMER_ID_WAIT = 0,
		TIMER_ID_IDLE = 0,
		TIMER_ID_RESET,
		TIMER_ID_READ,
		TIMER_ID_WRITE,
		TIMER_ID_FILL,
		TIMER_ID_EMPTY,
		TIMER_ID_RXES,
		TIMER_ID_ECODE,
		TIMER_ID_SECTOR,
		TIMER_ID_TRACK,
	};

	void position_head();
	void read_sector();
	void write_sector(int ddam);

private:
	line_state	m_done;

	void command(int command);

	UINT16 m_rxcs;
	UINT16 m_rxdb;
	UINT16 m_rxta;
	UINT16 m_rxsa;
	UINT16 m_rxes[2];
	UINT16 m_rxec;

	legacy_floppy_image_device *m_image[2];
	std::unique_ptr<bool[]> m_ddam;
	std::unique_ptr<UINT8[]> m_buf;
	int m_count_read;
	int m_count_write;

	int m_unit;
	int m_state;

	int m_seektime;
	int m_waittime;

	emu_timer *m_timer_wait;
};


// device type definition
extern const device_type DEC_RX11;

#endif  /* __RX11_H__ */
