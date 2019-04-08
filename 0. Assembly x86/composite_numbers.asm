TITLE Composite Numbers     (Program3.asm)

; Author: Ivan Timothy Halim
; Course / Project ID CS271           Date:07/09/2018
; Description: This program calculates Composite Numbers

INCLUDE Irvine32.inc

MAX_SIZE = 2000
.data
intro           BYTE    "       Composite Numbers      by Ivan Timothy Halim", 0
ec1             BYTE    "**EC: Program displays the numbers in aligned columns.", 0
ec2             BYTE    "**EC: Program displays more composite numbers one page at a time.", 0
ec3             BYTE    "**EC: Program checks against only prime divisor.", 0
intro1          BYTE    "Enter the number of composite numbers you would like to see.", 0
intro2          BYTE    "I'll accept orders for up to 15390 composites.", 0
prompt1         BYTE    "Enter the number of composites to display [1 .. 15390]: ", 0
prompt2         BYTE    "Out of range. Try again.", 0
prompt3         BYTE    "Do you want to repeat? (1 for yes, 0 for no)", 0
goodbye1        BYTE    "Results certified by Ivan Timothy Halim. Goodbye.", 0
pause1          BYTE    "Press enter to continue . . . ", 0
num             DWORD   ?                       ;integer to be entered by user
num1            DWORD   ?                       ;To store current composite number
rowPosition     DWORD   ?                       ;An integer that specifies the position of the composite number in a row (1-10)
rowNumber       DWORD   ?                       ;An integer that specifies the row number
n               DWORD   ?                       ;An integer that specifies how many composite numbers have been printed
primeArray      DWORD   MAX_SIZE    DUP(?)      ;An array to store all the prime numbers found so far
count           DWORD   ?                       ;An integer that specifies how many prime number is in primeArray
boolean         DWORD   ?                       ;An integer that stores bool values (true or false)

.code
main PROC
    call    introduction
    call    userInstruction
returnLoop:
    call    getUserData
    call    displayComposite
    call    return
    mov     eax, boolean
    cmp     eax, 0
    je      L2
    call    CrLf
    jmp     returnLoop
L2:
    call    farewell
    exit
main ENDP

return  PROC
    call    CrLf
    call    CrLf
    mov     edx, OFFSET prompt3
    call    WriteString
    call    CrLf
error:
    call    ReadInt
    cmp     eax, 1
    je      yes
    cmp     eax, 0
    jne     error
    jmp     no
yes:
    mov     boolean, 1
    jmp     done
no:
    mov     boolean, 0
done:
    ret
return  ENDP

introduction PROC
    mov     edx, OFFSET intro
    call    WriteString
    call    CrLf
    mov     edx, OFFSET ec1
    call    WriteString
    call    CrLf
    mov     edx, OFFSET ec2
    call    WriteString
    call    CrLf
    mov     edx, OFFSET ec3
    call    WriteString
    call    CrLf
    call    CrLf
    ret
introduction ENDP

userInstruction PROC
    mov     edx, OFFSET intro1
    call    WriteString
    call    CrLf
    mov     edx, OFFSET intro2
    call    WriteString
    call    CrLf
    call    CrLf
    ret
userInstruction ENDP

getUserData PROC
repeatLoop:                         ;This procedure prompts the user to enter the number of composite numbers
    mov     edx, OFFSET prompt1
    call    WriteString
    call    ReadInt
    mov     num, eax
    call    validate
    mov     eax, boolean
    cmp     eax, 1
    je      endLoop
    mov     edx, OFFSET prompt2     ;If the user-entered integer is out of range
    call    WriteString             ;display a message to the user that the number is out of range
    call    CrLf
    jmp     repeatLoop              ;repeat the procedure
endLoop:
    call    CrLf
    ret
getUserData ENDP

validate    PROC                    ;This procedure validates if the user-entered integer is out of range
    mov     boolean, 1
    cmp     eax, 1                  ;If the user-entered integer is less than 1 or greater than 15390
    jl      outRange                ;Then the number is out of range
    cmp     eax, 15390
    jg      outRange
    jmp     endValidate
outRange:
    mov     boolean, 0
endValidate:
    ret
validate    ENDP

displayComposite PROC               ;This procedure checks if a number is a composite number and displays it if it is a composite number
    mov     num1, 2                 ;The first number is going to be 3, but we initialize num1 as 2 because we're going to increment it later
    mov     primeArray, 2           ;initialize the first prime number in primeArray as 2
    mov     count, 1                ;initialize count as 1 because there is 1 prime number in primeArray which is 2
    mov     n, 0                    ;initialize n as 0 because no composite number has been printed
    mov     rowPosition, 1          ;initialize rowPosition as 1 because it is the first position in a row
    mov     rowNumber, 1            ;initialize rowNumber as 1 because it is the first row
calculateComposite:
    inc     num1
    call    isComposite
    mov     eax, boolean
    cmp     eax, 1
    je      display
    mov     eax, count              ;If the number is not divisible by any prime number in primeArray,
    mov     ebx, 4                  ;it means that the number is a prime number.
    mul     ebx                     ;Put the number in the primeArray.
    mov     edi, eax
    mov     eax, num1               ;If the number is divisible by a prime number in primeArray
    mov     primeArray[edi], eax    ;it means that the number is a composite number.
    inc     count                   ;Display the composite number
    jmp     calculateComposite
display:
    cmp     rowPosition, 10
    jle     continue                ;If rowPosition is greater than 10
    call    CrLf                    ;move to the next row
    mov     rowPosition, 1          ;reset rowPosition to 1
    inc     rowNumber               ;increment rowNumber
    cmp     rowNumber, 26
    jle     continue                ;If rowNumber is greater than 26
    call    CrLf                    ;pause the system
    mov     edx, OFFSET pause1      ;reset rowNumber to 1
    call    WriteString
    call    ReadInt
    call    CrLf
    mov     rowNumber, 1
continue:
    mov     eax, num1
    call    WriteDec
    mov     al, 9
    call    WriteChar
    inc     rowPosition
    inc     n
    mov     eax, n
    cmp     eax, num
    jl      calculateComposite
    call    clearPrime
    ret
displayComposite ENDP

isComposite PROC                    ;This procedure validates if a number is a composite number
    mov     boolean, 0
    mov     ecx, count              ;For all numbers, check to see if the number is divisible
    mov     edi, 0                  ;by a prime number that is currently in primeArray
primeLoop:
    mov     eax, num1
    cdq
    mov     ebx, primeArray[edi]
    div     ebx
    add     edi, 4
    mov     eax, edx
    cmp     eax, 0
    je      composite
    loop    primeLoop
    jmp     endIsComposite
composite:
    mov     boolean, 1
endIsComposite:
    ret
isComposite ENDP

clearPrime  PROC
    mov     eax, count              ;This procedure resets primeArray to its pre-condition state
    sub     eax, 1                  ;This is because we want to repeat the program
    mov     ecx, eax
clearLoop:
    mov     eax, ecx
    mov     ebx, 4
    mul     ebx
    mov     edi, eax
    mov     primeArray[edi], 0
    loop    clearLoop
    ret
clearPrime  ENDP

farewell    PROC
    call    CrLf
    mov     edx, OFFSET goodbye1
    call    WriteString
    call    CrLf
    ret
farewell    ENDP

END main