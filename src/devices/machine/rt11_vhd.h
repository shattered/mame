// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/***************************************************************************

    rt11_vhd.h

    DEC RT-11 Virtual Hard Drives

***************************************************************************/

#ifndef RT11VHD_H
#define RT11VHD_H


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> rt11_vhd_image_device

class rt11_vhd_image_device :   public device_t,
								public device_image_interface
{
public:
	// construction/destruction
	rt11_vhd_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~rt11_vhd_image_device();

	// image-level overrides
	virtual image_init_result call_load() override;

	virtual iodevice_t image_type() const override { return IO_HARDDISK; }

	virtual bool is_readable()  const override { return 1; }
	virtual bool is_writeable() const override { return 1; }
	virtual bool is_creatable() const override { return 1; }
	virtual bool must_be_loaded() const override { return 0; }
	virtual bool is_reset_on_load() const override { return 0; }
	virtual const char *image_interface() const override { return nullptr; }
	virtual const char *file_extensions() const override { return "vhd"; }
	virtual const option_guide *create_option_guide() const override { return nullptr; }

	// specific implementation
	DECLARE_READ16_MEMBER(read) { return read(offset); }
	DECLARE_WRITE16_MEMBER(write) { write(offset, data); }
	UINT16 read(offs_t offset);
	void write(offs_t offset, UINT16 data);

protected:
	// device-level overrides
	virtual void device_config_complete() override;
	virtual void device_start() override;

	void rt11_vhd_command(UINT16 data);
	void rt11_vhd_readwrite(UINT16 data);

private:
	cpu_device *            m_cpu;
	address_space *         m_cpu_space;
	UINT16                  m_hdcsr;
	UINT16                  m_hddata;
	UINT16                  m_logical_record_number;
	UINT16                  m_buffer_address;
	UINT16                  m_word_count;
	UINT16                  m_status;
};

// device type definition
extern const device_type RT11_VHD;

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_RT11_VHD_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, RT11_VHD, 0)

#endif /* RT11VHD_H */
