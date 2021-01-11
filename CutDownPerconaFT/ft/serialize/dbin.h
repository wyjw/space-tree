#ifndef DBIN_H
#define DBIN_H

#include <stdint.h>
#include <cstddef>

// node stuff
// preliminaries for node
typedef struct _msn { uint64_t msn; } _MSN;
typedef struct _blocknum_s { int64_t b; } _BLOCKNUM;
typedef uint64_t TXNID;

// pivot_keys
typedef struct _int_pivot_keys {
	char *_fixed_keys;
	size_t _fixed_keylen;
	size_t _fixed_keylen_aligned;
	struct _dbt *_dbt_keys;
	int _num_pivots;
	size_t _total_size;
} _pivot_keys;
 
struct _ftnode {
    _MSN max_msn_applied_to_node_on_disk;
    unsigned int flags;
    _BLOCKNUM blocknum;
    int layout_version;
    int layout_version_original;
    int layout_version_read_from_disk;
    uint32_t build_id;
    int height;
    int dirty_;
    uint32_t fullhash;
    void set_dirty() {
        dirty_ = 1;
    }
    void clear_dirty() {
        dirty_ = 0;
    }
    bool dirty() {
        return dirty_;
    }
    int n_children;
    _pivot_keys *pivotkeys;
    TXNID oldest_referenced_xid_known;

    struct ftnode_partition *bp;
    struct ctpair *ct_pair;
};

void *_mmalloc(int size);

struct _sub_block;
void _sub_block_init(struct _sub_block *sb);
int _read_compressed_sub_block(struct rbuf *rb, struct _sub_block *sb);
// int _deserialize_ftnode_info(struct _sub_block *sb, FTNODE *ftnode);
struct _sbt;
struct _dbt *_init_dbt(struct _dbt *dbt);
struct _comparator;
#endif /* DBIN_H */
