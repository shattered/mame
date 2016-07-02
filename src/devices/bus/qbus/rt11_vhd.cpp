// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

    rt11_vhd.c

    DEC RT-11 Virtual Hard Drives

****************************************************************************

    Technical specs on the Virtual Hard Disk interface (via
	http://zx-pk.ru/threads/17277-pomogite-najti-osnovnoj-test-kommand-elektroniki-60.html?p=436263&viewfull=1#post436263)

    Address       Description
    -------       -----------
    177720        HDCSR -- Command/status register.
    177722        HDDATA

    Set HDDATA register, and then issue a command to HDCSR as follows:

	SetUni		= 1	; Set HD unit number
	SetBlk		= 2	; Set HD block number (in 512-byte blocks)
	SetBuf		= 3	; Set memory buffer address
	SetWCn		= 4	; Set operation word count (in 16-bit words)
	CmdRea		= 5	; Execute READ  on HD
	CmdWri		= 6	; Execute WRITE on HD
	GetSiz		= 7	; Get HD size in 512-byte blocks.

    Error state is reported in bit 15 of HDCSR, except for command 7
	(zero is returned in HDDATA if unit is not attached).

 ***************************************************************************/

#include "rt11_vhd.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define VERBOSE_DBG 1       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f at %s: %-24s",machine().time().as_double(),machine().describe_context(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

#define VHDSTATUS_OK                    0x00
#define VHDSTATUS_NO_VHD_ATTACHED       0x02
#define VHDSTATUS_ACCESS_DENIED         0x05
#define VHDSTATUS_UNKNOWN_COMMAND       0xFE
#define VHDSTATUS_POWER_ON_STATE        0xFF

#define VHDCMD_SET_UNIT 1
#define VHDCMD_SET_LBN  2
#define VHDCMD_SET_BUF  3
#define VHDCMD_SET_CNT  4
#define VHDCMD_READ     5
#define VHDCMD_WRITE    6
#define VHDCMD_GET_SIZE 7


/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

const device_type RT11_VHD = &device_creator<rt11_vhd_image_device>;

//-------------------------------------------------
//  rt11_vhd_image_device - constructor
//-------------------------------------------------

rt11_vhd_image_device::rt11_vhd_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, RT11_VHD, "Virtual Hard Disk", tag, owner, clock, "rt11_vhd_image", __FILE__),
		device_image_interface(mconfig, *this),
		device_qbus_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  rt11_vhd_image_device - destructor
//-------------------------------------------------

rt11_vhd_image_device::~rt11_vhd_image_device()
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void rt11_vhd_image_device::device_config_complete()
{
	// set brief and instance name
	update_names();
}



//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void rt11_vhd_image_device::device_start()
{
	set_qbus_device();
	m_qbus->install_device(0177720, 0177723, read16_delegate(FUNC(rt11_vhd_image_device::read),this),
		write16_delegate(FUNC(rt11_vhd_image_device::write),this));

	m_status = VHDSTATUS_NO_VHD_ATTACHED;
	// XXX
	m_cpu = machine().device<cpu_device>("maincpu");
	m_cpu_space = &m_cpu->space(AS_PROGRAM);
	m_hdcsr = m_hddata = 0;
}



//-------------------------------------------------
//  call_load
//-------------------------------------------------

image_init_result rt11_vhd_image_device::call_load()
{
	m_status = VHDSTATUS_POWER_ON_STATE;
	m_logical_record_number = 0;
	m_buffer_address = 0;
	m_word_count = 0;

	return image_init_result::PASS;
}



//-------------------------------------------------
//  rt11_vhd_command
//-------------------------------------------------

void rt11_vhd_image_device::rt11_vhd_command(UINT16 data)
{
	switch (data) {
		case VHDCMD_SET_UNIT:
			/* XXX only unit 0 supported */
			if (m_hddata)
				m_status = VHDSTATUS_ACCESS_DENIED;
			else
				m_status = VHDSTATUS_OK;
			break;

		case VHDCMD_SET_LBN:
			m_logical_record_number = m_hddata;
			break;

		case VHDCMD_SET_BUF:
			m_buffer_address = m_hddata;
			break;

		case VHDCMD_SET_CNT:
			m_word_count = m_hddata;
			break;
	}
}


//-------------------------------------------------
//  rt11_vhd_readwrite
//-------------------------------------------------

void rt11_vhd_image_device::rt11_vhd_readwrite(UINT16 data)
{
	int result, i;
	UINT64 seek_position;
	char buffer[65536];

	/* access the image */
	if (!exists())
	{
		m_status = VHDSTATUS_NO_VHD_ATTACHED;
		return;
	}

	/* perform the seek */
	seek_position = ((UINT64) 512) * m_logical_record_number;
	result = fseek(seek_position, SEEK_SET);
	if (result < 0)
	{
		m_status = VHDSTATUS_ACCESS_DENIED;
		return;
	}

#if 0
	/* expand the disk, if necessary */
	if (data == VHDCMD_WRITE)
	{
		while(total_size < seek_position)
		{
			memset(buffer, 0, sizeof(buffer));

			bytes_to_write = (UINT32) MIN(seek_position - total_size, (UINT64) sizeof(buffer));
			result = fwrite(buffer, bytes_to_write);
			if (result != bytes_to_write)
			{
				m_status = VHDSTATUS_ACCESS_DENIED;
				return;
			}

			total_size += bytes_to_write;
		}
	}
#endif

	DBG_LOG(1,"I/O",("%s of %d words, lbn %06o memory %06o\n", 
		data == VHDCMD_READ?"read":"write", m_word_count, m_logical_record_number, m_buffer_address));

	switch(data)
	{
		case VHDCMD_READ: /* Read sector */
			result = fread(buffer, m_word_count*2);
			if (result != m_word_count*2)
			{
				m_status = VHDSTATUS_ACCESS_DENIED;
				return;
			}

			/* write the bytes to memory */
			for (i = 0; i < m_word_count*2; i++)
				m_cpu_space->write_byte(m_buffer_address + i, buffer[i]);

			m_status = VHDSTATUS_OK;
			break;

		case VHDCMD_WRITE: /* Write Sector */
			/* read the bytes from memory */
			for (i = 0; i < m_word_count*2; i++)
				buffer[i] = m_cpu_space->read_byte(m_buffer_address + i);

			/* and write to disk */
			result = fwrite(buffer, m_word_count*2);
			if (result != m_word_count*2)
			{
				m_status = VHDSTATUS_ACCESS_DENIED;
				return;
			}

			m_status = VHDSTATUS_OK;
			break;

		default:
			m_status = VHDSTATUS_UNKNOWN_COMMAND;
			break;
	}
}



UINT16 rt11_vhd_image_device::read(offs_t offset)
{
	UINT16 result = 0;

	switch(offset)
	{
		case 0:
			if (m_status)
				if (m_hdcsr == VHDCMD_GET_SIZE && m_status == VHDSTATUS_NO_VHD_ATTACHED)
					result = 0;
				else
					result = CSR_ERR;
			else
				result = 0;
			DBG_LOG(1,"Status read",("%02X -> %06o\n", m_status, result));
			break;

		case 1:
			if (m_hdcsr == VHDCMD_GET_SIZE && !m_status)
				result = (length() + 511) / 512;	// XXX
			else
				result = m_hddata;
			DBG_LOG(1,"Data read",("%06o -> %06o\n", m_hddata, result));
			break;
	}
	return result;
}



void rt11_vhd_image_device::write(offs_t offset, UINT16 data)
{
	switch(offset)
	{
		case 0:
			m_hdcsr = data;
			if (data == VHDCMD_READ || data == VHDCMD_WRITE)
				rt11_vhd_readwrite(data);
			else
				rt11_vhd_command(data);
			DBG_LOG(1,"Command write",("%d (%06o) -> %02X\n", data, m_hddata, m_status));
			break;

		case 1:
			m_hddata = data;
			DBG_LOG(1,"Data write",("%06o\n", data));
			break;
	}
}
