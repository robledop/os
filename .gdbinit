set breakpoint pending on

#add-symbol-file ./build/kernelfull.o 0x200000
#add-symbol-file ./rootfs/bin/echo 0x400000
#add-symbol-file ./rootfs/bin/ls 0x400000
#add-symbol-file ./rootfs/bin/cat 0x400000
#add-symbol-file ./rootfs/bin/forktest 0x400000
#add-symbol-file ./rootfs/bin/edit 0x400000
add-symbol-file ./rootfs/bin/sh 0x400000
#add-symbol-file ./rootfs/bin/fib 0x400000

break idt_exception_handler
break panic 

#break trap_return 
