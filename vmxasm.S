.intel_syntax noprefix
.extern vmexit_handler

.global __mystore_gdt
.global __mystore_idt
.global __myread_cr0
.global __myread_cr3
.global __myread_cr4
.global __read_es
.global __read_ss
.global __read_cs
.global __read_ds
.global __read_fs
.global __read_gs
.global __segment_limit 
.global __read_ldtr
.global vm_entrypoint
.global __load_ar
.global __read_tr
.global __vmlaunch
.global __vmcall
.global get_rsp
.global __vmread
.global __vmwrite
.global __vmxon
.global __vmxoff
.global __vmclear
.global __vmptrld
.global get_rflags
.global __mycpuid
.global __send_stop_cpuid
.global __bit_scan_forward
.global detach 

.macro save_regs
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
.endm

.macro restore_regs
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
.endm

__mystore_gdt:
    sgdt [rdi]
    ret

__mystore_idt:
    sidt [rdi]
    ret

__myread_cr0:
    mov rax, cr0
    ret

__myread_cr3:
    mov rax, cr3
    ret

__myread_cr4:
    mov rax, cr4
    ret

__read_ldtr:
    sldt ax
    ret

__read_tr:
    str ax
    ret

__read_cs:
    mov ax, cs
    ret

__read_ss:
    mov ax, ss
    ret

__read_ds:
    mov ax, ds
    ret

__read_es:
    mov ax, es
    ret

__read_fs:
    mov ax, fs
    ret

__read_gs:
    mov ax, gs
    ret

__segment_limit:
    lsl rax, edi
    ret

__load_ar:
    lar rax, rdi
    jz __load_ar_no_error
    xor rax, rax
__load_ar_no_error:
    ret

vm_entrypoint:
    save_regs
    mov rdi, [rsp + 0x78] // pointer to the vcpu_ctx at top of the stack

    sub rsp, 0x60 // Fortunalty we're aligned on 0x10 bytes 

    movaps xmmword ptr [rsp], xmm0
    movaps xmmword ptr [rsp + 0x10], xmm1
    movaps xmmword ptr [rsp + 0x20], xmm2
    movaps xmmword ptr [rsp + 0x30], xmm3
    movaps xmmword ptr [rsp + 0x40], xmm4
    movaps xmmword ptr [rsp + 0x50], xmm5

    mov rsi, rsp // guest_state

    sub rsp, 0x20
    call vmexit_handler
    add rsp, 0x20

    movaps xmm0, xmmword ptr [rsp]
    movaps xmm1, xmmword ptr [rsp + 0x10]
    movaps xmm2, xmmword ptr [rsp + 0x20]
    movaps xmm3, xmmword ptr [rsp + 0x30]
    movaps xmm4, xmmword ptr [rsp + 0x40]
    movaps xmm5, xmmword ptr [rsp + 0x50]

    add rsp, 0x60

    restore_regs

    vmresume
    jmp vmerror

vmerror:
    ud2

detach:
    mov rsp, rdi

    movaps xmm0, xmmword ptr [rsp]
    movaps xmm1, xmmword ptr [rsp + 0x10]
    movaps xmm2, xmmword ptr [rsp + 0x20]
    movaps xmm3, xmmword ptr [rsp + 0x30]
    movaps xmm4, xmmword ptr [rsp + 0x40]
    movaps xmm5, xmmword ptr [rsp + 0x50]

    add rsp, 0x60

    restore_regs

    push [rax + 0x10]
    popfq
    mov rsp, [rax]
    push [rax + 0x8]
    xor rax, rax
    ret

__vmlaunch:
    xor rax, rax
    vmlaunch
    setz al
    setc al
    ret

__vmcall:
    vmcall
    ret

get_rsp:
    mov rax, rsp
    ret

__vmread:
    xor rax, rax
    vmread [rsi], rdi
    setna al
    ret

__vmwrite:
    xor rax, rax
    vmwrite rdi, rsi
    setna al
    ret

__vmxon:
    push rdi
    vmxon [rsp]
    setna al
    pop rdi
    ret

__vmxoff:
    vmxoff
    ret

__vmclear:
    push rdi
    vmclear [rsp] 
    setna al
    pop rdi
    ret

__vmptrld:
    push rdi
    vmptrld [rsp]
    setna al
    pop rdi
    ret

get_rflags:
    pushfq
    pop rax
    ret

__mycpuid:
    push rax
    push rbx
    push rcx
    push rdx
    mov ecx, edx
    mov eax, esi
    cpuid
    mov dword ptr [rdi], eax
    mov dword ptr [rdi + 4], ebx
    mov dword ptr [rdi + 8], ecx
    mov dword ptr [rdi + 12], edx
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

__send_stop_cpuid:
    push rax
    push rcx
    mov rax, 0xdeadbeef
    mov rcx, 0xcafebabe
    cpuid
    pop rcx
    pop rax
    ret

__bit_scan_forward:
    bsf rax, rdi
    ret
