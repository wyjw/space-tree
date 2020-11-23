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

// Each FT maintains a sequential insert heuristic to determine if its
// worth trying to insert directly into a well-known rightmost leaf node.
//
// The heuristic is only maintained when a rightmost leaf node is known.
//
// This test verifies that sequential inserts increase the seqinsert score
// and that a single non-sequential insert resets the score.

static void test_inserts(void) {
    int r = 0;
    CACHETABLE ct;
    toku_cachetable_create(&ct, 0, ZERO_LSN, nullptr);
    FT_HANDLE ft_handle;
    r = btoku_setup(TOKU_TEST_FILENAME, 1, &ft_handle, 4*1024*1024, 64*1024, TOKU_NO_COMPRESSION_METHOD, ct, NULL, toku_builtin_compare_fun);    
    CKERR(r);

    int k;
    DBT key, val;
    const int val_size = 1024 * 1024;
    char *XMALLOC_N(val_size, val_buf);
    memset(val_buf, 'x', val_size);
    toku_fill_dbt(&val, val_buf, val_size);

    // Insert many rows sequentially. This is enough data to:
    // - force the root to split (the righmost leaf will then be known)
    // - raise the seqinsert score high enough to enable direct rightmost injections
    const int rows_to_insert = 200;
    for (int i = 0; i < rows_to_insert; i++) {
        k = toku_htonl(i);
        toku_fill_dbt(&key, &k, sizeof(k));
        toku_ft_insert(ft_handle, &key, &val, NULL);
    }
    invariant(ft->rightmost_blocknum.b != RESERVED_BLOCKNUM_NULL);
    invariant(ft->seqinsert_score == FT_SEQINSERT_SCORE_THRESHOLD);  	  

    toku_free(val_buf);
    toku_ft_handle_close(ft_handle);
    toku_cachetable_close(&ct);
    toku_os_recursive_delete(TOKU_TEST_FILENAME);
}

int test_main(int argc, const char *argv[]) {
    default_parse_args(argc, argv);
    test_inserts();
    return 0;
}
