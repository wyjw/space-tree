#ifndef DBIN_H
#define DBIN_H

#include <stdint.h>
#include <cstddef>
#include <stdio.h>
#include <cstring>

#define printk printf

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
    _pivot_keys pivotkeys;
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
//int _read_compressed_sub_block(struct rbuf *rb, struct _sub_block *sb);
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

struct _ftnode_nonleaf_childinfo;

typedef struct _ftnode_child_pointer {
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
struct _dbt *_get_pivot(_pivot_keys *pk, int a);
void _create_empty_pivot(_pivot_keys *pk);
void deserialize_from_rbuf_cutdown(_pivot_keys *pk, struct rbuf *rb, int n);

typedef int (*_compare_func)(struct _dbt *a, struct _dbt *b); 
typedef int (*_old_compare_func)(DB *db, const DBT *a, const DBT *b);
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

// compression declarations 
int read_compressed_sub_block_cutdown(struct rbuf *rb, struct _sub_block *sb);
int read_and_decompress_sub_block_cutdown(struct rbuf *rb, struct _sub_block *sb);
void just_decompress_sub_block_cutdown(struct _sub_block *sb);
void decompress_cutdown (_Bytef *dest, _uLongf destLen, const _Bytef *source, _uLongf sourceLen);
void dump_ftnode_cutdown(struct _ftnode *nd);
//inline void set_BNC_cutdown(struct _ftnode *node, int i, 
#endif /* DBIN_H */
