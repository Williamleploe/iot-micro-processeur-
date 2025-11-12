# 🔧 Projet IoT : Système de contrôle d’accès intelligent (RFID + Empreinte + Servo + MQTT + LCD + Web)

## 🧩 1. Définition du concept

### 🎯 Objectif
Créer un **système de contrôle d’accès intelligent** utilisant un **badge RFID** ou une **empreinte digitale** pour **ouvrir une porte via un servo moteur**, tout en :
- affichant les informations sur un **écran LCD**,
- communiquant via **MQTT**,
- et offrant une **interface web** de supervision et de contrôle.

### 🔹 Fonctions principales
- Lecture RFID et empreinte → validation d’accès  
- Mouvement du servo (porte) si autorisé  
- Envoi des logs au broker MQTT  
- Affichage local sur LCD  
- Supervision via interface web  

---

## ⚙️ 2. Matériel nécessaire

| Élément | Rôle | Remarques |
|----------|------|-----------|
| **ESP32** | Microcontrôleur principal | Wi-Fi intégré, idéal pour MQTT |
| **Module RFID RC522** | Lecture des badges | SPI |
| **Lecteur d’empreinte (R503)** | Identification biométrique | UART |
| **Servo moteur (SG90 / MG995)** | Ouverture/fermeture de porte | PWM |
| **Écran LCD 16x2 (I2C)** | Affichage local | 4 fils : VCC, GND, SDA, SCL |
| **Alimentation 5V / 2A** | Alimente les modules | Stable et sécurisée |
| **Câbles Dupont + breadboard** | Connexions | Mâle/Femelle |
| **Tapis antistatique (ESD)** | Sécurité électronique | Protection ESD |
| **PC avec MQTTX + Mosquitto** | Communication MQTT | Broker + client |
| **Navigateur Web** | Interface de supervision | MQTT.js |

---

## 🧠 3. Partie électronique (câblage et tests)

### Étapes :
1. Connecter le **RFID RC522** à l’ESP32 (SPI)  
2. Connecter le **lecteur d’empreinte** (TX/RX)  
3. Brancher le **servo moteur** (PWM)  
4. Connecter le **LCD I2C** (SDA, SCL)  
5. Alimenter et vérifier les tensions (3.3V / 5V)  
6. Tester chaque module séparément avec un code simple Arduino  

🎯 **Objectif :** Vérifier que tous les composants fonctionnent indépendamment avant intégration.

---

## 💻 4. Programmation ESP32 (Arduino IDE)

### Étapes logiques :
1. **Initialisation des modules** : RFID, empreinte, LCD, Wi-Fi  
2. **Connexion Wi-Fi** : relier l’ESP32 au réseau local  
3. **Connexion au broker MQTT** (Mosquitto)  
4. **Boucle principale** :
   - Lire RFID / empreinte  
   - Vérifier l’identité  
   - Si autorisé → ouvrir servo + afficher message  
   - Publier les logs (`/access/log`)
   - Mettre à jour le LCD
5. **Souscriptions MQTT** :
   - Réception de commandes web (`/servo/control = open/close`)  

---

## 🌐 5. Interface web

### Outils :
- **HTML / CSS / JavaScript**
- **MQTT.js** (connexion MQTT over WebSocket)
- (Optionnel) **Chart.js** pour visualiser les logs

### Fonctionnalités :
- Connexion au broker MQTT  
- Affichage des logs d’accès en temps réel  
- Bouton **"Ouvrir la porte"** (envoi d’une commande MQTT)  
- Statut du système (connecté / déconnecté)  
- Liste des derniers accès (RFID / empreinte / heure)

---

## 🧾 6. Tests, validation et amélioration

### Tests à effectuer :
- Lecture correcte RFID / empreinte  
- Mouvement du servo moteur  
- Réception des messages MQTT (MQTTX + web)  
- Vérification de la latence réseau  
- Test de reconnexion Wi-Fi automatique  

### Améliorations possibles :
- Ajouter une base de données (Firebase ou SQLite)  
- Capteur d’ouverture réel (fin de course)  
- Notifications (Telegram, e-mail, etc.)  
- Sécurisation MQTT (authentification + SSL)

---

## 🧭 7. Résumé du plan de travail

| Étape | Description | Objectif |
|--------|--------------|-----------|
| 1️⃣ Concept | Définir le projet | Vision claire |
| 2️⃣ Matériel | Préparer le matériel | Être prêt à travailler |
| 3️⃣ Électronique | Câblage et tests | Vérification matérielle |
| 4️⃣ Programmation | Code ESP32 + MQTT | Fonctionnement du système |
| 5️⃣ Interface Web | Dashboard + contrôle | Supervision et commande |
| 6️⃣ Tests | Vérifier et corriger | Projet stable et fiable |

---

## ✅ Résultat attendu
Un **système de contrôle d’accès IoT complet**, capable de :
- Identifier un utilisateur par RFID ou empreinte  
- Commander un servo moteur pour ouvrir/fermer une porte  
- Envoyer et recevoir des informations via MQTT  
- Afficher localement les états sur un écran LCD  
- Fournir une **interface web** pour le suivi et le contrôle à distance.
