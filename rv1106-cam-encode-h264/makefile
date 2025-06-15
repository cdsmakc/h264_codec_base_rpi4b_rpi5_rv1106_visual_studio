# RV1106 camera h264 encode and udp transfer app



RCE_CC := arm-rockchip830-linux-uclibcgnueabihf-gcc

RCE_SDK_PATH := /home/njl/rv1106_sdk/luckfox-pico

RCE_INCLUDE := -I$(RCE_SDK_PATH)/media/samples/example/common
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq/uAPI2
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq/common
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq/xcore
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq/algos
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq/iq_parser
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq/iq_parser_v2
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/out/include/rkaiq/smartIr
RCE_INCLUDE += -I$(RCE_SDK_PATH)/media/samples/example/common/isp3.x
RCE_INCLUDE += -I./iniparser-4.1/src/

PKG_CONF_OPTS := -DRKPLATFORM=ON
PKG_CONF_OPTS += -DARCH64=OFF

RK_MEDIA_OPTS := -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -ffunction-sections -fdata-sections
RK_MEDIA_OPTS += -Os
RK_MEDIA_OPTS += -Wl,-rpath-link,${RCE_SDK_PATH}/media/out/lib:$(RCE_SDK_PATH)/media/out/root/usr/lib 

RK_MEDIA_CROSS_CFLAGS := $(RK_MEDIA_OPTS)
RK_MEDIA_CROSS_CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=hard

RCE_LD_FLAGS += -L$(RCE_SDK_PATH)/media/samples/example/common/lib -lsample_comm
RCE_LD_FLAGS += $(RK_MEDIA_OPTS) -L$(RCE_SDK_PATH)/media/out/lib 
RCE_LD_FLAGS += -lrockit_full

RCE_CFLAGS := -g -Wall $(RCE_INCLUDE) $(PKG_CONF_OPTS) -lpthread $(RK_MEDIA_CROSS_CFLAGS)
RCE_CFLAGS += -DISP_HW_V30 -DRV1106 -DROCKIT_IVS -DROCKCOMBO -DUAPI2 -DRKAIQ_GRP -DAVS_ENABLE


all:
	@$(RCE_CC) $(RCE_CFLAGS) RCE.c RCE_CAM.c RCE_ENC.c RCE_DTR.c ./iniparser-4.1/src/dictionary.c ./iniparser-4.1/src/iniparser.c -o rce $(RCE_LD_FLAGS) -lrockchip_mpp -lrkaiq -lrtsp
	@cp ./rce /mnt/hgfs/share/rce

clean:
	@rm -rf common.d
	@rm -rf isp.d
	@rm -rf isp.o
	@rm -rf out.264
	@rm -rf RCE_CAM.o
	@rm -rf RCE_CAM.d
	@rm -rf RCE_ENC.o
	@rm -rf RCE_ENC.d
	@rm -rf RCE_DTR.o
	@rm -rf RCE_DTR.d
	@rm -rf RCE.o
	@rm -rf RCE.d
	@rm -rf rce
	
#$(RCE_CC) $(RCE_CFLAGS) -c RCE.c -o RCE.o $(RCE_LD_FLAGS) -lrockchip_mpp -lrkaiq -lrtsp
#$(RCE_CC) $(RCE_CFLAGS) -c RCE_CAM.c -o RCE_CAM.o $(RCE_LD_FLAGS) -lrockchip_mpp -lrkaiq -lrtsp
#$(RCE_CC) $(RCE_CFLAGS) -c RCE_ENC.c -o RCE_ENC.o $(RCE_LD_FLAGS) -lrockchip_mpp -lrkaiq -lrtsp
#$(RCE_CC) $(RCE_CFLAGS) -c RCE_DTR.c -o RCE_DTR.o $(RCE_LD_FLAGS) -lrockchip_mpp -lrkaiq -lrtsp
