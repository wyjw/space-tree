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

/* Insert a bunch of stuff */
#include <toku_time.h>

#define TOKU_TEST_FILENAME "/media/nvme/swapfile"
static const char *fname = TOKU_TEST_FILENAME;

enum { SERIAL_SPACING = 1<<6 };
int64_t ITEMS_TO_INSERT_PER_ITERATION = 1<<20;
int64_t BOUND_INCREASE_PER_ITERATION = SERIAL_SPACING*ITEMS_TO_INSERT_PER_ITERATION;

enum { NODE_SIZE = 1<<20 };
enum { BASEMENT_NODE_SIZE = 128 * 1024 };

static int nodesize = NODE_SIZE;
static int basementnodesize = BASEMENT_NODE_SIZE;
static enum toku_compression_method compression_method = TOKU_DEFAULT_COMPRESSION_METHOD;
static int keysize = sizeof (long long);
static int valsize = sizeof (long long);
static int do_verify =0; /* Do a slow verify after every k inserts. */
static int verify_period = 256; /* how many inserts between verifies. */
static int cachesize = 4096*4096*10;

static int do_serial = 1;
static int do_random = 1;

static CACHETABLE ct;
static FT_HANDLE t;

static void setup (int cachesize) {
    int r;
    unlink(fname);
    toku_cachetable_create(&ct, cachesize, ZERO_LSN, nullptr);
    r = toku_open_ft_handle(fname, 1, &t, nodesize, basementnodesize, compression_method, ct, nullptr, toku_builtin_compare_fun); assert(r==0);
}

static void toku_shutdown (void) {
    int r;
    r = toku_close_ft_handle_nolsn(t, 0); assert(r==0);
    toku_cachetable_close(&ct);
}
static void long_long_to_array (unsigned char *a, unsigned long long l) {
    int i;
    for (i=0; i<8; i++)
	a[i] = (l>>(56-8*i))&0xff;
}

static void insert (long long v) {
    unsigned char kc[keysize], vc[valsize];
    DBT  kt, vt;
    memset(kc, 0, sizeof kc);
    long_long_to_array(kc, v);
    memset(vc, 0, sizeof vc);
    long_long_to_array(vc, v);
    toku_ft_insert(t, toku_fill_dbt(&kt, kc, keysize), toku_fill_dbt(&vt, vc, valsize), 0);
    if (do_verify) {
        static int inserts_since_last_verify = 0;
        inserts_since_last_verify++;
        if (inserts_since_last_verify % verify_period == 0) {
            toku_cachetable_verify(ct);
        }
    }
}

static void serial_insert_from (long long from) {
    long long i;
    for (i=0; i<ITEMS_TO_INSERT_PER_ITERATION; i++) {
	insert((from+i)*SERIAL_SPACING);
    }
}

static long long llrandom (void) {
    return (((long long)(random()))<<32) + random();
}

static void random_insert_below (long long below) {
    long long i;
    assert(0 < below);
    for (i=0; i<ITEMS_TO_INSERT_PER_ITERATION; i++) {
	insert(llrandom()%below);
    }
}

static void biginsert (long long n_elements, struct timeval *starttime) {
    long long i;
    struct timeval t1,t2;
    int iteration;
    for (i=0, iteration=0; i<n_elements; i+=ITEMS_TO_INSERT_PER_ITERATION, iteration++) {
	gettimeofday(&t1,0);
	if (do_serial)
            serial_insert_from(i);
	gettimeofday(&t2,0);
	if (verbose && do_serial) {
	    printf("serial %9.6fs %8.0f/s    ", toku_tdiff(&t2, &t1), ITEMS_TO_INSERT_PER_ITERATION/toku_tdiff(&t2, &t1));
	    fflush(stdout);
	}
	gettimeofday(&t1,0);
        if (do_random)
            random_insert_below((i+ITEMS_TO_INSERT_PER_ITERATION)*SERIAL_SPACING);
	gettimeofday(&t2,0);
	if (verbose && do_random) {
	    printf("random %9.6fs %8.0f/s    ", toku_tdiff(&t2, &t1), ITEMS_TO_INSERT_PER_ITERATION/toku_tdiff(&t2, &t1));
	    fflush(stdout);
        }
        if (verbose && (do_serial || do_random)) {
            double f = 0;
            if (do_serial) f += 1.0;
            if (do_random) f += 1.0;
	    printf("cumulative %9.6fs %8.0f/s\n", toku_tdiff(&t2, starttime), (ITEMS_TO_INSERT_PER_ITERATION*f/toku_tdiff(&t2, starttime))*(iteration+1));
	    fflush(stdout);
	}
    }
}

static void usage(void) {
    printf("benchmark-test [OPTIONS] [ITERATIONS]\n");
    printf("[-v]\n");
    printf("[-q]\n");
    printf("[--nodesize NODESIZE]\n");
    printf("[--keysize KEYSIZE]\n");
    printf("[--valsize VALSIZE]\n");
    printf("[--noserial]\n");
    printf("[--norandom]\n");
    printf("[--verify]\n");
    printf("[--verify_period PERIOD]\n");
}

int
test_main (int argc, const char *argv[]) {
    verbose=1; //Default
    /* parse parameters */
    int i;
    for (i=1; i<argc; i++) {
        const char *arg = argv[i];
        if (arg[0] != '-')
            break;
        if (strcmp(arg, "--nodesize") == 0) {
            if (i+1 < argc) {
                i++;
                nodesize = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--keysize") == 0) {
            if (i+1 < argc) {
                i++;
                keysize = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--periter") == 0) {
            if (i+1 < argc) {
                i++;
                ITEMS_TO_INSERT_PER_ITERATION = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--valsize") == 0) {
            if (i+1 < argc) {
                i++;
                valsize = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--verify")==0) {
	    do_verify = 1;
        } else if (strcmp(arg, "--verify_period")==0) {
            if (i+1 < argc) {
                i++;
                verify_period = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--noserial") == 0) {
            do_serial = 0;
        } else if (strcmp(arg, "--norandom") == 0) {
            do_random = 0;
	} else if (strcmp(arg, "-v")==0) {
	    verbose++;
	} else if (strcmp(arg, "-q")==0) {
	    verbose = 0;
	} else if (strcmp(arg, "-c")==0) {
	    if (i+1 < argc) {
		i++;
	    	cachesize =  atoi(argv[i]);
	    }
	} else {
	    usage();
	    return 1;
	}
    }
    fname = TOKU_TEST_FILENAME;

    struct timeval t1,t2,t3;
    long long total_n_items;
    if (i < argc) {
	char *end;
	errno=0;
	total_n_items = ITEMS_TO_INSERT_PER_ITERATION * (long long) strtol(argv[i], &end, 10);
	assert(errno==0);
	assert(*end==0);
	assert(end!=argv[i]);
    } else {
	total_n_items = 1LL<<22; // 1LL<<16
    }

    if (verbose) {
	printf("nodesize=%d\n", nodesize);
	printf("keysize=%d\n", keysize);
	printf("valsize=%d\n", valsize);
	printf("cachesize=%d\n", cachesize);
	printf("Serial and random insertions of %" PRId64 " per batch\n", ITEMS_TO_INSERT_PER_ITERATION);
        fflush(stdout);
    }
    printf("Filename is %s\n", fname);
    setup(cachesize);
    gettimeofday(&t1,0);
    biginsert(total_n_items, &t1);
    gettimeofday(&t2,0);
    toku_shutdown();
    gettimeofday(&t3,0);
    if (verbose) {
        int f = 0;
        if (do_serial) f += 1;
        if (do_random) f += 1;
	printf("Shutdown %9.6fs\n", toku_tdiff(&t3, &t2));
	printf("Total time %9.6fs for %lld insertions = %8.0f/s\n", toku_tdiff(&t3, &t1), f*total_n_items, f*total_n_items/toku_tdiff(&t3, &t1));
        fflush(stdout);
    }
    unlink(fname);

    return 0;
}


