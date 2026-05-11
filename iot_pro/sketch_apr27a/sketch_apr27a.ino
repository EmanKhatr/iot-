#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>

// إعدادات الواي فاي والفايربيز
#define WIFI_SSID "Rator"
#define WIFI_PASSWORD "3h_1s@5n&3m$10e%"
#define FIREBASE_HOST "https://shit-323b1-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "AIzaSyANL10a7fyC90k8xYu6SkR0oeeQQFR4j1E"

// تعريف البينات حسب الجدول
#define DHTPIN D4
#define DHTTYPE DHT11
#define LED_TEMP_BUZZER D1 // D15 (GPIO15)
#define TRIG_PIN D5        // GPIO14
#define ECHO_PIN D6        // GPIO12
#define LED_PIR_BUZZER D2  // GPIO4
#define LDR_PIN A0
#define LED_WHITE D7       // GPIO13

DHT dht(DHTPIN, DHTTYPE);
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_TEMP_BUZZER, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIR_BUZZER, OUTPUT);
  pinMode(LED_WHITE, OUTPUT);
  
  dht.begin();
  
  // الاتصال بالواي فاي
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
}

void loop() {
  // 1. قراءة المسافة (Ultrasonic)
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;

  // التحكم في إنذار الحركة/المسافة
  if (distance < 10 && distance > 0) { // لو المسافة أقل من 10 سم
    digitalWrite(LED_PIR_BUZZER, HIGH);
  } else {
    digitalWrite(LED_PIR_BUZZER, LOW);
  }

  // // 2. قراءة الحرارة
  // float temp = dht.readTemperature();
  // if (temp > 30) { // لو الحرارة أعلى من 30 (احتمال حريق)
  //   digitalWrite(LED_TEMP_BUZZER, HIGH);
  // } else {
  //   digitalWrite(LED_TEMP_BUZZER, LOW);
  // }


//   float temp = dht.readTemperature();
// if (temp > 30) {
//     tone(LED_TEMP_BUZZER, 1000); // تردد عالي (وي)
//     delay(100);
//     tone(LED_TEMP_BUZZER, 100);  // تردد واطي (وا)
//     delay(200);
//   } else {
//     noTone(LED_TEMP_BUZZER);     // يوقف الصوت تماماً
//     digitalWrite(LED_TEMP_BUZZER, LOW);
//   }


// 2. التحكم في إنذار الحرارة (تأثير السرينة)
 float temp = dht.readTemperature();
  if (temp > 30) { 
    // صوت عالي وسريع (وي وا وي وا)
    digitalWrite(LED_TEMP_BUZZER, HIGH); // تشغيل الليد والبازر
    delay(150);                          // مدة الصفرة (سريعة)
    digitalWrite(LED_TEMP_BUZZER, LOW);  // إطفاء
    delay(150);                          // مدة الفصل
    
    // ممكن تكرريها مرة كمان عشان تضمنين إن اللوب سريعة
    digitalWrite(LED_TEMP_BUZZER, HIGH);
    delay(150);
    digitalWrite(LED_TEMP_BUZZER, LOW);
    delay(150);
  } else {
    digitalWrite(LED_TEMP_BUZZER, LOW); // طفي كل حاجة لو الحرارة طبيعية
  }

  // 3. قراءة الضوء (LDR)
  int ldrValue = analogRead(LDR_PIN);
  if (ldrValue > 500) { // قيمة الضوء منخفضة (ظلام)
    digitalWrite(LED_WHITE, HIGH);
  } else {
    digitalWrite(LED_WHITE, LOW);
  }

  // إرسال البيانات للفايربيز
  Firebase.pushFloat(firebaseData, "/Home/Temperature", temp);
  Firebase.pushInt(firebaseData, "/Home/Distance", distance);
  Firebase.pushInt(firebaseData, "/Home/LDR", ldrValue);


  delay(4000); // تحديث كل ثانية
}