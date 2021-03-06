#
# Makefile: Build and clean the program
# Copyright (C) 2011  Matt Miller
#
# Based on example from:
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

#IDIR =../include
CC=gcc
AR=ar
LN=ln
#CFLAGS=-I$(IDIR)
CFLAGS=-Wall -Werror -g -fPIC

ODIR=obj
LDIR =lib

#LIBS=-lm
LIBS=

DIR=mkavl
NAME=$(DIR)

MAJ_VER=1
MIN_VER=0
BUILD_VER=0

LIB_NAME=lib$(NAME).so
LIB_LD_NAME=lib$(NAME).so.$(MAJ_VER)
LIB_GCC_NAME= $(LIB_LD_NAME).$(MIN_VER).$(BUILD_VER)
STATIC_LIB_NAME=lib$(NAME).a

#_DEPS = hellomake.h
#DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
DEPS = mkavl.h

AVL_DIR=libavl
AVL_SRC=avl.c
AVL_HDR=avl.h
AVL_DEPS = $(patsubst %,$(AVL_DIR)/%,$(AVL_HDR))
_AVL_OBJ = avl.o 
AVL_OBJ = $(patsubst %,$(AVL_DIR)/$(ODIR)/%,$(_AVL_OBJ))

_OBJ = mkavl.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: lib_symlinks $(LDIR)/$(STATIC_LIB_NAME)

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(AVL_DIR)/$(ODIR)/%.o: $(AVL_DIR)/%.c $(AVL_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(LDIR)/$(LIB_GCC_NAME): $(OBJ) $(AVL_OBJ)
	$(CC) -shared -Wl,-soname,$(LIB_LD_NAME) -o $@ $^

$(LDIR)/$(LIB_LD_NAME): $(LDIR)/$(LIB_GCC_NAME)
	cd $(LDIR); $(LN) -sf $(LIB_GCC_NAME) $(LIB_LD_NAME)

$(LDIR)/$(LIB_NAME): $(LDIR)/$(LIB_GCC_NAME)
	cd $(LDIR); $(LN) -sf $(LIB_GCC_NAME) $(LIB_NAME)

lib_symlinks: $(LDIR)/$(LIB_NAME) $(LDIR)/$(LIB_LD_NAME)

$(LDIR)/$(STATIC_LIB_NAME): $(OBJ) $(AVL_OBJ)
	$(AR) rcs $@ $^

.PHONY: clean tar doc

clean:
	rm -f $(ODIR)/*.o $(AVL_DIR)/$(ODIR)/*.o *~ core 

doc:
	doxygen
	cd doc/latex; pdflatex refman.tex

tar:
	tar -czvf $(NAME).tar.gz ../$(NAME) --exclude *.swp --exclude *.o \
	--exclude test_$(NAME) --exclude employee_example \
        --exclude malloc_example --exclude *.a \
        --exclude *.so* --exclude .git --exclude $(NAME).tar.gz
