TP2: Procesos de usuario
========================

env_alloc
---------

Analizamos la siguiente porción de código:

<code>
    generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);

    if (generation <= 0) // Don't create a negative env_id.

        generation = 1 << ENVGENSHIFT;

    e->env_id = generation | (e - envs);

</code>

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

<code>
struct Trapframe {
   struct PushRegs tf_regs;
    uint16_t tf_es;
    uint16_t tf_padding1;
    uint16_t tf_ds;
    uint16_t tf_padding2;
    uint32_t tf_trapno;
    /* below here defined by x86 hardware */
    uint32_t tf_err;
    uintptr_t tf_eip;
    uint16_t tf_cs;
    uint16_t tf_padding3;
    uint32_t tf_eflags;
    /* below here only when crossing rings, such as from user to kernel */
    uintptr_t tf_esp;
    uint16_t tf_ss;
    uint16_t tf_padding4;
} \__attribute__((packed));
</code>

     CPU->TSS--->    +--------------------+
                     |        %ss         | \ Solo presente ante
                     |        %esp        | / cambios de ring (de privilegios).
                     |       EFLAGS       |
                     |        %cs         |
                     |        %eip        |
                     |       tf_err       |
                     |       tf_trapno    |
                     |        %ds         |
                     |        %es         |
                     |        %eax        |
                     |         ...        |
                     |        %edi        |
                     +--------------------+     tf <------- %esp

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

* consola1: make run-hello-nox-gdb

* consola2: make gdb

* (gdb) b env.c:521
Punto de interrupción 1 at 0xf0102fe1: file kern/env.c, line 521.

* (gdb) c
Continuando.
Se asume que la arquitectura objetivo es i386
=> 0xf0102fe1 <env_pop_tf>:	push   %ebp

Breakpoint 1, env_pop_tf (tf=0xf01c1000) at kern/env.c:522
522	{

* (qemu) info registers

EAX=003bc000 EBX=00010094 ECX=f03bc000 EDX=00000240

ESI=00010094 EDI=00000000 EBP=f0119fd8 ESP=f0119fbc

EIP=f0102fe1 EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0

ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]

CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]

SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]

DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]

FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

LDT=0000 00000000 00000000 00008200 DPL=0 LDT

TR =0028 f017ea20 00000067 00408900 DPL=0 TSS32-avl

GDT=     f011b320 0000002f

IDT=     f017e200 000007ff

CR0=80050033 CR2=00000000 CR3=003bc000 CR4=00000010

DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000

DR6=ffff0ff0 DR7=00000400

EFER=0000000000000000

FCW=037f FSW=0000 [ST=0] FTW=00 MXCSR=00001f80

FPR0=0000000000000000 0000 FPR1=0000000000000000 0000

FPR2=0000000000000000 0000 FPR3=0000000000000000 0000

FPR4=0000000000000000 0000 FPR5=0000000000000000 0000

FPR6=0000000000000000 0000 FPR7=0000000000000000 0000

XMM00=00000000000000000000000000000000 

XMM01=00000000000000000000000000000000

XMM02=00000000000000000000000000000000

XMM03=00000000000000000000000000000000

XMM04=00000000000000000000000000000000
 
XMM05=00000000000000000000000000000000

XMM06=00000000000000000000000000000000

XMM07=00000000000000000000000000000000


* (gdb) p tf
$1 = (struct Trapframe *) 0xf01c1000

* (gdb) print sizeof(struct Trapframe) / sizeof(int)
$2 = 17

* (gdb) x/17x tf
0xf01c1000: 0x00000000  0x00000000  0x00000000  0x00000000

0xf01c1010: 0x00000000  0x00000000  0x00000000  0x00000000

0xf01c1020: 0x00000023  0x00000023  0x00000000  0x00000000

0xf01c1030: 0x00800020  0x0000001b  0x00000000  0xeebfe000

0xf01c1040: 0x00000023

* (gdb) disas

Dump of assembler code for function env_pop_tf:

=> 0xf0102fe1 <+0>:	push   %ebp

   0xf0102fe2 <+1>:	mov    %esp,%ebp

   0xf0102fe4 <+3>:	sub    $0xc,%esp

   0xf0102fe7 <+6>:	mov    0x8(%ebp),%esp

   0xf0102fea <+9>:	popa 
  
   0xf0102feb <+10>:	pop    %es

   0xf0102fec <+11>:	pop    %ds

   0xf0102fed <+12>:	add    $0x8,%esp

   0xf0102ff0 <+15>:	iret  
 
   0xf0102ff1 <+16>:	push   $0xf0105878

   0xf0102ff6 <+21>:	push   $0x214

   0xf0102ffb <+26>:	push   $0xf0105816

   0xf0103000 <+31>:	call   0xf01000a9 <_panic>

End of assembler dump.

* (gdb) si 4

=> 0xf0102fea <env_pop_tf+9>:	popa 
  
0xf0102fea	523		asm volatile("\tmovl %0,%%esp\n"

* (gdb) x/17x $sp

0xf01c1000:	0x00000000	0x00000000	0x00000000	0x00000000

0xf01c1010:	0x00000000	0x00000000	0x00000000	0x00000000

0xf01c1020:	0x00000023	0x00000023	0x00000000	0x00000000

0xf01c1030:	0x00800020	0x0000001b	0x00000000	0xeebfe000

0xf01c1040:	0x00000023

* (ver tabla del ejercicio env_pop_tf)

  * Los primero 8 valores (0x00000000) son los registros de propósito general: %edi, %esi, %ebp, %oesp, %ebx, %edx, %ecx y %eax.
  * El que le sigue, 0x00000023, corresponde a  %es. El siguiente, tambien 0x00000023, corresponde a %ds. Estos 2 valores se configuraron inicialmente en env_alloc: e->env_tf.tf_es = GD_UD | 3 (env.c:255 y env.c:256). Si miramos la dirección en binario (0000000000100011) poder ver que el RPL asignado (3) son los ultimos 2 bits. El siguiente es la tabla GDT y el resto el índice.
  * 0x00000000 es trapno.
  * 0x00000000 es error code.
  * 0x00800020 es %eip. Su valor fue seteado en load_icode, env.c:416, instruccion: e->env_tf.tf_eip = elf->e_entry;
  * 0x0000001b es  %cs. En binario: 0000000000011011. Nuevamente, los ultimos 2 bits son los del RPL asignado (3). El siguiente indica la tabla GDT y los restantes el índice. Fue asignado en por medio de la sentancia e->env_tf.tf_cs = GD_UT | 3; ubicada en la función env_alloc (env.c:259).
  * 0x00000000 es EFLAGS.
  * 0xeebfe000 es %esp. Apunta al start user stack. De vuelta se configuró en env_alloc: e->env_tf.tf_esp = USTACKTOP; (env.c:258).
  * 0x00000023 es %ss. Se configuro en env_alloc: e->env_tf.tf_ss = GD_UD | 3; (env.c:258).

* (gdb) si 4

=> 0xf0102ff0 <env_pop_tf+15>:	iret   
0xf0102ff0	523		asm volatile("\tmovl %0,%%esp\n"

* (qemu) info registers

EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000

ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c1030

EIP=f0102ff0 EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0

ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]

SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]

DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

LDT=0000 00000000 00000000 00008200 DPL=0 LDT

TR =0028 f017ea20 00000067 00408900 DPL=0 TSS32-avl

GDT=     f011b320 0000002f

IDT=     f017e200 000007ff

CR0=80050033 CR2=00000000 CR3=003bc000 CR4=00000010

DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000

DR6=ffff0ff0 DR7=00000400

EFER=0000000000000000

FCW=037f FSW=0000 [ST=0] FTW=00 MXCSR=00001f80

FPR0=0000000000000000 0000 FPR1=0000000000000000 0000

FPR2=0000000000000000 0000 FPR3=0000000000000000 0000

FPR4=0000000000000000 0000 FPR5=0000000000000000 0000

FPR6=0000000000000000 0000 FPR7=0000000000000000 0000

XMM00=00000000000000000000000000000000
 
XMM01=00000000000000000000000000000000

XMM02=00000000000000000000000000000000
XMM03=00000000000000000000000000000000

XMM04=00000000000000000000000000000000

XMM05=00000000000000000000000000000000

XMM06=00000000000000000000000000000000

XMM07=00000000000000000000000000000000

ESP=f01b7030 es el stack, y vemos que en esa dirección anteriormente hemos impreso 0x00800020 correspondiente a %eip. Por lo tanto afirmamos que el stack queda apuntando al valor de %eip que debemos recuperar.
ES pasa a tener c igual a 3 (privilegio de usuario) porque lo recuperamos (notar que CS no se llegó a recuperar y mantiene un DPL = 0).

* (gdb) si

=> 0x800020:	cmp    $0xeebfe000,%esp

0x00800020 in ?? ()

* (gdb) p $pc

$3 = (void (*)()) 0x800020

* (gdb) symbol-file obj/user/hello

¿Cargar una tabla de símbolos nueva desde «obj/user/hello»? (y or n) y
Leyendo símbolos desde obj/user/hello...hecho.
Error in re-setting breakpoint 1: No hay un archivo fuente con el nombre env.c.

* (gdb) p $pc
 
$4 = (void (*)()) 0x800020 <_start>

* (qemu) info registers

EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000

ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000

EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0

ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]

SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]

LDT=0000 00000000 00000000 00008200 DPL=0 LDT

TR =0028 f017ea20 00000067 00408900 DPL=0 TSS32-avl

GDT=     f011b320 0000002f

IDT=     f017e200 000007ff

CR0=80050033 CR2=00000000 CR3=003bc000 CR4=00000010

DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000 

DR6=ffff0ff0 DR7=00000400

EFER=0000000000000000

FCW=037f FSW=0000 [ST=0] FTW=00 MXCSR=00001f80

FPR0=0000000000000000 0000 FPR1=0000000000000000 0000

FPR2=0000000000000000 0000 FPR3=0000000000000000 0000

FPR4=0000000000000000 0000 FPR5=0000000000000000 0000

FPR6=0000000000000000 0000 FPR7=0000000000000000 0000

XMM00=00000000000000000000000000000000

XMM01=00000000000000000000000000000000

XMM02=00000000000000000000000000000000
XMM03=00000000000000000000000000000000

XMM04=00000000000000000000000000000000

XMM05=00000000000000000000000000000000

XMM06=00000000000000000000000000000000

XMM07=00000000000000000000000000000000

Se recupera la %eip: EIP=00800020. No es casualidad ya que habia sido asignado en load_icode.

Ahora cambiamos de environment y efectivamente queda expresado en el valor de CPL que ahora vale 3.

Finalmente se recuperó el valor de %cs y a diferencia del punto anterios, ahora su DPL vale 3.

* (gdb) tbreak syscall

Punto de interrupción temporal 2 at 0x8009ea: file lib/syscall.c, line 23.

* (gdb) c

Continuando.

=> 0x8009ea <syscall+17>:	mov    0x8(%ebp),%ecx

Temporary breakpoint 2, syscall (num=0, check=-289415544, a1=4005551752, 
    a2=13, a3=0, a4=0, a5=0) at lib/syscall.c:23
23		asm volatile("int %1\n"

* (gdb) disas

Dump of assembler code for function syscall:
 
    0x008009f3 <+0>:	push %ebp
 
    0x008009f4 <+1>:	mov %esp,%ebp 

    0x008009f6 <+3>:	push %edi 

    0x008009f7 <+4>:	push %esi 

    0x008009f8 <+5>:	push %ebx 

    0x008009f9 <+6>: sub $0x1c,%esp 

    0x008009fc <+9>:	mov %eax,-0x20(%ebp) 

    0x008009ff <+12>:	mov %edx,-0x1c(%ebp) 

    0x00800a02 <+15>: mov %ecx,%edx 

=>  0x00800a04 <+17>:	mov 0x8(%ebp),%ecx 

    0x00800a07 <+20>:	mov 0xc(%ebp),%ebx 

    0x00800a0a <+23>:	mov 0x10(%ebp),%edi 

    0x00800a0d <+26>:	mov 0x14(%ebp),%esi 

    0x00800a10 <+29>:	int $0x30 

    0x00800a12 <+31>:	cmpl $0x0,-0x1c(%ebp) 

    0x00800a16 <+35>:	je 0x800a35 <syscall+66> 

    0x00800a18 <+37>:	test %eax,%eax 

    0x00800a1a <+39>:	jle 0x800a35 <syscall+66> 

    0x00800a1c <+41>:	mov -0x20(%ebp),%edx 

    0x00800a1f <+44>:	sub $0xc,%esp

    0x00800a22 <+47>:	push %eax 

    0x00800a23 <+48>:	push %edx 

---Type to continue, or q to quit--- 

    0x00800a24 <+49>:	push $0x800fd4 

    0x00800a29 <+54>:	push $0x23
 
    0x00800a2b <+56>:	push $0x800ff1 

    0x00800a30 <+61>:	call 

    0x800acd <_panic> 

    0x00800a35 <+66>:	lea -0xc(%ebp),%esp 

    0x00800a38 <+69>:	pop %ebx
 
    0x00800a39 <+70>:	pop %esi 

    0x00800a3a <+71>:	pop %edi 

    0x00800a3b <+72>:	pop %ebp 

    0x00800a3c <+73>:	ret

End of assembler dump.


* (gdb) si 4 

=>  0x800a10 <syscall+29>:	int $0x30
 
    0x00800a10	23	asm volatile("int %1\n" 

* (gdb) si 

aviso: A handler for the OS ABI "GNU/Linux" is not built into this configuration of GDB. Attempting to continue with the default i8086 settings.

Ocurre una excepción y finalizó.


kern_idt
---------

**PREGUNTA:**  
¿Cómo decidir si usar TRAPHANDLER o TRAPHANDLER_NOEC? ¿Qué pasaría si se usara solamente la primera?
**RESPUESTA:**
La diferencia es que la macro TRAPHANDLER_NOEC, se utiliza para los casos en los cuales la cpu no pushea un código de error. Si se usará siempre la primera, el trap frame quedaría distinto para aquellas traps en las cuales el cpu no pushea un código de error.

**PREGUNTA:**  
Qué cambia, en la invocación de handlers, el segundo parámetro (istrap) de la macro SETGATE? ¿Por qué se elegiría un comportamiento u otro durante un syscall?
**RESPUESTA:**
La diferencia esta en que en una interrupción se debe volver a la misma linea de código en la que estaba el cpu antes de que ocurriera la excepción. Para una trap, tiene que pasar a la siguiente.


**PREGUNTA:**  
Leer user/softint.c y ejecutarlo con make run-softint-nox. ¿Qué excepción se genera? Si hay diferencias con la que invoca el programa… ¿por qué mecanismo ocurre eso, y por qué razones?
**RESPUESTA:**


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

En el código figuraba sys_cputs(0xf010000c, 100); pero imprimia un caracter no deseado lógicamente y no concordaba con el enunciado. Por eso lo  cambiamos.

Se observa que en este código se pasa la dirección virtual directamente. En cambio en el código nuevo le terminamos pasando la dirección de memoria de "first" que almacena el primer byte del kernel pointer.

**PREGUNTA:**
¿En qué cambia el comportamiento durante la ejecución? ¿Por qué? ¿Cuál es el mecanismo?

**RESPUESTA:**
Los resultados de las ejecuciones son diferentes, en efecto, el código original imprime por pantalla la siguiente salida:

<code>
\+ cc[USER] user/evilhello.c

\+ ld obj/user/evilhello

\+ ld obj/kern/kernel

\+ mk obj/kern/kernel.img

qemu-system-i386 -nographic -drive 
file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp:127.0.0.1:26000 -D qemu.log  -d guest_errors

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

fIncoming TRAP frame at 0xefffffbc

[00001000] exiting gracefully

[00001000] free env 00001000

Destroyed the only environment - nothing more to do!

Welcome to the JOS kernel monitor!

Type 'help' for a list of commands.
</code>

Y el código nuevo imprime por pantalla la siguiente la salida:

<code>
\+ cc kern/init.c

\+ cc[USER] user/evilhello.c

\+ ld obj/user/evilhello

\+ ld obj/kern/kernel

\+ mk obj/kern/kernel.img

qemu-system-i386 -nographic -drive 
file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio 
-gdb tcp:127.0.0.1:26000 -D qemu.log  -d guest_errors

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

</code>

El comportamiento claramente cambia. En el código nuevo, a diferencia del orignal, no se llega a imprimir el primer byte del entry point dado que ocurre un page Fault antes.

Esto nos dió la pista de lo que está ocurriendo. Se trata de un tema de permisos y checkeos faltantes en la función sys_cputs. En efecto, no controla que el usuario que usa dicha sys call tenga los permisos necesarios para leer las direcciones de memoria que desea imprimir. Esto explica por qué en el primer caso (código original) se pudo imprimir, y es por que le enviamos directamente la dirección virtual a la sys call.

En el código nuevo en vez de llamar directamente a sys_cputs trata primero de asignar el byte a imprimir a la variable first. Pero eso no es posible ya que se controlan los permisos del usuario sobre las operaciones. Y claramente no tenemos permisos para leer la dirección de memoria del entry point, por eso provocamos un page fault.