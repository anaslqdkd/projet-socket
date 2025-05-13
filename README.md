# projet-socket

## Utilisation

Pour compiler tous les scripts, faire un `make` dans la racine du projet.

### Conditions d'utilisation

#### Dépendances

- openssl
```bash
sudo apt install libssl-dev # pour debian/ubuntu etc
sudo dnf install openssl openssl-devel # pour fedora  
sudo pacman -S openssl # arch
```
pour les autres dépendances, à installer de la même manière en remplaçant par le programme cible
- gcc pour compiler le projet
- nikto, nmap et zap/owasp sur chaque machine correspondant à l'agent

#### Prérequis

- sudo dnf install openssl openssl-devel

#### Remarques

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

- Générer les clés du CA, si elles ne sont pas déjà générées, en exécutant `./src/generate_ca.sh`.

- Enfin, exécutez les scripts `agent.c` sur chaque VM "agent", et `orchestrator.c` sur la dernière VM, en suivant les protocoles ci-dessous.

### Agents

Pour lancer les agents, il faut d'abord générer le certificat et la clé qui permettront l'encryption des communications. Pour cela, exécuter (sur chaque VM "agent") :

```bash
./generate_agent_cert.sh <nom de l'agent>
```

Vous aurez peut-être besoin de rendre ce script exécutable avec `chmod +x generate_certification.sh`. Une fois que le certificat et la clé sont générés, il suffit compiler le script, et exécuter :

```bash
./agent <numéro de port>
```

**Remarque :** le numéro de port doit être commun à tous les agents.

### Orchestrateur

De même, l'orchestrateur aura besoin de certificat et clé, il faudra donc exécuter sur la VM "orchestratrice" (avec éventuellement un `chmod`) :

```bash
./generate_orchestrator_cert.sh
```

L'orchestrateur nécessite enfin que les trois agents soient fonctionnels (et lancés) pour pouvoir s'initialiser correctement. Il suffit ensuite d'exécuter :

```bash
./orchestrator <IP1> <IP2> <IP3> <port> [name1 name2 name3]
```

**Notes :** Des noms peuvent être donnés aux agents. Le nombre d'agents est défini en macro dans `orchestrator.c` (`#define NUM_AGENTS 3`).

### Fonctionnement

Un agent execute la commande reçue dans un thread dédié, cela permet à l'orchestrateur de continuer à envoyer des commandes à d'autres agent sans être bloqué par l'exécution d'une autre commande. La réponse est envoyé uniquement lorsque **l'exécution est terminée** afin d'éviter d'avoir des mélanges des réponses des differents commandes préalablement executés, par conséquent, il est attendu de n'avoir un retour qu'après un certain temps, notamment lorsque la commande requert beaucoup de temps (par exemple, un scan de ports large ou une analyse complète de vulnérabilités). On a toute de même mis un temps limite afin d'éviter des commandes bloqués sans réponse.

On pourra essayer d'executer les commandes suivantes dans un premier temps pour voir le fonctionnement sans intrusion dans un domain non autorisé : 
```bash
nmap -T4 -F scanme.nmap.org 
```
```bash
sudo nmap -sS -sV -T4 -v -p- scanme.nmap.org # commande plus lente
```
Pour les commandes en sudo, il faudra les executer dans un shell root (pas testé mais à priori ça ne devrait pas poser problème) ou depuis une machine qui ne requert pas de mot de passe pour root (ça c'est testé).

