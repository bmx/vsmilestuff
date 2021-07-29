.pad 0x18000

mov r1, relocate_me
mov r2, 0
mov r3, 0x400

copy_loop:
        mov r4, [r1]
        mov [r2], r4
        mov r4, 1

        # dst++
        add r2, r4
        # src++
        add r1, r4
        # len--;
        sub r3, r4
        cmp r3, 0
jne copy_loop

# call x0
.byte 0x80, 0xfe, 0x00, 0x00

.pad 0x18200

relocate_me:
.incbin "count.bin"

.pad 0x18400

mov r1, 0x00
# sendbyte(0x00)
.byte 0x40, 0xf0, 0x00, 0x02

sendloop:
        mov r1, 0x41

        # call sendbyte
        .byte 0x40, 0xf0, 0x00, 0x02

        mov r1, 0x42

        # call sendbyte
        .byte 0x40, 0xf0, 0x00, 0x02
jmp sendloop

.pad 0x18600

sendbyte:
        mov r3, 0x3d31
        mov r2, [r3]
        .byte 0x0a, 0xb5, 0x40, 0x00
        jne sendbyte
        mov r3, 0x3d35
        mov [r3], r1
retf


.pad 0x1ffec

vectors:
        .word 0xFFFF
        .word 0xC000
        .word 0xFFFF
        .word 0xFFFF
        .word 0xFFFF
        .word 0xFFFF
        .word 0xFFFF
        .word 0xFFFF
        .word 0xFFFF
        .word 0xFFFF
