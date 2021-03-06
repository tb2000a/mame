// license:BSD-3-Clause
// copyright-holders:David Haywood

/*

main CPU is

  PHILIPS

P89C51RD2HBA
1C7415
AeD0118 G

(64kB of Flash ROM, 1kB of RAM)
internal ROM was read protected

there is also a M48T02-70PC1 TIMEKEEPER

board has very clear 'TAX' markings in addition to 'A.G Electronic'
and appears to have been manufactured in Italy based on other text
present.

--

This is probably a 'stealth' gambling game as the Break Out clone that
is presented is a rudimentary effort that is barely playable. Currently
there is no code to emulate tho as it is all inside the MCU.

*/


#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "sound/okim6295.h"
#include "screen.h"
#include "speaker.h"


class blocktax_state : public driver_device
{
public:
	blocktax_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu")
	{ }

	void blocktax(machine_config &config);

protected:
	virtual void video_start() override;

private:
	uint32_t screen_update_blocktax(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void blocktax_map(address_map &map);
	required_device<cpu_device> m_maincpu;
};

void blocktax_state::video_start()
{
}

uint32_t blocktax_state::screen_update_blocktax(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	return 0;
}

//unused function
void blocktax_state::blocktax_map(address_map &map)
{
}

static INPUT_PORTS_START( blocktax )
INPUT_PORTS_END

MACHINE_CONFIG_START(blocktax_state::blocktax)
	MCFG_CPU_ADD("maincpu", I80C51, 30_MHz_XTAL/2) /* P89C51RD2HBA (80C51 with internal flash rom) */

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(blocktax_state, screen_update_blocktax)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 0x200)
	MCFG_PALETTE_FORMAT(xRRRRRGGGGGBBBBB)

	MCFG_SPEAKER_STANDARD_MONO("speaker")

	MCFG_OKIM6295_ADD("oki", 30_MHz_XTAL/16, PIN7_HIGH) // clock frequency & pin 7 not verified
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "speaker", 1.00)
MACHINE_CONFIG_END

ROM_START( blocktax )
	ROM_REGION( 0x10000, "maincpu", 0 ) /* Internal MCU Flash */
	ROM_LOAD( "p89c51rd2hba.mcu", 0x00000, 0x10000, NO_DUMP )

	ROM_REGION( 0x040000, "oki", 0 ) /* Samples */
	ROM_LOAD( "1_ht27c010.bin", 0x00000, 0x20000, CRC(5e5c29f8) SHA1(e62f81be8e90a098ea4a8a55cdf02c5b4c226317) )

	ROM_REGION( 0x100000, "gfx1", 0 )
	ROM_LOAD( "4_ht27c020.bin", 0x40000, 0x40000, CRC(b43b91ff) SHA1(d5baad5819981d74aea2a142658af84b6445f324) )

	ROM_REGION( 0x80000, "gfx2", 0 )
	ROM_LOAD( "2_ht27c020.bin", 0x00000, 0x40000, CRC(4800c3be) SHA1(befaf07a75fe57a910e0a89578bf352102ae773e) )
	ROM_LOAD( "3_ht27c020.bin", 0x40000, 0x40000, CRC(ea1c66a2) SHA1(d10b9ca56d140235b6f31ab939613784f232caeb) )
ROM_END


GAME( 2002, blocktax, 0, blocktax, blocktax, blocktax_state, 0, ROT0,  "TAX / Game Revival", "Blockout (TAX)", MACHINE_IS_SKELETON )
