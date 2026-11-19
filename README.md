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

## Sayt bilan to'liq tenglashtirish (v2.6.0)

- **Qurilmalar** sahifasi saytdagi «Yangi ulangan qurilmalar» tartibida: «Shu kassa», chek printeri
  («Sinov savdo yaratmaydi…»), Rongta tarozi, **«Skaner va etiketkani sinash»** (kod, do'kon/tarozi
  etiketkasi yoki chek shtrixi — savdo yaratmaydi) va **USB qurilmalar** ro'yxati: har 10 soniyada
  tekshiriladi, yangi ulangani «Yangi», uzilgani xabarda. Saralash qoidalari saytdagi
  `config/qurilmalar.py` bilan bir xil; nomlar ham saytdagidek (sinovda 294 ta qurilmaning hammasi mos).
  Saytdagi «USB kassa yordamchisi» exe'ga kerak emas — chek to'g'ridan-to'g'ri printerga chiqadi.
- **Login / Parol**: Django'ning parol qoidalari — kamida 8 belgi, keng tarqalgan 20 000 parol
  ro'yxati, faqat raqam emas, loginga o'xshamasin. Standart parolli `server`/`kassa` akkauntida
  «Xavfsizlik uchun standart parolni o'zgartirish tavsiya etiladi.» Kirishda xato qizil qutida:
  «Login yoki parol xato.»
- **Qoldiqni tuzatish** oynasi saytdagidek: «Hozirgi qoldiq», «Yangi qoldiq», «Sabab (ixtiyoriy)»;
  o'zgarmagan qoldiq qabul qilinmaydi, shu orada boshqa kassa o'zgartirgan bo'lsa bosib ketilmaydi;
  tarixga «Qoldiq: 13,5 → 12 kg. sabab» yoziladi.
- **Shtrix kod**: 8/12/13/14 raqam; tarozi (13 xonali `2…`) va chek (`77…`, `88…`) prefikslari band.
  Boshqa oynada o'zgartirilgan shtrix bosib ketilmaydi.
- **Tovar formasi**: xarid narxi manfiy bo'lmasin, olish birligi sotuv birligi bilan bir xil bo'lmasin,
  saytdagi xabarlar; yangi tovarda «… skladga qo'shildi — 300 metr = 3 rulon.»
- **Hududlar**: ortiqcha bo'shliqlar olib tashlanadi, katta-kichik harfdan qat'i nazar takror bo'lmaydi.
- Sotuvlar: sotuvsiz kunda «Bu kuni sotuv bo'lmagan» va «Yangi sotuv boshlash». Hisobotda 500 tadan
  ko'p chek bo'lsa izoh.
- Tuzatilgan xatolar: Ombor qatoridagi «Tuzatish» va Qarzdorlar ro'yxatidagi «Qarz yozish» tugmalarini
  qator bosilishi «yutib» yuborardi; kun almashtirilganda Sotuvlar sarlavhasi eski kunda qolardi.

## Tovar qaytarish va Kassalar (v2.5.0 dan)

**Tovar qaytarish** saytdagi bilan bir xil, qisman ham bo'ladi (20 qopdan 5 tasi):
- Sotuvlar sahifasida har bir qatorda «Qaytarish»; «Chekni skanerlab qaytarish» — savdo
  chekidagi shtrix (`88` + chek raqami + nazorat, saytdagi kabi) bo'yicha chekni topadi.
- Chek oynasida «Mahsulot qaytarish»; chop etiladigan chekda endi qaytarish shtrixi bor.
- Qarzdor kartasida qarz qatorlari va chekdan qarzga o'tgan qatorlar uchun «Qaytarish».
- Tovar omborga qaytadi (tarixda «Qaytarish»). Chekda qarz bo'lsa pul avval shu qarzdan
  ayriladi (ko'pi bilan qarzdorning hozirgi qarzicha), qolgani qo'lga.

**Kassalar** (faqat server kompyuterida ochiladi): server IP, 2-kassa IP va tarmoq kartasi
indeksi `qurilmalar_data\kassalar.json` ga saqlanadi (sayt formati). Sozlangan server IP
server oynasida birinchi turadi. «2-kassa dasturini yuklab olish» ZIP beradi: 2-kassada
`ORNATISH.bat` 2-kassa IP manzilini tarmoq kartasiga qo'shadi (saytdagi `kassa_ip.ps1`),
dasturni `Program Files\Xozmag` ga qo'yadi va server manzilini yozadi.

## Etiketka va tarozi (v2.4.0 dan)

**Etiketka** — rasta uchun narx yorlig'i, saytdagi bilan bir xil: nomi, sotish narxi,
Code 128 shtrix (tovar kodi 6 raqam) va «KOD 0027». Ochiladi: Ombor qatoridagi printer
belgisi (nusxa = qoldiq), kirimdagi «Qo'shish va etiketka» (nusxa = kelgan dona/qadoq) va
yangi tovar saqlangach. O'lchamlar: 40×30, 58×40, 56×40 (tarozi), 50×30, 30×20 mm.
Chop etish shu kompyuterning Windows printeriga, qog'oz o'lchami yorliqniki; o'lcham va
printer eslab qolinadi.

**Rongta RLS1100C tarozisi (Ethernet)** — Qurilmalar sahifasida (Ombordagi «Taroziga
yuborish» ham shu yerga olib keladi). Tarozi server kompyuteriga Ethernet orqali ulangan,
shuning uchun aloqa sinovi va katalog yuborish **serverda** bajariladi. Qoidalar saytdagi
`config/RONGTA.md` bilan bir xil: faqat kg, faol, so'mdagi narxli tovarlar; yuborishdan
oldin zaxira, tugagach qayta o'qib solishtirish; boshqa PLU'lar yashiriladi. Tarozi IP
manzili sahifada yoziladi (standart 192.168.1.110).

Ishlab chiqaruvchining drayveri (`rtslabelscale.dll`, x86) dastur bilan tarqatilmaydi:
server kompyuterida `%LOCALAPPDATA%\Xozmag\qurilma_drivers\rongta` ga qo'yiladi.
Saytning `qurilma_drivers\rongta` papkasi va Rongta dasturi o'rnatilgan joy ham qidiriladi.
Zaxira va hisobotlar: `%LOCALAPPDATA%\Xozmag\qurilmalar_data`.

## Printerlar (v2.3.0 dan)

Dastur ochilishi bilan (kassa ham, server ham) shu kompyuterdagi **har bir printerga bir xil
savol** chiqadi: «Chek», «Etiketka» yoki «Ishlatilmaydi». «Sinov» tugmasi o'sha printerga
kichik sinov varag'ini yuboradi — qaysi printer qaysiligini bilish uchun. Chek ham, etiketka
ham bittadan printerga beriladi.

Javob shu kompyuterning sozlama.ini sida saqlanadi. Keyingi safar so'ralmaydi, faqat:
yangi printer paydo bo'lsa, tanlangan printer o'chirilsa yoki «Har safar dastur ochilganda
so'ralsin» belgilangan bo'lsa. Kassada keyin «Qurilmalar → Barcha printerlar», serverda
«Printerlar → O'zgartirish» orqali ham ochiladi.

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
| Qurilmalar («Yangi ulangan qurilmalar»): shu kassa, chek printeri, Rongta tarozi, skaner sinovi, USB qurilmalar | saytdagidek (v2.6.0) |
| Server–kassa: IP orqali ulanish, aloqa uzilsa qayta so'rash | v2.2.0 |
| Ochilganda printerlar so'raladi (kassa ham, server ham) | v2.3.0 |
| Etiketka (narx yorlig'i) va Rongta RLS1100C tarozi | v2.4.0 |
| Tovar qaytarish (chek, chek shtrixi, qarz) va Kassalar | v2.5.0 |
| Sayt bilan to'liq tenglashtirish: parol qoidalari, qoldiq tuzatish, shtrix/tovar/hudud tekshiruvlari | v2.6.0 |

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
