#ifndef DBIN_H
#define DBIN_H

#include <stdint.h>
#include <cstddef>
#include <stdio.h>
#include <cstring>

// node stuff
// preliminaries for node
typedef struct _msn { uint64_t msn; } _MSN;
typedef struct _blocknum_s { int64_t b; } _BLOCKNUM;
typedef uint64_t _TXNID;
typedef _BLOCKNUM _CACHEKEY;
struct _ftnode_partition;
struct _dbt;
struct __db_dbt;
struct __toku_dbt;
typedef struct __toku_dbt DBT;
struct _dbt{
	void *data;
	uint32_t size;
	uint32_t ulen;
	uint32_t flags;
};
struct cachefile;
typedef struct cachefile *CACHEFILE;
struct __toku_db;
typedef struct __toku_db DB;

#define RBUFDEF 1
struct rbuf {
    unsigned char *buf;
    unsigned int  size;
    unsigned int  ndone;
};
#define RBUF_INITIALIZER ((struct rbuf){.buf = NULL, .size=0, .ndone=0})
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
typedef char _Bytef;
typedef unsigned long int _uLongf;

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

#ifndef RBUF
// rbuf methods
static unsigned int _rbuf_int (struct rbuf *r) {
    //assert(r->ndone+4 <= r->size);
    uint32_t result = (*(uint32_t*)(r->buf+r->ndone));
    r->ndone+=4;
    return result;
}

static inline void _rbuf_literal_bytes (struct rbuf *r, const void **bytes, unsigned int n_bytes) {
    *bytes =   &r->buf[r->ndone];
    r->ndone+=n_bytes;
    //assert(r->ndone<=r->size);
}
#endif

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

struct _ftnode_nonleaf_childinfo;

struct _ftnode_child_pointer {
	union {
		struct _sub_block *subblock;
		struct _ftnode_nonleaf_childinfo *nonleaf;
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


// search structs
enum _ft_search_direction_e {
	_FT_SEARCH_LEFT = 1,
	_FT_SEARCH_RIGHT = 2,
};
struct _ft_search;

typedef int (*_ft_search_compare_func_t)(const struct _ft_search &, const struct _dbt *);

struct _ft_search {
	_ft_search_compare_func_t compare;
	enum _ft_search_direction_e direction;
	const struct _dbt *k;
	void *context;
	struct _dbt pivot_bound;
	const struct _dbt *k_bound;
};

/*
 * Function declarations begin here:
 */
struct _dbt *_fill_pivot(_pivot_keys* pk, int a, _dbt* k);
long int ftnode_cachepressure_size_cutdown(struct _ftnode* ftn);
_PAIR_ATTR make_ftnode_pair_cutdown(struct _ftnode* ftn);
void _convert_dbt_to_tokudbt(DBT *keya, struct _dbt* keyb);
void _convert_tokudbt_to_dbt(struct _dbt* keya, DBT *keyb);
_PAIR_ATTR make_ftnode_pair_attr_cutdown(struct _ftnode *node);
uint32_t toku_cachetable_hash_cutdown(CACHEFILE cf, _BLOCKNUM bn);

/*
 *
 * Functions are defined here
 */
struct _dbt *_fill_pivot(_pivot_keys *pk, int i, struct _dbt *a) {
	a->data = pk->_dbt_keys[i].data;
	a->size = pk->_dbt_keys[i].size;
	a->ulen = pk->_dbt_keys[i].ulen;
	a->flags = pk->_dbt_keys[i].flags;
	return a;
}

struct _dbt *_get_pivot(_pivot_keys *pk, int i) {
	// unless fixed format
	return &pk->_dbt_keys[i];
}

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

typedef int (*_compare_func)(struct _dbt *a, struct _dbt *b); 
typedef int (*_old_compare_func)(DB *db, const DBT *a, const DBT *b);

struct _dbt *_init_dbt(struct _dbt *dbt)
{
	memset(dbt, 0, sizeof(*dbt));
	return dbt;
}

struct _comparator{
	_compare_func _cmp;
	uint8_t _memcpy_magic;
};

/*void _convert_dbt_to_tokudbt(DBT *a, struct _dbt *b) {
	a->data = b->data;
	a->size = b->size;
	a->ulen= b->ulen;
	a->flags = b->flags;
}

void _convert_tokudbt_to_dbt(struct _dbt *a, DBT *b) {
	a->data = b->data;
	a->size = b->size;
	a->ulen = b->ulen;
	a->flags = b->flags;
}
*/

typedef uint64_t _TXNID;
struct __attribute__((__packed__)) _XIDS_S {
	uint8_t num_xids;
	_TXNID ids[];
};
typedef struct _XIDS_S *_XIDS;

struct __attribute__((__packed__)) buffer_entry {
	unsigned int keylen;
	unsigned int vallen;
	unsigned char type;
	bool is_fresh;
	_MSN msn;	 
};

struct _ftnode_nonleaf_childinfo {
	int msg_buffer;
	// all these are broken
	int broadcast_list;
	int fresh_message_tree;
       	int stale_message_tree;
	uint64_t flow[2];	
};

static inline struct _ftnode_nonleaf_childinfo _BNC(struct _ftnode* node, int i) {
	struct _ftnode_child_pointer fcptr = node->bp[i].ptr;
	return *fcptr.u.nonleaf; 
}

static int ft_compare_pivot_cutdown(const struct _comparator &cmp, _dbt *key,_dbt *pivot) {
    return cmp._cmp(key, pivot);
}

int toku_ftnode_which_child_cutdown(struct _ftnode *node, struct _dbt *k, struct _comparator &cmp);

int toku_ftnode_which_child_cutdown(struct _ftnode *node, struct _dbt *k, struct _comparator &cmp) {
    // a funny case of no pivots
    if (node->n_children <= 1) return 0;

    struct _dbt pivot;

    // check the last key to optimize seq insertions
    int n = node->n_children-1;
    int c = ft_compare_pivot_cutdown(cmp, k, _fill_pivot(node->pivotkeys, n - 1, &pivot));
    if (c > 0) return n;

    // binary search the pivots
    int lo = 0;
    int hi = n-1; // skip the last one, we checked it above
    int mi;
    while (lo < hi) {
        mi = (lo + hi) / 2;
        c = ft_compare_pivot_cutdown(cmp, k, _fill_pivot(node->pivotkeys, mi, &pivot));
        if (c > 0) {
            lo = mi+1;
            continue;
        }
        if (c < 0) {
            hi = mi;
            continue;
        }
        return mi;
    }
    return lo;
}

// compression declarations 
int read_compressed_sub_block_cutdown(struct rbuf *rb, struct _sub_block *sb);
int read_and_decompress_sub_block_cutdown(struct rbuf *rb, struct _sub_block *sb);
void just_decompress_sub_block_cutdown(struct _sub_block *sb);
void decompress_cutdown (_Bytef *dest, _uLongf destLen, const _Bytef *source, _uLongf sourceLen);

int read_compressed_sub_block_cutdown(struct rbuf *rb, struct _sub_block *sb)
{
	int r = 0;
	sb->compressed_size = _rbuf_int(rb);
	sb->uncompressed_size = _rbuf_int(rb);
	const void **cp = (const void **) &sb->compressed_ptr;
	_rbuf_literal_bytes(rb, cp, sb->compressed_size);
	sb->xsum = _rbuf_int(rb);
	
	// decompress; only no compression
	sb->uncompressed_ptr = _mmalloc(sb->uncompressed_size);
	memcpy(sb->uncompressed_ptr, sb->compressed_ptr + 1, sb->compressed_size -1);

	return r;
}

/*
int
read_compressed_sub_block_cutdown(struct rbuf *rb, struct _sub_block *sb)
{
    int r = 0;
    sb->compressed_size = _rbuf_int(rb);
    sb->uncompressed_size = _rbuf_int(rb);
    const void **cp = (const void **) &sb->compressed_ptr;
    _rbuf_literal_bytes(rb, cp, sb->compressed_size);
    sb->xsum = _rbuf_int(rb);
    return r;
}
*/

int read_and_decompress_sub_block_cutdown(struct rbuf *rb, struct _sub_block *sb)
{
    int r = 0;
    r = read_compressed_sub_block_cutdown(rb, sb);
    if (r != 0) {
        goto exit;
    }
exit:
    return r;
}

void just_decompress_sub_block_cutdown(struct _sub_block *sb)
{
    // <CER> TODO: Add assert that the subblock was read in.
    sb->uncompressed_ptr = _mmalloc(sb->uncompressed_size);

    decompress_cutdown(
        (_Bytef *) sb->uncompressed_ptr,
        sb->uncompressed_size,
        (_Bytef *) sb->compressed_ptr,
        sb->compressed_size
        );
}

void decompress_cutdown (_Bytef       *dest,   _uLongf destLen,
                      const _Bytef *source, _uLongf sourceLen)
{
    //assert(sourceLen>=1);
    memcpy(dest, source + 1, sourceLen - 1);
    return;
}

//inline void set_BNC_cutdown(struct _ftnode *node, int i, 
#endif /* DBIN_H */
