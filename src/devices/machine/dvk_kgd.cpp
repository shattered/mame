// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	CR bits:
	- 15 (W) 'graphics enable'
	- 14 (W) 'text disable'

***************************************************************************/

#include "dvk_kgd.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type DVK_KGD = &device_creator<dvk_kgd_device>;



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VERBOSE_DBG 1       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-24s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

enum
{
	REGISTER_CR = 0,
	REGISTER_DR,
	REGISTER_AR,
	REGISTER_CT
};

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  dvk_kgd_device - constructor
//-------------------------------------------------

dvk_kgd_device::dvk_kgd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, DVK_KGD, "DVK KGD", tag, owner, clock, "dvk_kgd", __FILE__)
	, m_screen(*this,"screen")
{
}


UINT32 dvk_kgd_device::draw_scanline(UINT16 *p, UINT16 offset)
{
	UINT8 gfx, fg, bg;
	UINT16 x;

	bg = 0; fg = 1;

	for (x = offset; x < offset + 50; x++)
	{
		gfx = m_videoram[x];

		*p++ = BIT(gfx, 0) ? fg : bg;
		*p++ = BIT(gfx, 0) ? fg : bg;
		*p++ = BIT(gfx, 1) ? fg : bg;
		*p++ = BIT(gfx, 1) ? fg : bg;
		*p++ = BIT(gfx, 2) ? fg : bg;
		*p++ = BIT(gfx, 2) ? fg : bg;
		*p++ = BIT(gfx, 3) ? fg : bg;
		*p++ = BIT(gfx, 3) ? fg : bg;
		*p++ = BIT(gfx, 4) ? fg : bg;
		*p++ = BIT(gfx, 4) ? fg : bg;
		*p++ = BIT(gfx, 5) ? fg : bg;
		*p++ = BIT(gfx, 5) ? fg : bg;
		*p++ = BIT(gfx, 6) ? fg : bg;
		*p++ = BIT(gfx, 6) ? fg : bg;
		*p++ = BIT(gfx, 7) ? fg : bg;
		*p++ = BIT(gfx, 7) ? fg : bg;
	}
	return 0;
}

TIMER_DEVICE_CALLBACK_MEMBER(dvk_kgd_device::scanline_callback)
{
	UINT16 y = m_screen->vpos();

	DBG_LOG(2,"scanline_cb",
		("frame %d x %.4d y %.3d\n", (int)m_screen->frame_number(), m_screen->hpos(), y));

	if (y < KGD_VERT_START) return;
	y -= KGD_VERT_START;
	if (y >= KGD_DISP_VERT) return;

	draw_scanline(&m_tmpbmp.pix16(y), y*50);
}

UINT32 dvk_kgd_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	copybitmap(bitmap, m_tmpbmp, 0, 0, KGD_HORZ_START, KGD_VERT_START, cliprect);
	return 0;
}



static GFXDECODE_START(kgd)
GFXDECODE_END

static MACHINE_CONFIG_FRAGMENT( dvk_kgd )
	MCFG_SCREEN_ADD_MONOCHROME("screen", RASTER, rgb_t::green)
	MCFG_SCREEN_UPDATE_DRIVER(dvk_kgd_device, screen_update)
	MCFG_SCREEN_RAW_PARAMS(XTAL_30_8MHz/2, KGD_TOTAL_HORZ, KGD_HORZ_START,
		KGD_HORZ_START+KGD_DISP_HORZ, KGD_TOTAL_VERT, KGD_VERT_START,
		KGD_VERT_START+KGD_DISP_VERT);

	MCFG_SCREEN_PALETTE("palette")

	MCFG_GFXDECODE_ADD("gfxdecode", "palette", kgd)
	MCFG_PALETTE_ADD_MONOCHROME("palette")

	MCFG_TIMER_DRIVER_ADD_SCANLINE("scantimer", dvk_kgd_device, scanline_callback, "kgd:screen", 0, 1)
MACHINE_CONFIG_END


machine_config_constructor dvk_kgd_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME(dvk_kgd);
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dvk_kgd_device::device_start()
{
	// save state
	save_item(NAME(m_cr));
	save_item(NAME(m_dr));
	save_item(NAME(m_ar));
	save_item(NAME(m_ct));

	m_videoram_base = std::make_unique<UINT8[]>(16384);
	m_videoram = m_videoram_base.get();

	m_tmpclip = rectangle(0, KGD_DISP_HORZ-1, 0, KGD_DISP_VERT-1);
	m_tmpbmp.allocate(KGD_DISP_HORZ, KGD_DISP_VERT);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void dvk_kgd_device::device_reset()
{
	m_cr = m_dr = m_ar = m_ct = 0;
}

//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ16_MEMBER( dvk_kgd_device::read )
{
	UINT16 data = 0;

	switch (offset & 0x03)
	{
	case REGISTER_CR:
		data = m_cr;
		break;

	case REGISTER_DR:
		data = m_videoram[m_ar];
		break;

	case REGISTER_AR:
		data = m_ar;
		break;

	case REGISTER_CT:
		data = m_ct;
		break;
	}

	if (offset == 0 || offset == 3)
	DBG_LOG(1,"DVK_KGD R", ("%d == %06o\n", offset, data));

	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE16_MEMBER( dvk_kgd_device::write )
{
	if (offset == 0 || offset == 3)
	DBG_LOG(1,"DVK_KGD W", ("%d <- %06o & %06o%s\n", offset, data, mem_mask, mem_mask==0xffff?"":" WARNING"));

	switch (offset & 0x03)
	{
	case REGISTER_CR:
		m_cr = ((m_cr & ~KGDCR_WR) | (data & KGDCR_WR));
		break;

	case REGISTER_DR:
		m_dr = ((m_dr & ~KGDDR_WR) | (data & KGDDR_WR));
		m_videoram[m_ar] = m_dr;
		break;

	case REGISTER_AR:
		m_ar = ((m_ar & ~KGDAR_WR) | (data & KGDAR_WR));
		break;

	case REGISTER_CT:
		m_ct = ((m_ct & ~KGDCT_WR) | (data & KGDCT_WR));
		break;
	}
}
