# Raytracing

Pour compiler : (Cmake, Ninja, Mingw-w64)

Depuis la racine du projet :

```bash
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build . --config Release -j 10
```

Ou ouvrir le projet à l'aide de CLion

Et pour exécuter (Depuis le dossier build) :

````bash
./Raytracing1
````

Si la compilation ne marche pas, l'exécutable est localisé dans le dossier `cmake-build-debug`

## Ce qui est fait

- **Ray-Sphere**
- **Ray-Plane**
- **Ray-TriangleMesh**
- **Movable Camera** *2 types de movements, contrôle avec ImGui et contrôle au clavier. Peut être changer avec la touche ***C****
- **Movable Light**
- **2x specular term: Phong and Blinn** *Peut être changer avec la touche ***&****
- **Shadows**
- **Antialiasing**
- **CSG** partiellement fonctionnel, le complément cause un glitch visuel
- **Control recursion depth** *Peut être augmenté/diminué avec les touches ***+*** / ***-*** (aucune limite)*
- **Reflexion**
- **Refraction**
- **Absorption**
- **Fresnel** glitch avec la sphère creuse
- **Soft shadows** Très laggy, mais fonctionnel. Requiert un sample rate élevé pour un rendu correct.
- **Glossy Reflection** Requiert antialiasing.
- **Motion blur**
- **Depth-of-Field**

## Bonus

- Compute shader pour optimisation des performances. (Pas d'ombre)
