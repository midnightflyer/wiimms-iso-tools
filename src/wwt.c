/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2009-2010 by Dirk Clemens <wiimm@wiimm.de>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "version.h"
#include "wiidisc.h"
#include "lib-sf.h"
#include "titles.h"
#include "wbfs-interface.h"

#include "ui-wwt.c"

//
///////////////////////////////////////////////////////////////////////////////

#define TITLE WWT_SHORT ": " WWT_LONG " v" VERSION " r" REVISION \
		" " SYSTEM " - " AUTHOR " - " DATE


time_t opt_set_time	= 0;
int  print_sections	= 0;
int  long_count		= 0;
int  testmode		= 0;
ccp  opt_dest		= 0;
bool opt_mkdir		= false;
u64  opt_size		= 0;
u32  opt_hss		= 0;
u32  opt_wss		= 0;
int  opt_limit		= -1;

enumIOMode io_mode	= 0;

//
///////////////////////////////////////////////////////////////////////////////

void help_exit( bool xmode )
{
    fputs( TITLE "\n", stdout );

    if (xmode)
    {
	int cmd;
	for ( cmd = 0; cmd < CMD__N; cmd++ )
	    PrintHelpCmd(&InfoUI,stdout,0,cmd);
    }
    else
	PrintHelpCmd(&InfoUI,stdout,0,0);

    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void version_exit()
{
    if (print_sections)
	fputs("[version]\n",stdout);

    if ( print_sections || long_count )
	fputs(	"prog=" WWT_SHORT "\n"
		"name=\"" WWT_LONG "\"\n"
		"version=" VERSION "\n"
		"revision=" REVISION  "\n"
		"system=" SYSTEM "\n"
		"author=\"" AUTHOR "\"\n"
		"date=" DATE "\n"
		, stdout );
    else
	fputs( TITLE "\n", stdout );
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void print_title ( FILE * f )
{
    static bool done = false;
    if (!done)
    {
	done = true;
	if ( verbose >= 1 && f == stdout )
	    fprintf(f,"\n%s\n\n",TITLE);
	else
	    fprintf(f,"*****  %s  *****\n",TITLE);
    }
}

///////////////////////////////////////////////////////////////////////////////

void hint_exit ( enumError stat )
{
    fprintf(stderr,
	    "-> Type '%s -h', '%s help' or '%s help command' for more help.\n\n",
	    progname, progname, progname );
    exit(stat);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     commands                    ///////////////
///////////////////////////////////////////////////////////////////////////////

// common commands of 'wwt' and 'wit'
#define IS_WWT 1
#include "wwt+wit-cmd.c"

///////////////////////////////////////////////////////////////////////////////

enumError cmd_test()
{
 #if 1 || !defined(TEST) // test options

    return cmd_test_options();

 #elif 1

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	printf("arg = |%s|\n",param->arg);
	time_t tim = ScanTime(param->arg);
	if ( tim == (time_t)-1 )
	    printf(" -> ERROR\n");
	else
	{
	    struct tm * tm = localtime(&tim);
	    char timbuf[40];
	    strftime(timbuf,sizeof(timbuf),"%F %T",tm);
	    printf(" -> %s\n",timbuf);
	}
    }
    return ERR_OK;
    
 #elif 0 && defined __CYGWIN__ // test AllocNormalizedFilenameCygwin

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	char * arg = AllocNormalizedFilenameCygwin(param->arg);
	printf("> %s\n",arg);
	FreeString(arg);
    }
    return ERR_OK;

 #elif 1 // test SubstString

    SubstString_t tab[] =
    {
	{ '1', 0,   0, "1234567890" },
	{ 'a', 'A', 0, "AbCdEfGhIjKlMn" },
	{ 'x', 'X', 0, "xyz" },
	{0,0,0,0}
    };

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	char buf[1000];
	int count;
	SubstString(buf,sizeof(buf),tab,param->arg,&count);
	printf("%s -> %s [%d]\n",param->arg,buf,count);
    }
    return ERR_OK;

 #else // test INT

    int i, max = 5;
    for ( i=1; i <= max; i++ )
    {
	fprintf(stderr,"sleep 20 sec (%d/%d)\n",i,max);
	sleep(20);
    }
    return ERR_OK;

 #endif
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_find()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    if ( verbose < 0 )
	return AnalyzePartitions(0,true,true);
    
    const enumError err = AnalyzePartitions(stderr,true,true);
    if ( err || verbose < 0 )
    {
	// if quiet: only status will be given, no print out
	return err;
    }

    PartitionInfo_t * info;
    const bool print_header = !(used_options&OB_NO_HEADER);
    switch(long_count)
    {
	case 0:
	    for ( info = first_partition_info; info; info = info->next )
		if ( info->part_mode >= PM_WBFS )
		    printf("%s\n",info->path);
	    break;

	case 1:
	    if (print_header)
		printf("\n"
			"type  wbfs d.usage    size  file (sizes in MiB)\n"
			"-----------------------------------------------\n");
	    for ( info = first_partition_info; info; info = info->next )
		 printf("%-5s %s %7lld %7lld  %s\n",
				GetFileModeText(info->filemode,false,"-"),
				info->part_mode >= PM_WBFS ? "WBFS" : " -- ",
				(info->disk_usage+MiB/2)/MiB,
				(info->file_size+MiB/2)/MiB,
				info->path );
	    if (print_header)
		printf("\n");
	    break;

	default: // >= 2x long_count
	    if (print_header)
		printf("\n"
			"type  wbfs    disk usage     file size  full path\n"
			"-------------------------------------------------\n");
	    for ( info = first_partition_info; info; info = info->next )
		printf("%-5s %s %13lld %13lld  %s\n",
				GetFileModeText(info->filemode,false,"-"),
				info->part_mode >= PM_WBFS ? "WBFS" : " -- ",
				info->disk_usage,
				info->file_size,
				info->real_path );
	    if (print_header)
		printf("\n");
	    break;
    }

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_space()
{
    opt_all++; // --auto is implied

    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    const bool print_header = !(used_options&OB_NO_HEADER);
    if (print_header)
	printf("\n   size    used used%%   free   discs    file   (sizes in MiB)\n"
		 "--------------------------------------------------------------\n");

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	printf("%7d %7d %3d%% %7d %4d/%-4d  %s\n",
		wbfs.total_mib,
		wbfs.used_mib,
		100*wbfs.used_mib/(wbfs.total_mib),
		wbfs.free_mib,
		wbfs.used_discs,
		wbfs.total_discs,
		long_count ? info->real_path : info->path );
    }
    if (print_header)
	printf("\n");

    return ResetWBFS(&wbfs);
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_analyze()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    File_t F;
    InitializeFile(&F);

    PartitionInfo_t * info;
    for ( info = first_partition_info; info; info = info->next )
    {
	if ( !info->path || !*info->path )
	    continue;

	const enumError stat = OpenFile(&F,info->path,IOM_IS_WBFS);
	if (stat)
	    continue;

	printf("\nANALYZE %s\n",info->path);
	AWData_t awd;
	AnalyzeWBFS(&awd,&F);
	PrintAnalyzeWBFS(&awd,stdout,0);
    }
    putchar('\n');
    ResetFile(&F,0);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_dump()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    const int invalid = long_count > 4
				? 2
				: used_options & OB_INODE || long_count > 3;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	printf("\nDUMP of %s\n\n",info->path);
	DumpWBFS(&wbfs,stdout,2,long_count,invalid,0);
    }
    return ResetWBFS(&wbfs);
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_id6()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if ( used_options & OB_UNIQUE )
	opt_all++;

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    const int err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    ScanPartitionGames();
    SortWDiscList(&pi_wlist,sort_mode,SORT_TITLE, used_options&OB_UNIQUE ? 2 : 0 );

    int i;
    WDiscListItem_t * witem = pi_wlist.first_disc;
    if (long_count)
	for ( i = pi_wlist.used; i-- > 0; witem++ )
	    printf("%s/%s\n",pi_list[witem->part_index]->path,witem->id6);
    else
	for ( i = pi_wlist.used; i-- > 0; witem++ )
	    printf("%s\n",witem->id6);

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////

int print_list_mixed()
{
    ScanPartitionGames();
    SortWDiscList(&pi_wlist,sort_mode,SORT_TITLE, (used_options&OB_UNIQUE) != 0 );

    WDiscListItem_t * witem;
    char footer[200];
    int footer_len = 0;
    int i, max_name_wd = 9;

    if ( print_sections )
    {
	printf("\n[wbfs]\n");
	printf("total_wbfs=%u\n",pi_count);
	printf("total_discs=%u\n",pi_wlist.used);
	printf("used_mib=%u\n",pi_wlist.total_size_mib);
	printf("free_mib=%u\n",pi_free_mib);
	printf("total_mib=%u\n",pi_wlist.total_size_mib+pi_free_mib);

	char buf[10];
	snprintf(buf,sizeof(buf),"%u",pi_wlist.used-1);
	const int fw = strlen(buf);
	for ( i = 0, witem = pi_wlist.first_disc; i < pi_wlist.used; i++, witem++ )
	{
	    printf("\n[disc-%0*u]\n",fw,i);
	    const int pi = witem->part_index;
	    PrintSectWDiscListItem(stdout,witem,
			pi > 0 && pi <= pi_count ? pi_list[pi]->path : 0 );
	}
	return ERR_OK;
    }

    const bool print_header = !(used_options&OB_NO_HEADER);
    if (print_header)
    {
	if ( long_count > 1 )
	{
	    // find max path width
	    for ( i = 1; i <= pi_count; i++ )
	    {
		const int plen = strlen(pi_list[i]->path);
		if ( max_name_wd < plen )
		    max_name_wd = plen;
	    }

	    printf("\n WI  WBFS file\n%.*s\n",max_name_wd+6,sep_200);
	    for ( i = 1; i <= pi_count; i++ )
		printf("%3d  %s\n",i,pi_list[i]->path);
	    printf("%.*s\n",max_name_wd+6,sep_200);
	}

	// find max name width
	max_name_wd = 0;
	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	{
	    const int plen = strlen( witem->title
					? witem->title : witem->name64 );
	    if ( max_name_wd < plen )
		max_name_wd = plen;
	}

	footer_len = snprintf(footer,sizeof(footer),
		"Total: %u discs, %u MiB ~ %u GiB used, %u MiB ~ %u GiB free.",
		pi_wlist.used,
		pi_wlist.total_size_mib, (pi_wlist.total_size_mib+512)/1024,
		pi_free_mib, (pi_free_mib+512)/1024 );
    }

    if ( long_count > 1 )
    {
	if (print_header)
	{
	    int n1, n2;
	    printf("\nID6     MiB Reg.  WI  %n%d discs (%d GiB)%n\n",
		    &n1, pi_wlist.used, (pi_wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;
	    if ( max_name_wd < footer_len )
		max_name_wd = footer_len;
	    printf("%.*s\n", max_name_wd, sep_200);
	}

	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	    printf("%s %4d %s %3d  %s\n",
		    witem->id6, witem->size_mib, witem->region4,
		    witem->part_index,
		    witem->title ? witem->title : witem->name64 );
    }
    else if (long_count)
    {
	if (print_header)
	{
	    int n1, n2;
	    printf("\nID6     MiB Reg.  %n%d discs (%d GiB)%n\n",
		    &n1, pi_wlist.used, (pi_wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;
	    if ( max_name_wd < footer_len )
		max_name_wd = footer_len;
	    printf("%.*s\n", max_name_wd, sep_200);
	}

	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	    printf("%s %4d %s  %s\n",
		    witem->id6, witem->size_mib, witem->region4,
		    witem->title ? witem->title : witem->name64 );
    }
    else
    {
	if (print_header)
	{
	    int n1, n2;
	    printf("\nID6    %n%d discs (%d GiB)%n\n",
		    &n1, pi_wlist.used, (pi_wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;
	    printf("%.*s\n", max_name_wd, sep_200 );
	}

	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	    printf("%s %s\n", witem->id6, witem->title ? witem->title : witem->name64 );
    }

    if (print_header)
	printf("%.*s\n%s\n\n", max_name_wd, sep_200, footer );

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_list ( int long_level )
{
    if ( long_level > 0 )
    {
	RegisterOption(&InfoUI,OPT_LONG,long_level,false);
	long_count += long_level;
    }

    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if ( used_options & OB_UNIQUE )
	opt_all++;

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,false);
    if (err)
	return err;

    if ( used_options & (OB_MIXED|OB_UNIQUE) )
	return print_list_mixed();

    const bool print_header = !print_sections && !(used_options&OB_NO_HEADER);

    int gt_wbfs = 0, gt_disc = 0, gt_max_disc = 0, gt_used_mib = 0, gt_free_mib = 0;

    PrintTime_t pt;
    int opt_time = opt_print_time;
    if ( long_count > 1 )
	opt_time = EnablePrintTime(opt_time);
    SetupPrintTime(&pt,opt_time);

    WBFS_t wbfs;
    int wbfs_index = 0;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	WDiscList_t * wlist = GenerateWDiscList(&wbfs,0);
	if (wlist)
	{
	    SortWDiscList(wlist,sort_mode,SORT_TITLE,false);
	    WDiscListItem_t * witem;

	    int i, max_name_wd = 0;
	    char footer[200];
	    int footer_len = 0;

	    if (print_header)
	    {
		// find max name width
		max_name_wd = 0;
		for ( i = wlist->used, witem = wlist->first_disc; i-- > 0; witem++ )
		{
		    const int plen = strlen( witem->title
						? witem->title : witem->name64 );
		    if ( max_name_wd < plen )
			max_name_wd = plen;
		}

		footer_len = snprintf(footer,sizeof(footer),
			"Total: %d/%d discs, %d MiB ~ %d GiB used, %d MiB ~ %d GiB free.",
			wlist->used, wbfs.total_discs,
			wlist->total_size_mib, (wlist->total_size_mib+512)/1024,
			wbfs.free_mib, (wbfs.free_mib+512)/1024 );
	    }

	    if (print_sections)
	    {
		printf("\n[wbfs-%u]\n",wbfs_index++);
		printf("file=%s\n",info->path);
		printf("used_discs=%u\n",wlist->used);
		printf("total_discs=%u\n",wbfs.total_discs);
		printf("used_mib=%u\n",wbfs.total_discs);
		printf("free_mib=%u\n",wbfs.free_mib);
		printf("total_mib=%u\n",wbfs.total_mib);

		char buf[10];
		snprintf(buf,sizeof(buf),"%u",wlist->used-1);
		const int fw = strlen(buf);
		for ( i = 0, witem = wlist->first_disc; i < wlist->used; i++, witem++ )
		{
		    printf("\n[disc-%0*u]\n",fw,i);
		    PrintSectWDiscListItem(stdout,witem,info->path);
		}
	    }
	    else if (long_count)
	    {
		if (print_header)
		{
		    int n1, n2;
		    putchar('\n');
		    printf("ID6   %s  MiB Reg. %n%d/%d discs (%d GiB)%n\n",
				pt.head, &n1, wlist->used, wbfs.total_discs,
				(wlist->total_size_mib+512)/1024, &n2 );
		    noTRACE("N1=%d N2=%d max_name_wd=%d\n",n1,n2,max_name_wd);
		    max_name_wd += n1;
		    if ( max_name_wd < n2 )
			max_name_wd = n2;
		    if ( max_name_wd < footer_len )
			max_name_wd = footer_len;
		    noTRACE(",%d\n",max_name_wd);
		    printf("%.*s\n", max_name_wd, sep_200);
		}
		for ( i = wlist->used, witem = wlist->first_disc; i-- > 0; witem++ )
		    printf("%s%s %4d %s %s\n",
				witem->id6, PrintTime(&pt,&witem->fatt),
				witem->size_mib, witem->region4,
				witem->title ? witem->title : witem->name64 );
	    }
	    else
	    {
		if (print_header)
		{
		    int n1, n2;
		    putchar('\n');
		    printf("ID6   %s  %n%d/%d discs (%d GiB)%n\n",
				pt.head, &n1, wlist->used, wbfs.total_discs,
				(wlist->total_size_mib+512)/1024, &n2 );
		    noTRACE("N1=%d N2=%d max_name_wd=%d\n",n1,n2,max_name_wd);
		    max_name_wd += n1;
		    if ( max_name_wd < n2 )
			max_name_wd = n2;
		    if ( max_name_wd < footer_len )
			max_name_wd = footer_len;
		    noTRACE(" -> %d\n",max_name_wd);
		    printf("%.*s\n", max_name_wd, sep_200 );
		}

		for ( i = wlist->used, witem = wlist->first_disc; i-- > 0; witem++ )
		    printf("%s%s  %s\n",witem->id6, PrintTime(&pt,&witem->fatt),
				witem->title ? witem->title : witem->name64 );
	    }

	    if (print_header)
		printf("%.*s\n%s\n\n", max_name_wd, sep_200, footer );

	    gt_wbfs++;
	    gt_disc	+= wlist->used;
	    gt_max_disc	+= wbfs.total_discs;
	    gt_used_mib	+= wlist->total_size_mib;
	    gt_free_mib	+= wbfs.free_mib;
	    FreeWDiscList(wlist);
	}
    }
    ResetWBFS(&wbfs);

    if ( print_header && gt_wbfs > 1 )
	printf("Grand total: %d WBFS, %d/%d discs, %d GiB used, %d GiB free.\n\n",
		gt_wbfs, gt_disc, gt_max_disc,
		(gt_used_mib+512)/1024, (gt_free_mib+512)/1024 );

    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_list_a()
{
    if (!opt_auto)
	ScanPartitions(true);
    return cmd_list(2);
}

//-----------------------------------------------------------------------------

enumError cmd_list_m()
{
    RegisterOption(&InfoUI,OPT_MIXED,1,false);
    opt_all++;
    return cmd_list(2);
}

//-----------------------------------------------------------------------------

enumError cmd_list_u()
{
    RegisterOption(&InfoUI,OPT_UNIQUE,1,false);
    opt_all++;
    return cmd_list(2);
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_format()
{
    if (!(used_options&OB_FORCE))
    {
	testmode++;
	print_title(stderr);
	fprintf(stderr,
		"!! %s: Option --force must be set to formatting a WBFS!\n"
		"!! %s:   => test mode (like --test) enabled!\n\n",
		    progname, progname );

    }
    else if (verbose>=0)
	print_title(stdout);

    if (!n_param)
	return ERROR0(ERR_MISSING_PARAM,"missing parameters => abort.\n");

    int error_count = 0, create_count = 0, format_count = 0;
    const u32 create_size_gib = (opt_size+GiB/2)/GiB;
    const bool opt_recover = 0 != ( used_options & OB_RECOVER );

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	if ( !param->arg || !*param->arg )
	    continue;

	bool recover = opt_recover;
	u32 size_gib = create_size_gib;
	enumFileMode filemode = FM_PLAIN;

	struct stat st;
	if (StatFile(&st,param->arg,0))
	{
	    // no file found
	    recover = false;
	    if (!opt_size)
	    {
		error_count++;
		ERROR0(ERR_SEMANTIC,"Option --size needed to create a new WBFS file: %s",
			param->arg );
		continue;
	    }

	    if (testmode)
	    {
		TRACE("WOULD CREATE FILE %s [%d GiB]\n", param->arg, size_gib );
		printf("WOULD CREATE FILE %s [%d GiB]\n", param->arg, size_gib );
		create_count++;
	    }
	    else
	    {
		TRACE("CREATE FILE %s [%d GiB]\n", param->arg, size_gib );
		if (verbose>=0)
		    printf("CREATE FILE %s [%d GiB]\n", param->arg, size_gib );

		File_t f;
		InitializeFile(&f);
		enumError err = CreateFile(&f,param->arg,IOM_NO_STREAM,1);
		if (err)
		{
		    error_count++;
		    continue;
		}

		if (opt_split)
		    SetupSplitFile(&f,OFT_WBFS,opt_split_size);

		if (SetSizeF(&f,opt_size))
		{
		    ClearFile(&f,true);
		    error_count++;
		    continue;
		}

		ClearFile(&f,false);
		create_count++;
	    }
	}
	else
	{
	    size_gib = (st.st_size+GiB/2)/GiB;
	    filemode = GetFileMode(st.st_mode);
	    if ( filemode == FM_OTHER )
	    {
		ERROR0(ERR_WRONG_FILE_TYPE,
		    "%s: Neither regular file nor block device: %s\n",param->arg);
		continue;
	    }
	}

	ccp filetype = GetFileModeText(filemode,true,"");

	u32 hss = opt_hss, wss = opt_wss;
	if ( recover && ( !hss || !wss ) )
	{
	    if ( testmode || verbose >= 0 )
		printf("ANALYZE %s %s\n", filetype, param->arg );

	    File_t f;
	    InitializeFile(&f);
	    if ( OpenFile(&f,param->arg,IOM_IS_WBFS) == ERR_OK )
	    {
		AWData_t awd;
		AnalyzeWBFS(&awd,&f);
		if (awd.n_record)
		{
		    hss = awd.rec[0].hd_sector_size;
		    wss = awd.rec[0].wbfs_sector_size;
		}
		ResetFile(&f,0);
	    }
	}

	if ( hss < HD_SECTOR_SIZE )
	    hss = HD_SECTOR_SIZE;

	char buf_wss[30];
	if (wss)
	{
	    if ( wss <= hss )
		wss = 2 * hss;
	    if ( wss >= MiB )
		snprintf(buf_wss,sizeof(buf_wss),", wss=%u MiB",wss/MiB);
	    else
		snprintf(buf_wss,sizeof(buf_wss),", wss=%u KiB",wss/KiB);
	}
	else
	    buf_wss[0] = 0;

	if (testmode)
	{
	    TRACE("WOULD FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );
	    printf("WOULD FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );
	    format_count++;
	}
	else
	{
	    TRACE("FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );
	    if (verbose>=0)
		printf("FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );

	    WBFS_t wbfs;
	    InitializeWBFS(&wbfs);

	    wbfs_param_t par;
	    memset(&par,0,sizeof(par));
	    par.hd_sector_size	= hss;
	    par.wbfs_sector_size= wss;
	    par.reset		= 1;
	    par.clear_inodes	= !recover;
	    par.setup_iinfo	= !recover
				&& ( filemode > FM_PLAIN || used_options & OB_INODE );

	    if ( FormatWBFS(&wbfs,param->arg,true,&par,0,0) == ERR_OK )
	    {
		format_count++;
		if (recover)
		    RecoverWBFS(&wbfs,param->arg,false);
	    }
	    else
		error_count++;
	    ResetWBFS(&wbfs);
	}
    }

    if (verbose>=0)
    {
	printf("** ");
	if ( create_count > 0 )
	    printf("%d file%s created, ",
		    create_count, create_count==1 ? "" : "s" );
	printf("%d file%s formatted.\n",
		    format_count, format_count==1 ? "" : "s" );
    }

    return error_count ? ERR_WRITE_FAILED : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_recover()
{
    if (verbose>=0)
    {
	print_title(stdout);
	fputc('\n',stdout);
    }

    AddEnvPartitions();

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    if (long_count)
	verbose = long_count;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    int err_count = 0;

    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	printf("%sRECOVER %s\n\n",testmode ? "WOULD " : "", info->path);
	RecoverWBFS(&wbfs,info->path,testmode);
    }
    ResetWBFS(&wbfs);

    if ( verbose > 0 )
	putchar('\n');

    return err_count ? ERR_WBFS_INVALID : max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_check()
{
    if (verbose>=0)
    {
	print_title(stdout);
	fputc('\n',stdout);
    }

    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto && !repair_mode )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    if (long_count)
	verbose = long_count;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    int err_count = 0;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	if ( verbose >= 0 )
	    printf("%sCHECK %s\n", verbose>0 ? "\n" : "", info->path );

	CheckWBFS_t ck;
	InitializeCheckWBFS(&ck);
	if (   CheckWBFS(&ck,&wbfs,verbose,stdout,1)
	    || repair_mode & REPAIR_FBT && wbfs.wbfs->head->wbfs_version < WBFS_VERSION
	    || repair_mode & REPAIR_INODES && ck.no_iinfo_count
	   )
	{
	    if (ck.err)
		err_count++;
	    if (repair_mode)
	    {
		if ( verbose >= 0 )
		    printf("%sREPAIR %s\n", verbose>0 ? "\n" : "", info->path );
		RepairWBFS(&ck,testmode,repair_mode,verbose,stdout,1);
	    }
	}
	ResetCheckWBFS(&ck);
    }
    ResetWBFS(&wbfs);

    if ( verbose > 0 )
	putchar('\n');

    return err_count ? ERR_WBFS_INVALID : max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_repair()
{
    if (!(used_options&OB_REPAIR))
    {
	used_options |= OB_REPAIR;
	repair_mode = REPAIR_DEFAULT;
    }
    return cmd_check();
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_edit()
{
    if (!(used_options&OB_FORCE))
    {
	testmode++;
	print_title(stderr);
    }
    else if (verbose>=0)
	print_title(stdout);

    if (!n_param)
    {
	printf("\n"
	 "EDIT is a dangerous command. It let you (de-)activate the disc slots and\n"
	 "edit the block assignments. Exactly 1 WBFS must be specified. All parameters\n"
	 "are sub commands. Modifications are only done if option --force is set.\n"
	 "\n"
	 "     **********************************************************\n"
	 "     *****  WARNING: This command can damage your WBFS!!  *****\n"
	 "%s"
	 "     *****  See documentation file 'wwt.txt' for details. *****\n"
	 "     **********************************************************\n"
	 "\n",
	used_options & OB_FORCE ? ""
		: "     *****     INFO: Use --force to leave the test mode.  *****\n" );

	return ERR_OK;
    }

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    if ( wbfs_count != 1 )
	return ERROR0( wbfs_count ? ERR_TO_MUCH_WBFS_FOUND : ERR_NO_WBFS_FOUND,
			"Exact one WBFS mus be selected, not %u\n",wbfs_count);

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    err = GetFirstWBFS(&wbfs,&info);
    if (err)
	return err;
    wbfs_t * w = wbfs.wbfs;

    printf("\n* MODIFY WBFS partition %s:\n"
	   "  > WBFS block size:  %x/hex = %u = %1.1f MiB\n"
	   "  > WBFS block range:   1..%u\n"
	   "  > ISO block range:    0..%u\n"
	   "  > Number of discs:  %3u\n"
	   "  > Number of slots:  %3u\n"
	   "\n",
	   info->path,
	   w->wbfs_sec_sz, w->wbfs_sec_sz,
	   (double)w->wbfs_sec_sz/MiB,
	   w->n_wbfs_sec - 1, w->n_wbfs_sec_per_disc,
	   wbfs.used_discs, w->max_disc );

    enum { DO_RM = -5, DO_ACT, DO_INV, DO_FREE, DO_USED };
    ASSERT( DO_USED < 0 );

    CommandTab_t * ctab = calloc(6+wbfs.used_discs,sizeof(*ctab));
    CommandTab_t * cptr = ctab;

    cptr->id	= DO_RM;
    cptr->name1	= "RM";
    cptr->name2	= "R";
    cptr++;

    cptr->id	= DO_ACT;
    cptr->name1	= "ACT";
    cptr->name2	= "A";
    cptr++;

    cptr->id	= DO_INV;
    cptr->name1	= "INV";
    cptr->name2	= "I";
    cptr++;

    cptr->id	= DO_FREE;
    cptr->name1	= "FREE";
    cptr->name2	= "F";
    cptr++;

    cptr->id	= DO_USED;
    cptr->name1	= "USE";
    cptr->name2	= "U";
    cptr++;

    int i;
    WDiscInfo_t dinfo;
    for ( i = 0; i < wbfs.used_discs; i++ )
	if ( GetWDiscInfo(&wbfs,&dinfo,i) == ERR_OK )
	{
	    cptr->id	= i;
	    cptr->name1	= strdup(dinfo.id6);
	    cptr++;
	}

    wbfs_load_freeblocks(w);

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	char * arg = param->arg;
	char cmd_buf[COMMAND_MAX];
	char *dest = cmd_buf;
	char *end  = cmd_buf + sizeof(cmd_buf) - 1;
	while ( *arg && *arg != '=' && dest < end )
	    *dest++ = *arg++;
	*dest = 0;
	int cstat;
	const CommandTab_t * cptr = ScanCommand(&cstat,cmd_buf,ctab);

	if (cstat)
	{
	    ERROR0(ERR_SYNTAX,"Unknown command: %s\n",cmd_buf);
	    putchar('\n');
	    continue;
	}
	ASSERT(cptr);

	if ( *arg != '=' )
	    continue;

	arg++;

	if ( cptr->id == DO_RM || cptr->id == DO_ACT || cptr->id == DO_INV )
	{
	    int mode;
	    ccp info;
	    switch(cptr->id)
	    {
		case DO_RM:  mode = 0; info = "remove"; break;
		case DO_ACT: mode = 1; info = "activate"; break;
		default:     mode = 2; info = "invalidate"; break;
	    }

	    while (*arg)
	    {
		u32 stat, n1, n2;
		arg = ScanRangeU32(arg,&stat,&n1,&n2,0,w->max_disc-1);
		if ( n1 >= 0 && n1 <= n2 )
		{
		    if ( n1 == n2 )
			printf("  - %s%s disc %u.\n",
				testmode ? "WOULD " : "", info, n1 );
		    else
			printf("  - %s%s discs %u..%u.\n",
				testmode ? "WOULD " : "", info, n1, n2 );
		    if (!testmode)
			while ( n1 <= n2 )
			    w->head->disc_table[n1++] = mode;
		}
		if ( *arg != ',' )
		    break;
		arg++;
	    }
	}
	else if ( cptr->id == DO_FREE || cptr->id == DO_USED )
	{
	    const bool is_free = cptr->id == DO_FREE;
	    while (*arg)
	    {
		u32 stat, n1, n2;
		arg = ScanRangeU32(arg,&stat,&n1,&n2,1,w->n_wbfs_sec-1);
		if ( n1 > 0 && n1 <= n2 )
		{
		    if ( n1 == n2 )
			printf("  - %smark block %u as %s.\n",
				testmode ? "WOULD " : "",
				n1, is_free ? "FREE" : "USED" );
		    else
			printf("  - %smark blocks %u..%u as %s.\n",
				testmode ? "WOULD " : "",
				n1, n2, is_free ? "FREE" : "USED" );
		    if (!testmode)
		    {
			if (is_free)
			    while ( n1 <= n2 )
				wbfs_free_block(w,n1++);
			else
			    while ( n1 <= n2 )
				wbfs_use_block(w,n1++);
		    }
		}
		if ( *arg != ',' )
		    break;
		arg++;
	    }
	}
	else
	{
	  wbfs_disc_t * disc = wbfs_open_disc_by_id6(w,(u8*)cptr->name1);
	  if (disc)
	  {
	    u16 * wlba_tab = disc->header->wlba_table;
	    while (*arg)
	    {
		u32 stat, n1, n2;
		arg = ScanRangeU32(arg,&stat,&n1,&n2,0,w->n_wbfs_sec_per_disc-1);
		if ( *arg != ':' )
		    break;
		arg++;

		if ( n1 > 0 && n1 <= n2 )
		{
		    u32 val;
		    arg = ScanNumU32(arg,&stat,&val,0,~(u16)0);
		    u32 add = val > 0, count = n2 - n1;

		    if ( n1 == n2 )
			printf("  - Disc %s: %sset block %u to WBFS block %u.\n",
				    cptr->name1, testmode ? "WOULD " : "",
				    n1, val );
		    else
			printf("  - Disc %s: %sset blocks %u..%u to WBFS blocks %u..%u.\n",
				cptr->name1, testmode ? "WOULD " : "",
				n1, n2, val, (u16)(val+count*add) );
		    if (!testmode)
			while ( n1 <= n2 )
			{
			    wlba_tab[n1] = htons(val);
			    n1++;
			    val += add;
			}
		}
		if ( *arg != ',' )
		    break;
		arg++;
	    }
	    wbfs_sync_disc_header(disc);
	    wbfs_close_disc(disc);
	  }
	}
	putchar('\n');
    }
    wbfs_sync(w);

    CheckWBFS_t ck;
    InitializeCheckWBFS(&ck);
    if (CheckWBFS(&ck,&wbfs,0,stdout,0))
	putchar('\n');
    ResetCheckWBFS(&ck);
    ResetWBFS(&wbfs);

    if (!(used_options&OB_FORCE))
	printf("* Use option --force to leave the test mode.\n\n" );

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_phantom()
{
    if (verbose>=0)
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    int total_add_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;
    const u32 max_mib = (u64)WII_MAX_SECTORS * WII_SECTOR_SIZE / MiB;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_t * w = wbfs.wbfs;
	ASSERT(w);

	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	// scan WBFS for already existing phantom ids

	char phantom_used[1000];
	memset(phantom_used,0,sizeof(phantom_used));
	int slot;
	for ( slot = 0; slot < w->max_disc; slot++ )
	{
	    wbfs_disc_t * d = wbfs_open_disc_by_slot(w,slot,0);
	    if (d)
	    {
		u8 * id6 = d->header->disc_header_copy;
		if ( !memcmp(id6,"PHT",3) )
		{
		    const u32 idx = (id6[3]-'0') * 100 + (id6[4]-'0') * 10 + (id6[5]-'0');
		    if ( idx < sizeof(phantom_used) )
			phantom_used[idx]++;
		}
		wbfs_close_disc(d);
	    }
	}

	int wbfs_add_count = 0;
	bool abort = false;
	ParamList_t * param;
	for ( param = first_param;
	      param && !SIGINT_level && !abort;
	      param = param->next )
	{
	    char *arg = param->arg;
	    if ( !arg || !*arg )
		continue;

	    u32 stat, n1 = 1, n2 = 1, s1, s2;

	    arg = ScanRangeU32(arg,&stat,&s1,&s2,1,10000);
	    if ( stat && *arg == 'x' )
	    {
		arg++;
		n1 = s1;
		n2 = s2;
		arg = ScanRangeU32(arg,&stat,&s1,&s2,0,max_mib);
	    }

	    if ( *arg == 'm' || *arg == 'M' )
		arg++;
	    else
	    {
		if ( *arg == 'g' || *arg == 'G' )
		    arg++;
		s1 *= 1024;
		s2 *= 1024;
		if ( s2 > max_mib )
		    s2 = max_mib;
	    }

	    if ( !stat || *arg || n1 > n2 || s1 > s2 )
	    {
		printf(" - PHANTOM ARGUMENT IGNORED: %s\n",param->arg);
		continue;
	    }

	    if ( testmode || verbose >= 0 )
		printf(" - %sGENERATE %d..%d PHANTOMS with %d..%d MiB\n",
			testmode ? "WOULD " : "", n1, n2, s1, s2 );

	    u32 n = n1 + Random32(n2-n1+1);
	    while ( !abort && n-- > 0 )
	    {
		u32 size = s1 + Random32(s2-s1+1);
		u32 n_sect = (u64)size * MiB /WII_SECTOR_SIZE;

		int idx;
		for ( idx = 0; idx < sizeof(phantom_used); idx++ )
		    if (!phantom_used[idx])
			break;
		if ( idx >= sizeof(phantom_used) )
		{
		    n = 0;
		    continue;
		}
		phantom_used[idx]++;

		char id6[7];
		snprintf(id6,sizeof(id6),"PHT%03u",idx);
		if ( testmode || verbose >= 1 )
		    printf("   - %sGENERATE PHANTOM [%s] with %4u MiB = %u Wii sectors.\n",
			testmode ? "WOULD " : "", id6, size, n_sect );
		if (!testmode)
		{
		    if (wbfs_add_phantom(w,id6,n_sect))
			abort = true;
		    else
			wbfs_add_count++;
		}
	    }
	}

	if ( wbfs_add_count )
	{
	    wbfs_mod_count++;
	    total_add_count += wbfs_add_count;

	    if (verbose>=0)
		printf("* WBFS #%d: %d phantom%s added.\n",
		    wbfs_count, wbfs_add_count, wbfs_add_count==1 ? "" : "s" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	{
	    printf("** %d phantom%s added, %d of %d WBFS used.\n",
			total_add_count, total_add_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );
	}
	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_truncate()
{
    if (verbose>=0)
	print_title(stdout);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    if (!n_param)
	return ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    int wbfs_count = 0, wbfs_mod_count = 0;
    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;

    SuperFile_t sf;
    InitializeSF(&sf);

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	if (testmode)
	{
	    TRACE("WOULD truncate WBFS to minimal size: %s\n",info->path);
	    printf(" - WOULD truncate WBFS to minimal size: %s\n",info->path);
	    wbfs_mod_count++;
	}
	else if ( TruncateWBFS(&wbfs) == ERR_OK )
	{
	    if (verbose>=0)
		printf(" - WBFS truncated to minimal size: %s\n",info->path);
	    wbfs_mod_count++;
	}
	ResetWBFS(&wbfs);
    }
    ResetSF(&sf,0);

    if ( verbose >= 0 && wbfs_count > 1 )
	printf("** %d of %d WBFS truncated.\n",wbfs_mod_count,wbfs_count);

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

static ID_DB_t sync_list;

enumError exec_scan_id ( SuperFile_t * sf, Iterator_t * it )
{
    if (sf->f.id6[0])
	InsertID(&sync_list,sf->f.id6,0);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError exec_add ( SuperFile_t * sf, Iterator_t * it )
{
    ASSERT(sf);
    ASSERT(it);
    ASSERT(it->wbfs);

    if (SIGINT_level)
	return ERR_INTERRUPT;

    enumFileType ft_test = FT_A_ISO|FT_A_SEEKABLE;
    if ( !(sf->f.ftype&FT_A_WDF) && !sf->f.seek_allowed )
	ft_test = FT_A_ISO;

    if ( IsExcluded(sf->f.id6) || PrintErrorFT(&sf->f,ft_test) )
	return ERR_OK;

    bool update    = it->update;
    bool overwrite = it->overwrite;
    int  exists    = -1; // -1=unknown, 0=nonexist, 1=exist

    if (   it->newer
	&& sf->f.fatt.mtime
	&& OpenWDiscID6(it->wbfs,sf->f.id6) == ERR_OK )
    {
	exists = 1;
	ASSERT(it->wbfs->disc);
	wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(it->wbfs->disc,1);
	ASSERT(iinfo);
	const time_t mtime = ntoh64(iinfo->mtime);
	if (mtime)
	{
	    overwrite = sf->f.fatt.mtime > mtime;
	    update = !overwrite;
	}
	CloseWDisc(it->wbfs);
    }
    
    if ( exists>0 || exists < 0 && !ExistsWDisc(it->wbfs,sf->f.id6))
    {
	if (update)
	    return ERR_OK;

	if (overwrite)
	{
	    ccp title = GetTitle(sf->f.id6,"");
	    if (testmode)
	    {
		TRACE("WOULD REMOVE [%s] %s\n",sf->f.id6,title);
		printf(" - WOULD REMOVE [%s] %s\n",sf->f.id6,title);
	    }
	    else
	    {
		TRACE("REMOVE [%s] %s\n",sf->f.id6,title);
		if (verbose>=0)
		    printf(" - REMOVE [%s] %s\n",sf->f.id6,title);
		if (RemoveWDisc(it->wbfs,sf->f.id6,0))
		    return ERR_WBFS;
	    }
	}
	else
	{
	    printf(" - DISC %s [%s] already exists -> ignore\n",
		    sf->f.fname, sf->f.id6 );
	    if ( max_error < ERR_JOB_IGNORED )
		max_error = ERR_JOB_IGNORED;
	    return ERR_OK;
	}
    }

    snprintf(iobuf,sizeof(iobuf),"%d",it->source_list.used);
    if (testmode)
    {
	TRACE("WOULD ADD [%s] %s\n",sf->f.id6,sf->f.fname);
	printf(" - WOULD ADD %*u/%u [%s] %s:%s\n",
		(int)strlen(iobuf), it->source_index+1, it->source_list.used,
		sf->f.id6, oft_name[sf->iod.oft], sf->f.fname );
    }
    else
    {
	TRACE("ADD [%s] %s\n",sf->f.id6,sf->f.fname);
	if ( verbose >= 0 || progress > 0 )
	    printf(" - ADD %*u/%u [%s] %s:%s\n",
			(int)strlen(iobuf), it->source_index+1, it->source_list.used,
			sf->f.id6, oft_name[sf->iod.oft], sf->f.fname );
	fflush(stdout);

	sf->indent		= 5;
	sf->show_progress	= verbose > 1 || progress > 0;
	sf->show_summary	= verbose > 0 || progress > 0;
	sf->show_msec		= verbose > 2;
	sf->f.read_behind_eof	= verbose > 1 ? 1 : 2;

	if ( AddWDisc(it->wbfs,sf,partition_selector) > ERR_WARNING )
	    return ERROR0(ERR_WBFS,"Error while creating disc [%s] @%s\n",
			sf->f.id6, it->wbfs->sf->f.fname );
    }
    it->done_count++;
    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_add()
{
    if (verbose>=0)
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    // fill the source_list
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    if ( !source_list.used && !recurse_list.used )
	return ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    if ( used_options & OB_SYNC )
	used_options |= OB_UPDATE;

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_non_exist	= it.act_non_iso;
    it.act_open		= it.act_non_iso;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.update		= used_options & OB_UPDATE	? 1 : 0;
    it.newer		= used_options & OB_NEWER	? 1 : 0;
    it.overwrite	= used_options & OB_OVERWRITE	? 1 : 0;
    it.remove_source	= used_options & OB_REMOVE	? 1 : 0;

    err = SourceIterator(&it,false,true);
    if (err)
    {
	ResetIterator(&it);
	return err;
    }

    if ( used_options & OB_SYNC )
    {
	it.func = exec_scan_id;
	err = SourceIteratorCollected(&it);
	if (err)
	{
	    ResetIterator(&it);
	    return err;
	}
    }
    it.func = exec_add;

    uint copy_count = 0, rm_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_rm_count = 0;
	if ( used_options & OB_SYNC )
	{
	    disable_exclude_db++;
	    WDiscList_t * wlist = GenerateWDiscList(&wbfs,0);
	    disable_exclude_db--;

	    WDiscListItem_t * ptr = wlist->first_disc;
	    WDiscListItem_t * end = ptr + wlist->used;
	    for ( ; ptr < end; ptr++ )
	    {
		TDBfind_t stat;
		FindID(&sync_list,ptr->id6,&stat,0);
		if ( stat != IDB_ID_FOUND || IsExcluded(ptr->id6) )
		{
		    if (testmode)
		    {
			printf(" - WOULD REMOVE [%s] %s\n",
				ptr->id6, GetTitle(ptr->id6,ptr->name64) );
			wbfs_rm_count++;
		    }
		    else
		    {
			printf(" - REMOVE [%s] %s\n",
				ptr->id6, GetTitle(ptr->id6,ptr->name64) );
			if (!RemoveWDisc(&wbfs,ptr->id6,false))
			    wbfs_rm_count++;
		    }
		}
	    }
	    FreeWDiscList(wlist);
	}

	it.done_count = 0;
	it.wbfs = &wbfs;
	it.open_dev = wbfs.sf->f.st.st_dev;
	it.open_ino = wbfs.sf->f.st.st_ino;
	err = SourceIteratorCollected(&it);
	if (err)
	    break;

	if ( it.done_count || wbfs_rm_count )
	{
	    wbfs_mod_count++;
	    copy_count += it.done_count;
	    rm_count += wbfs_rm_count;
	    if (verbose>=0)
	    {
		*iobuf = 0;
		if ( wbfs_rm_count )
		    snprintf(iobuf,sizeof(iobuf)," %d disc%s removed,",
			wbfs_rm_count, wbfs_rm_count == 1 ? "" : "s" );
		printf("* WBFS #%d:%s %d disc%s added.\n",
		    wbfs_count, iobuf,
		    it.done_count, it.done_count==1 ? "" : "s" );
	    }
	}

	if ( used_options & OB_TRUNC )
	{
	    if (testmode)
	    {
		TRACE("WOULD truncate WBFS #%d to minimal size.\n",wbfs_count);
		if (!it.done_count)
		    wbfs_mod_count++;
	    }
	    else if ( TruncateWBFS(&wbfs) == ERR_OK )
	    {
		if (verbose>=0)
		    printf("* WBFS #%d truncated to minimal size.\n",wbfs_count);
		if (!it.done_count)
		    wbfs_mod_count++;
	    }
	}
    }
    ResetIterator(&it);
    ResetWBFS(&wbfs);
    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_update()
{
    used_options |= OB_UPDATE;
    return cmd_add();
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_sync()
{
    used_options |= OB_SYNC;
    return cmd_add();
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_extract()
{
    if ( verbose >= 0 && testmode < 2 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    CheckParamID6( ( used_options & OB_UNIQUE ) != 0, testmode > 1 );
    if ( testmode > 1 )
	return PrintParamID6();

    if (!id6_param_found)
	return used_options & OB_IGNORE
		? ERR_OK 
		: ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    const int update = used_options & OB_UPDATE;
    if (update)
	DefineExcludePath( opt_dest && *opt_dest ? opt_dest : ".", 1 );

    //----- extract discs

    int extract_count = 0, rm_count = 0;
    int wbfs_count = 0, wbfs_used_count = 0, wbfs_mod_count = 0;
    const int overwrite = update ? -1 : used_options & OB_OVERWRITE ? 1 : 0;
    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;

    SuperFile_t fo;
    InitializeSF(&fo);

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_extract_count = 0, wbfs_rm_count = 0;
	ParamList_t * param;
	for ( param = first_param;
	      param && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if ( !*id6 || IsExcluded(id6) )
	    {
		param->count++;
		continue;
	    }

	    OpenWDiscID6(&wbfs,id6);
	    wd_header_t * dhead = GetWDiscHeader(&wbfs);
	    if (dhead)
	    {
		if (param->count)
		{
		    TRACE("ALREADY DONE %s @ %s\n",id6,info->path);
		    if ( verbose > 0 )
			printf(" - ALREADY EXTRACTED: [%s]\n",id6);
		    continue;
		}

		RemoveSF(&fo);
		enumOFT oft = CalcOFT(output_file_type,param->arg,0,OFT__DEFAULT);
		ccp title = GetTitle(id6,(ccp)dhead->game_title);
		ccp oname = param->arg || oft != OFT_WBFS ? 0 : id6;

		// calculate the default filename first ...
		GenFileName( &fo.f, 0, oname, title, id6, oft_ext[oft] );

		// and now the destination filename
		SubstString_t subst_tab[] =
		{
		    { 'i', 'I', 0, id6 },
		    { 'n', 'N', 0, (ccp)dhead->game_title },
		    { 't', 'T', 0, title },
		    { 'e', 'E', 0, oft_ext[oft]+1 },
		    { '+', '+', 1, fo.f.fname },
		    {0,0,0,0}
		};
		char dbuf[PATH_MAX], fbuf[PATH_MAX];
		int conv_count1, conv_count2;
		SubstString(dbuf,sizeof(dbuf),subst_tab,opt_dest,&conv_count1);

		char oname_buf[20];
		if ( param->arg )
		    oname = param->arg;
		else if ( oft == OFT_WBFS )
		{
		    snprintf(oname_buf,sizeof(oname_buf),"%s.wbfs",id6);
		    oname = oname_buf;
		}
		else
		    oname = fo.f.fname;

		SubstString(fbuf,sizeof(fbuf),subst_tab,oname,&conv_count2);
		fo.f.create_directory = conv_count1 || conv_count2 || opt_mkdir;
		GenFileName( &fo.f, dbuf, fbuf, 0, 0, 0 );

		if (testmode)
		{
		    if ( overwrite > 0 || stat(fo.f.fname,&fo.f.st) )
		    {
			printf(" - WOULD EXTRACT %s -> %s:%s\n",
				id6, oft_name[oft], fo.f.fname );
			wbfs_extract_count++;
		    }
		    else if (!update)
			printf(" - WOULD NOT OVERWRITE %s -> %s\n",id6,fo.f.fname);
		    param->count++;
		}
		else
		{
		    if ( verbose >= 0 || progress > 0 )
			printf(" - EXTRACT %s -> %s:%s\n",
				id6, oft_name[oft], fo.f.fname );
		    fflush(stdout);

		    if (CreateFile( &fo.f, 0, IOM_IS_IMAGE, overwrite ))
			goto abort;

		    if (opt_split)
			SetupSplitFile(&fo.f,oft,opt_split_size);

		    if (SetupWriteSF(&fo,oft))
		    {
			RemoveSF(&fo);
			goto abort;
		    }

		    fo.enable_fast	= (used_options&OB_FAST)  != 0;
		    fo.enable_trunc	= (used_options&OB_TRUNC) != 0;
		    fo.indent		= 5;
		    fo.show_progress	= verbose > 1 || progress > 0;
		    fo.show_summary	= verbose > 0 || progress > 0;
		    fo.show_msec	= verbose > 2;

		    FileAttrib_t fatt, *set_time_ref = 0;
		    const time_t mtime = ntoh64(dhead->iinfo.mtime);
		    if (mtime)
		    {
			memset(&fatt,0,sizeof(fatt));
			fatt.mtime = fatt.atime = mtime;
			set_time_ref = &fatt;
		    }

		    if (ExtractWDisc(&wbfs,&fo))
		    {
			RemoveSF(&fo);
			ERROR0(ERR_WBFS,"Can't extract disc [%s] @%s\n",id6,info->path);
		    }
		    else if (!ResetSF(&fo,set_time_ref))
		    {
			wbfs_extract_count++;
			param->count++;
		    }
		}
	    }
	  abort:
	    CloseWDisc(&wbfs);
	}

	if (used_options&OB_REMOVE)
	{
	    for ( param = first_param; param; param = param->next )
		if ( param->id6[0] && param->count )
		{
		    TRACE("REMOVE [%s] count=%d\n",param->id6,param->count);
		    if ( RemoveWDisc(&wbfs,param->id6,0) == ERR_OK )
			wbfs_rm_count++;
		}
	    TRACE("%d disc(s) removed\n",wbfs_rm_count);
	}

	if ( wbfs_extract_count || wbfs_rm_count )
	{
	    if (wbfs_extract_count)
	    {
		wbfs_used_count++;
		extract_count += wbfs_extract_count;
	    }
	    if (wbfs_rm_count)
	    {
		rm_count += wbfs_rm_count;
		wbfs_mod_count++;
	    }

	    if (verbose>=0)
	    {
		const int bufsize = 100;
		char buf[bufsize+1];

		if (wbfs_rm_count)
		    snprintf(buf,bufsize,", %d disc%s removed",
				wbfs_rm_count, wbfs_rm_count==1 ? "" : "s" );
		else
		    *buf = 0;
		printf("* WBFS #%d: %d disc%s extracted%s.\n",
		    wbfs_count, wbfs_extract_count, wbfs_extract_count==1 ? "" : "s", buf );
	    }
	}
    }
    ResetWBFS(&wbfs);
    ResetSF(&fo,0);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->id6[0] && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->id6);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	{
	    printf("** %d disc%s extracted, %d of %d WBFS used.\n",
			extract_count, extract_count==1 ? "" : "s",
			wbfs_used_count, wbfs_count );
	    if (rm_count)
		printf("** %d disc%s removed, %d of %d WBFS modified.\n",
			rm_count, rm_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );
	}
	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_remove()
{
    if ( verbose >= 0 && testmode < 2 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    CheckParamID6( ( used_options & OB_UNIQUE ) != 0, true );
    if ( testmode > 1 )
	return PrintParamID6();

    if (!id6_param_found)
	return used_options & OB_IGNORE
		? ERR_OK 
		: ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    //----- remove discs

    const bool check_it		= ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check	= ( used_options & OB_FORCE ) != 0;
    const bool free_slot_only	= ( used_options & OB_NO_FREE ) != 0;
    int rm_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_rm_count = 0;
	bool wbfs_go = true;
	ParamList_t * param;
	for ( param = first_param;
	      param && wbfs_go && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if (!*id6)
		continue;
	    fflush(stdout);

	    if (!OpenWDiscID6(&wbfs,id6))
	    {
		char disc_title[WII_TITLE_SIZE+1];
		wd_header_t *dh = GetWDiscHeader(&wbfs);
		if (dh)
		    StringCopyS(disc_title,sizeof(disc_title),(ccp)dh->game_title);
		else
		    *disc_title = 0;
		CloseWDisc(&wbfs);
		ccp title = GetTitle(id6,disc_title);
		param->count++;
		if (testmode || verbose > 0 )
		{
		    TRACE("%s%s %s @ %s\n",
			testmode ? "WOULD " : "",
			free_slot_only ? "DROP" : "REMOVE", id6, info->path );
		    printf(" - %s%s [%s] %s\n",
			testmode ? "WOULD " : "",
			free_slot_only ? "DROP" : "REMOVE",
			id6, title );
		}
		if (testmode)
		    wbfs_rm_count++;
		else if (RemoveWDisc(&wbfs,id6,free_slot_only))
		    wbfs_go = false;
		else
		    wbfs_rm_count++;
	    }
	}

	if (wbfs_rm_count)
	{
	    wbfs_mod_count++;
	    rm_count += wbfs_rm_count;
	    if (verbose>=0)
		printf("* WBFS #%d: %d disc%s %s.\n",
		    wbfs_count, wbfs_rm_count, wbfs_rm_count==1 ? "" : "s",
		    free_slot_only ? "droped" : "removed" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->id6[0] && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->id6);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	    printf("** %d disc%s %s, %d of %d WBFS modified.\n",
			rm_count, rm_count==1 ? "" : "s",
			free_slot_only ? "droped" : "removed",
			wbfs_mod_count, wbfs_count );

	if ( wbfs_mod_count > 0 && free_slot_only )
	    printf("*%s Run CHECK to fix the free blocks table%s!\n",
			wbfs_count > 1 ? "*" : "",
			wbfs_mod_count == 1 ? "" : "s" );

	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_rename ( bool rename_id )
{
    if ( verbose >= 0 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    if (!n_param)
	return ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    err = CheckParamRename(rename_id,!rename_id,true);
    if (err)
	return err;

    //----- rename

    const bool check_it		= 0 == ( used_options & OB_NO_CHECK );
    const bool ignore_check	= 0 != ( used_options & OB_FORCE );
    const bool change_wbfs	= 0 != ( used_options & OB_WBFS );
    const bool change_iso	= 0 != ( used_options & OB_ISO );

    int mv_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_mv_count = 0;
	bool wbfs_go = true;
	ParamList_t * param;
	for ( param = first_param;
	      param && wbfs_go && !SIGINT_level;
	      param = param->next )
	{
	    ccp sel = param->selector;
	    if (!*sel)
		continue;
	    fflush(stdout);

	    if ( *sel == '+' )
	    {
		int idx;
		for ( idx = 0; idx < wbfs.used_discs; idx++ )
		{
		    enumError err = OpenWDiscIndex(&wbfs,idx);
		    if (err)
			return err;
		    RenameWDisc( &wbfs, param->id6, param->arg,
				 change_wbfs, change_iso, verbose, testmode );
		    wbfs_mv_count++;
		}
		param->count++;
		continue;
	    }

	    enumError err;
	    switch (*sel)
	    {
		case '#': // disc slot
		    {
			u32 slot = strtoul(sel+1,0,10);
			err = OpenWDiscSlot(&wbfs,slot,0);
			if ( err == ERR_WDISC_NOT_FOUND )
			    ERROR0(err, "No disc at slot #%u; %s\n",slot,info->path);
		    }
		    break;

		case '$': // disc index
		    {
			u32 idx = strtoul(sel+1,0,10);
			err = OpenWDiscIndex(&wbfs,idx);
			if ( err == ERR_WDISC_NOT_FOUND )
			    ERROR0(err, "No disc at index %u; %s\n",idx,info->path);
		    }
		    break;

		default: // ID6
		    err = OpenWDiscID6(&wbfs,sel);
		    break;
	    }
	    if (!err)
	    {
		RenameWDisc( &wbfs, param->id6, param->arg,
				change_wbfs, change_iso, verbose, testmode );
		param->count++;
		wbfs_mv_count++;
	    }
	}

	if (wbfs_mv_count)
	{
	    wbfs_mod_count++;
	    mv_count += wbfs_mv_count;
	    if (verbose>=0)
		printf("* WBFS #%d: %d disc%s changed.\n",
		    wbfs_count, wbfs_mv_count, wbfs_mv_count==1 ? "" : "s" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->selector && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->selector);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	    printf("** %d disc%s changed, %d of %d WBFS modified.\n",
			mv_count, mv_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );

	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_touch()
{
    if ( verbose >= 0 && testmode < 2 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    CheckParamID6( ( used_options & OB_UNIQUE ) != 0, true );
    if ( testmode > 1 )
	return PrintParamID6();

    if (!id6_param_found)
	return used_options & OB_IGNORE
		? ERR_OK 
		: ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    //----- setup time modes

    int touch_count = 0, wbfs_count = 0, wbfs_mod_count = 0;
    const u64 set_time = !opt_set_time || opt_set_time == (time_t)-1
				? time(0)
				: opt_set_time;

    option_t opt = used_options & OB_GRP_XTIME;
    if (!opt)
	opt = OB_GRP_XTIME;

    const u64 itime = opt & OB_ITIME ? set_time : 0;
    const u64 mtime = opt & OB_MTIME ? set_time : 0;
    const u64 ctime = opt & OB_CTIME ? set_time : 0;
    const u64 atime = opt & OB_ATIME ? set_time : 0;
   
    //----- touch discs

    const bool check_it		= 0 == ( used_options & OB_NO_CHECK );
    const bool ignore_check	= 0 != ( used_options & OB_FORCE );

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_touch_count = 0;
	bool wbfs_go = true;
	ParamList_t * param;
	for ( param = first_param;
	      param && wbfs_go && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if ( !*id6 || IsExcluded(id6) )
		continue;
	    fflush(stdout);

	    if (!OpenWDiscID6(&wbfs,id6))
	    {
		char disc_title[WII_TITLE_SIZE+1];
		wd_header_t *dh = GetWDiscHeader(&wbfs);
		if (dh)
		    StringCopyS(disc_title,sizeof(disc_title),(ccp)dh->game_title);
		else
		    *disc_title = 0;
		ccp title = GetTitle(id6,disc_title);
		param->count++;
		if (testmode || verbose > 0 )
		{
		    TRACE("%sTOUCH %s @ %s\n",
			testmode ? "WOULD " : "", id6, info->path );
		    printf(" - %sTOUCH [%s] %s\n", testmode ? "WOULD " : "", id6, title );
		}
		if (testmode)
		    wbfs_touch_count++;
		else if (wbfs_touch_disc(wbfs.disc,itime,mtime,ctime,atime))
		    wbfs_go = false;
		else
		    wbfs_touch_count++;
		CloseWDisc(&wbfs);
	    }
	}

	if (wbfs_touch_count)
	{
	    wbfs_mod_count++;
	    touch_count += wbfs_touch_count;
	    if (verbose>=0)
		printf("* WBFS #%d: %d disc%s touched.\n",
		    wbfs_count, wbfs_touch_count, wbfs_touch_count==1 ? "" : "s" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->id6[0] && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->id6);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	    printf("** %d disc%s touched, %d of %d WBFS modified.\n",
			touch_count, touch_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );

	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_verify()
{
    if ( verbose >= 0 && testmode < 2 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    if (!n_param)
	AddParam("+",0);

    CheckParamID6( ( used_options & OB_UNIQUE ) != 0, true );
    if ( testmode > 1 )
	return PrintParamID6();


    //----- count discs

    const bool check_it		= 0 == ( used_options & OB_NO_CHECK );
    const bool ignore_check	= 0 != ( used_options & OB_FORCE );

    int disc_count = 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		info->ignore = true;
		continue;
	    }
	}

	ParamList_t * param;
	for ( param = first_param;
	      param && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if (*id6)
	    {
		param->count++;
		if (!IsExcluded(id6))
		    disc_count++;
	    }
	}
    }


    //----- verify discs

    int disc_index = 0, verify_count = 0, fail_count = 0, wbfs_count = 0;

    const bool remove		= ( used_options & OB_REMOVE )  != 0;
    const bool free_slot_only	= ( used_options & OB_NO_FREE ) != 0;
    ccp fail_verb = !remove ? "found" : free_slot_only ? "dropped" : "removed";
    char fail_buf[100];

    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	int wbfs_verify_count = 0, wbfs_fail_count = 0;

	ParamList_t * param;
	for ( param = first_param;
	      param && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if ( !*id6 || IsExcluded(id6) )
		continue;
	    disc_index++;

	    if (!OpenWDiscID6(&wbfs,id6))
	    {
		char disc_title[WII_TITLE_SIZE+1];
		wd_header_t *dh = GetWDiscHeader(&wbfs);
		if (dh)
		    StringCopyS(disc_title,sizeof(disc_title),(ccp)dh->game_title);
		else
		    *disc_title = 0;
		ccp title = GetTitle(id6,disc_title);

		if (testmode )
		    printf(" - WOULD VERIFY [%s] %s\n", id6, title );
		else
		{
		    SuperFile_t *sf = wbfs.sf;
		    ASSERT(sf);
		    Verify_t ver;
		    InitializeVerify(&ver,sf);
		    ver.long_count = long_count;
		    ver.disc_index = disc_index;
		    ver.disc_total = disc_count;
		    ver.fname = title;
		    ver.indent = 2;
		    if ( opt_limit >= 0 )
		    {
			ver.max_err_msg = opt_limit;
			if (!ver.verbose)
			    ver.verbose = 1;
		    }
		    
		    SetupIOD(sf,OFT_WBFS,OFT_WBFS);
		    sf->wbfs = &wbfs;
		    const enumError stat = VerifyDisc(&ver);
		    
		    SetupIOD(sf,OFT_PLAIN,OFT_PLAIN);
		    sf->wbfs = 0;
		    ResetVerify(&ver);
		    
		    if ( stat == ERR_DIFFER )
		    {
			wbfs_fail_count++;
			if (remove)
			{
			    if ( verbose >= 0 )
				printf(" - %s disc [%s] %s\n",
					free_slot_only ? "DROP" : "REMOVE", id6, title );
			    if (RemoveWDisc(&wbfs,id6,free_slot_only))
				fail_verb = "found";
			}
		    }
		}
		wbfs_verify_count++;
		CloseWDisc(&wbfs);
	    }
	}

	if (wbfs_verify_count)
	{
	    verify_count += wbfs_verify_count;
	    if (verbose>=0)
	    {
		if (wbfs_fail_count)
		{
		    fail_count += wbfs_fail_count;
		    snprintf(fail_buf,sizeof(fail_buf),", %d bad disc%s %s",
			    wbfs_fail_count, wbfs_fail_count==1 ? "" : "s", fail_verb );
		}
		else
		    *fail_buf = 0;
		printf("* WBFS #%d: %d disc%s verified%s.\n",
		    wbfs_count, wbfs_verify_count, wbfs_verify_count==1 ? "" : "s",
		     fail_buf);
	    }
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->id6[0] && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->id6);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	{
	    if (fail_count)
		snprintf(fail_buf,sizeof(fail_buf),", %d bad disc%s %s",
			fail_count, fail_count==1 ? "" : "s", fail_verb );
	    else
		*fail_buf = 0;
	    printf("** Total: %d disc%s of %d WBFS verified%s.\n",
			verify_count, verify_count==1 ? "" : "s", wbfs_count,fail_buf);
	}

	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_filetype()
{
    SuperFile_t sf;
    InitializeSF(&sf);

    if (!(used_options&OB_NO_HEADER))
    {
	if ( long_count > 1 )
	    printf("\n"
		"file     disc   size reg split\n"
		"type     ID6     MIB ion   N  %s\n"
		"%s\n",
		long_count > 2 ? "real path" : "filename", sep_79 );
	else if (long_count)
	    printf("\n"
		"file     disc  split\n"
		"type     ID6     N  filename\n"
		"%s\n", sep_79 );
	else
	    printf("\n"
		"file\n"
		"type     filename\n"
		"%s\n", sep_79 );
    }

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	ResetSF(&sf,0);
	sf.f.disable_errors = true;
	OpenSF(&sf,param->arg,true,false);
	AnalyzeFT(&sf.f);
	TRACELINE;

	ccp ftype = GetNameFT( sf.f.ftype, used_options & OB_IGNORE ? 1 : 0 );
	if (ftype)
	{
	    if ( long_count > 1 )
	    {
		char split[10] = " -";
		if ( sf.f.split_used > 1 )
		    snprintf(split,sizeof(split),"%2d",sf.f.split_used);

		ccp region = "-   ";
		char size[10] = "   -";
		if (sf.f.id6[0])
		{
		    region = GetRegionInfo(sf.f.id6[3])->name4;
		    u32 count = CountUsedIsoBlocksSF(&sf,partition_selector);
		    if (count)
			snprintf(size,sizeof(size),"%4u",
				(count+WII_SECTORS_PER_MIB/2)/WII_SECTORS_PER_MIB);
		}

		printf("%-8s %-6s %s %s %s  %s\n",
			ftype, sf.f.id6[0] ? sf.f.id6 : "-",
			size, region, split, sf.f.fname );
	    }
	    else if (long_count)
	    {
		char split[10] = " -";
		if ( sf.f.split_used > 1 )
		    snprintf(split,sizeof(split),"%2d",sf.f.split_used);
		printf("%-8s %-6s %s  %s\n",
			ftype, sf.f.id6[0] ? sf.f.id6 : "-",
			split, sf.f.fname );
	    }
	    else
		printf("%-8s %s\n", ftype, sf.f.fname );
	}
	CloseSF(&sf,0);
    }

    ResetSF(&sf,0);

    if (!(used_options&OB_NO_HEADER))
	putchar('\n');

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   check options                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckOptions ( int argc, char ** argv, bool is_env )
{
    TRACE("CheckOptions(%d,%p,%d) optind=%d\n",argc,argv,is_env,optind);

    optind = 0;
    int err = 0;

    for(;;)
    {
      const int opt_stat = getopt_long(argc,argv,OptionShort,OptionLong,0);
      if ( opt_stat == -1 )
	break;

      RegisterOption(&InfoUI,opt_stat,1,is_env);

      switch ((enumGetOpt)opt_stat)
      {
	case GO__ERR:		err++; break;

	case GO_VERSION:	version_exit();
	case GO_HELP:		help_exit(false);
	case GO_XHELP:		help_exit(true);

	case GO_QUIET:		verbose = verbose > -1 ? -1 : verbose - 1; break;
	case GO_VERBOSE:	verbose = verbose <  0 ?  0 : verbose + 1; break;
	case GO_PROGRESS:	progress++; break;
	case GO_LOGGING:	logging++; break;
	case GO_ESC:		err += ScanEscapeChar(optarg) < 0; break;
	case GO_IO:		ScanIOMode(optarg); break;

	case GO_TITLES:		AtFileHelper(optarg,true,AddTitleFile); break;
	case GO_UTF_8:		use_utf8 = true; break;
	case GO_NO_UTF_8:	use_utf8 = false; break;
	case GO_LANG:		lang_info = optarg; break;

	case GO_TEST:		testmode++; break;

	case GO_ALL:		opt_all++; break;
	case GO_PART:		AtFileHelper(optarg,false,AddPartition); break;
	case GO_RECURSE:	AppendStringField(&recurse_list,optarg,false); break;
	case GO_RAW:		partition_selector = WHOLE_DISC; break;

	case GO_INCLUDE:	AtFileHelper(optarg,0,AddIncludeID); break;
	case GO_INCLUDE_PATH:	AtFileHelper(optarg,0,AddIncludePath); break;
	case GO_EXCLUDE:	AtFileHelper(optarg,0,AddExcludeID); break;
	case GO_EXCLUDE_PATH:	AtFileHelper(optarg,0,AddExcludePath); break;
	case GO_IGNORE:		break;
	case GO_IGNORE_FST:	allow_fst = false; break;

	case GO_INODE:		break;
	case GO_DEST:		opt_dest = optarg; break;
	case GO_DEST2:		opt_dest = optarg; opt_mkdir = true; break;
	case GO_HOOK:		hook_enabled = true; break;
	case GO_ENC:		err += ScanOptEncoding(optarg); break;
	case GO_REGION:		err += ScanOptRegion(optarg); break;
	case GO_IOS:		err += ScanOptIOS(optarg); break;
	case GO_ID:		err += ScanOptId(optarg); break;
	case GO_NAME:		err += ScanOptName(optarg); break;
	case GO_MODIFY:		err += ScanOptModify(optarg); break;
	case GO_SPLIT:		opt_split++; break;
	case GO_SPLIT_SIZE:	err += ScanSplitSize(optarg); break;
	case GO_RECOVER:	break;
	case GO_FORCE:		break;
	case GO_NO_CHECK:	break;
	case GO_NO_FREE:	break;

	case GO_UPDATE:		break;
	case GO_SYNC:		break;
	case GO_NEWER:		break;
	case GO_OVERWRITE:	break;
	case GO_REMOVE:		break;
	case GO_TRUNC:		break;
	case GO_FAST:		break;

	case GO_WDF:		output_file_type = OFT_WDF; break;
	case GO_ISO:		output_file_type = OFT_PLAIN; break;
	case GO_CISO:		output_file_type = OFT_CISO; break;
	case GO_WBFS:		output_file_type = OFT_WBFS; break;

	case GO_ITIME:	    	SetTimeOpt(PT_USE_ITIME|PT_F_ITIME); break;
	case GO_MTIME:	    	SetTimeOpt(PT_USE_MTIME|PT_F_MTIME); break;
	case GO_CTIME:	    	SetTimeOpt(PT_USE_CTIME|PT_F_CTIME); break;
	case GO_ATIME:	    	SetTimeOpt(PT_USE_ATIME|PT_F_ATIME); break;

	case GO_LONG:		long_count++; break;
	case GO_MIXED:	    	break;
	case GO_UNIQUE:	    	break;
	case GO_NO_HEADER:	break;
	case GO_SECTIONS:	print_sections++; break;

	case GO_AUTO:
	    if (!opt_auto)
		ScanPartitions(false);
	    break;

	case GO_RDEPTH:
	    if (ScanSizeOptU32(&opt_recurse_depth,optarg,1,0,
				"rdepth",0,MAX_RECURSE_DEPTH,0,0,true))
		hint_exit(ERR_SYNTAX);
	    break;

	case GO_PSEL:
	    {

		const int new_psel = ScanPartitionSelector(optarg);
		if ( new_psel == -1 )
		    err++;
		else
		    partition_selector = new_psel;
	    }
	    break;

	case GO_SIZE:
	    if (ScanSizeOptU64(&opt_size,optarg,GiB,0,
				"size",MIN_WBFS_SIZE,0,0,0,true))
		err++;
	    break;

	case GO_HSS:
	    if (ScanSizeOptU32(&opt_hss,optarg,1,0,
				"hss",512,WII_SECTOR_SIZE,0,1,true))
		err++;
	    break;

	case GO_WSS:
	    if (ScanSizeOptU32(&opt_wss,optarg,1,0,
				"wss",1024,0,0,1,true))
		err++;
	    break;

	case GO_REPAIR:
	    {
		const int new_repair = ScanRepairMode(optarg);
		if ( new_repair == -1 )
		    err++;
		else
		    repair_mode = new_repair;
	    }
	    break;

	case GO_TIME:
	    if ( ScanAndSetPrintTimeMode(optarg) == PT__ERROR )
		err++;
	    break;

	case GO_SET_TIME:
	    {
		const time_t tim = ScanTime(optarg);
		if ( tim == (time_t)-1 )
		    err++;
		else
		    opt_set_time = tim;
	    }
	    break;

	case GO_SORT:
	    {
		const SortMode new_mode = ScanSortMode(optarg);
		if ( new_mode == SORT__ERROR )
		    err++;
		else
		{
		    TRACE("SORT-MODE set: %d -> %d\n",sort_mode,new_mode);
		    sort_mode = new_mode;
		}
	    }
	    break;

	case GO_LIMIT:
	    {
		u32 limit;
		if (ScanSizeOptU32(&limit,optarg,1,0,"limit",0,INT_MAX,0,0,true))
		    err++;
		else
		    opt_limit = limit;
	    }
	    break;

	// no default case defined
	//	=> compiler checks the existence of all enum values
      }
    }
 #ifdef DEBUG
    DumpUsedOptions(&InfoUI,TRACE_FILE,11);
 #endif

    if (is_env)
    {
	env_options = used_options;
    }
    else if ( verbose > 3 )
    {
	print_title(stdout);
	printf("PROGRAM_NAME   = %s\n",progname);
	if (lang_info)
	    printf("LANG_INFO      = %s\n",lang_info);
	ccp * sp;
	for ( sp = search_path; *sp; sp++ )
	    printf("SEARCH_PATH[%td] = %s\n",sp-search_path,*sp);
	printf("\n");
    }

    return !err ? ERR_OK : max_error ? max_error : ERR_SYNTAX;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   check command                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckCommand ( int argc, char ** argv )
{
    TRACE("CheckCommand(%d,) optind=%d\n",argc,optind);

    if ( optind >= argc )
    {
	ERROR0(ERR_SYNTAX,"Missing command.\n");
	hint_exit(ERR_SYNTAX);
    }

    int cmd_stat;
    const CommandTab_t * cmd_ct = ScanCommand(&cmd_stat,argv[optind],CommandTab);
    if (!cmd_ct)
    {
	if ( cmd_stat > 0 )
	    ERROR0(ERR_SYNTAX,"Command abbreviation is ambiguous: %s\n",argv[optind]);
	else
	    ERROR0(ERR_SYNTAX,"Unknown command: %s\n",argv[optind]);
	hint_exit(ERR_SYNTAX);
    }

    TRACE("COMMAND FOUND: #%d = %s\n",cmd_ct->id,cmd_ct->name1);

    enumError err = VerifySpecificOptions(&InfoUI,cmd_ct);
    if (err)
	hint_exit(err);

    argc -= optind+1;
    argv += optind+1;

    while ( argc-- > 0 )
	AtFileHelper(*argv++,false,AddParam);

    switch ((enumCommands)cmd_ct->id)
    {
	case CMD_VERSION:	version_exit();
	case CMD_HELP:		PrintHelp(&InfoUI,stdout,0); break;
	case CMD_TEST:		err = cmd_test(); break;
	case CMD_ERROR:		err = cmd_error(); break;
	case CMD_EXCLUDE:	err = cmd_exclude(); break;
	case CMD_TITLES:	err = cmd_titles(); break;

	case CMD_FIND:		err = cmd_find(); break;
	case CMD_SPACE:		err = cmd_space(); break;
	case CMD_ANALYZE:	err = cmd_analyze(); break;
	case CMD_DUMP:		err = cmd_dump(); break;

	case CMD_ID6:		err = cmd_id6(); break;
	case CMD_LIST:		err = cmd_list(0); break;
	case CMD_LIST_L:	err = cmd_list(1); break;
	case CMD_LIST_LL:	err = cmd_list(2); break;
	case CMD_LIST_A:	err = cmd_list_a(); break;
	case CMD_LIST_M:	err = cmd_list_m(); break;
	case CMD_LIST_U:	err = cmd_list_u(); break;

	case CMD_FORMAT:	err = cmd_format(); break;
	case CMD_RECOVER:	err = cmd_recover(); break;
	case CMD_CHECK:		err = cmd_check(); break;
	case CMD_REPAIR:	err = cmd_repair(); break;
	case CMD_EDIT:		err = cmd_edit(); break;
	case CMD_PHANTOM:	err = cmd_phantom(); break;
	case CMD_TRUNCATE:	err = cmd_truncate(); break;

	case CMD_ADD:		err = cmd_add(); break;
	case CMD_UPDATE:	err = cmd_update(); break;
	case CMD_SYNC:		err = cmd_sync(); break;
	case CMD_EXTRACT:	err = cmd_extract(); break;
	case CMD_REMOVE:	err = cmd_remove(); break;
	case CMD_RENAME:	err = cmd_rename(true); break;
	case CMD_SETTITLE:	err = cmd_rename(false); break;
	case CMD_TOUCH:		err = cmd_touch(); break;
	case CMD_VERIFY:	err = cmd_verify(); break;

	case CMD_FILETYPE:	err = cmd_filetype(); break;

	// no default case defined
	//	=> compiler checks the existence of all enum values

	case CMD__NONE:
	case CMD__N:
	    help_exit(false);
    }

    return PrintErrorStat(err,cmd_ct->name1);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       main()                    ///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    SetupLib(argc,argv,WWT_SHORT,PROG_WWT);
    allow_fst = true;

    ASSERT( OPT__N_SPECIFIC <= 64 );

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s%s for more info.\n\n",TITLE,URI_HOME,WWT_SHORT);
	hint_exit(ERR_OK);
    }

    enumError err = CheckEnvOptions("WWT_OPT",CheckOptions);
    if (err)
	hint_exit(err);

    err = CheckOptions(argc,argv,false);
    if (err)
	hint_exit(err);

    err = CheckCommand(argc,argv);

    if (SIGINT_level)
	err = ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////
