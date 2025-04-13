# projet-socket

## Utilisation

Pour compiler tous les scripts, faire un `make` dans la racine du projet.

### Conditions d'utilisation

#### Prérequis

- Toutes les VMs (4 par défaut) sont sur le même réseau virtuel ;
- Chacune a une adresse IP valide et unique ;
- Chaque VM "agent" a lancé le script "agent.c" sur un port spécifique, commun à toutes les VMs ;
- Pas de pare-feu bloquerait les transmissions sur le port choisi ;
- Des adresses IP internes correctes sont données en paramètres à l'orchestrateur.

#### Procédure

- S'assurer que chaque VM utilise "NAT" ou "Bridge", pour qu'elles appartiennent toutes au même sous-réseau (comme `192.168.122.X`).

- Sur chaque VM "agent", récupérer son IP (en exécutant `ip a` par exemple). Pour tester la connection avant de lancer le script `orchestrator.c`, vous pouvez exécuter par exemple `ping <ip>` depuis la VM "orchestratrice".

- Si besoin, pour vérifier qu'aucun pare-feu n'est présent sur le port choisi, vous pouvez avoir accès aux pare-feus en place via `sudo firewall-cmd --list-ports`. Si rien n'apparaît, exécutez :

```bash
sudo firewall-cmd --add-port=<port>/tcp --permanent
sudo firewall-cmd --reload
```

- Enfin, exécutez le script `agent.c` sur chaque VM "agent", et le script `orchestrator.c` sur la dernière VM.

### Agents

Pour lancer les agents, compiler le script, et exécuter :

```bash
./agent <numéro de port>
```

**Remarque :** le numéro de port doit être commun à tous les agents.

### Orchestrateur

L'orchestrateur nécessite que les trois agents soient fonctionnels (et lancés) pour pouvoir s'initialiser correctement. Il suffit ensuite d'exécuter :

```bash
./orchestrator <IP1> <IP2> <IP3> <port> [name1 name2 name3]
```

**Notes :** Des noms peuvent être donnés aux agents. Le nombre d'agents est défini en macro dans `orchestrator.c` (`#define NUM_AGENTS 3`).
