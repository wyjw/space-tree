/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
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

#pragma once

#include "portability/toku_portability.h"
#include "toku_htonl.h"
#include "toku_assert.h"
#include "toku_stdlib.h"

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "toku_path.h"

#include "ft/serialize/block_allocator.h"
#include "ft/serialize/block_table.h"
#include "ft/cachetable/cachetable.h"
#include "ft/cachetable/cachetable-internal.h"
#include "ft/cursor.h"
#include "ft/ft.h"
#include "ft/ft-ops.h"
#include "ft/serialize/ft-serialize.h"
#include "ft/serialize/ft_node-serialize.h"
#include "ft/logger/log-internal.h"
#include "ft/logger/logger.h"
#include "ft/node.h"
#include "util/bytestring.h"
#include <sys/ioctl.h>
#include <linux/treenvme_ioctl.h>
#include <iostream>

#define TOKU_TEST_FILENAME "/dev/treenvme0"
#define DEBUG 1

#define CKERR(r) ({ int __r = r; if (__r!=0) fprintf(stderr, "%s:%d error %d %s\n", __FILE__, __LINE__, __r, strerror(r)); assert(__r==0); })
#define CKERR2(r,r2) do { if (r!=r2) fprintf(stderr, "%s:%d error %d %s, expected %d\n", __FILE__, __LINE__, r, strerror(r), r2); assert(r==r2); } while (0)
#define CKERR2s(r,r2,r3) do { if (r!=r2 && r!=r3) fprintf(stderr, "%s:%d error %d %s, expected %d or %d\n", __FILE__, __LINE__, r, strerror(r), r2,r3); assert(r==r2||r==r3); } while (0)

#define DEBUG_LINE() do { \
    fprintf(stderr, "%s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__); \
    fflush(stderr); \
} while (0)

const uint32_t len_ignore = 0xFFFFFFFF;
const double USECS_PER_SEC = 1000000.0;

static const prepared_txn_callback_t NULL_prepared_txn_callback         __attribute__((__unused__)) = NULL;
static const keep_cachetable_callback_t  NULL_keep_cachetable_callback  __attribute__((__unused__)) = NULL;
static const TOKULOGGER NULL_logger                                     __attribute__((__unused__)) = NULL;

// dummymsn needed to simulate msn because test messages are injected at a lower level than toku_ft_root_put_msg()
#define MIN_DUMMYMSN ((MSN) {(uint64_t)1<<62})
static MSN dummymsn;      
static int dummymsn_initialized = 0;

// function used in tests
static int
noop_getf(uint32_t UU(keylen), const void *UU(key), uint32_t UU(vallen), const void *UU(val), void *extra, bool UU(lock_only))
{
	int *CAST_FROM_VOIDP(calledp, extra);
	(*calledp)++;
	return 0;
}

int btoku_setup(const char *name, int is_create, FT_HANDLE *ft_handle_p, int nodesize, int basementnodesize, enum toku_compression_method compression_method, CACHETABLE cachetable, TOKUTXN txn, int (*compare_fun)(DB *, const DBT *, const DBT *)) {
	FT_HANDLE ft_handle;
	// This makes the database create only
       	const int only_create = 1;

	toku_ft_handle_create(&ft_handle);
	toku_ft_handle_set_nodesize(ft_handle, nodesize);
	toku_ft_handle_set_basementnodesize(ft_handle, basementnodesize);
	toku_ft_handle_set_compression_method(ft_handle, compression_method);
	toku_ft_handle_set_fanout(ft_handle, 16);
	toku_ft_set_bt_compare(ft_handle, compare_fun);

    	int r = toku_ft_handle_open(ft_handle,
                            name, 1, 1, cachetable, txn);
	CKERR(r);
	FT ft = ft_handle->ft;
#ifdef DEBUG
	printf("FT is now set up.\n");
#endif
	*ft_handle_p = ft_handle;
	return r;
}

int btoku_setup_old(const char *name, int is_create, FT_HANDLE *ft_handle_p, int nodesize, int basementnodesize, enum toku_compression_method compression_method, CACHETABLE cachetable, TOKUTXN txn, int (*compare_fun)(DB *, const DBT *, const DBT *)) {
	FT_HANDLE ft_handle;
	// This makes the database create only
       	const int only_create = 0;

	toku_ft_handle_create(&ft_handle);
	toku_ft_handle_set_nodesize(ft_handle, nodesize);
	toku_ft_handle_set_basementnodesize(ft_handle, basementnodesize);
	toku_ft_handle_set_compression_method(ft_handle, compression_method);
	toku_ft_handle_set_fanout(ft_handle, 16);
	toku_ft_set_bt_compare(ft_handle, compare_fun);

    	int r = toku_ft_handle_open_block(ft_handle,
                            name, 1, 0, cachetable, txn);
	CKERR(r);
	FT ft = ft_handle->ft;
#ifdef DEBUG
	printf("FT is now set up.\n");
#endif
	*ft_handle_p = ft_handle;
	return r;
}

int sync_blocktable(FT ft_h, struct treenvme_block_table *tbl1, int fd, int filesize) {
	/*
	if (!tbl)
		tbl = (struct treenvme_block_table *) malloc(sizeof(struct treenvme_block_table));
	*/
	struct treenvme_block_table tbl;
	tbl.length_of_array = ft_h->blocktable._current.length_of_array;		    
	tbl.smallest = { .b = ft_h->blocktable._current.smallest_never_used_blocknum.b };
	tbl.next_head = { .b = 50 };
	tbl.block_translation = (struct treenvme_block_translation_pair *) ft_h->blocktable._current.block_translation;

#ifdef DEBUG
	std::cout << "Preparing to sync blocktable \n";
	std::cout << "Print out TRANSLATION: " << "\n";
	std::cout << "Length of array of: " << tbl.length_of_array << "\n";
	//for (int i = 0; i < tbl.length_of_array; i++) {
	for (int i = 0; i < tbl.length_of_array; i++) {
		if ((tbl.block_translation[i].size < filesize) && (tbl.block_translation[i].u.diskoff < filesize)){
			std::cout << "SIZE OF: " << tbl.block_translation[i].size << "\n";
			std::cout << "DISKOFF OF: " << tbl.block_translation[i].u.diskoff << "\n";
		}
	}
	std::cout << "Finish cycling \n";
	std::cout << "Fd is at " << fd << "\n";
#endif

	ioctl(fd, TREENVME_IOCTL_REGISTER_BLOCKTABLE, tbl);

#ifdef DEBUG
	std::cout << "Registered in ioctl \n";
#endif
	return 0;
}

static void
initialize_dummymsn(void) {
    if (dummymsn_initialized == 0) {
        dummymsn_initialized = 1;
        dummymsn = MIN_DUMMYMSN;
    }
}

static UU() MSN 
next_dummymsn(void) {
    assert(dummymsn_initialized);
    ++(dummymsn.msn);
    return dummymsn;
}

static UU() MSN 
last_dummymsn(void) {
    assert(dummymsn_initialized);
    return dummymsn;
}


struct check_pair {
    uint32_t keylen;  // A keylen equal to 0xFFFFFFFF means don't check the keylen or the key.
    const void *key;     // A NULL key means don't check the key.
    uint32_t vallen;  // Similarly for vallen and null val.
    const void *val;
    int call_count;
};
static int
lookup_checkf (uint32_t keylen, const void *key, uint32_t vallen, const void *val, void *pair_v, bool lock_only) {
    if (!lock_only) {
        struct check_pair *pair = (struct check_pair *) pair_v;
        if (key!=NULL) {
            if (pair->keylen!=len_ignore) {
#ifdef DEBUG
		printf("Keylen of given pair is %u, and pair is %u\n", pair->keylen, keylen);
#endif
                assert(pair->keylen == keylen);
                if (pair->key){ 
#ifdef DEBUG
		char tkey[keylen];
		memcpy(tkey, key, keylen);
		char tpkey[keylen];
		memcpy(tpkey, pair->key, keylen);
		printf("Pair key given by '%.*s', and result key is '%.*s'\n", keylen, tkey, keylen, tpkey);
#endif
                    assert(memcmp(pair->key, key, keylen)==0);
		}
            }
#ifdef DEBUG
		char tval[vallen];
		memcpy(tval, val, vallen);
		char tpval[vallen];
		memcpy(tpval, pair->val, vallen);
		printf("Pair value is given by len %u, '%.*s' and '%.*s'\n", vallen, vallen, tval, vallen, tpval);
#endif
            if (pair->vallen!=len_ignore) {
                assert(pair->vallen == vallen);
                if (pair->val)
		{
#ifdef DEBUG
		//char tval[vallen];
		memcpy(tval, val, vallen);
		//char tpval[vallen];
		memcpy(tpval, pair->val, vallen);
		printf("Pair value is given by %.*s and %.*s\n", vallen, tval, vallen, tpval);
#endif
                assert(memcmp(pair->val, val, vallen)==0);
                }
	    }
            pair->call_count++; // this call_count is really how many calls were made with r==0
        }
    }
#ifdef DEBUG
    printf("Finish call count.\n");
#endif
    return 0;
}

static inline void
ft_lookup_and_check_nodup (FT_HANDLE t, const char *keystring, const char *valstring)
{
    DBT k;
    toku_fill_dbt(&k, keystring, strlen(keystring) + 1);
    struct check_pair pair = {(uint32_t) (1+strlen(keystring)), keystring,
                              (uint32_t) (1+strlen(valstring)), valstring,
            0};
    int r = toku_ft_lookup(t, &k, lookup_checkf, &pair);
    assert(r==0);
    assert(pair.call_count==1);
}

static inline void
ft_lookup_and_fail_nodup (FT_HANDLE t, char *keystring)
{
    DBT k;
    toku_fill_dbt(&k, keystring, strlen(keystring) + 1);
    struct check_pair pair = {(uint32_t) (1+strlen(keystring)), keystring,
            0, 0,
            0};
    int r = toku_ft_lookup(t, &k, lookup_checkf, &pair);
    assert(r!=0);
    assert(pair.call_count==0);
}

static UU() void fake_ydb_lock(void) {
}

static UU() void fake_ydb_unlock(void) {
}

static UU() void
def_flush (CACHEFILE f __attribute__((__unused__)),
       int UU(fd),
       CACHEKEY k  __attribute__((__unused__)),
       void *v     __attribute__((__unused__)),
       void **dd     __attribute__((__unused__)),
       void *e     __attribute__((__unused__)),
       PAIR_ATTR s      __attribute__((__unused__)),
       PAIR_ATTR* new_size      __attribute__((__unused__)),
       bool w      __attribute__((__unused__)),
       bool keep   __attribute__((__unused__)),
       bool c      __attribute__((__unused__)),
       bool UU(is_clone)
       ) {
}

static UU() void 
def_pe_est_callback(
    void* UU(ftnode_pv),
    void* UU(dd), 
    long* bytes_freed_estimate, 
    enum partial_eviction_cost *cost, 
    void* UU(write_extraargs)
    )
{
    *bytes_freed_estimate = 0;
    *cost = PE_CHEAP;
}

static UU() int 
def_pe_callback(
    void *ftnode_pv __attribute__((__unused__)), 
    PAIR_ATTR bytes_to_free __attribute__((__unused__)), 
    void* extraargs __attribute__((__unused__)),
    void (*finalize)(PAIR_ATTR bytes_freed, void *extra),
    void *finalize_extra
    )
{
    finalize(bytes_to_free, finalize_extra);
    return 0;
}

static UU() void
def_pe_finalize_impl(PAIR_ATTR UU(bytes_freed), void *UU(extra)) { }

static UU() bool def_pf_req_callback(void* UU(ftnode_pv), void* UU(read_extraargs)) {
  return false;
}

  static UU() int def_pf_callback(void* UU(ftnode_pv), void* UU(dd), void* UU(read_extraargs), int UU(fd), PAIR_ATTR* UU(sizep)) {
  assert(false);
  return 0;
}

static UU() int
def_fetch (CACHEFILE f        __attribute__((__unused__)),
       PAIR UU(p),
       int UU(fd),
       CACHEKEY k         __attribute__((__unused__)),
       uint32_t fullhash __attribute__((__unused__)),
       void **value       __attribute__((__unused__)),
       void **dd     __attribute__((__unused__)),
       PAIR_ATTR *sizep        __attribute__((__unused__)),
       int  *dirtyp,
       void *extraargs    __attribute__((__unused__))
       ) {
    *dirtyp = 0;
    *value = NULL;
    *sizep = make_pair_attr(8);
    return 0;
}

static UU() void
put_callback_nop(
    CACHEKEY UU(key),
    void *UU(v),
    PAIR UU(p)) {
}

static UU() int
fetch_die(
    CACHEFILE UU(thiscf), 
    PAIR UU(p),
    int UU(fd), 
    CACHEKEY UU(key), 
    uint32_t UU(fullhash), 
    void **UU(value),
    void **UU(dd), 
    PAIR_ATTR *UU(sizep), 
    int *UU(dirtyp), 
    void *UU(extraargs)
    )
{
    assert(0); // should not be called
    return 0;
}


static UU() int
def_cleaner_callback(
    void* UU(ftnode_pv),
    BLOCKNUM UU(blocknum),
    uint32_t UU(fullhash),
    void* UU(extraargs)
    )
{
    assert(false);
    return 0;
}

static UU() CACHETABLE_WRITE_CALLBACK def_write_callback(void* write_extraargs) {
    CACHETABLE_WRITE_CALLBACK wc;
    wc.flush_callback = def_flush;
    wc.pe_est_callback = def_pe_est_callback;
    wc.pe_callback = def_pe_callback;
    wc.cleaner_callback = def_cleaner_callback;
    wc.write_extraargs = write_extraargs;
    wc.clone_callback = nullptr;
    wc.checkpoint_complete_callback = nullptr;
    return wc;
}

class evictor_test_helpers {
public:
    static void set_hysteresis_limits(evictor* ev, long low_size_watermark, long high_size_watermark) {
        ev->m_low_size_watermark = low_size_watermark;
        ev->m_low_size_hysteresis = low_size_watermark;
        ev->m_high_size_hysteresis = high_size_watermark;
        ev->m_high_size_watermark = high_size_watermark;
    }
    static void disable_ev_thread(evictor* ev) {
        toku_mutex_lock(&ev->m_ev_thread_lock);
        ev->m_period_in_seconds = 0;
        // signal eviction thread so that it wakes up
        // and then sleeps indefinitely
        ev->signal_eviction_thread_locked();
        toku_mutex_unlock(&ev->m_ev_thread_lock);
        // sleep for one second to ensure eviction thread picks up new period
        usleep(1*1024*1024);
    }
    static uint64_t get_num_eviction_runs(evictor* ev) {
        return ev->m_num_eviction_thread_runs;
    }
};

UU()
static void copy_dbt(DBT *dest, const DBT *src) {
    assert(dest->flags & DB_DBT_REALLOC);
    dest->data = toku_realloc(dest->data, src->size);
    dest->size = src->size;
    memcpy(dest->data, src->data, src->size);
}

int verbose=0;

static inline void
default_parse_args (int argc, const char *argv[]) {
    const char *progname=argv[0];
    argc--; argv++;
    while (argc>0) {
  if (strcmp(argv[0],"-v")==0) {
      ++verbose;
  } else if (strcmp(argv[0],"-q")==0) {
      verbose=0;
  } else {
      fprintf(stderr, "Usage:\n %s [-v] [-q]\n", progname);
      exit(1);
  }
  argc--; argv++;
    }
}

int test_main(int argc, const char *argv[]);

int
main(int argc, const char *argv[]) {
    initialize_dummymsn();
    int rinit = toku_ft_layer_init();
    CKERR(rinit);
    int r = test_main(argc, argv);
    toku_ft_layer_destroy();
    return r;
}
