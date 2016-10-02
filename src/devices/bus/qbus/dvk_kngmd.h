// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
#pragma once

#ifndef __KNGM_H__
#define __KNGM_H__

#include "emu.h"

#include "qbus.h"

#include "includes/pdp11.h"
#include "imagedev/floppy.h"


#define MXCSR_DRVSE_L	0000002
#define MXCSR_V_DRIVE	2
#define MXCSR_M_DRIVE	3
#define MXCSR_STEP		0000020
#define MXCSR_DIR		0000040
#define MXCSR_MON		0000100
#define MXCSR_TIMER		0000200
#define MXCSR_ERR		0000400
#define MXCSR_INDEX		0001000
#define MXCSR_WP		0002000
#define MXCSR_TRK0		0004000
#define MXCSR_TOPHEAD	0010000
#define MXCSR_WRITE		0020000
#define MXCSR_GO		0040000
#define MXCSR_TR		0100000
#define MXCSR_DRIVE		(MXCSR_M_DRIVE << MXCSR_V_DRIVE)
#define MXCSR_GETDRIVE(x)  (((x) >> MXCSR_V_DRIVE) & MXCSR_M_DRIVE)

#define MXCSR_RD		0177776
#define MXCSR_WR		0070356


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> dvk_kngmd_device

class dvk_kngmd_device : public device_t,
					public device_qbus_card_interface
{
public:
	dvk_kngmd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_FLOPPY_FORMATS( floppy_formats );

	DECLARE_READ16_MEMBER(read);
	DECLARE_WRITE16_MEMBER(write);

	void index_callback(floppy_image_device *floppy, int state);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;
	virtual machine_config_constructor device_mconfig_additions() const override;

	enum {
		TIMER_ID_2KHZ = 0,
		TIMER_ID_WAIT
	};

	enum {
		MX_READ,
		MX_WRITE
	};

private:
	void command(int command);

	UINT16 m_mxcs;
	UINT16 m_rbuf;
	UINT16 m_wbuf;

	floppy_image_device *m_floppies[4];
	floppy_image_device *m_floppy;

	int m_unit;
	int m_state;

	int m_seektime;
	int m_waittime;

	emu_timer *m_timer_2khz;
	emu_timer *m_timer_wait;
};


// device type definition
extern const device_type DVK_KNGMD;

#endif  /* __KNGM_H__ */
