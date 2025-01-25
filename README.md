# ara nofx2 software

pulled a ton of the crap out of other stuff to go here

* clockset_mcp.sh programs the clock
* mcpi2c_wr is used by clockset_mcp.sh to access the mcp2221
* libmcp2221-master is ripped from https://github.com/ZakKemble/libmcp2221
* libmcp wraps a lot of the libmcp2221 crap into ara-specific simple stuff
* nofx2ControlLib is an LD_PRELOADable override of the FX2 crap in ARAAcqd

# NOTES ON OPERATING FOR THE IMPATIENT

The nofx2 modifications have **nonvolatile firmware**. This means you do _not_ need to reprogram it every time! If the device is
in **DAQ MODE** (see below) it is ready to take data! To replace the DAQ firmware see "Programming New Firmware."

**THE STATIONS ALWAYS COME UP IN BOOTLOADER MODE AFTER A POWER CYCLE.**

## Firmware Modes

* Run ``ls /dev/xillybus*``.
* If ``/dev/xillybus_spi_in`` and ``/dev/xillybus_spi_out`` exist you are in **BOOTLOADER MODE**
* If ``/dev/xillybus_pkt_in``, ``/dev/xillybus_pkt_out``, and ``/dev/xillybus_ev_out`` exist you are in **DAQ MODE**
* If _no_ ``/dev/xillybus*`` entries exist, check ``lsmod`` if ``xillybus_pcie`` is loaded. The system may be waiting for you to reboot. Otherwise you are in **BROKEN MODE** and should power cycle and possibly reprogram the DAQ firmware using the bootloader.

You can switch from **BOOTLOADER MODE** to **DAQ MODE** by running ``toDaqFirmware.sh``
You can switch from **DAQ MODE** to **BOOTLOADER MODE** by running ``toBootloader.sh`` _or_ by power cycling.

## Programming New Firmware

If you are in **BOOTLOADER MODE** you can program using the ``loadSPIFlash.py`` script.

## DAQ Software Issues

The nofx2 software works by _replacing_ some of the library functions of ARAAcqd using LD_PRELOAD.
https://www.baeldung.com/linux/ld_preload-trick-what-is

Because of the ancient version of Linux that's on the ARA stations, we _also_ need to launch two "helper programs" beforehand. All of this is **wrapped**
in a script called ``simpleStartSoft.sh``. This script is launched by the silly start DAQ scripts in ``WorkingDAQ/scripts`` _instead_ of ``ARAAcqd``.

_Unfortunately_ ``ARAd`` for some immensely silly reason tries to spawn ``ARAAcqd`` if it is not running and it _tries to start it with a hardcoded command_ -
as in, it just does the equivalent of ``ARAAcqd &`` and assumes it'll work. **This should be changed in ARAd** - it should look up the command it should use
_in its config file_ (otherwise _why does it have a config file_) and then that value can be changed to the simpleStartSoft script.

Therefore _if something goes wrong_ with launching ``simpleStartSoft.sh`` _you will see the old version of ARAAcqd launched_. That's what it means.

## What Is Changed On Stations?

1. this repository is at /home/ara/atri-nofx2-software, and is compiled using ``makeAll.sh
2. ``startAraSoftWithTimeRecord.sh`` was changed to make an ``${ARAACQD}`` environment variable which it launches. That variable is set to point to ``simpleStartSoft.sh`` instead of ``ARAAcqd``
