// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

    15WWW-97-006 keyboard, normally used with 15IE-00-013.
    Irisha can use it too.

***************************************************************************/

#include "emu.h"
#include "machine/ie15_kbd.h"

#include "machine/keyboard.ipp"

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

DEFINE_DEVICE_TYPE(IE15_KEYBOARD, ie15_keyboard_device, "ie15kbd", "15WWW-97-006 Keyboard")

ie15_keyboard_device::ie15_keyboard_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, device_matrix_keyboard_interface(mconfig, *this, "TERM_LINE0", "TERM_LINE1", "TERM_LINE2", "TERM_LINE3")
	, m_io_kbdc(*this, "TERM_LINEC")
	, m_keyboard_cb(*this)
	, m_sdv_cb(*this)
{
}

ie15_keyboard_device::ie15_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: ie15_keyboard_device(mconfig, IE15_KEYBOARD, tag, owner, clock)
{
}

INPUT_CHANGED_MEMBER(ie15_keyboard_device::dip_changed)
{
	m_sdv_cb(newval);
}

void ie15_keyboard_device::key_make(uint8_t row, uint8_t column)
{
	uint16_t key_code = 0;
	int ctrl  = BIT(m_io_kbdc->read(), 0);
	int shift = BIT(m_io_kbdc->read(), 1);

	if (m_ruslat)
		row += 4;
	if (shift)
		row += 8;

	if (column < 16)
		key_code = m_rom [(row << 4) + column];
	else
		key_code = m_rom [(row << 4) + (column - 16) * 2 + 256];

	if (ctrl)
		key_code &= 0x1f;

	// XXX aux keyboard flag
	if ((column > 12 && column < 16) || (column > 20 && column < 24))
	{
		key_code |= (IE_KB_DK << 8);
	}

	logerror("key_make row %d col %d key_code %04X (%02X)\n", row, column, key_code, key_code & 0x7f);

	m_keyboard_cb(offs_t(0), key_code);
}

/***************************************************************************
    VIDEO HARDWARE
***************************************************************************/

ROM_START( ie15_keyboard )
	ROM_REGION( 0x200, "ie15kbd", 0 )
	ROM_LOAD( "15bbb.rt5", 0x000, 0x200, CRC(e6a4226e) SHA1(0ee46f5be1b01fa917a6d483bb51463106ae441f) )
ROM_END

const tiny_rom_entry *ie15_keyboard_device::device_rom_region() const
{
	return ROM_NAME( ie15_keyboard );
}


void ie15_keyboard_device::device_start()
{
	m_keyboard_cb.resolve_safe();
	m_sdv_cb.resolve_safe();

	m_rom = (uint8_t*)memregion("ie15kbd")->base();
}

void ie15_keyboard_device::device_reset()
{
	m_ruslat = false;

	reset_key_state();
	start_processing(attotime::from_hz(1'000));
}

/*
Y1  Y2  Y3  Y4  Y5  Y6  Y7  Y8  Y9  Y10 Y11 Y12 Y13 Y14 Y15 Y16 Y17 Y18 Y19 Y20 Y21 Y22 Y23 Y24
--
;+  1!  2"  3#  4$  5%  6&  7'  8(  9)  0   -=      7   8   9   ТАБ ГТ  СБР СТР СТС f1  f2  f3
ЙJ  ЦC  УU  КK  ЕE  НN  ГG  Ш[  Щ]  ЗZ  ХH  :*      4   5   6   ПС  ВК  АР1 С1  АР2 f4  f5  f6
ФF  ЫY  ВW  АA  ПP  РR  ОO  ЛL  ДD  ЖV  Э\  .>  ЗБ  1   2   3           ПРД ПРМ ПРС f7  f8  f9
ЯQ  Ч^  СS  МM  ИI  ТT  ЬX  БB  Ю@  ,<  /?  _   SPC 0       ,                       fA  fB  fC

rom:

00000000  3b 31 32 33 34 35 36 37  38 39 30 2d 00 37 38 39  |;1234567890-.789| rus + upper
00000010  6a 63 75 6b 65 6e 67 7b  7d 7a 68 3a 00 34 35 36  |jcukeng{}zh:.456|
00000020  66 79 77 61 70 72 6f 6c  64 76 7c 2e 7f 31 32 33  |fywaproldv|..123|
00000030  71 7e 73 6d 69 74 78 62  60 2c 2f 7f 20 30 30 2c  |q~smitxb`,/. 00,|

00000040  3b 31 32 33 34 35 36 37  38 39 30 2d 00 37 38 39  |;1234567890-.789| lat + upper
00000050  4a 43 55 4b 45 4e 47 5b  5d 5a 48 3a 00 34 35 36  |JCUKENG[]ZH:.456|
00000060  46 59 57 41 50 52 4f 4c  44 56 5c 2e 7f 31 32 33  |FYWAPROLDV\..123|
00000070  51 5e 53 4d 49 54 58 42  40 2c 2f 5f 20 30 30 2c  |Q^SMITXB@,/_ 00,|

00000080  2b 21 22 23 24 25 26 27  28 29 30 3d 00 37 38 39  |+!"#$%&'()0=.789| rus + lower
00000090  4a 43 55 4b 45 4e 47 5b  5d 5a 48 2a 00 34 35 36  |JCUKENG[]ZH*.456|
000000a0  46 59 57 41 50 52 4f 4c  44 56 5c 3e 7f 31 32 33  |FYWAPROLDV\>.123|
000000b0  51 5e 53 4d 49 54 58 42  40 3c 3f 5f 20 30 30 2c  |Q^SMITXB@<?_ 00,|

000000c0  2b 21 22 23 24 25 26 27  28 29 30 3d 00 37 38 39  |+!"#$%&'()0=.789| lat + lower
000000d0  6a 63 75 6b 65 6e 67 7b  7d 7a 68 2a 00 34 35 36  |jcukeng{}zh*.456|
000000e0  66 79 77 61 70 72 6f 6c  64 76 7c 3e 7f 31 32 33  |fywaproldv|>.123|
000000f0  71 7e 73 6d 69 74 78 62  60 3c 3f 7f 20 30 30 2c  |q~smitxb`<?. 00,|

00000100  18 18 09 09 0c 0c 1f 1f  0b 0b 15 15 1c 1c 0d 0d  |................|
00000110  0a 0a 0d 0d 10 10 01 01  1b 1b 1a 1a 08 08 19 19  |................|
00000120  7f 7f 7f 7f 1e 1e 11 11  06 06 14 14 1d 1d 13 13  |................|
00000130  0e 0e 0f 0f 7f 7f 7f 7f  7f 7f 16 16 02 02 12 12  |................|
*/

static INPUT_PORTS_START( ie15_keyboard )
	PORT_START("TERM_LINE0")
	PORT_BIT(0x000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x000800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x001000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAB (VT)") PORT_CODE(KEYCODE_F11) PORT_CHAR(UCHAR_MAMEKEY(F11)) // Vertical Tab
	PORT_BIT(0x020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SBR") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STR") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STS") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0x200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))   // 'f1'
	PORT_BIT(0x400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP)) // 'f2'
	PORT_BIT(0x800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))   // 'f3'

	PORT_START("TERM_LINE1")
	PORT_BIT(0x000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('j') PORT_CHAR(0x0a)
	PORT_BIT(0x000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('c')
	PORT_BIT(0x000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('u')
	PORT_BIT(0x000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('k')
	PORT_BIT(0x000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('e')
	PORT_BIT(0x000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('n')
	PORT_BIT(0x000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('g')
	PORT_BIT(0x000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('z')
	PORT_BIT(0x000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('h')
	PORT_BIT(0x000800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x001000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PS (LF)") PORT_CODE(KEYCODE_F12) PORT_CHAR(UCHAR_MAMEKEY(F12))  // Line Feed
	PORT_BIT(0x020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("AR1") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S1")  PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("AR2 (Escape)") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Begin") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
	PORT_BIT(0x800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("TERM_LINE2")
	PORT_BIT(0x000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('f')
	PORT_BIT(0x000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR('y')
	PORT_BIT(0x000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('w')
	PORT_BIT(0x000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('a')
	PORT_BIT(0x000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR('p')
	PORT_BIT(0x000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('r')
	PORT_BIT(0x000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('o')
	PORT_BIT(0x000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('l')
	PORT_BIT(0x000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('d')
	PORT_BIT(0x000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('v')
	PORT_BIT(0x000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x000800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x010000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x020000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PRD") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PRM") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PRS") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ins") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))

	PORT_START("TERM_LINE3")
	PORT_BIT(0x000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('q')
	PORT_BIT(0x000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('s')
	PORT_BIT(0x000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('m')
	PORT_BIT(0x000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('i')
	PORT_BIT(0x000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR('t')
	PORT_BIT(0x000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('x')
	PORT_BIT(0x000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('b')
	PORT_BIT(0x000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x000800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('_')
	PORT_BIT(0x001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x004000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(COMMA_PAD))
	PORT_BIT(0x010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Lat (SO)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Rus (SI)") PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x040000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x080000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x100000, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PgDn") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT(0x400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Next SetUp") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT(0x800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PgUp") PORT_CODE(KEYCODE_PGUP) PORT_CHAR(UCHAR_MAMEKEY(PGUP))

	PORT_START("TERM_LINEC")
	PORT_BIT(0x000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1) // PORT_TOGGLE
	PORT_BIT(0x000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SetUp") PORT_CODE(KEYCODE_MINUS_PAD) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD)) PORT_CHANGED_MEMBER(DEVICE_SELF, ie15_keyboard_device, dip_changed, 0)
INPUT_PORTS_END

ioport_constructor ie15_keyboard_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(ie15_keyboard);
}
