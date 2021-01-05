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

#define TOKU_TEST_FILENAME "/dev/nvme0n1"
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
		printf("Keylen of given pair is %d, and pair is %d\n", pair->keylen, keylen);
#endif
                assert(pair->keylen == keylen);
                if (pair->key) 
                    assert(memcmp(pair->key, key, keylen)==0);
            }
            if (pair->vallen!=len_ignore) {
                assert(pair->vallen == vallen);
                if (pair->val)
                    assert(memcmp(pair->val, val, vallen)==0);
            }
            pair->call_count++; // this call_count is really how many calls were made with r==0
        }
    }
    return 0;
}


static void * const zero_value = nullptr;
static PAIR_ATTR const zero_attr = {
    .size = 0, 
    .nonleaf_size = 0, 
    .leaf_size = 0, 
    .rollback_size = 0, 
    .cache_pressure_size = 0,
    .is_valid = true
};


static inline void ctpair_destroy(PAIR p) {
    p->value_rwlock.deinit();
    paranoid_invariant(p->refcount == 0);
    nb_mutex_destroy(&p->disk_nb_mutex);
    toku_cond_destroy(&p->refcount_wait);
    toku_free(p);
}

static inline void pair_lock(PAIR p) {
    toku_mutex_lock(p->mutex);
}

static inline void pair_unlock(PAIR p) {
    toku_mutex_unlock(p->mutex);
}

static void cachetable_remove_pair (pair_list* list, evictor* ev, PAIR p) {
    list->evict_completely(p);
    ev->remove_pair_attr(p->attr);
}

static void cachetable_free_pair(PAIR p) {
    CACHETABLE_FLUSH_CALLBACK flush_callback = p->flush_callback;
    CACHEKEY key = p->key;
    void *value = p->value_data;
    void* disk_data = p->disk_data;
    void *write_extraargs = p->write_extraargs;
    PAIR_ATTR old_attr = p->attr;
    
    PAIR_ATTR new_attr = p->attr;
    // Note that flush_callback is called with write_me false, so the only purpose of this 
    // call is to tell the ft layer to evict the node (keep_me is false).
    // Also, because we have already removed the PAIR from the cachetable in 
    // cachetable_remove_pair, we cannot pass in p->cachefile and p->cachefile->fd
    // for the first two parameters, as these may be invalid (#5171), so, we
    // pass in NULL and -1, dummy values
    flush_callback(NULL, -1, key, value, &disk_data, write_extraargs, old_attr, &new_attr, false, false, true, false);
    
    ctpair_destroy(p);
}

struct pair_flush_for_close{
    PAIR p;
    BACKGROUND_JOB_MANAGER bjm;
};


// assumes value_rwlock and disk_nb_mutex held on entry
// responsibility of this function is to only write a locked PAIR to disk
// and NOTHING else. We do not manipulate the state of the PAIR
// of the cachetable here (with the exception of ct->size_current for clones)
//
// No pair_list lock should be held, and the PAIR mutex should not be held
//
static void cachetable_only_write_locked_data(
    evictor* ev,
    PAIR p, 
    bool for_checkpoint,
    PAIR_ATTR* new_attr,
    bool is_clone
    ) 
{    
    CACHETABLE_FLUSH_CALLBACK flush_callback = p->flush_callback;
    CACHEFILE cachefile = p->cachefile;
    CACHEKEY key = p->key;
    void *value = is_clone ? p->cloned_value_data : p->value_data;
    void *disk_data = p->disk_data;
    void *write_extraargs = p->write_extraargs;
    PAIR_ATTR old_attr;
    // we do this for drd. If we are a cloned pair and only 
    // have the disk_nb_mutex, it is a race to access p->attr.
    // Luckily, old_attr here is only used for some test applications,
    // so inaccurate non-size fields are ok.
    if (is_clone) {
        old_attr = make_pair_attr(p->cloned_value_size);
    }
    else {
        old_attr = p->attr;
    }
    bool dowrite = true;
        
    // write callback
    flush_callback(
        cachefile, 
        cachefile->fd, 
        key, 
        value, 
        &disk_data, 
        write_extraargs, 
        old_attr, 
        new_attr, 
        dowrite, 
        is_clone ? false : true, // keep_me (only keep if this is not cloned pointer)
        for_checkpoint, 
        is_clone //is_clone
        );
    p->disk_data = disk_data;
    if (is_clone) {
        p->cloned_value_data = NULL;
        ev->remove_cloned_data_size(p->cloned_value_size);
        p->cloned_value_size = 0;
    }    
}


static void cachetable_flush_pair_for_close(void* extra) {
    struct pair_flush_for_close *CAST_FROM_VOIDP(args, extra);
    PAIR p = args->p;
    CACHEFILE cf = p->cachefile;
    CACHETABLE ct = cf->cachetable;
    PAIR_ATTR attr;
    cachetable_only_write_locked_data(
        &ct->ev,
        p,
        false, // not for a checkpoint, as we assert above
        &attr,
        false // not a clone
        );            
    p->dirty = CACHETABLE_CLEAN;
    bjm_remove_background_job(args->bjm);
    toku_free(args);
}


static void flush_pair_for_close_on_background_thread(
    PAIR p, 
    BACKGROUND_JOB_MANAGER bjm, 
    CACHETABLE ct
    ) 
{
    pair_lock(p);
    assert(p->value_rwlock.users() == 0);
    assert(nb_mutex_users(&p->disk_nb_mutex) == 0);
    assert(!p->cloned_value_data);
    if (p->dirty == CACHETABLE_DIRTY) {
        int r = bjm_add_background_job(bjm);
        assert_zero(r);
        struct pair_flush_for_close *XMALLOC(args);
        args->p = p;
        args->bjm = bjm;
        toku_kibbutz_enq(ct->ct_kibbutz, cachetable_flush_pair_for_close, args);
    }
    pair_unlock(p);
}

static void remove_pair_for_close(PAIR p, CACHETABLE ct, bool completely) {
    pair_lock(p);
    assert(p->value_rwlock.users() == 0);
    assert(nb_mutex_users(&p->disk_nb_mutex) == 0);
    assert(!p->cloned_value_data);
    assert(p->dirty == CACHETABLE_CLEAN);
    assert(p->refcount == 0);
    if (completely) {
        cachetable_remove_pair(&ct->list, &ct->ev, p);
        pair_unlock(p);
        // TODO: Eventually, we should not hold the write list lock during free
        cachetable_free_pair(p);
    }
    else {
        // if we are not evicting completely,
        // we only want to remove the PAIR from the cachetable,
        // that is, remove from the hashtable and various linked
        // list, but we will keep the PAIRS and the linked list
        // in the cachefile intact, as they will be cached away
        // in case an open comes soon.
        ct->list.evict_from_cachetable(p);
        pair_unlock(p);
    }
}

// helper function for cachetable_flush_cachefile, which happens on a close
// writes out the dirty pairs on background threads and returns when
// the writing is done
static void write_dirty_pairs_for_close(CACHETABLE ct, CACHEFILE cf) {
    BACKGROUND_JOB_MANAGER bjm = NULL;
    bjm_init(&bjm);
    ct->list.write_list_lock(); // TODO: (Zardosht), verify that this lock is unnecessary to take here
    PAIR p = NULL;
    // write out dirty PAIRs
    uint32_t i;
    if (cf) {
        for (i = 0, p = cf->cf_head;
            i < cf->num_pairs;
            i++, p = p->cf_next)
        {
            flush_pair_for_close_on_background_thread(p, bjm, ct);
        }
    }
    else {
        for (i = 0, p = ct->list.m_checkpoint_head;
            i < ct->list.m_n_in_table;
            i++, p = p->clock_next)
        {
            flush_pair_for_close_on_background_thread(p, bjm, ct);
        }
    }
    ct->list.write_list_unlock();
    bjm_wait_for_jobs_to_finish(bjm);
    bjm_destroy(bjm);
}

static void remove_all_pairs_for_close(CACHETABLE ct, CACHEFILE cf, bool evict_completely) {
    ct->list.write_list_lock();
    if (cf) {
        if (evict_completely) {
            // if we are evicting completely, then the PAIRs will
            // be removed from the linked list managed by the
            // cachefile, so this while loop works
            while (cf->num_pairs > 0) {
                PAIR p = cf->cf_head;
                remove_pair_for_close(p, ct, evict_completely);
            }
        }
        else {
            // on the other hand, if we are not evicting completely,
            // then the cachefile's linked list stays intact, and we must
            // iterate like this.
            for (PAIR p = cf->cf_head; p; p = p->cf_next) {
                remove_pair_for_close(p, ct, evict_completely);
            }
        }
    }
    else {
        while (ct->list.m_n_in_table > 0) {
            PAIR p = ct->list.m_checkpoint_head;
            // if there is no cachefile, then we better
            // be evicting completely because we have no
            // cachefile to save the PAIRs to. At least,
            // we have no guarantees that the cachefile
            // will remain good
            invariant(evict_completely);
            remove_pair_for_close(p, ct, true);
        } 
    }
    ct->list.write_list_unlock();
}

static void verify_cachefile_flushed(CACHETABLE ct UU(), CACHEFILE cf UU()) {
#ifdef TOKU_DEBUG_PARANOID
    // assert here that cachefile is flushed by checking
    // pair_list and finding no pairs belonging to this cachefile
    // Make a list of pairs that belong to this cachefile.
    if (cf) {
        ct->list.write_list_lock();
        // assert here that cachefile is flushed by checking
        // pair_list and finding no pairs belonging to this cachefile
        // Make a list of pairs that belong to this cachefile.
        uint32_t i;
        PAIR p = NULL;
        for (i = 0, p = ct->list.m_checkpoint_head; 
             i < ct->list.m_n_in_table; 
             i++, p = p->clock_next) 
         {
             assert(p->cachefile != cf);
         }
         ct->list.write_list_unlock();
    }
#endif
}

// Flush (write to disk) all of the pairs that belong to a cachefile (or all pairs if 
// the cachefile is NULL.
// Must be holding cachetable lock on entry.
// 
// This function assumes that no client thread is accessing or 
// trying to access the cachefile while this function is executing.
// This implies no client thread will be trying to lock any nodes
// belonging to the cachefile.
//
// This function also assumes that the cachefile is not in the process
// of being used by a checkpoint. If a checkpoint is currently happening,
// it does NOT include this cachefile.
//
static void cachetable_flush_cachefile(CACHETABLE ct, CACHEFILE cf, bool evict_completely) {
    //
    // Because work on a kibbutz is always done by the client thread,
    // and this function assumes that no client thread is doing any work
    // on the cachefile, we assume that no client thread will be adding jobs
    // to this cachefile's kibbutz.
    //
    // The caller of this function must ensure that there are 
    // no jobs added to the kibbutz. This implies that the only work other 
    // threads may be doing is work by the writer threads.
    //
    // first write out dirty PAIRs
    write_dirty_pairs_for_close(ct, cf);

    // now that everything is clean, get rid of everything
    remove_all_pairs_for_close(ct, cf, evict_completely);

    verify_cachefile_flushed(ct, cf);
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
