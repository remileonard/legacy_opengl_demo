# Notes de portage libtk : X11/GLX vers GLUT/Windows

## Résumé
La bibliothèque libtk a été portée de X11/GLX (Unix/Linux) vers GLUT pour Windows, tout en préservant la compatibilité de l'API.

## Modifications principales

### 1. Includes OpenGL
- **Problème** : Sous Windows, `windows.h` doit être inclus AVANT les headers OpenGL
- **Solution** : Ajout de `#include <windows.h>` dans `tk.h` avant `GL/gl.h` et `GL/glu.h`
- **Fichiers modifiés** : `tk.h`, `private.h`

### 2. Gestion des palettes de couleurs

#### Mode indexé simulé
En X11, les applications pouvaient utiliser des palettes de couleurs indexées. Sous Windows avec GLUT en mode RGB moderne, ces palettes sont simulées en mémoire :

- **`tkSetOneColor(index, r, g, b)`** : Stocke une couleur dans la palette simulée
- **`tkGetColorRGB(index, &r, &g, &b)`** : Récupère une couleur de la palette (nouvelle fonction)
- **`tkSetRGBMap(size, rgb)`** : Définit toute la palette à partir d'un tableau
- **`tkSetGreyRamp()`** : Crée une rampe de gris
- **`tkSetFogRamp(density, startIndex)`** : Crée une rampe pour le brouillard

**Note** : Les applications doivent maintenant appeler `glColor3f()` avec les valeurs de la palette pour utiliser les couleurs indexées.

### 3. Gestion de la souris

#### Position de la souris
- **Problème** : X11 permettait de récupérer la position de la souris à tout moment avec `XQueryPointer()`
- **Solution** : GLUT ne fournit la position que dans les callbacks. On stocke la dernière position connue.
- **`tkGetMouseLoc(x, y)`** : Retourne la dernière position connue de la souris
- **`tkUpdateMousePosition(x, y)`** : Fonction interne appelée par les callbacks GLUT

Les callbacks `glutMouseFunc`, `glutMotionFunc` et `glutPassiveMotionFunc` appellent automatiquement `tkUpdateMousePosition()`.

### 4. Curseurs personnalisés

#### Limitation GLUT
- **Problème** : GLUT ne supporte que des curseurs prédéfinis (pas de curseurs bitmap personnalisés comme X11)
- **Solution** : Mapping des IDs de curseurs vers des curseurs GLUT standard
- **`tkNewCursor(id, shapeBuf, maskBuf, ...):`** : Crée un curseur (mappe vers un curseur GLUT prédéfini)
- **`tkSetCursor(id)`** : Active un curseur par son ID

**Curseurs disponibles** : 
- ID 0 → `GLUT_CURSOR_LEFT_ARROW`
- ID 1 → `GLUT_CURSOR_CROSSHAIR`
- ID 2 → `GLUT_CURSOR_INFO`
- Autres → `GLUT_CURSOR_INHERIT`

Pour des curseurs vraiment personnalisés, il faudrait utiliser l'API Windows native (`LoadCursor`, `SetCursor`).

### 5. Overlays
- **Limitation** : GLUT ne supporte pas les overlays de la même manière que GLX
- **`tkSetWindowLevel(TK_OVERLAY)`** : Retourne `GL_FALSE` (non supporté)
- **`tkSetOverlayMap()`** : Stocke la palette mais ne l'applique pas

### 6. Fonctions système
- **`tkGetSystem(TK_X_DISPLAY)` / `tkGetSystem(TK_X_WINDOW)`** : Retourne NULL (pas d'accès direct au Display/Window)
- **`tkGetColorMapSize()`** : Retourne 256 (taille de la palette simulée)

## Compatibilité

### Fonctions complètement portées
✅ `tkInitDisplayMode()`, `tkInitPosition()`, `tkInitWindow()`
✅ `tkSwapBuffers()`, `tkCloseWindow()`, `tkQuit()`
✅ `tkExec()` - Utilise `glutMainLoop()`
✅ Tous les callbacks : Display, Reshape, Keyboard, Mouse, Motion, Idle
✅ `tkSetOneColor()`, `tkSetRGBMap()`, `tkSetGreyRamp()`, `tkSetFogRamp()`
✅ `tkGetMouseLoc()` - Avec limitation (dernière position connue)
✅ `tkNewCursor()`, `tkSetCursor()` - Avec limitation (curseurs prédéfinis uniquement)

### Fonctions avec limitations
⚠️ `tkSetWindowLevel()` - Overlays non supportés
⚠️ `tkGetSystem()` - Ne retourne pas de Display/Window X11
⚠️ Palettes de couleurs - Mode indexé simulé uniquement

### Fonctions inchangées
✅ Toutes les fonctions de dessin : `tkWireSphere()`, `tkSolidCube()`, etc.
✅ Gestion des polices : `tkCreateBitmapFont()`, etc.
✅ Chargement d'images : `tkRGBImageLoad()`

## Utilisation

Les applications utilisant libtk n'ont besoin d'aucune modification de code dans la plupart des cas. Les limitations principales concernent :

1. **Mode indexé** : Les applications en mode indexé doivent utiliser `tkGetColorRGB()` puis `glColor3f()` au lieu de `glIndex()`
2. **Curseurs** : Les curseurs personnalisés sont mappés vers des curseurs standard
3. **Overlays** : Non supportés

## Exemple : Mode indexé

```c
// Ancien code X11 (mode indexé hardware)
tkSetOneColor(42, 1.0f, 0.5f, 0.0f);
glIndexi(42);  // Utilise la couleur 42

// Nouveau code GLUT (mode indexé simulé)
tkSetOneColor(42, 1.0f, 0.5f, 0.0f);
float r, g, b;
tkGetColorRGB(42, &r, &g, &b);
glColor3f(r, g, b);  // Utilise la couleur RGB
```

## Fichiers modifiés

- `src/libtk/tk.h` - Ajout de `windows.h` et `glut.h`, ajout de `tkGetColorRGB()`
- `src/libtk/private.h` - Suppression des références X11, ajout de `tkUpdateMousePosition()`
- `src/libtk/window.c` - Portage complet vers GLUT
- `src/libtk/event.c` - Callbacks GLUT, mise à jour position souris
- `src/libtk/cursor.c` - Mapping curseurs GLUT
- `src/libtk/getset.c` - Simulation des palettes, gestion position souris
- `src/libtk/font.c` - Inchangé (utilise déjà OpenGL pur)
- `src/libtk/shapes.c` - Inchangé (utilise déjà OpenGL pur)
- `src/libtk/image.c` - Inchangé
