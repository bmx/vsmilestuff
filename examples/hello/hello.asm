.loc c000

int off
r1 = 55aa
[3d24] = r1
r1 = [3d23]
r1 = r1 & fff9
r1 |= 04
[3d23] = r1
r1 = [3d20]
r1 = r1 & 7fff
[3d20] = r1
r1 = 4006
[3d20] = r1
r1 = 02
[3d25] = r1
r1 = 08
[3d00] = r1
r1 = [3d0a]
r1 |= 07
[3d0a] = r1
r1 = [3d09]
r1 &= 07
r1 |= 17
[3d09] = r1
r1 = [3d08]
r1 &= 07
r1 |= 17
[3d08] = r1
r1 = [3d07]
r1 &= 07
r1 |= 17
[3d07] = r1
r1 = 88c0
[3d0e] = r1
[3d0d] = r1
r1 = f77f
[3d0b] = r1
r1 = 00
[3d04] = r1
[3d03] = r1
r1 = ffff
[3d01] = r1

# sp = 27ff

# serial init

r1 = 00c3
[3d30] = r1
r1 = [3d31]
r1 = 00ff
[3d34] = r1
r1 = 00f1
[3d33] = r1
r1 = 03
[3d31] = r1

r1 = [3d0f]
r1 = r1 | 6000
[3d0f] = r1
r1 = [3d0e]
r1 = r1 | 6000
[3d0e] = r1
r1 = [3d0d]
r1 = r1 | 4000
[3d0d] = r1

# Tx1 attribs

r1 = [3d20]
r1 = r1 & fffb
[3d20] = r1
r1 = 0041
[2812] = r1
r1 = 0a
[2813] = r1
r1 = 2400
[2814] = r1

# palette

r1 = 2b00
r2 = 00
.label p_loop
r4 = 0421
mr = r2*r4
[r1++] = r3
r2 += 02
cmp r2, 10
jne skip
r2 += 01
.label skip
cmp r2, 1f
jle p_loop

# main

.label hello
r3 = fe00
r4 = 2550
.label loopstr
r1 = [r3++]
je out
r2 = r1 + 0780
r2 -= 20
[r4] = r2
r4 += 01
jmp loopstr
.hex 0000
.label out

r1 = 00
.label xx
[2810] = r1
r1 += 01
r3 = 0000
.label loop2
r2 = 0000
.label loop
r2 += 0001
cmp r2, 0000
jne loop
r3 += 0001
cmp r3, 0002
jne loop2
cmp r1, 0200
jle xx

# end
.label end
jmp end

.loc f000
.inc font.bin

.loc fd00
.loc fe00
.ascii "Hello, World" 
.loc fff5
.hex ffff ffff c000 ffff ffff ffff ffff ffff ffff ffff ffff



