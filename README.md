# projet-socket

## Utilisation

Pour compiler tous les scripts, faire un `make` dans la racine du projet.

### Agents

```bash
./agent <numéro de port>
```

### Orchestrateur

L'orchestrateur nécessite que les trois agents soient fonctionnels (et lancés) pour pouvoir s'initialiser correctement. Il suffit ensuite d'exécuter :

```bash
./orchestrator 127.0.0.1 2222
```
