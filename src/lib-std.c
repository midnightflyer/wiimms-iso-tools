
#define _GNU_SOURCE 1
#define _XOPEN_SOURCE 1

#include <sys/time.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#if defined(__CYGWIN__)
  #include <cygwin/fs.h>
  #include <io.h>
  //#include <locale.h>
#elif defined(__APPLE__)
  #include <sys/disk.h>
#elif defined(__linux__)
  #include <linux/fs.h>
#endif

#include "version.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "wbfs-interface.h"
#include "crypt.h"
#include "titles.h"
#include "ui.h"
#include "dclib-utf8.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       Setup                     ///////////////
///////////////////////////////////////////////////////////////////////////////

enumProgID prog_id		= PROG_UNKNOWN;
u32 revision_id			= SYSTEMID + REVISION_NUM;
ccp progname			= "?";
ccp search_path[5]		= {0};
ccp lang_info			= 0;
volatile int SIGINT_level	= 0;
volatile int verbose		= 0;
volatile int logging		= 0;
int progress			= 0;
SortMode sort_mode		= SORT_DEFAULT;
RepairMode repair_mode		= REPAIR_NONE;
char escape_char		= '%';
enumOFT output_file_type	= OFT_UNKNOWN;
option_t used_options		= 0;
option_t env_options		= 0;
int opt_split			= 0;
u64 opt_split_size		= 0;
ccp opt_clone			= 0;

#ifdef __CYGWIN__
 bool use_utf8			= false;
#else
 bool use_utf8			= true;
#endif

char       iobuf [0x400000];		// global io buffer
const char zerobuf[0x40000] = {0};	// global zero buffer

const char sep_79[80] =		//  79 * '-' + NULL
	"----------------------------------------"
	"---------------------------------------";

const char sep_200[201] =	// 200 * '-' + NULL
	"----------------------------------------"
	"----------------------------------------"
	"----------------------------------------"
	"----------------------------------------"
	"----------------------------------------";

StringField_t source_list;
StringField_t recurse_list;
StringField_t created_files;

u32 opt_recurse_depth = DEF_RECURSE_DEPTH;

///////////////////////////////////////////////////////////////////////////////

static void sig_handler ( int signum )
{
    static const char usr_msg[] =
	"\n"
	"****************************************************************\n"
	"***  SIGNAL USR%d CATCHED: VERBOSE LEVEL %s TO %-4d    ***\n"
	"***  THE EFFECT IS DELAYED UNTIL BEGINNING OF THE NEXT JOB.  ***\n"
	"****************************************************************\n";

    fflush(stdout);
    switch(signum)
    {
      case SIGINT:
      case SIGTERM:
	SIGINT_level++;
	TRACE("#SIGNAL# INT/TERM, level = %d\n",SIGINT_level);

	switch(SIGINT_level)
	{
	  case 1:
	    fprintf(stderr,
		"\n"
		"****************************************************************\n"
		"***  PROGRAM INTERRUPTED BY USER (LEVEL=1).                  ***\n"
		"***  PROGRAM WILL TERMINATE AFTER CURRENT JOB HAS FINISHED.  ***\n"
		"****************************************************************\n" );
	    break;

	  case 2:
	    fprintf(stderr,
		"\n"
		"*********************************************************\n"
		"***  PROGRAM INTERRUPTED BY USER (LEVEL=2).           ***\n"
		"***  PROGRAM WILL TRY TO TERMINATE NOW WITH CLEANUP.  ***\n"
		"*********************************************************\n" );
	    break;

	  default:
	    fprintf(stderr,
		"\n"
		"*************************************************************\n"
		"***  PROGRAM INTERRUPTED BY USER (LEVEL>=3).              ***\n"
		"***  PROGRAM WILL TERMINATE IMMEDIATELY WITHOUT CLEANUP.  ***\n"
		"*************************************************************\n" );
	    fflush(stderr);
	    exit(ERR_INTERRUPT);
	  }
	  break;

      case SIGUSR1:
	if ( verbose >= -1 )
	    verbose--;
	TRACE("#SIGNAL# USR1: verbose = %d\n",verbose);
	fprintf(stderr,usr_msg,1,"DECREASED",verbose);
	break;

      case SIGUSR2:
	if ( verbose < 4 )
	    verbose++;
	TRACE("#SIGNAL# USR2: verbose = %d\n",verbose);
	fprintf(stderr,usr_msg,2,"INCREASED",verbose);
	break;

      default:
	TRACE("#SIGNAL# %d\n",signum);
    }
    fflush(stderr);
}

///////////////////////////////////////////////////////////////////////////////

void SetupLib ( int argc, char ** argv, ccp p_progname, enumProgID prid )
{
 #ifdef DEBUG
    if (!TRACE_FILE)
    {
	char fname[100];
	snprintf(fname,sizeof(fname),"_trace-%s.tmp",p_progname);
	TRACE_FILE = fopen(fname,"w");
	if (!TRACE_FILE)
	    fprintf(stderr,"open TRACE_FILE failed: %s\n",fname);
    }
 #endif

 #ifdef __BYTE_ORDER
    TRACE("__BYTE_ORDER=%d\n",__BYTE_ORDER);
 #endif
 #ifdef LITTLE_ENDIAN
    TRACE("LITTLE_ENDIAN=%d\n",LITTLE_ENDIAN);
 #endif
 #ifdef BIG_ENDIAN
    TRACE("BIG_ENDIAN=%d\n",BIG_ENDIAN);
 #endif

    // numeric types

    TRACE("-\n");
    TRACE_SIZEOF(bool);
    TRACE_SIZEOF(short);
    TRACE_SIZEOF(int);
    TRACE_SIZEOF(long);
    TRACE_SIZEOF(long long);
    TRACE_SIZEOF(size_t);
    TRACE_SIZEOF(off_t);
    TRACE_SIZEOF(option_t);

    TRACE_SIZEOF(u8);
    TRACE_SIZEOF(u16);
    TRACE_SIZEOF(u32);
    TRACE_SIZEOF(u64);
    TRACE_SIZEOF(s8);
    TRACE_SIZEOF(s16);
    TRACE_SIZEOF(s32);
    TRACE_SIZEOF(s64);

    // base types A-Z

    TRACE("-\n");
    TRACE_SIZEOF(AWData_t);
    TRACE_SIZEOF(AWRecord_t);
    TRACE_SIZEOF(CISO_Head_t);
    TRACE_SIZEOF(CISO_Info_t);
    TRACE_SIZEOF(CheckDisc_t);
    TRACE_SIZEOF(CheckWBFS_t);
    TRACE_SIZEOF(CommandTab_t);
    TRACE_SIZEOF(FileAttrib_t);
    TRACE_SIZEOF(FileCache_t);
    TRACE_SIZEOF(FilePattern_t);
    TRACE_SIZEOF(File_t);
    TRACE_SIZEOF(ID_DB_t);
    TRACE_SIZEOF(ID_t);
    TRACE_SIZEOF(InfoCommand_t);
    TRACE_SIZEOF(InfoOption_t);
    TRACE_SIZEOF(InfoUI_t);
    TRACE_SIZEOF(IOData_t);
    TRACE_SIZEOF(IsoFileIterator_t);
    TRACE_SIZEOF(IsoMappingItem_t);
    TRACE_SIZEOF(IsoMapping_t);
    TRACE_SIZEOF(Iterator_t);
    TRACE_SIZEOF(MemMapItem_t);
    TRACE_SIZEOF(MemMap_t);
    TRACE_SIZEOF(ParamList_t);
    TRACE_SIZEOF(PartitionInfo_t);
    TRACE_SIZEOF(PrintTime_t);
    TRACE_SIZEOF(RegionInfo_t);
    TRACE_SIZEOF(StringField_t);
    TRACE_SIZEOF(StringList_t);
    TRACE_SIZEOF(SubstString_t);
    TRACE_SIZEOF(SuperFile_t);
    TRACE_SIZEOF(TDBfind_t);
    TRACE_SIZEOF(Verify_t);
    TRACE_SIZEOF(WBFS_t);
    TRACE_SIZEOF(WDF_Chunk_t);
    TRACE_SIZEOF(WDF_Head_t);
    TRACE_SIZEOF(WDPartInfo_t);
    TRACE_SIZEOF(WDiscInfo_t);
    TRACE_SIZEOF(WDiscListItem_t);
    TRACE_SIZEOF(WDiscList_t);
    TRACE_SIZEOF(WiiFstFile_t);
    TRACE_SIZEOF(WiiFstInfo_t);
    TRACE_SIZEOF(WiiFstPart_t);
    TRACE_SIZEOF(WiiFst_t);

    // base types a-z

    TRACE("-\n");
    TRACE_SIZEOF(aes_key_t);
    TRACE_SIZEOF(dcUnicodeTripel);
    TRACE_SIZEOF(dol_header_t);
    TRACE_SIZEOF(wbfs_disc_info_t);
    TRACE_SIZEOF(wbfs_disc_t);
    TRACE_SIZEOF(wbfs_head_t);
    TRACE_SIZEOF(wbfs_inode_info_t);
    TRACE_SIZEOF(wbfs_param_t);
    TRACE_SIZEOF(wbfs_t);
    TRACE_SIZEOF(wd_boot_t);
    TRACE_SIZEOF(wd_fst_item_t);
    TRACE_SIZEOF(wd_header_t);
    TRACE_SIZEOF(wd_part_control_t);
    TRACE_SIZEOF(wd_part_count_t);
    TRACE_SIZEOF(wd_part_header_t);
    TRACE_SIZEOF(wd_part_sector_t);
    TRACE_SIZEOF(wd_part_table_entry_t);
    TRACE_SIZEOF(wd_region_set_t);
    TRACE_SIZEOF(wd_ticket_t);
    TRACE_SIZEOF(wd_tmd_content_t);
    TRACE_SIZEOF(wd_tmd_t);
    TRACE_SIZEOF(wiidisc_t);

    // assertions

    TRACE("-\n");
    ASSERT( 1 == sizeof(u8)  );
    ASSERT( 2 == sizeof(u16) );
    ASSERT( 4 == sizeof(u32) );
    ASSERT( 8 == sizeof(u64) );
    ASSERT( 1 == sizeof(s8)  );
    ASSERT( 2 == sizeof(s16) );
    ASSERT( 4 == sizeof(s32) );
    ASSERT( 8 == sizeof(s64) );

    ASSERT( Count1Bits32(sizeof(WDF_Hole_t)) == 1 );
    ASSERT( sizeof(CISO_Head_t) == CISO_HEAD_SIZE );

    ASSERT(  79 == strlen(sep_79) );
    ASSERT( 200 == strlen(sep_200) );

    validate_file_format_sizes(1);

    //----- setup textmode for cygwin stdout+stderr

    #if defined(__CYGWIN__)
	setmode(fileno(stdout),O_TEXT);
	setmode(fileno(stderr),O_TEXT);
	//setlocale(LC_ALL,"en_US.utf-8");
    #endif


    //----- setup prog id

    prog_id = prid;

    #ifdef WIIMM_TRUNK
	revision_id = (prid << 20) + SYSTEMID + REVISION_NUM + REVID_WIIMM_TRUNK;
    #elif defined(WIIMM)
	revision_id = (prid << 20) + SYSTEMID + REVISION_NUM + REVID_WIIMM;
    #else
	revision_id = (prid << 20) + SYSTEMID + REVISION_NUM + REVID_UNKNOWN;
    #endif


    //----- setup progname

    if ( argc > 0 && *argv && **argv )
	p_progname = *argv;
    progname = strrchr(p_progname,'/');
    progname = progname ? progname+1 : p_progname;
    argv[0] = (char*)progname;

    TRACE("##PROG## REV-ID=%08x, PROG-ID=%d, PROG-NAME=%s\n",
		revision_id, prog_id, progname );


    //----- setup signals

    static const int sigtab[] = { SIGTERM, SIGINT, SIGUSR1, SIGUSR2, -1 };
    int i;
    for ( i = 0; sigtab[i] >= 0; i++ )
    {
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = &sig_handler;
	sigaction(sigtab[i],&sa,0);
    }


    //----- setup search_path

    ccp *sp = search_path, *sp2;
    ASSERT( sizeof(search_path)/sizeof(*search_path) > 4 );

    // determine program path
    char proc_path[30];
    snprintf(proc_path,sizeof(proc_path),"/proc/%u/exe",getpid());
    TRACE("PROC-PATH: %s\n",proc_path);

    static char share[] = "/share/wit/";
    static char local_share[] = "/usr/local/share/wit/";

    char path[PATH_MAX];
    if (readlink(proc_path,path,sizeof(path)))
    {
	// program path found!
	TRACE("PROG-PATH: %s\n",path);

	char * file_ptr = strrchr(path,'/');
	if ( file_ptr )
	{
	    // seems to be a real path -> terminate string behind '/'
	    *++file_ptr = 0;
	    *sp = strdup(path);
	    if (!*sp)
		OUT_OF_MEMORY;
	    TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
	    sp++;

	    if ( file_ptr-5 >= path && !memcmp(file_ptr-4,"/bin/",5) )
	    {
		StringCopyS(file_ptr-5,sizeof(path),share);
		*sp = strdup(path);
		if (!*sp)
		    OUT_OF_MEMORY;
		TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
		sp++;
	    }
	}
    }

    // insert 'local_share' if not already done

    for ( sp2 = search_path; sp2 < sp && strcmp(*sp2,local_share); sp2++ )
	;
    if ( sp2 == sp )
    {
	*sp = strdup(local_share);
	if (!*sp)
	    OUT_OF_MEMORY;
	TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
	sp++;
    }

    // insert CWD if not already done
    getcwd(path,sizeof(path)-1);
    strcat(path,"/");
    for ( sp2 = search_path; sp2 < sp && strcmp(*sp2,path); sp2++ )
	;
    if ( sp2 == sp )
    {
	*sp = strdup("./");
	if (!*sp)
	    OUT_OF_MEMORY;
	TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
	sp++;
    }

    *sp = 0;
    ASSERT( sp - search_path < sizeof(search_path)/sizeof(*search_path) );


    //----- setup language info

    char * wit_lang = getenv("WIT_LANG");
    if ( !wit_lang || !*wit_lang )
	wit_lang = getenv("WWT_LANG");
    if ( wit_lang && *wit_lang )
    if ( wit_lang && *wit_lang )
    {
	lang_info = strdup(wit_lang);
	TRACE("LANG_INFO = %s [WIT_LANG]\n",lang_info);
    }
    else
    {
	char * lc_ctype = getenv("LC_CTYPE");
	if (lc_ctype)
	{
	    char * lc_ctype_end = lc_ctype;
	    while ( *lc_ctype_end >= 'a' && *lc_ctype_end <= 'z' )
		lc_ctype_end++;
	    const int len = lc_ctype_end - lc_ctype;
	    if ( len > 0 )
	    {
		char * temp = malloc(len+1);
		if (!temp)
		    OUT_OF_MEMORY;
		memcpy(temp,lc_ctype,len);
		temp[len] = 0;
		lang_info = temp;
		TRACE("LANG_INFO = %s\n",lang_info);
	    }
	}
    }


    //----- setup common key
    
    {
	static u8 key_base[] = "Wiimms WBFS Tool";
	static u8 key_mask[] =
	{
		0x87, 0xf1, 0xaf, 0x7e,  0x24, 0x7c, 0x50, 0x87,
		0x39, 0xca, 0x9e, 0xff,  0xd8, 0x35, 0x3c, 0x51
	};

	u8 h1[WII_HASH_SIZE], h2[WII_HASH_SIZE], key[WII_KEY_SIZE];
	
	SHA1(key_base,WII_KEY_SIZE,h1);
	SHA1(wd_get_common_key(),WII_KEY_SIZE,h2);
	
	int i;
	for ( i = 0; i < WII_KEY_SIZE; i++ )
	    key[i] = key_mask[i] ^ h1[i] ^ h2[i];

	wd_set_common_key(key);
    }

    //----- setup data structures

    InitializeAllFilePattern();
}

///////////////////////////////////////////////////////////////////////////////

enumError CheckEnvOptions ( ccp varname, check_opt_func func )
{
    TRACE("CheckEnvOptions(%s,%p)\n",varname,func);

    ccp env = getenv(varname);
    if ( !env || !*env )
	return ERR_OK;

    TRACE("env[%s] = %s\n",varname,env);

    const int envlen = strlen(env);
    char * buf = malloc(envlen+1);
    if (!buf)
	OUT_OF_MEMORY;
    char * dest = buf;

    int argc = 1; // argv[0] = progname
    ccp src = env;
    while (*src)
    {
	while ( *src > 0 && *src <= ' ' ) // skip blanks & control
	    src++;

	if (!*src)
	    break;

	argc++;
	while ( *(u8*)src > ' ' )
	    *dest++ = *src++;
	*dest++ = 0;
	ASSERT( dest <= buf+envlen+1 );
    }
    TRACE("argc = %d\n",argc);

    char ** argv = malloc((argc+1)*sizeof(*argv));
    if (!argv)
	OUT_OF_MEMORY;
    argv[0] = (char*)progname;
    argv[argc] = 0;
    dest = buf;
    int i;
    for ( i = 1; i < argc; i++ )
    {
	TRACE("argv[%d] = %s\n",i,dest);
	argv[i] = dest;
	while (*dest)
	    dest++;
	dest++;
	ASSERT( dest <= buf+envlen+1 );
    }

    enumError stat = func(argc,argv,true);
    if (stat)
	fprintf(stderr,
	    "Errors above while scanning the environment variable '%s'\n",varname);

    // don't free() because is's possible that there are pointers to arguments
    //free(argv);
    //free(buf);

    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  error messages                 ///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetErrorName ( int stat )
{
    switch(stat)
    {
	case ERR_OK:			return "OK";
	case ERR_DIFFER:		return "FILES DIFFER";
	case ERR_JOB_IGNORED:		return "JOB IGNORED";
	case ERR_WARNING:		return "WARNING";

	case ERR_INVALID_FILE:		return "INVALID FILE";

	case ERR_NO_WDF:		return "NO WDF";
	case ERR_WDF_VERSION:		return "WDF VERSION NOT SUPPORTED";
	case ERR_WDF_SPLIT:		return "SPLITTED WDF NOT SUPPORTED";
	case ERR_WDF_INVALID:		return "INVALID WDF";

	case ERR_NO_CISO:		return "NO WDF";
	case ERR_CISO_INVALID:		return "INVALID WDF";

	case ERR_WDISC_NOT_FOUND:	return "WDISC NOT FOUND";
	case ERR_NO_WBFS_FOUND:		return "NO WBFS FOUND";
	case ERR_TO_MUCH_WBFS_FOUND:	return "TO MUCH WBFS FOUND";
	case ERR_WBFS_INVALID:		return "INVALID WBFS";

	case ERR_ALREADY_EXISTS:	return "FILE ALREADY EXISTS";
	case ERR_CANT_OPEN:		return "CAN'T OPEN FILE";
	case ERR_CANT_CREATE:		return "CAN'T CREATE FILE";
	case ERR_CANT_CREATE_DIR:	return "CAN'T CREATE DIRECTORY";
	case ERR_WRONG_FILE_TYPE:	return "WRONG FILE TYPE";
	case ERR_READ_FAILED:		return "READ FILE FAILED";
	case ERR_REMOVE_FAILED:		return "REMOVE FILE FAILED";
	case ERR_WRITE_FAILED:		return "WRITE FILE FAILED";

	case ERR_WBFS:			return "WBFS ERROR";

	case ERR_MISSING_PARAM:		return "MISSING PARAMETERS";
	case ERR_SEMANTIC:		return "SEMANTIC ERROR";
	case ERR_SYNTAX:		return "SYNTAX ERROR";

	case ERR_INTERRUPT:		return "INTERRUPT";

	case ERR_NOT_IMPLEMENTED:	return "NOT IMPLEMENTED YET";
	case ERR_INTERNAL:		return "INTERNAL ERROR";
	case ERR_OUT_OF_MEMORY:		return "OUT OF MEMORY";
	case ERR_FATAL:			return "FATAL ERROR";
    }
    return "?";
}

///////////////////////////////////////////////////////////////////////////////

ccp GetErrorText ( int stat )
{
    switch(stat)
    {
	case ERR_OK:			return "Ok";
	case ERR_DIFFER:		return "Files differ";
	case ERR_JOB_IGNORED:		return "Job Ignored";
	case ERR_WARNING:		return "Warning";

	case ERR_INVALID_FILE:		return "File has invalid content";

	case ERR_NO_WDF:		return "File is not a WDF";
	case ERR_WDF_VERSION:		return "WDF version not supported";
	case ERR_WDF_SPLIT:		return "Splitted WDF not supported";
	case ERR_WDF_INVALID:		return "Invalid WDF";

	case ERR_NO_CISO:		return "File is not a CISO";
	case ERR_CISO_INVALID:		return "Invalid CISO";

	case ERR_WDISC_NOT_FOUND:	return "Wii disc not found";
	case ERR_NO_WBFS_FOUND:		return "No WBFS found";
	case ERR_TO_MUCH_WBFS_FOUND:	return "To much WBFS found";
	case ERR_WBFS_INVALID:		return "Invalid WBFS";

	case ERR_ALREADY_EXISTS:	return "File already exists";
	case ERR_CANT_OPEN:		return "Can't open file";
	case ERR_CANT_CREATE:		return "Can't create file";
	case ERR_CANT_CREATE_DIR:	return "Can't create directory";
	case ERR_WRONG_FILE_TYPE:	return "Wrong type of file";
	case ERR_READ_FAILED:		return "Reading from file failed";
	case ERR_REMOVE_FAILED:		return "Removing a file failed";
	case ERR_WRITE_FAILED:		return "Writing to file failed";

	case ERR_WBFS:			return "WBFS error";

	case ERR_MISSING_PARAM:		return "Missing parameters";
	case ERR_SEMANTIC:		return "Semantic error";
	case ERR_SYNTAX:		return "Syntax error";

	case ERR_INTERRUPT:		return "Program interrupted";

	case ERR_NOT_IMPLEMENTED:	return "Not implemented yet";
	case ERR_INTERNAL:		return "Internal error";
	case ERR_OUT_OF_MEMORY:		return "Allocation of dynamic memory failed";
	case ERR_FATAL:			return "Fatal error";
    }
    return "?";
}

///////////////////////////////////////////////////////////////////////////////

enumError last_error = ERR_OK;
enumError max_error  = ERR_OK;
u32 error_count = 0;

///////////////////////////////////////////////////////////////////////////////

int PrintError ( ccp func, ccp file, uint line,
		int syserr, enumError err_code, ccp format, ... )
{
    fflush(stdout);
    char msg[1000];
    const int plen = strlen(progname)+2;

    if (format)
    {
	va_list arg;
	va_start(arg,format);
	vsnprintf(msg,sizeof(msg),format,arg);
	msg[sizeof(msg)-2] = 0;
	va_end(arg);

	const int mlen = strlen(msg);
	if ( mlen > 0 && msg[mlen-1] != '\n' )
	{
	    msg[mlen]   = '\n';
	    msg[mlen+1] = 0;
	}
    }
    else
	StringCat2S(msg,sizeof(msg),GetErrorText(err_code),"\n");

    ccp prefix = err_code == ERR_OK ? "" : err_code <= ERR_WARNING ? "! " : "!! ";

 #ifdef DEBUG
    TRACE("%s%s #%d [%s] in %s() @ %s#%d\n",
		prefix, err_code <= ERR_WARNING ? "WARNING" : "ERROR",
		err_code, GetErrorName(err_code), func, file, line );
    TRACE("%s%*s%s",prefix,plen,"",msg);
    if (syserr)
	TRACE("!! ERRNO=%d: %s\n",syserr,strerror(syserr));
    fflush(TRACE_FILE);
 #endif

 #if defined(EXTENDED_ERRORS)
    if ( err_code > ERR_WARNING )
 #else
    if ( err_code >= ERR_NOT_IMPLEMENTED )
 #endif
    {
	if ( err_code > ERR_WARNING )
	    fprintf(stderr,"%s%s: ERROR #%d [%s] in %s() @ %s#%d\n",
		prefix, progname, err_code, GetErrorName(err_code), func, file, line );
	else
	    fprintf(stderr,"%s%s: WARNING in %s() @ %s#%d\n",
		prefix, progname, func, file, line );

     #if defined(EXTENDED_ERRORS) && EXTENDED_ERRORS > 1
	fprintf(stderr,"%s -> %s/%s?annotate=%d#l%d\n",
		prefix, URI_VIEWVC, file, REVISION_NEXT, line );
     #endif

	fprintf(stderr, "%s%*s%s", prefix, plen,"", msg );
    }
    else
	fprintf(stderr,"%s%s: %s",prefix,progname,msg);

    if (syserr)
	fprintf(stderr,"%s%*s-> %s\n",prefix,plen,"",strerror(syserr));
    fflush(stderr);

    if ( err_code > ERR_OK )
	error_count++;

    last_error = err_code;
    if ( max_error < err_code )
	max_error = err_code;

    if ( err_code > ERR_NOT_IMPLEMENTED )
	exit(err_code);

    return err_code;
}

///////////////////////////////////////////////////////////////////////////////

void HexDump16 ( FILE * f, int indent, u64 addr,
		 const void * data, size_t count )
{
    HexDump(f,indent,addr,4,16,data,count);
}

//-----------------------------------------------------------------------------

void HexDump ( FILE * f, int indent, u64 addr, int addr_fw, int row_len,
		const void * p_data, size_t count )
{
    if ( !f || !p_data || !count )
	return;

    const int MAX_LEN = 256;
    char buf[MAX_LEN+1];
    
    const u8 * data = p_data;

    indent = NormalizeIndent(indent);
    addr_fw = NormalizeIndent(addr_fw);
    if ( row_len < 1 )
	row_len = 16;
    else if ( row_len > MAX_LEN )
	row_len = MAX_LEN;

    const int fw = snprintf(buf,sizeof(buf),"%llx",addr+count-1);
    if ( addr_fw < fw )
	 addr_fw = fw;

    while ( count > 0 )
    {
	fprintf(f,"%*s%*llx:", indent,"", addr_fw, addr );
	addr += row_len;
	char * dest = buf;

	int i;
	for ( i = 0; i < row_len; i++ )
	{
	    u8 ch = *data++;
	    if ( count > 0 )
	    {
		count--;
		fprintf(f,"%s%02x ", i&3 ? "" : " ", ch );
		*dest++ = ch < ' ' || ch >= 0x7f ? '.' : ch;
	    }
	    else
		fprintf(f,"%s   ", i&3 ? "" : " " );
	}
	*dest = 0;
	fprintf(f,":%s:\n",buf);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			terminal cap			///////////////
///////////////////////////////////////////////////////////////////////////////

int GetTermWidth ( int default_value, int min_value )
{
    int term_width = GetTermWidthFD(STDOUT_FILENO,-1,min_value);
    if ( term_width <= 0 )
	term_width = GetTermWidthFD(STDERR_FILENO,-1,min_value);

    return term_width > 0 ? term_width : default_value;
}

//-----------------------------------------------------------------------------

int GetTermWidthFD ( int fd, int default_value, int min_value )
{
    TRACE("GetTermWidthFD(%d,%d)\n",fd,default_value);

 #ifdef TIOCGSIZE
    TRACE(" - have TIOCGSIZE\n");
 #endif

 #ifdef TIOCGWINSZ
    TRACE(" - have TIOCGWINSZ\n");
 #endif

    if (isatty(fd))
    {
     #ifdef TIOCGSIZE
	{
	    struct ttysize ts;
	    if ( !ioctl(fd,TIOCGSIZE,&ts))
	    {
		TRACE(" - TIOCGSIZE = %d*%d\n",ts.ts_cols,ts.ts_lines);
		if ( ts.ts_cols > 0 )
		    return ts.ts_cols > min_value ? ts.ts_cols : min_value;
	    }
	}
     #endif

     #ifdef TIOCGWINSZ
	{
	    struct winsize ws;
	    if ( !ioctl(fd,TIOCGWINSZ,&ws))
	    {
		TRACE(" - TIOCGWINSZ = %d*%d\n",ws.ws_col,ws.ws_row);
		if ( ws.ws_col > 0 )
		    return ws.ws_col > min_value ? ws.ws_col : min_value;
	    }
	}
     #endif
    }

    return default_value;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    timer                        ///////////////
///////////////////////////////////////////////////////////////////////////////

u32 GetTimerMSec()
{
    struct timeval tval;
    gettimeofday(&tval,NULL);

    static time_t timebase = 0;
    if (!timebase)
	timebase = tval.tv_sec;

    return ( tval.tv_sec - timebase ) * 1000 + tval.tv_usec/1000;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintMSec ( char * buf, int bufsize, u32 msec, bool PrintMSec )
{
    if (PrintMSec)
	snprintf(buf,bufsize,"%02d:%02d:%02d.%03d",
	    msec/3600000, msec/60000%60, msec/1000%60, msec%1000 );
    else
	snprintf(buf,bufsize,"%02d:%02d:%02d",
	    msec/3600000, msec/60000%60, msec/1000%60 );
    ccp ptr = buf;
    int colon_counter = 0;
    while ( *ptr == '0' || *ptr == ':' && !colon_counter++ )
	ptr++;
    return *ptr == ':' ? ptr-1 : ptr;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                string functions                 ///////////////
///////////////////////////////////////////////////////////////////////////////

const char EmptyString[] = "";
const char MinusString[] = "-";

///////////////////////////////////////////////////////////////////////////////

void FreeString ( ccp str )
{
    noTRACE("FreeString(%p) EmptyString=%p MinusString=%p\n",
	    str, EmptyString, MinusString );
    if ( str && str != EmptyString && str != MinusString )
	free((char*)str);
}

///////////////////////////////////////////////////////////////////////////////

void * MemDup ( const void * src, size_t copylen )
{
    char * dest = malloc(copylen+1);
    if (!dest)
	OUT_OF_MEMORY;
    memcpy(dest,src,copylen);
    dest[copylen] = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

char * StringCopyE ( char * buf, char * buf_end, ccp src )
{
    // RESULT: end of copied string pointing to NULL
    // 'src' may be a NULL pointer.

    ASSERT(buf);
    ASSERT(buf<buf_end);
    buf_end--;

    if (src)
	while( buf < buf_end && *src )
	    *buf++ = *src++;

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCopyS ( char * buf, size_t buf_size, ccp src )
{
    return StringCopyE(buf,buf+buf_size,src);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCat2E ( char * buf, char * buf_end, ccp src1, ccp src2 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    ASSERT(buf);
    ASSERT(buf<buf_end);
    buf_end--;

    if (src1)
	while( buf < buf_end && *src1 )
	    *buf++ = *src1++;

    if (src2)
	while( buf < buf_end && *src2 )
	    *buf++ = *src2++;

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCat2S ( char * buf, size_t buf_size, ccp src1, ccp src2 )
{
    return StringCat2E(buf,buf+buf_size,src1,src2);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCat3E ( char * buf, char * buf_end, ccp src1, ccp src2, ccp src3 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    ASSERT(buf);
    ASSERT(buf<buf_end);
    buf_end--;

    if (src1)
	while( buf < buf_end && *src1 )
	    *buf++ = *src1++;

    if (src2)
	while( buf < buf_end && *src2 )
	    *buf++ = *src2++;

    if (src3)
	while( buf < buf_end && *src3 )
	    *buf++ = *src3++;

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCat3S ( char * buf, size_t buf_size, ccp src1, ccp src2, ccp src3 )
{
    return StringCat3E(buf,buf+buf_size,src1,src2,src3);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCat2S ( char *buf, size_t bufsize, ccp path1, ccp path2 )
{
    if ( !path1 || !*path1 )
	return path2 ? path2 : "";

    if ( !path2 || !*path2 )
	return path1;

    char * ptr = StringCopyS(buf,bufsize-1,path1);
    ASSERT( ptr > buf );
    if ( ptr[-1] != '/' )
	*ptr++ = '/';
    while ( *path2 == '/' )
	path2++;
    StringCopyE(ptr,buf+bufsize,path2);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

int NormalizeIndent ( int indent )
{
    return indent < 0 ? 0 : indent < 50 ? indent : 50;
}

///////////////////////////////////////////////////////////////////////////////

int CheckIDHelper
	( const void * id, int max_len, bool allow_any_len, bool ignore_case )
{
    ASSERT(id);
    ccp ptr = id;
    ccp end = ptr + max_len;
    while ( ptr != end && ( *ptr >= 'A' && *ptr <= 'Z'
			|| *ptr >= 'a' && *ptr <= 'z' && ignore_case
			|| *ptr >= '0' && *ptr <= '9'
			|| *ptr == '_' ))
	ptr++;

    const int len = ptr - (ccp)id;
    return allow_any_len || len == 4 || len == 6 ? len : 0;
}

//-----------------------------------------------------------------------------

int CheckID ( const void * id, bool ignore_case )
{
    // check up to 7 chars
    return CheckIDHelper(id,7,false,ignore_case);
}

//-----------------------------------------------------------------------------

bool CheckID4 ( const void * id, bool ignore_case )
{
    // check exact 4 chars
    return CheckIDHelper(id,4,false,ignore_case) == 4;
}

//-----------------------------------------------------------------------------

bool CheckID6 ( const void * id, bool ignore_case )
{
    // check exact 6 chars
    return CheckIDHelper(id,6,false,ignore_case) == 6;
}

//-----------------------------------------------------------------------------

int CountIDChars( const void * id, bool ignore_case )
{
    // count number of valid ID chars
    return CheckIDHelper(id,1000,true,ignore_case);
}

//-----------------------------------------------------------------------------

char * ScanID ( char * destbuf7, int * destlen, ccp source )
{
    ASSERT(destbuf7);

    memset(destbuf7,0,7);
    ccp id_start = 0;
    if (source)
    {
	ccp src = source;

	// skip CTRL and SPACES
	while ( *src > 0 && *src <= ' ' )
	    src++;

	if ( ( *src == '*' || *src == '+' ) && ( !src[1] || src[1] == '=' ) )
	{
	    if (destlen)
		*destlen = 1;
	    return src[1] == '=' && src[2] ? (char*)src+2 : 0;
	}

	// scan first word
	const int id_len = CheckID(src,false);

	if ( id_len == 4 )
	{
	    TRACE("4 chars found:%.6s\n",id_start);
	    id_start = src;
	    src += id_len;

	    // skip CTRL and SPACES
	    while ( *src > 0 && *src <= ' ' )
		src++;

	    if (!*src)
	    {
		memcpy(destbuf7,id_start,4);
		if (destlen)
		    *destlen = 4;
		return 0;
	    }
	    id_start = 0;
	}
	else if ( id_len == 6 )
	{
	    // we have found an ID6 canidat
	    TRACE("6 chars found:%.6s\n",id_start);
	    id_start = src;
	    src += id_len;

	    // skip CTRL and SPACES
	    while ( *src > 0 && *src <= ' ' )
		src++;

	    if ( !*src || *src == '=' )
	    {
		if ( *src == '=' )
		    src++;

		// pure 'ID6' or 'ID6 = name found
		memcpy(destbuf7,id_start,6);
		if (destlen)
		    *destlen = 6;
		return *src ? (char*)src : 0;
	    }
	}

	// scan for latest '...[ID6]...'
	while (*src)
	{
	    while ( *src && *src != '[' ) // ]
		src++;

	    if ( *src == '[' && src[7] == ']' && CheckID(++src,false) == 6 )
	    {
		id_start = src;
		src += 8;
	    }
	    if (*src)
		src++;
	}
    }
    if (id_start)
	memcpy(destbuf7,id_start,6);
    if (destlen)
	*destlen = id_start ? 6 : 0;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////             time printing & scanning            ///////////////
///////////////////////////////////////////////////////////////////////////////

int opt_print_time  = PT__DEFAULT;
int opt_time_select = PT__DEFAULT & PT__USE_MASK;

///////////////////////////////////////////////////////////////////////////////

int ScanPrintTimeMode ( ccp arg, int prev_mode )
{
    #undef E
    #undef EM
    #undef IT
    #undef MT
    #undef CT
    #undef AT

    #define E PT_ENABLED
    #define EM PT__ENABLED_MASK
    #define IT PT_USE_ITIME|PT_F_ITIME
    #define MT PT_USE_MTIME|PT_F_MTIME
    #define CT PT_USE_CTIME|PT_F_CTIME
    #define AT PT_USE_ATIME|PT_F_ATIME

    static const CommandTab_t tab[] =
    {
	{ 0,			"RESET",	"-",	PT__MASK },

	{ PT_DISABLED,		"OFF",		0,	EM },
	{ PT_ENABLED,		"ON",		0,	EM },

	{ E|PT_SINGLE,		"SINGLE",	"1",	EM|PT__MULTI_MASK|PT__F_MASK },
	{ E|PT_MULTI,		"MULTI",	"+",	EM|PT__MULTI_MASK },

	{ E|PT_MULTI,		"NONE",		"0",	EM|PT__MULTI_MASK|PT__F_MASK },
	{ E|PT_MULTI|PT__F_MASK,"ALL",		"*",	EM|PT__MULTI_MASK|PT__F_MASK },

	{ E|IT,			"I",		0,	EM|PT__USE_MASK },
	{ E|MT,			"M",		0,	EM|PT__USE_MASK },
	{ E|CT,			"C",		0,	EM|PT__USE_MASK },
	{ E|AT,			"A",		0,	EM|PT__USE_MASK },

	{ E|PT_PRINT_DATE,	"DATE",		0,	EM|PT__PRINT_MASK },
	{ E|PT_PRINT_TIME,	"TIME",		"MIN",	EM|PT__PRINT_MASK },
	{ E|PT_PRINT_SEC,	"SEC",		0,	EM|PT__PRINT_MASK },

	{ E|IT|PT_PRINT_DATE,	"IDATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|MT|PT_PRINT_DATE,	"MDATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|CT|PT_PRINT_DATE,	"CDATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|AT|PT_PRINT_DATE,	"ADATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },

	{ E|IT|PT_PRINT_TIME,	"ITIME",	"IMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|MT|PT_PRINT_TIME,	"MTIME",	"MMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|CT|PT_PRINT_TIME,	"CTIME",	"CMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|AT|PT_PRINT_TIME,	"ATIME",	"AMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },

	{ E|IT|PT_PRINT_SEC,	"ISEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|MT|PT_PRINT_SEC,	"MSEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|CT|PT_PRINT_SEC,	"CSEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|AT|PT_PRINT_SEC,	"ASEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },

	{ 0,0,0,0 }
    };

    #undef E
    #undef EM
    #undef IT
    #undef MT
    #undef CT
    #undef AT

    const int stat = ScanCommandListMask(arg,tab);
    if ( stat >= 0 )
	return SetPrintTimeMode(prev_mode,stat);

    ERROR0(ERR_SYNTAX,"Illegal time mode (option --time): '%s'\n",arg);
    return PT__ERROR;
}

///////////////////////////////////////////////////////////////////////////////

int ScanAndSetPrintTimeMode ( ccp argv )
{
    const int stat = ScanPrintTimeMode(argv,opt_print_time);
    if ( stat >= 0 )
	opt_print_time  = stat;
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int SetPrintTimeMode ( int prev_mode, int new_mode )
{
    TRACE("SetPrintTimeMode(%x,%x)\n",prev_mode,new_mode);
    if ( new_mode & PT__USE_MASK )
	prev_mode = prev_mode & ~PT__USE_MASK | new_mode & PT__USE_MASK;

    prev_mode |= new_mode & PT__F_MASK;

    if ( new_mode & PT__MULTI_MASK )
	prev_mode = prev_mode & ~PT__MULTI_MASK | new_mode & PT__MULTI_MASK;

    if ( new_mode & PT__PRINT_MASK )
	prev_mode = prev_mode & ~PT__PRINT_MASK | new_mode & PT__PRINT_MASK;

    if ( new_mode & PT__ENABLED_MASK )
	prev_mode = prev_mode & ~PT__ENABLED_MASK | new_mode & PT__ENABLED_MASK;

    TRACE(" -> %x\n",prev_mode);
    return prev_mode;
}

///////////////////////////////////////////////////////////////////////////////

int EnablePrintTime ( int opt_time )
{
    return SetPrintTimeMode(PT__DEFAULT|PT_PRINT_DATE,opt_time|PT_ENABLED);
}

///////////////////////////////////////////////////////////////////////////////

void SetTimeOpt ( int opt_time )
{
    opt_print_time = SetPrintTimeMode( opt_print_time, opt_time|PT_ENABLED );
}

///////////////////////////////////////////////////////////////////////////////

void SetupPrintTime ( PrintTime_t * pt, int opt_time )
{
    TRACE("SetupPrintTime(%p,%x)\n",pt,opt_time);
    ASSERT(pt);
    memset(pt,0,sizeof(*pt));

    if ( opt_time & PT_SINGLE )
    {
	opt_time &= ~PT__F_MASK;
	switch( opt_time & PT__USE_MASK )
	{
	    case PT_USE_ITIME:	opt_time |= PT_F_ITIME; break;
	    case PT_USE_CTIME:	opt_time |= PT_F_CTIME; break;
	    case PT_USE_ATIME:	opt_time |= PT_F_ATIME; break;
	    default:		opt_time |= PT_F_MTIME; break;
	}
    }
    else if ( !(opt_time&PT__F_MASK) )
	opt_time |= PT_F_MTIME;

    ccp head_format;
    switch ( opt_time & (PT__ENABLED_MASK|PT__PRINT_MASK) )
    {
	case PT_ENABLED|PT_PRINT_SEC:
	    pt->format	= " %Y-%m-%d %H:%M:%S";
	    pt->undef	= " ---------- --:--:--";
	    head_format	= "   #-date    #-time ";
	    break;

	case PT_ENABLED|PT_PRINT_TIME:
	    pt->format	= " %Y-%m-%d %H:%M";
	    pt->undef	= " ---------- --:--";
	    head_format	= "   #-date  #-time";
	    break;

	case PT_ENABLED|PT_PRINT_DATE:
	    pt->format	= " %Y-%m-%d";
	    pt->undef	= " ----------";
	    head_format	= "   #-date  ";
	    break;

	default:
	    pt->format	= "";
	    pt->undef	= "";
	    head_format	= "";
	    opt_time &= ~PT__F_MASK;
	    break;
    }

    pt->mode = opt_time & PT__MASK;
    pt->wd1  = strlen(head_format);

    ASSERT(   pt->wd1 < PT_BUF_SIZE );
    ASSERT( 4*pt->wd1 < sizeof(pt->head) );
    ASSERT( 4*pt->wd1 < sizeof(pt->fill) );
    ASSERT( 4*pt->wd1 < sizeof(pt->tbuf) );

    pt->nelem = 0;
    char *head = pt->head, *fill = pt->fill;
    ccp mptr = "imca";
    while (*mptr)
    {
	char ch = *mptr++;
	if ( opt_time & PT_F_ITIME )
	{
	    ccp src;
	    for ( src = head_format; *src; src++ )
	    {
		*head++ = *src == '#' ? ch : *src;
		*fill++ = ' ';
	    }
	    pt->nelem++;
	}
	opt_time >>= 1;
    }
    *head = 0;
    *fill = 0;
    pt->wd  = pt->nelem * pt->wd1;

    TRACE(" -> head:   |%s|\n",pt->head);
    TRACE(" -> fill:   |%s|\n",pt->fill);
    TRACE(" -> format: |%s|\n",pt->format);
    TRACE(" -> undef:  |%s|\n",pt->undef);
}

///////////////////////////////////////////////////////////////////////////////

char * PrintTime ( PrintTime_t * pt, const FileAttrib_t * fa )
{
    ASSERT(pt);
    ASSERT(fa);

    if (!pt->wd)
	*pt->tbuf = 0;
    else
    {
	const time_t * timbuf[] = { &fa->itime, &fa->mtime, &fa->ctime, &fa->atime, 0 };
	const time_t ** timptr = timbuf;

	char *dest = pt->tbuf, *end = dest + sizeof(pt->tbuf);
	int mode;
	for ( mode = pt->mode; *timptr; timptr++, mode >>= 1 )
	    if ( mode & PT_F_ITIME )
	    {
		const time_t thetime = **timptr;
		if (!thetime)
		    dest = StringCopyE(dest,end,pt->undef);
		else
		{
		    struct tm * tm = localtime(&thetime);
		    dest += strftime(dest,end-dest,pt->format,tm);
		}
	    }
    }
    return pt->tbuf;
}

///////////////////////////////////////////////////////////////////////////////

time_t SelectTime ( const FileAttrib_t * fa, int opt_time )
{
    ASSERT(fa);
    switch ( opt_time & PT__USE_MASK )
    {
	case PT_USE_ITIME: return fa->itime;
	case PT_USE_CTIME: return fa->ctime;
	case PT_USE_ATIME: return fa->atime;
	default:	   return fa->mtime;
    }
}

///////////////////////////////////////////////////////////////////////////////

SortMode SelectSortMode ( int opt_time )
{
    switch ( opt_time & PT__USE_MASK )
    {
	case PT_USE_ITIME: return SORT_ITIME;
	case PT_USE_CTIME: return SORT_CTIME;
	case PT_USE_ATIME: return SORT_ATIME;
	default:	   return SORT_MTIME;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

time_t ScanTime ( ccp arg )
{
    static ccp tab[] =
    {
	"%Y-%m-%d %H:%M:%S",
	"%Y-%m-%d %H:%M",
	"%Y-%m-%d %H%M%S",
	"%Y-%m-%d %H%M",
	"%Y-%m-%d %H",
	"%Y-%m-%d",
	"%Y%m%d %H%M%S",
	"%Y%m%d %H%M",
	"%Y%m%d %H",
	"%Y%m%d",
	"%s",
	0
    };

    ccp * tptr;
    for ( tptr = tab; *tptr; tptr++ )
    {
	struct tm tm;
	memset(&tm,0,sizeof(tm));
	tm.tm_mon = 1;
	tm.tm_mday = 1;
	ccp res = strptime(arg,*tptr,&tm);
	if (res)
	{
	    while (isblank((int)*res))
		res++;
	    if (!*res)
	    {
		time_t tim = mktime(&tm);
		if ( tim != (time_t)-1 )
		    return tim;
	    }
	}
    }

    ERROR0(ERR_SYNTAX,"Illegal time format: %s",arg);
    return (time_t)-1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    scan size			///////////////
///////////////////////////////////////////////////////////////////////////////

u64 ScanSizeFactor ( char ch_factor, int force_base )
{
    if ( force_base == 1000 )
    {
	switch(ch_factor)
	{
	    case 'b': case 'c': return                   1;
	    case 'k': case 'K': return                1000;
	    case 'm': case 'M': return             1000000;
	    case 'g': case 'G': return          1000000000;
	    case 't': case 'T': return       1000000000000ull;
	    case 'p': case 'P': return    1000000000000000ull;
	    case 'e': case 'E': return 1000000000000000000ull;
	}
    }
    else if ( force_base == 1024 )
    {
	switch(ch_factor)
	{
	    case 'b': case 'c': return   1;
	    case 'k': case 'K': return KiB;
	    case 'm': case 'M': return MiB;
	    case 'g': case 'G': return GiB;
	    case 't': case 'T': return TiB;
	    case 'p': case 'P': return PiB;
	    case 'e': case 'E': return EiB;
	}
    }
    else
    {
	switch(ch_factor)
	{
	    case 'b':
	    case 'c': return                   1;
	    case 'k': return                1000;
	    case 'm': return             1000000;
	    case 'g': return          1000000000;
	    case 't': return       1000000000000ull;
	    case 'p': return    1000000000000000ull;
	    case 'e': return 1000000000000000000ull;

	    case 'K': return KiB;
	    case 'M': return MiB;
	    case 'G': return GiB;
	    case 'T': return TiB;
	    case 'P': return PiB;
	    case 'E': return EiB;
	}
    }
    return 0;
}

//-----------------------------------------------------------------------------

char * ScanSizeTerm ( double * num, ccp source, u64 default_factor, int force_base )
{
    ASSERT(source);

    char * end;
    double d = strtod(source,&end);
    if ( end > source )
    {
	// something was read
	u64 factor = ScanSizeFactor(*end,force_base);
	if (factor)
	    end++;
	else
	    factor = default_factor;

	if (factor)
	    d *= factor;
	else
	    end = (char*)source;
    }

    if (num)
	*num = d;

    return end;
}

//-----------------------------------------------------------------------------

char * ScanSize ( double * num, ccp source,
		  u64 default_factor1, u64 default_factor2, int force_base )
{
    ASSERT(source);
    TRACE("ScanSize(df=%llx,%llx, base=%u)\n",
			default_factor1, default_factor2, force_base );

    double sum = 0.0;
    bool add = true;
    char * end = 0;
    for (;;)
    {
	double term;
	end = ScanSizeTerm(&term,source,default_factor1,force_base);
	if ( end == source )
	    break;
	if (add)
	    sum += term;
	else
	    sum -= term;

	while ( *end > 0 && *end <= ' ' )
	    end++;

	if ( *end == '+' )
	    add = true;
	else if ( *end == '-' )
	    add = false;
	else
	    break;

	source = end+1;
	while ( *source > 0 && *source <= ' ' )
	    source++;

	if ( !*source && default_factor2 )
	{
	    if (add)
		sum += default_factor2;
	    else
		sum -= default_factor2;
	    end = (char*)source;
	    break;
	}

	default_factor1 = default_factor2;
    }

    if (num)
	*num = sum;

    return end;
}

//-----------------------------------------------------------------------------

char * ScanSizeU32 ( u32 * num, ccp source,
		     u64 default_factor1, u64 default_factor2, int force_base )
{
    double d;
    char * end = ScanSize(&d,source,default_factor1,default_factor2,force_base);
    //d = ceil(d+0.5);
    if ( d < 0 || d > ~(u32)0 )
	end = (char*)source;
    else if (num)
	*num = (u32)d;

    return end;
}

//-----------------------------------------------------------------------------

char * ScanSizeU64 ( u64 * num, ccp source,
		     u64 default_factor1, u64 default_factor2, int force_base )
{
    double d;
    char * end = ScanSize(&d,source,default_factor1,default_factor2,force_base);
    //d = ceil(d+0.5);
    if ( d < 0 || d > ~(u64)0 )
	end = (char*)source;
    else if (num)
	*num = (u64)d;

    return end;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanSizeOpt
	( double * num, ccp source,
	  u64 default_factor1, u64 default_factor2, int force_base,
	  ccp opt_name, u64 min, u64 max, bool print_err )
{
    double d;
    char * end = ScanSize(&d,source,default_factor1,default_factor2,force_base);

 #ifdef DEBUG
    {
	u64 size = d;
	TRACE("--%s %8.6g ~ %llu ~ %llu GiB ~ %llu GB\n",
		opt_name, d, size, (size+GiB/2)/GiB, (size+500000000)/1000000000 );
    }
 #endif

    enumError err = ERR_OK;

    if ( source == end || *end )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SYNTAX,
			"Illegal number for option --%s: %s\n",
			opt_name, source );
    }
    else if ( min > 0 && d < min )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SEMANTIC,
			"--%s to small (must not <%llu): %s\n",
			opt_name, min, source );
    }
    else if ( max > 0 && d > max )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SEMANTIC,
			"--%s to large (must not >%llu): %s\n",
			opt_name, max, source );
    }

    if ( num && !err )
	*num = d;
    return err;
}

//-----------------------------------------------------------------------------

enumError ScanSizeOptU64
	( u64 * num, ccp source, u64 default_factor1, int force_base,
	  ccp opt_name, u64 min, u64 max, u32 multiple, u32 pow2, bool print_err )
{
    if (!max)
	max = ~(u64)0;

    if ( pow2 && !force_base )
    {
	// try base 1024 first without error messages
	u64 val;
	if (!ScanSizeOptU64( &val, source, default_factor1, 1024,
				opt_name, min,max, multiple, pow2, false ))
	{
	    if (num)
		*num = val;
	    return ERR_OK;
	}
    }

    double d = 0.0;
    enumError err = ScanSizeOpt(&d,source,default_factor1,
				multiple ? multiple : 1,
				force_base,opt_name,min,max,print_err);

    u64 val;
    if ( d < 0.0 )
    {
	val = 0;
	err = ERR_SEMANTIC;
	if (print_err)
	    ERROR0(ERR_SEMANTIC, "--%s: negative values not allowed: %s\n",
			opt_name, source );
    }
    else
	val = d;

    if ( err == ERR_OK && pow2 > 0 )
    {
	int shift_count = 0;
	u64 shift_val = val;
	if (val)
	{
	    while (!(shift_val&1))
	    {
		shift_count++;
		shift_val >>= 1;
	    }
	}

	if ( shift_val != 1 || shift_count/pow2*pow2 != shift_count )
	{
	    err = ERR_SEMANTIC;
	    if (print_err)
		ERROR0(ERR_SYNTAX,
			"--%s: must be a power of %d but not %llu\n",
			opt_name, 1<<pow2, val );
	}
    }

    if ( err == ERR_OK && multiple > 1 )
    {
	u64 xval = val / multiple * multiple;
	if ( xval != val )
	{
	    if ( min > 0 && xval < min )
		xval += multiple;

	    if (print_err)
		ERROR0(ERR_WARNING,
			"--%s: must be a multiple of %u -> use %llu instead of %llu.\n",
			opt_name, multiple, xval, val );
	    val = xval;
	}
    }

    if ( num && !err )
	*num = val;
    return err;
}

//-----------------------------------------------------------------------------

enumError ScanSizeOptU32
	( u32 * num, ccp source, u64 default_factor1, int force_base,
	  ccp opt_name, u64 min, u64 max, u32 multiple, u32 pow2, bool print_err )
{
    if ( !max || max > ~(u32)0 )
	max = ~(u32)0;

    u64 val;
    enumError err = ScanSizeOptU64( &val, source, default_factor1, force_base,
				opt_name, min, max, multiple, pow2, print_err );

    if ( num && !err )
	*num = (u32)val;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

int ScanSplitSize ( ccp source )
{
    opt_split++;
    return ERR_OK != ScanSizeOptU64(
				&opt_split_size,
				source,
				GiB,
				0,
				"split-size",
				MIN_SPLIT_SIZE,
				0,
				DEF_SPLIT_FACTOR,
				0,
				true );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  scan num/range		///////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanNumU32 ( ccp arg, u32 * p_stat, u32 * p_num, u32 min, u32 max )
{
    ASSERT(arg);
    ASSERT(p_num);
    TRACE("ScanNumU32(%s)\n",arg);

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    char * end;
    u32 num = strtoul(arg,&end,0);
    u32 stat = end > arg;
    if (stat)
    {
	if ( num < min )
	    num = min;
	else if ( num > max )
	    num = max;

	while ( *end > 0 && *end <= ' ' )
	    end++;
    }
    else
	num = 0;

    if (p_stat)
	*p_stat = stat;
    *p_num = num;

    TRACE("END ScanNumU32() stat=%u, n=%u ->%s\n",stat,num,arg);
    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanRangeU32 ( ccp arg, u32 * p_stat, u32 * p_n1, u32 * p_n2, u32 min, u32 max )
{
    ASSERT(arg);
    ASSERT(p_n1);
    ASSERT(p_n2);
    TRACE("ScanRangeU32(%s)\n",arg);

    int stat = 0;
    u32 n1 = ~(u32)0, n2 = 0;

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    if ( *arg == '-' )
	n1 = min;
    else
    {
	char * end;
	u32 num = strtoul(arg,&end,0);
	if ( arg == end )
	    goto abort;

	stat = 1;
	arg = end;
	n1 = num;

	while ( *arg > 0 && *arg <= ' ' )
	    arg++;
    }

    if ( *arg != '-' )
    {
	stat = 1;
	n2 = n1;
	goto abort;
    }
    arg++;

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    char * end;
    n2 = strtoul(arg,&end,0);
    if ( end == arg )
	n2 = max;
    stat = 2;
    arg = end;

 abort:

    if ( stat > 0 )
    {
	if ( n1 < min )
	    n1 = min;
	if ( n2 > max )
	    n2 = max;
    }

    if ( !stat || n1 > n2 )
    {
	stat = 0;
	n1 = ~(u32)0;
	n2 = 0;
    }

    if (p_stat)
	*p_stat = stat;
    *p_n1 = n1;
    *p_n2 = n2;

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    TRACE("END ScanRangeU32() stat=%u, n=%u..%u ->%s\n",stat,n1,n2,arg);
    return (char*)arg;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  CommandTab_t                   ///////////////
///////////////////////////////////////////////////////////////////////////////

const CommandTab_t * ScanCommand
	( int * p_stat, ccp arg, const CommandTab_t * cmd_tab )
{
    ASSERT(arg);
    char cmd_buf[COMMAND_MAX];

    char *dest = cmd_buf;
    char *end  = cmd_buf + sizeof(cmd_buf) - 1;
    while ( *arg && dest < end )
	*dest++ = toupper((int)*arg++);
    *dest = 0;
    const int cmd_len = dest - cmd_buf;

    int abbrev_count = 0;
    const CommandTab_t *ct, *cmd_ct = 0, *abbrev_ct = 0;
    for ( ct = cmd_tab; ct->name1; ct++ )
    {
	if ( !strcmp(ct->name1,cmd_buf) || ct->name2 && !strcmp(ct->name2,cmd_buf) )
	{
	    cmd_ct = ct;
	    break;
	}
	if ( !memcmp(ct->name1,cmd_buf,cmd_len)
		|| ct->name2 && !memcmp(ct->name2,cmd_buf,cmd_len) )
	{
	    if ( !abbrev_ct || abbrev_ct->id != ct->id || abbrev_ct->opt != ct->opt )
	    {
		abbrev_ct = ct;
		abbrev_count++;
	    }
	}
    }

    if (cmd_ct)
	abbrev_count = 0;
    else if ( abbrev_count == 1 )
	cmd_ct = abbrev_ct;
    else if (!abbrev_count)
	abbrev_count = -1;

    if (p_stat)
	*p_stat = abbrev_count;

    return cmd_ct;
}

///////////////////////////////////////////////////////////////////////////////

int ScanCommandList ( ccp arg, const CommandTab_t * cmd_tab, CommandCallbackFunc func )
{
    ASSERT(arg);

    char cmd_buf[COMMAND_MAX];
    char *end  = cmd_buf + sizeof(cmd_buf) - 1;

    int result = 0;
    for (;;)
    {
	while ( *arg > 0 && *arg <= ' ' || *arg == ',' )
	    arg++;

	if (!*arg)
	    return result;

	char mode = 0;
	if ( !func && ( *arg == '+' || *arg == '-' || *arg == '=' ) && arg[1] )
	    mode = *arg++;
	
	char *dest = cmd_buf;
	while ( *arg > ' ' && *arg != ',' && dest < end )
	    *dest++ = *arg++;
	*dest = 0;

	const CommandTab_t * cptr = ScanCommand(0,cmd_buf,cmd_tab);
	if (!cptr)
	    return -1;

	if (func)
	{
	    result = func(cmd_buf,cmd_tab,cptr,result);
	    if ( result < 0 )
		return result;
	}
	else
	{
	    switch ( mode ? mode : cptr->opt ? '=' : '+' )
	    {
		case '+': result |=  cptr->id; break;
		case '-': result &= ~cptr->id; break;
		case '=': result  =  cptr->id; break;
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

static int ScanCommandListMaskHelper
	( ccp cmd_name, const CommandTab_t * cmd_tab,
		const CommandTab_t * cptr, int result )
{
    return cptr->opt
		? result & ~cptr->opt | cptr->id
		: cptr->id;
}

//-----------------------------------------------------------------------------

int ScanCommandListMask ( ccp arg, const CommandTab_t * cmd_tab )
{
    return ScanCommandList(arg,cmd_tab,ScanCommandListMaskHelper);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    sort mode                    ///////////////
///////////////////////////////////////////////////////////////////////////////

SortMode ScanSortMode ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ SORT_NONE,	"NONE",		"-",		SORT__MASK },

	{ SORT_ID,	"ID",		0,		SORT__MODE_MASK },
	{ SORT_NAME,	"NAME",		"N",		SORT__MODE_MASK },
	{ SORT_TITLE,	"TITLE",	"T",		SORT__MODE_MASK },
	{ SORT_FILE,	"FILE",		"F",		SORT__MODE_MASK },
	{ SORT_SIZE,	"SIZE",		"SZ",		SORT__MODE_MASK },
	{ SORT_OFFSET,	"OFFSET",	"OF",		SORT__MODE_MASK },
	{ SORT_REGION,	"REGION",	"R",		SORT__MODE_MASK },
	{ SORT_WBFS,	"WBFS",		0,		SORT__MODE_MASK },
	{ SORT_NPART,	"NPART",	0,		SORT__MODE_MASK },

	{ SORT_ITIME,	"ITIME",	"IT",		SORT__MODE_MASK },
	{ SORT_MTIME,	"MTIME",	"MT",		SORT__MODE_MASK },
	{ SORT_CTIME,	"CTIME",	"CT",		SORT__MODE_MASK },
	{ SORT_ATIME,	"ATIME",	"AT",		SORT__MODE_MASK },
	{ SORT_TIME,	"TIME",		"TI",		SORT__MODE_MASK },
	{ SORT_TIME,	"DATE",		"D",		SORT__MODE_MASK },

	{ SORT_DEFAULT,	"DEFAULT",	0,		SORT__MODE_MASK },

	{ 0,		"ASCENDING",	0,		SORT_REVERSE },
	{ SORT_REVERSE,	"DESCENDING",	"REVERSE",	SORT_REVERSE },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandListMask(arg,tab);
    if ( stat >= 0 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal sort mode (option --sort): '%s'\n",arg);
    return SORT__ERROR;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   repair mode                   ///////////////
///////////////////////////////////////////////////////////////////////////////

RepairMode ScanRepairMode ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ REPAIR_NONE,		"NONE",		"-",	1 },

	{ REPAIR_FBT,		"FBT",		"F",	0 },
	{ REPAIR_INODES,	"INODES",	"I",	0 },
	{ REPAIR_DEFAULT,	"STANDARD",	"STD",	0 },

	{ REPAIR_RM_INVALID,	"RM-INVALID",	"RI",	0 },
	{ REPAIR_RM_OVERLAP,	"RM-OVERLAP",	"RO",	0 },
	{ REPAIR_RM_FREE,	"RM-FREE",	"RF",	0 },
	{ REPAIR_RM_EMPTY,	"RM-EMPTY",	"RE",	0 },
	{ REPAIR_RM_ALL,	"RM-ALL",	"RA",	0 },

	{ REPAIR_ALL,		"ALL",		"*",	0 },
	{ 0,0,0,0 }
    };

    int stat = ScanCommandList(arg,tab,0);
    if ( stat != -1 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal repair mode (option --repair): '%s'\n",arg);
    return REPAIR__ERROR;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////              string lists & fields              ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeStringField ( StringField_t * sf )
{
    ASSERT(sf);
    memset(sf,0,sizeof(*sf));
}

///////////////////////////////////////////////////////////////////////////////

void ResetStringField ( StringField_t * sf )
{
    ASSERT(sf);
    if ( sf && sf->used > 0 )
    {
	ASSERT(sf->field);
	ccp *ptr = sf->field, *end;
	for ( end = ptr + sf->used; ptr < end; ptr++ )
	    free((char*)*ptr);
	free(sf->field);
    }
    InitializeStringField(sf);
}

///////////////////////////////////////////////////////////////////////////////

ccp FindStringField ( StringField_t * sf, ccp key )
{
    bool found;
    int idx = FindStringFieldHelper(sf,&found,key);
    return found ? sf->field[idx] : 0;
}

///////////////////////////////////////////////////////////////////////////////

bool InsertStringField ( StringField_t * sf, ccp key, bool move_key )
{
    if (!key)
	return 0;

    bool found;
    int idx = FindStringFieldHelper(sf,&found,key);
    if (found)
    {
	if (move_key)
	    free((char*)key);
    }
    else
    {
	ASSERT( sf->used <= sf->size );
	if ( sf->used == sf->size )
	{
	    sf->size += 0x100;
	    sf->field = realloc(sf->field,sf->size*sizeof(ccp));
	}
	TRACE("InsertStringField(%s,%d) %d/%d/%d\n",key,move_key,idx,sf->used,sf->size);
	ASSERT( idx <= sf->used );
	ccp * dest = sf->field + idx;
	memmove(dest+1,dest,(sf->used-idx)*sizeof(ccp));
	sf->used++;
	*dest = move_key ? key : strdup(key);
    }

    return !found;
}

///////////////////////////////////////////////////////////////////////////////

bool RemoveStringField ( StringField_t * sf, ccp key )
{
    bool found;
    uint idx = FindStringFieldHelper(sf,&found,key);
    if (found)
    {
	sf->used--;
	ASSERT( idx <= sf->used );
	ccp * dest = sf->field + idx;
	free((char*)dest);
	memmove(dest,dest+1,(sf->used-idx)*sizeof(ccp));
    }
    return found;
}

///////////////////////////////////////////////////////////////////////////////

void AppendStringField ( StringField_t * sf, ccp key, bool move_key )
{
    if (key)
    {
	ASSERT( sf->used <= sf->size );
	if ( sf->used == sf->size )
	{
	    sf->size += 0x100;
	    sf->field = realloc(sf->field,sf->size*sizeof(ccp));
	}
	TRACE("AppendStringField(%s,%d) %d/%d\n",key,move_key,sf->used,sf->size);
	ccp * dest = sf->field + sf->used++;
	*dest = move_key ? key : strdup(key);
    }
}

///////////////////////////////////////////////////////////////////////////////

uint FindStringFieldHelper ( StringField_t * sf, bool * p_found, ccp key )
{
    ASSERT(sf);

    int beg = 0;
    if ( sf && key )
    {
	int end = sf->used - 1;
	while ( beg <= end )
	{
	    uint idx = (beg+end)/2;
	    int stat = strcmp(key,sf->field[idx]);
	    if ( stat < 0 )
		end = idx - 1 ;
	    else if ( stat > 0 )
		beg = idx + 1;
	    else
	    {
		TRACE("FindStringFieldHelper(%s) FOUND=%d/%d/%d\n",
			key, idx, sf->used, sf->size );
		if (p_found)
		    *p_found = true;
		return idx;
	    }
	}
    }

    TRACE("FindStringFieldHelper(%s) failed=%d/%d/%d\n",
		key, beg, sf->used, sf->size );

    if (p_found)
	*p_found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadStringField
	( StringField_t * sf, bool keep_order, ccp filename, bool silent )
{
    ASSERT(sf);
    ASSERT(filename);
    ASSERT(*filename);

    TRACE("ReadStringField(%p,%d,%s,%d)\n",sf,keep_order,filename,silent);

    FILE * f = fopen(filename,"rb");
    if (!f)
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't open file: %s\n",filename);
	return ERR_CANT_OPEN;
    }

    while (fgets(iobuf,sizeof(iobuf)-1,f))
    {
	char * ptr = iobuf;
	while (*ptr)
	    ptr++;
	if ( ptr > iobuf && ptr[-1] == '\n' )
	{
	    ptr--;
	    if ( ptr > iobuf && ptr[-1] == '\r' )
		ptr--;
	}
	
	if ( ptr > iobuf )
	{
	    *ptr++ = 0;
	    const size_t len = ptr-iobuf;
	    ptr = malloc(len);
	    memcpy(ptr,iobuf,len);
	    if (keep_order)
		AppendStringField(sf,ptr,true);
	    else
		InsertStringField(sf,ptr,true);
	}
    }

    fclose(f);
    return ERR_OK;
}
	

///////////////////////////////////////////////////////////////////////////////

enumError WriteStringField
	( StringField_t * sf, ccp filename, bool rm_if_empty )
{
    ASSERT(sf);
    ASSERT(filename);
    ASSERT(*filename);

    TRACE("WriteStringField(%p,%s,%d)\n",sf,filename,rm_if_empty);

    if ( !sf->used && rm_if_empty )
    {
	unlink(filename);
	return ERR_OK;
    }
    
    FILE * f = fopen(filename,"wb");
    if (!f)
	return ERROR1(ERR_CANT_CREATE,"Can't create file: %s\n",filename);

    ccp *ptr = sf->field, *end;
    for ( end = ptr + sf->used; ptr < end; ptr++ )
	fprintf(f,"%s\n",*ptr);
    fclose(f);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  string lists                   ///////////////
///////////////////////////////////////////////////////////////////////////////

int AtFileHelper ( ccp arg, int mode, int (*func) ( ccp arg, int mode ) )
{
    if ( !arg || !*arg || !func )
	return 0;

    TRACE("AtFileHelper(%s)\n",arg);
    if ( *arg != '@' )
	return func(arg,mode);

    FILE * f;
    const bool use_stdin = arg[1] == '-' && !arg[2];
    if (use_stdin)
	f = stdin;
    else
    {
     #ifdef __CYGWIN__
	char buf[PATH_MAX];
	NormalizeFilenameCygwin(buf,sizeof(buf),arg+1);
	f = fopen(buf,"r");
     #else
	f = fopen(arg+1,"r");
     #endif
	if (!f)
	    return func(arg,mode);
    }

    ASSERT(f);

    const int bufsize = 1000;
    char buf[bufsize+1];

    u32 max_stat = 0;
    while (fgets(buf,bufsize,f))
    {
	char * ptr = buf;
	while (*ptr)
	    ptr++;
	if ( ptr > buf && ptr[-1] == '\n' )
	    ptr--;
	if ( ptr > buf && ptr[-1] == '\r' )
	    ptr--;
	*ptr = 0;
	const u32 stat = func(buf,true);
	if ( max_stat < stat )
	     max_stat = stat;
    }
    fclose(f);
    return max_stat;
}

///////////////////////////////////////////////////////////////////////////////

uint n_param = 0, id6_param_found = 0;
ParamList_t * first_param = 0;
ParamList_t ** append_param = &first_param;

///////////////////////////////////////////////////////////////////////////////

ParamList_t * AppendParam ( ccp arg, int is_temp )
{
    if ( !arg || !*arg )
	return 0;

    TRACE("ARG#%02d: %s\n",n_param,arg);

    static ParamList_t * pool = 0;
    static int n_pool = 0;

    if (!n_pool)
    {
	const int alloc_count = 100;
	pool = (ParamList_t*) calloc(alloc_count,sizeof(ParamList_t));
	if (!pool)
	    OUT_OF_MEMORY;
	n_pool = 100;
    }

    n_pool--;
    ParamList_t * param = pool++;
    if (is_temp)
    {
	param->arg = strdup(arg);
	if (!param->arg)
	    OUT_OF_MEMORY;
    }
    else
	param->arg = (char*)arg;
    param->count = 0;
    param->next  = 0;
    noTRACE("INS: A=%p->%p P=%p &N=%p->%p\n",
	    append_param, *append_param,
	    param, &param->next, param->next );
    *append_param = param;
    append_param = &param->next;
    noTRACE("  => A=%p->%p\n", append_param, *append_param );
    n_param++;

    return param;
}

///////////////////////////////////////////////////////////////////////////////

int AddParam ( ccp arg, int is_temp )
{
    return AppendParam(arg,is_temp) ? 0 : 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////              string substitutions               ///////////////
///////////////////////////////////////////////////////////////////////////////

char * SubstString
	( char * buf, size_t bufsize, SubstString_t * tab, ccp source, int * count )
{
    ASSERT(buf);
    ASSERT(bufsize > 1);
    ASSERT(tab);
    TRACE("SubstString(%s)\n",source);

    char tempbuf[PATH_MAX];
    int conv_count = 0;

    char *dest = buf;
    char *end = buf + bufsize + 1;
    if (source)
	while ( dest < end && *source )
	{
	    if ( *source != escape_char || *++source == escape_char )
	    {
		*dest++ = *source++;
		continue;
	    }
	    ccp start = source - 1;

	    u32 p1, p2, stat;
	    source = ScanRangeU32(source,&stat,&p1,&p2,0,~(u32)0);
	    if ( stat == 1 )
		p1 = 0;
	    else if ( stat < 1 )
	    {
		p1 = 0;
		p2 = ~(u32)0;
	    }

	    char ch = *source++;
	    int convert = 0;
	    if ( ch == 'u' || ch == 'U' )
	    {
		convert++;
		ch = *source++;
	    }
	    else if ( ch == 'l' || ch == 'L' )
	    {
		convert--;
		ch = *source++;
	    }
	    if (!ch)
		break;

	    size_t count = source - start;

	    SubstString_t * ptr;
	    for ( ptr = tab; ptr->c1; ptr++ )
		if ( ch == ptr->c1 || ch == ptr->c2 )
		{
		    if (ptr->str)
		    {
			const size_t slen = strlen(ptr->str);
			if ( p1 > slen )
			    p1 = slen;
			if ( p2 > slen )
			    p2 = slen;
			count = p2 - p1;
			start = ptr->str+p1;
			conv_count++;
		    }
		    else
			count = 0;
		    break;
		}

	    if (!ptr->c1) // invalid conversion
		convert = 0;

	    if ( count > sizeof(tempbuf)-1 )
		 count = sizeof(tempbuf)-1;
	    TRACE("COPY '%.*s' conv=%d\n",(int)count,start,convert);
	    if ( convert > 0 )
	    {
		char * tp = tempbuf;
		while ( count-- > 0 )
		    *tp++ = toupper((int)*start++);
		*tp = 0;
	    }
	    else if ( convert < 0 )
	    {
		char * tp = tempbuf;
		while ( count-- > 0 )
		    *tp++ = tolower((int)*start++); // cygwin needs the '(int)'
		*tp = 0;
	    }
	    else
	    {
		memcpy(tempbuf,start,count);
		tempbuf[count] = 0;
	    }
	    dest = NormalizeFileName(dest,end,tempbuf,ptr->allow_slash);
	}

    if (count)
	*count = conv_count;
    *dest = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

int ScanEscapeChar ( ccp arg )
{
    if ( arg && strlen(arg) > 1 )
    {
	ERROR0(ERR_SYNTAX,"Illegal character (option --esc): '%s'\n",arg);
	return -1;
    }

    escape_char = arg ? *arg : 0;
    return (unsigned char)escape_char;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   Memory Maps                   ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeMemMap ( MemMap_t * mm )
{
    ASSERT(mm);
    memset(mm,0,sizeof(*mm));
}

///////////////////////////////////////////////////////////////////////////////

void ResetMemMap ( MemMap_t * mm )
{
    ASSERT(mm);

    uint i;
    if (mm->field)
    {
	for ( i = 0; i < mm->used; i++ )
	    free(mm->field[i]);
	free(mm->field);
    }
    memset(mm,0,sizeof(*mm));
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * FindMemMap ( MemMap_t * mm, off_t off, off_t size )
{
    ASSERT(mm);

    off_t off_end = off + size;
    
    int beg = 0;
    int end = mm->used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	MemMapItem_t * mi = mm->field[idx];
	if ( off_end <= mi->off )
	    end = idx - 1 ;
	else if ( off >= mi->off + mi->size )
	    beg = idx + 1;
	else
	    return mi;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * InsertMemMap ( MemMap_t * mm, off_t off, off_t size )
{
    ASSERT(mm);
    uint idx = FindMemMapHelper(mm,off,size);

    ASSERT( mm->used <= mm->size );
    if ( mm->used == mm->size )
    {
	mm->size += 64;
	mm->field = realloc(mm->field,mm->size*sizeof(MemMapItem_t*));
    }

    ASSERT( idx <= mm->used );
    MemMapItem_t ** dest = mm->field + idx;
    memmove(dest+1,dest,(mm->used-idx)*sizeof(MemMapItem_t*));
    mm->used++;

    MemMapItem_t * mi = malloc(sizeof(MemMapItem_t));
    if (!mi)
	OUT_OF_MEMORY;
    mi->off  = off;
    mi->size = size;
    mi->overlap = 0;
    *dest = mi;
    return mi;
}

///////////////////////////////////////////////////////////////////////////////

uint FindMemMapHelper ( MemMap_t * mm, off_t off, off_t size )
{
    ASSERT(mm);

    int beg = 0;
    int end = mm->used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	MemMapItem_t * mi = mm->field[idx];
	if ( off < mi->off )
	    end = idx - 1 ;
	else if ( off > mi->off )
	    beg = idx + 1;
	else if ( size < mi->size )
	    end = idx - 1 ;
	else if ( size > mi->size )
	    beg = idx + 1;
	else
	{
	    TRACE("FindMemMapHelper(%llx,%llx) FOUND=%d/%d/%d\n",
		    (u64)off, (u64)size, idx, mm->used, mm->size );
	    return idx;
	}
    }

    TRACE("FindStringFieldHelper(%llx,%llx) failed=%d/%d/%d\n",
		(u64)off, (u64)size, beg, mm->used, mm->size );
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

uint CalCoverlapMemMap ( MemMap_t * mm )
{
    ASSERT(mm);

    uint i, count = 0;
    MemMapItem_t * prev = 0;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	ptr->overlap = 0;
	if ( prev && ptr->off < prev->off + prev->size )
	{
	    ptr ->overlap |= 1;
	    prev->overlap |= 2;
	    count++;
	}
	prev = ptr;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

void PrintMemMap ( MemMap_t * mm, FILE * f, int indent )
{
    ASSERT(mm);
    if ( !f || !mm->used )
	return;

    CalCoverlapMemMap(mm);
    indent = NormalizeIndent(indent);

    static char ovl[][3] = { "  ", "!.", ".!", "!!" };

    int i, max_ilen = 4;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	ptr->info[sizeof(ptr->info)-1] = 0;
	const int ilen = strlen(ptr->info);
	if ( max_ilen < ilen )
	    max_ilen = ilen;
    }

    fprintf(f,"%*s      unused :   off(beg) ..   off(end) :      size : info\n%*s%.*s\n",
	    indent, "", indent, "", max_ilen+54, sep_200 );

    off_t max_end = mm->begin;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	if ( !i && max_end > ptr->off )
	    max_end = ptr->off;
	const off_t end = ptr->off + ptr->size;
	if ( ptr->off > max_end )
	    fprintf(f,"%*s%s%10llx :%11llx ..%11llx :%10llx : %s\n",
		indent, "", ovl[ptr->overlap&3], (u64)( ptr->off - max_end ),
		(u64)ptr->off, (u64)end, (u64)ptr->size, ptr->info );
	else
	    fprintf(f,"%*s%s           :%11llx ..%11llx :%10llx : %s\n",
		indent, "", ovl[ptr->overlap&3],
		(u64)ptr->off, (u64)end, (u64)ptr->size, ptr->info );
	if ( max_end < end )
	    max_end = end;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			random mumbers			///////////////
///////////////////////////////////////////////////////////////////////////////
// thanx to Donald Knuth

const u32 RANDOM32_C_ADD = 2 * 197731421; // 2 Primzahlen
const u32 RANDOM32_COUNT_BASE = 4294967; // Primzahl == ~UINT_MAX32/1000;

static int random32_a_index = -1;	// Index in die a-Tabelle
static u32 random32_count = 1;		// Abwaerts-Zähler bis zum Wechsel von a,c
static u32 random32_a,
	   random32_c,
	   random32_X;			// Die letzten Werte

static u32 random32_a_tab[] =		// Init-Tabelle
{
    0xbb40e62d, 0x3dc8f2f5, 0xdc024635, 0x7a5b6c8d,
    0x583feb95, 0x91e06dbd, 0xa7ec03f5, 0
};

//-----------------------------------------------------------------------------

u32 Random32 ( u32 max )
{
    if (!--random32_count)
    {
	// Neue Berechnung von random32_a und random32_c faellig

	if ( random32_a_index < 0 )
	{
	    // allererste Initialisierung auf Zeitbasis
	    Seed32Time();
	}
	else
	{
	    random32_c += RANDOM32_C_ADD;
	    random32_a = random32_a_tab[++random32_a_index];
	    if (!random32_a)
	    {
		random32_a_index = 0;
		random32_a = random32_a_tab[0];
	    }

	    random32_count = RANDOM32_COUNT_BASE;
	}
    }

    // Jetzt erfolgt die eigentliche Berechnung

    random32_X = random32_a * random32_X + random32_c;

    if (!max)
	return random32_X;

    return ( (u64)max * random32_X ) >> 32;
}

//-----------------------------------------------------------------------------

u64 Seed32Time ()
{
    struct timeval tval;
    gettimeofday(&tval,NULL);
    const u64 random_time_bits = (u64) tval.tv_usec << 16 ^ tval.tv_sec;
    return Seed32( ( random_time_bits ^ getpid() ) * 197731421u );
}

//-----------------------------------------------------------------------------

u64 Seed32 ( u64 base )
{
    uint a_tab_len = 0;
    while (random32_a_tab[a_tab_len])
	a_tab_len++;
    const u32 base32 = base / a_tab_len;

    random32_a_index	= base % a_tab_len;
    random32_a		= random32_a_tab[random32_a_index];
    random32_c		= ( base32 & 15 ) * RANDOM32_C_ADD + 1;
    random32_X		= base32 ^ ( base >> 32 );
    random32_count	= RANDOM32_COUNT_BASE;

    return base;
}

//-----------------------------------------------------------------------------

void RandomFill ( void * buf, size_t size )
{
    size_t xsize = size / sizeof(u32);
    if (xsize)
    {
	size -= xsize * sizeof(u32);
	u32 * ptr = buf;
	while ( xsize-- > 0 )
	    *ptr++ = Random32(0);
	buf = ptr;
    }

    u8 * ptr = buf;
    while ( size-- > 0 )
	*ptr++ = Random32(0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    bit handling		///////////////
///////////////////////////////////////////////////////////////////////////////

const uchar TableBitCount[0x100] =
{
	0,1,1,2, 1,2,2,3, 1,2,2,3, 2,3,3,4,
	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,

	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,

	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,

	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,
	4,5,5,6, 5,6,6,7, 5,6,6,7, 6,7,7,8
};

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits ( const void * data, size_t len )
{
    uint count = 0;
    const uchar * d = data;
    while ( len-- > 0 )
	count += TableBitCount[*d++];
    return count;
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits8 ( u8 data )
{
    return TableBitCount[data];
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits16 ( u16 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]];
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits32 ( u32 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]]
	 + TableBitCount[d[2]]
	 + TableBitCount[d[3]];
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits64 ( u64 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]]
	 + TableBitCount[d[2]]
	 + TableBitCount[d[3]]
	 + TableBitCount[d[4]]
	 + TableBitCount[d[5]]
	 + TableBitCount[d[6]]
	 + TableBitCount[d[7]];
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////
