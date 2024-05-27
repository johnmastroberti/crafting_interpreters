CC = gcc
LD = gcc

DEPDIR := .deps
OBJDIR := .obj

CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused -std=c11 -g -Iinclude
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
LDFLAGS = -lm

SRCS = $(shell find src -type f)
OBJS = $(SRCS:src/%.c=.obj/%.o)

clox: $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c $(DEPDIR)/%.d | $(DEPDIR) $(OBJDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) -o $@ -c $<

$(DEPDIR): ; @mkdir -p $@
$(OBJDIR): ; @mkdir -p $@

compile_commands.json:
	make clean
	bear -- make

DEPFILES := $(SRCS:src/%.c=$(DEPDIR)/%.d)

$(DEPFILES):

include $(wildcard $(DEPFILES))

clean:
	rm -rf .deps .obj clox

run: clox
	./clox

.PHONY: run clean compile_commands.json
