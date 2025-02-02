/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
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

#include "ft/serialize/block_table.h"
#include "ft/ft-cachetable-wrappers.h"
#include "ft/ft-flusher.h"
#include "ft/ft-internal.h"
#include "ft/ft.h"
#include "ft/node.h"
#include "ft/serialize/dbin.h"

#include <util/context.h>

static PAIR cast_from__ctpair(struct _ctpair *cp) {
	return (struct ctpair *)&(*cp);
}

static void
ftnode_get_key_and_fullhash(
    BLOCKNUM* cachekey,
    uint32_t* fullhash,
    void* extra)
{
    FT ft = (FT) extra;
    BLOCKNUM blocknum;
    ft->blocktable.allocate_blocknum(&blocknum, ft);
    *cachekey = blocknum;
    *fullhash = toku_cachetable_hash(ft->cf, blocknum);
}

void
cachetable_put_empty_node_with_dep_nodes(
    FT ft,
    uint32_t num_dependent_nodes,
    FTNODE* dependent_nodes,
    BLOCKNUM* blocknum, //output
    uint32_t* fullhash, //output
    FTNODE* result)
{
    FTNODE XCALLOC(new_node);
    PAIR dependent_pairs[num_dependent_nodes];
    enum cachetable_dirty dependent_dirty_bits[num_dependent_nodes];
    for (uint32_t i = 0; i < num_dependent_nodes; i++) {
        dependent_pairs[i] = dependent_nodes[i]->ct_pair;
        dependent_dirty_bits[i] = (enum cachetable_dirty) dependent_nodes[i]->dirty();
    }

    toku_cachetable_put_with_dep_pairs(
        ft->cf,
        ftnode_get_key_and_fullhash,
        new_node,
        make_pair_attr(sizeof(FTNODE)),
        get_write_callbacks_for_node(ft),
        ft,
        num_dependent_nodes,
        dependent_pairs,
        dependent_dirty_bits,
        blocknum,
        fullhash,
        toku_ftnode_save_ct_pair);
    *result = new_node;
}

void
create_new_ftnode_with_dep_nodes(
    FT ft,
    FTNODE *result,
    int height,
    int n_children,
    uint32_t num_dependent_nodes,
    FTNODE* dependent_nodes)
{
    uint32_t fullhash = 0;
    BLOCKNUM blocknum;

    cachetable_put_empty_node_with_dep_nodes(
        ft,
        num_dependent_nodes,
        dependent_nodes,
        &blocknum,
        &fullhash,
        result);

    assert(ft->h->basementnodesize > 0);
    if (height == 0) {
        assert(n_children > 0);
    }

    toku_initialize_empty_ftnode(
        *result,
        blocknum,
        height,
        n_children,
        ft->h->layout_version,
        ft->h->flags);

    (*result)->fullhash = fullhash;
}

void
toku_create_new_ftnode (
    FT_HANDLE t,
    FTNODE *result,
    int height,
    int n_children)
{
    return create_new_ftnode_with_dep_nodes(
        t->ft,
        result,
        height,
        n_children,
        0,
        NULL);
}

//
// On success, this function assumes that the caller is trying to pin the node
// with a PL_READ lock. If message application is needed,
// then a PL_WRITE_CHEAP lock is grabbed
//
int
toku_pin_ftnode_for_query(
    FT_HANDLE ft_handle,
    BLOCKNUM blocknum,
    uint32_t fullhash,
    UNLOCKERS unlockers,
    ANCESTORS ancestors,
    const pivot_bounds &bounds,
    ftnode_fetch_extra *bfe,
    bool apply_ancestor_messages, // this bool is probably temporary, for #3972, once we know how range query estimates work, will revisit this
    FTNODE *node_p,
    bool* msgs_applied)
{
    void *node_v;
    *msgs_applied = false;
    FTNODE node = nullptr;
    MSN max_msn_in_path = ZERO_MSN;
    bool needs_ancestors_messages = false;
    // this function assumes that if you want ancestor messages applied,
    // you are doing a read for a query. This is so we can make some optimizations
    // below.
    if (apply_ancestor_messages) {
        paranoid_invariant(bfe->type == ftnode_fetch_subset);
    }
    
    int r = toku_cachetable_get_and_pin_nonblocking(
            ft_handle->ft->cf,
            blocknum,
            fullhash,
            &node_v,
            get_write_callbacks_for_node(ft_handle->ft),
            toku_ftnode_fetch_callback,
            toku_ftnode_pf_req_callback,
            toku_ftnode_pf_callback,
            PL_READ,
            bfe, //read_extraargs
            unlockers);
    if (r != 0) {
        assert(r == TOKUDB_TRY_AGAIN); // Any other error and we should bomb out ASAP.
        goto exit;
    }
    node = static_cast<FTNODE>(node_v);
    if (apply_ancestor_messages && node->height == 0) {
        needs_ancestors_messages = toku_ft_leaf_needs_ancestors_messages(
            ft_handle->ft, 
            node, 
            ancestors, 
            bounds, 
            &max_msn_in_path, 
            bfe->child_to_read
            );
        if (needs_ancestors_messages) {
            toku::context apply_messages_ctx(CTX_MESSAGE_APPLICATION);

            toku_unpin_ftnode_read_only(ft_handle->ft, node);
            int rr = toku_cachetable_get_and_pin_nonblocking(
                 ft_handle->ft->cf,
                 blocknum,
                 fullhash,
                 &node_v,
                 get_write_callbacks_for_node(ft_handle->ft),
                 toku_ftnode_fetch_callback,
                 toku_ftnode_pf_req_callback,
                 toku_ftnode_pf_callback,
                 PL_WRITE_CHEAP,
                 bfe, //read_extraargs
                 unlockers);
            if (rr != 0) {
                assert(rr == TOKUDB_TRY_AGAIN); // Any other error and we should bomb out ASAP.
                r = TOKUDB_TRY_AGAIN;
                goto exit;
            }
            node = static_cast<FTNODE>(node_v);
            toku_apply_ancestors_messages_to_node(
                ft_handle, 
                node, 
                ancestors, 
                bounds, 
                msgs_applied,
                bfe->child_to_read
                );
        } else {
            // At this point, we aren't going to run
            // toku_apply_ancestors_messages_to_node but that doesn't
            // mean max_msn_applied shouldn't be updated if possible
            // (this saves the CPU work involved in
            // toku_ft_leaf_needs_ancestors_messages).
            //
            // We still have a read lock, so we have not resolved
            // checkpointing.  If the node is pending and dirty, we
            // can't modify anything, including max_msn, until we
            // resolve checkpointing.  If we do, the node might get
            // written out that way as part of a checkpoint with a
            // root that was already written out with a smaller
            // max_msn.  During recovery, we would then inject a
            // message based on the root's max_msn, and that message
            // would get filtered by the leaf because it had too high
            // a max_msn value. (see #5407)
            //
            // So for simplicity we only update the max_msn if the
            // node is clean.  That way, in order for the node to get
            // written out, it would have to be dirtied.  That
            // requires a write lock, and a write lock requires you to
            // resolve checkpointing.
            if (!node->dirty()) {
                toku_ft_bn_update_max_msn(node, max_msn_in_path, bfe->child_to_read);
            }
        }
    }
    *node_p = node;
exit:
    return r;
}

int
toku_pin_ftnode_for_query(
    FT_HANDLE ft_handle,
    _BLOCKNUM blocknum,
    uint32_t fullhash,
    UNLOCKERS unlockers,
    struct _ancestors *ancestors,
    const pivot_bounds &bounds,
    ftnode_fetch_extra *bfe,
    bool apply_ancestor_messages, // this bool is probably temporary, for #3972, once we know how range query estimates work, will revisit this
    struct _ftnode **node_p,
    bool* msgs_applied)
{
    void *node_v;
    *msgs_applied = false;
    struct _ftnode* node = NULL;
    MSN max_msn_in_path = ZERO_MSN;
    bool needs_ancestors_messages = false;
    // this function assumes that if you want ancestor messages applied,
    // you are doing a read for a query. This is so we can make some optimizations
    // below.
    if (apply_ancestor_messages) {
        paranoid_invariant(bfe->type == ftnode_fetch_subset);
    }
    
    int r = toku_cachetable_get_and_pin_nonblocking(
            ft_handle->ft->cf,
	    // unsan
            *(BLOCKNUM *)&blocknum,
            fullhash,
            &node_v,
            get_write_callbacks_for_node(ft_handle->ft),
            toku_ftnode_fetch_callback,
            toku_ftnode_pf_req_callback,
            toku_ftnode_pf_callback,
            PL_READ,
            bfe, //read_extraargs
            unlockers);
    if (r != 0) {
        assert(r == TOKUDB_TRY_AGAIN); // Any other error and we should bomb out ASAP.
        goto exit;
    }
    //node = static_cast<FTNODE>(node_v);
    node = static_cast<struct _ftnode*>(node_v);
    /*
    if (apply_ancestor_messages && node->height == 0) {
        needs_ancestors_messages = toku_ft_leaf_needs_ancestors_messages(
            ft_handle->ft, 
            node,
	    // unsan 
            *(ANCESTORS *)&ancestors, 
            bounds, 
            &max_msn_in_path, 
            bfe->child_to_read
            );
        if (needs_ancestors_messages) {
            toku::context apply_messages_ctx(CTX_MESSAGE_APPLICATION);

            toku_unpin_ftnode_read_only(ft_handle->ft, node);
            int rr = toku_cachetable_get_and_pin_nonblocking(
                 ft_handle->ft->cf,
                 *(BLOCKNUM *)&blocknum,
                 fullhash,
                 &node_v,
                 get_write_callbacks_for_node(ft_handle->ft),
                 toku_ftnode_fetch_callback,
                 toku_ftnode_pf_req_callback,
                 toku_ftnode_pf_callback,
                 PL_WRITE_CHEAP,
                 bfe, //read_extraargs
                 unlockers);
            if (rr != 0) {
                assert(rr == TOKUDB_TRY_AGAIN); // Any other error and we should bomb out ASAP.
                r = TOKUDB_TRY_AGAIN;
                goto exit;
            }
            node = static_cast<FTNODE>(node_v);
            toku_apply_ancestors_messages_to_node(
                ft_handle, 
                node, 
                ancestors, 
                bounds, 
                msgs_applied,
                bfe->child_to_read
                );
        } else {
            // At this point, we aren't going to run
            // toku_apply_ancestors_messages_to_node but that doesn't
            // mean max_msn_applied shouldn't be updated if possible
            // (this saves the CPU work involved in
            // toku_ft_leaf_needs_ancestors_messages).
            //
            // We still have a read lock, so we have not resolved
            // checkpointing.  If the node is pending and dirty, we
            // can't modify anything, including max_msn, until we
            // resolve checkpointing.  If we do, the node might get
            // written out that way as part of a checkpoint with a
            // root that was already written out with a smaller
            // max_msn.  During recovery, we would then inject a
            // message based on the root's max_msn, and that message
            // would get filtered by the leaf because it had too high
            // a max_msn value. (see #5407)
            //
            // So for simplicity we only update the max_msn if the
            // node is clean.  That way, in order for the node to get
            // written out, it would have to be dirtied.  That
            // requires a write lock, and a write lock requires you to
            // resolve checkpointing.
            if (!node->dirty()) {
                toku_ft_bn_update_max_msn(node, max_msn_in_path, bfe->child_to_read);
            }
        }
    }
    */
    *node_p = node;
exit:
    return r;
}

void
toku_pin_ftnode_with_dep_nodes(
    FT ft,
    BLOCKNUM blocknum,
    uint32_t fullhash,
    ftnode_fetch_extra *bfe,
    pair_lock_type lock_type,
    uint32_t num_dependent_nodes,
    FTNODE *dependent_nodes,
    FTNODE *node_p,
    bool move_messages)
{
    void *node_v;
    PAIR dependent_pairs[num_dependent_nodes];
    enum cachetable_dirty dependent_dirty_bits[num_dependent_nodes];
    for (uint32_t i = 0; i < num_dependent_nodes; i++) {
        dependent_pairs[i] = dependent_nodes[i]->ct_pair;
        dependent_dirty_bits[i] = (enum cachetable_dirty) dependent_nodes[i]->dirty();
    }

    int r = toku_cachetable_get_and_pin_with_dep_pairs(
        ft->cf,
        blocknum,
        fullhash,
        &node_v,
        get_write_callbacks_for_node(ft),
        toku_ftnode_fetch_callback,
        toku_ftnode_pf_req_callback,
        toku_ftnode_pf_callback,
        lock_type,
        bfe,
        num_dependent_nodes,
        dependent_pairs,
        dependent_dirty_bits
        );
    invariant_zero(r);
    FTNODE node = (FTNODE) node_v;
    if (lock_type != PL_READ && node->height > 0 && move_messages) {
        toku_move_ftnode_messages_to_stale(ft, node);
    }
    *node_p = node;
}

void toku_pin_ftnode(FT ft,
                     BLOCKNUM blocknum,
                     uint32_t fullhash,
                     ftnode_fetch_extra *bfe,
                     pair_lock_type lock_type,
                     FTNODE *node_p,
                     bool move_messages) {
    toku_pin_ftnode_with_dep_nodes(ft, blocknum, fullhash, bfe, lock_type, 0, nullptr, node_p, move_messages);
}

void
toku_pin_ftnode_with_dep_nodes_cutdown(
    FT ft,
    _BLOCKNUM blocknum,
    uint32_t fullhash,
    ftnode_fetch_extra *bfe,
    pair_lock_type lock_type,
    uint32_t num_dependent_nodes,
    struct _ftnode **dependent_nodes,
    struct _ftnode **node_p,
    bool move_messages)
{
    void *node_v;
    PAIR dependent_pairs[num_dependent_nodes];
    enum cachetable_dirty dependent_dirty_bits[num_dependent_nodes];
    for (uint32_t i = 0; i < num_dependent_nodes; i++) {
        dependent_pairs[i] = (PAIR)&dependent_nodes[i]->ct_pair;
        dependent_dirty_bits[i] = (enum cachetable_dirty) dependent_nodes[i]->dirty();
    }

    int r = toku_cachetable_get_and_pin_with_dep_pairs_cutdown(
        ft->cf,
        blocknum,
        fullhash,
        &node_v,
        get_write_callbacks_for_node(ft),
        toku_ftnode_fetch_callback,
        toku_ftnode_pf_req_callback,
        toku_ftnode_pf_callback,
        lock_type,
        bfe,
        num_dependent_nodes,
        dependent_pairs,
        dependent_dirty_bits
        );
    invariant_zero(r);
    struct _ftnode *node = (struct _ftnode *) node_v;
    /*
    if (lock_type != PL_READ && node->height > 0 && move_messages) {
        toku_move_ftnode_messages_to_stale(ft, node);
    }
    */
    *node_p = node;
}

void toku_pin_ftnode_cutdown(FT ft,
                     _BLOCKNUM blocknum,
                     uint32_t fullhash,
                     ftnode_fetch_extra *bfe,
                     pair_lock_type lock_type,
                     struct _ftnode **node_p,
                     bool move_messages) {
    toku_pin_ftnode_with_dep_nodes_cutdown(ft, blocknum, fullhash, bfe, lock_type, 0, nullptr, node_p, move_messages);
}

int toku_maybe_pin_ftnode_clean(FT ft, BLOCKNUM blocknum, uint32_t fullhash, pair_lock_type lock_type, FTNODE *nodep) {
    void *node_v;
    int r = toku_cachetable_maybe_get_and_pin_clean(ft->cf, blocknum, fullhash, lock_type, &node_v);
    if (r != 0) {
        goto cleanup;
    }
    CAST_FROM_VOIDP(*nodep, node_v);
    if ((*nodep)->height > 0 && lock_type != PL_READ) {
        toku_move_ftnode_messages_to_stale(ft, *nodep);
    }
cleanup:
    return r;
}

void toku_unpin_ftnode(FT ft, FTNODE node) {
    int r = toku_cachetable_unpin(ft->cf,
                                  node->ct_pair,
                                  static_cast<enum cachetable_dirty>(node->dirty()),
                                  make_ftnode_pair_attr(node));
    invariant_zero(r);
}

void
toku_unpin_ftnode_read_only(FT ft, FTNODE node)
{
    int r = toku_cachetable_unpin(
        ft->cf,
        node->ct_pair,
        (enum cachetable_dirty) node->dirty(),
        make_invalid_pair_attr()
        );
    assert(r==0);
}

void toku_unpin_ftnode_cutdown(FT ft, struct _ftnode *node) {
     
    int r = toku_cachetable_unpin_cutdown(ft->cf,
                                  cast_from__ctpair(node->ct_pair),
                                  static_cast<enum cachetable_dirty>(node->dirty()),
                                  make_ftnode_pair_attr_cutdown(node));
    invariant_zero(r);
}

void
toku_unpin_ftnode_read_only_cutdown(FT ft, struct _ftnode *node)
{
    int r = toku_cachetable_unpin(
        ft->cf,
        cast_from__ctpair(node->ct_pair),
        (enum cachetable_dirty) node->dirty(),
        make_invalid_pair_attr()
        );
    assert(r==0);
}

void toku_ftnode_swap_pair_values(FTNODE a, FTNODE b)
// Effect: Swap the blocknum, fullhash, and PAIR for for a and b
// Requires: Both nodes are pinned
{
    BLOCKNUM tmp_blocknum = a->blocknum;
    uint32_t tmp_fullhash = a->fullhash;
    PAIR tmp_pair = a->ct_pair;

    a->blocknum = b->blocknum;
    a->fullhash = b->fullhash;
    a->ct_pair = b->ct_pair;

    b->blocknum = tmp_blocknum;
    b->fullhash = tmp_fullhash;
    b->ct_pair = tmp_pair;

    // A and B swapped pair pointers, but we still have to swap
    // the actual pair values (ie: the FTNODEs they represent)
    // in the cachetable.
    toku_cachetable_swap_pair_values(a->ct_pair, b->ct_pair);
}
