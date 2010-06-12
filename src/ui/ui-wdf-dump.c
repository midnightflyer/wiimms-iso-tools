
// *************************************************************************
// *****   This file is automatically generated by the tool 'gen-ui'   *****
// *************************************************************************
// *****                 ==> DO NOT EDIT THIS FILE <==                 *****
// *************************************************************************

#include <getopt.h>
#include "ui-wdf-dump.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  OptionInfo[]                   ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoOption_t OptionInfo[OPT__N_TOTAL+1] =
{
    {0,0,0,0,0}, // OPT_NONE,

    {	OPT_VERSION, 'V', "version",
	0,
	"Stop parsing the command line, print a version info and exit."
    },

    {	OPT_HELP, 'h', "help",
	0,
	"Stop parsing the command line, print a help message and exit."
    },

    {	OPT_XHELP, 0, "xhelp",
	0,
	"Same as --help."
    },

    {	OPT_QUIET, 'q', "quiet",
	0,
	"Be quiet and print only error messages."
    },

    {	OPT_VERBOSE, 'v', "verbose",
	0,
	"Be verbose -> print program name."
    },

    {	OPT_IO, 0, "io",
	"flags",
	"Setup the IO mode for experiments. The standard file IO is based on"
	" open() function. The value '1' defines that WBFS IO is based on"
	" fopen() function. The value '2' defines the same for ISO files and"
	" the value '3' for both, WBFS and ISO."
    },

    {	OPT_CHUNK, 'c', "chunk",
	0,
	"Print table with chunk header."
    },

    {	OPT_LONG, 'l', "long",
	0,
	"Alternative for --chunk: Print table with chunk header."
    },

    {0,0,0,0,0} // OPT__N_TOTAL == 9

};

//
///////////////////////////////////////////////////////////////////////////////
///////////////            OptionShort & OptionLong             ///////////////
///////////////////////////////////////////////////////////////////////////////

const char OptionShort[] = "Vhqvcl";

const struct option OptionLong[] =
{
	{ "version",		0, 0, 'V' },
	{ "help",		0, 0, 'h' },
	{ "xhelp",		0, 0, GO_XHELP },
	{ "quiet",		0, 0, 'q' },
	{ "verbose",		0, 0, 'v' },
	{ "io",			1, 0, GO_IO },
	{ "chunk",		0, 0, 'c' },
	{ "long",		0, 0, 'l' },

	{0,0,0,0}
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////            OptionUsed & OptionIndex             ///////////////
///////////////////////////////////////////////////////////////////////////////

u8 OptionUsed[OPT__N_TOTAL+1] = {0};

const u8 OptionIndex[OPT_INDEX_SIZE] = 
{
	/*00*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*10*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*20*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*30*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*40*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*50*/	 0,0,0,0, 0,0,
	/*56*/	OPT_VERSION,
	/*57*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	/*63*/	OPT_CHUNK,
	/*64*/	 0,0,0,0, 
	/*68*/	OPT_HELP,
	/*69*/	 0,0,0,
	/*6c*/	OPT_LONG,
	/*6d*/	 0,0,0,0, 
	/*71*/	OPT_QUIET,
	/*72*/	 0,0,0,0, 
	/*76*/	OPT_VERBOSE,
	/*77*/	 0,0,0,0, 0,0,0,0, 0,
	/*80*/	OPT_XHELP,
	/*81*/	OPT_IO,
	/*82*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,
	/*90*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*a0*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 InfoOption tabs                 ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoOption_t * option_tab_tool[] =
{
	OptionInfo + OPT_VERSION,
	OptionInfo + OPT_HELP,
	OptionInfo + OPT_XHELP,
	OptionInfo + OPT_QUIET,
	OptionInfo + OPT_VERBOSE,
	OptionInfo + OPT_IO,

	OptionInfo + OPT_NONE, // separator

	OptionInfo + OPT_CHUNK,
	OptionInfo + OPT_LONG,

	0
};


//
///////////////////////////////////////////////////////////////////////////////
///////////////                   InfoCommand                   ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoCommand_t CommandInfo[CMD__N+1] =
{
    {	0,
	false,
	false,
	"wdf-dump",
	0,
	"wdf-dump [option]... files...",
	"Dump the data structure of WDF and CISO files for analysis.",
	8,
	option_tab_tool
    },

    {0,0,0,0,0,0,0,0}
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     InfoUI                      ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoUI_t InfoUI =
{
	"wdf-dump",
	0, // n_cmd
	0, // cmd_tab
	CommandInfo,
	0, // n_opt_specific
	OPT__N_TOTAL,
	OptionInfo,
	OptionUsed,
	OptionIndex,
	OptionShort,
	OptionLong
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       END                       ///////////////
///////////////////////////////////////////////////////////////////////////////

