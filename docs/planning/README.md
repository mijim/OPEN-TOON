# Preparación del plan largo de implementación

**El plan todavía no está elaborado.** Esta etapa entrega el análisis para revisarlo antes de planificar todas las funcionalidades, tal como solicitó el usuario.

## Información ya preparada

- 282 capacidades con IDs estables, módulos, fuentes y aceptación inicial.
- 26 dominios y sus relaciones; no interpretar una relación como obligación de completar todo un dominio.
- Inventario de nodos que permite desglosar operadores sin perder áreas menos frecuentes.
- Arquitectura, formato, principios de UI y 26 objetivos de calidad.
- Ocho pruebas de viabilidad, doce flujos de usuario y riesgos explícitos.

## Cómo construir el plan en la siguiente etapa

1. Revisar la recomendación tecnológica y decidir las pruebas que cierran incertidumbres fundamentales.
2. Fijar el primer flujo útil y las plataformas/dispositivos iniciales. Propuesta de partida: crear una escena, dibujar, exponer, guardar, reabrir y exportar imágenes.
3. Convertir relaciones de dominio en dependencias **entre capacidades**. El audio básico no debe esperar a terminar rigging solo porque el dominio AUD incluya lip-sync.
4. Agrupar entregas verticales demostrables; añadir dibujo/pintura, animación/rig y composición sobre contratos estables.
5. Desglosar todo el horizonte, incluyendo deformación avanzada, 3D, gaming, scripting y extensión de estudio. Separar de la base IA opcional y formatos legacy con decisión explícita sobre cada uno.
6. Para cada entrega: objetivo, IDs incluidos, dependencias, diseño necesario, fixtures, aceptación, riesgos, documentación y criterio de salida.
7. Estimar esfuerzo en rangos con supuestos de equipo. Recalibrar después de los primeros spikes; no inferir plazos por el número de funciones.

## Formato propuesto para una unidad de trabajo

```yaml
id: PLAN-0001
status: proposed
outcome: "Un animador puede completar una operación observable"
feature_ids: []
requires: []
modules: []
contracts_to_define: []
fixtures: []
acceptance: []
quality_requirements: []
risks: []
effort_range: null
assumptions: []
exit_evidence: []
```

El orden deberá seguir dependencias y valor para el animador: persistencia, identidades, tiempo y entrada condicionan muchas áreas; una galería de pantallas sin modelo de datos no demuestra esas capacidades. Colaboración distribuida y paridad avanzada no deben bloquear la herramienta local útil.

## Decisiones que requieren revisión

Tecnología y posible reutilización de OpenToonz; prioridades FBF/cut-out; primer sistema y tableta; nivel de compatibilidad de intercambio; recursos humanos; licencia inicial y nombre de producto. No son solicitudes de confirmación para los documentos ya creados: son entradas del siguiente trabajo de planificación.
