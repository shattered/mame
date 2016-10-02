// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/**********************************************************************

    Qbus cards for ex-USSR PDP-11 clones

**********************************************************************/

#include "mpi_cards.h"

SLOT_INTERFACE_START( mpi_cards )
	SLOT_INTERFACE("vhd", RT11_VHD)		// Virtual disk
	SLOT_INTERFACE("pc11", DEC_PC11)	// Interface I8, I9 ??
	SLOT_INTERFACE("rx11", DEC_RX11)	// Interface I4
//	SLOT_INTERFACE("rk11", DEC_RK11)	// Interface I15 + I16
//	SLOT_INTERFACE("mt11", DEC_MT11)	// Interface I17
	SLOT_INTERFACE("kgd", DVK_KGD)		// Monochrome graphics
	SLOT_INTERFACE("kngmd", DVK_KNGMD)	// FM floppy controller (device MX:)
	SLOT_INTERFACE("kzd", DVK_KZD)		// MFM harddisk controller (device DW:)
SLOT_INTERFACE_END

SLOT_INTERFACE_START( uknc_cards )
	SLOT_INTERFACE("vhd", RT11_VHD)		// Virtual disk
//	SLOT_INTERFACE("lan", UKNC_LAN)		// Serial network adapter
//	SLOT_INTERFACE("rtc", UKNC_RTC)		// RAM disk + real time clock
SLOT_INTERFACE_END

SLOT_INTERFACE_START( uknc_carts )
	SLOT_INTERFACE("rom", UKNC_ROM)		// Standard ROM cart
//	SLOT_INTERFACE("fdc", UKNC_FDC)		// Standard floppy controller
//	SLOT_INTERFACE("ram_ed", UKNC_RAM_ED)	// RAM disk (device ED:)
//	SLOT_INTERFACE("ram_me", UKNC_RAM_ME)	// RAM disk (device ME: or RE:)
//	SLOT_INTERFACE("ide_xx", UKNC_IDE_XX)	// IDE disk controller (device xx:)
	SLOT_INTERFACE("ide_wd", UKNC_IDE_WD)	// IDE disk controller (device WD:)
//	SLOT_INTERFACE("joy", UKNC_JOY)		// Joystick adapter
//	SLOT_INTERFACE("mfm", UKNC_MFM)		// MFM harddisk controller
SLOT_INTERFACE_END

