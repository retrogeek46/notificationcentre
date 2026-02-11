# Tasker Focus Mode Setup

ESP32 sends `POST /focus` with `focus=on` or `focus=off` to your phone.

## Prerequisites
- Tasker app installed
- Tasker accessibility service enabled (Settings → Accessibility → Tasker)

## 1. Start HTTP Server

**Profile**: Event → `Net → HTTP Request`
- **Port**: `8765`
- **Path**: `/focus`

## 2. Focus ON Task

**Condition**: `%http_request_body ~ *focus=on*`

**Actions**:
1. Variable Set → `%FocusMode` = `1`
2. Profile Status → Enable profile "App Blocker"
3. Flash → "🔒 Focus Mode ON"

## 3. Focus OFF Task

**Condition**: `%http_request_body ~ *focus=off*`

**Actions**:
1. Variable Set → `%FocusMode` = `0`
2. Profile Status → Disable profile "App Blocker"
3. Flash → "✅ Focus Mode OFF"

## 4. App Blocker Profile

**Profile**: Event → `App → App Changed`
- **Apps**: YouTube, Discord, Instagram, Telegram, (add your games)

**Condition**: `%FocusMode = 1`

**Task**:
1. Flash → "⚠️ Focus Mode! Get back to work!"
2. Wait → 500ms
3. Go Home → Page 0

## 5. HTTP Response Task (linked to HTTP Request profile)

**Actions**:
1. HTTP Response → Code `200`, Body `{"ok":true}`

## Testing

From PC, test with curl:
```bash
# Focus ON
curl -X POST http://<phone-ip>:8765/focus -d "focus=on"

# Focus OFF
curl -X POST http://<phone-ip>:8765/focus -d "focus=off"
```

## Notes
- Set a **static IP** on your phone (Settings → Wi-Fi → your network → Static)
- Update `FOCUS_PHONE_IP` in ESP32 `config.h` to match
- You mentioned you'll share a sample task later — replace the App Blocker task with your preferred implementation
