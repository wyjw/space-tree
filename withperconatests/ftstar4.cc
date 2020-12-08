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
#include "bndata.h"
//#include <sys/ioctl.h>
#include <stdint.h>
#include <iostream>
//#include <linux/treenvme_ioctl.h>

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
//const double USECS_PER_SEC = 1000000.0;

#define TOKU_TEST_FILENAME "/dev/treenvme0"
//#define TOKU_TEST_FILENAME1 "/dev/nvme0n1"
#define DEBUG 1
#define DEBUGSMALL 1

static size_t le_add_to_bn(bn_data *bn,
                           uint32_t idx,
                           const char *key,
                           int keysize,
                           const char *val,
                           int valsize) {
    LEAFENTRY r = NULL;
    uint32_t size_needed = LE_CLEAN_MEMSIZE(valsize);
    void *maybe_free = nullptr;
    bn->get_space_for_insert(idx, key, keysize, size_needed, &r, &maybe_free);
    if (maybe_free) {
        toku_free(maybe_free);
    }
    resource_assert(r);
    r->type = LE_CLEAN;
    r->u.clean.vallen = valsize;
    memcpy(r->u.clean.val, val, valsize);
    return size_needed + keysize + sizeof(uint32_t);
}

static int long_key_cmp(DB *UU(e), const DBT *a, const DBT *b) {
    const long *CAST_FROM_VOIDP(x, a->data);
    const long *CAST_FROM_VOIDP(y, b->data);
    return (*x > *y) - (*x < *y);
}

static void test_serialize_nonleaf_two(int valsize,
                                   int nelts,
                                   double entropy,
                                   int ser_runs,
                                   int deser_runs, int height) {
    //    struct ft_handle source_ft;
    struct ftnode sn, *dn;
    struct ftnode sn1, *dn1;

    int fd = open(TOKU_TEST_FILENAME,
                  O_RDWR | O_CREAT | O_DIRECT | O_BINARY,
                  S_IRWXU | S_IRWXG | S_IRWXO);
    invariant(fd >= 0);

    int r;

    //    source_ft.fd=fd;
    sn.max_msn_applied_to_node_on_disk.msn = 0;
    sn.flags = 0x11223344;
    sn.blocknum.b = 20;
    sn.layout_version = FT_LAYOUT_VERSION;
    sn.layout_version_original = FT_LAYOUT_VERSION;
    sn.height = height;
    sn.n_children = 8;
    sn.set_dirty();
    sn.oldest_referenced_xid_known = TXNID_NONE;
    MALLOC_N(sn.n_children, sn.bp);
    sn.pivotkeys.create_empty();
    for (int i = 0; i < sn.n_children; ++i) {
        BP_BLOCKNUM(&sn, i).b = 30 + (i * 5);
        BP_STATE(&sn, i) = PT_AVAIL;
        set_BNC(&sn, i, toku_create_empty_nl());
    }
    // Create XIDS
    XIDS xids_0 = toku_xids_get_root_xids();
    XIDS xids_123;
    r = toku_xids_create_child(xids_0, &xids_123, (TXNID)123);
    CKERR(r);
    toku::comparator cmp;
    cmp.create(long_key_cmp, nullptr);
    int nperchild = nelts / 8;
    for (int ck = 0; ck < sn.n_children; ++ck) {
        long k;
        NONLEAF_CHILDINFO bnc = BNC(&sn, ck);
        for (long i = 0; i < nperchild; ++i) {
            k = ck * nperchild + i;
            char buf[valsize];
            int c;
            for (c = 0; c < valsize * entropy;) {
                int *p = (int *)&buf[c];
                *p = rand();
                c += sizeof(*p);
            }
            memset(&buf[c], 0, valsize - c);

            toku_bnc_insert_msg(bnc,
                                &k,
                                sizeof k,
                                buf,
                                valsize,
                                FT_NONE,
                                next_dummymsn(),
                                xids_123,
                                true,
                                cmp);
        }
        if (ck < 7) {
            DBT pivotkey;
            sn.pivotkeys.insert_at(toku_fill_dbt(&pivotkey, &k, sizeof(k)), ck);
        }
    }

    //    source_ft.fd=fd;
    sn1.max_msn_applied_to_node_on_disk.msn = 0;
    sn1.flags = 0x11223344;
    sn1.blocknum.b = 50;
    sn1.layout_version = FT_LAYOUT_VERSION;
    sn1.layout_version_original = FT_LAYOUT_VERSION;
    sn1.height = height;
    sn1.n_children = 8;
    sn1.set_dirty();
    sn1.oldest_referenced_xid_known = TXNID_NONE;
    MALLOC_N(sn1.n_children, sn1.bp);
    sn1.pivotkeys.create_empty();
    for (int i = 0; i < sn1.n_children; ++i) {
        BP_BLOCKNUM(&sn1, i).b = 40 + (i * 5);
        BP_STATE(&sn1, i) = PT_AVAIL;
        set_BNC(&sn1, i, toku_create_empty_nl());
    }
    // Create XIDS
    XIDS xids_0_1 = toku_xids_get_root_xids();
    XIDS xids_123_1;
    r = toku_xids_create_child(xids_0_1, &xids_123_1, (TXNID)123);
    CKERR(r);
    toku::comparator cmp_1;
    cmp_1.create(long_key_cmp, nullptr);
    int nperchild_1 = nelts / 8;
    for (int ck = 0; ck < sn1.n_children; ++ck) {
        long k;
        NONLEAF_CHILDINFO bnc = BNC(&sn1, ck);
        for (long i = 0; i < nperchild_1; ++i) {
            k = ck * nperchild_1 + i;
            char buf[valsize];
            int c;
            for (c = 0; c < valsize * entropy;) {
                int *p = (int *)&buf[c];
                *p = rand();
                c += sizeof(*p);
            }
            memset(&buf[c], 0, valsize - c);

            toku_bnc_insert_msg(bnc,
                                &k,
                                sizeof k,
                                buf,
                                valsize,
                                FT_NONE,
                                next_dummymsn(),
                                xids_123_1,
                                true,
                                cmp_1);
        }
        if (ck < 7) {
            DBT pivotkey;
            sn1.pivotkeys.insert_at(toku_fill_dbt(&pivotkey, &k, sizeof(k)), ck);
        }
    }
    // Cleanup:
    toku_xids_destroy(&xids_0);
    toku_xids_destroy(&xids_123);
    toku_xids_destroy(&xids_0_1);
    toku_xids_destroy(&xids_123_1);
    cmp.destroy();

    FT_HANDLE XMALLOC(ft);
    FT XCALLOC(ft_h);
    toku_ft_init(ft_h,
                 make_blocknum(0),
                 ZERO_LSN,
                 TXNID_NONE,
                 4 * 1024 * 1024,
                 128 * 1024,
                 TOKU_NO_COMPRESSION,
                 16);
    ft_h->cmp.create(long_key_cmp, nullptr);
    ft->ft = ft_h;

    ft_h->blocktable.create();

    //invariant(b.b == 50);

    BLOCKNUM b = make_blocknum(0); 
    while (b.b < 100) {
        ft_h->blocktable.allocate_blocknum(&b, ft_h);
    }
    invariant(b.b == 100);

    // Verification stuff
    for (int i = 0; i < 100; i++)
	    ft_h->blocktable.verify_blocknum_allocated(make_blocknum(i));
    ft_h->blocktable.verify_no_free_blocknums();

    for (int i = RESERVED_BLOCKNUMS; i < 100; i++)
    {
    	// Allocate hundred blocks.
        DISKOFF offset;
        DISKOFF size;
#ifdef DEBUG
	int64_t file_size;
	int r = toku_os_get_file_size(fd, &file_size);
	printf("FILE SIZE OF %d\n", file_size);
#endif
        ft_h->blocktable.realloc_on_disk(make_blocknum(i), 100, &offset, ft_h, fd, false);
        //invariant(offset == (DISKOFF)BlockAllocator::BLOCK_ALLOCATOR_TOTAL_HEADER_RESERVE);

        ft_h->blocktable.translate_blocknum_to_offset_size(make_blocknum(i), &offset, &size);
        //invariant(offset == (DISKOFF)BlockAllocator::BLOCK_ALLOCATOR_TOTAL_HEADER_RESERVE);
    }

    struct timeval t[2];
    
    struct treenvme_block_table tbl;
    tbl.length_of_array = ft_h->blocktable._current.length_of_array;
    tbl.smallest = {.b = ft_h->blocktable._current.smallest_never_used_blocknum.b}; 
    // tbl.next_head = ft_h->blocktable._current.blocknum_freelist_head.b;
    tbl.next_head = {.b = 50};
    tbl.block_translation = (struct treenvme_block_translation_pair *) ft_h->blocktable._current.block_translation;

#ifdef DEBUGSMALL
	std::cout << "LENGTH OF ARRAY: " << tbl.length_of_array << "\n";
#endif 
#ifdef DEBUG
    std::cout << "Print out TRANSLATION: " << "\n";
    for (int i = 0; i < 100; i++) {
	std::cout << "SIZE OF: " << tbl.block_translation[i].size << "\n";
        std::cout << "DISKOFF OF: " << tbl.block_translation[i].u.diskoff << "\n";
    }
   /* 
    for (int i = 0; i < tbl.length_of_array; i++)
    {
	std::cout << "DISKOFF OF " << tbl.block_translation[i].u.diskoff << "\n";
    }
    */
#endif

    ioctl(fd, TREENVME_IOCTL_REGISTER_BLOCKTABLE, tbl);
    gettimeofday(&t[0], NULL);
    FTNODE_DISK_DATA ndd = NULL;
    for (int i = RESERVED_BLOCKNUMS; i <= 100; i++){
#ifdef DEBUG
	std::cout << "Node num " << i << "\n";
#endif
    sn1.flags = 0x11223344;
    sn1.blocknum.b = i;
    sn1.layout_version = FT_LAYOUT_VERSION;
    sn1.layout_version_original = FT_LAYOUT_VERSION;
    sn1.height = height;
    sn1.n_children = 8;
    sn1.set_dirty();
    sn1.oldest_referenced_xid_known = TXNID_NONE;
    sn1.pivotkeys.create_empty();
    for (int j = 0; j < sn1.n_children; ++j) {
        BP_BLOCKNUM(&sn1, j).b = 40 + (j * 5) % 100;
        BP_STATE(&sn1, j) = PT_AVAIL;
        set_BNC(&sn1, j, toku_create_empty_nl());
    }
    XIDS xids_0_1 = toku_xids_get_root_xids();
    XIDS xids_123_1;
    r = toku_xids_create_child(xids_0_1, &xids_123_1, (TXNID)123);
    CKERR(r);
    toku::comparator cmp_1;
    cmp_1.create(long_key_cmp, nullptr);
    int nperchild_1 = nelts / 8;
    for (int ck = 0; ck < sn1.n_children; ++ck) {
        long k;
        NONLEAF_CHILDINFO bnc = BNC(&sn1, ck);
        for (long i = 0; i < nperchild_1; ++i) {
            k = ck * nperchild_1 + i;
            char buf[valsize];
            int c;
            for (c = 0; c < valsize * entropy;) {
                int *p = (int *)&buf[c];
                *p = rand();
                c += sizeof(*p);
            }
            memset(&buf[c], 0, valsize - c);

            toku_bnc_insert_msg(bnc,
                                &k,
                                sizeof k,
                                buf,
                                valsize,
                                FT_NONE,
                                next_dummymsn(),
                                xids_123_1,
                                true,
                                cmp_1);
        }
        if (ck < 7) {
            DBT pivotkey;
            sn1.pivotkeys.insert_at(toku_fill_dbt(&pivotkey, &k, sizeof(k)), ck);
        }
    }
    // Cleanup:
    toku_xids_destroy(&xids_0);
    toku_xids_destroy(&xids_123);
    toku_xids_destroy(&xids_0_1);
    toku_xids_destroy(&xids_123_1);
    cmp_1.destroy();
    r = toku_serialize_ftnode_to(fd, make_blocknum(i), &sn1, &ndd, true, ft->ft, false); 
    }
    //r = toku_serialize_ftnode_to(
    //    fd, make_blocknum(50), &sn1, &ndd, true, ft->ft, false);
    // lookup ndd info
    printf("NDD info: start of %u, size of %u\n", ndd->start, ndd->size);

    invariant(r == 0);
    gettimeofday(&t[1], NULL);
    double dt;
    dt = (t[1].tv_sec - t[0].tv_sec) +
         ((t[1].tv_usec - t[0].tv_usec) / USECS_PER_SEC);
    dt *= 1000;
    printf(
        "serialize nonleaf(ms):   %0.05lf (IGNORED RUNS=%d)\n", dt, ser_runs);

    ftnode_fetch_extra bfe;
    bfe.create_for_full_read(ft_h);
    //gettimeofday(&t[0], NULL);
    tbl.length_of_array = ft_h->blocktable._current.length_of_array;
    tbl.smallest = {.b = ft_h->blocktable._current.smallest_never_used_blocknum.b}; 
    // tbl.next_head = ft_h->blocktable._current.blocknum_freelist_head.b;
    tbl.next_head = {.b = 50};
    tbl.block_translation = (struct treenvme_block_translation_pair *) ft_h->blocktable._current.block_translation;

    ioctl(fd, TREENVME_IOCTL_REGISTER_BLOCKTABLE, tbl);
    gettimeofday(&t[0], NULL);
    FTNODE_DISK_DATA ndd2 = NULL;
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd2, &bfe); 
    //r = toku_deserialize_ftnode_from(
    //   fd, make_blocknum(50), 0 /*pass zero for hash*/, &dn, &ndd2, &bfe);
    
    invariant(r == 0);
    gettimeofday(&t[1], NULL);
    dt = (t[1].tv_sec - t[0].tv_sec) +
         ((t[1].tv_usec - t[0].tv_usec) / USECS_PER_SEC);
    dt *= 1000;
    printf(
        "deserialize nonleaf(ms): %0.05lf (IGNORED RUNS=%d)\n", dt, deser_runs);
    printf(
        "io time(ms) %lf decompress time(ms) %lf deserialize time(ms) %lf "
        "(IGNORED RUNS=%d)\n",
        tokutime_to_seconds(bfe.io_time) * 1000,
        tokutime_to_seconds(bfe.decompress_time) * 1000,
        tokutime_to_seconds(bfe.deserialize_time) * 1000,
        deser_runs);

    toku_ftnode_free(&dn);
    toku_destroy_ftnode_internals(&sn);

    //ft_h->blocktable.block_free(BlockAllocator::BLOCK_ALLOCATOR_TOTAL_HEADER_RESERVE, 100);
    //ft_h->blocktable.destroy();
    toku_free(ft_h->h);
    ft_h->cmp.destroy();
    toku_free(ft_h);
    toku_free(ft);
    toku_free(ndd);
    toku_free(ndd2);

    r = close(fd);
    invariant(r != -1);
}

int test_main(int argc __attribute__((__unused__)),
              const char *argv[] __attribute__((__unused__))) {
    const int DEFAULT_RUNS = 5;
    long valsize, nelts, ser_runs = DEFAULT_RUNS, deser_runs = DEFAULT_RUNS;
    double entropy = 0.3;

    if (argc != 3 && argc != 5) {
        fprintf(stderr,
                "Usage: %s <valsize> <nelts> [<serialize_runs> "
                "<deserialize_runs>]\n",
                argv[0]);
        fprintf(stderr, "Default (and min) runs is %d\n", DEFAULT_RUNS);
        return 2;
    }
    valsize = strtol(argv[1], NULL, 0);
    nelts = strtol(argv[2], NULL, 0);
    if (argc == 5) {
        ser_runs = strtol(argv[3], NULL, 0);
        deser_runs = strtol(argv[4], NULL, 0);
    }

    if (ser_runs <= 0) {
        ser_runs = DEFAULT_RUNS;
    }
    if (deser_runs <= 0) {
        deser_runs = DEFAULT_RUNS;
    }

    initialize_dummymsn();
    //test_serialize_leaf(valsize, nelts, entropy, ser_runs, deser_runs);
    //test_serialize_nonleaf(valsize, nelts, entropy, ser_runs, deser_runs);
    //test_serialize_nonleaf_one(valsize, nelts, entropy, ser_runs, deser_runs);
    test_serialize_nonleaf_two(valsize, nelts, entropy, ser_runs, deser_runs, 6);
    //test_serialize_nonleaf_two(valsize, nelts, entropy, ser_runs, deser_runs, 8);
	
    return 0;
}
