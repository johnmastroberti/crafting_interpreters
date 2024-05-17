Lox.jar: manifest.mf ast $(shell find src -type f)
	javac -d . src/*
	jar cmf $< $@ lox

AstPrinter.jar: ast_printer_manifest.mf src ast
	javac -d . src/*
	jar cmf $< $@ lox

run: Lox.jar
	java -jar $<

print-test: AstPrinter.jar
	java -jar $<

ast: src/Expr.java

src/Expr.java src/Stmt.java: tools/generate_ast.py tools/expression_defs.txt tools/statement_defs.txt
	python3 tools/generate_ast.py src

.PHONY: run ast print-test

