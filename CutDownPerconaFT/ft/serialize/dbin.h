#ifndef DBIN_H
#define DBIN_H 

void *_mmalloc(int size);

struct _sub_block;
void _sub_block_init(struct _sub_block *sb);
int _read_compressed_sub_block(struct rbuf *rb, struct _sub_block *sb);
// int _deserialize_ftnode_info(struct _sub_block *sb, FTNODE *ftnode);
struct _sbt;
struct _dbt *_init_dbt(struct _dbt *dbt);
struct _comparator;
#endif /* DBIN_H */
