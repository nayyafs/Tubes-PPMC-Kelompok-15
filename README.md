# README

## Folder Utama -> Final
Didalam folder final terdapat kode Actuator dan Checker
- Checker merepresentasikan Blood Glucose Level Detector
- Actuator merepresentasikan Insulin Pump
Dikarenakan harga detector dan pump yang mahal, kami merepresentasikan kedua alat tersebut dengan prototype yang memanfaatkan esp32. Bacaan hasil Blood Glucose Level direpresentasikan sebagai bacaan jarak sensor ultrasonic pada papan. Sedangkan Jumlah tetesan dari Insulin Pump direpresentasikan sebagai kedipan LED dari ESP32

Kami menggunakan 2 esp32
- ESP32 pertama sebagai checker yang akan menerima input berupa jarak sensor ultrasonic dengan papan, lalu melakukan publish data berupa hasil enkripsi panjang jarak terbaca melalui protokol MQTT
- ESP32 Kedia sebagai actuator yang akan melakukan subscribe data dan melakukan dekripsi sehingga menghasilkan data panjang jarak. Data tersebut lalu dikonversi menjadi jumlah kedipan LED built in dari ESP32


### Cara penggunaan
- Pada kode checker dan actuator, Ubah nama dan password wifi pada bagian wifi setup sesuai dengan wifi yang digunakan
- Upload kode Checker dan Actuator pada 2 esp32 berbeda
- Set up MQTT di laptop dengan menginstall terlebih dahulu pada link berikut https://mosquitto.org/download/
- Aktifkan mosquitto melalui command prompt dengan menuliskan C:/<PATH>/mosquitto/mosquitto.exe
- Buka 4 window commad prompt untuk mempermudah visualisasi
- Siapkan command prompt pertama untuk publish message
- Aktifkan command prompt kedua sebagai subscriber dengan mengetikkan C:/<PATH>/mosquitto>mosquitto_sub -h "broker.mqtt-dashboard.com" -t "ESP32_Insulin_Checker_topic"
- Aktifkan command prompt ketiga sebagai subscriber dengan mengetikkan C:/<PATH>/mosquitto>mosquitto_sub -h "broker.mqtt-dashboard.com" -t "ESP32_Insulin_Status_topic"
- Aktifkan command prompt keempat sebagai subscriber dengan mengetikkan C:/<PATH>/mosquitto>mosquitto_sub -h "broker.mqtt-dashboard.com" -t "ESP32_Insulin_Pump_topic"
- Publish message menggunakan command prompt pertama dengan mengetikkan C:/<PATH>/mosquitto>mosquitto_pub -h "broker.mqtt-dashboard.com" -t "ESP32_Insulin_Checker_topic" -m "READ"
- Setiap kali message di-Publish, ESP32 Checker akan membaca jarak sensor ultrasonik ke papan lalu melakukan enkripsi data jarak tersebut
- Hasil enkripsi akan di-Publish pada topic ESP32_Insulin_Pump_topic
- ESP32 Actuator akan membaca data pada topic ESP32_Insulin_Pump_topic dan melakukan dekripsi menjadi jumlah kedipan LED yang perlu dilakukan
- LED pada ESP32 Actuator akan berkedip sebanyak jarak yang terbaca ESP32 Checker
