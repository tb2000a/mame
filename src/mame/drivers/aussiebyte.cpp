// license:BSD-3-Clause
// copyright-holders:Robbbert
/*************************************************************************************************

    The Aussie Byte II Single-Board Computer, created by SME Systems, Melbourne, Australia.
    Also known as the Knight 2000 Microcomputer.

    Status:
    Boots up from floppy.
    Output to serial terminal and to 6545 are working. Serial keyboard works.

    Developed in conjunction with members of the MSPP. Written in July, 2015.

    ToDo:
    - CRT8002 attributes controller
    - Graphics
    - Hard drive controllers and drives
    - Test Centronics printer
    - PIO connections

    Note of MAME restrictions:
    - Votrax doesn't sound anything like the real thing
    - WD1001/WD1002 device is not emulated
    - CRT8002 device is not emulated

**************************************************************************************************/

/***********************************************************

    Includes

************************************************************/
#include "emu.h"
#include "includes/aussiebyte.h"

#include "screen.h"
#include "speaker.h"


/***********************************************************

    Address Maps

************************************************************/

void aussiebyte_state::aussiebyte_map(address_map &map)
{
	map(0x0000, 0x3fff).bankr("bankr0").bankw("bankw0");
	map(0x4000, 0x7fff).bankrw("bank1");
	map(0x8000, 0xbfff).bankrw("bank2");
	map(0xc000, 0xffff).ram().region("mram", 0x0000);
}

void aussiebyte_state::aussiebyte_io(address_map &map)
{
	map.global_mask(0xff);
	map.unmap_value_high();
	map(0x00, 0x03).rw("sio1", FUNC(z80sio_device::ba_cd_r), FUNC(z80sio_device::ba_cd_w));
	map(0x04, 0x07).rw(m_pio1, FUNC(z80pio_device::read), FUNC(z80pio_device::write));
	map(0x08, 0x0b).rw(m_ctc, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
	map(0x0c, 0x0f).noprw(); // winchester interface
	map(0x10, 0x13).rw(m_fdc, FUNC(wd2797_device::read), FUNC(wd2797_device::write));
	map(0x14, 0x14).rw(m_dma, FUNC(z80dma_device::read), FUNC(z80dma_device::write));
	map(0x15, 0x15).w(this, FUNC(aussiebyte_state::port15_w)); // boot rom disable
	map(0x16, 0x16).w(this, FUNC(aussiebyte_state::port16_w)); // fdd select
	map(0x17, 0x17).w(this, FUNC(aussiebyte_state::port17_w)); // DMA mux
	map(0x18, 0x18).w(this, FUNC(aussiebyte_state::port18_w)); // fdc select
	map(0x19, 0x19).r(this, FUNC(aussiebyte_state::port19_r)); // info port
	map(0x1a, 0x1a).w(this, FUNC(aussiebyte_state::port1a_w)); // membank
	map(0x1b, 0x1b).w(this, FUNC(aussiebyte_state::port1b_w)); // winchester control
	map(0x1c, 0x1f).w(this, FUNC(aussiebyte_state::port1c_w)); // gpebh select
	map(0x20, 0x23).rw(m_pio2, FUNC(z80pio_device::read), FUNC(z80pio_device::write));
	map(0x24, 0x27).rw("sio2", FUNC(z80sio_device::ba_cd_r), FUNC(z80sio_device::ba_cd_w));
	map(0x28, 0x28).r(this, FUNC(aussiebyte_state::port28_r)).w(m_votrax, FUNC(votrax_sc01_device::write));
	map(0x2c, 0x2c).w(m_votrax, FUNC(votrax_sc01_device::inflection_w));
	map(0x30, 0x30).w(this, FUNC(aussiebyte_state::address_w));
	map(0x31, 0x31).r(m_crtc, FUNC(mc6845_device::status_r));
	map(0x32, 0x32).w(this, FUNC(aussiebyte_state::register_w));
	map(0x33, 0x33).r(this, FUNC(aussiebyte_state::port33_r));
	map(0x34, 0x34).w(this, FUNC(aussiebyte_state::port34_w)); // video control
	map(0x35, 0x35).w(this, FUNC(aussiebyte_state::port35_w)); // data to vram and aram
	map(0x36, 0x36).r(this, FUNC(aussiebyte_state::port36_r)); // data from vram and aram
	map(0x37, 0x37).r(this, FUNC(aussiebyte_state::port37_r)); // read dispen flag
	map(0x40, 0x4f).rw(this, FUNC(aussiebyte_state::rtc_r), FUNC(aussiebyte_state::rtc_w));
}

/***********************************************************

    Keyboard

************************************************************/
static INPUT_PORTS_START( aussiebyte )
INPUT_PORTS_END

/***********************************************************

    I/O Ports

************************************************************/
WRITE8_MEMBER( aussiebyte_state::port15_w )
{
	membank("bankr0")->set_entry(m_port15); // point at ram
	m_port15 = true;
}

/* FDD select
0 Drive Select bit O
1 Drive Select bit 1
2 Drive Select bit 2
3 Drive Select bit 3
  - These bits connect to a 74LS145 binary to BCD converter.
  - Drives 0 to 3 are 5.25 inch, 4 to 7 are 8 inch, 9 and 0 are not used.
  - Currently we only support drive 0.
4 Side Select to Disk Drives.
5 Disable 5.25 inch floppy spindle motors.
6 Unused.
7 Enable write precompensation on WD2797 controller. */
WRITE8_MEMBER( aussiebyte_state::port16_w )
{
	floppy_image_device *m_floppy = nullptr;
	if ((data & 15) == 0)
		m_floppy = m_floppy0->get_device();
	else
	if ((data & 15) == 1)
		m_floppy = m_floppy1->get_device();

	m_fdc->set_floppy(m_floppy);

	if (m_floppy)
	{
		m_floppy->mon_w(BIT(data, 5));
		m_floppy->ss_w(BIT(data, 4));
	}
}

/* DMA select
0 - FDC
1 - SIO Ch A
2 - SIO Ch B
3 - Winchester bus
4 - SIO Ch C
5 - SIO Ch D
6 - Ext ready 1
7 - Ext ready 2 */
WRITE8_MEMBER( aussiebyte_state::port17_w )
{
	m_port17 = data & 7;
	m_dma->rdy_w(BIT(m_port17_rdy, data));
}

/* FDC params
2 EXC: WD2797 clock frequency. H = 5.25"; L = 8"
3 WIEN: WD2797 Double density select. */
WRITE8_MEMBER( aussiebyte_state::port18_w )
{
	m_fdc->set_unscaled_clock(BIT(data, 2) ? 1e6 : 2e6);
	m_fdc->dden_w(BIT(data, 3));
}

READ8_MEMBER( aussiebyte_state::port19_r )
{
	return m_port19;
}

// Memory banking
WRITE8_MEMBER( aussiebyte_state::port1a_w )
{
	data &= 7;
	switch (data)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			m_port1a = data*3+1;
			if (m_port15)
				membank("bankr0")->set_entry(data*3+1);
			membank("bankw0")->set_entry(data*3+1);
			membank("bank1")->set_entry(data*3+2);
			membank("bank2")->set_entry(data*3+3);
			break;
		case 5:
			m_port1a = 1;
			if (m_port15)
				membank("bankr0")->set_entry(1);
			membank("bankw0")->set_entry(1);
			membank("bank1")->set_entry(2);
			membank("bank2")->set_entry(13);
			break;
		case 6:
			m_port1a = 14;
			if (m_port15)
				membank("bankr0")->set_entry(14);
			membank("bankw0")->set_entry(14);
			membank("bank1")->set_entry(15);
			//membank("bank2")->set_entry(0); // open bus
			break;
		case 7:
			m_port1a = 1;
			if (m_port15)
				membank("bankr0")->set_entry(1);
			membank("bankw0")->set_entry(1);
			membank("bank1")->set_entry(4);
			membank("bank2")->set_entry(13);
			break;
	}
}

// Winchester control
WRITE8_MEMBER( aussiebyte_state::port1b_w )
{
}

// GPEHB control
WRITE8_MEMBER( aussiebyte_state::port1c_w )
{
}

WRITE8_MEMBER( aussiebyte_state::port20_w )
{
	m_speaker->level_w(BIT(data, 7));
	m_rtc->cs_w(BIT(data, 0));
	m_rtc->hold_w(BIT(data, 0));
}

READ8_MEMBER( aussiebyte_state::port28_r )
{
	return m_port28;
}

/***********************************************************

    RTC

************************************************************/
READ8_MEMBER( aussiebyte_state::rtc_r )
{
	m_rtc->read_w(1);
	m_rtc->address_w(offset);
	uint8_t data = m_rtc->data_r(space,0);
	m_rtc->read_w(0);
	return data;
}

WRITE8_MEMBER( aussiebyte_state::rtc_w )
{
	m_rtc->address_w(offset);
	m_rtc->data_w(space,0,data);
	m_rtc->write_w(1);
	m_rtc->write_w(0);
}

/***********************************************************

    DMA

************************************************************/
READ8_MEMBER( aussiebyte_state::memory_read_byte )
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM);
	return prog_space.read_byte(offset);
}

WRITE8_MEMBER( aussiebyte_state::memory_write_byte )
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM);
	prog_space.write_byte(offset, data);
}

READ8_MEMBER( aussiebyte_state::io_read_byte )
{
	address_space& prog_space = m_maincpu->space(AS_IO);
	return prog_space.read_byte(offset);
}

WRITE8_MEMBER( aussiebyte_state::io_write_byte )
{
	address_space& prog_space = m_maincpu->space(AS_IO);
	prog_space.write_byte(offset, data);
}

WRITE_LINE_MEMBER( aussiebyte_state::busreq_w )
{
// since our Z80 has no support for BUSACK, we assume it is granted immediately
	m_maincpu->set_input_line(Z80_INPUT_LINE_BUSRQ, state);
	m_dma->bai_w(state); // tell dma that bus has been granted
}

/***********************************************************

    DMA selector

************************************************************/
WRITE_LINE_MEMBER( aussiebyte_state::sio1_rdya_w )
{
	m_port17_rdy = (m_port17_rdy & 0xfd) | (uint8_t)(state << 1);
	if (m_port17 == 1)
		m_dma->rdy_w(state);
}

WRITE_LINE_MEMBER( aussiebyte_state::sio1_rdyb_w )
{
	m_port17_rdy = (m_port17_rdy & 0xfb) | (uint8_t)(state << 2);
	if (m_port17 == 2)
		m_dma->rdy_w(state);
}

WRITE_LINE_MEMBER( aussiebyte_state::sio2_rdya_w )
{
	m_port17_rdy = (m_port17_rdy & 0xef) | (uint8_t)(state << 4);
	if (m_port17 == 4)
		m_dma->rdy_w(state);
}

WRITE_LINE_MEMBER( aussiebyte_state::sio2_rdyb_w )
{
	m_port17_rdy = (m_port17_rdy & 0xdf) | (uint8_t)(state << 5);
	if (m_port17 == 5)
		m_dma->rdy_w(state);
}


/***********************************************************

    Video

************************************************************/

/* F4 Character Displayer */
static const gfx_layout crt8002_charlayout =
{
	8, 12,                   /* 7 x 11 characters */
	128,                  /* 128 characters */
	1,                  /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
	8*16                    /* every char takes 16 bytes */
};

static GFXDECODE_START( crt8002 )
	GFXDECODE_ENTRY( "chargen", 0x0000, crt8002_charlayout, 0, 1 )
GFXDECODE_END

/***************************************************************

    Daisy Chain

****************************************************************/

static const z80_daisy_config daisy_chain_intf[] =
{
	{ "dma" },
	{ "pio2" },
	{ "sio1" },
	{ "sio2" },
	{ "pio1" },
	{ "ctc" },
	{ nullptr }
};


/***********************************************************

    CTC

************************************************************/

// baud rate generator. All inputs are 1.2288MHz.
WRITE_LINE_MEMBER( aussiebyte_state::ctc_z2_w )
{
	m_ctc->trg3(1);
	m_ctc->trg3(0);
}

/***********************************************************

    Centronics ack

************************************************************/
WRITE_LINE_MEMBER( aussiebyte_state::write_centronics_busy )
{
	m_centronics_busy = state;
}

/***********************************************************

    Speech ack

************************************************************/
WRITE_LINE_MEMBER( aussiebyte_state::votrax_w )
{
	m_port28 = state;
}


/***********************************************************

    Floppy Disk

************************************************************/

WRITE_LINE_MEMBER( aussiebyte_state::fdc_intrq_w )
{
	uint8_t data = (m_port19 & 0xbf) | (state ? 0x40 : 0);
	m_port19 = data;
}

WRITE_LINE_MEMBER( aussiebyte_state::fdc_drq_w )
{
	uint8_t data = (m_port19 & 0x7f) | (state ? 0x80 : 0);
	m_port19 = data;
	state ^= 1; // inverter on pin38 of fdc
	m_port17_rdy = (m_port17_rdy & 0xfe) | (uint8_t)state;
	if (m_port17 == 0)
		m_dma->rdy_w(state);
}

static SLOT_INTERFACE_START( aussiebyte_floppies )
	SLOT_INTERFACE( "525qd", FLOPPY_525_QD )
SLOT_INTERFACE_END


/***********************************************************

    Quickload

    This loads a .COM file to address 0x100 then jumps
    there. Sometimes .COM has been renamed to .CPM to
    prevent windows going ballistic. These can be loaded
    as well.

************************************************************/

QUICKLOAD_LOAD_MEMBER( aussiebyte_state, aussiebyte )
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM);

	if (quickload_size >= 0xfd00)
		return image_init_result::FAIL;

	/* RAM must be banked in */
	m_port15 = true;    // disable boot rom
	m_port1a = 4;
	membank("bankr0")->set_entry(m_port1a); /* enable correct program bank */
	membank("bankw0")->set_entry(m_port1a);

	/* Avoid loading a program if CP/M-80 is not in memory */
	if ((prog_space.read_byte(0) != 0xc3) || (prog_space.read_byte(5) != 0xc3))
	{
		machine_reset();
		return image_init_result::FAIL;
	}

	/* Load image to the TPA (Transient Program Area) */
	for (uint16_t i = 0; i < quickload_size; i++)
	{
		uint8_t data;
		if (image.fread( &data, 1) != 1)
			return image_init_result::FAIL;
		prog_space.write_byte(i+0x100, data);
	}

	/* clear out command tail */
	prog_space.write_byte(0x80, 0); prog_space.write_byte(0x81, 0);

	/* Roughly set SP basing on the BDOS position */
	m_maincpu->set_state_int(Z80_SP, 256 * prog_space.read_byte(7) - 0x400);
	m_maincpu->set_pc(0x100);                // start program

	return image_init_result::PASS;
}

/***********************************************************

    Machine Driver

************************************************************/
void aussiebyte_state::machine_reset()
{
	m_port15 = false;
	m_port17 = 0;
	m_port17_rdy = 0;
	m_port1a = 1;
	m_alpha_address = 0;
	m_graph_address = 0;
	membank("bankr0")->set_entry(16); // point at rom
	membank("bankw0")->set_entry(1); // always write to ram
	membank("bank1")->set_entry(2);
	membank("bank2")->set_entry(3);
	m_maincpu->reset();
}

MACHINE_CONFIG_START(aussiebyte_state::aussiebyte)
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL(16'000'000) / 4)
	MCFG_CPU_PROGRAM_MAP(aussiebyte_map)
	MCFG_CPU_IO_MAP(aussiebyte_io)
	MCFG_Z80_DAISY_CHAIN(daisy_chain_intf)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE_DEVICE("crtc", sy6545_1_device, screen_update)
	MCFG_GFXDECODE_ADD("gfxdecode", "palette", crt8002)
	MCFG_PALETTE_ADD_MONOCHROME("palette")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_DEVICE_ADD("votrax", VOTRAX_SC01, 720000) /* 720kHz? needs verify */
	MCFG_VOTRAX_SC01_REQUEST_CB(WRITELINE(aussiebyte_state, votrax_w))
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MCFG_CENTRONICS_ADD("centronics", centronics_devices, "printer")
	MCFG_CENTRONICS_DATA_INPUT_BUFFER("cent_data_in")
	MCFG_CENTRONICS_BUSY_HANDLER(WRITELINE(aussiebyte_state, write_centronics_busy))
	MCFG_DEVICE_ADD("cent_data_in", INPUT_BUFFER, 0)
	MCFG_CENTRONICS_OUTPUT_LATCH_ADD("cent_data_out", "centronics")

	MCFG_DEVICE_ADD("ctc_clock", CLOCK, XTAL(4'915'200) / 4)
	MCFG_CLOCK_SIGNAL_HANDLER(DEVWRITELINE("ctc", z80ctc_device, trg0))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("ctc", z80ctc_device, trg1))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("ctc", z80ctc_device, trg2))

	MCFG_DEVICE_ADD("ctc", Z80CTC, XTAL(16'000'000) / 4)
	MCFG_Z80CTC_INTR_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_Z80CTC_ZC0_CB(DEVWRITELINE("sio1", z80sio_device, rxca_w))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("sio1", z80sio_device, txca_w))
	MCFG_Z80CTC_ZC1_CB(DEVWRITELINE("sio1", z80sio_device, rxtxcb_w))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("sio2", z80sio_device, rxca_w))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("sio2", z80sio_device, txca_w))
	MCFG_Z80CTC_ZC2_CB(WRITELINE(aussiebyte_state, ctc_z2_w))    // SIO2 Ch B, CTC Ch 3
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("sio2", z80sio_device, rxtxcb_w))

	MCFG_DEVICE_ADD("dma", Z80DMA, XTAL(16'000'000) / 4)
	MCFG_Z80DMA_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_Z80DMA_OUT_BUSREQ_CB(WRITELINE(aussiebyte_state, busreq_w))
	// BAO, not used
	MCFG_Z80DMA_IN_MREQ_CB(READ8(aussiebyte_state, memory_read_byte))
	MCFG_Z80DMA_OUT_MREQ_CB(WRITE8(aussiebyte_state, memory_write_byte))
	MCFG_Z80DMA_IN_IORQ_CB(READ8(aussiebyte_state, io_read_byte))
	MCFG_Z80DMA_OUT_IORQ_CB(WRITE8(aussiebyte_state, io_write_byte))

	MCFG_DEVICE_ADD("pio1", Z80PIO, XTAL(16'000'000) / 4)
	MCFG_Z80PIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_Z80PIO_OUT_PA_CB(DEVWRITE8("cent_data_out", output_latch_device, write))
	MCFG_Z80PIO_IN_PB_CB(DEVREAD8("cent_data_in", input_buffer_device, read))
	MCFG_Z80PIO_OUT_ARDY_CB(DEVWRITELINE("centronics", centronics_device, write_strobe)) MCFG_DEVCB_INVERT

	MCFG_DEVICE_ADD("pio2", Z80PIO, XTAL(16'000'000) / 4)
	MCFG_Z80PIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_Z80PIO_OUT_PA_CB(WRITE8(aussiebyte_state, port20_w))

	MCFG_DEVICE_ADD("sio1", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_Z80SIO_OUT_WRDYA_CB(WRITELINE(aussiebyte_state, sio1_rdya_w))
	MCFG_Z80SIO_OUT_WRDYB_CB(WRITELINE(aussiebyte_state, sio1_rdyb_w))

	MCFG_DEVICE_ADD("sio2", Z80SIO, XTAL(16'000'000) / 4)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_Z80SIO_OUT_WRDYA_CB(WRITELINE(aussiebyte_state, sio2_rdya_w))
	MCFG_Z80SIO_OUT_WRDYB_CB(WRITELINE(aussiebyte_state, sio2_rdyb_w))
	MCFG_Z80SIO_OUT_TXDA_CB(DEVWRITELINE("rs232", rs232_port_device, write_txd))
	MCFG_Z80SIO_OUT_DTRA_CB(DEVWRITELINE("rs232", rs232_port_device, write_dtr))
	MCFG_Z80SIO_OUT_RTSA_CB(DEVWRITELINE("rs232", rs232_port_device, write_rts))

	MCFG_RS232_PORT_ADD("rs232", default_rs232_devices, "keyboard")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("sio2", z80sio_device, rxa_w))

	MCFG_WD2797_ADD("fdc", XTAL(16'000'000) / 16)
	MCFG_WD_FDC_INTRQ_CALLBACK(WRITELINE(aussiebyte_state, fdc_intrq_w))
	MCFG_WD_FDC_DRQ_CALLBACK(WRITELINE(aussiebyte_state, fdc_drq_w))
	MCFG_FLOPPY_DRIVE_ADD("fdc:0", aussiebyte_floppies, "525qd", floppy_image_device::default_floppy_formats)
	MCFG_FLOPPY_DRIVE_SOUND(true)
	MCFG_FLOPPY_DRIVE_ADD("fdc:1", aussiebyte_floppies, "525qd", floppy_image_device::default_floppy_formats)
	MCFG_FLOPPY_DRIVE_SOUND(true)

	/* devices */
	MCFG_MC6845_ADD("crtc", SY6545_1, "screen", XTAL(16'000'000) / 8)
	MCFG_MC6845_SHOW_BORDER_AREA(false)
	MCFG_MC6845_CHAR_WIDTH(8)
	MCFG_MC6845_UPDATE_ROW_CB(aussiebyte_state, crtc_update_row)
	MCFG_MC6845_ADDR_CHANGED_CB(aussiebyte_state, crtc_update_addr)

	MCFG_MSM5832_ADD("rtc", XTAL(32'768))

	/* quickload */
	MCFG_QUICKLOAD_ADD("quickload", aussiebyte_state, aussiebyte, "com,cpm", 3)

MACHINE_CONFIG_END


void aussiebyte_state::machine_start()
{
	// Main ram is divided into 16k blocks (0-15). The boot rom is block number 16.
	// For convenience, bank 0 is permanently assigned to C000-FFFF
	uint8_t *main = memregion("roms")->base();
	uint8_t *ram = memregion("mram")->base();

	membank("bankr0")->configure_entries(0, 16, &ram[0x0000], 0x4000);
	membank("bankw0")->configure_entries(0, 16, &ram[0x0000], 0x4000);
	membank("bank1")->configure_entries(0, 16, &ram[0x0000], 0x4000);
	membank("bank2")->configure_entries(0, 16, &ram[0x0000], 0x4000);
	membank("bankr0")->configure_entry(16, &main[0x0000]);
}


/***********************************************************

    Game driver

************************************************************/


ROM_START(aussieby)
	ROM_REGION(0x4000, "roms", 0) // Size of bank 16
	ROM_LOAD( "knight_boot_0000.u27", 0x0000, 0x1000, CRC(1f200437) SHA1(80d1d208088b325c16a6824e2da605fb2b00c2ce) )

	ROM_REGION(0x800, "chargen", 0)
	ROM_LOAD( "8002.bin", 0x0000, 0x0800, CRC(fdd6eb13) SHA1(a094d416e66bdab916e72238112a6265a75ca690) )

	ROM_REGION(0x40000, "mram", ROMREGION_ERASE00) // main ram, 256k dynamic
	ROM_REGION(0x10000, "vram", ROMREGION_ERASEFF) // video ram, 64k dynamic
	ROM_REGION(0x00800, "aram", ROMREGION_ERASEFF) // attribute ram, 2k static
ROM_END

//    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT        CLASS             INIT        COMPANY         FULLNAME           FLAGS
COMP( 1984, aussieby,     0,        0,  aussiebyte, aussiebyte,  aussiebyte_state, 0,          "SME Systems",  "Aussie Byte II" , MACHINE_IMPERFECT_GRAPHICS )
