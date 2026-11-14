# PS3 G923 Driver

Proyecto homebrew para investigar la compatibilidad del Logitech G923 Xbox con PS3 (HEN).

## Estado actual

La aplicación de diagnóstico ahora inicializa la biblioteca USB de PSL1GHT, registra un Logical Device Driver para el VID/PID observado y, cuando recibe el callback de conexión, solicita la configuración USB y enumera los descriptores de endpoint.

**Esta etapa todavía no lee reportes de entrada ni envía el comando de inicialización del volante.** Tampoco implementa un dispositivo de entrada para juegos ni Force Feedback.

## Evidencia confirmada en PC

- G923 Xbox detectado inicialmente como USB VID:PID `046D:C26D`.
- Interfaz inicial observada: `FF/47/D0`, endpoints observados `0x01 OUT` y `0x81 IN`; el tipo exacto debe confirmarse con los descriptores completos.
- Con G HUB cerrado, enviar `0F 00 01 01 42` al endpoint OUT `0x01` produjo calibración y el volante pasó a estar disponible en `joy.cpl`.
- El cambio de PID, la topología posterior y el formato de los reportes aún deben confirmarse. No se asumen en el código.

## Diagnóstico en PS3

La aplicación escribe un registro en:

```
/dev_hdd0/tmp/G923Diag.log
```

El registro incluye el resultado de cargar el módulo USB, inicializar libusb, registrar el controlador VID/PID, conectar el dispositivo, establecer la configuración y enumerar endpoints (dirección, tipo, tamaño máximo, intervalo y resultado de apertura del pipe).

La capa USB usa la biblioteca `libusb` de PSL1GHT y su registro de LDD específico por VID/PID. El código de transferencia permanece deliberadamente desactivado hasta confirmar el tipo y tamaño reales de los endpoints del G923.

## Próximas etapas

1. Confirmar en consola que el LDD recibe el G923 y obtener los descriptores completos.
2. Implementar transferencias OUT/IN según el tipo real de endpoint.
3. Reproducir la inicialización observada en PC y validar la respuesta.
4. Interpretar reportes de volante, pedales y botones.
5. Exponer entrada compatible con juegos.
6. Investigar Force Feedback por separado.

No considerar esta versión un driver funcional hasta verificar lectura de reportes en hardware real.
