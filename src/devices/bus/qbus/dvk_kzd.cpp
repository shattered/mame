// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

  KZD MFM hard disk controller

  Maximum supported geometry: 1100 cylinders, 8 heads, 16 sectors ??

***************************************************************************/

#include "emu.h"

#include "dvk_kzd.h"


//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VERBOSE_DBG 1       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-16s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

enum
{
	REGISTER_ID = 0,	// 174000
	REGISTER_ERR = 2,	// 174004
	REGISTER_SECTOR,	// 174006
	REGISTER_DATA,		// 174010
	REGISTER_CYL,		// 174012
	REGISTER_HEAD,		// 174014
	REGISTER_CSR,		// 174016
	REGISTER_SI			// 174020
};


#define raise_zoa()	do { \
		DBG_LOG(3,"IRQ", ("raise_zoa\n")); \
		m_regs[REGISTER_SI] |= KZDSI_DONE; \
		raise_virq(m_qbus->birq4_w, m_regs[REGISTER_SI], CSR_IE, m_zoa); } while (0)

#define clear_zoa()	do { \
		DBG_LOG(3,"IRQ", ("clear_zoa\n")); \
		m_regs[REGISTER_SI] &= ~KZDSI_DONE; \
		clear_virq(m_qbus->birq4_w, m_regs[REGISTER_SI], CSR_IE, m_zoa); } while (0)

#define raise_zob()	do { \
		DBG_LOG(3,"IRQ", ("raise_zob\n")); \
		m_regs[REGISTER_SI] |= KZDSI_DRQ1; \
		raise_virq(m_qbus->birq4_w, m_regs[REGISTER_SI], CSR_IE, m_zob); } while (0)

#define clear_zob()	do { \
		DBG_LOG(3,"IRQ", ("clear_zob\n")); \
		m_regs[REGISTER_SI] &= ~KZDSI_DRQ1; \
		clear_virq(m_qbus->birq4_w, m_regs[REGISTER_SI], CSR_IE, m_zob); } while (0)

#define clear_err() \
		m_regs[REGISTER_ERR] &= ~KZDERR_WR;


int dvk_kzd_device::sector_to_lba(unsigned int cylinder, unsigned int head, unsigned int sector, UINT32 *lba)
{
	if (cylinder >= m_hd_geom->cylinders || head >= m_hd_geom->heads || sector >= m_hd_geom->sectors)
		return 1;

	*lba = ((cylinder*m_hd_geom->heads + head) * m_hd_geom->sectors) + sector;

	return 0;
}

DEVICE_IMAGE_LOAD_MEMBER( dvk_kzd_device, dvk_kzd_hd )
{
	m_hd_file = dynamic_cast<harddisk_image_device *>(&image)->get_hard_disk_file();
	if (!m_hd_file)
		return image_init_result::FAIL;

	m_hd_geom = hard_disk_get_info(m_hd_file);
	if (m_hd_geom->cylinders > 1024 ||
		m_hd_geom->heads > 4 ||
		m_hd_geom->sectors > 16 ||
		m_hd_geom->sectorbytes != 512)
	{
		return image_init_result::FAIL;
	}

	return image_init_result::PASS;
}

DEVICE_IMAGE_UNLOAD_MEMBER( dvk_kzd_device, dvk_kzd_hd )
{
	m_hd_file = nullptr;
}

READ16_MEMBER(dvk_kzd_device::read)
{
	UINT16 data = 0;

	switch (offset)
	{
	case REGISTER_ID:
		data = 0401;
		clear_zob();
		break;

	case REGISTER_ERR:
		data = m_regs[offset] & KZDERR_RD;
		break;

	case REGISTER_SECTOR:
		data = m_regs[offset] & KZDSEC_RDWR;
		clear_zoa();
		break;

	case REGISTER_DATA:
		if (m_count_read < 256) {
			data = m_buf[m_count_read++];
			clear_zob();
		}
		if (m_count_read == 256) {
			m_count_read = 0;
			m_regs[REGISTER_CSR] &= ~KZDCSR_DRQ2;
			m_timer_wait->adjust(attotime::never, 0, attotime::never);
			raise_zoa();
		} else
			m_timer_wait->adjust(attotime::from_usec(m_waittime));
		break;

	case REGISTER_CYL:
		data = m_regs[offset] & KZDCYL_RDWR;
		break;

	case REGISTER_HEAD:
		data = m_regs[offset] & KZDHED_RDWR;
		break;

	case REGISTER_CSR:
		data = m_regs[offset] & KZDCSR_RD;
		// fails interrupt test
//		clear_zoa();
		break;

	case REGISTER_SI:
		data = m_regs[offset] & KZDSI_RD;
		break;
	}

	DBG_LOG(2,"KZD R", ("%6o == %06o @ %d\n", 0174000+(offset<<1), data, m_count_read));

	return data;
}

WRITE16_MEMBER(dvk_kzd_device::write)
{
	DBG_LOG(2,"KZD W", ("%6o <- %06o & %06o%s @ %d\n", 0174000+(offset<<1), data, mem_mask, mem_mask==0xffff?"":" WARNING", m_count_write));

	switch (offset)
	{
	case REGISTER_ID:
		clear_zob();
		break;

	case REGISTER_ERR:
		UPDATE_16BIT(&m_regs[offset], data, KZDERR_WR);
		break;

	case REGISTER_SECTOR:
		UPDATE_16BIT(&m_regs[offset], data, KZDSEC_RDWR);
		m_regs[REGISTER_SI] &= ~KZDSI_DONE;
		clear_zoa();
		break;

	case REGISTER_CYL:
		UPDATE_16BIT(&m_regs[offset], data, KZDCYL_RDWR);
		break;

	case REGISTER_HEAD:
		UPDATE_16BIT(&m_regs[offset], data, KZDHED_RDWR);
		break;

	case REGISTER_DATA:
		if (m_count_write < 256) {
			m_buf[m_count_write++] = data;
			clear_zob();
		} 
		if (m_count_write == 256) {
			m_regs[REGISTER_SI] |= KZDSI_BUSY;
			m_timer_wait->adjust(attotime::never, 0, attotime::never);
			m_timer_write->adjust(attotime::from_msec(m_seektime));
		} else
			m_timer_wait->adjust(attotime::from_usec(m_waittime));
		break;

	case REGISTER_CSR:
		UPDATE_16BIT(&m_regs[offset], data, KZDCSR_WR);
		clear_zoa();
		command(data & KZDCSR_WR);
		break;

	case REGISTER_SI:
        if ((data & CSR_IE) == 0) {
			clear_virq(m_qbus->birq4_w, 1, 1, m_zoa);
			clear_virq(m_qbus->birq4_w, 1, 1, m_zob);
		} else if ((m_regs[offset] & (KZDSI_DONE + CSR_IE)) == KZDSI_DONE) {
			raise_virq(m_qbus->birq4_w, 1, 1, m_zoa);
		} else if ((m_regs[offset] & (KZDSI_DRQ1 + CSR_IE)) == KZDSI_DRQ1) {
			raise_virq(m_qbus->birq4_w, 1, 1, m_zob);
		}
		UPDATE_16BIT(&m_regs[offset], data, KZDSI_WR);
		if (data & KZDSI_INIT)
			device_reset();
		break;
	}
}

void dvk_kzd_device::command(int command)
{
	clear_err();

	switch(command)
	{
	case KZDCMD_TRK0:
		m_regs[REGISTER_SI] |= KZDSI_BUSY;
		m_timer_seek->adjust(attotime::from_msec(m_seektime));
		break;

	case KZDCMD_FORMAT:
		m_regs[REGISTER_CSR] |= KZDCSR_DRQ2;
		m_count_write = 0;
		raise_zob();
		break;

	case KZDCMD_READ:
		m_regs[REGISTER_SI] |= KZDSI_BUSY;
		m_count_read = 0;
		m_timer_read->adjust(attotime::from_msec(m_seektime));
		break;

	case KZDCMD_WRITE:
		m_regs[REGISTER_CSR] |= KZDCSR_DRQ2;
		m_count_write = 0;
		raise_zob();
		break;
	}
}

void dvk_kzd_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	DBG_LOG(2,"KZD Timer",("fired %d\n", id));
	UINT32 lba;

	switch(id) 
	{
		case TIMER_ID_TRK0:
			m_track = 0;
			m_regs[REGISTER_SI] &= ~KZDSI_BUSY;
			raise_zoa();
			break;

		case TIMER_ID_READ:
			if (!sector_to_lba(m_regs[REGISTER_CYL], m_regs[REGISTER_HEAD], m_regs[REGISTER_SECTOR], &lba)) {
				m_track = m_regs[REGISTER_CYL];
				if (hard_disk_read(m_hd_file, lba, (void *)&m_buf[0])) 
				{
					m_regs[REGISTER_SI] &= ~KZDSI_BUSY;
					m_regs[REGISTER_CSR] |= KZDCSR_DRQ2;
					raise_zob();
					DBG_LOG(1,"KZD", ("read c:h:s %3d:%d:%2d lba %5d CSR %06o SI %06o zoa %d zob %d\n", 
						m_regs[REGISTER_CYL], m_regs[REGISTER_HEAD], m_regs[REGISTER_SECTOR], lba, 
						m_regs[REGISTER_CSR], m_regs[REGISTER_SI], m_zoa, m_zob));
				} else {
					DBG_LOG(1,"KZD", ("FAILED(1) read c:h:s %d:%d:%d lba %d\n", 
						m_regs[REGISTER_CYL], m_regs[REGISTER_HEAD], m_regs[REGISTER_SECTOR], lba));
				}
			} else {
				DBG_LOG(1,"KZD", ("FAILED(2) read c:h:s %d:%d:%d lba %d\n", 
					m_regs[REGISTER_CYL], m_regs[REGISTER_HEAD], m_regs[REGISTER_SECTOR], lba));
				m_regs[REGISTER_ERR] |= KZDERR_AM;
				m_regs[REGISTER_CSR] |= KZDCSR_ERR;
			}
			break;

		case TIMER_ID_WRITE:
			DBG_LOG(1,"KZD", ("write to c:h:s %d:%d:%d\n", m_regs[REGISTER_CYL], m_regs[REGISTER_HEAD], m_regs[REGISTER_SECTOR]));
			m_count_write = 0;
			m_track = m_regs[REGISTER_CYL];
			m_regs[REGISTER_SI] &= ~KZDSI_BUSY;
			m_regs[REGISTER_CSR] &= ~KZDCSR_DRQ2;
			raise_zoa();
			break;

		case TIMER_ID_WAIT:
			raise_zob();
			break;
	}
}



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

static MACHINE_CONFIG_FRAGMENT( dvk_kzd )
	MCFG_HARDDISK_ADD("harddisk")
	MCFG_HARDDISK_LOAD(dvk_kzd_device, dvk_kzd_hd)
	MCFG_HARDDISK_UNLOAD(dvk_kzd_device, dvk_kzd_hd)
MACHINE_CONFIG_END

const device_type DVK_KZD = &device_creator<dvk_kzd_device>;


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  dvk_kzd_device - constructor
//-------------------------------------------------

dvk_kzd_device::dvk_kzd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
		: device_t(mconfig, DVK_KZD, "MFM Fixed Drive Adapter", tag, owner, clock, "dvk_kzd", __FILE__)
		, device_qbus_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dvk_kzd_device::device_start()
{
	set_qbus_device();

	// save state
	save_item(NAME(m_regs));
	save_item(NAME(m_count_read));
	save_item(NAME(m_count_write));
	save_item(NAME(m_track));
	save_item(NAME(m_seektime));
	save_item(NAME(m_waittime));

	m_timer_seek = timer_alloc(TIMER_ID_TRK0);
	m_timer_read = timer_alloc(TIMER_ID_READ);
	m_timer_write = timer_alloc(TIMER_ID_WRITE);
	m_timer_wait = timer_alloc(TIMER_ID_WAIT);

	m_buf = std::make_unique<UINT16[]>(256);
	m_seektime = 8; // ms. ST251-1
	m_waittime = 100; // us.

	m_qbus->install_device(0174000, 0174037, read16_delegate(FUNC(dvk_kzd_device::read),this), 
		write16_delegate(FUNC(dvk_kzd_device::write),this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void dvk_kzd_device::device_reset()
{
	DBG_LOG(1,"KZD",("Reset in progress\n"));

	memset(m_regs, 0, sizeof(m_regs));
	m_regs[REGISTER_ERR] = 128/4;
	m_regs[REGISTER_CSR] = KZDCSR_DONE|KZDCSR_DRDY;
//	m_regs[REGISTER_SI] = KZDSI_DONE|KZDSI_INIT|KZDSI_SLOW;
	m_regs[REGISTER_SI] = KZDSI_DONE|KZDSI_SLOW;

	m_zoa = m_zob = CLEAR_LINE;
	m_qbus->birq4_w(CLEAR_LINE);

	m_timer_seek->adjust(attotime::never, 0, attotime::never);
	m_timer_read->adjust(attotime::never, 0, attotime::never);
	m_timer_write->adjust(attotime::never, 0, attotime::never);
	m_timer_wait->adjust(attotime::never, 0, attotime::never);

	m_count_read = m_count_write = m_track = 0;
}

int dvk_kzd_device::z80daisy_irq_state()
{
	if ((m_zoa|m_zob) == ASSERT_LINE)
		return Z80_DAISY_INT;
	else
		return 0;
}

int dvk_kzd_device::z80daisy_irq_ack()
{
	int vec = -1;

	if (m_zoa == ASSERT_LINE) {
		m_zoa = CLEAR_LINE;
		vec = 0300;
	} else if (m_zob == ASSERT_LINE) {
		m_zob = CLEAR_LINE;
		vec = 0300;
	}

	if (vec > 0)
		DBG_LOG(2,"IRQ ACK", ("vec %03o\n", vec));

	return vec;
}

void dvk_kzd_device::z80daisy_irq_reti()
{
}

//-------------------------------------------------
//  device_mconfig_additions - return a pointer to
//  the device's machine fragment
//-------------------------------------------------

machine_config_constructor dvk_kzd_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( dvk_kzd );
}
