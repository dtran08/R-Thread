.globl thread_switch

thread_switch:

    movl %esp, %ecx

    movl 4(%ecx), %eax
    movl 8(%ecx), %edx

    pushal

    movl %esp, (%eax)
    movl (%edx), %esp
    
    movl %edx, current_thread

    popal

    movl $0, next_thread

    ret
    
