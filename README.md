# PS3 G923 Driver

Proyecto homebrew para investigar la compatibilidad del Logitech G923 Xbox con PS3 (HEN).

## Estado

**Etapa inicial: esqueleto de protocolo y diagnóstico.** Todavía no es un driver de entrada para juegos y no implementa transferencia USB, enumeración, sys_usbd ni Force Feedback.

## Evidencia confirmada en PC

- G923 Xbox detectado inicialmente como USB VID:PID `046D:C26D`.
- Interfaz inicial observada: `FF/47/D0`, endpoints observados `0x01 OUT` y `0x81 IN`; el tipo exacto debe confirmarse con el descriptor completo.
- Con G HUB cerrado, enviar `0F 00 01 01 42` al endpoint OUT `0x01` produjo calibración y el volante pasó a estar disponible en `joy.cpl`.
- El cambio de PID, la topología posterior y el formato de los reportes aún deben confirmarse a partir del PCAP/descriptor. No se asumen en el código.

## Estructura

- `src/main.c`: aplicación homebrew mínima para validar el entorno PSL1GHT.
- `src/g923_protocol.c/.h`: constantes del protocolo observadas en PC.
- `.github/workflows/build.yml`: compilación automatizada.

## Compilar

Con PS3DEV/PSL1GHT instalado y las variables `PS3DEV` y `PSL1GHT` configuradas:

```sh
make
make pkg
```

El paquete generado se llama `G923Diag.pkg`.

## Próximas etapas

1. Validar PCAP USBPcap y descriptores pre/post inicialización.
2. Implementar enumeración y control USB mediante la API disponible en PSL1GHT/sys_usbd.
3. Confirmar reportes de volante, pedales y botones.
4. Exponer entrada compatible con juegos.
5. Investigar Force Feedback por separado.

No instalar ni probar en una consola principal hasta que el PKG y el manejo USB estén validados.
