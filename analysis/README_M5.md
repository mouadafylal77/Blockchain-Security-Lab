# Reproduire les benchmarks M5 sans modifier M4

Le protocole est figé dans `m5_protocol.json`. Rapport : `M5_BENCHMARK_REPORT.md`.
Le moteur, les configurations M4 et les tests sont des entrées en lecture seule.
La demande M5 autorise explicitement l'analyse statistique et les figures ici,
sans attendre le jalon d'analyse ultérieur du plan. Aucun tuning n'est exécuté.

Environnement utilisé : Python 3.11.9 installé, NumPy/pandas/psutil déjà présents.
Matplotlib et ses petites dépendances ont été installés seulement dans
`build/m5-python`, sans modifier Python global ni le moteur ; versions dans
`requirements-m5.txt`. L'orchestrateur n'a besoin que de psutil et de la stdlib.
Les scripts doivent utiliser `-B` pour éviter des caches dans les sources.

```powershell
$m5Python = 'C:\Users\LENOVO\AppData\Local\Programs\Python\Python311\python.exe'
& $m5Python -B -m unittest discover -s analysis -p 'test_*.py' -v
& $m5Python -B analysis/benchmark.py verify
# Sur une première campagne seulement, freeze enregistre l'état exact courant.
# Le manifeste existant ne doit pas être remplacé pour masquer une modification.
& $m5Python -B analysis/benchmark.py pilot
& $m5Python -B analysis/benchmark.py run
& $m5Python -B analysis/aggregate.py
& $m5Python -B analysis/audit_statistics.py
```

Le pilote est exclu des 720 évaluations. Une reprise relit les empreintes et valide
les fichiers existants avant de les sauter. Les processus C++ sont strictement
séquentiels. Un verrou Windows empêche deux orchestrateurs pour cette campagne.
Ctrl+C termine proprement le processus courant ; les artefacts des tentatives
précédentes sont conservés. Une éventuelle fin de journal tronquée est archivée
avant réparation du journal, sans toucher aux résultats validés.

Les erreurs de processus, de validation et de résumé restent dans le manifeste.
`benchmark.py repair` ne lance aucun C++ : il revalide/reconstruit uniquement un
résumé manquant après une erreur Python si le processus avait retourné 0.
`--retry-errors` permet une nouvelle tentative technique séparée, sans augmenter
les budgets ; cette option n'a pas été utilisée pour la campagne livrée.

Artefacts bruts : `results/raw/m5_30seeds/manifest.jsonl` et sous-dossiers par
phase/attaque/détecteur/mode/seed/tentative. Les sorties détaillées sont ignorées
par Git ; les tableaux légers et figures sont conservés dans `analysis/`.
Les seuls changements des configurations temporaires sont seed et chemin de
référence honnête. Les fichiers M4 originaux ne sont jamais réécrits.

La synthèse traite les résumés (720 petites lignes), pas toutes les traces en
mémoire. Le bootstrap utilise la seed comme cluster, 5000 répétitions, et des
différences déjà appariées. Aucun graphique n'est généré pendant les simulations.
Les métriques non exportées sont indiquées comme non disponibles dans le rapport.

Pour installer les seuls outils de tracé dans un autre environnement compatible :

```powershell
& $m5Python -m pip install --target build/m5-python --only-binary=:all: --no-deps matplotlib==3.9.2 contourpy==1.3.1 cycler==0.12.1 fonttools==4.55.0 kiwisolver==1.4.7 pillow==11.0.0 pyparsing==3.2.0
```

NumPy, packaging, python-dateutil et six doivent déjà être disponibles à
l'interpréteur ; aucune installation de ces dépendances n'a été nécessaire ici.
Les mesures CPU/RSS sont des observations de cette machine, pas une promesse de
performance. Les références de calibration restent celles de M4, seed 7.
