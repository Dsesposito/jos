TP3: Multitarea con desalojo
=============================

static_assert
-------------

**PREGUNTA:** 

¿Cómo y por qué funciona la macro static_assert que define JOS?

**RESPUESTA:**

Esto se puede ver facilmente si se define un static assert que no tiene sentido. Por ejemplo si definimos el siguiente bloque

```c
    #define TEST_STATIC_ASSERT 0x100
    static_assert(TEST_STATIC_ASSERT % PGSIZE == 0);
```

observamos el siguiente error al querer compilar:

```c
    error: duplicate case value
```

Esto se debe a que si observamos la definición de la macro vemos que:

```c
    #define static_assert(x)	switch (x) case 0: case (x):
```

Entonces si el valor de la operacion resto devuelve 0, el case 0 queda duplicado y esto falla en tiempo de compilación.


Tarea: env_return
-----------------
al terminar un proceso su función umain() ¿dónde retoma la ejecución el kernel? Describir la secuencia de llamadas desde que termina umain() hasta que el kernel dispone del proceso.

El scheduller elije otro environment para ejecutar. Si no hay mas environments el scheduller invoca el monitor del kernel.


Tarea: sys_yield
----------------

**PREGUNTA:**

Leer y estudiar el código del programa user/yield.c. Cambiar la función i386_init() para lanzar tres instancias de dicho programa, y mostrar y explicar la salida de make qemu-nox

**RESPUESTA:**

Salida de qemu-nox
```
qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp:127.0.0.1:26000 -D qemu.log -smp 1  -d guest_errors
6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
Starting page init
check_page_free_list() succeeded!
Free page list checked. Continuing to page alloc check
check_page_alloc() succeeded!
Page alloc checked. Continuing to page check
check_page() succeeded!
Page checked. Continuing to set up virtual memory
check_kern_pgdir() succeeded!
check_page_free_list() succeeded!
check_page_installed_pgdir() succeeded!
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
```

Cada proceso yield posee un for y la primera instrucción del for es un sys_yield. 
Lo que hace ese sys_yield es desalojarse a si mismo de la CPU para que luego el sheduller elija con que proceso seguir. 
Luego de las 3 sentencias de "Hello, I am environment" se ejecuta el primer yield del primer proceso. Entonces le concede la ejecución al siguiente proceso que tambien realiza una llamada a sys_yield. Le concede la ejecución el otro proceso y tambien llama a sys_yield. Luego, retoma el primer proceso que realizo yield y continua con la siguiente linea del for que justamente es imprimir "Back in environment %08x, iteration %d.\n". Continua con la nueva iteracion y vuelve a invocar sys_yield. 
El proceso continua de forma similar hasta que finalizan el for y se liberan los environments.
Finalmente como no hay más environmentes para ejecutar, el scheduller invoca el monitor del kernel: "Welcome to the JOS kernel monitor!". 


multicore_init
-------------

**PREGUNTA:** 

¿Qué código copia, y a dónde, la siguiente línea de la función boot_aps()?

```c
    memmove(code, mpentry_start, mpentry_end - mpentry_start);
```

**RESPUESTA:**

La linea mencionada copia el codigo de entrada del application processors (AP) ubicado en kern/mpentry.S (.globl mpentry_start) a la dirección de memoria virtual correspondiente a la memoria dirección de memoria física MPENTRY_PADDR

**PREGUNTA:**  FALTA CONTESTAR LA SEGUNDA PREGUNTA

¿Para qué se usa la variable global mpentry_kstack? ¿Qué ocurriría si el espacio para este stack se reservara en el archivo kern/mpentry.S, de manera similar a bootstack en el archivo kern/entry.S?

**RESPUESTA:**

Al momento de comenzar a correr un AP se ejecutan las siguientes lineas de código

```c
    // Tell mpentry.S what stack to use 
    mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE;
    // Start the CPU at mpentry_start
    lapic_startap(c->cpu_id, PADDR(code));
```

Como se puede ver , antes de mandar a correr el AP lo que se hace es apuntar mpentry_kstack a la primera dirección de memoria del kernel stack del cpu que se esta mandando a correr. En mpentry.S se puede ver como mueven dicha direccion de memoria al registro %esp

```c
	movl    mpentry_kstack, %esp 
```

**PREGUNTA:** 

Cuando QEMU corre con múltiples CPUs, éstas se muestran en GDB como hilos de ejecución separados. Mostrar una sesión de GDB en la que se muestre cómo va cambiando el valor de la variable global mpentry_kstack

**RESPUESTA:**

A continuación se muestra la salida de gdb pedida

```
    make gdb 
    gdb -q -s obj/kern/kernel -ex 'target remote 127.0.0.1:26000' -n -x .gdbinit
    Reading symbols from obj/kern/kernel...done.
    Remote debugging using 127.0.0.1:26000
    0x0000fff0 in ?? ()
    (gdb) b init.c:109
    Breakpoint 1 at 0xf0100171: file kern/init.c, line 109.
    (gdb) c
    Continuing.
    The target architecture is assumed to be i386
    => 0xf0100171 <boot_aps+108>:	mov    %esi,%ecx
    
    Thread 1 hit Breakpoint 1, boot_aps () at kern/init.c:109
    109			lapic_startap(c->cpu_id, PADDR(code));
    (gdb) p mpentry_kstack
    $1 = (void *) 0xf024d000 <percpu_kstacks+65536>
    (gdb) p/a mpentry_kstack 
    $2 = 0xf024d000 <percpu_kstacks+65536>
    (gdb) c
    Continuing.
    => 0xf0100171 <boot_aps+108>:	mov    %esi,%ecx
    
    Thread 1 hit Breakpoint 1, boot_aps () at kern/init.c:109
    109			lapic_startap(c->cpu_id, PADDR(code));
    (gdb) p/a mpentry_kstack 
    $3 = 0xf0255000 <percpu_kstacks+98304>
    (gdb) c
    Continuing.
    => 0xf0100171 <boot_aps+108>:	mov    %esi,%ecx
    
    Thread 1 hit Breakpoint 1, boot_aps () at kern/init.c:109
    109			lapic_startap(c->cpu_id, PADDR(code));
    (gdb) p/a mpentry_kstack 
    $4 = 0xf025d000 <percpu_kstacks+131072>
    (gdb) c
    Continuing.
```

Las direcciones apuntadas por mpentry_kstack fueron:

 * 0xf024d000
 * 0xf0255000
 * 0xf025d000
 
 Notar que la diferencia entre las dos primeras direcciones de memoria , en decimal, da 32768 bytes y si además nos fijamos en memlayout.h vemos que KSTKSIZE define que el kernel stack size es de 8*PGSIZE es decir 8*4*1024 bytes = 32768 bytes con lo cual las direcciones leídas son consistentes.

**PREGUNTA:**  FALTA CONTESTAR 

En el archivo kern/mpentry.S se puede leer:

```c                                                          
    # We cannot use kern_pgdir yet because we are still
    # running at a low EIP.
    movl $(RELOC(entry_pgdir)), %eax
```

 * ¿Qué valor tiene el registro %eip cuando se ejecuta esa línea?

Responder con redondeo a 12 bits, justificando desde qué región de memoria se está ejecutando este código.

 * ¿Se detiene en algún momento la ejecución si se pone un breakpoint en mpentry_start? ¿Por qué?                                                           
**RESPUESTA:**  

La funcion boot_aps llama a la función lapic_startap pasandole por parámetro la dirección de memoria fisíca del código que debe ejecutar el AP . Luego la función lapic_startap le envia al procesdador la startup IPI para que el procesador arranque y ejecute el código de inicio.  

**PREGUNTA:** FALTA CONTESTAR 

Con GDB, mostrar el valor exacto de %eip y mpentry_kstack cuando se ejecuta la instrucción anterior en el último AP. 

**RESPUESTA:**
