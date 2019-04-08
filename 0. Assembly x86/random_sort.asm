TITLE Sorting Random Integers     (Program4.asm)

; Author: Ivan Timothy Halim
; Course / Project ID CS271           Date:07/29/2018
; Description: This program generates and sorts random integers

INCLUDE Irvine32.inc

.data
intro           BYTE    "       Sorting Random Integers      by Ivan Timothy Halim", 0
ec1             BYTE    "**EC: Use a recursive algorithm (merge sort)", 0
intro1          BYTE    "This program generates random numbers in the range [100 .. 999],", 0
intro2          BYTE    "displays the original list, sorts the list, and calculates the", 0
intro3          BYTE    "median value. Finally, it displays the list sorted in descending order.", 0
prompt1         BYTE    "How many numbers should be generated? [10 .. 200]: ", 0
prompt2         BYTE    "Invalid input", 0
goodbye1        BYTE    "Results certified by Ivan Timothy Halim. Goodbye.", 0
unsorted        BYTE    "The unsorted random numbers:", 0
median          BYTE    "The median is ", 0
sorted          BYTE    "The sorted list:", 0
request         DWORD   ?                       ;integer to be entered by user
rowPosition     DWORD   1                       ;An integer that specifies the position of the number in a row (1-10)
array           DWORD   200     DUP(?)          ;An array to store all the random numbers
half1           DWORD   101     DUP(0)          ;An array to store the first half of the original array
half2           DWORD   101     DUP(0)          ;An array to store the second half of the original array
first           DWORD   0                       ;An integer that specifies the first element of the array to be sorted
midpoint        DWORD   ?                       ;An integer that specifies the midpoint of the array to be sorted
boolean         DWORD   ?                       ;An integer that stores bool values (true or false)
period          BYTE    '.'

.code
main PROC
    call    Randomize
    call    introduction
    push    OFFSET request
    push    OFFSET boolean
    call    getData
    push    request
    push    OFFSET array
    call    fillArray
    push    OFFSET unsorted
    push    rowPosition
    push    request
    push    OFFSET array
    call    displayList
    push    OFFSET array
    push    OFFSET half1
    push    OFFSET half2
    push    first
    mov     eax, request
    sub     eax, 1
    push    eax
    push    OFFSET midpoint
    call    mergeSort
    push    OFFSET median
    push    request
    push    OFFSET array
    call    displayMedian
    push    OFFSET sorted
    push    rowPosition
    push    request
    push    OFFSET array
    call    displayList
    call    farewell
    exit
main ENDP

;Procedure to display an introduction
;receives: none
;returns: display introduction
;preconditions: intro, ec1, intro1, intro2, intro3 has been declared and initialized
;registers changed: edx
introduction PROC
    mov     edx, OFFSET intro
    call    WriteString
    call    CrLf
    mov     edx, OFFSET ec1
    call    WriteString
    call    CrLf
    mov     edx, OFFSET intro1
    call    WriteString
    call    CrLf
    mov     edx, OFFSET intro2
    call    WriteString
    call    CrLf
    mov     edx, OFFSET intro3
    call    WriteString
    call    CrLf
    call    CrLf
    ret
introduction ENDP

;Procedure to get data from user
;receives: address of request, address of boolean
;returns: user-entered integer in request memory
;preconditions: request and boolean have been declared
;registers changed: eax, edi, edx
getData PROC
    push    ebp
    mov     ebp, esp
    mov     edi, [ebp+12]
repeatGetData:
    mov     edx, OFFSET prompt1
    call    WriteString
    call    ReadInt
    push    [ebp+8]
    call    validate
    push    eax
    push    edi
    mov     edi, [ebp+8]
    mov     eax, [edi]
    cmp     eax, 1
    je      endGetData
    mov     edx, OFFSET prompt2
    call    WriteString
    call    CrLf
    pop     edi
    pop     eax
    jmp     repeatGetData
endGetData:
    pop     edi
    pop     eax
    mov     [edi], eax
    call    CrLf
    pop     ebp
    ret     8
getData ENDP

;Procedure to validate if the user-entered integer is out of range
;receives: address of boolean
;returns: boolean value in boolean memory (1 if within range, 0 if out of range)
;preconditions: boolean has been declared, eax contains the integer to be validated
;registers changed: none
validate    PROC USES eax edi
    push    ebp
    mov     ebp, esp
    mov     edi, [ebp+16]
    mov     eax, 1
    mov     [edi], eax
    mov     eax, [ebp+8]
    cmp     eax, 10                 ;If the user-entered integer is less than 10 or greater than 200
    jl      outRange                ;Then the number is out of range
    cmp     eax, 200
    jg      outRange
    jmp     endValidate
outRange:
    mov     eax, 0
    mov     [edi], eax
endValidate:
    pop     ebp
    ret     4
validate    ENDP

;Procedure to fill the array with random numbers
;receives: an integer to specify how many random numbers, address of array
;returns: array filled with random numbers
;preconditions: array has been declared, integer doesn't exceed array size
;registers changed: esi, ecx, eax
fillArray   PROC
    push    ebp
    mov     ebp, esp
    mov     esi, [ebp+8]
    mov     ecx, [ebp+12]
fillArrayLoop:
    mov     eax, 900
    call    RandomRange
    add     eax, 100
    mov     [esi], eax
    add     esi, 4
    loop    fillArrayLoop
    pop     ebp
    ret     8
fillArray   ENDP

;Procedure to display the array
;receives: an integer to specify row position, an integer to specify how many elements, address of array, address of prompt
;returns: array displayed
;preconditions: array has been declared and initialized, prompt has been declared and initialized, integer doesn't exceed array size, row position is 1
;registers changed: esi, ecx, eax, edx, al
displayList PROC
    push    ebp
    mov     ebp, esp
    mov     edx, [ebp+20]
    call    WriteString
    call    CrLf
    mov     esi, [ebp+8]
    mov     ecx, [ebp+12]
displayListLoop:
    mov     eax, [ebp+16]
    cmp     eax, 10
    jle     continue
    call    CrLf                    ;if row position is greater than 10, move to a new line
    mov     eax, 1                  ;reset row position to 1
    mov     [ebp+16], eax
continue:
    mov     eax, [esi]
    call    WriteDec
    mov     al, 9
    call    WriteChar
    mov     eax, 1
    add     [ebp+16], eax
    add     esi, 4
    loop    displayListLoop
    call    CrLf
    call    CrLf
    mov     eax, 1
    mov     [ebp+16], eax
    pop     ebp
    ret     16
displayList ENDP

;Procedure to recursively sort the array
;receives: address of array, address of half1, address of half2, integer that specifies the first element of the array,
;           integer that specifies the last element of the array, address of midpoint
;returns: array sorted in descending order
;preconditions: array has been declared and initialized, half1, half2 and midpoint has been declared,
;               integer doesn't exceed array size
;registers changed: eax, ebx, edx
mergeSort   PROC
    push    [edi]
    push    edi
    push    ebp
    mov     ebp, esp
;if (low < high)
    mov     eax, [ebp+24]
    cmp     eax, [ebp+20]
    jge     endMergeSort
;calculate the midpoint ((low+high)/2)
    mov     edi, [ebp+16]
    mov     eax, [ebp+24]
    add     eax, [ebp+20]
    cdq
    mov     ebx, 2
    div     ebx
    mov     [edi], eax
;mergeSort(low, mid)
    push    [ebp+36]
    push    [ebp+32]
    push    [ebp+28]
    push    [ebp+24]
    push    [edi]
    push    [ebp+16]
    call    mergeSort
;mergeSort(mid+1, high)
    mov     eax, 1
    add     [edi], eax
    push    [ebp+36]
    push    [ebp+32]
    push    [ebp+28]
    push    [edi]
    push    [ebp+20]
    push    [ebp+16]
    call    mergeSort
    mov     eax, 1
    sub     [edi], eax
;merge(low, mid, high)
    push    [ebp+36]
    push    [ebp+32]
    push    [ebp+28]
    push    [ebp+24]
    push    [ebp+20]
    push    [edi]
    call    merge
endMergeSort:
    pop     ebp
    pop     edi
    pop     [edi]
    ret     24
mergeSort   ENDP

;Procedure to merge two sorted arrays into one sorted array
;receives: address of array, address of half1, address of half2, integer that specifies the first element of the array,
;           integer that specifies the last element of the array, integer that specifies the midpoint of the array
;returns: arrays merged into one sorted array
;preconditions: array has been declared and initialized, half1, half2 has been declared,
;               integer doesn't exceed array size, midpoint has been calculated
;registers changed: eax, ecx, esi, ebx
merge   PROC
    push    ebp
    mov     ebp, esp
;half1                              ;First we're going to split the array into two arrays: half1 and half2
    mov     eax, [ebp+8]            ;half1 has the size of mid-low+1
    sub     eax, [ebp+16]           ;Fill half1 with the elements of the first half of the array
    add     eax, 1
    mov     ecx, eax
    mov     esi, [ebp+24]
half1Loop:
    push    esi
    mov     esi, [ebp+28]
    mov     eax, [ebp+8]            ;half1[i] = array[i+low], we add low because the first element of the array
    add     eax, 1                  ;is going to change depending on which part of the array that we are going to sort
    sub     eax, ecx                ;since i = size of half1 - ecx,
    mov     ebx, 4                  ;i + low = (mid-low+1) - ecx + low = mid + 1 - ecx
    mul     ebx                     ;multiply i by 4 because DWORD has the size of 4 bytes
    mov     eax, [esi+eax]
    pop     esi
    mov     [esi], eax
    add     esi, 4
    loop    half1Loop
    mov     eax, 0                  ;We're going to set the last element of half1 to 0, to mark the endpoint of half1
    mov     [esi], eax
;half2                              ;half2 has the size of high-mid
    mov     eax, [ebp+12]           ;Fill half2 with the elements of the second half of the array
    sub     eax, [ebp+8]
    mov     ecx, eax
    mov     esi, [ebp+20]
half2Loop:
    push    esi
    mov     esi, [ebp+28]
    mov     eax, [ebp+12]           ;half2[i] = array[i+low+half1], we add size of half1 because we are sorting the second half of the array
    add     eax, 1                  ;since i = size of half2 - ecx
    sub     eax, ecx                ;i + low + half1 = (high-mid) - ecx + low + (mid-low+1) = high + 1 - ecx
    mov     ebx, 4                  ;multiply i by 4 because the size of DWORD is 4 bytes
    mul     ebx
    mov     eax, [esi+eax]
    pop     esi
    mov     [esi], eax
    add     esi, 4
    loop    half2Loop
    mov     eax, 0                  ;set the last element of half2 to 0, to mark the endpoint of half2
    mov     [esi], eax
;reordering
    mov     eax, [ebp+12]           ;we're going to start filling in the elements of the original array
    sub     eax, [ebp+16]           ;we want to change all the elements from the first element to the last element
    add     eax, 1                  ;therefore the for loop is going to be for(i=low,i<=high,i++)
    mov     ecx, eax                ;The number of counts for this loop is going to be high-low+1
reorderLoop:
    mov     esi, [ebp+24]
    mov     eax, [esi]              ;We're going to compare half1 and half2 element by element
    mov     esi, [ebp+20]           ;First, let's compare the first element of half1 with the first element of half2
    cmp     eax, [esi]
    jle     array2
    mov     esi, [ebp+24]           ;If the first element of half1 is greater than the first element of half2
    push    [esi]                   ;Since we are sorting the array in descending order
    mov     eax, [ebp+12]           ;move the first element of half1 to the original array
    add     eax, 1
    sub     eax, ecx
    mov     ebx, 4
    mul     ebx
    mov     esi, [ebp+28]
    pop     [esi+eax]
    mov     esi, [ebp+24]
    mov     eax, 0
    mov     [esi], eax
    mov     eax, 4                  ;Add 4 to the address of half1, so that the next time we're comparing half1 and half2
    add     [ebp+24], eax           ;We'll be comparing the second element of half1 to the first element of half2
    jmp     endReorder
array2:
    mov     esi, [ebp+20]           ;If the first element of half2 is greater than the first element of half1
    push    [esi]                   ;Since we are sorting the array in descending order
    mov     eax, [ebp+12]           ;move the first element of half2 to the original array
    add     eax, 1
    sub     eax, ecx
    mov     ebx, 4
    mul     ebx
    mov     esi, [ebp+28]
    pop     [esi+eax]
    mov     esi, [ebp+20]
    mov     eax, 0
    mov     [esi], eax
    mov     eax, 4                  ;Add 4 to the address of half2, so that the next time we're comparing half1 and half2
    add     [ebp+20], eax           ;We'll be comparing the second element of half2 to the first element of half1
endReorder:
    loop    reorderLoop
    pop     ebp
    ret 24
merge       ENDP

;Procedure to display the median of the array
;receives: address of array, address of median, an integer that specifies the number of elements in the array
;returns: median displayed
;preconditions: array has been declared, initialized and sorted, median has been declared, integer doesn't exceed array size
;registers changed: edx, esi, eax, ebx, al
displayMedian PROC
    push    ebp
    mov     ebp, esp
    mov     edx, [ebp+16]
    call    WriteString
    mov     esi, [ebp+8]            ;To calculate the median, there's going to be two cases
    mov     eax, [ebp+12]           ;If the number of elements is even or odd
    sub     eax, 1                  ;We're going to determine if the number of elements is even or odd
    cdq                             ;By looking at the remainder when it is divided by 2
    mov     ebx, 2                  ;If the remainder is zero, then number of elements is even
    div     ebx                     ;Otherwise, the number of elements is odd
    push    eax
    mov     eax, edx
    cmp     eax, 0
    jne     evenNum
    pop     eax                     ;If the number of elements in the array is odd
    mov     ebx, 4                  ;Then the median is simply going to be the middle element of the array
    mul     ebx
    mov     eax, [esi+eax]
    call    WriteDec
    jmp     oddNum
evenNum:
    pop     eax                     ;If the number of elements in the array is even
    mov     ebx, 4                  ;Then the median is going to be the average of the two middle elements of the array
    mul     ebx
    mov     ebx, [esi+eax]
    add     eax, 4
    mov     eax, [esi+eax]
    add     eax, ebx
    cdq
    mov     ebx, 2
    div     ebx
    call    WriteDec
    mov     al, period
    call    WriteChar
    mov     eax, edx
    mov     ebx, 10
    mul     ebx
    cdq
    mov     ebx, 2
    div     ebx
    call    WriteDec
oddNum:
    mov     al, period
    call    WriteChar
    call    CrLf
    call    CrLf
    pop     ebp
    ret     12
displayMedian ENDP

;Procedure to display the farewell message
;receives: none
;returns: farewell message displayed
;preconditions: goodbye1 has been declared and initialized
;registers changed: edx
farewell    PROC
    mov     edx, OFFSET goodbye1
    call    WriteString
    call    CrLf
    ret
farewell    ENDP

END main