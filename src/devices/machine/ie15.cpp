// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

    15IE-00-013 Terminal

    A serial (RS232 or current loop) green-screen terminal, mostly VT52
    compatible (no Hold Screen mode and no graphics character set).

    Alternate character set (selected by SO/SI chars) is Cyrillic.

****************************************************************************/

#include "machine/ie15.h"


#define VERBOSE_DBG 0       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-24s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)


ie15_device::ie15_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source)
	: device_t(mconfig, type, name, tag, owner, clock, shortname, source)
	, device_serial_interface(mconfig, *this)
	, m_maincpu(*this, "maincpu")
	, m_beeper(*this, "beeper")
	, m_rs232(*this, "rs232")
	, m_screen(*this, "screen")
	, m_keyboard(*this, "keyboard")
	, m_io_keyboard(*this, "io_keyboard")
{
}

ie15_device::ie15_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ie15_device(mconfig, IE15, "IE15 Terminal", tag, owner, clock, "ie15", __FILE__)
{
}


READ8_MEMBER( ie15_device::mem_r ) {
	UINT8 ret;

	ret = m_p_videoram[m_video.ptr1];
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0 && m_video.ptr1 >= SCREEN_PAGE)
	{
		DBG_LOG(2,"memory",("R @ %03x == %02x\n", m_video.ptr1, ret));
	}
	m_video.ptr1++;
	m_video.ptr1 &= 0xfff;
	m_latch = 0;
	return ret;
}

WRITE8_MEMBER( ie15_device::mem_w ) {
	if ((m_latch ^= 1) == 0) {
		if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0 && m_video.ptr1 >= SCREEN_PAGE)
		{
			DBG_LOG(2,"memory",("W @ %03x <- %02x\n", m_video.ptr1, data));
		}
		m_p_videoram[m_video.ptr1++] = data;
		m_video.ptr1 &= 0xfff;
	}
}

WRITE8_MEMBER( ie15_device::mem_addr_inc_w ) {
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		DBG_LOG(2,"memory",("++ %03x\n", m_video.ptr1));
	}
	m_video.ptr1++;
	m_video.ptr1 &= 0xfff;
	if (m_video.enable)
		m_video.ptr2 = m_video.ptr1;
}

WRITE8_MEMBER( ie15_device::mem_addr_dec_w ) {
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		DBG_LOG(2,"memory",("-- %03x\n", m_video.ptr1));
	}
	m_video.ptr1--;
	m_video.ptr1 &= 0xfff;
	if (m_video.enable)
		m_video.ptr2 = m_video.ptr1;
}

WRITE8_MEMBER( ie15_device::mem_addr_lo_w ) {
	UINT16 tmp = m_video.ptr1;

	tmp &= 0xff0;
	tmp |= ((data >> 4) & 0xf);
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		DBG_LOG(2,"memory",("lo %03x <- %02x = %03x\n", m_video.ptr1, data, tmp));
	}
	m_video.ptr1 = tmp;
	if (m_video.enable)
		m_video.ptr2 = tmp;
}

WRITE8_MEMBER( ie15_device::mem_addr_hi_w ) {
	UINT16 tmp = m_video.ptr1;

	tmp &= 0xf;
	tmp |= (data << 4);
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		DBG_LOG(2,"memory",("hi %03x <- %02x = %03x\n", m_video.ptr1, data, tmp));
	}
	m_video.ptr1 = tmp;
	if (m_video.enable)
		m_video.ptr2 = tmp;
}

TIMER_CALLBACK_MEMBER(ie15_device::ie15_beepoff)
{
	m_beeper->set_state(0);
}

WRITE8_MEMBER( ie15_device::beep_w ) {
	UINT16 length = (m_long_beep & IE_TRUE) ? 150 : 400;

	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		DBG_LOG(1,"beep",("(%s)\n", m_long_beep ? "short" : "long"));
	}
	machine().scheduler().timer_set(attotime::from_msec(length), timer_expired_delegate(FUNC(ie15_device::ie15_beepoff),this));
	m_beeper->set_state(1);
}

/* keyboard */

// active high
READ8_MEMBER( ie15_device::kb_r ) {
	DBG_LOG(2,"keyboard",("R %02X '%c'\n", m_kb_data, m_kb_data < 0x20 ? ' ' : m_kb_data));
	return m_kb_data;
}

// active low
READ8_MEMBER( ie15_device::kb_ready_r ) {
	m_kb_flag &= IE_TRUE;
	if (m_kb_flag != m_kb_flag0) {
		DBG_LOG(2,"keyboard",("? %c\n", m_kb_flag ? 'n' : 'y'));
		m_kb_flag0 = m_kb_flag;
	}
	return m_kb_flag;
}

// active low
WRITE8_MEMBER( ie15_device::kb_ready_w ) {
	DBG_LOG(2,"keyboard",("clear ready\n"));
	m_kb_flag = IE_TRUE | IE_KB_ACK;
}


// active high; active = interpret controls, inactive = display controls
READ8_MEMBER( ie15_device::kb_s_red_r ) {
	return m_io_keyboard->read() & IE_KB_RED ? IE_TRUE : 0;
}

// active high; active = setup mode
READ8_MEMBER( ie15_device::kb_s_sdv_r ) {
	return m_kb_control & IE_KB_SDV ? IE_TRUE : 0;
}

// active high; active = keypress detected on aux keypad
READ8_MEMBER( ie15_device::kb_s_dk_r ) {
	return m_kb_control & IE_KB_DK ? IE_TRUE : 0;
}

// active low; active = full duplex, inactive = half duplex
READ8_MEMBER( ie15_device::kb_s_dupl_r ) {
	return m_io_keyboard->read() & IE_KB_DUP ? IE_TRUE : 0;
}

// active high; active = on-line, inactive = local editing
READ8_MEMBER( ie15_device::kb_s_lin_r ) {
	return m_io_keyboard->read() & IE_KB_LIN ? IE_TRUE : 0;
}

/* serial port */

void ie15_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	device_serial_interface::device_timer(timer, id, param, ptr);
}

WRITE_LINE_MEMBER( ie15_device::serial_rx_callback )
{
	device_serial_interface::rx_w(state);
}

void ie15_device::rcv_complete()
{
	receive_register_extract();
	m_serial_rx_char = get_received_char();
	m_serial_rx_ready = IE_FALSE;
}

void ie15_device::tra_callback()
{
	UINT8 bit = transmit_register_get_data_bit();
	m_rs232->write_txd(bit);
}

void ie15_device::tra_complete()
{
	m_serial_tx_ready = IE_TRUE;
}

// active low
READ8_MEMBER( ie15_device::serial_rx_ready_r ) {
	return m_serial_rx_ready;
}

// active high
READ8_MEMBER( ie15_device::serial_tx_ready_r ) {
	return m_serial_tx_ready;
}

// not called unless data are ready
READ8_MEMBER( ie15_device::serial_r ) {
	m_serial_rx_ready = IE_TRUE;
	DBG_LOG(1,"serial",("R %02X '%c'\n", m_serial_rx_char, m_serial_rx_char < 0x20?' ':m_serial_rx_char&127));
	return m_serial_rx_char;
}

WRITE8_MEMBER( ie15_device::serial_w ) {
	DBG_LOG(1,"serial",("W %02X '%c'\n", data, data < 0x20?' ':data&127));

	m_serial_tx_ready = IE_FALSE;
	transmit_register_setup(data);
}

WRITE8_MEMBER( ie15_device::serial_speed_w ) {
	return;
}

READ8_MEMBER( ie15_device::flag_r ) {
	UINT8 ret = 0;

	switch (offset)
	{
		case 0: // hsync pulse (not hblank)
			ret = m_screen->hpos() < IE15_HORZ_START;
			break;
		case 1: // marker scanline
			ret = (m_screen->vpos() % 11) > 7;
			break;
		case 2: // vblank
			ret = !m_screen->vblank();
			break;
		case 4:
			ret = m_kb_ruslat;
			break;
		default:
			break;
	}
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0 && ret)
	{
		DBG_LOG(2,"flag",("read %d: %d\n", offset, ret));
	}
	return ret;
}

WRITE8_MEMBER( ie15_device::flag_w ) {
	switch (offset)
	{
		case 0:
			m_video.enable = data;
			break;
		case 1:
			m_video.cursor = data;
			break;
		case 2:
			m_long_beep = data;
			break;
		case 3:
			m_video.line25 = data;
			break;
		case 4:
			m_kb_ruslat = data;
			break;
		default:
			break;
	}
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0 && !offset)
	{
		DBG_LOG(2,"flag",("%sset %d\n", data?"":"re", offset));
	}
}

static ADDRESS_MAP_START( ie15_mem, AS_PROGRAM, 8, ie15_device )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ie15_io, AS_IO, 8, ie15_device )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(000, 000) AM_READWRITE(mem_r, mem_w)   // 00h W: memory request, R: memory data [6.1.2.2]
	AM_RANGE(001, 001) AM_READ(serial_rx_ready_r) AM_WRITENOP   // 01h W: memory latch [6.1.2.2]
	AM_RANGE(002, 002) AM_WRITE(mem_addr_hi_w)      // 02h W: memory address high [6.1.2.2]
	AM_RANGE(003, 003) AM_WRITE(mem_addr_lo_w)      // 03h W: memory address low [6.1.2.2]
	AM_RANGE(004, 004) AM_WRITE(mem_addr_inc_w)     // 04h W: memory address counter + [6.1.2.2]
	AM_RANGE(005, 005) AM_WRITE(mem_addr_dec_w)     // 05h W: memory address counter - [6.1.2.2]
	AM_RANGE(006, 006) AM_READWRITE(serial_r, serial_w)     // 06h W: serial port data [6.1.5.4]
// port 7 is handled in cpu core
	AM_RANGE(010, 010) AM_READWRITE(serial_tx_ready_r, beep_w)  // 08h W: speaker control [6.1.5.4]
	AM_RANGE(011, 011) AM_READ(kb_r)            // 09h R: keyboard data [6.1.5.2]
	AM_RANGE(012, 012) AM_READ(kb_s_red_r)          // 0Ah I: keyboard mode "RED" [6.1.5.2]
	AM_RANGE(013, 013) AM_READ(kb_ready_r)          // 0Bh R: keyboard data ready [6.1.5.2]
	AM_RANGE(014, 014) AM_READWRITE(kb_s_sdv_r, serial_speed_w) // 0Ch W: serial port speed [6.1.3.1], R: keyboard mode "SDV" [6.1.5.2]
	AM_RANGE(015, 015) AM_READWRITE(kb_s_dk_r, kb_ready_w)  // 0Dh I: keyboard mode "DK" [6.1.5.2]
	AM_RANGE(016, 016) AM_READ(kb_s_dupl_r)         // 0Eh I: keyboard mode "DUPL" [6.1.5.2]
	AM_RANGE(017, 017) AM_READ(kb_s_lin_r)          // 0Fh I: keyboard mode "LIN" [6.1.5.2]
// simulation of flag registers
	AM_RANGE(020, 027) AM_READWRITE(flag_r, flag_w)
ADDRESS_MAP_END

WRITE16_MEMBER( ie15_device::kbd_put )
{
	DBG_LOG(2,"keyboard",("W %02X<-%02X '%c' %02X (%c)\n", m_kb_data, data, 'x' /* data < 0x20 ? ' ' : (data & 255) */,
		m_kb_flag, m_kb_flag ? 'n' : 'y'));
	m_kb_control = (data >> 8) & 255;
	// send new key only when firmware has processed previous one
	if (m_kb_flag == IE_TRUE) {
		m_kb_data = data & 255;
		m_kb_flag = 0;
	}
}

void ie15_device::device_reset()
{
	memset(&m_video, 0, sizeof(m_video));
	m_kb_ruslat = m_long_beep = m_kb_control = m_kb_data = m_kb_flag0 = 0;
	m_kb_flag = IE_TRUE;

	m_beeper->set_state(0);

	m_serial_tx_ready = m_serial_rx_ready = IE_TRUE;
	set_data_frame(1 /* start bits */, 8 /* data bits */, PARITY_NONE, STOP_BITS_1);
	// device supports rates from 150 to 9600 baud but null_modem has hardcoded 9600
	set_rate(9600);
}

void ie15_device::device_start()
{
	m_p_chargen = memregion("chargen")->base();
	m_p_videoram = memregion("video")->base();
	m_video.ptr1 = m_video.ptr2 = m_latch = 0;

	m_tmpclip = rectangle(0, IE15_DISP_HORZ-1, 0, IE15_DISP_VERT-1);
	m_tmpbmp.allocate(IE15_DISP_HORZ, IE15_DISP_VERT);
	m_offbmp.allocate(IE15_DISP_HORZ, 1);
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0) {
		m_offbmp.fill(0);
	}
}

/*
    Usable raster is 800 x 275 pixels (80 x 25 characters).  24 lines are
    available to the user and 25th (topmost) line is the status line.
    Status line, if enabled, displays current serial port speed, 16 setup
    bits, and clock.  There is no NVRAM, so setup bits are always 0 after
    reset and clock starts counting at 0 XXX.

    No character attributes are available, but in 'display controls' mode
    control characters stored in memory are shown as blinking chars.

    Character cell is 10 x 11; character generator provides 7 x 8 of that.
    3 extra horizontal pixels are always blank.  Blinking cursor may be
    displayed on 3 extra scan lines.

    On each scan line, video board draws 80 characters from any location
    in video memory; this is used by firmware to provide instant scroll
    and cursor, which is a character with code 0x7F stored in XXX.

    Video board output is controlled by
    - control flag 0 "disable video": 0 == disable
    - control flag 1 "cursor": 0 == if this scan line is one of extra 3,
      enable video every 5 frames.
    - control flag 3 "status line": 0 == current scan line is part of status line
    - keyboard mode 'RED' ('display controls'): if character code is
      less than 0x20 and RED is set, enable video every 5 frames; if RED is
      unset, disable video.

    XXX 'dotted look' is caused by strobe at 2x pixel clock -- use HLSL for this.
*/

UINT32 ie15_device::draw_scanline(UINT16 *p, UINT16 offset, UINT8 scanline)
{
	UINT8 gfx, fg, bg, ra, blink, red;
	UINT16 x, chr;

	bg = 0; fg = 1; ra = scanline % 8;
	blink = (m_screen->frame_number() % 10) > 4;
	red = m_io_keyboard->read() & IE_KB_RED;

	for (x = offset; x < offset + 80; x++)
	{
		chr = m_p_videoram[x] << 3;
		gfx = m_p_chargen[chr | ra];

		if (scanline > 7 && (!m_video.cursor || blink))
			gfx = 0;
		else if (chr < (0x20<<3)) {
			if (red && blink && m_video.line25)
				gfx = m_p_chargen[chr | 0x200 | ra];
			else
				gfx = 0;
		}

		*p++ = BIT(gfx, 7) ? fg : bg;
		*p++ = BIT(gfx, 6) ? fg : bg;
		*p++ = BIT(gfx, 5) ? fg : bg;
		*p++ = BIT(gfx, 4) ? fg : bg;
		*p++ = BIT(gfx, 3) ? fg : bg;
		*p++ = BIT(gfx, 2) ? fg : bg;
		*p++ = BIT(gfx, 1) ? fg : bg;
		*p++ = bg;
		*p++ = bg;
		*p++ = bg;
	}
	return 0;
}

void ie15_device::update_leds()
{
#if 0
	UINT8 data = m_io_keyboard->read();
	output().set_value("lat_led", m_kb_ruslat ^ 1);
	output().set_value("nr_led", BIT(m_kb_control, IE_KB_NR_BIT) ^ 1);
	output().set_value("pch_led", BIT(data, IE_KB_PCH_BIT) ^ 1);
	output().set_value("dup_led", BIT(data, IE_KB_DUP_BIT) ^ 1);
	output().set_value("lin_led", BIT(data, IE_KB_LIN_BIT) ^ 1);
	output().set_value("red_led", BIT(data, IE_KB_RED_BIT) ^ 1);
	output().set_value("sdv_led", BIT(m_kb_control, IE_KB_SDV_BIT) ^ 1);
	output().set_value("prd_led", 1); // XXX
#endif
}

/*
    VBlank is active for 3 topmost on-screen rows and 1 at the bottom; however, control flag 3 overrides VBlank,
    allowing status line to be switched on and off.
*/
TIMER_DEVICE_CALLBACK_MEMBER(ie15_device::scanline_callback)
{
	UINT16 y = m_screen->vpos();

	DBG_LOG(3,"scanline_cb",
		("addr %03x frame %d x %.4d y %.3d row %.2d e:c:s %d:%d:%d\n",
		m_video.ptr2, (int)m_screen->frame_number(), m_screen->hpos(), y,
		y%11, m_video.enable, m_video.cursor, m_video.line25));

	if (y < IE15_VERT_START) return;
	y -= IE15_VERT_START;
	if (y >= IE15_DISP_VERT) return;

	if (!m_video.enable || (y < IE15_STATUSLINE && m_video.line25)) {
		copybitmap(m_tmpbmp, m_offbmp, 0, 0, 0, y, m_tmpclip);
	} else {
		draw_scanline(&m_tmpbmp.pix16(y), m_video.ptr2, y%11);
	}
}

UINT32 ie15_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	update_leds();
	copybitmap(bitmap, m_tmpbmp, 0, 0, IE15_HORZ_START, IE15_VERT_START, cliprect);
	return 0;
}


/* F4 Character Displayer */
static const gfx_layout ie15_charlayout =
{
	7, 8,                   /* 7x8 pixels in 10x11 cell */
	256,                    /* 256 characters */
	1,                  /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8                 /* every char takes 8 bytes */
};

static GFXDECODE_START( ie15 )
	GFXDECODE_ENTRY("chargen", 0x0000, ie15_charlayout, 0, 1)
GFXDECODE_END

PALETTE_INIT_MEMBER( ie15_device, ie15 )
{
	palette.set_pen_color(0, rgb_t::black); // black
	palette.set_pen_color(1, 0x00, 0xc0, 0x00); // green
}

static MACHINE_CONFIG_FRAGMENT( ie15 )
	/* Basic machine hardware */
	MCFG_CPU_ADD("maincpu", IE15_CPU, XTAL_30_8MHz/10)
	MCFG_CPU_PROGRAM_MAP(ie15_mem)
	MCFG_CPU_IO_MAP(ie15_io)
	MCFG_TIMER_DRIVER_ADD_PERIODIC("scantimer", ie15_device, scanline_callback, attotime::from_hz(50*28*11))
	MCFG_TIMER_START_DELAY(attotime::from_hz(XTAL_30_8MHz/(2*IE15_HORZ_START)))

	/* Video hardware */
	MCFG_SCREEN_ADD_MONOCHROME("screen", RASTER, rgb_t::green)
	MCFG_SCREEN_UPDATE_DRIVER(ie15_device, screen_update)
	MCFG_SCREEN_RAW_PARAMS(XTAL_30_8MHz/2, IE15_TOTAL_HORZ, IE15_HORZ_START,
		IE15_HORZ_START+IE15_DISP_HORZ, IE15_TOTAL_VERT, IE15_VERT_START,
		IE15_VERT_START+IE15_DISP_VERT);

	MCFG_SCREEN_PALETTE("palette")

	MCFG_GFXDECODE_ADD("gfxdecode", "palette", ie15)
	MCFG_PALETTE_ADD_MONOCHROME("palette")

	/* Devices */
	MCFG_DEVICE_ADD("keyboard", IE15_KEYBOARD, 0)
	MCFG_IE15_KEYBOARD_CB(WRITE16(ie15_device, kbd_put))

	MCFG_RS232_PORT_ADD("rs232", default_rs232_devices, "null_modem")
	MCFG_RS232_RXD_HANDLER(WRITELINE(ie15_device, serial_rx_callback))

	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("beeper", BEEP, 2400)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)
MACHINE_CONFIG_END

ROM_START( ie15 )
	ROM_REGION(0x1000, "maincpu", ROMREGION_ERASE00)
	ROM_DEFAULT_BIOS("5chip")
	ROM_SYSTEM_BIOS(0, "5chip", "5-chip firmware (newer)")
	ROMX_LOAD("dump1.bin", 0x0000, 0x1000, CRC(14b82284) SHA1(5ac4159fbb1c3b81445605e26cd97a713ae12b5f), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "6chip", "6-chip firmware (older)")
	ROMX_LOAD("dump5.bin", 0x0000, 0x1000, CRC(01f2e065) SHA1(2b72dc0594e38a528400cd25aed0c47e0c432895), ROM_BIOS(2))

	ROM_REGION(0x1000, "video", ROMREGION_ERASE00)

	ROM_REGION(0x0800, "chargen", ROMREGION_ERASE00)
#if 1
	ROM_LOAD("chargen-15ie.bin", 0x0000, 0x0800, CRC(ed16bf6b) SHA1(6af9fb75f5375943d5c0ce9ed408e0fb4621b17e))
#else
	ROMX_LOAD( "1_d18.rt4",  0x0100, 0x0100, CRC(1c5b3f53) SHA1(45d00e41a88c5abac8b5d78d7aeaf558e8480d34), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO )
	ROMX_LOAD( "1_d18.rt4",  0x0500, 0x0100, CRC(1c5b3f53) SHA1(45d00e41a88c5abac8b5d78d7aeaf558e8480d34), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO )
	ROMX_LOAD( "2_d19.rt4",  0x0300, 0x0100, CRC(17351d55) SHA1(9ab134e93e753121f15344b15efca420cc806bcd), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO )
	ROMX_LOAD( "3_d20.rt4",  0x0200, 0x0100, CRC(10b537b9) SHA1(add655c550fd74351c95ae62c1160a525db76931), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO )
	ROMX_LOAD( "4_d21.rt4",  0x0700, 0x0100, CRC(1edb2de8) SHA1(3910f746c97875f6bec307faa566b562791d1a46), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO )
	ROMX_LOAD( "5_d22.rt4",  0x0600, 0x0100, CRC(135b978a) SHA1(e754d5beb2bb26b45db354afe004f1ab2d589412), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO )
	ROMX_LOAD( "6_d23.rt4",  0x0100, 0x0100, CRC(1459e51b) SHA1(e1666e8afb31447e9e5b88312144b346f6c948a4), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI )
	ROMX_LOAD( "6_d23.rt4",  0x0500, 0x0100, CRC(1459e51b) SHA1(e1666e8afb31447e9e5b88312144b346f6c948a4), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI )
	ROMX_LOAD( "7_d24.rt4",  0x0300, 0x0100, CRC(1bf3184b) SHA1(0517bc1ab071f6183dc9a4ff2792603f346938a6), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI )
	ROMX_LOAD( "8_d25.rt4",  0x0200, 0x0100, CRC(1246a05c) SHA1(b618d7af7391890ebc4e80cc9cd707ef130e7e04), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI )
	ROMX_LOAD( "9_d26.rt4",  0x0700, 0x0100, CRC(2246a05c) SHA1(f648d3d6bccdb29eafb9f89158b21579636e44c7), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI )
	ROMX_LOAD( "10_d27.rt4", 0x0600, 0x0100, CRC(3246a05c) SHA1(c1e34924fd5148e562c1e851a26c68713494a08f), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI )
#endif
ROM_END

INPUT_PORTS_START( ie15 )
	PORT_START("io_keyboard")
	PORT_DIPNAME(IE_KB_RED, IE_KB_RED, "RED (Interpret controls)")
	PORT_DIPSETTING(0x00, "Off")
	PORT_DIPSETTING(IE_KB_RED, "On")
	PORT_DIPNAME(IE_KB_DUP, IE_KB_DUP, "DUP (Full duplex)")
	PORT_DIPSETTING(0x00, "Off")
	PORT_DIPSETTING(IE_KB_DUP, "On")
	PORT_DIPNAME(IE_KB_LIN, IE_KB_LIN, "LIN (Online)")
	PORT_DIPSETTING(0x00, "Off")
	PORT_DIPSETTING(IE_KB_LIN, "On")
INPUT_PORTS_END

machine_config_constructor ie15_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME(ie15);
}

ioport_constructor ie15_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(ie15);
}

const tiny_rom_entry *ie15_device::device_rom_region() const
{
	return ROM_NAME(ie15);
}

const device_type IE15 = &device_creator<ie15_device>;
