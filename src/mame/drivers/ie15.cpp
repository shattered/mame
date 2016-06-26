// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

    15IE-00-013 Terminal

    A serial (RS232 or current loop) green-screen terminal, mostly VT52
    compatible (no Hold Screen mode and no graphics character set).

    Alternate character set (selected by SO/SI chars) is Cyrillic.

	todo: make layout work again

****************************************************************************/


#include "emu.h"

#include "machine/ie15.h"
#include "ie15.lh"


class ie15_state : public driver_device
{
public:
	ie15_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_ie15(*this, "ie15")
	{ }

private:
	virtual void machine_reset() override;
	virtual void machine_start() override;

	required_device<ie15_device> m_ie15;
};


void ie15_state::machine_reset()
{
}

void ie15_state::machine_start()
{
}


static MACHINE_CONFIG_START( ie15, ie15_state )
	MCFG_DEVICE_ADD("ie15", IE15, 0)

	MCFG_DEFAULT_LAYOUT(layout_ie15)
MACHINE_CONFIG_END


ROM_START(ie15)
ROM_END

/*    YEAR  NAME      PARENT  COMPAT   MACHINE    INPUT    INIT                      COMPANY     FULLNAME       FLAGS */
COMP( 1980, ie15,     0,      0,       ie15,      0,       driver_device,     0,     "USSR",     "15IE-00-013", 0)
