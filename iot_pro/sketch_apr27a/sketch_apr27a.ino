#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <PubSubClient.h>
#include <DHT.h>

// ── WiFi ────────────────────────────────────────────────────────
#define WIFI_SSID     "Rator"
#define WIFI_PASSWORD "3h_1s@5n&3m$10e%"

// ── Firebase ────────────────────────────────────────────────────
#define FIREBASE_HOST "https://shit-323b1-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "AIzaSyANL10a7fyC90k8xYu6SkR0oeeQQFR4j1E"

// ── MQTT ────────────────────────────────────────────────────────
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT   1883

// Topics
#define TOPIC_SENSORS    "iot/home/sensors"
#define TOPIC_WHITE_LED  "iot/home/control/white_led"
#define TOPIC_ALARM_LED  "iot/home/control/alarm_led"
#define TOPIC_DIST_BUZZ  "iot/home/control/dist_buzzer"

// ── Pins ────────────────────────────────────────────────────────
#define DHTPIN          D4
#define DHTTYPE         DHT11
#define LED_TEMP_BUZZER D1   // fire alarm
#define TRIG_PIN        D5
#define ECHO_PIN        D6
#define LED_PIR_BUZZER  D2   // proximity alarm
#define LDR_PIN         A0
#define LED_WHITE       D7

DHT          dht(DHTPIN, DHTTYPE);
FirebaseData firebaseData;
FirebaseConfig fbConfig;
FirebaseAuth   fbAuth;
WiFiClient     espClient;
PubSubClient   mqtt(espClient);

// ── State ───────────────────────────────────────────────────────
float cachedTemp     = 0;
int   cachedDist     = 0;
int   cachedLDR      = 0;

unsigned long lastPublish      = 0;
unsigned long lastBuzzerToggle = 0;
bool          buzzerOn         = false;
int           buzzerCount      = 0;   // counts half-cycles (8 = 4 on-off)

bool manualWhiteLed  = false;
bool whiteLedState   = false;
bool manualAlarmLed  = false;

// ── MQTT callback ───────────────────────────────────────────────
void onMqttMessage(char* topic, byte* payload, unsigned int len) {
  char msg[16] = {};
  for (unsigned int i = 0; i < len && i < 15; i++) msg[i] = (char)payload[i];

  if (strcmp(topic, TOPIC_WHITE_LED) == 0) {
    manualWhiteLed = true;
    whiteLedState  = (strcmp(msg, "ON") == 0);
    digitalWrite(LED_WHITE, whiteLedState ? HIGH : LOW);

  } else if (strcmp(topic, TOPIC_ALARM_LED) == 0) {
    manualAlarmLed = (strcmp(msg, "ON") == 0);
    if (!manualAlarmLed) {
      digitalWrite(LED_TEMP_BUZZER, LOW);
      buzzerOn = false; buzzerCount = 0;
    }

  } else if (strcmp(topic, TOPIC_DIST_BUZZ) == 0) {
    digitalWrite(LED_PIR_BUZZER, (strcmp(msg, "ON") == 0) ? HIGH : LOW);
  }
}

// ── MQTT reconnect ──────────────────────────────────────────────
void reconnectMqtt() {
  char clientId[32];
  snprintf(clientId, sizeof(clientId), "esp8266-%06X", ESP.getChipId());

  while (!mqtt.connected()) {
    Serial.print("MQTT connecting...");
    if (mqtt.connect(clientId)) {
      Serial.println(" OK");
      mqtt.subscribe(TOPIC_WHITE_LED);
      mqtt.subscribe(TOPIC_ALARM_LED);
      mqtt.subscribe(TOPIC_DIST_BUZZ);
    } else {
      Serial.print(" fail rc="); Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

// ── Setup ───────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(LED_TEMP_BUZZER, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIR_BUZZER, OUTPUT);
  pinMode(LED_WHITE, OUTPUT);

  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" connected");

  fbConfig.host = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&fbConfig, &fbAuth);

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
}

// ── Loop ────────────────────────────────────────────────────────
void loop() {
  // Keep MQTT alive
  if (!mqtt.connected()) reconnectMqtt();
  mqtt.loop();

  unsigned long now = millis();

  // ── Non-blocking temp buzzer (4 on-off cycles every sensor tick) ──
  if (!manualAlarmLed) {
    if (cachedTemp > 30) {
      if (now - lastBuzzerToggle >= 150 && buzzerCount < 8) {
        lastBuzzerToggle = now;
        buzzerOn = !buzzerOn;
        digitalWrite(LED_TEMP_BUZZER, buzzerOn ? HIGH : LOW);
        buzzerCount++;
      }
    } else {
      digitalWrite(LED_TEMP_BUZZER, LOW);
      buzzerOn = false; buzzerCount = 0;
    }
  }

  // ── Sensor read + publish every 4 s ──────────────────────────
  if (now - lastPublish < 4000) return;
  lastPublish  = now;
  buzzerCount  = 0;   // reset buzzer cycle for next period

  // Ultrasonic
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  cachedDist = (int)(pulseIn(ECHO_PIN, HIGH) * 0.034 / 2);


  // Temperature
  float t = dht.readTemperature();
  if (!isnan(t)) cachedTemp = t;

  // LDR
  cachedLDR = analogRead(LDR_PIN);

  // Auto white LED
  if (!manualWhiteLed)
    digitalWrite(LED_WHITE, cachedLDR > 500 ? HIGH : LOW);

  // ── Firebase ──────────────────────────────────────────────────
  Firebase.pushFloat(firebaseData, "/Home/Temperature", cachedTemp);
  Firebase.pushInt(firebaseData,   "/Home/Distance",    cachedDist);
  Firebase.pushInt(firebaseData,   "/Home/LDR",         cachedLDR);

  // ── MQTT publish ──────────────────────────────────────────────
  char payload[128];
  snprintf(payload, sizeof(payload),
    "{\"temp\":%.1f,\"distance\":%d,\"ldr\":%d,\"white_led\":%d,\"alarm_led\":%d,\"dist_buzzer\":%d}",
    cachedTemp, cachedDist, cachedLDR,
    digitalRead(LED_WHITE), digitalRead(LED_TEMP_BUZZER), digitalRead(LED_PIR_BUZZER));
  mqtt.publish(TOPIC_SENSORS, payload);

  Serial.println(payload);
}
