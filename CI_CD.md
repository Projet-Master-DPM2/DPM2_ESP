# CI/CD Pipeline - ESP32 DPM2

Ce document décrit la pipeline CI/CD mise en place pour le projet ESP32 du distributeur de boissons intelligent.

## 🔄 Workflow GitHub Actions

Le workflow `esp32-ci.yml` s'exécute automatiquement sur :
- **Push** sur les branches `main` et `develop`
- **Pull Requests** 
- **Tags** de version (format `v*.*.*`)

## 🧪 Jobs de la Pipeline

### 1. `native-tests` - Tests Unitaires Natifs
- **Durée** : ~30 secondes
- **Tests** : 43 tests unitaires sur 8 modules
- **Environnement** : Ubuntu + PlatformIO native
- **Flags** : `-Werror` (warnings = erreurs)

**Modules testés :**
- `cli` - Parsing des commandes CLI
- `uart_parser` - Validation et parsing UART  
- `http_utils` - Validation des URLs
- `nfc_ndef` - Parsing NDEF et conversion UID
- `orchestrator_logic` - Logique métier des événements
- `wifi_validation` - Validation des paramètres Wi-Fi
- `nfc_utils` - Utilitaires NFC avancés
- `http_builder` - Construction des requêtes HTTP

### 2. `cppcheck` - Analyse Statique
- **Outil** : cppcheck
- **Mode** : Conseil (n'interrompt pas la pipeline)
- **Sortie** : Rapport XML en artefact

### 3. `build-esp32` - Compilation Firmware
- **Cible** : ESP32-WROOM-32D (`esp32dev`)
- **Flags** : `-Werror` (warnings = erreurs)
- **Artefacts** : `.bin`, `.elf`, `.map`
- **Format** : ZIP avec hash du commit

### 4. `release` - Publication GitHub
- **Condition** : Tags `v*.*.*` uniquement
- **Contenu** : Firmware + métadonnées
- **Automatique** : Création de release GitHub

### 5. `email` - Notification
- **Condition** : Toujours (succès ou échec)
- **Contenu** : Status, commit, liens vers artefacts/release

## 🛠️ Configuration Requise

### Secrets GitHub à configurer :

```bash
# Configuration SMTP pour les emails
SMTP_HOST=smtp.gmail.com          # Serveur SMTP
SMTP_PORT=587                     # Port SMTP
SMTP_USERNAME=your@email.com      # Utilisateur SMTP
SMTP_PASSWORD=your-app-password   # Mot de passe d'application
MAIL_FROM=ci@yourproject.com      # Expéditeur
MAIL_TO=team@yourproject.com      # Destinataire
```

### Variables d'environnement :

- `CI_WERROR=-Werror` : Active le mode strict (warnings = erreurs)
- `PIO_CACHE_KEY` : Cache PlatformIO (automatique)

## 📊 Métriques de Performance

| Job | Durée Typique | Cache |
|-----|---------------|-------|
| Tests natifs | ~30s | PlatformIO |
| Cppcheck | ~20s | APT packages |
| Build ESP32 | ~60s | PlatformIO |
| Release | ~10s | - |
| Email | ~5s | - |

**Total** : ~2-3 minutes

## 🚀 Utilisation

### Tests locaux avant push :
```bash
# Tests natifs (comme CI)
cd DPM2_ESP
CI_WERROR=-Werror pio test -e native

# Build avec warnings stricts
CI_WERROR=-Werror pio run -e esp32dev

# Tests sur cible ESP32
pio test -e esp32dev
```

### Créer une release :
```bash
git tag v1.0.0
git push origin v1.0.0
# → Déclenche automatiquement la pipeline de release
```

### Monitoring des builds :
- **GitHub Actions** : Onglet Actions du repository
- **Emails** : Notifications automatiques sur succès/échec
- **Artefacts** : Téléchargeables pendant 90 jours

## 🔧 Maintenance

### Mise à jour des dépendances :
- Modifier `platformio.ini`
- Les caches se mettent à jour automatiquement

### Ajout de nouveaux tests :
1. Créer le test dans `test/test_*_native/`
2. Ajouter au `test_filter` de `[env:native]`
3. La pipeline détecte automatiquement les nouveaux tests

### Debug des échecs CI :
1. Consulter les logs GitHub Actions
2. Reproduire localement avec les mêmes flags
3. Les artefacts cppcheck aident pour l'analyse statique

## 📈 Qualité Code

- **Coverage** : Tests unitaires sur logique métier critique
- **Static Analysis** : cppcheck intégré
- **Compiler Warnings** : Mode `-Werror` strict
- **Dependencies** : Versions épinglées dans `platformio.ini`

La pipeline garantit que seul du code de qualité atteint les branches principales et les releases.
