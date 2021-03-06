# part 1: variables

 PROJECT_PATH     = $(shell find . -mindepth 1 -maxdepth 1 -type d)
 PROJECT_SOURCES  = $(shell find ${PROJECT_PATH} -name *.c -o -name *.s)
 PROJECT_HEADERS  = $(shell find ${PROJECT_PATH} -name *.h             )
 PROJECT_OBJECTS  = $(addsuffix .o, $(basename ${PROJECT_SOURCES}))
 PROJECT_TARGETS  = image.elf image.bin

 QEMU_PATH        = /usr/local/Cellar/qemu/2.8.0
 QEMU_GDB         =        127.0.0.1:1234
 QEMU_UART        = mon:stdio
 QEMU_UART        += telnet:127.0.0.1:1235,server
 QEMU_UART        += telnet:127.0.0.1:1236,server
 QEMU_DISPLAY     = -nographic -display none
#QEMU_DISPLAY     =            -display  sdl

 LINARO_PATH      = /usr/local/Cellar/gcc-arm-none-eabi/20160928
 LINARO_PREFIX    = arm-none-eabi

# part 2: build commands

%.o   : %.s
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-as  $(addprefix -I , ${PROJECT_PATH} ${LINARO_PATH}/arm-none-eabi/include) -mcpu=cortex-a8                                       -g       -o ${@} ${<}
%.o   : %.c
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-gcc $(addprefix -I , ${PROJECT_PATH} ${LINARO_PATH}/arm-none-eabi/include) -mcpu=cortex-a8 -mabi=aapcs -ffreestanding -std=gnu99 -g -c -O -o ${@} ${<}

%.elf : ${PROJECT_OBJECTS}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-ld  $(addprefix -L , ${LINARO_PATH}/lib/gcc/arm-none-eabi/5.4.1) $(addprefix -L, ${LINARO_PATH}/arm-none-eabi/lib) -T ${*}.ld -o ${@} ${^} -lc -lgcc

%.bin : %.elf
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-objcopy -O binary ${<} ${@}

# part 3: targets

.PRECIOUS   : ${PROJECT_OBJECTS} ${PROJECT_TARGETS}

build       : ${PROJECT_TARGETS}

launch-qemu : ${PROJECT_TARGETS}
	@${QEMU_PATH}/bin/qemu-system-arm -M realview-pb-a8 -m 128M ${QEMU_DISPLAY} $(addprefix -serial , ${QEMU_UART}) -gdb tcp:${QEMU_GDB}  -S -kernel $(filter %.bin, ${PROJECT_TARGETS})

launch-gdb  : ${PROJECT_TARGETS}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-gdb -ex "file $(filter %.elf, ${PROJECT_TARGETS})" -ex "target remote ${QEMU_GDB}"

kill-qemu   :
	@-killall KILL qemu-system-arm

kill-gdb    :
	@-killall KILL ${LINARO_PREFIX}-gdb

clean       :
	@rm -f core ${PROJECT_OBJECTS} ${PROJECT_TARGETS}

include Makefile.console
include Makefile.disk
