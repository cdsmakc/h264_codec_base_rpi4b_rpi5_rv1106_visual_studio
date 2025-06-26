
CC_TOOL   = gcc
CC_FLAGS := -lpthread
CC_FLAGS += -Wno-maybe-uninitialized
CC_FLAGS += -Wshadow
CC_FLAGS += -O3
CC_FLAGS += -ffast-math
CC_FLAGS += -Wall
CC_FLAGS += -std=gnu99
CC_FLAGS += -D_GNU_SOURCE
CC_FLAGS += -fomit-frame-pointer
CC_FLAGS += -fno-tree-vectorize
CC_FLAGS += -fvisibility=hidden

SRC_FILE := RCE_CAM.c
SRC_FILE += RCE_ENC.c
SRC_FILE += RCE_DTR.c
SRC_FILE += RCE.c
SRC_FILE += ./iniparser-4.1/src/dictionary.c
SRC_FILE += ./iniparser-4.1/src/iniparser.c

OUT_FILE  = rce

CPP_TOOL   = g++
CPP_CAM_SO_FILES := /usr/lib/aarch64-linux-gnu/libboost_program_options.so.1.74.0
CPP_CAM_SO_FILES += /usr/lib/aarch64-linux-gnu/libcamera-base.so
CPP_CAM_SO_FILES += /usr/lib/aarch64-linux-gnu/libcamera.so
CPP_CAM_FLAGS    := -I/usr/include/libcamera/libcamera/
CPP_CAM_FLAGS    += -I/usr/include/libcamera/
CPP_CAM_FLAGS    += -std=c++17
CPP_CAM_FLAGS    += -lpthread

all : 
	@$(CPP_TOOL) $(CPP_CAM_FLAGS) -I. -c RCE_CAM.cpp -o RCE_CAM.o
	@$(CC_TOOL) $(CC_FLAGS) -I. -c RCE_ENC.c -o RCE_ENC.o
	@$(CC_TOOL) $(CC_FLAGS) -I. -c RCE_DTR.c -o RCE_DTR.o
	@$(CC_TOOL) $(CC_FLAGS) -I. -I./iniparser-4.1/src/ -c RCE.c -o RCE.o
	@$(CC_TOOL) $(CC_FLAGS) -I./iniparser-4.1/src/ -c ./iniparser-4.1/src/dictionary.c -o dictionary.o
	@$(CC_TOOL) $(CC_FLAGS) -I./iniparser-4.1/src/ -c ./iniparser-4.1/src/iniparser.c -o iniparser.o
	@$(CPP_TOOL) -o $(OUT_FILE) RCE_CAM.o RCE_ENC.o RCE_DTR.o RCE.o dictionary.o iniparser.o ./x264-master/libx264.a $(CPP_CAM_SO_FILES) -lm -lpthread -ldl -std=c++17
	@rm -rf RCE_CAM.o
	@rm -rf RCE_ENC.o
	@rm -rf RCE_DTR.o
	@rm -rf dictionary.o
	@rm -rf iniparser.o
	@rm -rf RCE.o

clean :
	@rm -rf RCE_CAM.o
	@rm -rf RCE_CAM.d
	@rm -rf RCE_ENC.o
	@rm -rf RCE_ENC.d
	@rm -rf RCE_DTR.o
	@rm -rf RCE_DTR.d
	@rm -rf dictionary.o
	@rm -rf dictionary.d
	@rm -rf iniparser.o
	@rm -rf iniparser.d
	@rm -rf RCE.o
	@rm -rf RCE.d






