// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/**********************************************************************

    Digital Equipment Corp. LSI-11 Bus (Qbus) emulation

**********************************************************************

via http://www.ibiblio.org/pub/academic/computer-science/history/pdp-11/hardware/qbus.backplane


BIRQ5	AA1/-:--	    MSpareB AL1/-:--		BDCOK	BA1/D:A1	    MSpareB BL1/-:--
+5		AA2/-:--	    BIRQ4   AL2/C:B10		+5		BA2/B:B1	    BDAL07  BL2/B:B10
BIRQ6	AB1/-:--	    GND	    AM1/-:--		BPOK	BB1/D:A2	    GND	    BM1/B:A11
-12		AB2/-:--	    BIAKI   AM2/C:B11		-12		BB2/-:--	    BDAL08  BM2/B:B11
BDAL16  AC1/-:--	    BDMR    AN1/C:A12		BDAL18  BC1/-:--	    BSACK   BN1/B:A12
GND		AC2/-:--	    BIAKO   AN2/C:B12		GND		BC2/B:B3	    BDAL09  BN2/B:B12
BDAL17  AD1/-:--	    BHALT   AP1/C:A7		BDAL19  BD1/-:--	    BIRQ7   BP1/-:--
+12		AD2/-:--	    BBS7    AP2/A:B13		+12		BD2/-:--	    BDAL10  BP2/B:B13
SSpare1 AE1/-:--	    BREF    AR1/A:A14		BDAL20  BE1/-:--	    BEVNT   BR1/B:A14
BDOUT	AE2/A:B5	    BDMRI   AR2/C:B14		BDAL02  BE2/B:B5	    BDAL11  BR2/B:B14
SSpare2 AF1/-:--	    +5B	    AS1/-:--		BDAL21  BF1/-:--	    +12B    BS1/-:--
BRPLY	AF2/C:B6	    BDMRO   AS2/C:B15		BDAL03  BF2/B:B6	    BDAL12  BS2/B:B15
SSpare3 AH1/-:--	    GND	    AT1/-:--		SSpare8 BH1/B:A7	    GND	    BT1/B:A16
BDIN	AH2/A:B7	    BINIT   AT2/A:B16		BDAL04  BH2/B:B7	    BDAL13  BT2/B:B16
GND		AJ1/-:--	    PSpare1 AU1/-:--		GND		BJ1/B:A8	    ASpare2 BU1/-:--
BSYNC	AJ2/A:B8	    BDAL00  AU2/A:B17		BDAL05  BJ2/B:B8	    BDAL14  BU2/B:B17
MSpareA AK1/-:--	    +5B	    AV1/-:--		MSpareB BK1/-:--	    +5	    BV1/-:--
BWTBT	AK2/A:B9	    BDAL01  AV2/A:B18		BDAL06  BK2/B:B9	    BDAL15  BV2/B:B18

ordered by signal name:

+12		AD2	    BDAL06  BK2		BDMRI	AR2	    BWTBT   AK2
+12		BD2	    BDAL07  BL2		BDMRO	AS2	    GND	    AC2
+12B	BS1	    BDAL08  BM2		BDMR	AN1	    GND	    AJ1
+5		AA2	    BDAL09  BN2		BDOUT	AE2	    GND	    AM1
+5		BA2	    BDAL10  BP2		BEVNT	BR1	    GND	    AT1
+5		BV1	    BDAL11  BR2		BHALT	AP1	    GND	    BC2
+5B		AS1	    BDAL12  BS2		BIAKI	AM2	    GND	    BJ1
+5B		AV1	    BDAL13  BT2		BIAKO	AN2	    GND	    BM1
-12		AB2	    BDAL14  BU2		BINIT	AT2	    GND	    BT1
-12		BB2	    BDAL15  BV2		BIRQ4	AL2	    MSpareA AK1
ASpare2 BU1	    BDAL16  AC1		BIRQ5	AA1	    MSpareB AL1
BBS7	AP2	    BDAL17  AD1		BIRQ6	AB1	    MSpareB BK1
BDAL00  AU2	    BDAL18  BC1		BIRQ7	BP1	    MSpareB BL1
BDAL01  AV2	    BDAL19  BD1		BPOK	BB1	    PSpare1 AU1
BDAL02  BE2	    BDAL20  BE1		BREF	AR1	    SSpare1 AE1
BDAL03  BF2	    BDAL21  BF1		BRPLY	AF2	    SSpare2 AF1
BDAL04  BH2	    BDCOK   BA1		BSACK	BN1	    SSpare3 AH1
BDAL05  BJ2	    BDIN    AH2		BSYNC	AJ2	    SSpare8 BH1

uknc bus:

BDAL00..15
BSYNC
BDIN
BDOUT
BWTBT
BRPLY
BINIT
BBS7
BIRQ4
BIAKO
BDMR
BDMRO
BSACK
BDCOK
BPOK
'reg2'
'40/80'
'CE0'
'CE1'
'CE2'
'CE3' || '/CE3'
'4000kHz'
'A16'


MPI is nearly identical to Qbus, with different pin and signal naming, plus
a few different pin assignments.  Standardized in OST 11.305.903-80 and
GOST 26765.51-86.

ms1201 boards are 16-bit and support only 1 level of interrupts, and so 
don't carry BDAL16..21 and BIRQ5..7.  Also:
- SSpare8 carries 50 Hz line clock signal;
- BIAKO and BDMRO are apparently duplicated on A:B12 and A:B15;
- BDAL19 is REZ1 and is optionally connected via S2 to pim 10 ("WRQ") on CPU
- BDAL21 is REZ2 and is optionally connected via S1 to pin 11 ("WAKI") on CPU

from device to cpu
- BIRQ4
- BDMR
(optional)
- BIRQ5..7
- BHALT
- BEVNT
- BWTBT
- BPOK
- BDCOK

from cpu to device
- BINIT
- BIAKI
- BDMRI
(optional)
- BBS7

from device to next device
- BIAKO
- BDMRO

**********************************************************************/

#pragma once

#ifndef __QBUS_SLOT__
#define __QBUS_SLOT__

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_QBUS_CPU(_cputag) \
	qbus_device::static_set_cputag(*device, _cputag);

#define MCFG_QBUS_SLOT_ADD(_qbustag, _tag, _clock, _slot_intf, _def_slot) \
	MCFG_DEVICE_ADD(_tag, QBUS_SLOT, _clock) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, false) \
	qbus_slot_device::static_set_qbus_slot(*device, owner, _qbustag);


#define MCFG_QBUS_OUT_BIRQ4_CB(_devcb) \
	devcb = &qbus_device::set_out_birq4_callback(*device, DEVCB_##_devcb);

#define MCFG_QBUS_OUT_BDMR_CB(_devcb) \
	devcb = &qbus_device::set_out_bdmr_callback(*device, DEVCB_##_devcb);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> qbus_device

class qbus_device : public device_t,
					public device_memory_interface
{
public:
	// construction/destruction
	qbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	qbus_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);

	// inline configuration
	static void static_set_cputag(device_t &device, const char *tag);
	template<class _Object> static devcb_base &set_out_birq4_callback(device_t &device, _Object object) { return downcast<qbus_device &>(device).m_out_birq4_cb.set_callback(object); }
	template<class _Object> static devcb_base &set_out_bdmr_callback(device_t &device, _Object object) { return downcast<qbus_device &>(device).m_out_bdmr_cb.set_callback(object); }

	virtual const address_space_config *memory_space_config(address_spacenum spacenum) const override
	{
		return &m_program_config;
	}

	void install_device(offs_t start, offs_t end, read16_delegate rhandler, write16_delegate whandler);
	template<typename T> void install_device(offs_t addrstart, offs_t addrend, T &device, void (T::*map)(class address_map &map, device_t &device), int bits = 16, UINT64 unitmask = U64(0xffffffffffffffff))
	{
		m_prgspace->install_device(addrstart, addrend, device, map, bits, unitmask);
	}

	DECLARE_WRITE_LINE_MEMBER( birq4_w );
	DECLARE_WRITE_LINE_MEMBER( birq5_w );
	DECLARE_WRITE_LINE_MEMBER( birq6_w );
	DECLARE_WRITE_LINE_MEMBER( birq7_w );

	DECLARE_WRITE_LINE_MEMBER( bdmr_w );

	const address_space_config m_program_config;

protected:
	void install_space(address_spacenum spacenum, offs_t start, offs_t end, read16_delegate rhandler, write16_delegate whandler);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_config_complete() override;

	// internal state
	cpu_device   *m_maincpu;

	// address spaces
	address_space *m_prgspace;
	int m_prgwidth;

	devcb_write_line    m_out_birq4_cb;
	devcb_write_line    m_out_birq5_cb;
	devcb_write_line    m_out_birq6_cb;
	devcb_write_line    m_out_birq7_cb;
	devcb_write_line    m_out_bdmr_cb;

	const char          *m_cputag;
};

// device type definition
extern const device_type QBUS;


// ======================> device_qbus_card_interface

class qbus_slot_device;

class device_qbus_card_interface : public device_slot_card_interface
{
	friend class qbus_device;
public:
	// construction/destruction
	device_qbus_card_interface(const machine_config &mconfig, device_t &device);

	device_qbus_card_interface *next() const { return m_next; }

	void set_qbus_device();

	// device_qbus_card_interface overrides
	virtual void biaki_w(int state) { }
	virtual void bdmgi_w(int state) { }

	// inline configuration
	static void static_set_qbus(device_t &device, device_t *qbus_device);

	qbus_device  *m_qbus;
	device_t     *m_qbus_dev;
	device_qbus_card_interface *m_next;

protected:
	qbus_slot_device *m_slot;
};


// ======================> qbus_slot_device

class qbus_slot_device : public device_t,
							public device_slot_interface
{
public:
	// construction/destruction
	qbus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// inline configuration
	static void static_set_qbus_slot(device_t &device, device_t *owner, const char *qbus_tag);

	// computer interface
	DECLARE_WRITE_LINE_MEMBER( biaki_w ) { if (m_card) m_card->biaki_w(state); }
	DECLARE_WRITE_LINE_MEMBER( bdmgi_w ) { if (m_card) m_card->bdmgi_w(state); }

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override { if (m_card) get_card_device()->reset(); }

	devcb_write_line   m_write_birq4;
	devcb_write_line   m_write_bdmr;

	device_qbus_card_interface *m_card;

	// configuration
	device_t *m_owner;
	const char *m_qbus_tag;
};


// device type definition
extern const device_type QBUS_SLOT;


#endif
