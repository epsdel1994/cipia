#
#    This file is part of cipia.
#    See [cipia](https://github.com/epsdel1994/cipia) for detail.
#
#    Copyright 2017 T.Hironaka
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

LIBS = -lm

ifeq ($(BUILD),)
	BUILD = optimize
endif

ifeq ($(ARCH),)
	ARCH = x64-modern
endif

ifeq ($(COMP),)
	COMP = clang
endif

ifeq ($(OS),)
	OS = osx
endif

ifeq ($(CC),cc)
	CC = $(COMP)
endif


# gcc 4.x (x >= 7)
ifeq ($(COMP),gcc)
	CFLAGS = -std=c99 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1
	PGO_GEN = -fprofile-generate
	PGO_USE = -fprofile-correction -fprofile-use
	
	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -fwhole-program -flto -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x32-modern)
		CFLAGS += -mx32 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x32)
		CFLAGS += -mx32 -march=native -DUSE_GAS_X64
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -mtune=generic -DUSE_GAS_X86
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),ARM)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -DUSE_GCC_ARM
		endif
	endif
	ifeq ($(ARCH),ARMv7)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -march=armv7-a -mfpu=neon -DUSE_GCC_ARM
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.6
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO
		ifeq ($(ARCH),x86)
			CFLAGS += -DUSE_PTHREAD
		endif
	endif

endif

ifeq ($(COMP),gcc-old)
	CFLAGS = -std=c99 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1
	
	ifeq ($(BUILD),optimize)
		CFLAGS += -O3 -fwhole-program -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X86
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),ARM)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -DUSE_GCC_ARM
		endif
	endif
	ifeq ($(ARCH),ARMv7)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -march=armv7-a -mfpu=neon -DUSE_GCC_ARM
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.6
	endif
	ifeq ($(OS),android)
		CFLAGS += -DANDROID=1
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO
		ifeq ($(ARCH),x86)
			CFLAGS += -DUSE_PTHREAD
		endif
	endif

endif

# g++
ifeq ($(COMP),g++)
	CFLAGS = -x c++ -std=c++11 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1
	PGO_GEN = -fprofile-generate
	PGO_USE = -fprofile-correction -fprofile-use

	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -fwhole-program -flto -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -mtune=generic -DUSE_GAS_X86
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.6 -mdynamic-no-pic
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO
		ifeq ($(ARCH),x86)
			CFLAGS += -DUSE_PTHREAD
		endif
	endif

endif

#icc
ifeq ($(COMP),icc)
	CFLAGS = -std=c99 -Wall -Wcheck -wd2259 -wd913 -D_GNU_SOURCE=1
	PGO_GEN = -prof_gen
	PGO_USE = -prof_use -wd11505

	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -DNDEBUG -rcd -DRCD=0.0 -ansi-alias
	else
		CFLAGS += -O0 -g -DDEBUG -rcd -DRCD=0.0
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -xHOST -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -xHost -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x32-modern)
		CFLAGS += -m64 -xHost -ipo -auto-ilp32 -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -DUSE_GAS_X86
	endif
endif

#pcc
ifeq ($(COMP),pcc)
	CFLAGS = - -D_GNU_SOURCE=1

	ifeq ($(BUILD),optimize)
		CFLAGS += -O4 -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -DUSE_GAS_X64
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -DUSE_GAS_X86
	endif
endif

#clang
ifeq ($(COMP),clang)
	CFLAGS = -std=c99 -pedantic -W -Wall -D_GNU_SOURCE=1

	ifeq ($(BUILD),optimize)
		CFLAGS += -O3 -ffast-math -fomit-frame-pointer -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -DUSE_GAS_X86
	endif

	ifeq ($(OS),linux)
		CFLAGS += -Xlinker -plugin /usr/lib64/llvm/LLVMgold.so
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.6 -mdynamic-no-pic
	endif
endif

#EXE & LIBS
ifeq ($(OS),linux)
	EXE = cipia
 	LIBS += -lrt -lpthread
endif
ifeq ($(OS),android)
	EXE = cipia
endif
ifeq ($(OS),windows)
	EXE = cipia.exe
	LIBS += -lws2_32
	ifeq ($(ARCH),x86)
		LIBS += -lpthread
	endif
endif
ifeq ($(OS),osx)
	EXE = cipia
	LIBS += -lpthread
endif

#SRC
SRC= bit.c board.c move.c hash.c ybwc.c eval.c endgame.c midgame.c root.c search.c \
book.c opening.c game.c base.c bench.c perft.c obftest.c util.c event.c histogram.c \
stats.c options.c play.c ui.c edax.c cassio.c gtp.c ggs.c nboard.c xboard.c main.c mmain.c

# RULES

all: cipia www

www: www/index.html www/FEcipia_eval.js www/main.js www/cache.manifest

www/cache.manifest: cache.manifest
	mkdir -p www
	cp cache.manifest www/cache.manifest

www/index.html: index.html
	mkdir -p www
	cp index.html www/index.html

www/FEcipia_eval.js: cipia-ems.c FEcipia_eval.js
	mkdir -p www
	emcc -O3\
		-s FORCE_FILESYSTEM=1\
		-s ASSERTIONS=1\
		-s TOTAL_MEMORY=134217728\
		--pre-js FEcipia_eval.js\
		-s EXPORTED_FUNCTIONS='["_c_ems_book","_c_ems_load","_c_ems_eval","_c_ems_cache"]'\
		-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'\
		-std=c99 -pedantic -W -Wall -D_GNU_SOURCE=1\
		-Wno-pointer-bool-conversion\
		-Wno-invalid-source-encoding\
		-Wno-empty-body\
		-Wno-dollar-in-identifier-extension\
		-Wno-unused-parameter\
		-Wno-unused-variable\
		-Wno-unused-function\
		-ffast-math -fomit-frame-pointer -DNDEBUG\
		-m64 -lpthread\
		-o www/FEcipia_eval.js cipia-ems.c

www/main.js: FEcipia.js FWcyan.js JSprinCore.js
	mkdir -p www
	cat FWcyan.js JSprinCore.js FEcipia.js > www/main.js

cipia: cipia.c
	/usr/bin/clang $(CFLAGS)\
		-Wno-pointer-bool-conversion\
		-Wno-invalid-source-encoding\
		-Wno-empty-body\
		-Wno-dollar-in-identifier-extension\
		-Wno-unused-parameter\
		-Wno-unused-variable\
		-Wno-unused-function\
		-o $(EXE) $(LIBS) cipia.c

clean:
	rm -rf $(EXE) www

.PHONY: all www
