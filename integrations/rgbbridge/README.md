# Hyperion RGBBridge EventFX

Esta integración hace que RGBBridge envíe un solo evento JSON a Hyperion. Hyperion genera la animación a 40 FPS y la mezcla sobre la fuente que esté visible en ese instante. RGBBridge no transmite cuadros de LED.

## Base y mantenimiento

La rama `rgbbridge` contiene un único commit sobre `upstream/master`. La rama
`master` del fork se mantiene idéntica al proyecto oficial y sirve como base de
la PR interna. Para actualizar, se vuelve a aplicar y validar ese commit sobre
el nuevo `upstream/master` antes de reemplazar la rama desplegada.

## Comando

El comando se envía una sola vez a `POST /json-rpc`:

```json
{
  "command": "rgbeventfx",
  "instance": [0],
  "effect": "projectile",
  "duration": 350,
  "brightness": 255,
  "start": 0,
  "count": 68,
  "origin": 45,
  "overlay": true,
  "backgroundColor": [0, 0, 0],
  "options": {
    "color": [255, 0, 0],
    "pointSize": 3,
    "tailSize": 5,
    "reverse": false
  }
}
```

También acepta `effects`, un arreglo con hasta ocho efectos para iniciar configuraciones distintas con el mismo comando. Los efectos disponibles son `projectile`, `shockwave`, `flash`, `pulse`, `progress` y `sparks`.

`start` y `count` seleccionan el rango de LED de Hyperion. Un `count` igual a `0` usa todos los LED desde `start`. `overlay: true` mezcla el evento sobre la fuente actual. Con `overlay: false`, sólo el rango seleccionado recibe `backgroundColor` durante el evento.

Hyperion mantiene hasta ocho eventos simultáneos para permitir disparos consecutivos y ráfagas. Los eventos no crean una prioridad ni reemplazan la fuente activa. Al terminar, la salida vuelve al cuadro actual de esa fuente.

## Origen por regla

`origin` es opcional y contiene el número **físico** del LED dentro del rango `start`/`count`. Con origen, el rango forma un circuito cerrado: después del último LED continúa el primero. Projectile y Progress comienzan en ese LED; `reverse` cambia el sentido manteniendo el mismo origen. Shockwave nace en ese punto y viaja en ambos sentidos hasta el lado opuesto, incluyendo la salida de la cola dentro de la duración. Sparks rota su distribución; Flash y Pulse conservan su aspecto uniforme.

RGBBridge guarda este valor por regla como `HyperionOriginLed` y lo incluye en el único JSON del evento. El campo vacío omite `origin` y conserva el comportamiento lineal anterior. Los valores fuera del rango real son rechazados antes de activar el evento. Esto no cambia el mapeo del ambilight.

La compilación usada en este equipo mantiene habilitados `ENABLE_DDA`, `ENABLE_MF`, `ENABLE_QT` y `ENABLE_AUDIO`.
