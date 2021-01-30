#!/usr/bin/env python

#===- cindex-dump.py - cindex/Python Source Dump -------------*- python -*--===#
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#

from clang.cindex import Index, Config, CursorKind

def get_diag_info(diag):
    return { 'severity' : diag.severity,
             'location' : diag.location,
             'spelling' : diag.spelling,
             'ranges' : diag.ranges,
             'fixits' : diag.fixits }

def get_cursor_id(cursor, cursor_list = []):
    if not opts.showIDs:
        return None

    if cursor is None:
        return None

    # FIXME: This is really slow. It would be nice if the index API exposed
    # something that let us hash cursors.
    for i,c in enumerate(cursor_list):
        if cursor == c:
            return i
    cursor_list.append(cursor)
    return len(cursor_list) - 1

def get_info(node, depth=0):
    if opts.maxDepth is not None and depth >= opts.maxDepth:
        children = None
    else:
        children = [get_info(c, depth+1)
                    for c in node.get_children()]
    return { 'id' : get_cursor_id(node),
             'kind' : node.kind,
             'usr' : node.get_usr(),
             'spelling' : node.spelling,
             'location' : node.location,
             'extent.start' : node.extent.start,
             'extent.end' : node.extent.end,
             'is_definition' : node.is_definition(),
             'definition id' : get_cursor_id(node.get_definition()),
             'children' : children }

# Globals
global struct_defs
global class_defs

struct_defs = {}
class_defs = {}

def get_struct(node, depth=0):
    if opts.maxDepth is not None and depth >= opts.maxDepth:
        children = None
    else:
        if node.is_definition() and node.kind == CursorKind.STRUCT_DECL:
            struct_defs[node.spelling] = [node.extent.start, node.extent.end]
        for c in node.get_children():
            get_struct(c, depth+1)

def get_class(node, depth=0):
    if opts.maxDepth is not None and depth >= opts.maxDepth:
        children = None
    else:
        if node.is_definition() and node.kind == CursorKind.CLASS_DECL:
            class_defs[node.spelling] = [node.extent.start, node.extent.end]
        for c in node.get_children():
            get_struct(c, depth+1)

def main():
    from pprint import pprint

    from optparse import OptionParser, OptionGroup

    global opts

    parser = OptionParser("usage: %prog [options] {filename} [clang-args*]")
    parser.add_option("", "--show-ids", dest="showIDs",
                      help="Compute cursor IDs (very slow)",
                      action="store_true", default=False)
    parser.add_option("", "--max-depth", dest="maxDepth",
                      help="Limit cursor expansion to depth N",
                      metavar="N", type=int, default=None)
    parser.disable_interspersed_args()
    (opts, args) = parser.parse_args()

    # set config library
    Config.set_library_file("/usr/lib/llvm-10/lib/libclang.so") 
    if len(args) == 0:
        parser.error('invalid number arguments')

    index = Index.create()
    tu = index.parse(None, args)
    if not tu:
        parser.error("unable to load input")

    #pprint(('nodes', get_info(tu.cursor)))
    get_struct(tu.cursor)
    get_class(tu.cursor)
    pprint(struct_defs)
    pprint(class_defs)
    pprint(('diags', [get_diag_info(d) for d in  tu.diagnostics]))

if __name__ == '__main__':
    main()

