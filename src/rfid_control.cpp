#include "rfid_control.h"
#include "config.h"
#include "state.h"
#include "screen.h"
#include "api_handlers.h"
#include "motor_control.h"
#include "notif_screen.h"

#if RFID_ENABLED
  #include <SPI.h>
  #include <MFRC522.h>
  #include <HTTPClient.h>
  
  MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
  unsigned long lastRfidPoll = 0;
#endif

void initRFID() {
#if RFID_ENABLED
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID: Initialized on VSPI. Ready to scan!");
#else
  Serial.println("RFID: Disabled in config.");
#endif
}

static void performRfidAction(const RfidCard& card) {
  Serial.printf("RFID: Executing action '%s' with param '%s'\n", 
                card.action.c_str(), card.param.c_str());

  if (card.action == "screen") {
    if (card.param == "timer") setScreen(SCREEN_TIMER);
    else if (card.param == "reminder") setScreen(SCREEN_REMINDER);
    else if (card.param == "calendar") setScreen(SCREEN_CALENDAR);
    else if (card.param == "todo") setScreen(SCREEN_TODO);
    else if (card.param == "notifs") setScreen(SCREEN_NOTIFS);
  } else if (card.action == "notify") {
    addNotification("RFID", "System", card.param, TFT_BLUE);
  } else if (card.action == "motor") {
    int speed = card.param.toInt();
    setMotorRaw(speed);
  } else if (card.action == "clear") {
    clearAllNotifications();
    setScreen(getDefaultScreen());
  } else if (card.action == "play_playlist") {
#if RFID_ENABLED
    HTTPClient http;
    String url = "http://" + String(MUSIC_API_HOST) + ":" + String(MUSIC_API_PORT) + "/play";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"playlist\":\"" + card.param + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("RFID: Music API triggered %s, response code: %d\n", url.c_str(), httpCode);
    http.end();
#endif
  } else if (card.action == "play_album") {
#if RFID_ENABLED
    HTTPClient http;
    String url = "http://" + String(MUSIC_API_HOST) + ":" + String(MUSIC_API_PORT) + "/play";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"album\":\"" + card.param + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("RFID: Music API triggered %s, response code: %d\n", url.c_str(), httpCode);
    http.end();
#endif
  } else if (card.action == "play_artist") {
#if RFID_ENABLED
    HTTPClient http;
    String url = "http://" + String(MUSIC_API_HOST) + ":" + String(MUSIC_API_PORT) + "/play";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"artist\":\"" + card.param + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("RFID: Music API triggered %s, response code: %d\n", url.c_str(), httpCode);
    http.end();
#endif
  } else if (card.action == "play_title") {
#if RFID_ENABLED
    HTTPClient http;
    String url = "http://" + String(MUSIC_API_HOST) + ":" + String(MUSIC_API_PORT) + "/play";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"title\":\"" + card.param + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("RFID: Music API triggered %s, response code: %d\n", url.c_str(), httpCode);
    http.end();
#endif
  }
}

#if RFID_ENABLED
// Helper function to read NDEF text from a MIFARE Classic 1K card (4-byte UID)
String readMifareNdefText() {
  String text = "";
  byte keys[][6] = {
    {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}, // NDEF MAD key (NFC Forum Application)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Factory Default
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}, // NXP Default MAD key
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // Empty/Blank key
  };

  MFRC522::MIFARE_Key key;
  bool authenticated = false;
  byte blockAddr = -1;
  
  // NDEF messages on MIFARE Classic usually start at Sector 1 (Block 4)
  // Sometimes they start at Sector 2 (Block 8) if Sector 1 is a large MAD
  byte possibleStartBlocks[] = {4, 8}; 
  
  for (byte startBlock : possibleStartBlocks) {
    if (authenticated) break;
    
    for (int k = 0; k < 4; k++) {
      for (int i = 0; i < 6; i++) key.keyByte[i] = keys[k][i];
      
      MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, startBlock, &key, &(rfid.uid));
      if (status == MFRC522::STATUS_OK) {
        authenticated = true;
        blockAddr = startBlock;
        Serial.printf("RFID: Authenticated block %d with key index %d!\n", startBlock, k);
        break;
      }
    }
  }

  if (!authenticated) {
    Serial.println("RFID: Failed to authenticate Mifare Classic blocks 4 and 8. Cannot read NDEF.");
    return "";
  }

  // Read the 3 data blocks of the authenticated sector
  byte buffer[54]; // 3 blocks * 16 bytes = 48 bytes + padding
  byte size = 18;
  int bufIdx = 0;
  
  for (byte b = blockAddr; b < blockAddr + 3; b++) {
    if (b % 4 == 3) continue; // Skip sector trailer blocks (e.g. 7, 11)
    
    size = 18;
    if (rfid.MIFARE_Read(b, &buffer[bufIdx], &size) == MFRC522::STATUS_OK) {
      bufIdx += 16;
    } else {
      Serial.printf("RFID: Failed to read block %d\n", b);
      break;
    }
  }
  
  if (bufIdx == 0) return "";
  
  // Parse TLV blocks to properly find NDEF Message (0x03)
  int ndefStart = -1;
  int i = 0;
  while (i < bufIdx - 1) {
    if (buffer[i] == 0x00) { // NULL TLV
      i++;
    } else if (buffer[i] == 0x03) { // NDEF Message TLV found
      ndefStart = i;
      break;
    } else if (buffer[i] == 0xFE) { // Terminator
      break;
    } else {
      // Unknown TLV length skip
      int tlvLen = buffer[i + 1];
      if (tlvLen == 0xFF) {
        tlvLen = (buffer[i+2] << 8) | buffer[i+3];
        i += 4 + tlvLen;
      } else {
        i += 2 + tlvLen;
      }
    }
  }
  
  if (ndefStart == -1) {
    Serial.println("RFID: No NDEF TLV (0x03) found in Mifare data.");
    return "";
  }
  
  int recordStart = ndefStart + 2;
  
  // NDEF Record Header Debug
  byte header = buffer[recordStart];
  byte typeLen = buffer[recordStart + 1];
  byte payloadLen = buffer[recordStart + 2];
  Serial.printf("RFID: NDEF Header=%02X, TypeLen=%d, PayloadLen=%d\n", header, typeLen, payloadLen);
  
  if (recordStart + 3 < bufIdx && buffer[recordStart + 3] == 0x54) { // 'T' check
    int langLen = buffer[recordStart + 4] & 0x3F;
    int textStart = recordStart + 5 + langLen;
    int textLength = payloadLen - langLen - 1;
    
    if (textStart + textLength <= bufIdx) {
      for (int i = 0; i < textLength; i++) text += (char)buffer[textStart + i];
      Serial.printf("RFID: Extracted Mifare NDEF Text '%s'\n", text.c_str());
    } else {
      Serial.println("RFID: NDEF payload extends beyond read buffer.");
    }
  } else {
    Serial.printf("RFID: Mifare NDEF record is not Text. Type: 0x%02X\n", buffer[recordStart+3]);
  }

  return text;
}

// Helper function to read NDEF text from NTAG/MIFARE Ultralight cards (7-byte UID)
String readNtagNdefText() {
  String text = "";
  byte fullBuffer[64]; // Read up to 16 pages (64 bytes)
  int bufIdx = 0;
  
  // NTAG NDEF data usually starts at page 4. Each read command reads 4 pages (16 bytes).
  // We'll read starting at page 4, then page 8, then page 12
  for (byte page = 4; page <= 12; page += 4) {
    byte size = 18;
    MFRC522::StatusCode status = rfid.MIFARE_Read(page, &fullBuffer[bufIdx], &size);
    if (status == MFRC522::STATUS_OK) {
      bufIdx += 16;
    } else {
      Serial.println("RFID: NTAG read failed");
      break;
    }
  }
  
  if (bufIdx == 0) return ""; // Read completely failed

  // Parse TLV blocks to properly find NDEF Message (0x03)
  int ndefStart = -1;
  int i = 0;
  while (i < bufIdx - 1) {
    if (fullBuffer[i] == 0x00) { // NULL TLV, skip 1 byte
      i++;
    } else if (fullBuffer[i] == 0x03) { // NDEF Message TLV found
      ndefStart = i;
      break;
    } else if (fullBuffer[i] == 0xFE) { // Terminator TLV, end of data
      break;
    } else {
      // Unknown TLV (e.g. Lock Control 0x01/0x02) - skip its length block
      int tlvLen = fullBuffer[i + 1];
      if (tlvLen == 0xFF) {
        // 3-byte length format
        tlvLen = (fullBuffer[i+2] << 8) | fullBuffer[i+3];
        i += 4 + tlvLen;
      } else {
        i += 2 + tlvLen;
      }
    }
  }
  
  if (ndefStart == -1) {
    Serial.println("RFID: No NDEF TLV (0x03) found in NTAG data.");
    return "";
  }
  
  int ndefLength = fullBuffer[ndefStart + 1];
  if (ndefLength <= 0 || ndefLength == 0xFF) {
    Serial.println("RFID: NDEF length invalid or too long for parser.");
    return "";
  }
  
  int recordStart = ndefStart + 2;
  
  // NDEF Record Header:
  // [0] Header (e.g. 0xD1 = MB=1, ME=1, SR=1, IL=0, TNF=1)
  // [1] Type Length
  // [2] Payload Length
  // [3...] Type
  // [...] Payload
  
  byte header = fullBuffer[recordStart];
  byte typeLen = fullBuffer[recordStart + 1];
  byte payloadLen = fullBuffer[recordStart + 2];
  
  Serial.printf("RFID: NDEF Header=%02X, TypeLen=%d, PayloadLen=%d\n", header, typeLen, payloadLen);
  
  if (recordStart + 3 < bufIdx && fullBuffer[recordStart + 3] == 0x54) { // 'T' = 0x54
    int langLen = fullBuffer[recordStart + 4] & 0x3F;
    
    int textStart = recordStart + 5 + langLen;
    int textLength = payloadLen - langLen - 1;
    
    if (textStart + textLength <= bufIdx) {
      for (int i = 0; i < textLength; i++) {
        text += (char)fullBuffer[textStart + i];
      }
      Serial.printf("RFID: Extracted NTAG NDEF Text '%s'\n", text.c_str());
    } else {
      Serial.println("RFID: NDEF payload extends beyond read buffer.");
    }
  } else {
    Serial.printf("RFID: NTAG NDEF record is not Text. Type: 0x%02X\n", fullBuffer[recordStart+3]);
  }

  return text;
}
#endif

void checkRFID() {
#if RFID_ENABLED
  if (millis() - lastRfidPoll < RFID_POLL_INTERVAL) return;
  lastRfidPoll = millis();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("\n=============================");
  Serial.println("RFID: Tag detected!");
  
  // 1. Extract raw UID string
  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();
  lastScannedUid = uidStr;
  Serial.printf("UID: %s (%d bytes)\n", uidStr.c_str(), rfid.uid.size);
  
  // 2. Try to read NDEF text payload based on tag type
  String payloadText = "";
  if (rfid.uid.size == 7) {
    // 7-byte UID defines NTAG/Ultralight series (no crypto auth)
    payloadText = readNtagNdefText();
  } else if (rfid.uid.size == 4) {
    // 4-byte UID defines Classic 1K (crypto auth required)
    payloadText = readMifareNdefText();
  }
  
  // Look up card in registry
  bool found = false;
  
  // 3. Auto-route specific music prefixes before checking registry
  if (payloadText.length() > 0) {
    if (payloadText.startsWith("playlist:")) {
      String param = payloadText.substring(9);
      Serial.printf("RFID: Auto-routing to playlist '%s'\n", param.c_str());
      RfidCard virtualCard;
      virtualCard.uid = "virtual";
      virtualCard.action = "play_playlist";
      virtualCard.param = param;
      performRfidAction(virtualCard);
      found = true;
    } else if (payloadText.startsWith("album:")) {
      String param = payloadText.substring(6);
      Serial.printf("RFID: Auto-routing to album '%s'\n", param.c_str());
      RfidCard virtualCard;
      virtualCard.uid = "virtual";
      virtualCard.action = "play_album";
      virtualCard.param = param;
      performRfidAction(virtualCard);
      found = true;
    } else if (payloadText.startsWith("artist:")) {
      String param = payloadText.substring(7);
      Serial.printf("RFID: Auto-routing to artist '%s'\n", param.c_str());
      RfidCard virtualCard;
      virtualCard.uid = "virtual";
      virtualCard.action = "play_artist";
      virtualCard.param = param;
      performRfidAction(virtualCard);
      found = true;
    } else if (payloadText.startsWith("track:")) {
      String param = payloadText.substring(6);
      Serial.printf("RFID: Auto-routing to track title '%s'\n", param.c_str());
      RfidCard virtualCard;
      virtualCard.uid = "virtual";
      virtualCard.action = "play_title";
      virtualCard.param = param;
      performRfidAction(virtualCard);
      found = true;
    }
  }

  // 4. Check against NDEF Text first
  if (!found && payloadText.length() > 0) {
    for (int i = 0; i < registeredCardCount; i++) {
      if (registeredCards[i].uid == payloadText) {
        found = true;
        Serial.printf("Matched by NDEF text: %s\n", payloadText.c_str());
        performRfidAction(registeredCards[i]);
        break;
      }
    }
  }
  
  // 5. Fallback to UID match if text didn't match (or didn't exist)
  if (!found) {
    for (int i = 0; i < registeredCardCount; i++) {
      if (registeredCards[i].uid == uidStr) {
        found = true;
        Serial.printf("Matched by raw UID: %s\n", uidStr.c_str());
        performRfidAction(registeredCards[i]);
        break;
      }
    }
  }

  if (!found) {
    Serial.println("RFID: Tag data/UID not registered. Ignoring.");
  }
  
  Serial.println("=============================");
  
  // Halt PICC and stop crypto to prevent continuous reads
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  delay(1000); // Debounce
#endif
}

// ----------------- Registration API Methods -----------------

bool registerRfidCard(String uid, String action, String param) {
  // We no longer blindly toUpperCase() because the identifier might be case-sensitive NDEF text!
  
  // Check if UID/text already registered (update it)
  for (int i = 0; i < registeredCardCount; i++) {
    if (registeredCards[i].uid == uid) {
      registeredCards[i].action = action;
      registeredCards[i].param = param;
      Serial.printf("RFID: Updated card identifier '%s' -> %s(%s)\n", uid.c_str(), action.c_str(), param.c_str());
      return true;
    }
  }

  // Add new card if there's space
  if (registeredCardCount < RFID_MAX_CARDS) {
    registeredCards[registeredCardCount].uid = uid;
    registeredCards[registeredCardCount].action = action;
    registeredCards[registeredCardCount].param = param;
    registeredCardCount++;
    Serial.printf("RFID: Registered new identifier '%s' -> %s(%s)\n", uid.c_str(), action.c_str(), param.c_str());
    return true;
  }

  Serial.println("RFID: Registry full!");
  return false;
}

bool unregisterRfidCard(String uid) {
  for (int i = 0; i < registeredCardCount; i++) {
    if (registeredCards[i].uid == uid) {
      // Shift remaining items left
      for (int j = i; j < registeredCardCount - 1; j++) {
        registeredCards[j] = registeredCards[j + 1];
      }
      registeredCardCount--;
      registeredCards[registeredCardCount] = RfidCard(); // clear last slot
      Serial.printf("RFID: Unregistered identifier '%s'\n", uid.c_str());
      return true;
    }
  }
  return false;
}

String getRegisteredCardsJson() {
  String json = "[";
  for (int i = 0; i < registeredCardCount; i++) {
    if (i > 0) json += ",";
    json += "{\"uid\":\"" + registeredCards[i].uid + "\",";
    json += "\"action\":\"" + registeredCards[i].action + "\",";
    String escapedParam = registeredCards[i].param;
    escapedParam.replace("\"", "\\\"");
    json += "\"param\":\"" + escapedParam + "\"}";
  }
  json += "]";
  return json;
}

