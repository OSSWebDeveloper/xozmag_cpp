# Xozmag (C++)

Qurilish mollari do'koni uchun **kassa, ombor va qarz daftari** — Windows uchun o'rnatiladigan
dastur. Django'dagi Xozmag'ning (BizaroPOS) C++ dagi ko'rinishi: internet va server kerak emas,
bitta kompyuterda ishlaydi.

## O'rnatish

`natija\Xozmag_Ornatuvchi_vX.Y.Z.exe` ni ishga tushiring. Administrator huquqi kerak emas.

- Dastur: `%LOCALAPPDATA%\Programs\Xozmag` (Xozmag.exe, sqlite3.dll, Ochirish.exe)
- Ma'lumotlar: `%LOCALAPPDATA%\Xozmag\xozmag.sqlite3` va `zaxira\` — qayta o'rnatganda ham,
  o'chirganda ham tegilmaydi
- Ish stoli va Pusk menyusida yorliq; «Ilovalar va imkoniyatlar» dan o'chiriladi

## Bo'limlar

| Bo'lim | Nima qiladi |
|---|---|
| **Kassa** | Kod / skaner (27 = 0027, eski 5–6 xonali etiketka, tarozi EAN-13 og'irligi bilan), miqdor, chek. Raqamlar klaviaturasi o'ngda. «Jami» savdolashib o'zgaradi, «To'landi» kam bo'lsa qolgani tanlangan qarzdorga yoziladi. Ombor faqat yakunlashda kamayadi. F2 — tovar qidirish, F4 — qarzdor, F12 — yakunlash, Del — qatorni o'chirish |
| **Qarz daftari** | Qarzdorlar (so'm va $ alohida), qarz/to'lov tarixi, to'lov qabul qilish, hududlar, «Qarz yozish» kassaga o'tkazadi |
| **Ombor** | ~10 000 tovar (virtual ro'yxat), qidiruv, ustun bo'yicha tartiblash, yangi tovar, tahrir, kirim (rulon → metr kabi ikki birlik), qoldiqni tuzatish, yashirish, harakatlar tarixi |
| **Hisobot** | Kun bo'yicha naqd tushum, qarzga yozilgan, qarz to'lovlari, cheklar; chekni ko'rish va qayta chop etish |
| **Sozlamalar** | Do'kon nomi, chek printeri (58/80 mm), dollar kursi (qo'lda yoki cbu.uz), **eski bazani import**, zaxira |

## Eski bazadan ko'chirish

Jadvallar Django'dagi Xozmag bilan **aynan bir xil** (`ombor_mahsulot`, `qarz_qarz`,
`sotuv_sotuv` ...). Sozlamalar → «Eski bazani import qilish» → `db.sqlite3` ni tanlang.
Fayl faqat o'qiladi; oldin dasturdagi joriy ma'lumotlarning zaxirasi olinadi. Eski
versiyalardagi bazada yo'q ustunlarga standart qiymat qo'yiladi.

## Yig'ish

Kerak: WinLibs GCC (`D:\mingw64\bin`), Python (faqat ikonka va versiya uchun).

```
powershell -ExecutionPolicy Bypass -File qurish.ps1
```

Natija: `build\Xozmag.exe` va `natija\Xozmag_Ornatuvchi_vX.Y.Z.exe`. SQLite —
`tashqi\sqlite3.dll` (public domain), g++ unga to'g'ridan-to'g'ri ulanadi.

Sinov uchun alohida baza bilan: `Xozmag.exe --malumot D:\sinov_papka`.

## Versiyalash

`versiya.txt` — yagona manba (qurishda .exe ichiga yoziladi).

```
python versiya.py 1.1.0 "Qisqa izoh" -b "band"
```

## Hali ko'chirilmagan (Django versiyasida bor)

Rang (kolerovka) bo'limi, tarozi va etiketka printeriga yuborish, Excel orqali omborni
yangilash, kassirlar akkaunti, tovar qaytarish, bir nechta kassa tarmoqda.
