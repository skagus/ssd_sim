ASM_SRCS := $(wildcard *.s)
ALL_BIN := $(patsubst %.s,%.bin,$(ASM_SRCS))

PREFIX:=riscv-none-embed-

CFLAGS:=-fno-stack-protector
CFLAGS+=-static-libgcc -fdata-sections -ffunction-sections
CFLAGS+=-g -O0 -march=rv32ima -mabi=ilp32 -static
#LDFLAGS:= -T flatfile.lds -nostdlib -Wl,--gc-sections
LDFLAGS:= -nostdlib -Wl,--gc-sections

%.bin: %.s
	$(PREFIX)gcc $(CFLAGS) $(LDFLAGS) -x assembler -MMD -MP -MF$(@:%.bin=%.d) -MT$(@) -c $< -o $(@:%.bin=%.elf)
	$(PREFIX)objcopy -O binary $(@:%.bin=%.elf) $@
	$(PREFIX)objdump --source --all-headers --demangle --wide $(@:%.bin=%.elf) > $(@:%.bin=%.lst)

all : $(ALL_BIN)

#	$(PREFIX)gcc $(LD_FLAGS) -Wl,--cref,-Map,$(@:%.bin=%.map) $(@:%.bin=%.o) -o $(@:%.bin=%.elf)

clean :
	rm -f *.bin
	rm -f *.elf
	rm -f *.map
	