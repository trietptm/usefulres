/*
 * 	TLCS900 processor module for IDA Pro.
 *	Copyright (c) 1998-2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#include "tosh.hpp"

// (�᫮ ����-1) -> ⨯
static const uchar bt_type[4]={dt_byte,dt_word,dt_tbyte,dt_dword};

// �⥯��� ������ -> ⨯
static const uchar btp_type[4]={dt_byte, dt_word, dt_dword, uchar(-1)};

// ���⪨� ��� ॣ���� � ������
static uchar Reg7ToFull(uchar reg7, uchar size)
{
reg7&=7;   // � ���� �����
// �����
if(size==0)return(0xE0+(1-(reg7&1))+(reg7&6)*2);
// ᫮�� ��� ������ ᫮��
return(0xE0+reg7*4);
}

// ��⠭����� ����� ॣ���� � ���࠭��
// reg_code - ���⮢� ��� ॣ����
// size - 0,1,2 (2^x ����)
static void SetRegistr(op_t &op, uchar reg_code, uchar size)
{
 // ⨯ ���࠭�� - ॣ����
 op.type=o_reg;
 // ����� ॣ����
 op.addr=op.value=reg_code;
 // ࠧ��� ॣ����
 op.dtyp=btp_type[size&3];
}

// ��⠭����� ���⮩ ॣ����
// regcode - ����� ॣ���� (����訥 3 ���)
// size - 0,1,2 (2^x ����)
static void SetRegistr7(op_t &op, ushort regcode, ushort size)
{
SetRegistr(op,Reg7ToFull(regcode,size),size);
}

// ����㧨�� N ���� � ������ ���祭��
static ulong LoadDataValue(int bytes)
{
ulong val=0;
int i;
        // ����㧨� �� �����
        for(i=0;i<bytes;i++)
                val|=((unsigned long)ua_next_byte())<<(8*i);
return(val);
}

// ⥪. ����(�) - ��אַ� ���� � �����
// len - �᫮ ����
static void SetDirectMemRef(op_t &op, int len)
{
        op.type=o_mem;
        // ᬥ饭�� �� �����
        op.offb=cmd.size;
        // ࠧ��� ᬥ饭��
        op.dtyp=bt_type[(len-1)&3];
        // ���祭�� �����
        op.addr=op.value=LoadDataValue(len);
}


// ����� � ��⠭����� ��뫪� �� ���
static void SetJmp(op_t &op, int len)
{
        op.type=o_near;
        op.offb=cmd.size;
        op.dtyp=dt_dword;
        if(len>0){
                // ��᮫�⭠� ������
                op.addr=op.value=LoadDataValue(len);
        }
        else{
                // �⭮�⥫쭠� ������
                len=-len;
                op.addr=LoadDataValue(len);
                // ���᫨� ����
                if(op.addr&(1<<(8*len-1))){
                                // ������� ��� �⮨� - ���ਬ ���
                                op.addr|=0xFFFFFFFFL<<(8*len);
                }
                // ���᫨� 楫���� ᬥ饭��
                op.addr+=cmd.ip+cmd.size;
                op.value=op.addr;
        }
}

// ����⥫� ��뫪� �� ������ (o_mem/o_displ)
typedef struct{
optype_t        type;   // ⨯: o_mem/o_displ
ulong  disp;            // ᬥ饭��
ushort off_pos;         // ᬥ饭�� ᬥ饭�� � ������� (+䫠� ��� ������)
uchar flags;            // 䫠��
uchar  base_reg;        // �᭮���� ॣ���� (����� �����)
uchar  add_reg;         // ����� ॣ���� (����� �����)
uchar  inc_size;        // ࠧ��� ���६��� (+/-4)
uchar  dtyp;            // ࠧ��� ����뫠���� ������
}MemRefDef;

// ����஢��� ����㦥���� ��뫪� � ����⥫� ���࠭��
static void MemRefToOp(op_t &op, MemRefDef &mr)
{
op.value=op.addr=mr.disp;       // ᬥ襭��
op.dtyp=mr.dtyp;                // ⨯ ����뫠���� ������
op.reg=mr.base_reg;             // �᭮���� ॣ����
op.specflag2=mr.inc_size;       // ࠧ��� ���६���
op.offb=mr.off_pos;             // ᬥ饭�� �� ᬥ饭�� � 䫠� ��� ������
op.specval_shorts.low=mr.add_reg;// ��� �ᯮ����⥫쭮�� ॣ����
op.specflag1=mr.flags;           // ࠧ��� �ᯮ�. ॣ���� � ��祥
//if(mr.flags&URB_LDA2){
//        op.type=o_imm;
//}
op.type=mr.type;                // ⨯
}

// ����㧨�� ��ࠬ���� ��뫪� �� ������ �� �६����� �������
// �������� ����� �������
// first_code - ���� ���� ������樨
static int LoadMemRef(MemRefDef &mr, uchar first_code)
{
// ��砫쭠� ����⪠
memset(&mr,0, sizeof(mr));
// ��।���� ࠧ��� ������
mr.dtyp=btp_type[(first_code>>4)&3];
if((first_code&0x40)==0){
        // ��뫪� - ॣ���� (� ᬥ饭��� ��� ��� ᬥ饭��)
        mr.type=o_displ;
        // �������� ॣ����
        mr.base_reg=Reg7ToFull(first_code,2);
        if(first_code&0x8){
                // ������� ᬥ饭��
                mr.off_pos=cmd.size;    // ⥪. ᬥ饭��
                mr.disp=ua_next_byte();
        }
}
else {
        // ���� ��ਠ��
        switch(first_code&7){
        // ��אַ� ����, ����
        case 0: mr.off_pos=cmd.size;
                mr.disp=LoadDataValue(1);
                mr.type=o_mem;
                break;
        // ��אַ� ����, ᫮��
        case 1: mr.off_pos=cmd.size;
                mr.disp=LoadDataValue(2);
                mr.type=o_mem;
                break;
        // ��אַ� ����, 24 ���
        case 2: mr.off_pos=cmd.size;
                mr.disp=LoadDataValue(3);
                mr.type=o_mem;
                break;
        // ���� ����ॣ���஢� ��ਠ���
        case 3: {
                uchar mem_val;
                mr.type=o_displ;
                mem_val=ua_next_byte();
                if((mem_val&2)==0){
                        // ॣ���� ���������
                        mr.base_reg=mem_val&0xFC;       // ��� ॣ����
                        // ᬥ饭�� ���� ?
                        if(mem_val&1){
                                mr.off_pos=cmd.size;
                                mr.disp=LoadDataValue(2);
                        }
                }
                else { // ��� ॣ����
                        if((mem_val&1)==0)return(0); // ����ୠ� ���������
                        if((mem_val>>2)>1){
                                // ����!!! ���ᨡ� ﯮ�栬 �� LDAR!!!!
                                // �஢��塞 �� F3/13
                                //msg("Ldar Op");
                                if((first_code==0xF3)&&
                                (mem_val==0x13)){
                                        // ���!!!
                                        ulong   target;
                                        cmd.itype=T900_ldar;
                                        // ᬥ饭�� �� �����
                                        cmd.Op2.offb=cmd.size;
                                        target=LoadDataValue(2);
                                        target+=cmd.ea+4;
                                        cmd.Op2.type=o_mem;
                                        // ࠧ��� ᬥ饭��
                                        cmd.Op2.dtyp=dt_word;
                                        // ���祭�� �����
                                        cmd.Op2.addr=cmd.Op2.value=target;
                                        // ����稬 ॣ����
                                        mem_val=ua_next_byte();
                                        // �����⨬ ?
                                        if((mem_val&0xE8)!=0x20)return(0);
                                        SetRegistr7(cmd.Op1,mem_val,((mem_val>>4)-1)&3);
                                        //msg("ldar ok");
                                        return(1);
                                }
                                // �� ��祥 �訡�筮
                                return(0);
                        }
                        mr.base_reg=ua_next_byte();     // ���� ॣ����
                        mr.add_reg=ua_next_byte();      // ��ன ॣ����
                        if(mem_val&0x4)mr.flags|=URB_WORD;// ��. ॣ���� - ᫮��
                }
                }
                break;
        // ���६���/���६���
        case 4:
        case 5: {uchar regg;
                regg=ua_next_byte();
                if((regg&3)==3)return(0);
                mr.type=o_displ;
                mr.base_reg=regg&0xFC;
                mr.inc_size=1<<(regg&3);
                // ���⠢�� 䫠�, �᫨ ����⥫�� ���६���
                if((first_code&1)==0)mr.inc_size|=URB_DECR;
                }
                break;
        }
}
// �८�ࠧ������ �ᯥ譮!
return(1);
}


// ⥪ ����(�) - �����।�⢥��� �����, N ����
static void SetImmData(op_t &op, int bytes)
{
        op.type=o_imm;
        // ᬥ饭�� ⥪. ����
        op.offb=cmd.size;
        // ࠧ��� �����
        op.dtyp=bt_type[(bytes-1)&3];
        // ���祭��
        op.addr=op.value=LoadDataValue(bytes);
}


// ��⠭����� ���祭�� �����।�⢥���, ⮫쪮 �᫮��
static void SetImm8Op(op_t &op, uchar code)
{
  op.type=o_imm;        // �����।�⢥����
  op.dtyp=dt_byte;      // ॠ�쭮 - �� ᮢᥬ ����
  op.flags|=OF_NUMBER;   // ⮫쪮 �᫮
  op.addr=op.value=code;   // ���祭�� �᫠
}

// ��⠭����� imm3 ��� inc/dec
static void SetImm3Op(op_t &op, uchar code)
{
code&=7;
SetImm8Op(op,code?code:8);
}

// ��⠭����� ���࠭� � �ࠧ� �᫮���
static void SetCondOp(op_t &op, int cond)
{
 static const uchar cond_p[16]={
         fCF,fCLT,fCLE,fCULE,fCPE,fCMI,fCZ,fCC,
        fCT,fCGE,fCGT,fCUGT,fCPO,fCPL,fCNZ,fCNC};
 op.type=o_phrase;
 op.phrase=cond_p[cond&0xf];
}


// ��䬥⨪�-�����᪨� ������樨
static const uchar Add_List[8]={
T900_add,T900_adc,T900_sub,T900_sbc,
T900_and,T900_xor,T900_or,T900_cp};

// ������樨 ᤢ��� - ��� �� ᮢᥬ ����
static const uchar Shift_List[8]={
T900_rlc,T900_rrc,T900_rl,T900_rr,
T900_sla,T900_sra,T900_sll,T900_srl};
// ᤢ��� ��� ࠡ��� � �祩����
static const uchar Shift_List1[8]={
T900_rlc_mem,T900_rrc_mem,T900_rl_mem,T900_rr_mem,
T900_sla_mem,T900_sra_mem,T900_sll_mem,T900_srl_mem};

// ����樨 � 䫠��� C
static const uchar COp_List[5]={
T900_andcf,T900_orcf,T900_xorcf,T900_ldcf,T900_stcf};

// ���� ��㯯� ����権 c C
static const uchar COp2_List[5]={
T900_res,T900_set,T900_chg,T900_bit,T900_tset};

//===========================================================================
// ࠧ��ઠ reg'��
static int RegAnalyser(uchar code)
{
uchar reg_byte;
uchar reg_num;         // ���⮢� ����� ॣ����
uchar reg_size;        // 0 - byte, 1 - word, 2 - long
uchar  reg_op=0;        // ���࠭� � ॣ���஬ (�� 㬮�砭�� - ����)
static const uchar reg_codes[32]={
        255,        255,       255,           255,
        T900_andcf, T900_andcf,T900_res,      T900_minc1,
        T900_mul,   T900_muls, T900_div,      T900_divs,
        T900_inc,   T900_dec,  T900_scc,      T900_scc,
        T900_add,   T900_ld,   T900_adc,      T900_ld,
        T900_sub,   T900_ld,   T900_sbc,      T900_ex,
        T900_and,   254,       T900_xor,      253,
        T900_or,    T900_rlc,  T900_cp,       T900_rlc
};
// ��।������ � ��ࠬ��ࠬ� ॣ����
 reg_size=(code>>4)&3;  // ���稬 ���� ࠧ��� ॣ����
 // ��।������ � ����஬ ॣ����
 if(code&8){ // ����� ॣ���� ����� � ����
        reg_num=Reg7ToFull(code,reg_size);   // � ���� �����
//        msg("reg_int\n");
 }
 else{ // ���� �������⥫�� ����
        reg_num=ua_next_byte();
 }
 // ���� ���� ����樨
 reg_byte=ua_next_byte();
 switch(cmd.itype=reg_codes[(reg_byte>>3)&0x1F]){
 // ��䬥��᪨�
 case T900_ex:
 case T900_add:
 case T900_adc:
 case T900_sub:
 case T900_sbc:
 case T900_and:
 case T900_xor:
 case T900_or:
 case T900_cp:  SetRegistr7(cmd.Op1,reg_byte,reg_size);
                reg_op=1;
                break;
 // ����訥 �������
 case 255:     {static const uchar  LCodes[]={
                0,         0,         0,         T900_ld,
                T900_push, T900_pop,  T900_cpl,  T900_neg,
                T900_mul,  T900_muls, T900_div,  T900_divs,
                T900_link, T900_unlk, T900_bs1f, T900_bs1b,
                T900_daa,  0,         T900_extz, T900_exts,
                T900_paa,  0,         T900_mirr, 0,
                0,         T900_mula, 0,         0,
                T900_djnz, 0,         0,         0};

                switch((cmd.itype=LCodes[reg_byte])){
                // ����饭�� ����
                case 0:         return(0);

                // LD r, #
                case T900_ld:   SetImmData(cmd.Op2,1<<reg_size);
                                break;

                // MUL rr, #
                // DIV rr, #
                case T900_div:
                case T900_divs:
                case T900_mul:
                case T900_muls: SetImmData(cmd.Op2,1<<reg_size);
                                // ��. ����� ���訩 ॣ����
                                reg_size++;
                                if(reg_size==3)return(0);
                                break;

                // LINK r, dd
                case T900_link: SetImmData(cmd.Op2,2);
                                break;

                // BS1F A,r
                case T900_bs1f:
                case T900_bs1b:
                                SetRegistr7(cmd.Op1, 1, 0);
                                reg_op=1;
                                break;
                // MULA r
                case T900_mula:
                                // ��. ����� ���訩 ॣ����
                                reg_size++;
                                if(reg_size==3)return(0);
                                break;
                // DJNZ r, d
                case T900_djnz: //if(reg_num==0){
                                //        // �᪫�祭�� �� reg=0
                                //       SetJmp(cmd.Op1,-1);
                                //        return(cmd.size);
                                //}
                                SetJmp(cmd.Op2,-1);
                                break;
                }
                break;
                }

 // ANDCF-STCF  XXX #, r
 case T900_andcf: if(reg_byte>0x2C){
                        switch(reg_byte){
                        case 0x2D: return(0);
                        // ���� ������� LDC - ���� ��⠢�塞 ��� ����
                        case 0x2E:      SetImmData(cmd.Op1,1);
                                        reg_op=1;
                                        break;
                        case 0x2F:      SetImmData(cmd.Op2,1);
                                        break;
                        }
                        // ���⠢�� ⨯ �������
                        cmd.itype=T900_ldc;
                }
                // �� �� ldc
                else if ((reg_byte&7)<5){
                        reg_op=1;
                        cmd.itype=COp_List[reg_byte&7];
                        if(reg_byte&8)  SetRegistr7(cmd.Op1,1,0);
                        else            SetImmData(cmd.Op1,1);
                }
                // �訡��� �������
                else return(0);
                break;

 // RES-TSET
 case T900_res: if((reg_byte&7)>4)return(0);     // �訡��� ����
                cmd.itype=COp2_List[reg_byte&7];
                SetImmData(cmd.Op1,1);
                reg_op=1;
                break;

 // ��蠭�� MINC/MDEC
 case T900_minc1:{static const uchar dinc[8]={
                T900_minc1,T900_minc2,T900_minc4,0,
                T900_mdec1,T900_mdec2,T900_mdec4,0};
                if((cmd.itype=dinc[reg_byte&7])==0)return(0);
                SetImmData(cmd.Op1,2);
                // ���ࠢ�� ���࠭�
                cmd.Op1.value+=1<<(reg_byte&3);
				cmd.Op1.addr=cmd.Op1.value;
                reg_op=1;
                }
                break;
 // 㬭������/�������  XXX R,r
 case T900_mul:
 case T900_muls:
 case T900_div:
 case T900_divs:SetRegistr7(cmd.Op1, (reg_size==0)?(reg_byte&7)/2:reg_byte, reg_size+1);
                reg_op=1;
                break;

 // INC/DEC #3, r
 case T900_inc:
 case T900_dec: SetImm3Op(cmd.Op1,reg_byte);
                reg_op=1;
                break;

 // ��⠭���� SCC, r
 case T900_scc: SetCondOp(cmd.Op1,reg_byte&0xF);
                reg_op=1;
                break;
 // LD
 case T900_ld:  if(reg_byte<0x90)reg_op=1;
                if(reg_byte<0xA0) SetRegistr7(cmd.Operands[1-reg_op], reg_byte, reg_size);
                else            SetImm8Op(cmd.Op2,reg_byte&7);
                break;

 // ��ன ��ਠ�� ��䬥��᪨� XXX r, #)
 case 254:      cmd.itype=Add_List[reg_byte&7];
                SetImmData(cmd.Op2,1<<reg_size);
                break;
 // CP r, #3
 case 253:      cmd.itype=T900_cp;
                SetImm8Op(cmd.Op2,reg_byte&7);
                break;
 // ᤢ���
 case T900_rlc: cmd.itype=Shift_List[reg_byte&7];
                if(reg_byte>=0xF8)      SetRegistr7(cmd.Op1,1,0);
                else    {       uchar ShL;
                                ShL=ua_next_byte();
                                SetImm8Op(cmd.Op1,(ShL==0)?16:ShL);
                }
                reg_op=1;
                break;
 // ��祥
 default:       return(0);
 }
 // ����襬 ॣ����
 SetRegistr(cmd.Operands[reg_op], reg_num, reg_size);
// msg("ANA: reg_num=%x, size=%x\n",(int)reg_num, (int)reg_size);
 return(cmd.size);
}

//===========================================================================
// ࠧ��ઠ dst
static int DSTAnalyser(uchar code)
{
// ࠧ��� ��ண� ���� ⨯� DST
uchar dst_byte;
// ����� ���࠭�� � �������
char memrefop=1;
MemRefDef mr;   // ����⥫� ��뫪� �� ������
// �᭮���� ���ᨢ �����
static const uchar dst_codes[32]={
        255,        0,         255,           0,
        T900_lda,   255,       T900_lda,      0,
        T900_ld,    0,         T900_ldw,      0,
        T900_ld,    0,         0,             0,
        T900_andcf, T900_orcf, T900_xorcf,    T900_ldcf,
        T900_stcf,  T900_tset, T900_res,      T900_set,
        T900_chg,   T900_bit,  T900_jp_cond,  T900_jp_cond,
        T900_call,  T900_call, T900_ret_cond, T900_ret_cond
};
// ���쬥� ��뫪� �� ������
if(LoadMemRef(mr,code)==0)return(0);
// �஢�ઠ �� �᪫�祭�� - LDAR (��㡨��� ��!!!)
if(cmd.itype==T900_ldar)return(cmd.size);
// ��� ����樨
dst_byte=ua_next_byte();
// �㦭� �ய��뢠�� mr.dtyp - �� 㬮�砭�� ����
mr.dtyp=dt_byte;

cmd.itype=dst_codes[(dst_byte>>3)&0x1F];
switch(cmd.itype){

// ⠪��� ���� ���
case 0:         return(0);

// �ॡ���� �������⥫�� ࠧ���
case 255:       if(dst_byte<0x2D && dst_byte>=0x28){
                        // �� ����樨 � ��⮬ c
                        // ⨯ ���࠭��
                        cmd.itype=COp_List[dst_byte-0x28];
                        // ���� ���࠭� - ॣ���� A
                        SetRegistr7(cmd.Op1,1,0);
                        // ��ன ���࠭� - ��뫪� �� ������
                        break;
                }
                // ��祥
                switch(dst_byte){
                // ld byte
                case 0x00:      cmd.itype=T900_ld;
                                SetImmData(cmd.Op2,1);
                                memrefop=0;
                                break;

                // ld word
                case 0x02:      cmd.itype=T900_ldw;
                                SetImmData(cmd.Op2,2);
                                mr.dtyp=dt_word;
                                memrefop=0;
                                break;

                // pop byte
                case 0x04:      cmd.itype=T900_pop;
                                memrefop=0;
                                break;

                // pop word
                case 0x06:      cmd.itype=T900_popw;
                                mr.dtyp=dt_word;
                                memrefop=0;
                                break;

                // ld byte xx - � ��୮�, � �� ���� �⮩ �������!!!
                case 0x14:      cmd.itype=T900_ld;
                                SetDirectMemRef(cmd.Op2,2);
                                memrefop=0;
                                break;
                // ld word
                case 0x16:      cmd.itype=T900_ldw;
                                SetDirectMemRef(cmd.Op2,2);
                                mr.dtyp=dt_word;
                                memrefop=0;
                                break;

                // �� ��������
                default:        return(0);
                }
                break;
// ����㧪� 40, 50, 60
case T900_ldw:
case T900_ld:           SetRegistr7(cmd.Op2,dst_byte,(dst_byte>>4)&0x3);
                        mr.dtyp=btp_type[(dst_byte>>4)&3];
                        memrefop=0;
                        break;
// ����㧪�A 20, 30
case T900_lda:          {uchar size=((dst_byte>>4)&0x3)-1;
                        SetRegistr7(cmd.Op1,dst_byte,size);
                        mr.dtyp=btp_type[size];
                        mr.flags|=URB_LDA|URB_LDA2;// ����, �� �����!
                        break;
                        }
// ���室�
case T900_jp_cond:      if((dst_byte&0xF)==0x8)cmd.itype=T900_jp;
case T900_call:         // ��⠢��� ��� �᫮���
                        SetCondOp(cmd.Op1,dst_byte&0xF);
                        mr.flags|=URB_LDA;      // ����, �� �����!
                        break;
// ������
case T900_ret_cond:     // ���� ���� �.�. = 0xb0
                        if(code!=0xB0)return(0);
                        if((dst_byte&0xF)==0x8)cmd.itype=T900_ret;
                        SetCondOp(cmd.Op1,dst_byte&0xF);
                        return(cmd.size);

// �� ��稥 ANDCF,....
default:                SetImm8Op(cmd.Op1,dst_byte&7);
                        break;
}
// ����襬 ����⥫� � ���࠭�
MemRefToOp(cmd.Operands[memrefop],mr);
return(cmd.size);
}
//===========================================================================
static int SRCAnalyser(uchar code)
{
uchar src_byte;
uchar memrefop=1;        // ����� ���࠭�� - ��뫪� �� ������
uchar reg_size;         // ࠧ��� ॣ���� 0,1,2
MemRefDef mr;   // ����⥫� ��뫪� �� ������
static const uchar aa[]={
        255,      0,         255,      255,
        T900_ld,  0,         T900_ex,  254,
        T900_mul, T900_muls, T900_div, T900_divs,
        T900_inc, T900_dec,  0,        253,
        T900_add, T900_add,  T900_adc, T900_adc,
        T900_sub, T900_sub,  T900_sbc, T900_sbc,
        T900_and, T900_and,  T900_xor, T900_xor,
        T900_or,  T900_or,   T900_cp,  T900_cp};

// ���쬥� ��뫪� �� ������
if(LoadMemRef(mr,code)==0)return(0);
// ����稬 ���� ���� ����樨
src_byte=ua_next_byte();
cmd.itype=aa[(src_byte>>3)&0x1F];
reg_size=(code>>4)&3;
switch(cmd.itype){
// ��� ������権
case 0: return(0);
// �ॡ���� ���. ࠧ��� - ᢠ���
case 255:       switch(src_byte){
                // �� ��稥 - �訡���
                default:        return(0);
                // push
                case 4:         cmd.itype=T900_push;
                                memrefop=0;
                                break;
                // rld
                case 6:         cmd.itype=T900_rld;
                                SetRegistr7(cmd.Op1,1,0);
                                break;
                // rrd
                case 7:         cmd.itype=T900_rrd;
                                SetRegistr7(cmd.Op1,1,0);
                                break;
                // ldi
                case 0x10:      cmd.itype=T900_ldi;
                                mr.inc_size|=URB_UINC;
                                mr.base_reg--;
                                MemRefToOp(cmd.Op1,mr);
                                mr.base_reg++;
                                if(reg_size)cmd.itype++;
                                break;
                // ldir
                case 0x11:      cmd.itype=T900_ldir;
                                mr.inc_size|=URB_UINC;
                                mr.base_reg--;
                                MemRefToOp(cmd.Op1,mr);
                                mr.base_reg++;
                                if(reg_size)cmd.itype++;
                                break;
                // ldd
                case 0x12:      cmd.itype=T900_ldd;
                                mr.inc_size|=URB_UDEC;
                                mr.base_reg--;
                                MemRefToOp(cmd.Op1,mr);
                                mr.base_reg++;
                                if(reg_size)cmd.itype++;
                                break;
                // lddr
                case 0x13:      cmd.itype=T900_lddr;
                                mr.inc_size|=URB_UDEC;
                                mr.base_reg--;
                                MemRefToOp(cmd.Op1,mr);
                                mr.base_reg++;
                                if(reg_size)cmd.itype++;
                                break;
                // cpi
                case 0x14:      cmd.itype=T900_cpi;
                                mr.inc_size|=URB_UINC;
                                if(reg_size)    SetRegistr7(cmd.Op1,0,1);
                                else            SetRegistr7(cmd.Op1,1,0);
                                break;
                // cpir
                case 0x15:      cmd.itype=T900_cpir;
                                mr.inc_size|=URB_UINC;
                                if(reg_size)    SetRegistr7(cmd.Op1,0,1);
                                else            SetRegistr7(cmd.Op1,1,0);
                                break;
                // cpd
                case 0x16:      cmd.itype=T900_cpd;
                                mr.inc_size|=URB_UDEC;
                                if(reg_size)    SetRegistr7(cmd.Op1,0,1);
                                else            SetRegistr7(cmd.Op1,1,0);
                                break;
                // cpdr
                case 0x17:      cmd.itype=T900_cpdr;
                                mr.inc_size|=URB_UDEC;
                                if(reg_size)    SetRegistr7(cmd.Op1,0,1);
                                else            SetRegistr7(cmd.Op1,1,0);
                                break;
                // ld
                case 0x19:      if(code&0x10) cmd.itype=T900_ldw;
                                else cmd.itype=T900_ld;
                                SetDirectMemRef(cmd.Op1,2);
                                break;
                }
                break;
// add � ��稥
case 254:       cmd.itype=Add_List[src_byte&7];
                SetImmData(cmd.Op2,1<<((code>>4)&3));
                // ���ࠢ�� ⨯ ������樨 �� ᫮��
                if(reg_size!=0)cmd.itype++;
                memrefop=0;
                break;
// ᤢ���  xxxx (mem)
case 253:       cmd.itype=Shift_List1[src_byte&7];
                // ���ࠢ�� ⨯ ������樨 �� ᫮��
                if(reg_size!=0)cmd.itype++;
                memrefop=0;
                break;
// ���६���
case T900_inc:
case T900_dec:  SetImm3Op(cmd.Op1,src_byte);
                // ���ࠢ�� ⨯ ������樨 �� ᫮��
                if(reg_size!=0)cmd.itype++;
                break;

case T900_ld:   SetRegistr7(cmd.Op1,src_byte,reg_size);
                break;
// mul/div
case T900_mul:
case T900_div:
case T900_muls:
case T900_divs: SetRegistr7(cmd.Op1,(reg_size==0)?(src_byte&7)/2:src_byte,reg_size+1);
                break;
// ex
case T900_ex:   SetRegistr7(cmd.Op2, src_byte,reg_size);
                memrefop=0;
                break;
// add � ��稥
case T900_add:
case T900_adc:
case T900_sub:
case T900_sbc:
case T900_and:
case T900_xor:
case T900_or:
case T900_cp:   if(src_byte&0x8)memrefop=0;
                SetRegistr7(cmd.Operands[1-memrefop],src_byte,reg_size);
                break;
}
// ����襬 ����⥫� � ���࠭�
MemRefToOp(cmd.Operands[memrefop],mr);
return(cmd.size);
}

static void ClearOperand(op_t &op)
{
op.dtyp=dt_byte;
op.type=o_void;
op.specflag1=0;
op.specflag2=0;
op.offb=0;
op.offo=0;
//op.flags=0;
op.reg=0;
op.value=0;
op.addr=0;
op.specval=0;
}


//----------------------------------------------------------------------
// ���������
int idaapi T900_ana(void)
{
  ClearOperand(cmd.Op1);       // ���� ���࠭� - �����誠
  ClearOperand(cmd.Op2);       // ��ன ���࠭� - �����誠
  ClearOperand(cmd.Op3);       // ��ன ���࠭� - �����誠

  // ����稬 ���� ���� ������樨
  uchar code = ua_next_byte();
  // ࠧ����� ����࠭�⢮ �� ��� ���
  if(code&0x80){
        // �஢�ਬ �᪫�祭��
        if((code&0xF8)==0xF8){
                // ������� SWI (F8-FF)
                cmd.itype=T900_swi;
                // ⮫쪮 ���� ���࠭� - ����� ���뢠���
                SetImm8Op(cmd.Op1,code&7);
                // ���� ���뢠��� - ���� �� ࠡ�⠥�
                cmd.Op1.addr=0xFFFF00+(code&7)*4;
				cmd.Op1.value=cmd.Op1.addr;
                return(cmd.size);
        }
        if(code==0xF7){
                // ������� LDX
                cmd.itype=T900_ldx;
                // �ய��⨬ �㫥��� ����
                if(ua_next_byte()!=0)return(0);
                // ���� � ॣ�����
                SetDirectMemRef(cmd.Op1, 1);
                // �ய��⨬ �㫥��� ����
                if(ua_next_byte()!=0)return(0);
                // �����
                SetImmData(cmd.Op2,1);
                // ����� �ᥣ�� 6 ����
                return((cmd.size=6));
        }
        // ���������騥 ���� ������権 (C6, D6, E6, F6)
        if((code & 0xCF)==0xC6)return(0);
        // ����让 ��ଠ��� reg (C8,D8,E8)
        if((code&0x48)==0x48)return(RegAnalyser(code));
        // �� ���� reg ? (C7, D7, E7, F7)
        if((code&0xCF)==0xC7)return(RegAnalyser(code));
        // ��⠫�� memref
        // ᥣ����� dst (B0, B8, F0)
        if((code&0xB0)==0xB0)return(DSTAnalyser(code));
        // �� src � ࠧ����� �����
        return(SRCAnalyser(code));
  }
  // ������ ���� - ࠧ����ࠧ�� �।
  else  if(code<0x20){
        static const uchar FirstOp[]={
        T900_nop,  T900_normal, T900_push, T900_pop,
        T900_max,  T900_halt,   T900_ei,   T900_reti,
        T900_ld,   T900_push,   T900_ldw,  T900_pushw,
        T900_incf, T900_decf,   T900_ret,  T900_retd,
        T900_rcf,  T900_scf,    T900_ccf,  T900_zcf,
        T900_push, T900_pop,    T900_ex,   T900_ldf,
        T900_push, T900_pop,    T900_jp,   T900_jp,
        T900_call, T900_call,   T900_calr, 0};

        switch((cmd.itype=FirstOp[code])){
        // �������⨬��
        case 0x00:      return(0);

        case T900_push:
        case T900_pop:  switch(code&0x18){
                        case 0x00:      cmd.Op1.type=o_phrase;
                                        cmd.Op1.phrase=fSR;
                                        break;
                        // push only
                        case 0x08:      SetImmData(cmd.Op1, 1);
                                        break;
                        // xxx A
                        case 0x10:      SetRegistr7(cmd.Op1,1,0);
                                        break;
                        // xxx F
                        case 0x18:      cmd.Op1.type=o_phrase;
                                        cmd.Op1.phrase=fSF;
                                        break;
                        }
                        break;
        // ei
        case T900_ei:   // ᫥� ���� - �����।�⢥���
                        SetImmData(cmd.Op1, 1);
                        if(cmd.Op1.value==7){
                                cmd.itype=T900_di;
                                cmd.Op1.type=o_void;
                        }
                        break;
        // ld (n),n
        case T900_ld:   SetDirectMemRef(cmd.Op1,1);
                        SetImmData(cmd.Op2, 1);
                        break;

        // ldw
        case T900_ldw:  SetDirectMemRef(cmd.Op1,1);
                        SetImmData(cmd.Op2, 2);
                        break;
        // pushW
        // retd
        case T900_pushw:
        case T900_retd: SetImmData(cmd.Op1, 2);
                        break;
        // ex F,F'
        case T900_ex:   cmd.Op1.type=o_phrase;
                        cmd.Op1.phrase=fSF;
                        cmd.Op2.type=o_phrase;
                        cmd.Op2.phrase=fSF1;
                        break;
        // ldf
        case T900_ldf:  SetImmData(cmd.Op1,1);
                        break;

        case T900_jp:
        case T900_call: SetJmp(cmd.Op1,2+(code&1));
                        cmd.Op1.specflag1|=URB_LDA;
                        break;

        // callr 16
        case T900_calr: SetJmp(cmd.Op1,-2);
                        cmd.Op1.specflag1|=URB_LDA;
                        break;
        }
  }else switch(code&0x78){
  // ld
  case 0x20:
  case 0x30:
  case 0x40:    cmd.itype=T900_ld;
                SetRegistr7(cmd.Op1,code,(code>>4)-2);
                SetImmData (cmd.Op2,1<<((code>>4)-2));
                break;
  // push
  case 0x28:
  case 0x38:    cmd.itype=T900_push;
                SetRegistr7(cmd.Op1,code,(code>>4)-1);
                break;
  // pop
  case 0x48:
  case 0x58:    cmd.itype=T900_pop;
                SetRegistr7(cmd.Op1,code,(code>>4)-3);
                break;
  // reserved
  case 0x50:    return(0);
  // JR
  case 0x60:
  case 0x68:    if((code&0xF)==0x8)cmd.itype=T900_jr;
                else cmd.itype=T900_jr_cond;
                SetCondOp(cmd.Op1,code&0xF);
                SetJmp(cmd.Op2,-1);
                cmd.Op2.specflag1|=URB_LDA;
                break;
  // JRL
  case 0x70:
  case 0x78:    if((code&0xF)==0x8)cmd.itype=T900_jrl;
                else cmd.itype=T900_jrl_cond;
                SetCondOp(cmd.Op1,code&0xF);
                SetJmp(cmd.Op2,-2);
                cmd.Op2.specflag1|=URB_LDA;
                break;
  }
  return cmd.size;
}
