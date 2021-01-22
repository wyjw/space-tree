#include "ft/serialize/dbin.h"

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

struct _dbt *_init_dbt(struct _dbt *dbt)
{
	memset(dbt, 0, sizeof(*dbt));
	return dbt;
}


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

struct _dbt *_get_pivot(_pivot_keys *pk, int a) {
	return pk->_dbt_keys[a];
}

