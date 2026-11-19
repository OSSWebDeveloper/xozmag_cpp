# Ulug'bek Xozmag — exe (C++)

Saytdagi Xozmag (Django / BizaroPOS) bilan **bir xil ko'rinish va bir xil ishlash**, lekin
brauzersiz: kompyuterga o'rnatiladigan `Xozmag.exe`. Sahifalar saytning CSS'i (`uslub.css`)
bo'yicha Direct2D bilan chiziladi: o'sha ranglar (kechki / kunduzgi mavzu), o'sha ikonkalar
(`ikonlar.html` dan SVG), o'sha yon menyu, kartalar, jadvallar va oynalar.

## Server va kassalar (v2.2.0 dan)

Baza bitta kompyuterda turadi, boshqalar unga ichki tarmoq orqali ulanadi:

- **Xozmag Server** (`Xozmag.exe --server`) do'kondagi asosiy kompyuterda ochiq turadi.
  Oynasida kassalar uchun IP manzil yoziladi (masalan `192.168.1.5`), ulangan kassalar
  ro'yxati, baza fayli va «Kompyuter yoqilganda server o'zi ishga tushsin» belgisi bor.
  Port: 5577. Windows xavfsizlik devoriga ruxsat o'zi qo'shiladi.
- **Xozmag** (kassa) ochilganda login so'ragandek **server IP manzilini** so'raydi. Manzil
  saqlanadi va keyingi safar o'zi ulanadi. Aloqa uzilsa (server o'chdi, tarmoq yo'q) dastur
  ~5 soniyada sezadi va manzilni yana so'raydi; qayta ulangach o'sha sahifaga qaytadi.
  Ochiq chek bazada turgani uchun yo'qolmaydi.
- Login/parol server tomonida tekshiriladi va sessiya beriladi (14 kun, saytdagi kabi).
  Sessiyasiz kassa bazaga so'rov yubora olmaydi.
- Excel'ni qo'llash va bekor qilish serverning o'zida bajariladi. Tranzaksiya ichidagi
  yozuvlar bir yo'la yuboriladi: 8 924 qatorli qoralama 9 ta so'rov bilan tuziladi.
- Server ham, kassa ham ochilganda administrator huquqini so'raydi (UAC).

Server qaysi bazani ochadi:

1. `%LOCALAPPDATA%\Xozmag\baza_yoli.txt` da saqlangan fayl, bo'lmasa
2. **saytning o'z bazasi** `%LOCALAPPDATA%\BizaroPOS-Server\db.sqlite3`, bo'lmasa
3. alohida `%LOCALAPPDATA%\Xozmag\xozmag.sqlite3`.

Jadvallar Django bilan aynan bir xil, shuning uchun exe va sayt bitta bazada ishlaydi.

## Bo'limlar

| Bo'lim | Holat |
|---|---|
| Kirish, yon menyu, Kechki/Sensor richaglari, Chiqish | saytdagidek |
| Bosh sahifa (zarrachali fon, shisha kartalar) | saytdagidek |
| Sotuv (kassa): kod/skaner, qidiruv, F2 tovar oynasi, miqdorni ko'paytirish, «Qarzga» oynachasi (avtomatik bo'linish, yangi qarzdor), yakunlash, bekor qilish | saytdagidek |
| Qarz yozish (kassa ko'rinishi, «Oldindan» to'lov) | saytdagidek |
| Qarzdorlar: tanlov, ro'yxat, hududlar (+ qo'shish), qarzdor kartasi, to'lov, yangi/tahrir, «Oldin qarz olgan» | saytdagidek |
| Ombor: qidiruv, filtrlar, 50 tadan sahifalash, kirim, tuzatish, tarix, shtrix, tovar formasi | saytdagidek |
| Excel: yuklab olish, shablon, Excel'dan yangilash (qoralama/qo'llash/bekor), Yangilanishlar | saytdagidek (v2.1.0) |
| Sotuvlar (kunlik cheklar), Hisobot | saytdagidek |
| Login / Parol | saytdagidek |
| Qurilmalar | hozircha faqat chek printeri |
| Server–kassa: IP orqali ulanish, aloqa uzilsa qayta so'rash | v2.2.0 |
| Etiketka, tarozi, tovar qaytarish, Kassalar | keyingi versiyalarda |

## O'rnatish va yig'ish

`natija\Xozmag_Ornatuvchi_vX.Y.Z.exe` administrator huquqi bilan (UAC) `Program Files\Xozmag`
ga o'rnatadi. O'rnatishda «Bu kompyuter do'kon serveri bo'ladimi?» deb so'raladi: «Ha» bo'lsa
ish stoliga «Xozmag Server» ham qo'yiladi. Pusk menyusida ikkalasi ham bor. v2.1 dagi
foydalanuvchi papkasidagi eski nusxa o'zi olib tashlanadi.

```
powershell -ExecutionPolicy Bypass -File qurish.ps1
```

Kerak: WinLibs GCC (`D:\mingw64\bin`), Windows 10 1703+ (SVG ikonkalar uchun Direct2D).

Sinov uchun (yig'ishda `-DSINOV` bilan UAC so'ralmaydi):

```
Xozmag.exe --server --baza D:\sinov\db.sqlite3 --port 5578 --malumot D:\sinov\server
Xozmag.exe --malumot D:\sinov\kassa
```

`--malumot` — sozlama.ini papkasi (server manzili, sessiya, mavzu, printer).
`XOZMAG_JURNAL=<fayl>` muhit o'zgaruvchisi bo'lsa kassa har bir so'rovni faylga yozadi.

`versiya.txt` — yagona manba; `python versiya.py 2.2.0 "izoh"`.
