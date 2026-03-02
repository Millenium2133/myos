CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-gcc

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra

OBJS = boot.o kernel.o gdt.o gdt_flush.o idt.o idt_flush.o isr.o pic.o keyboard.o splash.o string.o vga.o

GordOS: $(OBJS) linker.ld
	$(LD) -T linker.ld -o GordOS -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

gdt_flush.o: gdt_flush.s
	$(AS) gdt_flush.s -o gdt_flush.o

gdt.o: gdt.c gdt.h
	$(CC) $(CFLAGS) -c gdt.c -o gdt.o

idt.o: idt.c idt.h
	$(CC) $(CFLAGS) -c idt.c -o idt.o

idt_flush.o: idt_flush.s
	$(AS) idt_flush.s -o idt_flush.o

isr.o: isr.s
	$(AS) isr.s -o isr.o

pic.o: pic.c pic.h
	$(CC) $(CFLAGS) -c pic.c -o pic.o

keyboard.o: keyboard.c keyboard.h
	$(CC) $(CFLAGS) -c keyboard.c -o keyboard.o

boot.o: boot.s
	$(AS) boot.s -o boot.o

splash.o: splash.c splash.h vga.h string.h
	$(CC) $(CFLAGS) -c splash.c -o splash.o

vga.o: vga.c vga.h string.h pic.h
	$(CC) $(CFLAGS) -c vga.c -o vga.o

string.o: string.c string.h
	$(CC) $(CFLAGS) -c string.c -o string.o

kernel.o: kernel.c gdt.h idt.h pic.h keyboard.h splash.h vga.h string.h
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

iso: GordOS
	mkdir -p isodir/boot/grub
	cp GordOS isodir/boot/GordOS
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub2-mkrescue -o GordOS.iso isodir

clean:
	rm -rf *.o GordOS GordOS.iso
	rm -rf isodir/

.PHONY: clean iso
