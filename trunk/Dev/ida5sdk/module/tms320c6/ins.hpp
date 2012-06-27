/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 *
 *      TMS320C6xx - VLIW (very long instruction word) architecture
 *
 */

#ifndef __INSTRS_HPP
#define __INSTRS_HPP

extern instruc_t Instructions[];

enum nameNum {

TMS6_null = 0,  // Unknown Operation
TMS6_abs,       // Absolute value
TMS6_add,       // Integer addition without saturation (signed)
TMS6_addu,      // Integer addition without saturation (unsigned)
TMS6_addab,     // Integer addition using addressing mode (byte)
TMS6_addah,     // Integer addition using addressing mode (halfword)
TMS6_addaw,     // Integer addition using addressing mode (word)
TMS6_addk,      // Integer addition 16bit signed constant
TMS6_add2,      // Two 16bit Integer adds on register halves
TMS6_and,       // Logical AND
TMS6_b,         // Branch
TMS6_clr,       // Clear a bit field
TMS6_cmpeq,     // Compare for equality
TMS6_cmpgt,     // Compare for greater than (signed)
TMS6_cmpgtu,    // Compare for greater than (unsigned)
TMS6_cmplt,     // Compare for less than (signed)
TMS6_cmpltu,    // Compare for less than (unsigned)
TMS6_ext,       // Extract and sign-extend a bit filed
TMS6_extu,      // Extract an unsigned bit field
TMS6_idle,      // Multicycle NOP with no termination until interrupt
TMS6_ldb,       // Load from memory (signed 8bit)
TMS6_ldbu,      // Load from memory (unsigned 8bit)
TMS6_ldh,       // Load from memory (signed 16bit)
TMS6_ldhu,      // Load from memory (unsigned 16bit)
TMS6_ldw,       // Load from memory (32bit)
TMS6_lmbd,      // Leftmost bit detection
TMS6_mpy,       // Signed Integer Multiply (LSB16 x LSB16)
TMS6_mpyu,      // Unsigned Integer Multiply (LSB16 x LSB16)
TMS6_mpyus,     // Integer Multiply Signed*Unsigned (LSB16 x LSB16)
TMS6_mpysu,     // Integer Multiply Unsigned*Signed (LSB16 x LSB16)
TMS6_mpyh,      // Signed Integer Multiply (MSB16 x MSB16)
TMS6_mpyhu,     // Unsigned Integer Multiply (MSB16 x MSB16)
TMS6_mpyhus,    // Integer Multiply Unsigned*Signed (MSB16 x MSB16)
TMS6_mpyhsu,    // Integer Multiply Signed*Unsigned (MSB16 x MSB16)
TMS6_mpyhl,     // Signed Integer Multiply (MSB16 x LSB16)
TMS6_mpyhlu,    // Unsigned Integer Multiply (MSB16 x LSB16)
TMS6_mpyhuls,   // Integer Multiply Signed*Unsigned (MSB16 x LSB16)
TMS6_mpyhslu,   // Integer Multiply Unsigned*Signed (MSB16 x LSB16)
TMS6_mpylh,     // Signed Integer Multiply (LSB16 x MB16)
TMS6_mpylhu,    // Unsigned Integer Multiply (LSB16 x MSB16)
TMS6_mpyluhs,   // Integer Multiply Signed*Unsigned (LSB16 x MSB16)
TMS6_mpylshu,   // Integer Multiply Unsigned*Signed (LSB16 x MSB16)
TMS6_mv,        // Move from register to register
TMS6_mvc,       // Move between the control file & register file
TMS6_mvk,       // Move a 16bit signed constant into register
TMS6_mvkh,      // Move a 16bit constant into the upper bits of a register
TMS6_mvklh,     // Move a 16bit constant into the upper bits of a register
TMS6_neg,       // Negate
TMS6_nop,       // No operation
TMS6_norm,      // Normalize
TMS6_not,       // Bitwise NOT
TMS6_or,        // Logical or
TMS6_sadd,      // Integer addition with saturation
TMS6_sat,       // Saturate 40bit value to 32bits
TMS6_set,       // Set a bit field
TMS6_shl,       // Arithmetic shift left
TMS6_shr,       // Arithmetic shift right
TMS6_shru,      // Logical shift left
TMS6_smpy,      // Integer multiply with left shift & saturation (LSB16*LSB16)
TMS6_smpyhl,    // Integer multiply with left shift & saturation (MSB16*LSB16)
TMS6_smpylh,    // Integer multiply with left shift & saturation (LSB16*MSB16)
TMS6_smpyh,     // Integer multiply with left shift & saturation (MSB16*MSB16)
TMS6_sshl,      // Shift left with saturation
TMS6_ssub,      // Integer substraction with saturation
TMS6_stb,       // Store to memory (signed 8bit)
TMS6_stbu,      // Store to memory (unsigned 8bit)
TMS6_sth,       // Store to memory (signed 16bit)
TMS6_sthu,      // Store to memory (unsigned 16bit)
TMS6_stw,       // Store to memory (32bit)
TMS6_sub,       // Integer substaraction without saturation (signed)
TMS6_subu,      // Integer substaraction without saturation (unsigned)
TMS6_subab,     // Integer subtraction using addressing mode (byte)
TMS6_subah,     // Integer subtraction using addressing mode (halfword)
TMS6_subaw,     // Integer subtraction using addressing mode (word)
TMS6_subc,      // Conditional subtract & shift (for division)
TMS6_sub2,      // Two 16bit integer subtractions on register halves
TMS6_xor,       // Exclusive OR
TMS6_zero,      // Zero a register

TMS6_last,

    };

#endif
