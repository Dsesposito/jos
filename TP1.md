TP1: Memoria virtual en JOS
===========================

page2pa
-------

La función page2pa devuelve  la dirección física de comienzo de una página en dado un PageInfoStruct. El shift que se ejecuta (<<PGSHIFT) es debido a que el tamaño de cada página es de 4KB.
La deducción de la dirección se realiza calculando la posición que ocupa la página de interés en la lista pages y dicha diferencia se multiplica por 4KB.

...


boot_alloc_pos
--------------

 * Cálculo manual de la primera dirección de memoria que devolverá boot_alloc() tras el arranque. Se puede calcular a partir del binario compilado (obj/kern/kernel), usando los comandos readelf y/o nm y operaciones matemáticas:
 
Al leer el section header con readelf del árchivo kernel obtenemos :

```

Section Headers:
  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            00000000 000000 000000 00      0   0  0
  [ 1] .text             PROGBITS        f0100000 001000 001ee1 00  AX  0   0 16
  [ 2] .rodata           PROGBITS        f0101f00 002f00 00095c 00   A  0   0 32
  [ 3] .stab             PROGBITS        f010285c 00385c 004891 0c   A  4   0  4
  [ 4] .stabstr          STRTAB          f01070ed 0080ed 001e50 00   A  0   0  1
  [ 5] .data             PROGBITS        f0109000 00a000 00a300 00  WA  0   0 4096
  [ 6] .bss              NOBITS          f0113300 014300 000650 00  WA  0   0 32
  [ 7] .comment          PROGBITS        00000000 014300 000035 01  MS  0   0  1
  [ 8] .shstrtab         STRTAB          00000000 0150a8 00004c 00      0   0  1
  [ 9] .symtab           SYMTAB          00000000 014338 0008c0 10     10  70  4
  [10] .strtab           STRTAB          00000000 014bf8 0004b0 00      0   0  1
```

Se puede observar que el segmento bss empieza en la dirección 0xf0113300 y tiene un tamaño 000650 . Esto nos dice que el segmento termina en 0xf0113950 . Con lo cual , la primer dirección de memoria libre será el multiplo de 4096 mas cercano a 0xf0113950 . El cual resulta ser : 0xf0114000 

La primera vez que se llama boot_alloc , se la llama para reservar la memoria para el page directory . Entonces la página que contendrá el page directory será la correspondiente a la dirección 0xf0114000 . Como boot_alloc devuelve la siguiente página libre, la dirección que devolverá será la correspondiente al siguiente multiplo de 4096 lo cual corresponde a la dirección 0xf0115000 . 


 * Mostrar una sesión de GDB en la que, poniendo un breakpoint en la función boot_alloc(), se muestre el valor de end y nextfree al comienzo y fin de esa primera llamada a boot_alloc().

```
make gdb
gdb -q -s obj/kern/kernel -ex 'target remote 127.0.0.1:26000' -n -x .gdbinit
Reading symbols from obj/kern/kernel...done.
Remote debugging using 127.0.0.1:26000
0x0000fff0 in ?? ()
(gdb) b boot_alloc 
Breakpoint 1 at 0xf0100a32: file kern/pmap.c, line 98.
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0xf0100a32 <boot_alloc>:	cmpl   $0x0,0xf0113538

Breakpoint 1, boot_alloc (n=4096) at kern/pmap.c:98
98		if (!nextfree) {
(gdb) n
=> 0xf0100a3b <boot_alloc+9>:	mov    $0xf011494f,%edx
100			nextfree = ROUNDUP((char *) end, PGSIZE);
(gdb) n
=> 0xf0100a4c <boot_alloc+26>:	cmp    %eax,0xf0113944
109		if(npages<n){
(gdb) p/x end
$1 = 0x21
(gdb) p/x nextfree 
$2 = 0xf0114000
(gdb) n
=> 0xf0100a6b <boot_alloc+57>:	mov    0xf0113538,%edx
117		return nextfree;
(gdb) n
=> 0xf0100a71 <boot_alloc+63>:	test   %eax,%eax
112		if (n != 0) {
(gdb) n
=> 0xf0100a75 <boot_alloc+67>:	lea    0xfff(%edx,%eax,1),%eax
114			nextfree = ROUNDUP((char *) (nextfree+n), PGSIZE);
(gdb) n
=> 0xf0100a86 <boot_alloc+84>:	mov    %edx,%eax
118	}
(gdb) p/x nextfree 
$3 = 0xf0115000
(gdb) n
=> 0xf0100e70 <mem_init+27>:	sub    $0x4,%esp
mem_init () at kern/pmap.c:144
144		memset(kern_pgdir, 0, PGSIZE);

```

...


page_alloc
----------
¿en qué se diferencia page2pa() de page2kva()?

page2pa devuelve la dirección física a la que mapea la página. 
page2kva devuelve la dirección virtual del kernel a la que mapea la página. 

...


