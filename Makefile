#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .obj and executable files
#

# define the C compiler to use
CC = mingw32-gcc

# define the assembler compiler to use
AS = nasm

# define any compile-time flags
CFLAGS =  -D_GNU_SOURCE=1 -Dmain=SDL_main -g -lpthread -Wall -mwin32 -s

# define any directories containing header files other than /usr/include
#
INCLUDES = -I/usr/include/ -I/usr/local/include/ -I/usr/include/SDL/ -I/usr/local/include/SDL

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS = -L/lib -L/usr/lib/ -L/usr/local/lib

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = -lmingw32 -lSDLmain -lSDL -mwindows -lSDL_mixer

# define the source files
SRCS = source/ai.asm        \
       source/enemy.asm     \
       source/input.asm     \
       source/main.asm      \
       source/mapeng.asm    \
       source/menu.asm      \
       source/player.asm    \
       source/ppe.asm       \
       source/rand.asm      \
       source/sdl.asm       \
       source/sse_mem.asm

# define the object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .asm of all words in the macro SRCS
# with the .obj suffix
#
OBJS = $(SRCS:.asm=.obj)

# define the executable file 
MAIN = win_app

# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .obj's from .asm's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .asm file) and $@: the name of the target of the rule (a .obj file) 
# (see the gnu make manual section about automatic variables)
%.obj: %.asm
	$(AS) -f win32 --prefix _ -I./include/ $*.asm -o $*.obj
    
clean:
	$(RM) source/*.obj *~ $(MAIN)

# DO NOT DELETE THIS LINE -- make depend needs it