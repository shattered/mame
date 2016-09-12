// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, Sergey Svishchev
/***************************************************************************

	UKNC

	to do:

	. verify all read-/write-only bits, set/reset status at poweron and RESET
		SD/RD -- set/reset at poweron only (-> device_start)
		SIN/RIN -- set/reset at poweron and RESET (-> device_reset)
	- verify all byte-wide writes
	+ maincpu RESET -> subcpu vector 314 (177066 bit 6)
	+ "sram" access via vram registers
	+ video: graphical cursor
	. video: verify all color setup incl. cursor
	- video: cut off 18 scanlines at the top
	+ carts: ROM only, then full devices (RAM disk, IDE controller)
	. keyboard via xm1_031.cpp + debug stuck input
	. timer via xm1_031.cpp
	- "index" line (bit 3 of scsr)
	- "external event" (cassette data in, etc.) -> subcpu vector 310
	- sound ?? via xm1_031.cpp
	. cassette tape
	. parallel port (i8255), 12 out (PA, PC4-PC7), 12 in (PB, PC0-PC3)
	- floppy
	- network adapter 
	- break detection in dl11.cpp

	maybe:

	- channel 0 unmapping (177076 bit 2)
	- breakpoint registers
	- rom replacement with ram (useful only for debugging)
	+ maincpu power off via DCLO/ACLO
	- find older ROM dump

	swre

	+ built-in test
	- ODT command 'T' (network adapter test)
	- testuk.bin ROM cart
	- FTMON tests ??

****************************************************************************/

#include "emu.h"

#include "bus/centronics/ctronics.h"
#include "bus/qbus/qbus.h"
#include "bus/qbus/mpi_cards.h"
#include "bus/rs232/rs232.h"
#include "cpu/t11/t11.h"
#include "imagedev/cassette.h"
#include "machine/bankdev.h"
#include "machine/dl11.h"
#include "machine/i8255.h"
#include "machine/vp1_120.h"
#include "machine/xm1_031.h"
#include "sound/speaker.h"


#define UKNC_TOTAL_HORZ 800
#define UKNC_DISP_HORZ  640
#define UKNC_HORZ_START 80	// XXX verify

#define UKNC_TOTAL_VERT 312
#define UKNC_DISP_VERT  288+18
#define UKNC_VERT_START 0	// XXX verify


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


typedef struct {
	UINT16 addr;
	bool cursor;
	UINT32 aux;
	bool color;
	bool control;
	bool cursor_type;
	int cursor_color;
	int cursor_octet;
	int cursor_pixel;
} scanline_t;


class uknc_state : public driver_device
{
public:
	uknc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_subcpu(*this, "subcpu")
		, m_bankdev0(*this, "bankdev0")
		, m_bankdev1(*this, "bankdev1")
		, m_bankdev2(*this, "bankdev2")
		, m_dl11com(*this, "dl11com")
		, m_rs232(*this, "rs232")
		, m_cassette(*this, "cassette")
		, m_centronics(*this, "centronics")
		, m_subqbus(*this, "subqbus")
		, m_palette(*this, "palette")
		, m_screen(*this, "screen")
	{ }

	virtual void machine_start() override;
	virtual void machine_reset() override;
	virtual void video_start() override;

	enum
	{
		TIMER_ID_VSYNC,
	};
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;
	emu_timer *m_vsync_timer;

	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(scanline_callback);
	DECLARE_PALETTE_INIT(uknc);

	DECLARE_WRITE16_MEMBER(odt_main_w);

	DECLARE_WRITE16_MEMBER(lights);

	DECLARE_READ16_MEMBER(bcsr_r);
	DECLARE_READ16_MEMBER(scsr_r);
	DECLARE_WRITE16_MEMBER(bcsr_w);
	DECLARE_WRITE16_MEMBER(scsr_w);

	DECLARE_READ16_MEMBER(vram_addr_r);
	DECLARE_READ16_MEMBER(vram_data_p0_r);
	DECLARE_READ16_MEMBER(vram_data_p12_r);
	DECLARE_READ16_MEMBER(sprite_r);
	DECLARE_WRITE16_MEMBER(vram_addr_w);
	DECLARE_WRITE16_MEMBER(vram_data_p0_w);
	DECLARE_WRITE16_MEMBER(vram_data_p12_w);
	DECLARE_WRITE16_MEMBER(sprite_w);

	DECLARE_READ16_MEMBER(maincpu_vram_addr_r);
	DECLARE_READ16_MEMBER(maincpu_vram_data_r);
	DECLARE_WRITE16_MEMBER(maincpu_vram_addr_w);
	DECLARE_WRITE16_MEMBER(maincpu_vram_data_w);

private:
	int m_odt_main;
	int m_odt_sub;

	void draw_scanline(UINT32 *p, scanline_t *scanline);
	void update_scanlines();
	void set_palette_yrgb(UINT32 aux);
	void set_palette_ygrb(UINT32 aux);
	void set_palette__rgb(UINT32 aux);
	void set_palette_gray(UINT32 aux);
	bitmap_rgb32 m_tmpbmp;

	// maincpu registers

	// subcpu registers
	UINT16 m_bcsr;	// 177054
	UINT16 m_scsr;	// 177716

	// video
	struct {
		UINT16 vram_addr;
		UINT16 vram_data_p0;
		UINT16 vram_data_p12;
		UINT16 maincpu_vram_addr;
		UINT16 maincpu_vram_data;
		UINT8 foreground;	// 177016
		UINT32 background;	// 177020
		UINT8 sprite_mask;	// 177024
		UINT8 plane_mask;	// 177026
		UINT32 control;
	} m_video;
	std::unique_ptr<UINT8[]> m_videoram;
	std::unique_ptr<scanline_t[]> m_scanlines;

protected:
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;
	required_device<address_map_bank_device> m_bankdev0;
	required_device<address_map_bank_device> m_bankdev1;
	required_device<address_map_bank_device> m_bankdev2;
	optional_device<dl11_device> m_dl11com;
	optional_device<rs232_port_device> m_rs232;
	required_device<cassette_image_device> m_cassette;
	required_device<centronics_device> m_centronics;
	required_device<qbus_device> m_subqbus;
	required_device<palette_device> m_palette;
	required_device<screen_device> m_screen;
};


static ADDRESS_MAP_START( uknc_mem, AS_PROGRAM, 16, uknc_state )
	AM_RANGE (0000000, 0177777) AM_DEVREADWRITE("bankdev0", address_map_bank_device, read16, write16)
ADDRESS_MAP_END

static ADDRESS_MAP_START(uknc_mem_banked, AS_PROGRAM, 16, uknc_state)
// USER mode
	AM_RANGE (0000000, 0157777) AM_MIRROR(0200000) AM_RAM
//	AM_RANGE (0176560, 0176567) AM_DEVREADWRITE("dl11lan", dl11_device, read, write)	// network
	AM_RANGE (0176570, 0176577) AM_DEVREADWRITE("dl11com", dl11_device, read, write)	// serial port
	AM_RANGE (0176640, 0176641) AM_READWRITE(maincpu_vram_addr_r, maincpu_vram_addr_w)
	AM_RANGE (0176642, 0176643) AM_READWRITE(maincpu_vram_data_r, maincpu_vram_data_w)
//	AM_RANGE (0176644, 0176647) AM_UNMAP	// hardware breakpoint
	AM_RANGE (0176660, 0176667) AM_DEVREADWRITE("channel", vp1_120_device, read1, write1)	// subcpu channel 1
	AM_RANGE (0176670, 0176677) AM_DEVREADWRITE("channel", vp1_120_device, read2, write2)	// subcpu channel 2
	AM_RANGE (0177560, 0177567) AM_DEVREADWRITE("channel", vp1_120_device, read0, write0)	// subcpu channel 0

// HALT mode
	AM_RANGE (0360000, 0377777) AM_RAM AM_SHARE("sram")
ADDRESS_MAP_END

static ADDRESS_MAP_START(uknc_sub_mem, AS_PROGRAM, 16, uknc_state)
	AM_RANGE (0000000, 0077777) AM_RAM
	AM_RANGE (0100000, 0117777) AM_DEVREADWRITE("bankdev1", address_map_bank_device, read16, write16)
	AM_RANGE (0120000, 0176777) AM_DEVREADWRITE("bankdev2", address_map_bank_device, read16, write16)
	AM_RANGE (0177010, 0177011) AM_READWRITE(vram_addr_r, vram_addr_w)
	AM_RANGE (0177012, 0177013) AM_READWRITE(vram_data_p0_r, vram_data_p0_w)
	AM_RANGE (0177014, 0177015) AM_READWRITE(vram_data_p12_r, vram_data_p12_w)
	AM_RANGE (0177016, 0177027) AM_READWRITE(sprite_r, sprite_w)
	AM_RANGE (0177054, 0177055) AM_READWRITE(bcsr_r, bcsr_w)
	AM_RANGE (0177060, 0177077) AM_DEVREADWRITE("subchan", vp1_120_sub_device, read, write)
	AM_RANGE (0177100, 0177103) AM_DEVREADWRITE8("ppi8255", i8255_device, read, write, 0xffff)
	AM_RANGE (0177700, 0177707) AM_DEVREADWRITE("keyboard", xm1_031_kbd_device, read, write)
	AM_RANGE (0177710, 0177715) AM_DEVREADWRITE("timer", xm1_031_timer_device, read, write)
	AM_RANGE (0177716, 0177717) AM_READWRITE(scsr_r, scsr_w)
ADDRESS_MAP_END

/*
 *	0 = value of bit 0 in BCSR 
 *	  0		mapping at 100000: 0 - read from VRAM plane 0, 1 - read from ROM (reset to 1)
 *	1 = value of bit 4 in BCSR (ROM banks replacement: 0 - writes ignored, 1 - writes go to VRAM plane 0)
 *	  4		100000..117777
 */
static ADDRESS_MAP_START( uknc_cart_mem, AS_PROGRAM, 16, uknc_state )
// map 0
	AM_RANGE (0000000, 0017777) AM_READ_BANK("plane0bank4") //AM_WRITENOP
// map 1 (default) -- read from system ROM, writes ignored
	AM_RANGE (0100000, 0117777) AM_ROM AM_REGION("subcpu", 0) //AM_WRITENOP
// map 2
	AM_RANGE (0200000, 0217777) AM_RAMBANK("plane0bank4")
// map 3
	AM_RANGE (0300000, 0317777) AM_ROM AM_REGION("subcpu", 0) AM_WRITE_BANK("plane0bank4")
ADDRESS_MAP_END

/*
 *	0-2	= value of bits 5-7 in BCSR (ROM banks replacement: 0 - writes ignored, 1 - writes go to VRAM plane 0)
 *	  5		120000..137777
 *	  6		140000..157777
 *	  7		160000..177777
 */
static ADDRESS_MAP_START( uknc_rom_mem, AS_PROGRAM, 16, uknc_state )
// map 0: all ROM, no VRAM
	AM_RANGE (0000000, 0057777) AM_ROM AM_REGION("subcpu", 020000)
// map 1:
	AM_RANGE (0100000, 0117777) AM_ROM AM_REGION("subcpu", 020000) AM_WRITE_BANK("plane0bank5")
	AM_RANGE (0120000, 0157777) AM_ROM AM_REGION("subcpu", 040000)
// map 2:
	AM_RANGE (0200000, 0217777) AM_ROM AM_REGION("subcpu", 020000)
	AM_RANGE (0220000, 0237777) AM_ROM AM_REGION("subcpu", 040000) AM_WRITE_BANK("plane0bank6")
	AM_RANGE (0240000, 0257777) AM_ROM AM_REGION("subcpu", 060000)
// map 3:
	AM_RANGE (0300000, 0317777) AM_ROM AM_REGION("subcpu", 020000) AM_WRITE_BANK("plane0bank5")
	AM_RANGE (0320000, 0337777) AM_ROM AM_REGION("subcpu", 040000) AM_WRITE_BANK("plane0bank6")
	AM_RANGE (0340000, 0357777) AM_ROM AM_REGION("subcpu", 060000)
// map 4:
	AM_RANGE (0400000, 0437777) AM_ROM AM_REGION("subcpu", 020000)
	AM_RANGE (0440000, 0457777) AM_ROM AM_REGION("subcpu", 060000) AM_WRITE_BANK("plane0bank7")
// map 5:
	AM_RANGE (0500000, 0517777) AM_ROM AM_REGION("subcpu", 020000) AM_WRITE_BANK("plane0bank5")
	AM_RANGE (0520000, 0537777) AM_ROM AM_REGION("subcpu", 040000)
	AM_RANGE (0540000, 0557777) AM_ROM AM_REGION("subcpu", 060000) AM_WRITE_BANK("plane0bank7")
// map 6:
	AM_RANGE (0600000, 0617777) AM_ROM AM_REGION("subcpu", 020000)
	AM_RANGE (0620000, 0637777) AM_ROM AM_REGION("subcpu", 040000) AM_WRITE_BANK("plane0bank6")
	AM_RANGE (0640000, 0657777) AM_ROM AM_REGION("subcpu", 060000) AM_WRITE_BANK("plane0bank7")
// map 7:
	AM_RANGE (0700000, 0717777) AM_ROM AM_REGION("subcpu", 020000) AM_WRITE_BANK("plane0bank5")
	AM_RANGE (0720000, 0737777) AM_ROM AM_REGION("subcpu", 040000) AM_WRITE_BANK("plane0bank6")
	AM_RANGE (0740000, 0757777) AM_ROM AM_REGION("subcpu", 060000) AM_WRITE_BANK("plane0bank7")
ADDRESS_MAP_END


static INPUT_PORTS_START( uknc )
	PORT_START("YRGB")
	PORT_DIPNAME(0x03, 0x00, "variant")
	PORT_DIPSETTING(0x00, "Color, YRGB")
	PORT_DIPSETTING(0x01, "Color, YGRB")
	PORT_DIPSETTING(0x02, "Color, xRGB")
	PORT_DIPSETTING(0x03, "Grayscale")
INPUT_PORTS_END


#define YRGB(rgb, y) ((rgb) ? ((y) ? 255 : 128) : 0)

void uknc_state::set_palette_yrgb(UINT32 aux)
{
	for (int i = 0; i < 8; i++)
	{
		m_palette->set_pen_color(i, rgb_t(
			YRGB(BIT(aux,i*4+2), BIT(aux,i*4+3)),
			YRGB(BIT(aux,i*4+1), BIT(aux,i*4+3)),
			YRGB(BIT(aux,i*4  ), BIT(aux,i*4+3)))
		);
	}
}

void uknc_state::set_palette_ygrb(UINT32 aux)
{
	for (int i = 0; i < 8; i++)
	{
		m_palette->set_pen_color(i, rgb_t(
			YRGB(BIT(aux,i*4+1), BIT(aux,i*4+3)),
			YRGB(BIT(aux,i*4+2), BIT(aux,i*4+3)),
			YRGB(BIT(aux,i*4  ), BIT(aux,i*4+3)))
		);
	}
}

void uknc_state::set_palette__rgb(UINT32 aux)
{
	for (int i = 0; i < 8; i++)
	{
		m_palette->set_pen_color(i, rgb_t(
			YRGB(BIT(aux,i*4+2), !BIT(aux,i*4+3)),
			YRGB(BIT(aux,i*4+1), !BIT(aux,i*4+3)),
			YRGB(BIT(aux,i*4  ), !BIT(aux,i*4+3)))
		);
	}
}

void uknc_state::set_palette_gray(UINT32 aux)
{
	for (int i = 0; i < 8; i++)
	{
		m_palette->set_pen_color(i, rgb_t(i*255/8, i*255/8, i*255/8));
	}
}


/*
 * aux bits, word 0:
 *
 * 0-3	cursor color, YRGB
 * 4	cursor type, 0: character, 1: graphics
 * 5-7	graphics cursor position in octet
 * 8-14	cursor position in line
 * 15	-
 *
 * aux bits, word 1:
 *
 * 16-18	RGB intensity
 * 19	-
 * 20-21	scaling
 */
void uknc_state::draw_scanline(UINT32 *p, scanline_t *scanline)
{
	int i, start, wide = 0, pixel_x;
	const pen_t *pen = m_palette->pens();

	if (scanline->addr < 0100000) {
		DBG_LOG(4,"draw_s",("scanline addr UNIMP: %06o\n", scanline->addr));
		return;
	}

	// TODO per-row intensity
	if (scanline->color) {
		switch (ioport("YRGB")->read()) 
		{
			case 0: set_palette_yrgb(scanline->aux); break;
			case 1: set_palette_ygrb(scanline->aux); break;
			case 2: set_palette__rgb(scanline->aux); break;
			case 3: set_palette_gray(scanline->aux); break;
		}
	}
	if (scanline->control) {
		m_video.control = scanline->aux;
	}
	wide = (m_video.control >> 20) & 3;
	start = scanline->addr - 0100000; 

	DBG_LOG(4,"draw_s",("row %3d pixaddr %06o aux 0x%08x cursor: %c at %d, color %d\n", 
		m_screen->vpos(), scanline->addr, scanline->aux, scanline->cursor?(scanline->cursor_type?'g':'t'):'-', scanline->cursor_octet, scanline->cursor_color));

	for ( i = 0; i < (80 >> wide); i++ )
	{
		UINT8 bit_plane_0 = m_videoram[ start + i ];
		UINT8 bit_plane_1 = m_videoram[ start + i + 32768 ];
		UINT8 bit_plane_2 = m_videoram[ start + i + 65536 ];

		if (scanline->cursor && (i<<wide) == scanline->cursor_octet) {
			// graphics cursor
			if (scanline->cursor_type) {
				bit_plane_0 &= ~(1 << scanline->cursor_pixel);
				bit_plane_1 &= ~(1 << scanline->cursor_pixel);
				bit_plane_2 &= ~(1 << scanline->cursor_pixel);
				bit_plane_0 |= (BIT(scanline->cursor_color, 0) << scanline->cursor_pixel);
				bit_plane_1 |= (BIT(scanline->cursor_color, 1) << scanline->cursor_pixel);
				bit_plane_2 |= (BIT(scanline->cursor_color, 2) << scanline->cursor_pixel);
			// text cursor
			} else {
				bit_plane_0 = (BIT(scanline->cursor_color, 0)) ? 255 : 0;
				bit_plane_1 = (BIT(scanline->cursor_color, 1)) ? 255 : 0;
				bit_plane_2 = (BIT(scanline->cursor_color, 2)) ? 255 : 0;
			}
		}

		for (pixel_x = 0; pixel_x < 8; pixel_x++)
		{
			UINT8 pen_bit_0, pen_bit_1, pen_bit_2;
			UINT8 pen_selected;

			pen_bit_0 = (bit_plane_0 >> (pixel_x)) & 0x01;
			pen_bit_1 = (bit_plane_1 >> (pixel_x)) & 0x01;
			pen_bit_2 = (bit_plane_2 >> (pixel_x)) & 0x01;

			pen_selected = (pen_bit_2 << 2 | pen_bit_1 << 1 | pen_bit_0);

			*p++ = pen[pen_selected];
			if (wide)
			*p++ = pen[pen_selected];
			if (wide > 1)
			*p++ = pen[pen_selected];
			if (wide > 2)
			*p++ = pen[pen_selected];
		}
	}
}

TIMER_DEVICE_CALLBACK_MEMBER(uknc_state::scanline_callback)
{
	UINT16 y = m_screen->vpos();

	if (y < UKNC_VERT_START) return;
	y -= UKNC_VERT_START;
	if (y >= UKNC_DISP_VERT) return;

	draw_scanline(&m_tmpbmp.pix32(y), &m_scanlines[y]);
}

void uknc_state::update_scanlines()
{
	int row, wide = 0;
	UINT16 addr = 0270, nextaddr;
	bool cursor = FALSE, cursor_type = FALSE;
	int cursor_color = 0, cursor_octet = 0, cursor_pixel = 0;
	scanline_t *scanline;

	for (row = 0; row < UKNC_DISP_VERT; row++) {
		scanline = &m_scanlines[row];
		scanline->addr = m_subcpu->space(AS_PROGRAM).read_word(addr);
		nextaddr = m_subcpu->space(AS_PROGRAM).read_word(addr + 2);
		if (BIT(nextaddr, 0))
			cursor = !cursor;
		scanline->control = scanline->color = FALSE;
		scanline->cursor = cursor;
		if (wide) {
			scanline->aux = m_subcpu->space(AS_PROGRAM).read_word(addr - 4) |
				(m_subcpu->space(AS_PROGRAM).read_word(addr - 2) << 16);
			if (wide == 1) {
				scanline->control = TRUE;
				cursor_type = BIT(scanline->aux, 4);
				cursor_color = scanline->aux & 15;
				cursor_pixel = (scanline->aux >> 4) & 3;
				cursor_octet = (scanline->aux >> 8) & 127;
			} else {
				scanline->color = TRUE;
			}
		}
		scanline->cursor_type = cursor_type;
		scanline->cursor_color = cursor_color;
		scanline->cursor_pixel = cursor_pixel;
		scanline->cursor_octet = cursor_octet;
//		DBG_LOG(2,"update_s",("row %3d wide: %d pixaddr %06o aux 0x%08x <%c %c> nextat %06o\n", 
//			row, wide, scanline->addr, scanline->aux, scanline->color ? 't' : '.', scanline->control ? 't' : '.', nextaddr));
		DBG_LOG(4,"update_s",("row %3d pixaddr %06o aux 0x%08x cursor: %c at %d, color %d\n", 
			row, scanline->addr, scanline->aux, cursor?(cursor_type?'g':'t'):'-', cursor_octet, cursor_color));
		if (BIT(nextaddr, 1)) {
			wide = BIT(nextaddr, 2) + 1;
			nextaddr &= ~7;
			nextaddr += 4;
		} else {
			wide = 0;
			nextaddr &= ~3;
		}
		addr = nextaddr;
	}
}

UINT32 uknc_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	copybitmap(bitmap, m_tmpbmp, 0, 0, UKNC_HORZ_START, UKNC_VERT_START, cliprect);
	update_scanlines();
	return 0;
}

WRITE16_MEMBER(uknc_state::odt_main_w)
{
	DBG_LOG(4,"ODT main", ("%d -> %d -- %s\n", m_odt_main, data, data?"HALT":"USER"));

	if (m_odt_main != data) {
		m_bankdev0->set_bank(data & 1);
		m_odt_main = data;
	}
}


void uknc_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
    case TIMER_ID_VSYNC:
        if (!BIT(m_bcsr, 8)) 
			m_subcpu->set_input_line(INPUT_LINE_EVNT, ASSERT_LINE);
        if (!BIT(m_bcsr, 9)) 
			m_maincpu->set_input_line(INPUT_LINE_EVNT, ASSERT_LINE);
        break;
	}
}

static const z80_daisy_config uknc_daisy_chain[] =
{
	{ "channel" },
//	{ "trap" },
	{ "dl11com" },
	{ "qbus" },
	{ nullptr }
};

static const z80_daisy_config uknc_sub_daisy_chain[] =
{
//	{ "event" },
	{ "timer" },
	{ "keyboard" },
//	{ "reset" },
	{ "subchan" },
	{ "subqbus" },
	{ nullptr }
};


void uknc_state::machine_start()
{
	logerror("UKNC: machine_start()\n");

	m_bankdev0->space(AS_PROGRAM).set_trap_unmap(TRUE);
	m_bankdev0->space(AS_PROGRAM).set_trap_line(INPUT_LINE_BUSERR);

	// 3 bitplanes
	m_videoram = std::make_unique<UINT8[]>(32768*3);

	membank("plane0bank4")->set_base(&m_videoram[0]);
	membank("plane0bank5")->set_base(&m_videoram[020000]);
	membank("plane0bank6")->set_base(&m_videoram[040000]);
	membank("plane0bank7")->set_base(&m_videoram[060000]);
}

void uknc_state::machine_reset()
{
	logerror("UKNC: machine_reset()\n");

	bcsr_w(m_bankdev0->space(AS_PROGRAM), 0, 01401);
	scsr_w(m_bankdev0->space(AS_PROGRAM), 0, 0);
}

void uknc_state::video_start()
{
	m_scanlines = std::make_unique<scanline_t[]>(UKNC_DISP_VERT);

	m_tmpbmp.allocate(UKNC_DISP_HORZ, UKNC_DISP_VERT);

    m_vsync_timer = timer_alloc(TIMER_ID_VSYNC);
    m_vsync_timer->adjust(m_screen->time_until_pos(0, 0), 0, m_screen->frame_period());
}

PALETTE_INIT_MEMBER(uknc_state, uknc)
{
	for (int i = 0; i < 8; i++)
	{
		palette.set_pen_color(i, rgb_t(i<<5,i<<5,i<<5));
	}
}

// VRAM access from maincpu

WRITE16_MEMBER(uknc_state::maincpu_vram_addr_w)
{
	DBG_LOG(3,"VRAM WA", ("<- %06o & %06o%s (%06o)\n", data, mem_mask, mem_mask==0xffff?"":" WARNING", data<<1));
	m_video.maincpu_vram_addr = data;
	if (data < 0100000) {
		m_video.maincpu_vram_data = m_bankdev0->space(AS_PROGRAM).read_word(0200000 + (data << 1));
	} else {
		m_video.maincpu_vram_data = m_videoram[data] | (m_videoram[data + 0100000] << 8);
	}
}

READ16_MEMBER(uknc_state::maincpu_vram_addr_r)
{
	DBG_LOG(3,"VRAM RA", ("== %06o (%06o)\n", m_video.maincpu_vram_addr, m_video.maincpu_vram_addr<<1));
	return m_video.maincpu_vram_addr;
}

WRITE16_MEMBER(uknc_state::maincpu_vram_data_w)
{
	DBG_LOG(2,"VRAM 12 W", ("%06o (%06o) <- %06o & %06o%s\n", 
		m_video.maincpu_vram_addr, m_video.maincpu_vram_addr<<1, data, mem_mask, mem_mask==0xffff?"":" WARNING"));
	COMBINE_DATA(&m_video.maincpu_vram_data);
	if (m_video.maincpu_vram_addr < 0100000) {
		m_bankdev0->space(AS_PROGRAM).write_word(0200000 + (m_video.maincpu_vram_addr << 1), m_video.maincpu_vram_data);
	} else {
		if (ACCESSING_BITS_0_7) {
			m_videoram[m_video.maincpu_vram_addr] = data & 0xff;
		}
		if (ACCESSING_BITS_8_15) {
			m_videoram[m_video.maincpu_vram_addr + 0100000] = (data >> 8) & 0xff;
		}
	}
}

READ16_MEMBER(uknc_state::maincpu_vram_data_r)
{
	DBG_LOG(2,"VRAM 12 R", ("%06o (%06o) == %06o\n", m_video.vram_addr, m_video.vram_addr<<1, m_video.vram_data_p12));
	return m_video.maincpu_vram_data;
}

// VRAM access from subcpu

WRITE16_MEMBER(uknc_state::vram_addr_w)
{
	DBG_LOG(3,"VRAM WA", ("<- %06o & %06o%s (%06o)\n", data, mem_mask, mem_mask==0xffff?"":" WARNING", data<<1));
	m_video.vram_addr = data;
	if (data < 0100000) {
		m_video.vram_data_p12 = m_bankdev0->space(AS_PROGRAM).read_word(0200000 + (data << 1));
	} else {
		m_video.vram_data_p12 = m_videoram[data] | (m_videoram[data + 0100000] << 8);
		m_video.vram_data_p0 = m_videoram[data - 0100000];
	}
}

READ16_MEMBER(uknc_state::vram_addr_r)
{
	DBG_LOG(3,"VRAM RA", ("== %06o (%06o)\n", m_video.vram_addr, m_video.vram_addr<<1));
	return m_video.vram_addr;
}

WRITE16_MEMBER(uknc_state::vram_data_p0_w)
{
	DBG_LOG(2,"VRAM 00 W", ("%06o (%06o) <- %06o & %06o%s\n", 
		m_video.vram_addr, m_video.vram_addr<<1, data, mem_mask, mem_mask==0xffff?"":" WARNING"));
	m_videoram[m_video.vram_addr - 0100000] = data;
	COMBINE_DATA(&m_video.vram_data_p0);
}

WRITE16_MEMBER(uknc_state::vram_data_p12_w)
{
	DBG_LOG(2,"VRAM 12 W", ("%06o (%06o) <- %06o & %06o%s\n", 
		m_video.vram_addr, m_video.vram_addr<<1, data, mem_mask, mem_mask==0xffff?"":" WARNING"));
	COMBINE_DATA(&m_video.vram_data_p12);
	if (m_video.vram_addr < 0100000) {
		m_bankdev0->space(AS_PROGRAM).write_word(0200000 + (m_video.vram_addr << 1), m_video.vram_data_p12);
	} else {
		if (ACCESSING_BITS_0_7) {
			m_videoram[m_video.vram_addr] = data & 0xff;
		}
		if (ACCESSING_BITS_8_15) {
			m_videoram[m_video.vram_addr + 0100000] = (data >> 8) & 0xff;
		}
	}
}

READ16_MEMBER(uknc_state::vram_data_p0_r)
{
	DBG_LOG(2,"VRAM 00 R", ("%06o (%06o) == %06o\n", m_video.vram_addr, m_video.vram_addr<<1, m_video.vram_data_p0));
	return m_video.vram_data_p0;
}

READ16_MEMBER(uknc_state::vram_data_p12_r)
{
	DBG_LOG(2,"VRAM 12 R", ("%06o (%06o) == %06o\n", m_video.vram_addr, m_video.vram_addr<<1, m_video.vram_data_p12));
	return m_video.vram_data_p12;
}

WRITE16_MEMBER(uknc_state::sprite_w)
{
	DBG_LOG(2,"Sprite W", ("%06o <- %06o & %06o @ %06o%s (color %d bg %08x)\n", 
		0177016 + (offset<<1), data, mem_mask, m_video.vram_addr, mem_mask==0xffff?"":" WARNING", m_video.foreground, m_video.background));
	switch (offset) {
		case 0:	// 177016
			m_video.foreground = data & 7;
			break;

		case 1:	// 177020
			m_video.background &= ~0xffff;
			m_video.background |= data & 0x7777;
			break;

		case 2:	// 177022
			m_video.background &= 0xffff;
			m_video.background |= (data & 0x7777) << 16;
			break;

		case 3:	// 177024
			m_video.sprite_mask = data;
			if (!BIT(m_video.plane_mask, 0)) {
				UINT8 vdata;

				vdata  = (BIT(data, 0) ? BIT(m_video.foreground, 0) : BIT(m_video.background,  0));
				vdata |= (BIT(data, 1) ? BIT(m_video.foreground, 0) : BIT(m_video.background,  4)) << 1;
				vdata |= (BIT(data, 2) ? BIT(m_video.foreground, 0) : BIT(m_video.background,  8)) << 2;
				vdata |= (BIT(data, 3) ? BIT(m_video.foreground, 0) : BIT(m_video.background, 12)) << 3;
				vdata |= (BIT(data, 4) ? BIT(m_video.foreground, 0) : BIT(m_video.background, 16)) << 4;
				vdata |= (BIT(data, 5) ? BIT(m_video.foreground, 0) : BIT(m_video.background, 20)) << 5;
				vdata |= (BIT(data, 6) ? BIT(m_video.foreground, 0) : BIT(m_video.background, 24)) << 6;
				vdata |= (BIT(data, 7) ? BIT(m_video.foreground, 0) : BIT(m_video.background, 28)) << 7;
				m_videoram[m_video.vram_addr - 0100000] = vdata;
			}
			if (!BIT(m_video.plane_mask, 1)) {
				UINT8 vdata;

				vdata  = (BIT(data, 0) ? BIT(m_video.foreground, 1) : BIT(m_video.background,  1));
				vdata |= (BIT(data, 1) ? BIT(m_video.foreground, 1) : BIT(m_video.background,  5)) << 1;
				vdata |= (BIT(data, 2) ? BIT(m_video.foreground, 1) : BIT(m_video.background,  9)) << 2;
				vdata |= (BIT(data, 3) ? BIT(m_video.foreground, 1) : BIT(m_video.background, 13)) << 3;
				vdata |= (BIT(data, 4) ? BIT(m_video.foreground, 1) : BIT(m_video.background, 17)) << 4;
				vdata |= (BIT(data, 5) ? BIT(m_video.foreground, 1) : BIT(m_video.background, 21)) << 5;
				vdata |= (BIT(data, 6) ? BIT(m_video.foreground, 1) : BIT(m_video.background, 25)) << 6;
				vdata |= (BIT(data, 7) ? BIT(m_video.foreground, 1) : BIT(m_video.background, 29)) << 7;
				m_videoram[m_video.vram_addr] = vdata;
			}
			if (!BIT(m_video.plane_mask, 2)) {
				UINT8 vdata;

				vdata  = (BIT(data, 0) ? BIT(m_video.foreground, 2) : BIT(m_video.background,  2));
				vdata |= (BIT(data, 1) ? BIT(m_video.foreground, 2) : BIT(m_video.background,  6)) << 1;
				vdata |= (BIT(data, 2) ? BIT(m_video.foreground, 2) : BIT(m_video.background, 10)) << 2;
				vdata |= (BIT(data, 3) ? BIT(m_video.foreground, 2) : BIT(m_video.background, 14)) << 3;
				vdata |= (BIT(data, 4) ? BIT(m_video.foreground, 2) : BIT(m_video.background, 18)) << 4;
				vdata |= (BIT(data, 5) ? BIT(m_video.foreground, 2) : BIT(m_video.background, 22)) << 5;
				vdata |= (BIT(data, 6) ? BIT(m_video.foreground, 2) : BIT(m_video.background, 26)) << 6;
				vdata |= (BIT(data, 7) ? BIT(m_video.foreground, 2) : BIT(m_video.background, 30)) << 7;
				m_videoram[m_video.vram_addr + 0100000] = vdata;
			}
			break;

		// old variant
		case 33:	// 177024
			m_video.sprite_mask = data;
			if (!BIT(m_video.plane_mask, 0)) {
				UINT8 vdata;

				vdata  = BIT(m_video.background,  0);
				vdata |= BIT(m_video.background,  4) << 1;
				vdata |= BIT(m_video.background,  8) << 2;
				vdata |= BIT(m_video.background, 12) << 3;
				vdata |= BIT(m_video.background, 16) << 4;
				vdata |= BIT(m_video.background, 20) << 5;
				vdata |= BIT(m_video.background, 24) << 6;
				vdata |= BIT(m_video.background, 28) << 7;
				vdata &= ~data; if (BIT(m_video.foreground, 0)) vdata |= data;
				m_videoram[m_video.vram_addr - 0100000] = vdata;
			}
			if (!BIT(m_video.plane_mask, 1)) {
				UINT8 vdata;

				vdata  = BIT(m_video.background,  1);
				vdata |= BIT(m_video.background,  5) << 1;
				vdata |= BIT(m_video.background,  9) << 2;
				vdata |= BIT(m_video.background, 13) << 3;
				vdata |= BIT(m_video.background, 17) << 4;
				vdata |= BIT(m_video.background, 21) << 5;
				vdata |= BIT(m_video.background, 25) << 6;
				vdata |= BIT(m_video.background, 29) << 7;
				vdata &= ~data; if (BIT(m_video.foreground, 1)) vdata |= data;
				m_videoram[m_video.vram_addr] = vdata;
			}
			if (!BIT(m_video.plane_mask, 2)) {
				UINT8 vdata;

				vdata  = BIT(m_video.background,  2);
				vdata |= BIT(m_video.background,  6) << 1;
				vdata |= BIT(m_video.background, 10) << 2;
				vdata |= BIT(m_video.background, 14) << 3;
				vdata |= BIT(m_video.background, 18) << 4;
				vdata |= BIT(m_video.background, 22) << 5;
				vdata |= BIT(m_video.background, 26) << 6;
				vdata |= BIT(m_video.background, 30) << 7;
				vdata &= ~data; if (BIT(m_video.foreground, 2)) vdata |= data;
				m_videoram[m_video.vram_addr + 0100000] = vdata;
			}
			break;

		case 4:	// 177026
			m_video.plane_mask = data & 7;
			break;
	}
}

READ16_MEMBER(uknc_state::sprite_r)
{
	UINT16 data = 0;
	switch (offset) {
		case 0:
			data = m_video.foreground;
			break;

		case 1:
			data = m_video.background & 0xffff;
			break;

		case 2:
			data = (m_video.background >> 16) & 0xffff;
			break;

		case 3:
			// vram read
			if (m_video.vram_addr >= 0100000) {
				UINT8 vdata;

				// plane 0
				vdata = m_videoram[m_video.vram_addr - 0100000];
				m_video.background  = BIT(vdata, 0);
				m_video.background |= BIT(vdata, 1) << 4;
				m_video.background |= BIT(vdata, 2) << 8;
				m_video.background |= BIT(vdata, 3) << 12;
				m_video.background |= BIT(vdata, 4) << 16;
				m_video.background |= BIT(vdata, 5) << 20;
				m_video.background |= BIT(vdata, 6) << 24;
				m_video.background |= BIT(vdata, 7) << 28;

				// plane 1
				vdata = m_videoram[m_video.vram_addr];
				m_video.background |= BIT(vdata, 0) << 1;
				m_video.background |= BIT(vdata, 1) << 5;
				m_video.background |= BIT(vdata, 2) << 9;
				m_video.background |= BIT(vdata, 3) << 13;
				m_video.background |= BIT(vdata, 4) << 17;
				m_video.background |= BIT(vdata, 5) << 21;
				m_video.background |= BIT(vdata, 6) << 25;
				m_video.background |= BIT(vdata, 7) << 29;

				// plane 2
				vdata = m_videoram[m_video.vram_addr + 0100000];
				m_video.background |= BIT(vdata, 0) << 2;
				m_video.background |= BIT(vdata, 1) << 6;
				m_video.background |= BIT(vdata, 2) << 10;
				m_video.background |= BIT(vdata, 3) << 14;
				m_video.background |= BIT(vdata, 4) << 18;
				m_video.background |= BIT(vdata, 5) << 22;
				m_video.background |= BIT(vdata, 6) << 26;
				m_video.background |= BIT(vdata, 7) << 30;
			} else {
				DBG_LOG(1,"sprite_r",("vram addr UNIMP: %06o\n", m_video.vram_addr));
			}
			break;

		case 4:
			data = m_video.plane_mask;
			break;
	}
	if (offset != 3)
		DBG_LOG(2,"Sprite R", ("%06o == %06o\n", 0177016 + (offset<<1), data));
	else
		DBG_LOG(2,"Sprite R", ("%06o == %08x @ %06o\n", 0177016 + (offset<<1), m_video.background, m_video.vram_addr));
	return data;
}


/*
 *	00	R	cassette data in
 *	01	RW	cassette data out
 *	02	RW	cassette data gate
 *	03	RW	cassette/external event/index gate.  0: cassette  1: ???
 *	04	RW	maincpu USER/HALT mode. 0: USER, 1: HALT
 *	05	RW	maincpu DCLO line
 *	06	-
 *	07	RW	sound data out, if 08-12 == 0
 *	12:08	sound frequency matrix
 *	13	RW	reserved
 *	14	-
 *	15	RW	maincpu ACLO line
 */
WRITE16_MEMBER(uknc_state::scsr_w)
{
	DBG_LOG(1,"SCSR W", ("<- %06o & %06o%s%s\n", 
		data, mem_mask, mem_mask==0xffff?"":" WARNING", data&0x7fef?" UNIMP":""));
	m_scsr = data;

	if (BIT(data, 2))
		m_cassette->output(BIT(data, 1) ? -1 : 1);

	m_maincpu->set_input_line(INPUT_LINE_1801HALT, BIT(data, 4) ? ASSERT_LINE : CLEAR_LINE);

	if (BIT(data, 15) && m_maincpu->suspended(SUSPEND_REASON_DISABLE)) {
		m_maincpu->resume(SUSPEND_REASON_DISABLE);
	} else if (!BIT(data, 15) && !m_maincpu->suspended(SUSPEND_REASON_DISABLE)) {
		m_maincpu->execute_set_input(INPUT_LINE_ACLO, ASSERT_LINE);
	}
}

READ16_MEMBER(uknc_state::scsr_r)
{
	double tap_val = m_cassette->input();

	m_scsr = ( m_scsr & ~1 ) | ( tap_val < 0 ? 1 : 0 );

	DBG_LOG(1,"SCSR R", ("== %06o\n", m_scsr));

	return m_scsr;
}

/*
 *	0-3	CE0..CE3 bus signals
 *	  CE0	mapping at 100000: 0 - read from VRAM plane 0, 1 - read from ROM (reset to 1)
 *	  CE1-2	ROM carts: bank selector
 *	  CE3	slot number
 *	4-7	ROM banks replacement: 0 - writes ignored, 1 - writes go to VRAM plane 0
 *	  4		100000..117777
 *	  5		120000..137777
 *	  6		140000..157777
 *	  7		160000..177777
 *	8	line clock interrupt (subcpu): 0 - enable
 *	9	line clock interrupt (maincpu): 0 - enable
 */
WRITE16_MEMBER(uknc_state::bcsr_w)
{
	int bank1, bank2;
	address_space *subspace = &m_subcpu->space(AS_PROGRAM);

	bank1 = (data & 1) | ((data >> 3) & 1);
	bank2 = (data >> 5) & 7;

	DBG_LOG(1,"BCSR W", ("<- %06o & %06o%s (banks %d %d)\n", 
		data, mem_mask, mem_mask==0xffff?"":" WARNING", bank1, bank2));

	// order matters: device may unmap itself from 'window' region and we restore the mapping
	m_subqbus->ce0_ce3_w(data & 15);

	if (BIT(m_bcsr ^ data, 0) && BIT(data, 0)) {
		DBG_LOG(1,"BCSR",("restoring bankdev1 mapping\n"));
		// restore AM_RANGE (0100000, 0117777) AM_DEVREADWRITE("bankdev1", address_map_bank_device, read16, write16)
		subspace->unmap_readwrite(0100000, 0117777);
		subspace->install_readwrite_handler(0100000, 0117777, 
			read16_delegate(FUNC(address_map_bank_device::read16), subdevice<address_map_bank_device>("bankdev1")),
			write16_delegate(FUNC(address_map_bank_device::write16), subdevice<address_map_bank_device>("bankdev1")));
	}

	m_bankdev1->set_bank(bank1);
	m_bankdev2->set_bank(bank2);

	m_bcsr = data;
}

READ16_MEMBER(uknc_state::bcsr_r)
{
	DBG_LOG(1,"BCSR R", ("== %06o\n", m_bcsr));
	return m_bcsr;
}


static MACHINE_CONFIG_START( uknc, uknc_state )
	MCFG_CPU_ADD("maincpu", K1801VM2, XTAL_16MHz/4)	// external clock is /2 + internal divider /2
	MCFG_CPU_PROGRAM_MAP(uknc_mem)
	MCFG_DEVICE_DISABLE()
	MCFG_T11_INITIAL_MODE(0160000)
	MCFG_T11_FEATURES(T11FEAT_BUSERR)
	MCFG_Z80_DAISY_CHAIN(uknc_daisy_chain)
	MCFG_K1801VM2_BANKSWITCH_CB(WRITE16(uknc_state, odt_main_w))

	MCFG_DEVICE_ADD("bankdev0", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(uknc_mem_banked)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_BIG)
	MCFG_ADDRESS_MAP_BANK_ADDRBUS_WIDTH(17)
	MCFG_ADDRESS_MAP_BANK_DATABUS_WIDTH(16)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0200000)

	MCFG_CPU_ADD("subcpu",  K1801VM2, XTAL_12_5MHz/4)
	MCFG_CPU_PROGRAM_MAP(uknc_sub_mem)
	MCFG_T11_INITIAL_MODE(0160000)
	MCFG_T11_FEATURES(T11FEAT_BUSERR)
	MCFG_Z80_DAISY_CHAIN(uknc_sub_daisy_chain)

	MCFG_DEVICE_ADD("bankdev1", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(uknc_cart_mem)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_BIG)
	MCFG_ADDRESS_MAP_BANK_ADDRBUS_WIDTH(18)
	MCFG_ADDRESS_MAP_BANK_DATABUS_WIDTH(16)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0100000)

	MCFG_DEVICE_ADD("bankdev2", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(uknc_rom_mem)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_BIG)
	MCFG_ADDRESS_MAP_BANK_ADDRBUS_WIDTH(18)
	MCFG_ADDRESS_MAP_BANK_DATABUS_WIDTH(16)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0100000)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(XTAL_12_5MHz, 
		UKNC_TOTAL_HORZ, UKNC_HORZ_START, UKNC_HORZ_START+UKNC_DISP_HORZ, 
		UKNC_TOTAL_VERT, UKNC_VERT_START, UKNC_VERT_START+UKNC_DISP_VERT);
	MCFG_SCREEN_UPDATE_DRIVER(uknc_state, screen_update)

	MCFG_PALETTE_ADD("palette", 8)
	MCFG_PALETTE_INIT_OWNER(uknc_state, uknc)

	MCFG_TIMER_DRIVER_ADD_SCANLINE("scantimer", uknc_state, scanline_callback, "screen", 0, 1)

	/*
	 * devices on the main cpu 
	 */

	MCFG_DEVICE_ADD("qbus", QBUS, 0)
	MCFG_QBUS_CPU(":maincpu")
	MCFG_QBUS_OUT_BIRQ4_CB(INPUTLINE("maincpu", INPUT_LINE_VIRQ))
	MCFG_QBUS_SLOT_ADD("qbus", "qbus1", 0, uknc_cards, "vhd")
	MCFG_QBUS_SLOT_ADD("qbus", "qbus2", 0, uknc_cards, nullptr)

	MCFG_DEVICE_ADD("channel", VP1_120, 0)
	MCFG_VP1_120_VIRQ_HANDLER(INPUTLINE("maincpu", INPUT_LINE_VIRQ))
	MCFG_VP1_120_CHANNEL0_OUT_HANDLER(DEVWRITE16("subchan", vp1_120_sub_device, write_channel0))
	MCFG_VP1_120_CHANNEL1_OUT_HANDLER(DEVWRITE16("subchan", vp1_120_sub_device, write_channel1))
	MCFG_VP1_120_CHANNEL2_OUT_HANDLER(DEVWRITE16("subchan", vp1_120_sub_device, write_channel2))
	MCFG_VP1_120_CHANNEL0_ACK_HANDLER(DEVWRITELINE("subchan", vp1_120_sub_device, ack_channel0))
	MCFG_VP1_120_CHANNEL1_ACK_HANDLER(DEVWRITELINE("subchan", vp1_120_sub_device, ack_channel1))
	MCFG_VP1_120_RESET_HANDLER(DEVWRITELINE("subchan", vp1_120_sub_device, write_reset))

	// serial connection to host
	MCFG_DEVICE_ADD("dl11com", DL11, XTAL_4_608MHz)
	MCFG_DL11_RXC(9600)
	MCFG_DL11_TXC(9600)
	MCFG_DL11_RXVEC(0370)
	MCFG_DL11_TXVEC(0374)
	MCFG_DL11_TXD_HANDLER(DEVWRITELINE("rs232", rs232_port_device, write_txd))
	MCFG_DL11_RTS_HANDLER(DEVWRITELINE("rs232", rs232_port_device, write_rts))
	MCFG_DL11_RXRDY_HANDLER(INPUTLINE("maincpu", INPUT_LINE_VIRQ))
	MCFG_DL11_TXRDY_HANDLER(INPUTLINE("maincpu", INPUT_LINE_VIRQ))

	MCFG_RS232_PORT_ADD("rs232", default_rs232_devices, "null_modem")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("dl11com", dl11_device, rx_w))

#if 0
	// local area network
	MCFG_DEVICE_ADD("dl11lan", DL11, XTAL_4_608MHz)
	MCFG_DL11_RXC(9600)
	MCFG_DL11_TXC(9600)
	MCFG_DL11_RXVEC(0360)
	MCFG_DL11_TXVEC(0364)
	MCFG_DL11_TXD_HANDLER(DEVWRITELINE("lan", rs232_port_device, write_txd))
	MCFG_DL11_RTS_HANDLER(DEVWRITELINE("lan", rs232_port_device, write_rts))
	MCFG_DL11_RXRDY_HANDLER(INPUTLINE("maincpu", INPUT_LINE_VIRQ))
	MCFG_DL11_TXRDY_HANDLER(INPUTLINE("maincpu", INPUT_LINE_VIRQ))

	MCFG_RS232_PORT_ADD("lan", default_rs232_devices, "null_modem")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("dl11lan", dl11_device, rx_w))
#endif

	/*
	 * devices on the sub cpu
	 */

	// has extra signals: CE0..CE3 etc.
	MCFG_DEVICE_ADD("subqbus", QBUS, 0)
	MCFG_QBUS_CPU(":subcpu")
	MCFG_QBUS_OUT_BIRQ4_CB(INPUTLINE("subcpu", INPUT_LINE_VIRQ))
	MCFG_QBUS_SLOT_ADD("subqbus", "cart1", 0, uknc_carts, nullptr)
	MCFG_QBUS_SLOT_ADD("subqbus", "cart2", 0, uknc_carts, nullptr)

	MCFG_DEVICE_ADD("subchan", VP1_120_SUB, 0)
	MCFG_VP1_120_SUB_VIRQ_HANDLER(INPUTLINE("subcpu", INPUT_LINE_VIRQ))
	MCFG_VP1_120_SUB_CHANNEL0_OUT_HANDLER(DEVWRITE16("channel", vp1_120_device, write_channel0))
	MCFG_VP1_120_SUB_CHANNEL1_OUT_HANDLER(DEVWRITE16("channel", vp1_120_device, write_channel1))
	MCFG_VP1_120_SUB_CHANNEL0_ACK_HANDLER(DEVWRITELINE("channel", vp1_120_device, ack_channel0))
	MCFG_VP1_120_SUB_CHANNEL1_ACK_HANDLER(DEVWRITELINE("channel", vp1_120_device, ack_channel1))
	MCFG_VP1_120_SUB_CHANNEL2_ACK_HANDLER(DEVWRITELINE("channel", vp1_120_device, ack_channel2))

	MCFG_DEVICE_ADD("keyboard", XM1_031_KBD, 0)
	MCFG_XM1_031_KBD_VIRQ_HANDLER(INPUTLINE("subcpu", INPUT_LINE_VIRQ))

	MCFG_DEVICE_ADD("timer", XM1_031_TIMER, 0)
	MCFG_XM1_031_TIMER_VIRQ_HANDLER(INPUTLINE("subcpu", INPUT_LINE_VIRQ))

	// centronics port
	MCFG_DEVICE_ADD("ppi8255", I8255, 0)
	MCFG_I8255_OUT_PORTA_CB(DEVWRITE8("cent_data_out", output_latch_device, write))
	MCFG_I8255_IN_PORTB_CB(DEVREAD8("cent_status_in", input_buffer_device, read))
//	MCFG_I8255_OUT_PORTC_CB(WRITE8(uknc_state, ppi_portc_w))	// bit 7 -- /STROBE

	MCFG_CENTRONICS_ADD("centronics", centronics_devices, "printer")
	MCFG_CENTRONICS_ACK_HANDLER(DEVWRITELINE("cent_status_in", input_buffer_device, write_bit7))
	MCFG_CENTRONICS_BUSY_HANDLER(DEVWRITELINE("cent_status_in", input_buffer_device, write_bit1))
	MCFG_DEVICE_ADD("cent_status_in", INPUT_BUFFER, 0)
	MCFG_CENTRONICS_OUTPUT_LATCH_ADD("cent_data_out", "centronics")

	MCFG_CASSETTE_ADD( "cassette" )
	MCFG_CASSETTE_DEFAULT_STATE(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED)

#if 0
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, "cassette")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
#endif
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( uknc )
	ROM_REGION16_LE(0100000, "subcpu", ROMREGION_ERASE00)
	ROM_DEFAULT_BIOS("208")
	ROM_SYSTEM_BIOS(0, "135", "mask 135 (198x)")
	ROMX_LOAD("uknc135.rom", 0, 0100000, NO_DUMP, ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "208", "mask 208 (198x)")
	ROMX_LOAD("uknc.rom", 0, 0100000, CRC(a1536994) SHA1(b3c7c678c41ffa9b37f654fbf20fef7d19e6407b), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY     FULLNAME       FLAGS */
COMP( 1987, uknc,   0,      0,       uknc,      uknc, driver_device,    0,    "<unknown>",  "UKNC", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
