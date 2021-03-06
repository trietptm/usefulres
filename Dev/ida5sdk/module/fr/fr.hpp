
#ifndef __FR_HPP
#define __FR_HPP

#include "../idaidp.hpp"
#include "ins.hpp"
#include <diskio.hpp>
#include <frame.hpp>

// uncomment this for the final release
//#define __DEBUG__

// FR registers
enum fr_registers {

    // general purpose registers :

    rR0,
    rR1,
    rR2,
    rR3,
    rR4,
    rR5,
    rR6,
    rR7,
    rR8,
    rR9,
    rR10,
    rR11,
    rR12,
    rR13,
    rR14,
    rR15,

    // coprocessor registers :

    rCR0,
    rCR1,
    rCR2,
    rCR3,
    rCR4,
    rCR5,
    rCR6,
    rCR7,
    rCR8,
    rCR9,
    rCR10,
    rCR11,
    rCR12,
    rCR13,
    rCR14,
    rCR15,

    // dedicated registers :

    rPC,        // program counter
    rPS,        // program status
    rTBR,       // table base register
    rRP,        // return pointer
    rSSP,       // system stack pointer
    rUSP,       // user stack pointer
    rMDL,       // multiplication/division register (LOW)
    rMDH,       // multiplication/division register (HIGH)

    // system use dedicated registers
    rReserved6,
    rReserved7,
    rReserved8,
    rReserved9,
    rReserved10,
    rReserved11,
    rReserved12,
    rReserved13,
    rReserved14,
    rReserved15,

    // these 2 registers are required by the IDA kernel :

    rVcs,
    rVds
};

enum fr_phrases {
    fIGR,       // indirect general register
    fIRA,       // indirect relative address
    fIGRP,      // indirect general register with post-increment
    fIGRM,      // indirect general register with pre-decrement
    fR13RI,     // indirect displacement between R13 and a general register
};

// shortcut for a new operand type
#define o_reglist              o_idpspec0

// flags for insn.auxpref
#define INSN_DELAY_SHOT        0x00000001           // postfix insn mnem by ":D"

// flags for opt.specflag1
#define OP_DISPL_IMM_R14       0x00000001           // @(R14, #i)
#define OP_DISPL_IMM_R15       0x00000002           // @(R15, #i)
#define OP_ADDR_R              0x00000010           // read-access to memory
#define OP_ADDR_W              0x00000012           // write-access to memory

inline bool op_displ_imm_r14(op_t &op) { return op.specflag1 & OP_DISPL_IMM_R14; }
inline bool op_displ_imm_r15(op_t &op) { return op.specflag1 & OP_DISPL_IMM_R15; }

// exporting our routines
void header(void);
void footer(void);
int ana(void);
int emu(void);
void out(void);
bool outop(op_t &op);
void gen_segm_header(ea_t ea);
void interr(const char *file, const int line, const char *format, ...);
const ioport_t *find_sym(int address);
bool create_func_frame(func_t *pfn);
int get_frame_retsize(func_t *pfn);
bool is_sp_based(const op_t &x);
int is_align_insn(ea_t ea);

extern char device[];

#define IDA_ERROR(msg) {                                                     \
    interr(__FILE__, __LINE__, "%s", msg);                                   \
}

#define IDA_ASSERT(exp,msg) {                                                \
    if (!(exp)) {                                                            \
        interr(__FILE__, __LINE__, "Assertion '%s' failed (cause: %s)",      \
            #exp, msg);                                                      \
    }                                                                        \
}

#endif /* __FR_HPP */

