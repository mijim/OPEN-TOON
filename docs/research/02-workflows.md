# Flujos de trabajo y aceptación integrada

Estos escenarios atraviesan módulos y evitan construir funciones aisladas que no sirvan para terminar un plano. Son especificaciones propias para futuras pruebas; no se han ejecutado.

| ID | Persona y trabajo | Recorrido | Resultado verificable |
|---|---|---|---|
| WF-01 | Animador tradicional: pelota de 2 segundos | Crear 1080p/24 fps → dibujar poses → exponer a doses → onion skin → guardar → exportar PNG | 48 imágenes; dibujos compartidos correctos; sin ayudas en exportación; reabrir conserva el trabajo |
| WF-02 | Cleanup: corregir una línea | Abrir rough → subcapa Line Art → lápiz variable → editar contorno → goma → deshacer | Se conserva el rough y el grosor; el historial revierte el gesto completo |
| WF-03 | Colorista: variante nocturna | Paleta por IDs → Colour Art → relleno → clon de paleta → override en una rama | La variante cambia la instancia elegida; otra mantiene el esquema original |
| WF-04 | Animador cut-out: saludo | Rig de torso/brazo/mano → pivotes/pegs → claves → sustituir mano → ajustar ease | Sin saltos de pivote o de registro; curva y preview coinciden |
| WF-05 | Rigger: brazo deformable | Dibujo de brazo → rest pose → curva/envelope → influencias → accesorio con kinematic output | Flexión sin costura relevante en fixture; accesorio sigue el extremo; reset recupera reposo |
| WF-06 | Rigger avanzado: giro de cabeza | Preparar poses compatibles → rejilla de controlador → interpolar → guardar plantilla | Cada muestra recupera su pose; controlador importado funciona sin enlaces a la escena original |
| WF-07 | Animador de diálogo | Importar WAV → onda → scrubbing → detección → mapear bocas → corregir → exportar | Correcciones conservadas; audio y boca alineados dentro de un frame; informe de etiquetas sin mapa |
| WF-08 | Compositor: plano con paralaje | Fondo/medio/personaje → cámara → máscaras → blur/sombra → Write de imagen y vídeo | Orden y alfa coherentes; calidad incompleta de preview visible; salida completa por perfil |
| WF-09 | Generalista 2D/3D | Importar modelo autorizado → unidades → clip → override → componer con dibujos | Misma escala y timing en preview y resultado; límites de materiales expresados |
| WF-10 | Artista de juegos | Ciclo → anclas → hornear efectos → atlas y metadata → reconstruir en visor de referencia | Registro estable de pivotes, duraciones y anclas; sin dependencia del SDK de Harmony |
| WF-11 | Estudio distribuido | Obtener escena/revisión → bloquear recurso → trabajar offline → subir entrega divergente | El conflicto se detecta; ninguna revisión se sobrescribe sin resolución explícita |
| WF-12 | Artista recuperando un fallo | Editar → autoguardar → interrupción durante save → reabrir → localizar textura ausente | Se recupera una revisión coherente; se conservan referencias y se puede relocalizar el recurso |

## Casos límite transversales

| Situación | Resultado requerido |
|---|---|
| Un dibujo expuesto 200 veces | Cambiarlo actualiza sus exposiciones; duplicarlo rompe el vínculo intencionadamente |
| Dos muestras con igual RGB | Su identidad permanece diferenciada |
| Peg con escala negativa | Transformaciones y pivotes siguen reglas explícitas y finitas |
| Capa bloqueada durante un drag | Cancelar o completar de forma definida, sin modificación parcial |
| Frame vacío entre holds | Se distingue de repetir el dibujo anterior |
| Cambio de FPS racional | Audio, marcadores y claves siguen la política seleccionada |
| Plugin de efecto ausente | Se conserva su bloque; el render final no lo omite silenciosamente |
| Escena enorme | Cancelar preview y reducir calidad sigue siendo posible |
| Guardar mientras continúa el dibujo | Se identifica qué revisión se guardó; el documento puede seguir marcado modificado |
| Archivo ajeno con scripts | Abrir no ejecuta código automáticamente |
| Cambio de espacio de color | Preview y salida comunican la conversión aplicada |
| Import con pérdidas | Se informa del subconjunto soportado y se conserva el original según política |

Cada escenario tendrá fixtures propios, criterios cuantitativos donde proceda y grabación de un recorrido real. Durante el plan se asociarán a IDs de capacidades concretos y se descompondrán en entregas verticales completas.
