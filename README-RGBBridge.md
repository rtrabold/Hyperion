# Hyperion EventFX para RGBBridge

Motor de efectos local controlado por [RGBBridge](https://github.com/rtrabold/RGBBridge).
La rama `master` del fork replica `upstream/master`; la rama `rgbbridge` agrega
un único commit con la integración EventFX. Se conservan el historial, las
licencias y las atribuciones del proyecto original.

RGBBridge envía un JSON `rgbeventfx` por evento. Hyperion genera la animación a
40 FPS y la mezcla sobre su fuente actual. Soporta Projectile, Shockwave, Flash,
Pulse, Progress y Sparks; el parámetro opcional `origin` elige el LED físico de
inicio y convierte el rango seleccionado en un circuito cerrado.

Consulta `integrations/rgbbridge/README.md` para el protocolo y sus parámetros.
Las pruebas de origen usan un reloj determinista y no encienden LED físicos.

Las instrucciones de compilación y dependencias son las del proyecto Hyperion.
La configuración de Windows utilizada mantiene habilitados DDA, MF, Qt y Audio.

`origin` corresponde a este fork público. `upstream` apunta a
https://github.com/hyperion-project/hyperion.ng y nunca recibe nuestros cambios.
La PR interna `rgbbridge` → `master` permite revisar la integración sin enviarla
al proyecto oficial.
