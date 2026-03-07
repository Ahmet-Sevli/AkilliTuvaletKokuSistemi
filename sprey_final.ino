#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h> 
#include <Servo.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// 1. Wi-Fi Bilgileri
#define WIFI_SSID "Kacireis"
#define WIFI_PASSWORD "kacireis24"

// 2. Firebase Bilgileri
#define API_KEY "AIzaSyBEC4tQpsNDLWeb8d5scii_vj4DiFzuk24"
#define DATABASE_URL "iotproject-3c6f6-default-rtdb.firebaseio.com"

// 3. YOL AYARLARI
#define FIREBASE_PATH "/akilli_tuvalet/izleme_verisi/giris_kayitlari" 
#define SPRAY_COUNTER_PATH "/akilli_tuvalet/izleme_verisi/sprey/sprey_sayaci"

#define SERVO_PIN 5 // D1 Pini

struct {
  int eskiKayitSayisi;
  uint32_t dogrulamaKodu; 
} rtcVerisi;

#define MAGIC_NUMBER 0xAA55AA55 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Servo kokuServo;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n[Sistem] Uyanildi... (3 Kisi Modu)");

  // RTC Okuma
  ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcVerisi, sizeof(rtcVerisi));
  if (rtcVerisi.dogrulamaKodu != MAGIC_NUMBER) {
    rtcVerisi.eskiKayitSayisi = 0;
    rtcVerisi.dogrulamaKodu = MAGIC_NUMBER;
    ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcVerisi, sizeof(rtcVerisi));
  } else {
    Serial.print("[Hafiza] Son kayit: "); Serial.println(rtcVerisi.eskiKayitSayisi);
  }

  // Servo Kurulumu
  kokuServo.attach(SERVO_PIN);
  kokuServo.write(0);
  delay(200);
  kokuServo.detach();

  // Temiz Başlangıç
  WiFi.disconnect();   
  delay(100);
  WiFi.mode(WIFI_STA); 
  WiFi.persistent(false); 
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Baglaniyor");
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 60) { 
    Serial.print(".");
    delay(500);
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Baglandi.");
    
    Serial.println("Baglanti oturuyor, 3 sn bekle...");
    delay(5000); 

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.signer.test_mode = true;
    
    // Hata Toleransları
    config.timeout.wifiReconnect = 10 * 1000;
    config.timeout.sslHandshake = 20 * 1000; 
    config.timeout.serverResponse = 10 * 1000;
    
    // Tampon bellek
    fbdo.setBSSLBufferSize(4096, 512); 
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    delay(1000); 

    Serial.println("[Firebase] Veri çekiliyor...");

    bool basariliOldu = false;
    int denemeSayisi = 0;

    // Retry Mekanizması
    while (!basariliOldu && denemeSayisi < 3) {
        if(denemeSayisi > 0) {
          Serial.println("Tekrar deneniyor...");
          delay(2000); 
        }

        // Sadece ID'leri çek (Shallow)
        if (Firebase.RTDB.getShallowData(&fbdo, FIREBASE_PATH)) {
            basariliOldu = true;
            
            FirebaseJson &json = fbdo.jsonObject(); 
            size_t elemanSayisi = json.iteratorBegin();
            int gercekIDSayisi = 0;
            String key; String value; int type;
            
            for (size_t i = 0; i < elemanSayisi; i++) {
                json.iteratorGet(i, type, key, value);
                if (key.startsWith("-")) { 
                   gercekIDSayisi++;
                }
            }
            json.iteratorEnd();
            fbdo.clear(); 

            Serial.print("[Sonuc] Kisi Sayisi: "); Serial.println(gercekIDSayisi);

            int fark = gercekIDSayisi - rtcVerisi.eskiKayitSayisi;
            if (fark < 0) { fark = 0; rtcVerisi.eskiKayitSayisi = gercekIDSayisi; }

            Serial.print("[Fark] "); Serial.println(fark);

            // --- DEĞİŞİKLİK BURADA: EŞİK DEĞERİ 3 ---
            if (fark >= 3) {  
                Serial.println("[Aksiyon] 3 KISI SINIRI ASILDI -> 3 FIS SIKILIYOR...");
                
                // 1. Sprey Sıkma İşlemi
                kokuServo.attach(SERVO_PIN);
                for(int i=1; i<=3; i++){
                    kokuServo.write(180); delay(800);
                    kokuServo.write(0);   delay(1000);
                }
                kokuServo.detach();
                Serial.println("[Aksiyon] Fiziksel sikma tamamlandi.");

                // 2. Firebase Sprey Sayacını Güncelleme
                Serial.println("[Firebase] Sprey sayaci guncelleniyor...");
                int mevcutSpreySayisi = 0;
                
                // Önce mevcut sayıyı oku
                if(Firebase.RTDB.getInt(&fbdo, SPRAY_COUNTER_PATH)){
                   mevcutSpreySayisi = fbdo.intData();
                }
                
                // Üzerine 3 ekle ve gönder
                int yeniSpreySayisi = mevcutSpreySayisi + 3;
                if(Firebase.RTDB.setInt(&fbdo, SPRAY_COUNTER_PATH, yeniSpreySayisi)){
                    Serial.print("[Basarili] Yeni Toplam Sprey: ");
                    Serial.println(yeniSpreySayisi);
                } else {
                    // Hata buradaydı, tırnak işareti kapatıldı
                    Serial.println("[Hata] Sayac guncellenemedi: " + fbdo.errorReason());
                }

                // 3. Hafızayı Güncelle
                rtcVerisi.eskiKayitSayisi = gercekIDSayisi;
                ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcVerisi, sizeof(rtcVerisi));
                Serial.println("[Hafiza] Kisi sayisi guncellendi.");
                
            } else {
                Serial.println("[Durum] Henuz 3 yeni kisi birikmedi.");
            }

        } else {
            Serial.print("Hata: "); Serial.println(fbdo.errorReason());
            denemeSayisi++;
        }
    }
  } else {
    Serial.println("\n[HATA] WiFi Yok.");
  }

  Serial.println("Uyku...");
  ESP.deepSleep(60e6); 
}

void loop() {}