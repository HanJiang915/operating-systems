TITLE Designing low-level I/O procedures     (Program5.asm)

; Author: Ivan Timothy Halim
; Course / Project ID CS271           Date:08/11/2018
; Description: This program displays list using low-level I/O procedures

INCLUDE Irvine32.inc

NO_LOOPS = 10   ;numer of inputs to loop
STRSIZE = 20    ;max size of input strings

;-----------------------------------------------------
getString MACRO buffer, prompt_string
;
; Displays a variable, using its known attributes.
; Receives: 1: A string to store the string in.
;           2: Address of a prompt
;-----------------------------------------------------
    push    edx
    push    ecx
    displayString OFFSET prompt_string
    mov     edx, buffer
    mov     ecx, STRSIZE - 1
    call    ReadString
    pop     ecx
    pop     edx
ENDM

;-----------------------------------------------------
displayString MACRO buffer
;
; Displays a string.
; Receives: address of a string.
;-----------------------------------------------------
    push    edx
    mov     edx, buffer
    call    WriteString
    pop     edx
ENDM

.data
array           DWORD   NO_LOOPS DUP(?)
intro1          BYTE    "PROGRAMMING ASSIGNMENT 5: Designing low-level I/O procedures", 0
intro2          BYTE    "Written by: Ivan Timothy Halim", 0
intro3          BYTE    "Please provide 10 unsigned decimal integers.",0
intro4          BYTE    "Each number needs to be small enough to fit inside a 32 bit register.", 0
intro5          BYTE    "After you have finished inputting the raw numbers I will display a list",0
intro6          BYTE    "of the integers, their sum, and their average value. ",0
prompt1         BYTE    "Please enter an unsigned number: ", 0
error           BYTE    "ERROR: You did not enter an unsigned number or your number was too big.", 0
results1        BYTE    "You entered the following numbers: ", 0
results2        BYTE    "The sum of these numbers is: ", 0
results3        BYTE    "The average is: ", 0
goodBye         BYTE    "Thanks for playing!", 0
spacer          BYTE    ", ",0

.code
;-----------------------------------------------------
main PROC
;-----------------------------------------------------
    displayString OFFSET intro1
    call    CrLf
    displayString OFFSET intro2
    call    CrLf
    call    CrLf
    displayString OFFSET intro3
    call    CrLf
    displayString OFFSET intro4
    call    CrLf
    displayString OFFSET intro5
    call    CrLf
    displayString OFFSET intro6
    call    CrLf
    call    CrLf
    mov     ecx, NO_LOOPS               ;global variable
    mov     edi, OFFSET array           ;array[0]
mainloop:
    push    edi                         ;push the array
    call    ReadVal
    mov     eax, [edi]
    add     edi, 4                      ;incriment array
    add     eax, ebx
    jc      overflow
    loop    mainloop
    jmp     skipend
overflow:
    displayString OFFSET error
    call    CrLf
skipend:
    push    NO_LOOPS                    ;size of array
    push    OFFSET array
    call    Results
    call    CrLf
    displayString OFFSET goodBye
    call    CrLf
    exit
main ENDP

;-----------------------------------------------------
ReadVal PROC
; Converts a string of user input to an integer.
; Receives: address of a dword integer
; Returns: user-specified integer in address
;-----------------------------------------------------
    push    ebp
    mov     ebp, esp
    jmp     skip1
errormess:
    displayString OFFSET error
    call    CrLf
    jmp     skip1
errormess2:
    displayString OFFSET error
    pop     eax
    call    CrLf
skip1:
    mov     esi, [ebp+8]
    getString esi, prompt1
    mov     eax, 0
    push    eax
reads:
    mov     eax, 0
    LODSB
    cmp     al, 0
    je      endread                     ;NULL terminate
    pop     ebx                         ;restore calculated value
    push    eax                         ;save ascii char
    mov     eax, ebx
    mov     ebx, 10
    mul     ebx                         ;multiply result by 10
    jc      errormess2
    mov     edx, eax                    ;store result in edx
    pop     eax                         ;reload ascii char
    cmp     al, 48                      ;cmp to ascii 0
    jl      errormess
    cmp     al, 57                      ;cmp to ascii 9
    jg      errormess
    mov     ah, 48
    sub     al, ah                      ;convert ascii to dec
    mov     ah, 0
    add     eax, edx
    jc      errormess2
    push    eax                         ;save calculated value
    jmp     reads
endread:
    pop     eax
    mov     esi, [ebp+8]
    mov     [esi], eax
    pop     ebp
    ret 4
ReadVal ENDP

;-----------------------------------------------------
WriteVal PROC
; Converts a numeric value to a string of digits and displays output.
; Receives: address of a dword unsigned integer
; Returns: console output
;-----------------------------------------------------
    push    ebp
    mov     ebp, esp
    pushad
    sub     esp, 2                      ;make space for the character string
    mov     eax, [ebp+8]
    lea     edi, [ebp-2]                ;LEA to access the local address
    mov     ebx, 10
    mov     edx, 0
    div     ebx                         ;divide input by 10
    cmp     eax, 0
    jle     endwrite                    ;end recursion when eax = 0
    push    eax
    call    WriteVal
endwrite:
    mov     eax, edx
    add     eax, 48                     ;convert to ascii
    stosb                               ;store in edi
    mov     eax, 0                      ;null terminator
    stosb
    sub     edi, 2                      ;reset edi
    displayString edi
    add     esp, 2
    popad
    pop     ebp
    ret     4
WriteVal ENDP

;-----------------------------------------------------
Results PROC
; Calculates the sum and mean of ints in an array.
; Receives: address of an array, size of the array
; Returns: console output of the sum and mean
;-----------------------------------------------------
    push    ebp
    mov     ebp, esp
    mov     esi, [ebp+8]
    mov     ecx, [ebp+12]
    sub     esp, 4
    mov     edx, 0                      ;use eax to calculate the sum
    call    CrLf
    displayString OFFSET results1
    call    CrLf
    jmp     s1
resultsLoop:
    displayString OFFSET spacer
s1:
    push    [esi]
    call    WriteVal
    mov     ebx, [esi]
    add     edx, ebx
    add     esi, 4
    loop    resultsLoop
    call    CrLf
    displayString OFFSET results2
    push    edx                         ;display the sum
    call    WriteVal
    call    CrLf
    displayString OFFSET results3
    mov     eax, edx
    mov     edx, 0
    mov     ebx, [ebp+12]
    div     ebx
    push    eax
    call    WriteVal
    call    CrLf
    add     esp, 4
    pop     ebp
    ret     8
Results ENDP

END main