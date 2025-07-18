# STM32 I2C NEMA 17 Stepper Motor Control (with ESP32 Bridge)

Ushbu loyiha **STM32F103C8T6** mikrokontrolleri yordamida **I2C** aloqasi orqali **NEMA 17 (17HS4401)** step dvigatelini boshqarishni namoyish etadi. Boshqaruv buyruqlari **ESP32** orqali veb-ilovadan qabul qilinadi va I2C orqali STM32'ga uzatiladi, bu esa chekka hisoblash (Edge Computing) va masofaviy boshqaruv uchun qulay yechim yaratadi.

---

## Asosiy Xususiyatlar

* **STM32F103C8T6** ("Blue Pill") mikrokontrolleriga asoslangan dvigatel boshqaruvi.
* **NEMA 17 (17HS4401)** step dvigatelini boshqarish.
* **TB6600** step dvigatel drayveridan foydalaniladi.
* **I2C** aloqa protokoli orqali ESP32'dan STM32'ga buyruq uzatish.
* **ESP32** Wi-Fi/Internet orqali veb-ilovadan buyruqlarni qabul qilish uchun aloqa ko'prigi vazifasini bajaradi.
* Dvigatelning quyidagi xususiyatlarini nazorat qilish:
    * **Motor faolligi** (0/1 bit hex formatida: dvigatelni yoqish/o'chirish).
    * **Dvigatel holati** (0/1 bit hex formatida: enable/disable).
    * **Burilish yo'nalishi** (0/1 bit hex formatida: soat yo'nalishi/teskari soat yo'nalishi).
    * **Burilish tezligi** (1 bit hex formatida, desimalga o'tkaziladi).
    * **Burilish burchagi** (2 bit hex formatida: 0xnm, 0xop; umumiy burchak = `nm` desimal * 100 + `op` desimal).
* **STM32CubeIDE** muhitida ishlab chiqilgan.

---

## Loyihaning ishlash prinsipi

1.  **Buyruq qabul qilish (ESP32):** ESP32 mikrokontrolleri Wi-Fi yoki internet orqali veb-ilovadan hex formatidagi buyruqlarni qabul qiladi. Bu buyruq quyidagi ma'lumotlarni o'z ichiga oladi:
    * Motorning yoqish/o'chirish holati (0/1).
    * Dvigatelning enable holati (0/1).
    * Burilish yo'nalishi (0/1).
    * Burilish tezligi (desimal qiymat).
    * Burilish burchagi (ikki baytli hex, masalan, 0xnm 0xop, bu yerda umumiy burchak $nm \times 100 + op$ desimalga teng).
2.  **Buyruqni uzatish (ESP32 dan STM32 ga):** ESP32 qabul qilingan buyruqni I2C protokoli orqali STM32F103C8T6'ga yuboradi.
3.  **Buyruqni bajarish (STM32):** STM32 I2C orqali buyruqni qabul qiladi, uni tahlil qiladi va NEMA 17 dvigatelini TB6600 drayveri orqali talab qilingan tezlik, yo'nalish va burchakka mos ravishda boshqaradi.

---

## Kerakli Apparat (Hardware Requirements)

* **STM32F103C8T6** mikrokontroller plati ("Blue Pill" yoki shunga o'xshash).
* **NEMA 17 (17HS4401)** step dvigateli.
* **TB6600** step dvigatel drayveri.
* **ESP32** plati (ESP32-WROOM-32 yoki shunga o'xshash).
* Quvvat manbai (NEMA 17 va TB6600 uchun mos keladigan).
* Ulanish kabellari, breadboard (agar kerak bo'lsa).

---

## Kerakli Dasturiy ta'minot (Software Requirements)

* **STM32CubeIDE** (STM32 dasturini kompilyatsiya qilish va yuklash uchun).
* **STM32CubeMX** (STM32 periferik sozlamalari uchun, `.ioc` fayli orqali).
* **Arduino IDE** (yoki PlatformIO) ESP32 dasturini yuklash uchun.
* ESP32 veb-ilovasi va server kodi (agar uchinchi tomon yechimi ishlatilgan bo'lsa, havolasini kiriting).

---

## Ulanish sxemasi (Wiring Diagram)

Quyida STM32, ESP32, TB6600 va NEMA 17 dvigatellarining ulanish sxemasi keltirilgan.

**STM32F103C8T6 (Master) va ESP32 (Slave) I2C ulanishi:**
* **STM32 I2C1 SCL** (PA9) ga **ESP32 I2C SCL** ulanadi.
* **STM32 I2C1 SDA** (PA10) ga **ESP32 I2C SDA** ulanadi.
* **GND** pinlarini ulang.

**STM32F103C8T6 va TB6600 ulanishi:**
* **STM32 GPIO (Step)** piniga **TB6600 STEP** pinini ulang.
* **STM32 GPIO (Dir)** piniga **TB6600 DIR** pinini ulang.
* **STM32 GPIO (Enable)** piniga **TB6600 ENA** pinini ulang.
* **GND** pinlarini ulang.

**TB6600 va NEMA 17 ulanishi:**
* TB6600 drayverining **A+, A-, B+, B-** terminallarini NEMA 17 dvigatelining mos keladigan g'altaklariga ulang.
* TB6600 drayveriga tashqi quvvat manbaini (NEMA 17 uchun mos keladigan) ulang.

---

## O'rnatish va ishga tushirish (Setup and Getting Started)

1.  **Loyihani klonlash:** Ushbu repozitoriyani lokal kompyuteringizga klonlang:
    ```bash
    git clone [https://github.com/YourUsername/STM32-I2C-NEMA17-Stepper-Control.git](https://github.com/YourUsername/STM32-I2C-NEMA17-Stepper-Control.git)
    cd STM32-I2C-NEMA17-Stepper-Control
    ```
2.  **STM32 loyihasini import qilish:**
    * **STM32CubeIDE**'ni oching.
    * `File -> Import -> General -> Existing Projects into Workspace` ni tanlang.
    * "Select root directory" qismida klonlagan repozitoriy papkasini (yoki sizning holatingizda, fayllarni ko'chirgan ichki `Your_Repo_Name` papkasini) tanlang.
    * `Finish` tugmasini bosing.
3.  **ESP32 loyihasini yuklash:**
    * ESP32 kodi va veb-ilova joylashgan papkaga o'ting.
    * **Arduino IDE** (yoki PlatformIO) orqali ESP32 dasturini kompilyatsiya qiling va ESP32 plataningizga yuklang. Wi-Fi sozlamalarini o'zingizning tarmog'ingizga mos ravishda yangilashni unutmang.
4.  **STM32 loyihasini yuklash:**
    * STM32CubeIDE'da loyihaning barcha fayllari to'g'ri import qilinganligiga ishonch hosil qiling.
    * `main.c` faylida I2C va GPIO pinlari to'g'ri sozlangani va TB6600 uchun pin ta'riflari mos kelishini tekshiring.
    * Loyihani kompilyatsiya qiling va STM32F103C8T6 plataningizga yuklang (Flash).

---

## Foydalanish (Usage)

ESP32 veb-ilovasi ishga tushgandan so'ng, siz veb-interfeys orqali step dvigatelini boshqarish buyruqlarini yuborishingiz mumkin. Buyruqlar STM32'ga I2C orqali uzatiladi, va STM32 motor harakatini boshqaradi.

Veb-ilovaning foydalanish yo'riqnomalari va buyruq formatlari ESP32 loyiha papkasida batafsilroq berilgan.

---

## Hisobot berish (Contributing)

Agar loyihani yaxshilashga yordam bermoqchi bo'lsangiz, quyidagi bosqichlarni bajaring:

1.  Ushbu repozitoriyani (fork) qiling.
2.  Yangi branch (tarmoq) yarating (`git checkout -b feature/NewFeature`).
3.  O'zgarishlaringizni qo'shing (commit qiling) (`git commit -am 'Add new feature'`).
4.  O'zgarishlaringizni branch'ga yuklang (push qiling) (`git push origin feature/NewFeature`).
5.  Pull Request yarating.

---

## Aloqa

Savollar yoki takliflar bo'lsa, GitHub orqali Issues bo'limida murojaat qilishingiz mumkin.

---
