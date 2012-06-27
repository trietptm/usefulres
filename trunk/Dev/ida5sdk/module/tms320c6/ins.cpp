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

#include "tms6.hpp"

instruc_t Instructions[] = {

{ "",           0                               },      // Unknown Operation
{ "ABS",        CF_USE1|CF_CHG2                 },      // Absolute value
{ "ADD",        CF_USE1|CF_USE2|CF_CHG3         },      // Integer addition without saturation (signed)
{ "ADDU",       CF_USE1|CF_USE2|CF_CHG3         },      // Integer addition without saturation (unsigned)
{ "ADDAB",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer addition using addressing mode (byte)
{ "ADDAH",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer addition using addressing mode (halfword)
{ "ADDAW",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer addition using addressing mode (word)
{ "ADDK",       CF_USE1|CF_CHG2                 },      // Integer addition 16bit signed constant
{ "ADD2",       CF_USE1|CF_USE2|CF_CHG3         },      // Two 16bit Integer adds on register halves
{ "AND",        CF_USE1|CF_USE2|CF_CHG3         },      // Logical AND
{ "B",          CF_USE1                         },      // Branch
{ "CLR",        CF_USE1|CF_USE2|CF_CHG3         },      // Clear a bit field
{ "CMPEQ",      CF_USE1|CF_USE2|CF_CHG3         },      // Compare for equality
{ "CMPGT",      CF_USE1|CF_USE2|CF_CHG3         },      // Compare for greater than (signed)
{ "CMPGTU",     CF_USE1|CF_USE2|CF_CHG3         },      // Compare for greater than (unsigned)
{ "CMPLT",      CF_USE1|CF_USE2|CF_CHG3         },      // Compare for less than (signed)
{ "CMPLTU",     CF_USE1|CF_USE2|CF_CHG3         },      // Compare for less than (unsigned)
{ "EXT",        CF_USE1|CF_USE2|CF_CHG3         },      // Extract and sign-extend a bit filed
{ "EXTU",       CF_USE1|CF_USE2|CF_CHG3         },      // Extract an unsigned bit field
{ "IDLE",       CF_STOP                         },      // Multicycle NOP with no termination until interrupt
{ "LDB",        CF_USE1|CF_CHG2                 },      // Load from memory (signed 8bit)
{ "LDBU",       CF_USE1|CF_CHG2                 },      // Load from memory (unsigned 8bit)
{ "LDH",        CF_USE1|CF_CHG2                 },      // Load from memory (signed 16bit)
{ "LDHU",       CF_USE1|CF_CHG2                 },      // Load from memory (unsigned 16bit)
{ "LDW",        CF_USE1|CF_CHG2                 },      // Load from memory (32bit)
{ "LMBD",       CF_USE1|CF_USE2|CF_CHG3         },      // Leftmost bit detection
{ "MPY",        CF_USE1|CF_USE2|CF_CHG3         },      // Signed Integer Multiply (LSB16 x LSB16)
{ "MPYU",       CF_USE1|CF_USE2|CF_CHG3         },      // Unsigned Integer Multiply (LSB16 x LSB16)
{ "MPYUS",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Signed*Unsigned (LSB16 x LSB16)
{ "MPYSU",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Unsigned*Signed (LSB16 x LSB16)
{ "MPYH",       CF_USE1|CF_USE2|CF_CHG3         },      // Signed Integer Multiply (MSB16 x MSB16)
{ "MPYHU",      CF_USE1|CF_USE2|CF_CHG3         },      // Unsigned Integer Multiply (MSB16 x MSB16)
{ "MPYHUS",     CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Unsigned*Signed (MSB16 x MSB16)
{ "MPYHSU",     CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Signed*Unsigned (MSB16 x MSB16)
{ "MPYHL",      CF_USE1|CF_USE2|CF_CHG3         },      // Signed Integer Multiply (MSB16 x LSB16)
{ "MPYHLU",     CF_USE1|CF_USE2|CF_CHG3         },      // Unsigned Integer Multiply (MSB16 x LSB16)
{ "MPYHULS",    CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Signed*Unsigned (MSB16 x LSB16)
{ "MPYHSLU",    CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Unsigned*Signed (MSB16 x LSB16)
{ "MPYLH",      CF_USE1|CF_USE2|CF_CHG3         },      // Signed Integer Multiply (LSB16 x MB16)
{ "MPYLHU",     CF_USE1|CF_USE2|CF_CHG3         },      // Unsigned Integer Multiply (LSB16 x MSB16)
{ "MPYLUHS",    CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Signed*Unsigned (LSB16 x MSB16)
{ "MPYLSHU",    CF_USE1|CF_USE2|CF_CHG3         },      // Integer Multiply Unsigned*Signed (LSB16 x MSB16)
{ "MV",         CF_USE1|CF_CHG2                 },      // Move from register to register
{ "MVC",        CF_USE1|CF_CHG2                 },      // Move between the control file & register file
{ "MVK",        CF_USE1|CF_CHG2                 },      // Move a 16bit signed constant into register
{ "MVKH",       CF_USE1|CF_CHG2                 },      // Move a 16bit constant into the upper bits of a register
{ "MVKLH",      CF_USE1|CF_CHG2                 },      // Move a 16bit constant into the upper bits of a register
{ "NEG",        CF_USE1|CF_CHG2                 },      // Negate
{ "NOP",        CF_USE1                         },      // No operation
{ "NORM",       CF_USE1|CF_CHG2                 },      // Normalize
{ "NOT",        CF_USE1|CF_CHG2                 },      // Bitwise NOT
{ "OR",         CF_USE1|CF_USE2|CF_CHG3         },      // Logical or
{ "SADD",       CF_USE1|CF_USE2|CF_CHG3         },      // Integer addition with saturation
{ "SAT",        CF_USE1|CF_CHG2                 },      // Saturate 40bit value to 32bits
{ "SET",        CF_USE1|CF_USE2|CF_CHG3         },      // Set a bit field
{ "SHL",        CF_USE1|CF_USE2|CF_CHG3         },      // Arithmetic shift left
{ "SHR",        CF_USE1|CF_USE2|CF_CHG3         },      // Arithmetic shift right
{ "SHRU",       CF_USE1|CF_USE2|CF_CHG3         },      // Logical shift left
{ "SMPY",       CF_USE1|CF_USE2|CF_CHG3         },      // Integer multiply with left shift & saturation (LSB16*LSB16)
{ "SMPYHL",     CF_USE1|CF_USE2|CF_CHG3         },      // Integer multiply with left shift & saturation (MSB16*LSB16)
{ "SMPYLH",     CF_USE1|CF_USE2|CF_CHG3         },      // Integer multiply with left shift & saturation (LSB16*MSB16)
{ "SMPYH",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer multiply with left shift & saturation (MSB16*MSB16)
{ "SSHL",       CF_USE1|CF_USE2|CF_CHG3         },      // Shift left with saturation
{ "SSUB",       CF_USE1|CF_USE2|CF_CHG3         },      // Integer substraction with saturation
{ "STB",        CF_USE1|CF_CHG2                 },      // Store to memory (signed 8bit)
{ "STBU",       CF_USE1|CF_CHG2                 },      // Store to memory (unsigned 8bit)
{ "STH",        CF_USE1|CF_CHG2                 },      // Store to memory (signed 16bit)
{ "STHU",       CF_USE1|CF_CHG2                 },      // Store to memory (unsigned 16bit)
{ "STW",        CF_USE1|CF_CHG2                 },      // Store to memory (32bit)
{ "SUB",        CF_USE1|CF_USE2|CF_CHG3         },      // Integer substaraction without saturation (signed)
{ "SUBU",       CF_USE1|CF_USE2|CF_CHG3         },      // Integer substaraction without saturation (unsigned)
{ "SUBAB",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer subtraction using addressing mode (byte)
{ "SUBAH",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer subtraction using addressing mode (halfword)
{ "SUBAW",      CF_USE1|CF_USE2|CF_CHG3         },      // Integer subtraction using addressing mode (word)
{ "SUBC",       CF_USE1|CF_USE2|CF_CHG3         },      // Conditional subtract & shift (for division)
{ "SUB2",       CF_USE1|CF_USE2|CF_CHG3         },      // Two 16bit integer subtractions on register halves
{ "XOR",        CF_USE1|CF_USE2|CF_CHG3         },      // Exclusive OR
{ "ZERO",       CF_CHG1                         },      // Zero a register

  };

#ifdef __BORLANDC__
#if sizeof(Instructions)/sizeof(Instructions[0]) != TMS6_last
#error          No match:  sizeof(InstrNames) !!!
#endif
#endif
