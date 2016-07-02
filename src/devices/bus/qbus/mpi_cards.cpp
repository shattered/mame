// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/**********************************************************************

    Qbus cards for ex-USSR PDP-11 clones

**********************************************************************/

#include "mpi_cards.h"

SLOT_INTERFACE_START( mpi_cards )
	SLOT_INTERFACE("pc11", DEC_PC11)	// Interface I8, I9 ??
//	SLOT_INTERFACE("rk11", DEC_RK11)	// Interface I15 + I16
//	SLOT_INTERFACE("mt11", DEC_MT11)	// Interface I17
	SLOT_INTERFACE("kgd", DVK_KGD)		// Monochrome graphics
	SLOT_INTERFACE("vhd", RT11_VHD)		// Virtual disk
SLOT_INTERFACE_END

