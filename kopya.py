from ultralytics import YOLO
import cv2 as cv
import firebase_admin
from firebase_admin import credentials, db
import os
from datetime import datetime 
import threading

# dosya yolları her pc de çalışşsın diye
current_dir = os.path.dirname(os.path.abspath(__file__))
firebase_json = os.path.join(current_dir, "iot.json")




# Firebase başlatmak veritabanının  için json dosyası bi de linki lazım 

cred = credentials.Certificate(firebase_json)  
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://iotproject-3c6f6-default-rtdb.firebaseio.com/'
})


def threadle_guncelle(giris_cikis):
    # """Firebase işlemini arka planda yapar, ana ekranı dondurmaz."""
    threading.Thread(target=firebase_guncelle, args=(giris_cikis,), daemon=True).start()


def firebase_guncelle(giris_cikis):

    # bu fonksiyonda lavaboya giren kişi sayısını firabse veri tabanına göndericem
    # ilk önce güncelleyeceğim dizine girmem lazım veri tabanında
    ref = db.reference(f"/akilli_tuvalet/izleme_verisi")
    
    # kişi sayısını firebase den çekmeme gerek yok 
    
    # Firebase de kişi sayısını  güncellemem lazım 
    # rümeysa veritabanı yapısını atınca diğer güncellemem gerkeen şeyleri yaparım
    # 1. Toplam kişi sayısını güncelle
    # Sadece kisi_sayisi'ni güncellemek için update kullanıyoruz, 
    # böylece 'sprey' gibi diğer veriler silinmez.
    mevcut_kisi=ref.child("kisi_sayisi").get() #mvcut kişilere eklemek veya çıkarmak için önceki değerli alıp toplamamk lazım
         
    print("Mevcut kişi sayısı:", mevcut_kisi)
        

    if giris_cikis==1: # eğer değer 1 ise bu giriş demek
        mevcut_kisi+=1
          # 2. Giriş kaydını tarih ve saat ile ekle çünjkü burası giriş
          # 'giris_kayitlari' altına yeni bir kayıt (push) oluşturur
        simdi = datetime.now().strftime("%d.%m.%Y %H:%M:%S")
        ref.child("giris_kayitlari").push(simdi)
    else:
         mevcut_kisi-=1
    

    # en son da kişi sayısını güncelliyoruz
    ref.update({
        "kisi_sayisi": mevcut_kisi
    })

   





# YOLOv11n 
model = YOLO("yolo11n.pt")  # COCO dataset, class 0 = insan

# Çizgi koordinatı (x ekseni) lavabodan girmelerini tespit için
line_x = 250

tolerans=15 # şuan özel bi veri seti kullanmadığım için takip ederken frameler arasında insanları kaçırıyor bunu tolare edebilmek için gerekli 

girecekler=set() # içeri girme ihtimali olan kişilerin id leri burda tutulacak set de kullandım ki frameler arasında aynı id yi birden fazla eklemesin

son_yer={} # kişilerin son yerleri tutulacak id ye göre  id si ve x kordinatı anahtar değer çifti olarak

# tuvalettteki mevcut kişiyi görmek de istendi o yüzden dışarı çıkanları da saymamız lazım
cikacaklar=set()


kisi_sayisi = 0 # giren kişileri tutmak için

url = 'http://10.22.47.34:8080/video'
# Video capture
cap = cv.VideoCapture(url)

genislik = 640
yukseklik = 480  # daha hızlı işleme yapmak için boyutu sınırlandırdım

fps=0 # sayım yapmak için bütün framelere gerek yok arada frameleri atlamak için içeride çalışacak her framei alsak performans düşebilir

while True:
    ret, frame_ = cap.read()
    if not ret:
        break
    
    if fps%3==0:
        fps+=1
        continue  # her 3 frameden 2 sini alıyor hız için

    frame= cv.resize(frame_, (genislik, yukseklik))
   
    # YOLO botsort ile takip kısmı->bytetrack daha hızlı ama bot sort deep sort gibi daha güvenli burası değişebilir performansa göre
    results = model.track(source=frame, persist=True, classes=[0], conf=0.6, tracker="botsort.yaml")[0]

    frame=results.plot() # tespitleri çizmek için kontol amaçlı
    
    for r in results.boxes:
        if r.id is None:
            continue  # buradaki kod atlanır yoksa eğer 
        
        x1, y1, x2, y2 = map(int, r.xyxy[0])
        cx = int((x1 + x2) / 2)
        cy = int((y1 + y2) / 2)
        tid = int(r.id)  # Tracker ID
        #cv.circle(frame, (cx, cy), 4, (0, 0, 255), -1) # merkezlerine göre giriş çıkışları gözle görebilmek için tespit kutularıın ortasına küçük bir nokta
        son_yer[tid]=cx  # kişinin son yerini güncellemek için
      
        
        # lavabodan içeri girmemiş kişileri  listeye eklemek lazım 

        if line_x > cx :
           girecekler.add(tid)

        # çıkanlları da saymamız istendi
        if line_x<cx:
            cikacaklar.add(tid)


        # şimdi giren kişilerin sınırı geçip geömediğini yani gerçekten içeri girip girmediğini belirliycez
        # eğer girme potansiyeli olan kişilerin(girecekler listesindeki kişiler) merkezi çizgiyi geçerse kişi sayısı bir artacak ve o id listeden silinecek çünkü girdi artık lavaboya
       
        for girecek_mi in list(girecekler): 
            if son_yer[girecek_mi]> (line_x+tolerans):
                kisi_sayisi+=1
                girecekler.remove(girecek_mi)  # çizgiyi geçtikten sonra artık o kişi ile işimiz kalmadı listeden çıkarıyorum 
                threadle_guncelle(1) # gönderirken ekranı kastırıyordu o yüzden theradle gönderdim 
                #firebase_guncelle(kisi_sayisi) #buranın parametreleri arkadaşımın oluşturacağı veri tabanına göre değişecek şimdilik yorum satırında
        
        # aynı işlem çıkanları kontrol etmek için yaptım
        for cikacak_mi in list(cikacaklar):
            if son_yer[cikacak_mi] < (line_x - tolerans):
                kisi_sayisi-=1
                cikacaklar.remove(cikacak_mi)
                threadle_guncelle(0) # ama dur burda çıkanların çıış saati olmayacak orayı başka bir fonksiyonda ya da parametre vereyim girenler 1 çıkanlar sıfır diye
        
    # Çizgi ve sayaçları ekrana yazma yeri
    cv.line(frame, (line_x, 0), (line_x, frame.shape[0]), (0, 0, 255), 2)
    cv.putText(frame, f"toplam: {kisi_sayisi}", (10, 30), cv.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
    
    cv.imshow("Tuvalet giriş sayım", frame)
    if cv.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv.destroyAllWindows()

