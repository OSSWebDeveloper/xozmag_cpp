# Versiyalar tarixi

## 2.3.0 — 2026-11-19

Printerlar: dastur ochilganda (kassa ham, server ham) har bir printerga bir xil savol — chek, etiketka yoki ishlatilmaydi

## 2.2.0 — 2026-11-19

Server va kassa: IP orqali ulanish, UAC

- Xozmag Server (Xozmag.exe --server): baza shu kompyuterda, kassalar 5577-portga ulanadi; oynada IP manzil va ulangan kassalar
- Kassa ochilganda server IP manzilini so'raydi va saqlaydi; aloqa uzilsa yana so'raydi, qayta ulangach o'sha sahifaga qaytadi
- Kirish server tomonida tekshiriladi, sessiya 14 kun; sessiyasiz so'rov bajarilmaydi
- Excel qo'llash/bekor qilish serverda; tranzaksiya yozuvlari bir yo'la ketadi (8 924 qatorli qoralama 9 ta so'rov)
- Server, kassa va o'rnatuvchi UAC (administrator) so'raydi; o'rnatish Program Files ga, o'rnatishda server yoki kassa tanlanadi
- Tuzatildi: aylantirilganda maydonda eski piksellar; tovar formasida xato yozuvi ustma-ust

## 2.1.0 — 2026-11-19

Ombor: Excel orqali yangilash va yangilanishlar — saytdagi bilan bir xil.

- .xlsx o'qish/yozish o'zimizniki (zip + deflate + XML), tashqi kutubxonasiz
- Excel'dan yangilash: fayl -> qoralama -> qatorni tuzatish/chiqarish -> qo'llash -> bekor qilish
- Yangilanishlar ro'yxati va qoralama sahifasi, «Products …xlsx» fayllarini ham tushunadi
- Omborni Excel'ga yuklab olish va bo'sh shablon (filtr va tartib bilan)
- Natijalar Django versiyasi bilan solishtirildi: qoralama, qo'llash, bekor qilish bir xil
- Tuzatildi: tasdiqlash oynasi ochilganda tugma ikki marta ishlamasin (ichma-ich chizish)
- Ombor tartibi saytdagidek: kg tovarlar birinchi
- 9999 kod to'lganda sayt yiqiladi, dastur esa qatorni «bo'sh kod qolmadi» xatosi qiladi

## 2.0.0 — 2026-11-18

Sayt bilan bir xil ko'rinish: Direct2D chizish qatlami, saytning ranglari, ikonkalari va sahifalari.

- Kirish (Django parollari), yon menyu, Kechki/Kunduzgi mavzu, Sensor rejimi
- Bosh sahifa, Sotuv (kassa) va Qarz yozish — saytdagi mantiq bilan (chek bazada turadi)
- Qarzdorlar, hududlar, qarzdor kartasi va to'lovlar
- Ombor ro'yxati, kirim, tuzatish, tarix, shtrix, tovar formasi
- Sotuvlar (kunlik), Hisobot, Login/Parol, chek printeri
- Saytning o'z bazasini (BizaroPOS-Server\db.sqlite3) to'g'ridan-to'g'ri ochadi

## 1.0.0 — 2026-11-18

Xozmag'ning C++ (Win32 + SQLite) desktop versiyasi — birinchi ishlaydigan holat.

- Kassa: kod/skaner (tarozi EAN-13 ham), miqdor, raqamlar klaviaturasi, savdolashuv, qarzga bo'lish, chek chop etish
- Qarz daftari: qarzdorlar, so'm/$ alohida balans, tarix, to'lov, hududlar
- Ombor: ~10 000 tovar, qidiruv, tovar formasi, kirim (ikki birlik), tuzatish, yashirish, tarix
- Hisobot: kunlik tushum, qarzlar, to'lovlar, cheklar
- Sozlamalar: printer, chek eni, dollar kursi (cbu.uz), Django bazasini import, zaxira
- O'rnatuvchi: administrator huquqisiz, yorliqlar, «Ilovalar» ro'yxatidan o'chirish

