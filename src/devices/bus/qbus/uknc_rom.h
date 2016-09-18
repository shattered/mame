// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

	UKNC ROM cartridge

***************************************************************************/

#pragma once

#ifndef __UKNC_ROM__
#define __UKNC_ROM__

#include "emu.h"

#include "qbus.h"

#include "includes/pdp11.h"


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> uknc_rom_device

class uknc_rom_device : public device_t,
					public device_image_interface,
					public device_qbus_card_interface
{
public:
	// construction/destruction
	uknc_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// image-level overrides
	virtual iodevice_t image_type() const override { return IO_ROM; }

	virtual bool is_readable()  const override { return 1; }
	virtual bool is_writeable() const override { return 0; }
	virtual bool is_creatable() const override { return 0; }
	virtual bool must_be_loaded() const override { return 0; }
	virtual bool is_reset_on_load() const override { return 0; }
	virtual const char *image_interface() const override { return nullptr; }
	virtual const char *file_extensions() const override { return "bin,rom"; }

	virtual image_init_result call_load() override;
	virtual void call_unload() override;

	UINT8* base() { return m_base.get(); }

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
	// device-level overrides
	virtual void device_config_complete() override { update_names(); }
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_qbus_card_interface overrides
	virtual void biaki_w(int state) override { }
	virtual void ce0_ce3_w(int data) override;

private:
	int m_slot;

	std::unique_ptr<UINT8[]> m_base;
};


// device type definition
extern const device_type UKNC_ROM;


#endif
