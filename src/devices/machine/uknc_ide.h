// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
#pragma once

#ifndef __UKNC_IDE_H__
#define __UKNC_IDE_H__

#include "emu.h"

#include "machine/ataintf.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> uknc_ide_device

class uknc_ide_device : public device_t
{
public:
	uknc_ide_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const override;

	READ16_MEMBER(read);
	WRITE16_MEMBER(write);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	required_device<ata_interface_device> m_ide;
};


// device type definition
extern const device_type UKNC_IDE;

#endif  /* __UKNC_IDE_H__ */
