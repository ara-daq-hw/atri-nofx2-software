# ara nofx2 software

pulled a ton of the crap out of other stuff to go here

* clockset_mcp.sh programs the clock
* mcpi2c_wr is used by clockset_mcp.sh to access the mcp2221
* libmcp2221-master is ripped from https://github.com/ZakKemble/libmcp2221
* libmcp wraps a lot of the libmcp2221 crap into ara-specific simple stuff
* nofx2ControlLib is an LD_PRELOADable override of the FX2 crap in ARAAcqd
