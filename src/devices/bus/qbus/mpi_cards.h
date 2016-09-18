// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/**********************************************************************

    Qbus cards for ex-USSR PDP-11 clones

**********************************************************************/

#pragma once

#ifndef __MPI_CARDS_H__
#define __MPI_CARDS_H__

#include "emu.h"

// generic storage
#include "pc11.h"
#include "rt11_vhd.h"

// dvk video
#include "dvk_kgd.h"

// uknc cartridges
#include "uknc_rom.h"

// supported devices
SLOT_INTERFACE_EXTERN( mpi_cards );
SLOT_INTERFACE_EXTERN( uknc_cards );
SLOT_INTERFACE_EXTERN( uknc_carts );

#endif // __MPI_CARDS_H__
