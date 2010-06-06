
#ifndef WIT_WBFS_INTERFACE_H
#define WIT_WBFS_INTERFACE_H 1

#include <stdio.h>

#include "types.h"
#include "lib-sf.h"
#include "iso-interface.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    some constants               ///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
	MIN_WBFS_SIZE		=  10000000, // minimal WBFS partition size
	MIN_WBFS_SIZE_MIB	=       100, // minimal WBFS partition size
	MAX_WBFS		=	999, // maximal number of WBFS partitions
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     partitions                  ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumPartMode
{
	PM_UNKNOWN,		// not analyzed yet
	PM_CANT_READ,		// can't read file
	PM_WRONG_TYPE,		// type must be regular or block
	PM_NO_WBFS_MAGIC,	// no WBFS Magic found
	PM_WBFS_MAGIC_FOUND,	// WBFS Magic found, further test needed
	PM_WBFS_INVALID,	// WBFS Magic found, but not a legal WBFS
//	PM_WBFS_CORRUPTED,	// WBFS with errors found
	PM_WBFS,		// WBFS found, no errors detected

} enumPartMode;

//-----------------------------------------------------------------------------

typedef enum enumPartSource
{
	PS_PARAM,	// set by --part or by parameter
	PS_AUTO,	// set by scanning because --auto is set
	PS_ENV,		// set by scanninc env 'WWT_WBFS'

} enumPartSource;

//-----------------------------------------------------------------------------

typedef struct PartitionInfo_t
{
	int  part_index;

	ccp  path;
	ccp  real_path;
	enumFileMode filemode;
	bool is_checked;
	bool ignore;
	u64  file_size;
	u64  disk_usage;
	enumPartMode part_mode;
	enumPartSource source;

	struct WDiscList_t   * wlist;
	struct PartitionInfo_t * next;

} PartitionInfo_t;

//-----------------------------------------------------------------------------

extern int wbfs_count;

extern PartitionInfo_t *  first_partition_info;
extern PartitionInfo_t ** append_partition_info;

extern int pi_count;
extern PartitionInfo_t * pi_list[MAX_WBFS+1];
extern struct WDiscList_t pi_wlist;
extern u32 pi_free_mib;

extern int opt_part;
extern int opt_auto;
extern int opt_all;

PartitionInfo_t * CreatePartitionInfo ( ccp path, enumPartSource source );
int  AddPartition ( ccp arg, int unused );
int  ScanPartitions ( bool all );
void AddEnvPartitions();
enumError AnalyzePartitions ( FILE * outfile, bool non_found_is_ok, bool scan_wbfs );
void ScanPartitionGames();

//-----------------------------------------------------------------------------

ParamList_t * CheckParamID6 ( bool unique, bool lookup_title_db );
ParamList_t * SearchParamID6 ( ccp id6 );
int PrintParamID6();

enumError CheckParamRename ( bool rename_id, bool allow_plus, bool allow_index );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    wbfs structs                 ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct WBFS_t
{
    // handles

	SuperFile_t * sf;	// attached super file
	bool sf_alloced;	// true if 'sf' is alloced
	wbfs_t * wbfs;		// the pure wbfs handle
	wbfs_disc_t * disc;	// the wbfs disc handle
	int disc_slot;		// >=0: last opened slot

    // infos calced by CalcWBFSUsage()

	u32 used_discs;
	u32 free_discs;
	u32 total_discs;

	u32 free_blocks;
	u32 used_mib;
	u32 free_mib;
	u32 total_mib;

} WBFS_t;

//-----------------------------------------------------------------------------

typedef struct CheckDisc_t
{
	char id6[7];		// id of the disc
	char no_blocks;		// no valid blocks assigned
	u16  bl_fbt;		// num of blocks marked free in fbt
	u16  bl_overlap;	// num of blocks that overlaps other discs
	u16  bl_invalid;	// num of blocks with invalid blocks
	u16  err_count;		// total count of errors

	char no_iinfo_count;	// no inode defined (not a error)

} CheckDisc_t;

//-----------------------------------------------------------------------------

typedef struct CheckWBFS_t
{
    // handles

	WBFS_t * wbfs;		// attached WBFS

    // data

	off_t  fbt_off;		// offset of fbt
	size_t fbt_size;	// size of fbt
	u32 * cur_fbt;		// current free blocks table (1 bit per block)
	u32 * good_fbt;		// calculated free blocks table (1 bit per block)

	u8  * ubl;		// used blocks (1 byte per block), copy of fbt
	u8  * blc;		// block usage counter
	CheckDisc_t * disc;	// disc list

    // statistics

	u32 err_fbt_used;	// number of wrong used marked blocks
	u32 err_fbt_free;	// number of wrong free marked blocks
	u32 err_fbt_free_wbfs0;	// number of 'err_fbt_free' depend on WBFS v0
	u32 err_no_blocks;	// total num of 'no_blocks' errors
	u32 err_bl_overlap;	// total num of 'bl_overlap' errors
	u32 err_bl_invalid;	// total num of 'bl_invalid' errors
	u32 err_total;		// total of all above

	u32 no_iinfo_count;	// total number of missing inode infos (informative)
	u32 invalid_disc_count;	// total number of invalid games

	enumError err;		// status: OK | WARNING | WBFS_INVALID

} CheckWBFS_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   Analyze WBFS                  ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumAnalyzeWBFS
{
	AW_NONE,		// invalid

	AW_HEADER,		// WBFS header found
	AW_INODES,		// Inodes with WBFS info found
	AW_DISCS,		// Discs found
	AW_CALC,		// Calculation
	AW_OLD_CALC,		// Old and buggy calculation

	AW__N,			// Number of previous values

	AW_MAX_RECORD = 20	// max number of stored records

} enumAnalyzeWBFS;

//-----------------------------------------------------------------------------

typedef struct AWRecord_t
{
	enumAnalyzeWBFS status;		// status of search
	char    title[11];		// short title of record
	char    info[30];		// additional info

	bool	magic_found;		// true: magic found
	u8	wbfs_version;		// source: wbfs_head::wbfs_version

	u32	hd_sectors;		// source: wbfs_head::n_hd_sec
	u32	hd_sector_size;		// source: wbfs_head::hd_sec_sz_s
	u32	wbfs_sectors;		// source:    wbfs_t::n_wbfs_sec
	u32	wbfs_sector_size;	// source: wbfs_head::wbfs_sec_sz_s
	u32	max_disc;		// source:    wbfs_t::max_disc
	u32	disc_info_size;		// source:    wbfs_t::disc_info_sz
	
} AWRecord_t;

//-----------------------------------------------------------------------------

typedef struct AWData_t
{
	uint n_record;			// number of used records
	AWRecord_t rec[AW_MAX_RECORD];	// results of sub search

} AWData_t;

//-----------------------------------------------------------------------------

int AnalyzeWBFS      ( AWData_t * ad, File_t * f );
int PrintAnalyzeWBFS ( AWData_t * ad, FILE * out, int indent );

//
///////////////////////////////////////////////////////////////////////////////
///////////////             discs & wbfs interface              ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeWBFS	( WBFS_t * w );
enumError ResetWBFS	( WBFS_t * w );
enumError OpenParWBFS	( WBFS_t * w, SuperFile_t * sf, bool print_err, wbfs_param_t * par );
enumError SetupWBFS	( WBFS_t * w, SuperFile_t * sf, bool print_err,
			  int sector_size, bool recover );
enumError CreateGrowingWBFS
			( WBFS_t * w, SuperFile_t * sf, off_t size, int sector_size );
enumError OpenWBFS	( WBFS_t * w, ccp filename, bool print_err, wbfs_param_t * par );
enumError FormatWBFS	( WBFS_t * w, ccp filename, bool print_err,
			  wbfs_param_t * par, int sector_size, bool recover );
enumError RecoverWBFS	( WBFS_t * w, ccp fname, bool testmode );
enumError TruncateWBFS	( WBFS_t * w );

enumError CalcWBFSUsage	( WBFS_t * w );
enumError SyncWBFS	( WBFS_t * w );
enumError ReloadWBFS	( WBFS_t * w );

enumError OpenPartWBFS	( WBFS_t * w, struct PartitionInfo_t *  info );
enumError GetFirstWBFS	( WBFS_t * w, struct PartitionInfo_t ** info );
enumError GetNextWBFS	( WBFS_t * w, struct PartitionInfo_t ** info );

enumError DumpWBFS	( WBFS_t * w, FILE * f, int indent,
			  int dump_level, int view_invalid_discs, CheckWBFS_t * ck );

//-----------------------------------------------------------------------------

void InitializeCheckWBFS ( CheckWBFS_t * ck );
void ResetCheckWBFS	 ( CheckWBFS_t * ck );
enumError CheckWBFS	 ( CheckWBFS_t * ck, WBFS_t * w, int verbose, FILE * f, int indent );
enumError AutoCheckWBFS	 ( WBFS_t * w, bool ignore_check );

enumError PrintCheckedWBFS ( CheckWBFS_t * ck, FILE * f, int indent );

enumError RepairWBFS
	( CheckWBFS_t * ck, int testmode, RepairMode rm, int verbose, FILE * f, int indent );
enumError CheckRepairWBFS
	( WBFS_t * w, int testmode, RepairMode rm, int verbose, FILE * f, int indent );
enumError RepairFBT
	( WBFS_t * w, int testmode, FILE * f, int indent );

// returns true if 'good_ftb' differ from 'cur_ftb'
bool CalcFBT ( CheckWBFS_t * ck );

//-----------------------------------------------------------------------------

void InitializeWDiscInfo     ( WDiscInfo_t * dinfo );
enumError ResetWDiscInfo     ( WDiscInfo_t * dinfo );
enumError GetWDiscInfo	     ( WBFS_t * w, WDiscInfo_t * dinfo, int disc_index );
enumError GetWDiscInfoBySlot ( WBFS_t * w, WDiscInfo_t * dinfo, u32 disc_slot );
enumError FindWDiscInfo	     ( WBFS_t * w, WDiscInfo_t * dinfo, ccp id6 );

enumError LoadIsoHeader	( WBFS_t * w, wd_header_t * iso_header, wbfs_disc_t * disc );

void CalcWDiscInfo ( WDiscInfo_t * winfo, SuperFile_t * sf /* NULL possible */ );

enumError CountPartitions ( SuperFile_t * sf, WDiscInfo_t * dinfo );
enumError LoadPartitionInfo ( SuperFile_t * sf, WDiscInfo_t * dinfo, MemMap_t * mm );

enumError DumpWDiscInfo
	( WDiscInfo_t * dinfo, wd_header_t * iso_header, FILE * f, int indent );

//-----------------------------------------------------------------------------

WDiscList_t * GenerateWDiscList ( WBFS_t * w, int part_index );
void InitializeWDiscList ( WDiscList_t * wlist );
void ResetWDiscList ( WDiscList_t * wlist );
void FreeWDiscList ( WDiscList_t * wlist );

WDiscListItem_t *  AppendWDiscList ( WDiscList_t * wlist, WDiscInfo_t * winfo );
void CopyWDiscInfo ( WDiscListItem_t * item, WDiscInfo_t * winfo );

void ReverseWDiscList	( WDiscList_t * wlist );
void SortWDiscList	( WDiscList_t * wlist, enum SortMode sort_mode,
			  enum SortMode default_sort_mode, int unique );

void PrintSectWDiscListItem ( FILE * out, WDiscListItem_t * witem, ccp def_fname );

//-----------------------------------------------------------------------------

enumError OpenWDiscID6	( WBFS_t * w, ccp id6 );
enumError OpenWDiscIndex( WBFS_t * w, u32 index );
enumError OpenWDiscSlot	( WBFS_t * w, u32 slot, bool force_open );
enumError CloseWDisc	( WBFS_t * w );
enumError ExistsWDisc	( WBFS_t * w, ccp id6 );

wd_header_t * GetWDiscHeader ( WBFS_t * w );

enumError AddWDisc	( WBFS_t * w, SuperFile_t * sf, int partition_selector );
enumError ExtractWDisc	( WBFS_t * w, SuperFile_t * sf );
enumError RemoveWDisc	( WBFS_t * w, ccp id6, bool free_slot_only );
enumError RenameWDisc	( WBFS_t * w, ccp new_id6, ccp new_title,
	bool change_wbfs_head, bool change_iso_head, int verbose, int testmode );

int RenameISOHeader ( void * data, ccp fname,
	ccp new_id6, ccp new_title, int verbose, int testmode );


//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_WBFS_INTERFACE_H