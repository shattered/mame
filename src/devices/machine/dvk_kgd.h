// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	DVK KGD

***************************************************************************/

#pragma once

#ifndef __DVK_KGD__
#define __DVK_KGD__

#include "emu.h"

#include "includes/pdp11.h"
#include "machine/ie15.h"

/* registers */

#define KGDCR_WR		0140000
#define KGDDR_WR		0000377
#define KGDAR_WR		0037777
#define KGDCT_WR		0037777


#define KGD_TOTAL_HORZ 1000
#define KGD_DISP_HORZ  800
#define KGD_HORZ_START 200

#define KGD_TOTAL_VERT 11*28
#define KGD_DISP_VERT  11*26
#define KGD_VERT_START 11


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> dvk_kgd_device

class dvk_kgd_device : public device_t
{
public:
	dvk_kgd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER( scanline_callback );

	virtual machine_config_constructor device_mconfig_additions() const override;

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	UINT32 draw_scanline(UINT16 *p, UINT16 offset);
//	UINT32 mix_scanline(UINT16 *p, UINT16 offset);
	rectangle m_tmpclip;
	bitmap_ind16 m_tmpbmp;
	bitmap_ind16 m_offbmp;
	bitmap_ind16 *m_bitmap_ie15;

	std::unique_ptr<UINT8[]> m_videoram_base;
	UINT8 *m_videoram;

	UINT16 m_cr;
	UINT16 m_dr;
	UINT16 m_ar;
	UINT16 m_ct;

protected:
	required_device<screen_device> m_screen;
};


// device type definition
extern const device_type DVK_KGD;


#endif
