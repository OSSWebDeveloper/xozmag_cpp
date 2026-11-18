# Ulug'bek Xozmag — exe (C++)

Saytdagi Xozmag (Django / BizaroPOS) bilan **bir xil ko'rinish va bir xil ishlash**, lekin
brauzersiz: kompyuterga o'rnatiladigan `Xozmag.exe`. Sahifalar saytning CSS'i (`uslub.css`)
bo'yicha Direct2D bilan chiziladi: o'sha ranglar (kechki / kunduzgi mavzu), o'sha ikonkalar
(`ikonlar.html` dan SVG), o'sha yon menyu, kartalar, jadvallar va oynalar.

## Baza

Jadvallar Django bilan aynan bir xil. Dastur ochilganda:

1. `%LOCALAPPDATA%\Xozmag\baza_yoli.txt` da saqlangan fayl, bo'lmasa
2. **saytning o'z bazasi** `%LOCALAPPDATA%\BizaroPOS-Server\db.sqlite3`, bo'lmasa
3. alohida `%LOCALAPPDATA%\Xozmag\xozmag.sqlite3`.

Ya'ni do'kon kompyuterida exe va sayt **bitta bazada** ishlaydi: exe'da yozilgan chek saytda
ko'rinadi va aksincha. Kirish sahifasining pastida qaysi fayl ochilgani yoziladi va
«Bazani tanlash» bilan boshqasini tanlash mumkin. Login va parol — saytdagisi (Django
`pbkdf2_sha256` xeshi tekshiriladi).

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
| Etiketka, tarozi, tovar qaytarish, Kassalar | keyingi versiyalarda |

## O'rnatish va yig'ish

`natija\Xozmag_Ornatuvchi_vX.Y.Z.exe` — administrator huquqisiz o'rnatadi.

```
powershell -ExecutionPolicy Bypass -File qurish.ps1
```

Kerak: WinLibs GCC (`D:\mingw64\bin`), Windows 10 1703+ (SVG ikonkalar uchun Direct2D).
Sinov uchun boshqa baza bilan: `Xozmag.exe --baza D:\sinov\db.sqlite3`.

`versiya.txt` — yagona manba; `python versiya.py 2.1.0 "izoh"`.
