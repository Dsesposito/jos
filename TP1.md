TP1: Memoria virtual en JOS
===========================

page2pa
-------

La función page2pa devueve  la dirección fisica de comienzo de una página en dado un PageInfoStruct. El shift que se ejecuta (<<PGSHIFT) es debido a que el tamaño de cada página es de 4KB.
La deducción de la dirección se realiza calculado la posición en que ocupa la página de interés en la lista pages y dicha diferencia se multiplica por 4KB.

...


boot_alloc_pos
--------------

...


page_alloc
----------

...


