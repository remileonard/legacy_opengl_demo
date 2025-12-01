# Notes de Portage X11 vers GLUT

## Vue d'ensemble

Ce dossier contient une démonstration OpenGL "Space" qui a été portée de X11/GLX vers GLUT pour assurer la compatibilité multiplateforme, notamment Windows.

## Modifications effectuées

### 1. Layer de compatibilité pour les images SGI

**Fichiers créés :**
- `image_compat.h` - Header de remplacement pour `gl/image.h`
- `image_compat.c` - Implémentation des fonctions `iopen()`, `iclose()`, `getrow()`, `putrow()`

**Description :**
La bibliothèque image SGI (`gl/image.h`) n'est pas disponible sur les systèmes modernes. Un layer de compatibilité a été créé pour lire et écrire les fichiers RGB au format SGI. L'implémentation supporte :
- Lecture/écriture de fichiers RGB
- Format verbatim (non compressé)
- Conversion automatique big-endian/little-endian
- Support partiel du format RLE (à améliorer si nécessaire)

### 2. Portage X11/GLX vers GLUT

**Fichiers créés :**
- `events_glut.c` - Remplacement de `events.c` avec des appels GLUT
- `main_glut.c` - Remplacement de `main.c` avec une boucle principale GLUT

**Fichiers modifiés :**
- `space.h` - Structure `t_wndw` simplifiée (suppression de Display, Window, XVisualInfo, etc.)
- `imlib.c` - Utilisation du nouveau `image_compat.h`
- `input.c` - Headers système conditionnels pour Windows
- `CMakeLists.txt` - Exclusion des fichiers X11, inclusion des versions GLUT

**Changements principaux :**

#### Gestion des fenêtres
- `XOpenDisplay()` → `glutInit()`
- `XCreateWindow()` → `glutCreateWindow()`
- `glXSwapBuffers()` → `glutSwapBuffers()`
- Plein écran : `glutFullScreen()`

#### Gestion des événements
- Boucle événementielle X11 → Callbacks GLUT
- `glutKeyboardFunc()` pour le clavier
- `glutSpecialFunc()` pour les touches spéciales
- `glutMouseFunc()` pour les boutons de souris
- `glutMotionFunc()` / `glutPassiveMotionFunc()` pour les mouvements

#### Boucle principale
L'ancienne boucle `while(1)` a été remplacée par :
- `glutDisplayFunc(display_callback)` - Callback d'affichage
- `glutIdleFunc(idle_callback)` - Callback de mise à jour
- `glutMainLoop()` - Boucle principale GLUT

#### Rendu de texte
- Remplacement des fontes X11 par `glutBitmapCharacter()`
- Utilisation de `GLUT_BITMAP_HELVETICA_12`

### 3. Compatibilité Windows

**Modifications pour Windows :**
- Includes conditionnels (`#ifdef _WIN32`)
- `Sleep()` au lieu de `usleep()`
- `MessageBeep()` pour le bell
- `_getcwd()` au lieu de `getcwd()`
- `GetCurrentDirectory()` pour les chemins
- Gestion des chemins avec backslashes

## Compilation

```bash
# Configuration avec CMake
cmake -B build -S .

# Compilation
cmake --build build
```

## Limitations connues

1. **spReadFloat() et spReadStar()** - Ces fonctions nécessitent une implémentation plus complexe avec GLUT car elles attendent une saisie clavier interactive. Pour l'instant, elles retournent des valeurs par défaut.

2. **Spaceball** - Non supporté par GLUT (l'ancien code utilisait des devices SGI spécifiques).

3. **RLE compression** - La compression RLE dans le format RGB SGI est partiellement implémentée. Les fichiers fortement compressés peuvent ne pas se charger correctement.

4. **Bell/Beep** - Sur non-Windows, le bell n'est pas implémenté car GLUT n'a pas d'API pour cela.

5. **Fonts** - Les polices X11 ne sont plus supportées, utilisation de fonts GLUT bitmap basiques.

## Améliorations futures

1. Améliorer l'implémentation RLE dans `image_compat.c`
2. Implémenter un système de saisie clavier pour `spReadFloat()` et `spReadStar()`
3. Utiliser des polices TrueType avec FreeType au lieu des fonts bitmap GLUT
4. Ajouter support FreeGLUT pour `glutMainLoopEvent()` (meilleur contrôle de la boucle)

## Dépendances

- **GLUT** ou **FreeGLUT** - Gestion des fenêtres et événements
- **OpenGL** - Rendu graphique
- **GLU** - Utilitaires OpenGL

## Fichiers X11 originaux (non utilisés)

- `events.c` - Version X11 originale
- `main.c` - Version X11 originale

Ces fichiers sont conservés pour référence mais ne sont pas compilés.
