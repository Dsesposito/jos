TP2: Procesos de usuario
========================

env_alloc
---------

Analizamos la siguiente porción de código:

    generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
    if (generation <= 0) // Don't create a negative env_id.
        generation = 1 << ENVGENSHIFT;
    e->env_id = generation | (e - envs);

ENVGENSHIFT = 12. Es un valor fijo.
NENV también es un valor fijo: 1024

para los primeros 5 ocurre lo siguiente:

"e" es un struct que hace referencia a un environment sin utilizar por lo tanto e->env_id vale 0 (así se define en env_init()).
Generation = (0 + (4096)) & ~(1023) = 4096 (1000 en hexadecimal).
Para el primer environment e y envs coinciden y el resultado (e-envs) es 0.

Finalmente e->env_id = 4096 | 0 = 4096. (0x00001000 en hexa)
Para el segundo environment, generation sigue valiendo 4096 porque es fijo (vendría a ser una especie de offset).
Pero en este caso (e-envs) vale 1 porque e apunta a la segunda posición del array de environments (el primero que encuentra libre).
Finalmente e->env_id = 4096 | 1 = 4097. (0x00001001 en hexa)
Con los otros 3 ocurre lo mismo.

**PREGUNTA:**
¿Qué identificadores se asignan a los primeros 5 procesos creados? (Usar base hexadecimal.)

**RESPUESTA:**
Se asignan los siguientes 5 identificadores:
0x00001000
0x00001001
0x00001002
0x00001003
0x00001004

La segunda pregunta propone un escenario donde ya se utilizaron todos los environments disponibles, con lo cual se destruyen procesos y se reutilizan los environments asociados.
En estos casos ocurre que:
"e" es un struct que hace referencia a un environment ya utilizado por lo tanto e->env_id NO vale 0.
el environment envs[630] tendrá un e→env_id = 4096 + 630 = 4726 (0x00001276 en hexa)
En este caso, generation ya no vale 4096:
Generation = (4726 + (4096)) & ~(1023) = 8192 (2000 en hexadecimal).
e → env_id = 8192 | 630 = 8822. (0x00002276 en hexa)

Con los siguientes 5 ocurre lo mismo:
Generation = (8822 + (4096)) & ~(1023) = 12288 (3000 en hexadecimal).
e → env_id = 12288 | 630 = 12918. (0x00003276 en hexa)

Generation = (12918 + (4096)) & ~(1023) = 16384 (4000 en hexadecimal).
e → env_id = 16384 | 630 = 17014. (0x00004276 en hexa)

Generation = (17014 + (4096)) & ~(1023) = 20480 (5000 en hexadecimal).
e → env_id = 20480 | 630 = 21110. (0x00005276 en hexa)

Generation = (21110 + (4096)) & ~(1023) = 24576 (6000 en hexadecimal).
e → env_id = 24576 | 630 = 25206. (0x00006276 en hexa)

Entonces:

**PREGUNTA:**
Supongamos que al arrancar el kernel se lanzan NENV proceso a ejecución. A continuación se destruye el proceso asociado a envs[630] y se lanza un proceso que cada segundo muere y se vuelve a lanzar. ¿Qué identificadores tendrá este proceso en sus sus primeras cinco ejecuciones?

**RESPUESTA:**
0x00002276
0x00003276
0x00004276
0x00005276
0x00006276

env_init_percpu
---------------

**PREGUNTA:**
¿Cuántos bytes escribe la función lgdt, y dónde?
¿Qué representan esos bytes?

**RESPUESTA:**
Escribe 6 bytes en el registro “global descriptor table” (GDT). Dichos bytes representan  la ubicación que contiene la dirección base (una dirección linear) y el límite (tamaño de la tabla en bytes) del GDT.
El registro es de 48 bits: en los 32 más significativos se encuentra la ubicación. En los restantes 16 (menos significativos) contienen el límite de la tabla.

env_pop_tf
----------

Para responder estas preguntas hay que saber que el environment guarda el estado en una estructura llamada trapframe. La estructura la podemos observar en el archivo trap.h de la carpeta inc:

> struct Trapframe {
> sentence. struct PushRegs tf_regs;
> sentence. uint16_t tf_es;
> sentence. uint16_t tf_padding1;
> sentence. uint16_t tf_ds;
> sentence. uint16_t tf_padding2;
> sentence. uint32_t tf_trapno;
> sentence. /* below here defined by x86 hardware */
> sentence. uint32_t tf_err;
> sentence. uintptr_t tf_eip;
> sentence. uint16_t tf_cs;
> sentence. uint16_t tf_padding3;
> sentence. uint32_t tf_eflags;
> sentence. /* below here only when crossing rings, such as from user to kernel */
> sentence. uintptr_t tf_esp;
> sentence. uint16_t tf_ss;
> sentence. uint16_t tf_padding4;
> } \__attribute__((packed));

     CPU->TSS--->    +--------------------+
                     |         SS         | \ Solo presente ante
                     |         ESP        | / cambios de ring (de privilegios).
                     |       EFLAGS       |
                     |         CS         |
                     |         EIP        |
                     |     error code     |
                     |       trapno       |
                     |         DS         |
                     |         ES         |
                     |         EAX        |
                     |         ...        |
                     |         EDI        |
                     +--------------------+     tf <------- ESP
**PREGUNTA:**
¿Qué hay en (%esp) tras el primer movl de la función?
**RESPUESTA:**
Hay un puntero al trapframe.

**PREGUNTA:**
¿Qué hay en (%esp) justo antes de la instrucción iret? ¿Y en 8(%esp)?
**RESPUESTA:**
El popall hace un pop desde el stack sobre los registros de propósito general. 
Luego hace pop de “es” y “ds”. Los siguientes registros son los de trapno y error code, pero los saltea directamente con la instrucción: "\taddl $0x8,%%esp\n" (esto lo menciona como un comentario).
Entonces, en %esp se encuentra EIP.
En 8(%esp) se encuentra CS.
**PREGUNTA:**  
¿Cómo puede determinar la CPU si hay un cambio de ring (nivel de privilegio)?
**RESPUESTA:**
Cuando hay un cambio de privilegio IRET hace pop del stack pointer y de SS (además de los que normalmente hace: EIP, CS y EFLAGS).  Si ESP y SS no están presentes es porque no hubo un cambio de ring.
El CPU puede saberlo mediante los valores RPL (Requested Privilege Level) y CPL (Current Privilege Level).
Luego de realizar todo esto el CPU resume la ejecución con el nuevo EIP y CS.
...

gdb_hello
---------

...

user_evilhello
--------------

**PREGUNTA:**  
¿En qué se diferencia el código de la versión en evilhello.c mostrada arriba?
**RESPUESTA:**
En este punto consideramos el código mostrado en el enunciado ya que el código fuente difiere.

Es decir:
user/evilhello.c lo consideramos así:

// evil hello world -- kernel pointer passed to kernel
// kernel should destroy user environment in response

\#include <inc/lib.h>
void
umain(int argc, char **argv)
{
    // Imprime el primer byte del entry point como caracter.
    sys_cputs(0xf010000c, 1);
}

Se observa que en este código se pasa la dirección virtual directamente. En cambio en el código nuevo le terminamos pasando la dirección de memoria de "first" que almacena el primer byte del kernel pointer.

Los resultados de las ejecuciones son diferentes, en efecto, el código original imprime por pantalla la siguiente salida:

\+ cc kern/init.c
\+ ld obj/kern/kernel
\+ mk obj/kern/kernel.img
qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp:127.0.0.1:26000 -D qemu.log  -d guest_errors
6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
Starting page init
Free page list checked. Continuing to page alloc check
check_page_alloc() succeeded!
Page alloc checked. Continuing to page check
check_page() succeeded!
Page checked. Continuing to set up virtual memory
check_kern_pgdir() succeeded!
check_page_installed_pgdir() succeeded!
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
f�r#Incoming TRAP frame at 0xefffffbc
[00001000] exiting gracefully
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.

Y el código nuevo imprime por pantalla la siguiente la salida:

\+ cc kern/init.c
\+ cc[USER] user/evilhello.c
\+ ld obj/user/evilhello
\+ ld obj/kern/kernel
\+ mk obj/kern/kernel.img
qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp:127.0.0.1:26000 -D qemu.log  -d guest_errors
6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
Starting page init
Free page list checked. Continuing to page alloc check
check_page_alloc() succeeded!
Page alloc checked. Continuing to page check
check_page() succeeded!
Page checked. Continuing to set up virtual memory
check_kern_pgdir() succeeded!
check_page_installed_pgdir() succeeded!
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
[00001000] user fault va f010000c ip 00800039
TRAP frame at 0xf01c1000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfd0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000e Page Fault
  cr2  0xf010000c
  err  0x00000005 [user, read, protection]
  eip  0x00800039
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfb0
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.

**PREGUNTA:**
¿En qué cambia el comportamiento durante la ejecución? ¿Por qué? ¿Cuál es el mecanismo?
**RESPUESTA:**