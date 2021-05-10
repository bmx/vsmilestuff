// SPG Assembler
//

/*

! comment				A comment, can be appended to a line. Can't use ! in .ascii though
# comment				A comment, at a beginnning of a line only
.inc FILENAME				includes a binary file in the output flow, must be targetly ordered.
.label LABEL				creates a label, for call, goto or branch usage
.hex AA AA AAAA AA AAAAAAAA [...]	includes raw data, 8, 16 or 32 bits, space separated
.ascii "string"				includes ascii string
.loc hexquantity			inserts n zero words in the flow, must be a hex quantity

# insn syntax must be like uu-disas produces
# here are new insns not in uu-disas:
ds = AA
call label
goto label
goto mr
break
call mr
nop
mr = Rd*Rs, {ss|us|uu|<none>}
mr = [Rd]*[Rs], {ss|us|uu}, N
jxx label  ! see all branches like je, jcc, jnae, jss, ja, jb, jz, ...

! there is no real syntax checking, so be carefull respecting case, spaces and wording.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// regs: sp,r1,r2,r3,r4,bp,sr,pc
const char regs[] = "sp__r1__r2__r3__r4__bp__sr__pc";
const char sfts[] = "nop_asr_lsl_lsr_rol_ror";

int debug = 4;
int offset=0;
int pass=0;
int nolabel=0;
FILE *in, *out = NULL;

int strtoval(char *);

struct label_list {
	char *label;
	unsigned int offset;
	struct label_list *next;
} label_list;
typedef struct label_list alabel;

alabel *curr_label, *head_label = NULL;

unsigned int find_label(char *label) {
	curr_label = head_label;
	if (debug) printf("Seeking for label %s from pos %08x\n", label, offset);
	//if (nolabel) {
	//	return(strtoval(label));
	//}
	while (curr_label) {
		if (strcmp(curr_label->label, label) == 0) 
			return curr_label->offset;
		curr_label = curr_label->next;
	}
	return(0);
}

void output(unsigned int val) {
#ifdef _BIG_ENDIAN
	unsigned int revval=0; unsigned short r2=0; 
#endif
	unsigned char n;
	if (val & 0xffff0000) {
		if (pass) {
			if (out) {
#				ifdef _BIG_ENDIAN
					revval = (val&0x00ff0000) << 8 |
						 (val&0xff000000) >> 8 |
						 (val&0x000000ff) << 8 |
						 (val&0x0000ff00) >> 8;
					fwrite(&revval, sizeof(int), 1, out);
#				else
					n = (val >> 16 & 0xff); fwrite(&n, 1,1, out);
					n = (val >> 24 & 0xff); fwrite(&n, 1,1, out);
					n = (val >>  0 & 0xff); fwrite(&n, 1,1, out);
					n = (val >>  8 & 0xff); fwrite(&n, 1,1, out);
#				endif
			} else {
				printf("%06X: %08x\n", offset, val);
			}
		}
		offset += 2;
	} else {
		if (pass) {
		 	if (out) {
#				ifdef _BIG_ENDIAN
					r2 = (val&0x00ff) << 8 | 
						 (val&0xff00) >> 8;
					fwrite(&r2, sizeof(short), 1, out);
#				else
					n = (val >>  0 & 0xff); fwrite(&n, 1,1, out);
					n = (val >>  8 & 0xff); fwrite(&n, 1,1, out);
#				endif
			} else {
				printf("%06X: %04x\n", offset, val);
			}
		}
		offset += 1;
	}
}

char *strip(char *str) {
	while (str && *str == ' ') str++;
	while (str && str[strlen(str)-1] == ' ') str[strlen(str)-1] = 0;
	return(str);
}

int strtoval(char *str) {
	int value;
	char *s_val = (char *)malloc(8*sizeof(char));
	sprintf(s_val, "0x%s", str);
	value = (int)strtoul(s_val, NULL, 16);
	if(debug==2) fprintf(stderr,"VALUE = 0x%0x\n", value);
	free(s_val);
	return(value);
}
int get_reg(char *str) {
	int reg=-1;
	str = strip(str);
	if ( strstr(regs, str)) reg = (strstr(regs, str) - regs) >> 2;
	return(reg);
}

unsigned int do_reg(char *buf) { 
	int ope = -1;
	int value = -1;
	int im6=0, im16=0;
	char *p1, *p2, *p3, *p4, *p5;
	int carry = 0; 
	unsigned int code=0;
	int ra=-1,rb=-1; int sft=0,sfc=0,bp=0,ds=0,indirect=-1;
	

	p1 = strtok(buf, "=");			// "r1 = r2 + 1234, carry" => "r1 "
	p2 = strip(strtok(NULL, "="));			// => "r2 + 1234, carry"
	p3 = strtok(p2, " ");		
	p3 = strtok(NULL, " ");		
	p4 = strtok(NULL, "+-|&^");
	p5 = strtok(p4, ",");
	p5 = strtok(NULL, ",");
	if(debug==1) {
		printf("P1 {%s}\n", p1?p1:"<NULL>");
		printf("P2 {%s}\n", p2?p2:"<NULL>");
		printf("P3 {%s}\n", p3?p3:"<NULL>");
		printf("P4 {%s}\n", p4?p4:"<NULL>");
		printf("P5 {%s}\n", p5?p5:"<NULL>");
	}

	if (p5) carry = 1;
	if (p3 && strstr(p3, "carry")) { carry = 1; p3 = 0; if (p2[2] == ',') p2[2] = 0; }
	if (p1[strlen(p1)-1] == '+') { ope = 0+carry; }
	if (p1[strlen(p1)-1] == '-') { ope = 2+carry; }
	if (p1[strlen(p1)-1] == '^') { ope = 8; }
	if (p1[strlen(p1)-1] == '|') { ope = 10; }
	if (p1[strlen(p1)-1] == '&') { ope = 11; }
	p1[2]=0;
	if (p2[0] == '-' && p2[1] != 0) { ope = 6; p2++; }
	if (p2[0] == '-') { 
		ope = 6; p2=p3; p3=p4; p4=p5; 
		if (! p4 ) p4 = strtok(p3, " "); p4 = strtok(NULL, " ");
	}
	if(debug==1) {
		printf("P1 {%s}\n", p1?p1:"<NULL>");
		printf("P2 {%s}\n", p2?p2:"<NULL>");
		printf("P3 {%s}\n", p3?p3:"<NULL>");
		printf("P4 {%s}\n", p4?p4:"<NULL>");
		printf("P5 {%s}\n", p5?p5:"<NULL>");
	}
	ra = get_reg(strip(p1)); 
	if(debug==1) printf("ra = %d\n", ra);
	if (p3 == NULL ) { //A=B
		if (ope == -1) ope = 9;
		rb = get_reg(strip(p2)); 
		if (rb == -1) {
			if (p2[0] == 'd' && p2[1] == 's') {
				ds=1; p2+=3;
			}
			if (p2[0] == '[') {
				p2++; indirect=0;
				if (p2[0] == '+' && p2[1] == '+') { // A=[++B]
					p2+=2; p2[2] = 0; rb=get_reg(p2);
					indirect=3;
				} else 
				if (p2[2] == '+' && p2[3] == '+') { // A=[B++]
					p2[2] = 0; rb=get_reg(p2);
					indirect=2;
				} else
				if (p2[2] == '-' && p2[3] == '-') { // A=[B--]
					p2[2] = 0; rb=get_reg(p2);
					indirect=1;
				} else
				if (p2[0] == 'b' && p2[1] == 'p' && p2[2] == '+') { // A=[bp+X]
					bp=1; p2+=3; p2[2] = 0; value = strtoval(p2);
				} else
				if (p2[2] == ']') { // A=[X] or A=[B]
					p2[2] = 0;
					rb = get_reg(p2); if(debug==1) printf("rb = %d\n", rb); // A=[B]
					if (rb<0) {			// A=[X]
						value = strtoval(p2); 
						if (strlen(p2) < 3 && value < 0x40 ) im6=1; else im16=1;
					}
				} else {				// A=[LLLL]
					p2[4] = 0; im16=1; rb=ra;
					value = strtoval(p2);
				}
			} else {
				p2 = strip(p2);
				value = strtoval(p2); 
				if (strlen(p2) < 3 && value < 0x40 && (ra == rb||rb==-1)) {
					im6 = 1; 
				} else { 
					im16 = 1; if (rb == -1) rb = ra;
				}
			}
		}
	} else 
	if (p3 && strstr(sfts,p3)) {
		if (ope <0) ope=9;
		rb = get_reg(p2);
		sft = (strstr(sfts, p3) - sfts) >> 2;
		sfc = atoi(p4)-1;
		if(debug==1) printf("sft = %d sfc=%d\n", sft, sfc);
	} else { // A=B op XXXX
		rb = get_reg(p2);
		if (p3[strlen(p3)-1] == '+') { ope = 0+carry; }
		if (p3[strlen(p3)-1] == '-') { ope = 2+carry; }
		if (p3[strlen(p3)-1] == '^') { ope = 8; }
		if (p3[strlen(p3)-1] == '|') { ope = 10; }
		if (p3[strlen(p3)-1] == '&') { ope = 11; }
		if ( p4[0] == '[') {
			indirect=0;
			p4++; p4[4] =0;
		}
		value = strtoval(p4);
		if ((ra == rb) && value < 0x40 && strlen(p4) < 3 ) im6=1; else im16=1;
	}
	if(debug==1) printf("%s op=%d ra=%d rb=%d val=%x im6=%d im16=%d bp=%d ind=%d ds=%d sft=%d sfc=%d\n", 
		buf, ope, ra, rb, value, im6, im16, bp, indirect, ds, sft, sfc);

	if ( bp )  
		code = ope<<12 | ra<<9 | (value&0x3f); // Base+Disp6
	else 
		if (im6) 
			if (indirect<0) 
				code = ope<<12 | ra<<9 | 1<<6 | (value&0x3f); // IMM6
			else
				code = ope<<12 | ra<<9 | 7<<6 | (value&0x3f); // Direct6
		else
			if (im16) 
				if (indirect<0)
					code = (ope<<12 | ra<<9 | 33<<3 | rb)<<16 | (value&0xffff); //IMM16
				else
					code = (ope<<12 | ra<<9 | 34<<3 | rb)<<16 | (value&0xffff); //Direct16
			else 
				if (ds | (indirect>=0)) 
					code = ope<<12 | ra<<9 | (24|ds<< 2|indirect)<<3 | rb; //DS_Indirect
				else
					code = ope<<12 | ra<<9 | 1<<8 | sft<<5 | sfc<<3 | rb; //Register
		
	return(code);
}

unsigned int do_ret(char *buf) {
	if (buf[3] == 'f') 
		return(0x9a90);
	else
		return(0x9a98);
}

unsigned int do_ds(char *buf) {
	char *ptr;
	ptr = strtok(buf, "=");
	ptr = strip(strtok(NULL, "="));
	return(0xfe00 | ( strtoval(ptr) & 0x3f));
}
unsigned int do_push(char *buf) {
	char *ptr; int ope=-1;
	int ra=-1, rb=-1, rs=-1;
	ptr = strip(strtok(buf, " "));
	if (strcmp(ptr, "pop")) {
		// push
		ope = 13;
	} else {
		ope = 9;
		// pop
	}
	ptr = strip(strtok(NULL, ","));
	ra = get_reg(ptr);
	ptr = strip(strtok(NULL, " "));
	rb = get_reg(ptr);
	ptr = strip(strtok(NULL," "));
	if (ope == 13 && strcmp(ptr, "to")) return(-1);
	if (ope == 9 && strcmp(ptr, "from")) return(-1);
	ptr = strip(strtok(NULL, " "));
	if (ptr && ptr++[0] == '[') {
		if (strlen(ptr) == 3) {
			ptr[2] = 0;
			rs = get_reg(ptr);
			return(ope<<12 | (ope==13?(rb):(ra-1)) << 9 | 1 << 7 | (rb-ra+1) << 3 | rs);
		}	
	}
	return -1;
}

unsigned int do_mr(char *buf) {
	char *ptr;
	int rd,rs;
	int srd = 1, srs = 1;
	int sum = 0, size = 1;

	ptr = strtok(buf, "=*,");
	// rd
	ptr = strip(strtok(NULL, "=*,"));
	if (ptr[0] == '[') {
		sum=1;
		ptr++; ptr[2] = 0;
	} 
	rd = get_reg(ptr);	
	// rs
	ptr = strip(strtok(NULL, "=*,"));
	if (sum) {
		ptr++; ptr[2]=0;
	}
	rs = get_reg(ptr);
	ptr = strip(strtok(NULL, "=*,"));
	if (ptr) {
		if (ptr[0] == 'u') 
			srd = 0;
		if (ptr[1] == 'u')
			srs = 0;
	}
	ptr = strip(strtok(NULL, "=*,"));
	if (ptr && sum) {
		size = atoi(ptr);
	}
	if(debug==1) printf("%s rs=%d rd=%d sum=%d srd=%d srs=%d size=%d\n", buf, rs, rd, sum, srd, srs, size);
	return(7<<13|srs<<12 | rd<<9 |srd<<8|sum<<7|size<<3|rs);
	
}

unsigned int do_branch(char *buf) {
	char *ptr, *label; int jump=-1; int dir=1; int val=0;
	ptr = strtok(buf, " ");
	label = strip(strtok(NULL, " "));
	switch(ptr[1]) {
		case 'a':
			if (ptr[2]== 0) jump = 9;
			else if (ptr[2]=='e') jump = 1;
			break;
		case 'b':
			if (ptr[2]==0) jump = 0;
			else if (ptr[2]=='e') jump = 8;
			break;
		case 'c':
			if (ptr[2]=='c') jump = 0;
			else if(ptr[2]=='s') jump = 1;
			break;
		case 'e':
			if (ptr[2]==0) jump = 5;
			break;
		case 'g':
			if (ptr[2]==0) jump = 11;
			else if (ptr[2]=='e') jump = 2;
			break;
		case 'l':
			if (ptr[2]==0) jump = 3;
			else if (ptr[2]=='e') jump = 10;
			break;
		case 'm':
			if (ptr[2]=='i') jump = 7;
			else if(ptr[2]=='p') jump = 14;
			break;
		case 'n':
			if (ptr[2]=='a') {
				if (ptr[3]==0) jump = 8;
				else if(ptr[3]=='e') jump = 0;
			} else 
			if (ptr[2]=='b') {
				if (ptr[3]==0) jump = 1;
				else if(ptr[3]=='e') jump = 9;
			} else
			if (ptr[2]=='e') {
				if (ptr[3]==0) jump = 4;
			} else
			if (ptr[2]=='g') {
				if (ptr[3]==0) jump = 10;
				else if(ptr[3]=='e') jump = 3;
			} else 
			if (ptr[2]=='l') {
				if (ptr[3]==0) jump = 2;
				else if(ptr[3]=='e') jump = 11;
			} else
			if (ptr[2]=='z')
				jump=4;
			break;
		case 'p':
			if (ptr[2]=='l') jump = 6;
			break;
		case 's':
			if (ptr[2]=='c') jump = 2;
			else if(ptr[2]=='s') jump = 3;
			break;
		case 'v':
			if (ptr[2]=='c') jump = 12;
			else if(ptr[2]=='s') jump = 13;
			break;
		case 'z':
			if (ptr[2]==0) jump = 5;
			break;
	}
	if (pass) {
		val = find_label(label);
		if(debug==1) printf("%s jump=%d dir=%d val=%x abs=%x\n", ptr, jump, dir, val, abs(offset-val+1));
		if ( val <= offset ) {
			dir=1; 
		} else {
			dir=0;
		}
		if (abs(offset - val +1) > 0x3f) { fprintf(stderr, "label (%06X -> %06X) too far from branch instr.\n", offset, val); exit(4); }
	}
	return(jump<<12|7<<9|dir<<6|(abs(offset-val+1)&0x3f));
}
unsigned int do_int(char *buf) { 
        char *ptr;

        ptr = strtok(buf, " ");
        if (!strcmp(ptr, "int")) {
                ptr = strip(strtok(NULL, " "));
                if (!strcmp(ptr, "off")) return(0xf140);
                if (!strcmp(ptr, "irq")) return(0xf141);
                if (!strcmp(ptr, "fiq")) return(0xf142);
                if (!strcmp(ptr, "fiq,irq")) return(0xf143);

        } else  
        if (!strcmp(ptr, "irq")) {
                ptr = strip(strtok(NULL, " "));
                if (!strcmp(ptr, "on")) return(0xf149);
                if (!strcmp(ptr, "off")) return(0xf148);
        }
        return -1; 
}

unsigned int do_fast(char *buf) {
	char *ptr;
	ptr = strtok(buf, " ");
	if (!strcmp(ptr, "fir_mov")) {
		ptr = strip(strtok(NULL, " "));
		if (!strcmp(ptr, "off")) return(0xf145);
		if (!strcmp(ptr, "on")) return(0xf144);
	}
	return(-1);
}

unsigned int do_goto(char *buf) {
	char *ptr;
	unsigned int addr = 0;
	ptr = strtok(buf, " ");
	ptr = strip(strtok(NULL, " "));
	if (strcmp(ptr, "mr")) { // goto a22
		addr = find_label(ptr);
		if(debug==1) printf("goto %d\n", addr);
		return( (0xfe00 | 1<<7 | ((addr >> 16)&0x3f)) << 16 | ( addr & 0xffff ));
	} else { // goto mr
		if(debug==1) printf("goto mr\n");
		return(0xfe00 | 3<<6);
	}
}

unsigned int do_test(char *buf) {
	char *ptr;
	int ope = -1;
	int value = -1;
	int im6=0;
	int im16=0;
	int ra=-1,rb=-1; int sft=-1,sfc=-1,indirect=-1,bp=0,ds=0,code=0;
	
	if(debug==1) printf(">>>> test/cmp : %s <<<<\n", buf);
	ptr = strtok(buf, " ");
	if(debug==1) fprintf(stderr,"ptr = [%s]\n", ptr);
	switch(ptr[0]) {
		case 'c': // cmp
			if(debug==1) fprintf(stderr, "CMP ");
			ope = 4;
			break;
		case 't': // test
			if(debug==1) fprintf(stderr, "TEST ");
			ope = 12;
			break;
	}
	ptr = strtok(NULL, ",");
	ra = get_reg(ptr);
	ptr = strtok(NULL, ",");
	if(debug==1) fprintf(stderr,"ptr = [%s]\n", ptr);
	ptr = strip(ptr);
	if(debug==1) fprintf(stderr,"ptr = [%s]\n", ptr);

	switch (strlen(ptr)) {
		case 2:
			if (strstr(regs, ptr)) {
				if(debug==1) fprintf(stderr, "REG >>\n");
				rb = get_reg(ptr); sft = 0; sfc = 0; 
			} else {
				if(debug==1) fprintf(stderr, "IM6 >>\n");
				value = strtoval(ptr); im6=1;
			}
			break;
		case 4:
			if (ptr[0] != '[') {
				if(debug==1) fprintf(stderr, "IM16 >>\n");
				value = strtoval(ptr); im16=1;
			} else {
				ptr++; ptr[2] = '\0';
				if (strstr(regs, ptr)) {
					if(debug==1) fprintf(stderr, "IND REG >>\n");
					rb = get_reg(ptr); sft = 0; sfc = 0; indirect=0;
				} else {
					if(debug==1) fprintf(stderr, "IND IM6 >>\n");
					value = strtoval(ptr); indirect=0; 
					if (strlen(ptr) < 3 && value < 0x40) im6=1; else im16=1;
				}
			}
			break;
		case 9:
		case 6:
			if (ptr[0] == 'd') {
				ds=1;
				ptr+=3;
			}
			ptr++; ptr[4] = '\0';
			if (ptr[0] == '+') {
				if(debug==1) fprintf(stderr, "IND ++REG >>\n");
				ptr+=2; 
				rb = get_reg(ptr);
				indirect=3;
			} else 
			if (ptr[3] == '+') {
				if(debug==1) fprintf(stderr, "IND REG++ >>\n");
				ptr[2] = '\0';
				rb = get_reg(ptr);
				indirect=2;
			} else 
			if (ptr[3] == '-') {
				if(debug==1) fprintf(stderr, "IND REG-- >>\n");
				ptr[2] = '\0';
				rb = get_reg(ptr);
				indirect=1;
			} else {
				if(debug==1) fprintf(stderr, "IND IM16 >>\n");
				indirect=0;
				value = strtoval(ptr); im16=1;
			}
			break;
		case 7:
			ptr+=3; ptr[3] = '\0';
			if (ptr[0] == '[') {
				if(debug==1) fprintf(stderr, "DS IND REG >>\n");
				indirect=0; ds=1;
				ptr++;
				rb = get_reg(ptr);
			} else {
				if(debug==1) fprintf(stderr, "IND BP+ >>\n");
				ptr++;
				value = strtoval(ptr); bp=1; im6=1;
			}
			break;
		case 8:
			rb = get_reg(strtok(ptr, " "));
			sft = (strstr(sfts, strtok(NULL, " ")) - sfts) >> 2;
			sfc = atoi(strtok(NULL, " "))-1;
			break;
			
	}
	if(debug==1) fprintf(stderr, "ope = %d ra = %d rb = %d sft = %d sfc = %d ind=%d ds=%d value=%x %d%d%d\n", ope, ra, rb, sft, sfc, indirect, ds, value, im6, im16, bp);
	if (im6 |  bp ) { // by short value
		if (bp) 
			code = 0;
		else if (indirect>=0) 
			code = 7;
		else code = 1;
		return(ope<<12 | ra << 9 | code << 6 | (value&0x3f));
	} else {  
		if (im16) { 
			rb = ra;
			if (indirect==-1) code = 33; else code = 34;
			return((ope<<12 | ra<<9 | code << 3 | rb ) << 16 | value);
		} else { // by reg
			if (indirect>=0)
				code = 24 | ds<<2 | indirect;
			else
				code = 32 | sft << 2 | sfc;
			return(ope<<12 | ra<<9 | code << 3 | rb );
		}
	}
	return (0);
}
unsigned int do_call(char *buf){ 
	char *ptr; unsigned int val = 0;
	ptr = strtok(buf, " ");
	if (strlen(ptr) == 4 && ptr[2]=='l' && ptr[3]=='l') {
		ptr = strtok(NULL, " ");
		if (strcmp(ptr, "mr")) {
			val = find_label(ptr);
			//if (val == 0) fprintf(stderr, "label %s not found\n", ptr); else printf("label %s found\n", ptr);
			return( ( 0x3c1 << 6 | ((val>>16) & 0x3f) ) << 16 | ( val & 0xffff ) );
			//return((15<<12|1<<6|((val>>16)&0x3f) << 16 )| (val & 0xffff));
		} else {
			return(0xf161);
		}
	} else
		return(-1);
}
unsigned int do_store(char *buf) {
	char *ptr;
	int ope = 0, carry = 0, write = 1, ds=0, indirect = -1;
	int ra=-1, rb=-1, value=0;
	if(debug==1) printf(">>>> %s <<<<\n", buf);
	ptr = strtok(buf, "="); 
	if (ptr[0] == 'd' && ptr[1] == 's' && ptr[2] == ':') {
		ds=1;
		ptr+=3;
	}
	if (ptr[3] == ']') { // [im6] or [reg]
		if ( ptr[1] >= '0' && ptr[1] <= '3') { // [im6]
			ope = 13;
			ptr++; ptr[2] = '\0';
			value = strtoval(ptr) & 0x3f;
			ptr = strtok(NULL, "=");
			ra = get_reg(ptr);
			rb = ra;
			if(debug==1) fprintf(stderr, "ope = %d ra = %d rb = %d value = %x\n", ope, ra, rb, value);
			return(ope<<12 | ra<<9 | 7<<6 | (value&0x3f));
		} else { // [reg]
			indirect = 0;
			ope = 13;
			// 11010110 11000100
			ptr++; ptr[2] = '\0';
			ra = get_reg(ptr);
			ptr = strtok(NULL, "=");
			rb = get_reg(ptr);
			if(debug==1) fprintf(stderr, "ope = %d ra = %d rb = %d value = %x\n", ope, ra, rb, value);
			return(ope<<12 | rb << 9 | 3<<6 | ds<<5 | indirect<<3 | ra);
		}
	} else
	if (ptr[1] == '+') { // [++reg]
		indirect = 3;
		ope = 13;
		ptr+=3; ptr[2] = '\0'; 
		ra = get_reg(ptr); rb = get_reg(strtok(NULL, "=")); 
		return(ope<<12 | rb << 9 | ds<<5 | 3<<6 | indirect<<3 | ra);
	} else
	if (ptr[3] == '+') { // [bp+im6] or [reg++]
		if (ptr[4] == '+') { // [reg++]
			indirect = 2; ope = 13;
			ptr++; ptr[2] = '\0';
			ra = get_reg(ptr); rb = get_reg(strtok(NULL, "=")); 
			return(ope<<12 | rb << 9 | 3<<6 | ds<<5 | indirect<<3 | ra);
		} else { // [bp+im6]
			ope = 13;
			ptr+=4; ptr[2] = '\0';
			value = strtoval(ptr) & 0x3f;
			ptr = strtok(NULL, "=");
			ra = get_reg(ptr);
			rb = ra;
			if(debug==1) fprintf(stderr, "ope = %d ra = %d rb = %d value = %x\n", ope, ra, rb, value);
			return(ope<<12 | ra<<9 | value );
		}
	} else 
	if (ptr[3] == '-') { // [reg--]
		indirect = 1;
		ope = 13;
		ptr++; ptr[2] = '\0';
		ra = get_reg(ptr); rb = get_reg(strtok(NULL, "=")); 
		return(ope<<12 | rb << 9 | 3<<6 | ds << 5 | indirect<<3 | ra);
	} else { // [a16]
		ptr++; ptr[4] = '\0';
		if(debug==1) fprintf(stderr,"A16 = [%s]\n", ptr);
		value = strtoval(ptr);
	}
	ptr = strtok(NULL, "=");
	if (ptr && strstr(ptr, ", carry")) {
		carry = 1;
		ptr = strtok(ptr, ",");
	}
	if (ptr && ptr[0] == '-') { // NEG
		if(debug==1) fprintf(stderr,"NEG \n");
		ope = 6;
		ptr++;
	}
	while (ptr && *ptr == ' ') ptr++; // clean up white spaces
	if (ptr[0] == '-') { // NEG again
		if(debug==1) fprintf(stderr,"NEG \n");
		ope = 6;
		ptr++;
	}
	if (strstr(ptr, "&")) { 
		ope = 11; 
		if(debug==1) fprintf(stderr,"ra = %d rb = %d ptr = [%s]\n", ra, rb, ptr);
		ra = get_reg(strtok(ptr, "&"));
		rb = get_reg(strtok(NULL, "&"));
	} else if (strstr(ptr, "+")) { 
		ope = 0+carry; 
		ra = get_reg(strtok(ptr, "+"));
		rb = get_reg(strtok(NULL, "+"));
	} else if (strstr(ptr, "-")) { 
		ope = 2+carry; 
		ra = get_reg(strtok(ptr, "-"));
		rb = get_reg(strtok(NULL, "-"));
	} else if (strstr(ptr, "^")) { 
		ope = 8; 
		ra = get_reg(strtok(ptr, "^"));
		rb = get_reg(strtok(NULL, "^"));
	} else if (strstr(ptr, "|")) { 
		ope = 10; 
		ra = get_reg(strtok(ptr, "|"));
		rb = get_reg(strtok(NULL, "|"));
	} else { // store
		if (!ope) ope = 13;
		ra = get_reg(ptr);
		rb = ra;
	}
	if(debug==1) fprintf(stderr,"ope = %d ra = %d rb = %d ptr = [%s]\n", ope, ra, rb, ptr);
	return( (ope<<12 | rb<<9 | 17 <<4 | write<<3 | ra) << 16 | value );
}

void do_dot(char *buf) {
	char *ptr;
	
	ptr = strtok(buf, " ");
	if (! strcmp(ptr, ".label") && !pass) {

		ptr = strip(strtok(NULL, " "));
		curr_label = (alabel *)malloc(sizeof(alabel));
		curr_label->offset = offset;
		curr_label->label = (char *)malloc(strlen(ptr)*sizeof(char));
		strcpy(curr_label->label, ptr);
		curr_label->next = head_label;
		head_label = curr_label;
		if (debug == 4) printf("new label at %06X: %s\n", offset, ptr);

	} else
	if (! strcmp(ptr, ".ascii")) {
		ptr = strip(buf+7);
		if (ptr && strlen(ptr)>2) {
			ptr++; ptr[strlen(ptr)-1]=0;
			while(*ptr) {
				output((unsigned char)*ptr++);
			}
		}
	} else
	if (! strcmp(ptr, ".hex")) {
		while ((ptr = strip(strtok(NULL, " ")))) {
			output(strtoval(ptr));
		}
	} else
	if (! strcmp(ptr, ".dec")) {
		while ((ptr = strip(strtok(NULL, " ")))) {
			output(atoi(ptr));
		}
	} else 
	if (! strcmp(ptr, ".inc")) {
		FILE *inc;
		unsigned short word;
		ptr = strip(strtok(NULL, " "));
		if (ptr && (inc=fopen(ptr, "rb"))) {
			while ( fread(&word, 1, 2, inc)) {
				output(word);
			}
			fclose(inc);
		} else {
			fprintf(stderr, "can't open file\n");
		}
	} else
	if (! strcmp(ptr, ".loc")) {
		int q=0;
		ptr = strip(strtok(NULL, " "));
		if (ptr) {
			q = strtoval(ptr);
			while (offset<q) output(0);
		}
	}
			
}

int main(int argc, char **argv) {
	char *readbuf;
	unsigned int retval=0; 
	int new_insn=1;

	readbuf = (char *)malloc(128*sizeof(char));

	if (argc > 1) {
		in = fopen(argv[1], "r");
		if (argc > 2)
			out = fopen(argv[2], "wb");
		if (argc > 3)
			nolabel = 1;
	} else {
		fprintf(stdout, "SPG Assembler\n");
		fprintf(stdout, "Usage: %s <file.asm> [ <file.out> ]\n", argv[0]);
		fprintf(stdout, "  if no out file, hex dump to stdout\n");
		exit(0);
	}

	for (pass=nolabel; pass<2; pass++) {
		fprintf(stderr, "PASS %d\n", pass);
		rewind(in);
		offset=0;
		while ( fgets(readbuf, 128, in) != NULL) {
			new_insn=1;
			readbuf[strlen(readbuf)-1] = '\0';
			readbuf=strip(readbuf);
			// remove comments
			if (strlen(readbuf) && readbuf[0] != '#') {
				readbuf = strtok(readbuf, "!");
				switch (readbuf[0]) {
					case '.':
						new_insn=0;
						do_dot(readbuf);
						break;
					case '[': 
						retval = do_store(readbuf);
						//printf("store\n");
						break;
					case 'f':
						retval = do_fast(readbuf);
						//printf("fast interrupt\n");
						break;
					case 'g':
						retval = do_goto(readbuf);
						//printf("goto\n");
						break;
					case 'i':
						retval = do_int(readbuf);
						//printf("interrupt\n");
						break;
					case 'j':
						retval = do_branch(readbuf);
						//printf("jump\n");
						break;
					case 'm':
						retval = do_mr(readbuf);
						//printf("mr\n");
						break;
					case 'n':
						if (readbuf[1] == 'o' && readbuf[2] == 'p')
							retval = 0xf165;
						break;
					case 'c':
					case 't':
						if (readbuf[1] == 'a') // call
							retval = do_call(readbuf); 
						else
							retval = do_test(readbuf);
						break;
					case 'r': 
					case 's': 
					case 'b': 
					case 'p':
						if (readbuf[0] == 'b' && readbuf[1] == 'r') {
							if(debug==1) printf("BREAK\n");
							retval = 0xf160;
							break;
						} 
						if (readbuf[0] == 'p' && (readbuf[1] == 'o' || readbuf[1] == 'u')) {
							retval = do_push(readbuf);
							break;
						}
						switch(readbuf[1]) {
							case 'e':
								retval = do_ret(readbuf);
								break;
							case 'c':
							case '1': case '2': case '3': case '4':
							case 'p': case 'r':
								retval = do_reg(readbuf);
								break;
						}
						break;
					case 'd':
						if (readbuf[2] == ':') 
							retval = do_store(readbuf);
						else
							retval = do_ds(readbuf);
						break;
					default:
						new_insn=0;
				}	
				// Display retval finally
				if (new_insn) {
					output(retval);
				}
			}
		}
		if (debug == 1 && pass == 0) { 
			printf("List of labels\n");
			curr_label = head_label;
			while (curr_label) {
				printf("%06X %s\n", curr_label->offset, curr_label->label);
				curr_label = curr_label->next;
			}
		}
	}
	if (out) fclose(out);
	return(0);
}



