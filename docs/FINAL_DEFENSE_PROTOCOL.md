# Renforcement M4 : protocole ciblé (avant implémentation)

Politique optionnelle `hardened_security`, désactivée dans les anciens JSON.
Ne changer ni stratégies M3, ni tirages, ni choix de chaîne, ni critères M3.

| Modification | Hypothèse et critère vérifiable | Coût / contrôle |
|---|---|---|
| Confirmations préventives et signaux publics persistants | Un plancher marchand configurable agit dès le début ; les signaux renforcent avant acceptation quand ils précèdent celle-ci. Suspension bornée, confirmations séparées. Mesurer reversals et non-acceptations séparément. | Retard honnête et censure en fin d'observation, sans garantie majoritaire. |
| Probation du producteur public | Des publications tardives répétées justifient un délai local borné visant le contenu du producteur, indépendamment du relais. Tester absence de pénalité du contenu honnête relayé par le suspect. | Retards honnêtes mesurés ; identité de producteur supposée authentique dans le modèle, défense expérimentale. |
| Séquestre de pool | Un déficit suffisant sous le modèle shares/blocs et persistant permet un séquestre temporaire, sans confiscation. Tester déclenchement, expiration, restitution après observations rassurantes. | Crédits honnêtes bloqués et restitués ; déficit statistique ne prouve pas la fraude. |
| Diversité préventive | Appliquer les quotas de groupes publics avant l'isolement, réserver une place à un groupe indépendant accessible. | Connexions/synchronisation supplémentaires ; aucun pair honnête inventé si annuaire inaccessible. |

Les alertes distinguent information, persistance et criticité ; les fenêtres qui se recouvrent ne sont pas des échantillons indépendants. Éviter les renouvellements sans nouvelles informations utiles. Comparaisons de développement sur seeds fixées avant exécution (42 et 43), mêmes budgets de production/observation sans hauteur cible, puis contrôles honnêtes et tests négatifs. Aucun ajustement de stratégie ou sélection de seed favorable après les résultats. Les échecs et les coûts restent publiés. Pas de campagne M5.
