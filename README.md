# saturnd - cassini

## À propos

Ce projet est un outil de planification de tâches, similaire à Cron. Programmé à l'ocasion d'un projet de 3eme année de licence, pour le cours de systèmes d'exploitation.

`saturnd` est un démon exécutant des tâches à intervalles réguliers, et `cassini` est le client qui permet de communiquer avec, utilisant des tubes nommés.
Le fichier `protocole.md` précise l'le protocole demandé par le projet.

Programmé par Paul Gangneux, Lily Olivier, et Olivier Moreau.

## Compilation et exécution

`make` permet de compiler le client et le démon. `make distclean` permet de nettoyer le dépot.

`saturnd` lance le démon, `cassini [options]` permet d'envoyer une requête au démon.
`cassini -h` liste toutes les requêtes possibles.


