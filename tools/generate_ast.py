#!/usr/bin/python3
import sys

# TODO: factor expression definitions out into their own file

def usage(argv):
    if len(argv) != 2:
        print(f"Usage: {argv[0]} <output directory>")
        sys.exit(1)

def main(argv):
    usage(argv)

    outputDir = argv[1]

    writeAst(outputDir, "Expr", *getExpressionDefs())

    sys.exit(0)

def writeAst(outputDir, baseName, *types):
    path = f"{outputDir}/{baseName}.java"

    with open(path, 'w') as file:
        file.write(makeAst(baseName, *types))

def makeAst(baseName, *types):
    return f"""
package lox;

import java.util.List;

abstract class {baseName} {{
    abstract <R> R accept(Visitor<R> visitor);
{makeVisitorInterface(baseName, *types)}
{makeTypes(baseName, *types)}
}}
"""

def makeTypes(baseName, *types):
    return '\n'.join([makeType(baseName, t) for t in types])

def makeType(baseName, t):
    [className, fields] = [x.strip() for x in t.split(':')]
    return f"""
    static class {className} extends {baseName} {{
        {className}({fields}) {{
{makeMemberAssignments(fields)}
        }}

        @Override
        <R> R accept(Visitor<R> visitor) {{
            return visitor.visit{className}{baseName}(this);
        }}

{makeMemberDecls(fields)}
    }}
"""

def makeMemberAssignments(fieldList):
    ret = ""
    for field in fieldList.split(", "):
        id = field.split(' ')[1].strip()
        ret += f"            this.{id} = {id};\n"
    return ret

def makeMemberDecls(fieldList):
    return '\n'.join([f"        final {field};" 
                      for field in fieldList.split(", ")])

def makeVisitorInterface(baseName, *types):
    typeNames = [t.split(":")[0].strip() for t in types]
    b = baseName
    decls = '\n'.join([
        f"        R visit{t}{b}({t} {b.lower()});" for t in typeNames])
    return f"""    interface Visitor<R> {{
{decls}
    }}
"""
                                     


def getExpressionDefs():
    ret = ""
    with open("tools/expression_defs.txt") as defs:
        ret = defs.read().split('\n')
    is_nonempty = lambda x : len(x) > 0
    return [*filter(is_nonempty, ret)]

if __name__ == "__main__":
    main(sys.argv)
