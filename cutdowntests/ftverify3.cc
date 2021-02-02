#ident "$Id$"
/*======
This file is part of PerconaFT.


Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved.

    PerconaFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2,
    as published by the Free Software Foundation.

    PerconaFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PerconaFT.  If not, see <http://www.gnu.org/licenses/>.

----------------------------------------

    PerconaFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License, version 3,
    as published by the Free Software Foundation.

    PerconaFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with PerconaFT.  If not, see <http://www.gnu.org/licenses/>.
======= */

#ident "Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved."

#include "test_benchmark.h"

#include <util/dbt.h>
#include <ft/ft-cachetable-wrappers.h>
#include <linux/fs.h>

#define DEBUG 1
#define DEBUGMAX 1
//#define DEBUGMAXONE 1
#define DEBUGVAL 1
#define TIME 1
#define LEN 30
#define VALLEN 120
#define FLUSH 1

static TOKUTXN const null_txn = 0;
static int fd = 0;

//#define TOKU_TEST_FILENAME "/dev/nvme0n1"
#define TOKU_TEST_FILENAME "/dev/treenvme0"

// lambda function

// Each FT maintains a sequential insert heuristic to determine if its
// worth trying to insert directly into a well-known rightmost leaf node.
//
// The heuristic is only maintained when a rightmost leaf node is known.
//
// This test verifies that sequential inserts increase the seqinsert score
// and that a single non-sequential insert resets the score.

static void test_inserts(void) {
    fd = open(TOKU_TEST_FILENAME,
                  O_RDWR | O_CREAT | O_DIRECT | O_BINARY,
                  S_IRWXU | S_IRWXG | S_IRWXO);
    
    // get filesize for a block device
    int filesize = 0;
    ioctl(fd, BLKGETSIZE64, &filesize);
    printf("File size of %d\n", filesize); 

    int r = 0;
    CACHETABLE ct;
    toku_cachetable_create(&ct, 0, ZERO_LSN, nullptr);
    FT_HANDLE ft_handle;
   
    // old version doesn't always try to create
     r = btoku_setup_old(TOKU_TEST_FILENAME, 1, &ft_handle, 4*1024*1024, 64*1024, TOKU_NO_COMPRESSION, ct, null_txn, toku_builtin_compare_fun);    
    CKERR(r);
    FT ft = ft_handle->ft;

    // Insert many rows sequentially. This is enough data to:
    // - force the root to split (the righmost leaf will then be known)
    // - raise the seqinsert score high enough to enable direct rightmost injections
    const int rows_to_insert = 200;
    for (int i = 0; i < rows_to_insert; i++) {
	// add key and value
    	int k;
    	DBT key, val;
    	const int val_size = VALLEN;
    	
	// Ignore these, memset is wierd. use char and for-loop instead.
	// char *XMALLOC_N(val_size, val_buf);
    	// memset(val_buf, (char)((i % 70) + '0'), val_size);

	int vlen = val_size;
	char tv[vlen];
	for (int j = i; j < i+vlen-1; j++) {
	   tv[j-i] = (j % 26) + '0';
	}
	tv[vlen-1] = '\0';	
#ifdef DEBUGVAL
	//printf("VAL at %d is %.*s\n", i, val_size, val_buf);
#endif
    	toku_fill_dbt(&val, &tv, val_size);

	k = toku_htonl(i);

	int len = LEN;
	char tk[len];
	for (int j = i; j < i+len-1; j++) {
	   tk[j-i] = (j % 26) + '0';
	}
	tk[len-1] = '\0';	
        toku_fill_dbt(&key, &tk, sizeof(tk));
#ifdef DEBUG
	printf("Print row: %d\n", i);
	printf("KEY SIZE is %d\n", key.size);
	printf("VAL SIZE is %d\n", val.size);
#ifdef DEBUGMAX
	printf("\nKEY IS: ");
	for (int j = 0; j < key.size; j++) {
		printf("%c", ((char *)key.data)[j]);
	}
	printf("\nVALUE IS: ");
	for (int j = 0; j < val.size; j++) {
		printf("%c", ((char *)val.data)[j]);
	}
#endif
	printf("\n");
	//printf("TxnManager: %d\n", toku_ft_get_txn_manager(ft_handle));
	printf("FtHandle: %d\n", ft_handle->ft);	
	//printf("Logger: %d\n", toku_cachefile_logger(ft_handle->ft->cf));
#endif
        toku_ft_insert(ft_handle, &key, &val, NULL);
    }
    

    // check state of all the nodes, everything
#ifdef DEBUGMAXONE
    ft->blocktable.dump_translation_table(stdout);
#endif

    //invariant(ft->rightmost_blocknum.b != RESERVED_BLOCKNUM_NULL);
    //invariant(ft->seqinsert_score == FT_SEQINSERT_SCORE_THRESHOLD);  	  
#ifdef FLUSH
	printf("Got here before second loop.\n");
#endif

#ifdef SYNC
	struct treenvme_block_table tbl;
	tbl.length_of_array = ft_h->blocktable._current.length_of_array;
	tbl.smallest = { .b = ft_h->blocktable._current.smallest_never_used_blocknum.b };
	tbl.next_head = { .b = 50 };
	tbl.block_translation = (struct treenvme_block_translation_pair *)ft_h->blocktable._current.block_translation;	
	ioctl(fd, TREENVME_IOCTL_REGISTER_BLOCKTABLE, tbl);	
#endif
    // sync parts
    // struct treenvme_block_table tbl;
    // sync_blocktable(ft, &tbl, fd, filesize); 
    for (int i = 0; i < rows_to_insert; i++)
    {
	DBT k;
	int r;
	int called;
#ifdef FLUSH
    	//CACHEFILE cf;
    	//cf = ft_h->ft->cf;
	cachetable_flush_cachefile(ct, NULL, true);
#endif
	FT_CURSOR cursor = 0;
	r = toku_ft_cursor(ft_handle, &cursor, null_txn, false, false);
	CKERR(r);
#ifdef TIME
	struct timeval t[2];
	gettimeofday(&t[0], NULL);
#endif
    	// do one search
	int len = LEN;
	char tk[len];
	for (int j = i; j < i+len-1; j++) {
	   tk[j-i] = (j % 26) + '0';
	}
	tk[len-1] = '\0';	
	int vlen = VALLEN;
	char tv[vlen];
	for (int j = i; j < i+vlen-1; j++) {
	   tv[j-i] = (j % 26) + '0';
	}
	tv[vlen-1] = '\0';	
	struct check_pair pair = {LEN,tk,VALLEN,tv,0};
	// r = toku_ft_cursor_first(cursor, lookup_checkf, &pair);
	r = toku_ft_lookup(ft_handle, toku_fill_dbt(&k, &tk, LEN), lookup_checkf, &pair);
#ifdef TIME
	gettimeofday(&t[1], NULL);
	double dt;
	dt = (t[1].tv_sec - t[0].tv_sec) + ((t[1].tv_usec - t[0].tv_usec) / USECS_PER_SEC);
	dt *= 1000;
	printf("Time (in ms): %0.05lf", dt);
#endif
	CKERR(r);
	toku_ft_cursor_close(cursor);
    }

    //toku_free(val_buf);
    toku_ft_handle_close(ft_handle);
    toku_cachetable_close(&ct);
}

int test_main(int argc, const char *argv[]) {
    default_parse_args(argc, argv);
    test_inserts();
    return 0;
}
