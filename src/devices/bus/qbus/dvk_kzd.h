// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
#pragma once

#ifndef __DVK_KZD_H__
#define __DVK_KZD_H__

#include "emu.h"

#include "qbus.h"

#include "includes/pdp11.h"
#include "imagedev/harddriv.h"


#define KZDERR_DM		0000400
#define KZDERR_TRK0		0001000
#define KZDERR_FAULT	0002000
#define KZDERR_AM		0010000
#define KZDERR_ACRC		0020000
#define KZDERR_DCRC		0040000

#define KZDERR_RD		0177400
#define KZDERR_WR		0000377

#define KZDSEC_RDWR		0177437
#define KZDCYL_RDWR		0001777
#define KZDHED_RDWR		0000007

#define KZDCMD_TRK0		0020
#define KZDCMD_READ		0040
#define KZDCMD_WRITE	0060
#define KZDCMD_FORMAT	0120

#define KZDCSR_ERR		0000400
#define KZDCSR_DRQ2		0004000
#define KZDCSR_DONE		0010000
#define KZDCSR_WERR		0020000
#define KZDCSR_DRDY		0040000

#define KZDCSR_RD		0177400
#define KZDCSR_WR		0000377

#define KZDSI_DONE		0000001
#define KZDSI_INIT		0000010
#define KZDSI_IE		CSR_IE
#define KZDSI_DRQ1		CSR_DONE
#define KZDSI_SLOW		0000400
#define KZDSI_BUSY		0100000

#define KZDSI_RD		0100711
#define KZDSI_WR		0000510


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> dvk_kzd_device

class dvk_kzd_device : public device_t,
					public device_qbus_card_interface
{
public:
	dvk_kzd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	READ16_MEMBER(read);
	WRITE16_MEMBER(write);

	DECLARE_DEVICE_IMAGE_LOAD_MEMBER(dvk_kzd_hd);
	DECLARE_DEVICE_IMAGE_UNLOAD_MEMBER(dvk_kzd_hd);

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
		TIMER_ID_TRK0,
		TIMER_ID_READ,
		TIMER_ID_WRITE,
		TIMER_ID_WAIT,
	};

private:
	line_state	m_zoa;
	line_state	m_zob;

	void command(int command);
	int sector_to_lba(unsigned int cylinder, unsigned int head, unsigned int sector, UINT32 *lba);

	hard_disk_file *m_hd_file;
	hard_disk_info *m_hd_geom;

	UINT16 m_regs[9];
	std::unique_ptr<UINT16[]> m_buf;
	int m_count_read;
	int m_count_write;

	int m_track;
	int m_seektime;
	int m_waittime;

	emu_timer *m_timer_seek;
	emu_timer *m_timer_read;
	emu_timer *m_timer_write;
	emu_timer *m_timer_wait;
};


// device type definition
extern const device_type DVK_KZD;

#endif  /* __DVK_KZD_H__ */
