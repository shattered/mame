// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev

#ifndef MAME_DEVICES_IE15_H
#define MAME_DEVICES_IE15_H

#include "bus/rs232/rs232.h"
#include "cpu/ie15/ie15.h"
#include "machine/ie15_kbd.h"
#include "sound/beep.h"

#define SCREEN_PAGE (80*48)

#define IE_TRUE         0x80
#define IE_FALSE    0

#define IE15_TOTAL_HORZ 1000
#define IE15_DISP_HORZ  800
#define IE15_HORZ_START 200

#define IE15_TOTAL_VERT 28*11
#define IE15_DISP_VERT  25*11
#define IE15_VERT_START 2*11
#define IE15_STATUSLINE 11


INPUT_PORTS_EXTERN(ie15);


class ie15_device : public device_t,
	public device_serial_interface
{
public:
	ie15_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);
	ie15_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_WRITE8_MEMBER(write) { term_write(data); }

	inline bitmap_ind16 *get_bitmap() { return &m_tmpbmp; }

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	virtual machine_config_constructor device_mconfig_additions() const override;
	virtual ioport_constructor device_input_ports() const override;
	virtual const tiny_rom_entry *device_rom_region() const override;

protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

	virtual void rcv_complete() override;
	virtual void tra_callback() override;
	virtual void tra_complete() override;

	void term_write(UINT8 data) { m_serial_rx_char = data; m_serial_rx_ready = IE_FALSE; }

public:
	DECLARE_WRITE_LINE_MEMBER( serial_rx_callback );
	TIMER_DEVICE_CALLBACK_MEMBER( scanline_callback );
	DECLARE_WRITE16_MEMBER( kbd_put );
	DECLARE_PALETTE_INIT( ie15 );

	DECLARE_WRITE8_MEMBER( mem_w );
	DECLARE_READ8_MEMBER( mem_r );
	DECLARE_WRITE8_MEMBER( mem_addr_lo_w );
	DECLARE_WRITE8_MEMBER( mem_addr_hi_w );
	DECLARE_WRITE8_MEMBER( mem_addr_inc_w );
	DECLARE_WRITE8_MEMBER( mem_addr_dec_w );
	DECLARE_READ8_MEMBER( flag_r );
	DECLARE_WRITE8_MEMBER( flag_w );
	DECLARE_WRITE8_MEMBER( beep_w );
	DECLARE_READ8_MEMBER( kb_r );
	DECLARE_READ8_MEMBER( kb_ready_r );
	DECLARE_READ8_MEMBER( kb_s_red_r );
	DECLARE_READ8_MEMBER( kb_s_sdv_r );
	DECLARE_READ8_MEMBER( kb_s_dk_r );
	DECLARE_READ8_MEMBER( kb_s_dupl_r );
	DECLARE_READ8_MEMBER( kb_s_lin_r );
	DECLARE_WRITE8_MEMBER( kb_ready_w );
	DECLARE_READ8_MEMBER( serial_tx_ready_r );
	DECLARE_WRITE8_MEMBER( serial_w );
	DECLARE_READ8_MEMBER( serial_rx_ready_r );
	DECLARE_READ8_MEMBER( serial_r );
	DECLARE_WRITE8_MEMBER( serial_speed_w );

private:
	TIMER_CALLBACK_MEMBER(ie15_beepoff);
	void update_leds();
	UINT32 draw_scanline(UINT16 *p, UINT16 offset, UINT8 scanline);
	rectangle m_tmpclip;
	bitmap_ind16 m_tmpbmp;
	bitmap_ind16 m_offbmp;

	const UINT8 *m_p_chargen;
	UINT8 *m_p_videoram;
	UINT8 m_long_beep;
	UINT8 m_kb_control;
	UINT8 m_kb_data;
	UINT8 m_kb_flag0;
	UINT8 m_kb_flag;
	UINT8 m_kb_ruslat;
	UINT8 m_latch;
	struct {
		UINT8 cursor;
		UINT8 enable;
		UINT8 line25;
		UINT32 ptr1;
		UINT32 ptr2;
	} m_video;

	UINT8 m_serial_rx_ready;
	UINT8 m_serial_rx_char;
	UINT8 m_serial_tx_ready;

protected:
	required_device<cpu_device> m_maincpu;
	required_device<beep_device> m_beeper;
	required_device<rs232_port_device> m_rs232;
	required_device<screen_device> m_screen;
	required_device<ie15_keyboard_device> m_keyboard;
	required_ioport m_io_keyboard;
};

extern const device_type IE15;

#endif /* MAME_DEVICES_IE15_H */
