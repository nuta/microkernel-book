#pragma once
#include <kernel/arch.h>

// mstatusレジスタのフィールド
#define MSTATUS_MIE      (1 << 3)      // interrupts are enabled in M-mode
#define MSTATUS_MPP_S    (0b01 << 11)  // MPP field value for S-mode
#define MSTATUS_MPP_MASK (0b11 << 11)  // MPP field mask

// mieレジスタのフィールド
#define MIE_MTIE (1 << 7)  // M-mode timer interrupt-enable bit

// sstausレジスタのフィールド
#define SSTATUS_SIE  (1 << 1)   // interrupt-enable bits
#define SSTATUS_SPIE (1 << 5)   // interrupt-enable bit active prior to the trap
#define SSTATUS_SPP  (1 << 8)   // previous privilege mode
#define SSTATUS_SUM  (1 << 18)  // permit Supervisor User Memory access

// sieレジスタのフィールド
#define SIE_SSIE (1 << 1)  // supervisor-level software interrupts
#define SIE_STIE (1 << 5)  // supervisor-level timer interrupts
#define SIE_SEIE (1 << 9)  // supervisor-level external interrupts

// sipレジスタのフィールド
#define SIP_SSIP (1 << 1)  // supervisor software interrupt-pending

// scauseレジスタのフィールド
#define SCAUSE_S_SOFT_INTR        ((1L << 31) | 1)
#define SCAUSE_S_EXT_INTR         ((1L << 31) | 9)
#define SCAUSE_INS_MISS_ALIGN     0
#define SCAUSE_INST_ACCESS_FAULT  1
#define SCAUSE_ILLEGAL_INST       2
#define SCAUSE_BREAKPOINT         3
#define SCAUSE_LOAD_ACCESS_FAULT  5
#define SCAUSE_AMO_MISS_ALIGN     6
#define SCAUSE_STORE_ACCESS_FAULT 7
#define SCAUSE_ENV_CALL           8
#define SCAUSE_INST_PAGE_FAULT    12
#define SCAUSE_LOAD_PAGE_FAULT    13
#define SCAUSE_STORE_PAGE_FAULT   15

// satpレジスタのフィールド
#define SATP_MODE_SV32 (1u << 31)  // Sv32モード
#define SATP_PPN_MASK  0x3fffffff
#define SATP_PPN_SHIFT 12

// Core Local Interrupt (CLINT) のメモリマップトレジスタ
#define CLINT_PADDR 0x2000000
#define CLINT_SIZE  0x10000
#define CLINT_MTIME (CLINT_PADDR + 0xbff8)
#define CLINT_MTIMECMP(hartid)                                                 \
    (CLINT_PADDR + 0x4000 + sizeof(uint64_t) * (hartid))

// CLINTのうちタイマー関連レジスタへのアクセス用マクロ
#define MTIMECMP                                                               \
    ((volatile uint64_t *) arch_paddr_to_vaddr(CPUVAR->arch.mtimecmp))
#define MTIME ((volatile uint64_t *) arch_paddr_to_vaddr(CPUVAR->arch.mtime))

// 1ミリ秒ごとにmtimeレジスタの値がどれだけ進むか。QEMUのタイマーからとった値。
#define MTIME_PER_1MS 10000

// mtimeレジスタの値の差からticksに変換するマクロ。
#define MTIME_TO_TICKS(mtime_diff)                                             \
    (((unsigned) (mtime_diff)) / (MTIME_PER_1MS / (TICK_HZ / 1000)))

// Advanced Core Local Interruptor (ACLINT) のメモリマップトレジスタ
#define ACLINT_SSWI_PADDR 0x2f00000
#define ACLINT_SSWI_SETSSIP(hartid)                                            \
    (ACLINT_SSWI_PADDR + (sizeof(uint32_t) * (hartid)))

// mepcレジスタへの書き込み関数
static inline void write_mepc(uint32_t value) {
    __asm__ __volatile__("csrw mepc, %0" ::"r"(value));
}

// mstatusレジスタからの読み込み関数
static inline uint32_t read_mstatus(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mstatus" : "=r"(value));
    return value;
}

// mstatusレジスタへの書き込み関数
static inline void write_mstatus(uint32_t value) {
    __asm__ __volatile__("csrw mstatus, %0" ::"r"(value));
}

// mhartidレジスタからの読み込み関数
static inline uint32_t read_mhartid(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mhartid" : "=r"(value));
    return value;
}

// mieレジスタからの読み込み関数
static inline uint32_t read_mie(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mie" : "=r"(value));
    return value;
}

// mieレジスタへの書き込み関数
static inline void write_mie(uint32_t value) {
    __asm__ __volatile__("csrw mie, %0" ::"r"(value));
}

// mtvecレジスタへの書き込み関数
static inline void write_mtvec(uint32_t value) {
    __asm__ __volatile__("csrw mtvec, %0" ::"r"(value));
}

// mscratchレジスタへの書き込み関数
static inline void write_mscratch(uint32_t value) {
    __asm__ __volatile__("csrw mscratch, %0" ::"r"(value));
}

// sieレジスタからの読み込み関数
static inline uint32_t read_sie(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sie" : "=r"(value));
    return value;
}

// sieレジスタへの書き込み関数
static inline void write_sie(uint32_t value) {
    __asm__ __volatile__("csrw sie, %0" ::"r"(value));
}

// sipレジスタからの読み込み関数
static inline uint32_t read_sip(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sip" : "=r"(value));
    return value;
}

// sipレジスタへの書き込み関数
static inline void write_sip(uint32_t value) {
    __asm__ __volatile__("csrw sip, %0" ::"r"(value));
}

// medelegレジスタからの読み込み関数
static inline uint32_t read_medeleg(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, medeleg" : "=r"(value));
    return value;
}

// midelegレジスタからの読み込み関数
static inline uint32_t read_mideleg(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mideleg" : "=r"(value));
    return value;
}

// medelegレジスタへの書き込み関数
static inline void write_medeleg(uint32_t value) {
    __asm__ __volatile__("csrw medeleg, %0" ::"r"(value));
}

// midelegレジスタへの書き込み関数
static inline void write_mideleg(uint32_t value) {
    __asm__ __volatile__("csrw mideleg, %0" ::"r"(value));
}

// pmpaddr0レジスタへの書き込み関数
static inline void write_pmpaddr0(uint32_t value) {
    __asm__ __volatile__("csrw pmpaddr0, %0" ::"r"(value));
}

// pmpcfg0レジスタへの書き込み関数
static inline void write_pmpcfg0(uint32_t value) {
    __asm__ __volatile__("csrw pmpcfg0, %0" ::"r"(value));
}

// stvecレジスタへの書き込み関数
static inline void write_stvec(uint32_t value) {
    __asm__ __volatile__("csrw stvec, %0" ::"r"(value));
}

// sepcレジスタからの読み込み関数
static inline uint32_t read_sepc(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sepc" : "=r"(value));
    return value;
}

// sepcレジスタへの書き込み関数
static inline void write_sepc(uint32_t value) {
    __asm__ __volatile__("csrw sepc, %0" ::"r"(value));
}

// sstatusレジスタからの読み込み関数
static inline uint32_t read_sstatus(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sstatus" : "=r"(value));
    return value;
}

// sstatusレジスタへの書き込み関数
static inline void write_sstatus(uint32_t value) {
    __asm__ __volatile__("csrw sstatus, %0" ::"r"(value));
}

// scauseレジスタからの読み込み関数
static inline uint32_t read_scause(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, scause" : "=r"(value));
    return value;
}

// stvalレジスタからの読み込み関数
static inline uint32_t read_stval(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, stval" : "=r"(value));
    return value;
}

// satpレジスタからの読み込み関数
static inline uint32_t read_satp(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, satp" : "=r"(value));
    return value;
}

// satpレジスタへの書き込み関数
static inline void write_satp(uint32_t value) {
    __asm__ __volatile__("csrw satp, %0" ::"r"(value));
}

// sscratchレジスタへの読み込み関数
static inline void write_sscratch(uint32_t value) {
    __asm__ __volatile__("csrw sscratch, %0" ::"r"(value));
}

// tpレジスタへの書き込み関数
static inline void write_tp(uint32_t value) {
    __asm__ __volatile__("mv tp, %0" ::"r"(value));
}

// sfence.vma命令
static inline void asm_sfence_vma(void) {
    __asm__ __volatile__("sfence.vma zero, zero" ::: "memory");
}

// mret命令
static inline void asm_mret(void) {
    __asm__ __volatile__("mret");
}

// wfi命令
static inline void asm_wfi(void) {
    __asm__ __volatile__("wfi");
}

// 指定された物理アドレスに1バイトを書き込む。必ず1バイト幅のメモリアクセス命令を使う。
static inline void mmio_write8_paddr(paddr_t paddr, uint8_t val) {
    vaddr_t vaddr = arch_paddr_to_vaddr(paddr);
    __asm__ __volatile__("sb %0, (%1)" ::"r"(val), "r"(vaddr) : "memory");
}

// 指定された物理アドレスに2バイトを書き込む。必ず2バイト幅のメモリアクセス命令を使う。
static inline void mmio_write16_paddr(paddr_t paddr, uint32_t val) {
    vaddr_t vaddr = arch_paddr_to_vaddr(paddr);
    __asm__ __volatile__("sh %0, (%1)" ::"r"(val), "r"(vaddr) : "memory");
}

// 指定された物理アドレスに4バイトを書き込む。必ず4バイト幅のメモリアクセス命令を使う。
static inline void mmio_write32_paddr(paddr_t paddr, uint32_t val) {
    vaddr_t vaddr = arch_paddr_to_vaddr(paddr);
    __asm__ __volatile__("sw %0, (%1)" ::"r"(val), "r"(vaddr) : "memory");
}

// 指定された物理アドレスから1バイトを読み込む。必ず1バイト幅のメモリアクセス命令を使う。
static inline uint8_t mmio_read8_paddr(paddr_t paddr) {
    vaddr_t vaddr = arch_paddr_to_vaddr(paddr);
    uint8_t val;
    __asm__ __volatile__("lb %0, (%1)" : "=r"(val) : "r"(vaddr));
    return val;
}

// 指定された物理アドレスから2バイトを読み込む。必ず2バイト幅のメモリアクセス命令を使う。
static inline uint32_t mmio_read16_paddr(paddr_t paddr) {
    vaddr_t vaddr = arch_paddr_to_vaddr(paddr);
    uint32_t val;
    __asm__ __volatile__("lh %0, (%1)" : "=r"(val) : "r"(vaddr));
    return val;
}

// 指定された物理アドレスから4バイトを読み込む。必ず4バイト幅のメモリアクセス命令を使う。
static inline uint32_t mmio_read32_paddr(paddr_t paddr) {
    vaddr_t vaddr = arch_paddr_to_vaddr(paddr);
    uint32_t val;
    __asm__ __volatile__("lw %0, (%1)" : "=r"(val) : "r"(vaddr));
    return val;
}
