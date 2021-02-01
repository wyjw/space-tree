#!/usr/bin/env python

#===- cindex-dump.py - cindex/Python Source Dump -------------*- python -*--===#
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#

from clang.cindex import Index, Config, CursorKind, TypeKind

SPACING = 4

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
             'children' : children
             }

# Globals
global struct_defs
global class_defs

struct_defs = {}
class_defs = {}
type_defs = {}

def get_struct(node, depth=0):
    if opts.maxDepth is not None and depth >= opts.maxDepth:
        children = None
    else:
        if node.is_definition() and node.kind == CursorKind.STRUCT_DECL:
            struct_defs[node.spelling] = node
        for c in node.get_children():
            get_struct(c, depth+1)

def get_class(node, depth=0):
    if opts.maxDepth is not None and depth >= opts.maxDepth:
        children = None
    else:
        if node.is_definition() and node.kind == CursorKind.CLASS_DECL:
            class_defs[node.spelling] = node
        for c in node.get_children():
            get_class(c, depth+1)

def use_struct(structs):
    for key in structs:
        curr = structs[key]
        for c in curr.get_children():
            if c.kind == CursorKind.FIELD_DECL:
                print([c.spelling, c.type.kind, c.type.spelling])
                if c.type.kind == TypeKind.TYPEDEF:
                    print([c.spelling, c.type.get_canonical().kind, c.type.get_canonical().spelling])
                if c.type.kind == TypeKind.ELABORATED:
                    print([c.spelling, c.type.get_canonical().kind, c.type.get_canonical().spelling])

def gen_struct_from_class(classes):
    for key in classes:
        c_res = "struct "
        curr = classes[key]
        c_res += key + " {\n"
        for c in curr.get_children():
            if c.kind == CursorKind.FIELD_DECL:
                print([c.spelling, c.type.kind, c.type.spelling])
                if c.type.spelling not in type_defs:
                    type_defs[c.type.spelling] = []
                if c.type.kind == TypeKind.TYPEDEF:
                    c_res += " " * SPACING + c.type.get_canonical().spelling + " " + c.spelling + ";\n"
                elif c.type.kind == TypeKind.ELABORATED:
                    c_res += " " * SPACING + c.type.get_canonical().spelling + " " + c.spelling + ";\n"
                else:
                    c_res += " " * SPACING + c.type.spelling + " " + c.spelling + ";\n"
        c_res += "}"
        print(c_res)
        print(type_defs)

def generate_compare(structs, classes):
    pass

def main():
    from pprint import pprint

    from optparse import OptionParser, OptionGroup
    import os

    global opts

    parser = OptionParser("usage: %prog [options] {filename} [clang-args*]")
    parser.add_option("", "--show-ids", dest="showIDs",
                      help="Compute cursor IDs (very slow)",
                      action="store_true", default=False)
    parser.add_option("", "--max-depth", dest="maxDepth",
                      help="Limit cursor expansion to depth N",
                      metavar="N", type=int, default=None)
    parser.add_option("", "--old", dest="old", action="store_true", default=False)
    parser.disable_interspersed_args()
    (opts, args) = parser.parse_args()
    
    tmpDir = "/tmp/"
    print(args)
    # set config library
    Config.set_library_file("/usr/lib/llvm-10/lib/libclang.so") 
    if len(args) == 0:
        #parser.error('invalid number arguments')
        
        kInputsDir = os.getcwd() + '/../CutDownPerconaFT/'
        print("Directory is " + str(kInputsDir))
        index = Index.create()
        with open('files.txt', 'r') as f:
            for line in f:
                if (str(line)[0] == '#'):
                    print("Passed " + line)
                    continue
                if (str(line.rstrip())[-1] != 'h'):
                    print("Passed " + line)
                    continue
                addr = '../CutDownPerconaFT/' + str(line.rstrip())
                
                # this adds some letters at the end of the word so that clang thinks it is a C++ file
                nfile = tmpDir + line.rstrip().split("/")[-1] + "pp"
                cmd = "cp " + addr + " " + nfile
                print("Command is " + cmd + " location is " + nfile)
                os.system(cmd) 
                #print("Parsing " + addr)
                #tu = index.parse(None, [str(addr)])
                tu = index.parse(None, [str(nfile)])
                if not tu:
                    parser.error("unable to load input")
                
                get_struct(tu.cursor)
                get_class(tu.cursor)
                
    else:
        index = Index.create()
        tu = index.parse(None, args)
        if not tu:
            parser.error("unable to load input")

        get_struct(tu.cursor)
        get_class(tu.cursor)
    
    pprint(struct_defs)
    pprint(class_defs)
    pprint(('diags', [get_diag_info(d) for d in  tu.diagnostics]))
    
    if len(args) != 0:
        if opts.old == True:
            index = Index.create()
            tu = index.parse(None, args)
            if not tu:
                parser.error("unable to load input")
            finalDict = get_info(tu.cursor)
            pprint(finalDict)
            
    needed_convs = []
    #use_struct(struct_defs)
    gen_struct_from_class(class_defs)

if __name__ == '__main__':
    main()

