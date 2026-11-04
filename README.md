# Blockchain Security Lab — Atelier 2

État : audit M0 et noyau minimal M1. Le programme C++17 valide une chaîne PoW
linéaire déterministe. Il ne constitue pas encore le simulateur expérimental complet.
`exo4.cpp` reste le code de référence Atelier 1, sans modification et hors compilation.

## Construire et vérifier sous Windows

Dépendances : GCC compatible C++17 **MSYS2 UCRT64**, OpenSSL Crypto UCRT64,
CMake >= 3.21, Ninja et CTest. Aucun besoin de Python ni de serveur web.
Installation MSYS2 habituelle, à effectuer dans un terminal UCRT64 si nécessaire :

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

Depuis PowerShell à la racine du dépôt :

```powershell
./scripts/test-ucrt64.ps1
```

Le script cherche CMake/Ninja dans UCRT64, puis le PATH, puis l'installation
Visual Studio 2022 BuildTools habituelle. Il affiche les chemins réellement employés.
CTest est recherché à côté de CMake. Les outils de construction natifs de Visual
Studio peuvent piloter GCC UCRT64 : le compilateur reste explicitement `g++.exe`,
sans MSVC. Le script ne télécharge ni n'installe de logiciel et restaure le PATH.
Il s'arrête avec une erreur si une dépendance ou une étape échoue.

Le preset suppose MSYS2 dans `C:/msys64/ucrt64`. Adapter le preset pour un autre
emplacement. Si les outils sont déjà dans le PATH, les étapes sont :

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
cmake --preset ucrt64
cmake --build --preset ucrt64 --parallel 2
ctest --preset ucrt64 --output-on-failure
./build/ucrt64/blockchain_sim.exe --demo
```

Le preset de compilation limite aussi les tâches à deux. Les fichiers générés
restent dans `build/ucrt64/`, ignoré par Git. CTest exécute 13 tests : neuf cas
du noyau et quatre cas CLI. Les vérifications C++ utilisent des exceptions et
restent actives dans une compilation Release (`NDEBUG`).

## CLI M1

```text
blockchain_sim [--demo|--help]
```

Sans argument ou avec `--demo`, le programme crée le genesis et mine deux blocs
avec difficulté 2, aux instants simulés 1 et 2. Il affiche le scénario, l'unité de
temps, les indices, nonces, hashes et `chain_valid=true`. Aucun aléa ni temps réel
n'intervient ; une seed est donc sans objet à ce stade. Codes de sortie : 0 succès,
1 erreur de minage/validation, 2 arguments invalides.

L'entrée JSON, `--config`, `--output` et les exports CSV/JSON sont différés aux
jalons du simulateur. Aucun résultat expérimental n'est produit par cette démo.
Le parseur JSON sera choisi lors de son introduction ; les tests actuels ne
nécessitent aucun framework externe.

## Modèle et périmètre

Voir [le modèle du noyau](docs/SIMULATION_MODEL.md) et
[l'audit et le journal](docs/DEVELOPMENT_LOG.md). Les branches et leur sélection
par travail cumulé, le réseau multi-nœuds, les attaques, la détection et les
contre-mesures restent à implémenter. Backend et frontend sont différés.
