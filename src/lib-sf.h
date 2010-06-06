
#ifndef WIT_LIB_SF_H
#define WIT_LIB_SF_H 1

#include "lib-wdf.h"
#include "lib-ciso.h"
#include "libwbfs.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			typedef functions		///////////////
///////////////////////////////////////////////////////////////////////////////

struct SuperFile_t;

typedef enumError (*ReadFunc)
	( struct SuperFile_t * sf, off_t off, void * buf, size_t count );

typedef enumError (*WriteFunc)
	( struct SuperFile_t * sf, off_t off, const void * buf, size_t count );

typedef enumError (*ZeroFunc)
	( struct SuperFile_t * sf, off_t off, size_t count );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct IOData_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct IOData_t
{
	enumOFT   oft;			// open file mode
	ReadFunc  read_func;		// read function
	WriteFunc write_func;		// write function
	WriteFunc write_sparse_func;	// sparse write function
	ZeroFunc  write_zero_func;	// zero data write function

} IOData_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct SuperFile_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct SuperFile_t
{
	// parameters, set by user

	File_t f;			// file handling struct
	bool enable_fast;		// enables fast prosessing
	bool enable_trunc;		// truncate iso image
	int  indent;			// indent of progress and summary
	bool show_progress;		// show progress info
	bool show_summary;		// show summary statistics
	bool show_msec;			// show milli seconds in statistics
	bool allow_fst;			// allow reading of fst

	// internal values: progress

	u32 progress_start_time;	// time of start
	u32 progress_last_view_sec;	// time of last progress viewing
	u32 progress_last_calc_time;	// time of last calculation
	u64 progress_sum_time;		// sum of time weighted intervalls
	u64 progress_time_divisor;	// divisor == sum of weights
	u32 progress_max_wd;		// max width used for progress output
	ccp progress_verb;		// default is "copied"

	// internal values: file handling

	off_t file_size;		// the size of the (virtual) ISO image
	off_t max_virt_off;		// maximal used offset of virtual image

	// read and write support

	IOData_t iod;			// open file mode & read+write functions
	ReadFunc std_read_func;		// standard read function

	// WDF support

	WDF_Head_t   wh;		// the WDF header
	WDF_Chunk_t *wc;		// field with 'wc_size' elements
	int wc_size;			// number of elements in 'wc'
	int wc_used;			// number of used elements in 'wc'

	// CISO support

	CISO_Info_t ciso;		// CISO info data

	// WBFS support (read only)

	struct WBFS_t * wbfs;		// a WBFS

	// FST support
	
	struct WiiFst_t * fst;		// a FST
	MemMap_t modified_list;		// sections that is modified while
					// reading data. This data should
					// be rewritten to the destination
					// before closing the files.
	bool merge_mode;		// enable merge mode

} SuperFile_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////               interface for SuperFile_t         ///////////////
///////////////////////////////////////////////////////////////////////////////

// initialize the super file
void InitializeSF ( SuperFile_t * sf );

// remove all dynamic data
void FreeSF ( SuperFile_t * sf );

// close file + remove all dynamic data
enumError CloseSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref );

// reset == CloseSF() + reset all but user settings
enumError ResetSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref );

// remove == ResetSF() + remove all files
enumError RemoveSF ( SuperFile_t * sf );

// status
bool IsOpenSF ( const SuperFile_t * sf );

// setup oft and modifier
enumOFT SetupIOD ( SuperFile_t * sf, enumOFT force, enumOFT def );

// setup reading
enumError SetupReadSF   ( SuperFile_t * sf );		// all files
enumError SetupReadISO  ( SuperFile_t * sf );		// only iso images
enumError SetupReadWBFS ( SuperFile_t * sf );		// setup wbfs/disc reading
enumError OpenSF
	( SuperFile_t * sf, ccp fname, bool allow_non_iso, bool open_modify );

enumError SetupISOModifier ( SuperFile_t * sf );
enumError RewriteModifiedSF ( SuperFile_t * fi, SuperFile_t * fo, struct WBFS_t * wbfs );

// setup writing
enumError SetupWriteSF	( SuperFile_t * sf, enumOFT );	// setup writing
enumError SetupWriteWBFS( SuperFile_t * sf );		// setup wbfs/disc writing

// filename helper
void SubstFileNameSF ( SuperFile_t * fo, SuperFile_t * fi, ccp f_name );
int SubstFileNameBuf ( char * fname, size_t fname_size,
		SuperFile_t * fi, ccp f_name, ccp fo_fname, enumOFT fo_oft );

// main read and write functions

enumError ReadZero	( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadSF	( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadISO	( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadWBFS	( SuperFile_t * sf, off_t off, void * buf, size_t count );

enumError WriteSF	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteSparseSF	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteISO	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteSparseISO( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteWBFS	( SuperFile_t * sf, off_t off, const void * buf, size_t count );

enumError WriteZeroSF	( SuperFile_t * sf, off_t off, size_t count );
enumError WriteZeroISO	( SuperFile_t * sf, off_t off, size_t count );
enumError WriteZeroWBFS	( SuperFile_t * sf, off_t off, size_t count );

enumError SetSizeSF	( SuperFile_t * sf, off_t off );

// standard read and write wrappers

enumError ReadSwitchSF		( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError WriteSwitchSF		( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteSparseSwitchSF	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteZeroSwitchSF	( SuperFile_t * sf, off_t off, size_t count );

// libwbfs read and write wrappers

int WrapperReadSF	  ( void * p_sf, u32 offset, u32 count, void * iobuf );
int WrapperWriteSF	  ( void * p_sf, u32 lba,    u32 count, void * iobuf );
int WrapperWriteSparseSF  ( void * p_sf, u32 lba,    u32 count, void * iobuf );

enumError SparseHelper
	( SuperFile_t * sf, off_t off, const void * buf, size_t count,
	  WriteFunc func, size_t min_chunk_size );

///////////////////////////////////////////////////////////////////////////////

// progress and statistics
void CopyProgressSF ( SuperFile_t * dest, SuperFile_t * src );
void PrintProgressSF ( u64 done, u64 total, void * param );
void PrintSummarySF ( SuperFile_t * sf );

// find file type
enumFileType AnalyzeFT ( File_t * f );
enumFileType AnalyzeMemFT ( const void * buf_hd_sect_size, off_t file_size );
enumError XPrintErrorFT ( XPARM File_t * f, enumFileType err_mask );
ccp GetNameFT ( enumFileType ftype, int ignore );
enumOFT GetOFT ( SuperFile_t * sf );
u32 CountUsedIsoBlocksSF ( SuperFile_t * sf, u32 psel );
u32 CountUsedBlocks ( u8 * usage_tab, u32 block_size );

// copy functions
enumError CopySF	( SuperFile_t * in, SuperFile_t * out, u32 psel );
enumError CopyRaw	( SuperFile_t * in, SuperFile_t * out );
enumError CopyRawData	( SuperFile_t * in, SuperFile_t * out, off_t off, off_t size );
enumError CopyWDF	( SuperFile_t * in, SuperFile_t * out );
enumError CopyWBFSDisc	( SuperFile_t * in, SuperFile_t * out );
enumError CopyToWBFS	( SuperFile_t * in, SuperFile_t * out, u32 psel );

// diff functions

enumError DiffSF
(
	SuperFile_t * f1,
	SuperFile_t * f2,
	int long_count,
	partition_selector_t psel
);

enumError DiffRawSF
(
	SuperFile_t * f1,
	SuperFile_t * f2,
	int long_count
);

struct FilePattern_t;
enumError DiffFilesSF
(
	SuperFile_t * f1,
	SuperFile_t * f2,
	int long_count,
	struct FilePattern_t *pat,
	partition_selector_t psel,
	iterator_prefix_mode_t pmode
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_SF_H
