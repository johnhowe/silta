# Sources

SRCS = main.c stm32f4xx_it.c system_stm32f4xx.c startup_stm32f4xx.c
S_SRCS = 

# USB
# SRCS += usbd_usr.c usbd_cdc_vcp.c usbd_desc.c usb_bsp.c

# Project name

PROJ_NAME=stm32f4-gcc-barebones
OUTPATH=build

###################################################

# Check for valid float argument
# NOTE that you have to run make clan after
# changing these as hardfloat and softfloat are not
# binary compatible
ifneq ($(FLOAT_TYPE), hard)
ifneq ($(FLOAT_TYPE), soft)
#override FLOAT_TYPE = hard
override FLOAT_TYPE = soft
endif
endif

###################################################

BINPATH=
AS=$(BINPATH)arm-none-eabi-as
CC=$(BINPATH)arm-none-eabi-gcc
LD=$(BINPATH)arm-none-eabi-ld
OBJCOPY=$(BINPATH)arm-none-eabi-objcopy
OBJDUMP=$(BINPATH)arm-none-eabi-objdump
SIZE=$(BINPATH)arm-none-eabi-size

LINKER_SCRIPT = stm32_flash.ld

CPU = -mcpu=cortex-m4 -mthumb

CFLAGS  = $(CPU) -c -std=gnu99 -g -O2 -Wall
LDLAGS  = $(CPU) -mlittle-endian -mthumb-interwork -nostartfiles -Wl,--gc-sections,-Map=$(OUTPATH)/$(PROJ_NAME).map,--cref --specs=nano.specs

ifeq ($(FLOAT_TYPE), hard)
CFLAGS += -fsingle-precision-constant -Wdouble-promotion
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
else
CFLAGS += -msoft-float
endif

# Default to STM32F40_41xxx if no device is passed
ifeq ($(DEVICE_DEF), )
CFLAGS += -DSTM32F40_41xxx
else
CFLAGS += -D$(DEVICE_DEF)
endif

###################################################

#vpath %.c src
vpath %.a lib


# Includes
INCLUDE_PATHS = -Iinc -Ilib/cmsis/stm32f4xx -Ilib/cmsis/include -lib/STM32F4xx_StdPeriph_Driver/inc
INCLUDE_PATHS += -Ilib/Conf

# Library paths
LIBPATHS = -Llib/STM32F4xx_StdPeriph_Driver
#LIBPATHS += -Llib/USB_Device/Core -Llib/USB_Device/Class/cdc -Llib/USB_OTG

# Libraries to link
LIBS = -lstdperiph 
#LIBS += -lusbdevcore -lusbdevcdc -lusbcore

# Extra includes
INCLUDE_PATHS += -Ilib/STM32F4xx_StdPeriph_Driver/inc
#INCLUDE_PATHS += -Ilib/USB_OTG/inc
#INCLUDE_PATHS += -Ilib/USB_Device/Core/inc
#INCLUDE_PATHS += -Ilib/USB_Device/Class/cdc/inc

#CFLAGS += -Map $(OUTPATH)/$(PROJ_NAME).map

OBJS = $(SRCS:.c=.o)
OBJS += $(S_SRCS:.s=.o)

###################################################

.PHONY: lib proj

all: lib proj
	$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf

lib:
	$(MAKE) -C lib FLOAT_TYPE=$(FLOAT_TYPE)

proj: $(OUTPATH)/$(PROJ_NAME).elf

.s.o:
	$(AS) $(CPU) -o $(addprefix $(OUTPATH)/, $@) $<

.c.o:
	$(CC) $(CFLAGS) -std=gnu99 $(INCLUDE_PATHS) -o $(addprefix  $(OUTPATH)/, $@) $<

$(OUTPATH)/$(PROJ_NAME).elf: $(OBJS)
	$(LD) $(LD_FLAGS) -T$(LINKER_SCRIPT) $(LIBPATHS) -o $@ $(addprefix $(OUTPATH)/, $^) $(LIBS) $(LD_SYS_LIBS)
	$(OBJCOPY) -O ihex $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).bin
	$(OBJDUMP) -S --disassemble $(OUTPATH)/$(PROJ_NAME).elf > $(OUTPATH)/$(PROJ_NAME).dis

clean:
	rm -f $(OUTPATH)/*.o
	rm -f $(OUTPATH)/$(PROJ_NAME).elf
	rm -f $(OUTPATH)/$(PROJ_NAME).hex
	rm -f $(OUTPATH)/$(PROJ_NAME).bin
	rm -f $(OUTPATH)/$(PROJ_NAME).dis
	rm -f $(OUTPATH)/$(PROJ_NAME).map
	$(MAKE) clean -C lib # Remove this line if you don't want to clean the libs as well
	
