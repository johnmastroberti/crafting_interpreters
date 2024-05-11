Lox.jar: src manifest.mf
	javac -d . src/*
	jar cmf manifest.mf Lox.jar lox

run: Lox.jar
	java -jar Lox.jar

.PHONY: run

