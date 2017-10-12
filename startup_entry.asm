/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; $Id:: startup_entry.asm 8086 2011-09-13 11:20:35Z ing03005          $
; 
; Project: Generic 32x0 startup code
;
; Notes:
;     Realview 3.x and Keil MDK toolchain version
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 ; Software that is described herein is for illustrative purposes only  
 ; which provides customers with programming information regarding the  
 ; products. This software is supplied "AS IS" without any warranties.  
 ; NXP Semiconductors assumes no responsibility or liability for the 
 ; use of the software, conveys no license or title under any patent, 
 ; copyright, or mask work right to the product. NXP Semiconductors 
 ; reserves the right to make changes in the software without 
 ; notification. NXP Semiconductors also make no representation or 
 ; warranty that such application will be suitable for the specified 
 ; use without further testing or modification. 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/

    .global arm926ejs_reset

    .global board_hw_init

    /*; Stack start addresses*/
    .global __fiq_stack_top__
    .global __irq_stack_top__
    .global __abort_stack_top__
    .global __undef_stack_top__
    .global __system_stack_top__
    .global __svc_stack_top__

    /*; This is the user application that is called by the startup code
    ; once board initialization is complete */
    .global c_entry

/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Private defines and data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/

.include "setup_gnu.inc"

.EQU MODE_USR,	0x010
.EQU MODE_FIQ,	0x011
.EQU MODE_IRQ,	0x012
.EQU MODE_SVC,	0x013
.EQU MODE_ABORT,	0x017
.EQU MODE_UNDEF,	0x01b
.EQU MODE_SYSTEM,	0x01f
.EQU MODE_BITS,	0x01f
.EQU I_MASK,	0x080
.EQU F_MASK,	0x040
.EQU IF_MASK,   0x0C0
.EQU MODE_SVC_NI, 0x0D3

/* End of internal RAM */
.EQU END_OF_IRAM, IRAM_SIZE

/*; Masks used to disable and enable the MMU and caches */
.EQU MMU_DISABLE_MASK,	0xFFFFEFFA
.EQU MMU_ENABLE_MASK,	0x00001005
.EQU MMU_ICACHE_BIT,	0x1000

    .section .rodata

    .text
    .code 32   /*; Startup code*/
    .align 2

arm926ejs_reset:
    B     arm926ejs_entry
    B     .
    B     .
    B     .
    B     .
    B     .
    B     .
    B     .

load_sections:

    .global __text_start
    .global __text_end
    .global __text_load
    .global __rodata_start
    .global __rodata_end
    .global __rodata_load
    .global __data_start
    .global __data_end
    .global __data_load
    .global __iram_uncached_start
    .global __iram_uncached_end
    .global __iram_uncached_load

    .word __text_start
    .word __text_end
    .word __text_load
    .word __rodata_start
    .word __rodata_end
    .word __rodata_load
    .word __data_start
    .word __data_end
    .word __data_load
    .word __iram_uncached_start
    .word __iram_uncached_end
    .word __iram_uncached_load
    .word 0, 1, 2

zero_sections:
    .global __bss_start
    .global __bss_end

    .word __bss_start
    .word __bss_end
    .word 0, 1

/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Function: arm926ejs_entry
;
; Purpose: Reset vector entry point
;
; Description:
;     Various support functions based on defines.
;
; Parameters: NA
;
; Outputs; NA
;
; Returns: NA
;
; Notes: NA
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/
arm926ejs_entry:
    /*; Put the processor is in system mode with interrupts disabled */
    MOV   r0, #MODE_SVC_NI
    MSR   cpsr_cxsf, r0

    /*; Ensure the MMU is disabled */
    MRC   p15, 0, r6, c1, c0, 0
    LDR   r2,=MMU_DISABLE_MASK
    AND   r6, r6, r2
    MCR   p15, 0, r6, c1, c0, 0

    /*; Invalidate TLBs and invalidate caches */
    MOV   r7,#0
    MCR   p15, 0, r7, c8, c7, 0
    MCR   p15, 0, r7, c7, c7, 0

    /*; Enable instruction cache */
    ORR   r6, r6, #MMU_ICACHE_BIT
    MCR   p15, 0, r6, c1, c0, 0

    LDR   sp, =__svc_stack_top__

    /* ; Relocate hw config functions segment before using it */
    LDR   r7, =__hwconfig_load
    /* ; Get address to move image too */
    LDR   r0, =__hwconfig_start
    LDR   r1, =__hwconfig_end
hwconfigmove:
    CMP   r0, r1
    BEQ   hwconfigmove_exit
    LDRB  r2, [r7], #1
    STRB  r2, [r0], #1
    B     hwconfigmove
hwconfigmove_exit:

    /*; Initialize board */
    LDR   lr, =after_hwinit
    LDR   pc, =board_hw_init
after_hwinit:

    LDR   r3, =load_sections
load_copy_section_descriptor:
    LDR   r0, [r3], #4
    LDR   r1, [r3], #4
    LDR   r7, [r3], #4
    CMP   r0, #0
    BEQ   copy_sections_end
copy_section:
    CMP   r0, r1
    BEQ   load_copy_section_descriptor
    LDRB  r2, [r7], #1
    STRB  r2, [r0], #1
    B     copy_section
copy_sections_end:

    LDR   r3, =zero_sections
    MOV   r7, #0
load_zero_section_descriptor:
    LDR   r0, [r3], #4
    LDR   r1, [r3], #4
    CMP   r0, #0
    BEQ   zero_sections_end
zero_section:
    CMP   r0, r1
    BEQ   load_zero_section_descriptor
    STRB  r7, [r0], #1
    B     zero_section
zero_sections_end:

    /* ; Relocate sdram segment before using it */
    /*
    LDR   r7, =__sdram_load
    LDR   r0, =__sdram_start
    LDR   r1, =__sdram_end
sdrammove:
    CMP   r0, r1
    BEQ   sdrammove_exit
    LDRB  r2, [r7], #1
    STRB  r2, [r0], #1
    B     sdrammove
sdrammove_exit:
*/
    /* ; MMU page table is at the end of IRAM, last 16K */
    LDR   r0, =END_OF_IRAM
    SUB   r0, r0, #(16*1024)
    MCR   p15, 0, r0, c2, c0, 0
    bl    mmu_setup

    /*; Setup the Domain Access Control as all Manager
    ; Make all domains open, user can impose restrictions */
    MVN   r7, #0
    MCR   p15, 0, r7, c3, c0, 0

    /*; Setup jump to run out of virtual memory at location inVirtMem */
    LDR   r5, =inVirtMem

    /*; Enable the MMU with instruction and data caches enabled */
    LDR   r2,=MMU_ENABLE_MASK
    ORR   r6, r6, r2
    MCR   p15, 0, r6, c1, c0, 0

    /*; Jump to the virtual address */
    MOV   pc, r5

    /*; The following NOPs are to clear the pipeline after the MMU virtual
    ; address jump */
    NOP
    NOP
    NOP
inVirtMem:

  /*; The code is operating out of virtual memory now - register R3
    ; contains the virtual address for the top of stack space */

  /*; SVC stack was previously setup, use it's location as the
    ; reference for the other stacks */
    MOV   r3, sp
    SUB   r3, r3, #SVC_STACK_SIZE

    /*; All interrupts disabled at core for all modes */
    MOV   r1, #IF_MASK /*; No Interrupts */

    /*; Enter FIQ mode and setup the FIQ stack pointer */
    ORR   r0, r1, #MODE_FIQ
    MSR   cpsr_cxsf, r0
    LDR   sp, =__fiq_stack_top__

    /*; Enter IRQ mode and setup the IRQ stack pointer */
    ORR   r0, r1, #MODE_IRQ
    MSR   cpsr_cxsf, r0
    LDR   sp, =__irq_stack_top__

    /*; Enter Abort mode and setup the Abort stack pointer */
    ORR   r0, r1, #MODE_ABORT
    MSR   cpsr_cxsf, r0
    LDR   sp, =__abort_stack_top__

    /*; Enter Undefined mode and setup the Undefined stack pointer */
    ORR   r0, r1, #MODE_UNDEF
    MSR   cpsr_cxsf, r0
    LDR   sp, =__undef_stack_top__

    /*; Enter System mode and setup the User/System stack pointer */
    ORR   r0, r1, #MODE_SYSTEM
    MSR   cpsr_cxsf, r0
    LDR   sp, =__system_stack_top__

    /*; Re-enter SVC mode for runtime initialization */
    ORR   r0, r1, #MODE_SVC
    MSR   cpsr_cxsf, r0

    /*; 1-way jump to application */
    LDR  pc, =c_entry

    .END
