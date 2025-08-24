# Changelog - DPM2 ESP32

Toutes les modifications notables du projet ESP32 seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/lang/fr/).

## [2.0.0] - 2025-08-XX

### 🎯 **Ajouté - Système de Supervision**

- **Service de supervision** : Nouveau service pour détecter et notifier les erreurs critiques
- **Notifications HTTP** : Envoi automatique d'erreurs vers l'API backend
- **Rate limiting** : Protection contre le spam (30 secondes entre notifications)
- **ID machine unique** : Génération automatique basée sur l'ESP32
- **Intégration orchestrator** : Notifications pour les échecs de parsing JSON, génération de commandes, et livraison
- **Réception erreurs NUCLEO** : Traitement des messages `SUPERVISION_ERROR:` via UART
- **Configuration environment** : Endpoint de supervision configurable via `.env`

### 🔧 **Modifié - Workflow de Commande Simplifié**

- **Workflow simplifié** : Suppression des étapes "Mise à jour Stock" et "Mise à jour Statut"
- **États du workflow** : Réduction de 7 à 5 états (`WORKFLOW_COMPLETED` passe de 7 à 5)
- **Gestion automatique backend** : Le backend gère maintenant automatiquement la mise à jour du stock et du statut
- **Performance améliorée** : Moins de requêtes HTTP, latence réduite
- **Fiabilité accrue** : Moins de points de défaillance potentiels

### 🔧 **Corrigé - Compilation et Intégration**

- **Header supervision** : Recréation complète du header manquant
- **HTTPClient** : Ajout de l'inclusion manquante
- **Inclusion main.cpp** : Force la compilation du service de supervision
- **Compilation réussie** : Tous les services compilent sans erreur

### 📚 **Ajouté - Documentation**

#### **Documentation Technique**
- **SUPERVISION_SYSTEM.md** : Documentation complète du système de supervision
- **SUPERVISION_COMPILATION_FIXES.md** : Corrections de compilation pour ESP32
- **WORKFLOW_SIMPLIFICATION.md** : Explication de la simplification du workflow
- **UART_PROTOCOL.md** : Documentation du protocole de communication UART
- **ORDER_WORKFLOW.md** : Workflow de commande mis à jour
- **QUANTITY_UPDATE.md** : Documentation de la mise à jour des quantités

#### **Configuration**
- **env.example** : Ajout de `API_SUPERVISION_ENDPOINT`
- **env_config.h/cpp** : Support pour l'endpoint de supervision

## [1.0.0] - 2025-07-XX

### 🎯 **Ajouté - Fonctionnalités de Base**

- **Validation QR Code** : Lecture et validation des tokens de commande
- **Workflow de commande complet** : Validation → Livraison → Mise à jour → Confirmation
- **Communication UART** : Protocole structuré avec NUCLEO
- **API Backend** : Intégration complète avec l'API de vente
- **Configuration environment** : Gestion des endpoints via `.env`
- **Services modulaires** : NFC, QR, WiFi, HTTP, UART, Orchestrator
- **Sécurité** : Chiffrement AES-256, validation TLS, masquage des données sensibles

### 🏗️ **Ajouté - Infrastructure**

#### **CI/CD Pipeline**
- **GitHub Actions** : Automatisation complète des builds et tests
- **Tests unitaires** : Framework Unity pour tests natifs et embarqués
- **Analyse statique** : Cppcheck pour détection d'erreurs
- **Notifications** : Emails automatiques avec résultats de tests
- **Artefacts** : Génération de firmwares prêts à flasher

#### **Tests et Qualité**
- **Tests unitaires** : Couverture complète des services critiques
- **Mocks** : Simulation des dépendances matérielles
- **Validation** : Tests d'intégration et de workflow
- **Documentation** : Guides d'utilisation et API

### 🔒 **Ajouté - Sécurité**

#### **Analyse OWASP**
- **Vulnérabilités critiques** : Identification et correction
- **Chiffrement** : AES-256 pour les données sensibles
- **Validation TLS** : Certificats CA et vérification d'hôte
- **Masquage** : Protection des informations sensibles dans les logs
- **Watchdog** : Protection contre les blocages système

### 📚 **Ajouté - Documentation**

#### **Documentation Technique**
- **README.md** : Documentation complète du projet
- **Configuration** : Guides d'installation et configuration
- **API** : Documentation des endpoints et protocoles
- **Sécurité** : Rapport d'analyse OWASP et mesures
- **Tests** : Documentation des tests et couverture

---

## Format des Versions

- **MAJOR** : Changements incompatibles avec les versions précédentes
- **MINOR** : Nouvelles fonctionnalités compatibles
- **PATCH** : Corrections de bugs compatibles

## Types de Changements

- **Ajouté** : Nouvelles fonctionnalités
- **Modifié** : Changements dans les fonctionnalités existantes
- **Déprécié** : Fonctionnalités qui seront supprimées
- **Supprimé** : Fonctionnalités supprimées
- **Corrigé** : Corrections de bugs
- **Sécurité** : Corrections de vulnérabilités
