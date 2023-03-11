;version-0.0:
;  在Ubuntu20.04-64bit系统上实现了正数的+ *
;  对应算法分别包装在addWrapper, mulWrapper, 计算时使用了32bit寄存器
;  所有的输入输出以及帮助函数(设置xy值的除外)都是基于64bit寄存器的
;version-0.1:
;  补充对非法输入的处理:
;    错误的操作符：- / 
;    错误的操作数：小数
;version-1.0:
;  补充计算负数需要的变量: 见.data .bss
;  补充减法方法: subWrapper
;  修改_start setXandY addWrapper同- mulWrapper
;


global _start


SECTION .syscalls
; 设置一些系统调用的常量
SYS_WRITE equ 1
SYS_READ equ 0
SYS_EXIT equ 60
STDOUT equ 1
STDIN  equ 0


SECTION .data
msg1    db    'Please Input x and y:', 0h  ;0h是字符串的终止符
msg2    db    'Invalid', 0h
ZERO    db    '0', 0h
;;; 实现负数部分需要用到的变量
xSign  db      0, 0h
ySign  db      0, 0h
resSign:   db  0, 0h

SECTION .bss
Input:  resb    60
x:      resb    30
y:      resb    30
xLen:   resb    4
yLen:   resb    4
operator:      resb    4
addResult: resb    60
mulResult: resb    60
x_mul_pointer:  resb    4
y_mul_pointer:  resb    4
;;; 减法要用的变量
largePointer:  resb    4       ; 指向较大数字的高位, 用于做减法
subLength:    resb    4        ; 保存减法结果的长度
subbedLength:   resb   4       ; 正在减的长度，算数的时候用


SECTION .text
_start:
    mov     dword[x], 0
    mov     dword[y], 0
    mov     dword[xLen], 0
    mov     dword[yLen], 0
    mov     dword[xSign], 0
    mov     dword[ySign], 0
    mov     dword[resSign], 0
    mov     qword[addResult], 0
    mov     qword[mulResult], 0
    mov     dword[largePointer], 0
    mov     dword[subLength], 0
    mov     dword[subbedLength], 0
    call    getInput
    cmp     byte[operator], '+'    ; 判断操作符
    jnz     mulWarpper
    ; 负数部分
    mov     al, byte[xSign]
    cmp     al, byte[ySign]
    jnz     subWrapper             ; 异号 大数减小数
    ; 负数部分
    call    addWrapper             ; 同号 加法包装
    ret


clearResult:
    push    rbx
    push    rcx
    push    rdx
    mov     rbx, 60    ; rbx = length
    dec     rbx
    push    rax
    add     rax, rbx    ; rax指向最后一位
    mov     rbx, rax    ; 存到rbx， 现在rbx指向最后一位
    pop     rax
    push    rax
LoopClear:
    mov     byte[rbx], 0
    dec     rbx
    cmp     rax, rbx        ; 没相遇继续换
    jl      LoopClear
ReturnClear:
    pop     rax
    pop     rdx
    pop     rcx
    pop     rbx
    ret

; used only for test
testIO:
    push    rax
    mov     rax, msg2
    call    printStr
    call    printEnter
    pop     rax
    ret


; 加法入口
addWrapper:
    call    Adder
    mov     rax, addResult
    call    Reverse
    ; 判断是不是负数
    mov     eax, xSign      ; 如果是'-'就打印
    call    printStr
    mov     rax, addResult
    call    printStr
    call    printEnter
    call    clearResult
    call    _start


; x = x + y， 计算结果存到x上再存到addResult上
Adder:
    push    rax         ; 存正在加的位
    push    rbx         ; 存正在加的位
    push    rcx         ; 存结果地址addResult
    push    rdx         ; 存进位
    push    rdi         ; x末位
    push    rsi         ; y末位
    mov     edi, 0
    add     edi, x
    add     edi, dword[xLen]        ; 指向x末位
    dec     edi
    mov     esi, 0
    add     esi, y
    add     esi, dword[yLen]        ; 指向y末位
    dec     esi
    mov     ecx, addResult
    mov     edx, 0                  ; 进位初始化为0 dl
    mov     eax, 0
    mov     ebx, 0
addLoop:
    mov     al, byte[edi]
    mov     bl, byte[esi]
    add     al, bl        ; 本位相加
    add     al, dl        ; 加上进位
    add     al, 48        ; 补上48
    mov     dl, 0
    cmp     al, 57        ; 判断加法溢出
    jng     notOverFlow
    mov     dl, 1         ; 溢出修改进位 本位-10
    sub     al, 10
notOverFlow:
    mov     byte[ecx], al ; 先把结果大端存储一下
    ; 判断有没有加完
    cmp     edi, x        ; x加完了，把y剩余的位数加到x上
    jz      addY
    cmp     esi, y        ; y加完了，要处理进位
    jz      addIF
    ; 都没加完继续loop
    inc     ecx
    dec     edi
    dec     esi 
    jmp     addLoop
addY:   
    cmp     esi, y       ; 先看y加完没
    jz      endAdd
    dec     esi
    inc     ecx
    mov     al, byte[esi]
    add     al, dl
    add     al, 48
    mov     dl, 0
    cmp     al, 57
    jng     yNotOF
    mov     dl, 1
    sub     al, 10
yNotOF:
    mov     byte[ecx], al
    jmp     addY
addIF:    ; 处理进位
    cmp     edi, x   ; 最后把x加完了就结束循环
    jz      endAdd
    dec     edi
    inc     ecx
    mov     al, byte[edi]
    add     al, dl
    add     al, 48
    mov     dl, 0
    cmp     al, 57
    jng     xNotOF
    mov     dl, 1
    sub     al, 10
xNotOF:
    mov     byte[ecx], al
    jmp     addIF    
endAdd:
    inc     ecx           ; 判断最高位有没有进位
    cmp     dl, 0
    jz      ReturnAdd
    add     dl, 48
    mov     byte[ecx], dl
    jmp     ReturnAdd
ReturnAdd:
    pop     rsi
    pop     rdi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
    ret



; 乘法入口
mulWarpper:
    cmp    byte[x], 0
    jz     ZeroOp
    cmp    byte[y], 0
    jz     ZeroOp
    ; 判断有没有负号, 原本应该从notNegtive开始
    cmp     byte[x], 0      ; x = 0
    jz      notNegative
    cmp     byte[y], 0
    jz      notNegative
    ; 有0就不讨论sign了 否则打印乘法结果的符号
    mov     al, byte[xSign]
    xor     al, byte[ySign]
    mov     byte[resSign], al
    mov     eax, resSign
    call    printStr
notNegative:
    call    Mul
    mov     rax, mulResult
    call    Reverse
    call    printStr
    call    printEnter
    call    clearResult
    call    _start
ZeroOp:
    mov     rax, ZERO
    call    printStr
    call    printEnter
    call    _start


; eax->x ebx->y
Mul:
    push    rax     ; 存X当前位
    push    rbx     ; 存Y当前位
    push    rcx     ; 进位
    push    rdx     
    push    rdi     ; point to x last bit
    push    rsi     ; point to y last bit
    mov     edi, x
    add     edi, dword[xLen]
    dec     edi
    mov     esi, y
    add     esi, dword[yLen]
    dec     esi

    mov     dword[x_mul_pointer], 0     ; 表示x相对于最末位的偏移量
    mov     dword[y_mul_pointer], 0     ; 表示y相对于最末位的偏移量
    mov     ebx, 0    ; mul-src先置为0
    mov     ecx, 0    ; 进位置为0
    mov     edx, 0    ; 工具人
    mov     dl, 10
mulLoop:    ; 就标记一下后面不用
oneNum:
    mov     dl, 10    ; 每轮都要用10
    mov     ax, 0
    mov     al, byte[edi]      ; X当前位
    mov     bl, byte[esi]      ; Y当前位
    mul     bl                 ; ax = al * bl
    add     ax, cx             ; 加上进位
    div     dl                 ; AH存本位(/10的余数)，AL存进位(/10的商)
    ;保存本位
    mov     edx, dword[x_mul_pointer]    ; 用edx存一下正在计算这一位的数量级
    add     edx, dword[y_mul_pointer]
    add     edx, mulResult     ; 大端存储 edx = result里与正在计算的这一位同数量级的那位
    add     ah, byte[edx]      ; 与之前已有的相加,现在ah中保留着计算完的本位，由于加上了上一轮的，可能超过10
    cmp     ah, 10
    jl      mulNotOverFlow
    sub     ah, 10
    inc     al                 ; 存进位的al自增
mulNotOverFlow:       
    mov     byte[edx], ah
    movzx   cx, al             ; 无符号扩展，将进位保存在cx中
    cmp     esi, y
    jz      endOneTurn         ; 这一轮的y轮完了
    dec     esi                ; 没完就*更高位的y
    inc     dword[y_mul_pointer]
    jmp     oneNum
endOneTurn:
    mov     edx, dword[x_mul_pointer]
    add     edx, dword[y_mul_pointer]
    add     edx, mulResult
    inc     edx                 ;一轮加完了，要保存进位
    mov     byte[edx], al
    mov     cx, 0               ;下一轮初始进位为0
    ; 判断这轮y乘完了x乘没乘完
    cmp     edi, x
    jz      endMul
    ; 没乘完x升一位 y从末尾接着承
    dec     edi
    inc     dword[x_mul_pointer]
    mov     dword[y_mul_pointer], 0
    mov     esi, y                ; esi指向y最末位
    add     esi, dword[yLen]
    dec     esi
    jmp     oneNum
endMul:
    inc     dword[x_mul_pointer]    ;处理最后一位进位
    mov     edx, dword[x_mul_pointer]
    add     edx, dword[y_mul_pointer]
    add     edx, mulResult
    mov     byte[edx], al
    mov     eax, mulResult
    ; 计算结果到底有几位->ebx
    mov     ebx, 0                  ; 有0就不用算位数了直接就一位
    cmp     byte[x], 0
    jz      add48
    cmp     byte[y], 0
    jz      add48
    mov     ebx, dword[xLen]
    add     ebx, dword[yLen]        ; 结果的位数只有两种可能：1.x y位数之和; 2.x y位数之和减1
    ;单独判断最后一轮有没有进位
    dec     ebx
    cmp     byte[mulResult + ebx], 0
    jnz     add48                   ; 最后一位不为0，说明有进位, 给进位加48
    dec     ebx                     ; 没进位
add48:
    mov     al, byte[mulResult + ebx]
    add     al, 48
    mov     byte[mulResult + ebx], al
    cmp     ebx, 0
    jz      ReturnMul
    dec     ebx
    jmp     add48
ReturnMul:
    pop     rsi
    pop     rdi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
    ret



; 计算字符串长度, 字符串以'\0'结尾
; 函数会使用rbx, 结果保存在rax中
strLen:
    push    rbx
    mov     rbx, rax            ; 保存首地址rax到rbx
nextChar:                       ; 遍历字符串
    cmp     byte[rax], 0
    jz      meetZero
    inc     rax
    jmp     nextChar
meetZero:
    sub     rax, rbx            ; rax = rax - rbx
    pop     rbx
    ret


; 反转rax
Reverse:
    push    rbx
    push    rcx
    push    rdx
    push    rax
    call    strLen      ; 先计算一下rax(addResult)有多长
    mov     rbx, rax    ; rbx = length
    dec     rbx
    pop     rax         ; 获取一下首地址
    push    rax
    add     rax, rbx    ; rax指向最后一位
    mov     rbx, rax    ; 存到rbx， 现在rbx指向最后一位
    pop     rax
    push    rax
Loop:                       ; 开始反转
    mov     cl, byte[rax]   ; 交换rax rbx
    mov     dl, byte[rbx]
    mov     byte[rax], dl
    mov     byte[rbx], cl
    inc     rax
    dec     rbx
    cmp     rax, rbx        ; 没相遇继续换
    jl      Loop
ReturnReverse:
    pop     rax
    pop     rdx
    pop     rcx
    pop     rbx
    ret


; 获取输入的字符串
getInput:
    mov     rax, msg1
    call    printStr        ; 打印字符串
    call    printEnter      ; 打印换行符
    ;call sys_read
    mov     rsi, Input       ; read into Input
    mov     rdx , 60         ; maxium bytes
    mov     rax , SYS_READ
    mov     rdi , STDIN
    syscall
    ; test input
    ; mov     rax, Input
    ; call    printStr
    ; call    printEnter
    cmp    byte[Input], 'q'
    jz     quit
    ; set x, y
    call    setXAndY
    ; test x, y 
    ; if read ySign successfully then we read x and y successfully
    ; mov     rax, ySign
    ; call    printStr
    ret


; 打印字符串
; rax：字符串首地址
printStr:
    push    rdi
    push    rsi
    push    rdx
    mov     rsi, rax
    call    strLen      ; string length 保存在rax中
    ; call sys_write
    mov     rdx , rax
    mov     rax , SYS_WRITE
    mov     rdi , STDOUT
    syscall
    mov     rax, rsi    ; 还原rax
    pop     rdx
    pop     rsi
    pop     rdi
    ret


; 打印换行符
printEnter:
    push    rax
    mov     rax, 0Ah    ; 换行符
    push    rax         ; 用Stack拿到换行符地址
    mov     rax, rsp    ; 把栈顶指针esp（指向换行符）存到rax
    call    printStr    ; 打印换行符
    pop     rax
    pop     rax
    ret


; 解析x y
; operator存储计算符号 填补x y xSign ySign xLen yLen
setXAndY:
    push    rdx
    push    rsi
    push    rax
    push    rbx
    mov     esi, 0      
    mov     eax, 0      ; rsi存储在字符串中的偏移量
setX:
    ; 判断x符号 原本的setX直接从positive开始
    cmp     byte[Input + eax], '-'
    jnz     positive            ; 非负
    mov     byte[xSign], '-'
    inc     eax
    inc     esi                 ; esi在处理x时跟随eax 处理y时保存y的首地址方便存储到y中
    mov     ebx, -1             ; 存x的时候把-位置顶掉
positive:
    cmp     byte[Input + eax], 42
    jz      isMul
    cmp     byte[Input + eax], 43
    jz      isAdd
    ; 处理错误的操作符和浮点数
    cmp     byte[Input + eax], 45
    jz      error
    cmp     byte[Input + eax], 47
    jz      error
    cmp     byte[Input + eax], '.'
    jz      error
    ; 处理错误的操作符和浮点数
    mov     dl, byte[Input + eax]       ; dl low 8 bits of rdx
    sub     dl, 48                      ; dl = dl - 48    ; 48 is zero in ascii
    mov     byte[x + eax + ebx], dl     ; 加当前字节到x中 
    inc     dword[xLen]  
    inc     eax
    mov     esi, eax
    inc     esi
    ;jmp     setX
    jmp     positive
isMul:
    mov    byte[operator], '*'
    jmp    setY
isAdd:
    mov    byte[operator], '+'
    jmp    setY
setY:
    ; 检查符号 原本的setY不用检查符号直接从positive开始
    cmp     byte[Input + eax + 1], '-'
    jnz     positiveY                ; 非负
    mov     byte[ySign], '-'
    inc     eax                      ; 将eax指向负号后面inc
    inc     esi
positiveY:
    inc     eax                      ; rax point to operator or '-'
    cmp     byte[Input + eax], 10    ; 判断是不是换行
    jz      Return
    ; 处理错误的浮点数
    cmp     byte[Input + eax], '.'
    jz      error
    ; 处理错误的浮点数
    mov     dl, byte[Input + eax]
    sub     dl, 48
    sub     eax, esi
    mov     byte[y + eax], dl
    inc     dword[yLen]
    add     eax, esi
    jmp     setY
Return:
    pop     rbx
    pop     rax
    pop     rsi
    pop     rdx
    ret


; 错误的操作符如：- / 
; 错误的操作数
error:
    push    rax
    mov     rax, msg2
    call    printStr
    call    printEnter
    pop     rax
    call    _start


; exit
quit:
    push rax
    mov rax, SYS_EXIT
    pop rdi
    mov rdi, 0
    syscall

 
; 减法入口
subWrapper:
    push    rax
    push    rbx
    call    cmpXandY                     ; 大数->eax 小数->ebx 结果符号->resSign
    mov     dword[largePointer], eax    ; 大数地址->largePointer，便于判断是否减完了
    ; 开减 结果addResult
    call    Sub
    mov     eax, addResult              ; 大端存储的结果
    call    Reverse                     ; 调用反转函数反转eax
    ; 打印计算结果
    mov     eax, resSign
    call    printStr
    mov     eax, addResult
    call    printStr
    call    printEnter
    call    clearResult
    pop     rbx
    pop     rax
    call    _start


; 比较x y大小
; ret: 大数eax 小数ebx 大数最后一位edi 小数最后一位esi
cmpXandY:
    mov     eax, dword[xLen]
    mov     ebx, dword[yLen]
    push    rcx
    push    rdx
    ; compare length
    cmp     eax, ebx
    jg      xGreater            ; x位数更多，所以直接返回
    je      sameLength          ; x和y位数相同，特殊处理
    jl      yGreater            ; x位数少，x小，交换x和y
xGreater:                       ; 结果符号就是x符号
    mov     al, byte[xSign]
    mov     byte[resSign], al
    mov     eax, x
    mov     ebx, y
    mov     edi, x              
    add     edi, dword[xLen]
    dec     edi                 ;  edi <- 大数最后一位
    mov     esi, y
    add     esi, dword[yLen]    ;  esi <- 小数最后一位
    dec     esi
    jmp     ReturnCmp
sameLength:                     ; 从高位开始比较，ecx是最后一位偏移量，edx保存遍历时偏移的位数
    mov     ecx, dword[xLen]
    dec     ecx
    mov     edx, -1
cmpLoop:
    cmp     ecx, edx
    jz      equal
    inc     edx
    mov     al, byte[x + edx]
    cmp     al, byte[y + edx]
    jg      xGreater
    jl      yGreater
    je      cmpLoop
equal:
    mov     eax, x
    mov     ebx, y
    mov     byte[resSign], 0    ; 两数相同做减法结果为0
    mov     edi, x
    add     edi, dword[xLen]
    dec     edi
    mov     esi, y
    add     esi, dword[yLen]
    dec     esi
    jmp     ReturnCmp
yGreater:
    mov     al, byte[ySign]
    mov     byte[resSign], al    ; 结果与y同号
    mov     eax, y
    mov     ebx, x
    mov     edi, y
    add     edi, dword[yLen]
    dec     edi
    mov     esi, x
    add     esi, dword[xLen]
    dec     esi
    jmp     ReturnCmp
ReturnCmp:
    pop     rdx
    pop     rcx
    ret


; eax - ebx
; edi指向eax最后一位，esi指向ebx最后一位
; ret: 减法结果ecx
Sub:
    push    rcx     ; 保存减法结果, 在addResult中用大端存储
    push    rdx     ; 保存借位
    mov     ecx, addResult
    mov     edx, 0  ; dl = 0
subLoop:
    inc     dword[subbedLength]      ; 增加正在减的结果长度
    mov     al, byte[edi]
    mov     bl, byte[esi]

    sub     al, bl      ; 同位相减->al
    sub     al, dl      ; 减去借位
    mov     dl, 0
    cmp     al, 0       ; 看看够不够减
    jnl     enough
    add     al, 10      ; 不够减 借位补10
    mov     dl, 1       ; 借位置1
enough:
    cmp     al, 0       ; 如果al等于0，要么是正好够减，要么是减完了（134-133）
    jz      checkLarger    ; 判断大数减没减完
    ; al不是0，先把减法结果位数更新(存到subLength中)
    push    rax
    mov     eax, dword[subbedLength]
    mov     dword[subLength], eax
    pop     rax
checkLarger:
    mov     byte[ecx], al                  ; 先把结果大端存储到addResult里
    cmp     edi, dword[largePointer]       ; 判断edi（正在减的大数的位的地址）是否等于largePointer（大数首地址）
    jz      returnSub                      ; 减完了
    inc     ecx
    dec     edi
    dec     esi
    jmp     subLoop                        ; 没减完jmp回去 增加正在减的结果长度subbedLength
returnSub:
    mov     ecx, addResult 
; 从头开始 结果的每位加上48得到对应数字ascii码
LoopAscii:
    mov     al, byte[ecx]
    add     al, 48
    mov     byte[ecx], al
    inc     ecx
    dec     dword[subLength]
    cmp     dword[subLength], 0
    jng     ReturnRes
    jmp     LoopAscii
ReturnRes:
    pop     rdx
    pop     rcx
    ret

