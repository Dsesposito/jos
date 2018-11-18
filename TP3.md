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

...

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
