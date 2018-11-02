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