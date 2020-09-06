#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include "test.h"
#include "helper.h"

static void
run_test(unsigned long eltsize, unsigned long nodesize, unsigned long repeat)
{
    int cur = 0;
    int r = 0;
    long *keys = (long *)toku_xmalloc(1024 * sizeof(long));
    char **vals = (char **)toku_xmalloc(1024 * sizeof(char*));
    long long unsigned nbytesinserted = 0;
    struct timeval t[2];
    XIDS xids_0;
    XIDS xids_123;
    NONLEAF_CHILDINFO bnc;
    long int i = 0;
    
    assert(keys);
    assert(vals);

    for (i = 0; i < 1024; ++i) {
        keys[i] = (long) rand();

        vals[i] = (char *) toku_xmalloc(eltsize - sizeof(keys[i]));
        assert(vals[i]);
        
        unsigned int long j = 0;
        char *val = vals[i];
        for (;j< eltsize - sizeof(keys[i]) - sizeof(int); j+=sizeof(int)) {
            int *p = (int *) &val[j];
            *p = (int) rand();
        }
        for (; j < eltsize - sizeof(keys[i]); ++j) {
            char *p = &val[j];
            *p =  (char) (rand() & 0xff);
        }  
    }

    xids_0 = xids_get_root_xids();
    r = xids_create_child(xids_0, &xids_123, (TXNID)123);
    CKERR(r);
    gettimeofday(&t[0], NULL);

    INT(r);	
    DBG;
    assert_zero(r);  

    for (unsigned int j = 0; j < repeat; ++j) {
        bnc = toku_create_empty_nl();
        for (; toku_bnc_nbytesinbuf(bnc) <= nodesize; ++cur) {
            FT_MSG_S msg;
            DBT key, val;
            toku_fill_dbt(&key, &keys[cur % 1024], sizeof keys[cur % 1024]);
            toku_fill_dbt(&val,vals[cur % 1024], eltsize - (sizeof keys[cur % 1024]));
            ft_msg_init(&msg, FT_NONE, next_dummymsn(), xids_123, &key, &val); 
            
            toku_bnc_insert_msg(bnc,
                                NULL,
                                &msg,
                                true,
                                NULL,
                                long_key_cmp); 
            assert_zero(r);
        }
        nbytesinserted += toku_bnc_nbytesinbuf(bnc);
        destroy_nonleaf_childinfo(bnc);
    }
    

    for (i=0; i< 1024; i++) {
	if(vals[i] != NULL) 
		toku_free(vals[i]);
    } 
    toku_free(vals);
    toku_free(keys); 
}

int main(){
    unsigned long eltsize, nodesize, repeat;

    initialize_dummymsn();
    eltsize = 2048;
    nodesize = 2048;
    repeat = 100;

    run_test(eltsize, nodesize, repeat);
}