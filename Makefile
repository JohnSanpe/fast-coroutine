# SPDX-License-Identifier: GPL-2.0-or-later
flags := -g -O0 -Wall -Werror -I include/
heads := fcoro.h list.h macro.h osdep.h rbtree.h
objs := core.o rbtree.o list.o sleep.o thread.o
demos := example-simple

objs := $(addprefix src/,$(objs))
demos := $(addprefix example/,$(demos))
heads := $(addprefix include/,$(heads))

all: $(demos)

%.o:%.c $(heads)
	@ echo -e "  \e[32mCC\e[0m	" $@
	@ gcc $(flags) -o $@ -c $<

$(demos): $(objs) $(addsuffix .c,$(demos))
	@ echo -e "  \e[34mMKELF\e[0m	" $@
	@ gcc $(flags) -o $@ $@.c $(objs)

clean:
	@ rm -f $(objs) $(demos)
