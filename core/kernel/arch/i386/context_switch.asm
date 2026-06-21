global context_read_eip
global context_read_esp
global context_read_ebp
global context_read_eflags
global scheduler_context_switch

section .text
bits 32

context_read_eip:
    mov eax, [esp]
    ret

context_read_esp:
    mov eax, esp
    ret

context_read_ebp:
    mov eax, ebp
    ret

context_read_eflags:
    pushfd
    pop eax
    ret

; void scheduler_context_switch(
;     scheduler_context_t* old_context,
;     scheduler_context_t* next_context
; );
;
; scheduler_context_t layout:
;   +0  eip
;   +4  esp
;   +8  ebp
;   +12 eflags
;   +16 eax
;   +20 ebx
;   +24 ecx
;   +28 edx
;   +32 esi
;   +36 edi
;
; This saves and restores the core 32-bit general-purpose context.
scheduler_context_switch:
    pushad

    ; After pushad:
    ;   [esp + 0]  edi
    ;   [esp + 4]  esi
    ;   [esp + 8]  ebp
    ;   [esp + 12] original esp at pushad entry
    ;   [esp + 16] ebx
    ;   [esp + 20] edx
    ;   [esp + 24] ecx
    ;   [esp + 28] eax
    ;   [esp + 32] return address
    ;   [esp + 36] old_context
    ;   [esp + 40] next_context

    mov eax, [esp + 36]

    mov ecx, .resume_old_context
    mov [eax + 0], ecx

    mov ecx, [esp + 12]
    mov [eax + 4], ecx

    mov ecx, [esp + 8]
    mov [eax + 8], ecx

    pushfd
    pop ecx
    mov [eax + 12], ecx

    mov ecx, [esp + 28]
    mov [eax + 16], ecx

    mov ecx, [esp + 16]
    mov [eax + 20], ecx

    mov ecx, [esp + 24]
    mov [eax + 24], ecx

    mov ecx, [esp + 20]
    mov [eax + 28], ecx

    mov ecx, [esp + 4]
    mov [eax + 32], ecx

    mov ecx, [esp + 0]
    mov [eax + 36], ecx

    mov edx, [esp + 40]

    mov esp, [edx + 4]
    mov ebp, [edx + 8]

    push dword [edx + 0]

    push dword [edx + 12]
    popfd

    mov eax, [edx + 16]
    mov ebx, [edx + 20]
    mov ecx, [edx + 24]
    mov esi, [edx + 32]
    mov edi, [edx + 36]
    mov edx, [edx + 28]

    ret

.resume_old_context:
    ret