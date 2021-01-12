#ifndef DBIN_H
#define DBIN_H

#include <stdint.h>
#include <cstddef>

// node stuff
// preliminaries for node
typedef struct _msn { uint64_t msn; } _MSN;
typedef struct _blocknum_s { int64_t b; } _BLOCKNUM;
typedef uint64_t _TXNID;
typedef _BLOCKNUM _CACHEKEY;
struct _ftnode_partition;

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
    _TXNID oldest_referenced_xid_known;

    struct _ftnode_partition *bp;
    struct ctpair *ct_pair;
};

void *_mmalloc(int size);
struct _sub_block {
	void *uncompressed_ptr;
	uint32_t uncompressed_size;
	void *compressed_ptr;
	uint32_t compressed_size;
	uint32_t compressed_size_bound;
	uint32_t xsum;
};
void sub_block_init_cutdown(struct _sub_block *sb);
int _read_compressed_sub_block(struct rbuf *rb, struct _sub_block *sb);
int deserialize_ftnode_info_cutdown(struct _sub_block *sb, struct _ftnode *ftnode);
struct _sbt;
struct _dbt *_init_dbt(struct _dbt *dbt);
struct _comparator;

// PAIR STUFF
struct _klpair_struct {
	uint32_t le_offset;
	uint8_t key[0];
};

enum _pt_state {
	_PT_INVALID = 0,
	_PT_ON_DISK = 1,
	_PT_COMPRESSED = 2,
	_PT_AVAIL = 3
};

/*
struct _ftnode_leaf_basement_node {
	bn_data data_buffer;
};
*/

enum _ftnode_child_tag {
	_BCT_INVALID = 0,
	_BCT_NULL,
	_BCT_SUBBLOCK,
	_BCT_LEAF,
	_BCT_NONLEAF
};

#define BP_START(node_dd,i) ((node_dd)[i].start)
#define BP_SIZE(node_dd,i) ((node_dd)[i].size)
#define BP_BLOCKNUM(node,i) ((node)->bp[i].blocknum)
#define BP_STATE(node,i) ((node)->bp[i].state)
#define BP_WORKDONE(node, i)((node)->bp[i].workdone)
#define BP_TOUCH_CLOCK(node, i) ((node)->bp[i].clock_count = 1)
#define BP_SWEEP_CLOCK(node, i) ((node)->bp[i].clock_count = 0)
#define BP_SHOULD_EVICT(node, i) ((node)->bp[i].clock_count == 0)
#define BP_INIT_TOUCHED_CLOCK(node, i) ((node)->bp[i].clock_count = 1)
#define BP_INIT_UNTOUCHED_CLOCK(node, i) ((node)->bp[i].clock_count = 0)
#define BLB_MAX_MSN_APPLIED(node,i) (BLB(node,i)->max_msn_applied)
#define BLB_MAX_DSN_APPLIED(node,i) (BLB(node,i)->max_dsn_applied)
#define BLB_DATA(node,i) (&(BLB(node,i)->data_buffer))
#define BLB_NBYTESINDATA(node,i) (BLB_DATA(node,i)->get_disk_size())
#define BLB_SEQINSERT(node,i) (BLB(node,i)->seqinsert)
#define BLB_LRD(node, i) (BLB(node,i)->logical_rows_delta)

struct _ftnode_child_pointer {
	union {
		struct _sub_block *subblock;
		struct ftnode_nonleaf_childinfo *nonleaf;
		struct ftnode_leaf_basement_node *leaf;
	} u;
	enum _ftnode_child_tag tag;
} _FTNODE_CHILD_POINTER;


struct _ftnode_partition {
    _BLOCKNUM     blocknum; // blocknum of child 
    uint64_t     workdone;
    struct _ftnode_child_pointer ptr;
    enum _pt_state state; // make this an enum to make debugging easier.  
    uint8_t clock_count;
};

static long
ftnode_memory_size_cutdown(struct _ftnode *node)
// Effect: Estimate how much main memory a node requires.
{
    long retval = 0;
    int n_children = node->n_children;
    retval += sizeof(*node);
    retval += (n_children)*(sizeof(node->bp[0]));
    retval += node->pivotkeys->_total_size;

    for (int i = 0; i < n_children; i++) {
    	//struct _sub_block *sb = BSB(node, i);
    	struct _sub_block *sb = node->bp[i].ptr.u.subblock;
    	retval += sizeof(*sb);
    	retval += sb->compressed_size;
    }
    /*
    // now calculate the sizes of the partitions
    for (int i = 0; i < n_children; i++) {
        if (BP_STATE(node,i) == PT_INVALID || BP_STATE(node,i) == PT_ON_DISK) {
            continue;
        }
        else if (BP_STATE(node,i) == PT_COMPRESSED) {
            struct _sub_block *sb = BSB(node, i);
            retval += sizeof(*sb);
            retval += sb->compressed_size;
        }
        else if (BP_STATE(node,i) == PT_AVAIL) {
            if (node->height > 0) {
                retval += get_avail_internal_node_partition_size(node, i);
            }
            else {
                BASEMENTNODE bn = BLB(node, i);
                retval += sizeof(*bn);
                retval += BLB_DATA(node, i)->get_memory_size();
            }
        }
        else {
            abort();
        }
    }
    */
    return retval;
}

long ftnode_cachepressure_size_cutdown(struct _ftnode *node) {
    long retval = 0;
    bool totally_empty = true;
    if (node->height == 0) {
        goto exit;
    }
    else {
        for (int i = 0; i < node->n_children; i++) {
    		struct _sub_block *sb = node->bp[i].ptr.u.subblock;
                totally_empty = false;
                retval += sb->compressed_size;
        }
    }
exit:
    if (totally_empty) {
        return 0;
    }
    return retval;
}

struct _ancestors {
	struct _ftnode *node;
	int childnum;
	struct _ancestors *next;
};

typedef struct _pair_attr_s {
	long size;
	long nonleaf_size;
	long leaf_size;
	long rollback_size;
	long cache_pressure_size;
	bool is_valid;
} _PAIR_ATTR;

_PAIR_ATTR make_ftnode_pair_attr_cutdown(struct _ftnode *node) {
    long size = ftnode_memory_size_cutdown(node);
    long cachepressure_size = ftnode_cachepressure_size_cutdown(node);
    _PAIR_ATTR result={
        .size = size,
        .nonleaf_size = (node->height > 0) ? size : 0,
        .leaf_size = (node->height > 0) ? 0 : size,
        .rollback_size = 0,
        .cache_pressure_size = cachepressure_size,
        .is_valid = true
    };
    return result;
}

//inline void set_BNC_cutdown(struct _ftnode *node, int i, 
#endif /* DBIN_H */
