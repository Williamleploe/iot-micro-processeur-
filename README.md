# 🔧 Partie IoT – ESP32 (RFID + Empreinte + Servo + LCD + MQTT)

Cette partie correspond au **cœur du système de contrôle d’accès** : l’ESP32 lit un badge RFID ou une empreinte digitale, décide si l’accès est autorisé, ouvre la porte via un servo, affiche les informations sur un LCD et communique tous les événements via **MQTT**.

Le code complet gère :
- RFID RC522  
- Capteur d’empreintes Adafruit / R503  
- Servo moteur  
- Écran LCD I2C  
- Gestion locale des utilisateurs (EEPROM / Preferences)  
- Wi-Fi  
- MQTT (PubSubClient)  
- Protocoles d’enrôlement (RFID + empreintes)

---

# 🧩 Fonctionnalités principales

### 🔐 Contrôle d’accès
- Détection RFID  
- Détection empreinte  
- Recherche d’utilisateur en mémoire interne  
- Accès **granted / denied**

### 🕹️ Action physique
- Servo motorisé → ouvre la porte pour 800 ms  
- Retour automatique à la position fermée  

### 📟 Affichage local (LCD 16x2)
- Messages d’accès  
- Instructions d’enrôlement  
- État du système  

### 💾 Mémoire interne (Preferences)
- Stockage persistant des utilisateurs  
- Support RFID + empreintes  
- Auto-incrément `next_fp_id`  
- Listing, suppression complète, renommage

### 🌐 Communication MQTT
- Publication des événements  
- Réception de commandes  
- Auto-reconnexion Wi-Fi + MQTT

---

# 🏗️ Architecture IoT

```
[ESP32] ⇄ WiFi ⇄ [Broker MQTT] ⇄ Web Dashboard
```

---

# 📨 Topics MQTT utilisés

| Type | Topic | Sens | Description |
|------|--------|------|-------------|
| **Événements** | `auth/door/event` | ESP32 → Web | Résultat d’accès + logs + enrôlements |
| **Commandes** | `auth/door/command` | Web → ESP32 | OPEN / LIST / CLEAR |
| **Status** | `auth/door/status` | ESP32 → Web | État du device |

### Exemple d’événement envoyé :
```json
{
  "result": "granted",
  "method": "rfid",
  "key": "A1B2C3D4",
  "name": "Lucas",
  "ts": 1034213
}
```

---

# 🛠️ Composants matériels utilisés

| Module | Rôle |
|--------|------|
| ESP32 | Microcontrôleur principal |
| RC522 | Lecture badges RFID |
| Capteur empreinte (R503 / FPM383C) | Identification biométrique |
| Servo motorisé | Action ouverture porte |
| LCD 16x2 I2C | Affichage |
| Preferences | Base de données interne légère |

---

# 🔌 Connexions (pins ESP32)

| Module | Broches |
|--------|---------|
| RFID RC522 | SS=5, RST=4, SCK=18, MISO=19, MOSI=23 |
| Fingerprint | RX=16, TX=17 |
| LCD | SDA=21, SCL=22 |
| Servo | GPIO 14 |

---

# 🔐 Gestion des utilisateurs

Commandes série disponibles :
```
r      → enrôlement RFID
f      → enrôlement empreinte
list   → afficher tous les utilisateurs
clear  → effacer base interne
delmod → effacer base du capteur empreinte
help   → afficher aide
```

---

