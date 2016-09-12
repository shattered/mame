// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	K1801VP1-128 floppy controller, follow-up and compatible to VP1-097.

***************************************************************************/

#pragma once

#ifndef __VP1_128__
#define __VP1_128__

#include "emu.h"

#include "includes/pdp11.h"


// write bits
#define FLPCSR_REZ      0002000
#define FLPCSR_WM       0001000
#define FLPCSR_GOR      0000400
#define FLPCSR_ST       0000200
#define FLPCSR_DIR      0000100
#define FLPCSR_HS       0000040
#define FLPCSR_MSW      0000020
#define FLPCSR_DS3      0000010
#define FLPCSR_DS2      0000004
#define FLPCSR_DS1      0000002
#define FLPCSR_DS0      0000001

// read bits
#define FLPCSR_IND      0100000
#define FLPCSR_CRC      0040000
#define FLPCSR_WRP      0000004
#define FLPCSR_RDY      0000002
#define FLPCSR_TR0      0000001

#define FLPCSR_RD       (FLPCSR_IND|FLPCSR_CRC|CSR_DONE|FLPCSR_WRP|FLPCSR_RDY|FLPCSR_TR0)
#define FLPCSR_WR       0003777


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> vp1_128_device

class vp1_128_device : public device_t,
					public device_z80daisy_interface
{
public:
	// construction/destruction
	vp1_128_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

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
	UINT16 m_csr;
	UINT16 m_rbuf;
	UINT16 m_tbuf;
};


// device type definition
extern const device_type VP1_128;


#endif
