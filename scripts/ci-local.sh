#!/bin/bash
set -e

# Script de validation CI/CD local pour ESP32 DPM2
# Reproduit les étapes de la pipeline GitHub Actions

echo "🚀 CI/CD Local - ESP32 DPM2"
echo "=============================="

# Vérifier qu'on est dans le bon dossier
if [ ! -f "platformio.ini" ]; then
    echo "❌ Erreur: Exécuter depuis le dossier DPM2_ESP/"
    exit 1
fi

# Variables
export CI_WERROR=-Werror
FAILED=0

# Fonction de logging
log_step() {
    echo ""
    echo "📋 $1"
    echo "----------------------------------------"
}

# Fonction d'erreur
handle_error() {
    echo "❌ Échec: $1"
    FAILED=1
}

# Étape 1: Tests natifs
log_step "Tests unitaires natifs (43 tests)"
if pio test -e native --without-uploading; then
    echo "✅ Tests natifs: OK"
else
    handle_error "Tests natifs"
fi

# Étape 2: Analyse statique (optionnelle)
log_step "Analyse statique (cppcheck)"
if command -v cppcheck >/dev/null 2>&1; then
    mkdir -p reports
    if cppcheck --std=c11 --enable=all --inline-suppr \
               --suppress=missingIncludeSystem \
               -I include -I src \
               include src 2> reports/cppcheck-local.xml; then
        echo "✅ Cppcheck: OK"
        echo "📄 Rapport: reports/cppcheck-local.xml"
    else
        echo "⚠️  Cppcheck: Warnings détectés (voir reports/cppcheck-local.xml)"
    fi
else
    echo "⚠️  Cppcheck non installé (optionnel)"
    echo "   Installation: sudo apt-get install cppcheck (Ubuntu)"
    echo "                brew install cppcheck (macOS)"
fi

# Étape 3: Build ESP32
log_step "Build firmware ESP32"
if pio run -e esp32dev; then
    echo "✅ Build ESP32: OK"
    
    # Copier les artefacts
    mkdir -p out
    if cp .pio/build/esp32dev/*.{bin,elf,map} out/ 2>/dev/null; then
        echo "📦 Artefacts copiés dans out/"
        ls -la out/
    else
        echo "⚠️  Artefacts partiellement copiés"
    fi
else
    handle_error "Build ESP32"
fi

# Étape 4: Tests sur cible (optionnel)
log_step "Tests sur cible ESP32 (optionnel)"
echo "⚠️  Tests sur cible nécessitent un ESP32 connecté"
echo "   Commande: pio test -e esp32dev"

# Résumé
echo ""
echo "🏁 Résumé CI/CD Local"
echo "=============================="
if [ $FAILED -eq 0 ]; then
    echo "✅ Toutes les vérifications sont passées !"
    echo "🚀 Votre code est prêt pour le push"
else
    echo "❌ Des vérifications ont échoué"
    echo "🔧 Corrigez les erreurs avant de push"
    exit 1
fi

echo ""
echo "📊 Statistiques:"
echo "  - Tests natifs: 43 tests sur 8 modules"
echo "  - Build cible: ESP32-WROOM-32D"
echo "  - Flags: -Wall -Wextra -Werror"
echo ""
echo "🔗 Prochaines étapes:"
echo "  1. git add ."
echo "  2. git commit -m 'votre message'"
echo "  3. git push origin votre-branche"
echo "  4. La pipeline CI/CD se déclenchera automatiquement"
