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

r1 = 0780
r2 = 2484
.label next
[r2] = r1
r1 += 01
r2 += 01
r3 = r2 & 3f
cmp r3, 24
jne skipnl
r2 += 20
.label skipnl
cmp r1,07ff
jl next
 

# end
.label end
jmp end

.loc f000
.inc font.bin

.loc fff5
.hex ffff ffff c000 ffff ffff ffff ffff ffff ffff ffff ffff

