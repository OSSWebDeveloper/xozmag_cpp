// Umumiy dialoglar: tovar va qarzdor tanlash, tovar/qarzdor formalari, hududlar, chek.
#include <algorithm>

#include "sahifa.h"

static int indeks(const std::vector<wstring>& v, const wstring& s, int bosh = 0) {
    auto it = std::find(v.begin(), v.end(), s);
    return it == v.end() ? bosh : (int)(it - v.begin());
}

long long tovar_tanla(HWND ota) {
    std::vector<Tovar> tl = tovarlarni_ol(false);
    long long tanlangan = 0;
    RoyxatSozlama s;
    s.sarlavha = L"Tovar qidirish";
    s.qidiruv_ishora = L"Nomi yoki kodi bo'yicha — masalan: gvozd 8";
    s.ustunlar = {{L"Kod", 70}, {L"Nomi", 470}, {L"Qoldiq", -90}, {L"Birlik", 70}, {L"Narx", -130}};
    s.en = 920;
    s.boy = 600;
    s.pastki = L"↑↓ — tanlash, Enter — chekka qo'shish";
    s.toldir = [&](HWND lv, const wstring& q) {
        auto b = bolakla(q);
        int n = 0;
        for (size_t i = 0; i < tl.size() && n < 300; ++i) {
            auto& t = tl[i];
            if (!mos_keladi(t.izlash, b)) continue;
            lv_qosh(lv, {t.kod, t.nom, son(t.qoldiq), t.birlik,
                         t.narx > 0 ? pul(t.narx) + L" " + t.belgi() : L"—"},
                    (LPARAM)t.id);
            ++n;
        }
    };
    s.tugmalar = {L"Chekka qo'shish"};
    s.bosildi = [&](int, LPARAM p, HWND) {
        if (p < 0) return false;
        tanlangan = p;
        return true;
    };
    royxat_oyna(ota, s);
    return tanlangan;
}

long long qarzdor_tanla(HWND ota) {
    long long tanlangan = 0;
    RoyxatSozlama s;
    s.sarlavha = L"Qarzdorni tanlash";
    s.qidiruv_ishora = L"Familiya, ism, telefon yoki hudud";
    s.ustunlar = {{L"Familiya Ism", 280}, {L"Telefon", 150}, {L"Hudud", 150},
                  {L"Qarz, so'm", -140}, {L"Qarz, $", -100}};
    s.en = 900;
    s.toldir = [&](HWND lv, const wstring& q) {
        for (auto& d : qarzdorlar(q))
            lv_qosh(lv, {d.toliq(), d.telefon, d.hudud, pul(d.balans), pul(d.balans_d)}, (LPARAM)d.id);
    };
    s.tugmalar = {L"Tanlash", L"Yangi qarzdor"};
    s.bosildi = [&](int i, LPARAM p, HWND) {
        if (i == 1) {
            long long id = qarzdor_formasi(faol_oyna(), 0);
            if (!id) return false;
            tanlangan = id;
            return true;
        }
        if (p < 0) return false;
        tanlangan = p;
        return true;
    };
    royxat_oyna(ota, s);
    return tanlangan;
}

long long qarzdor_formasi(HWND ota, long long id) {
    Qarzdor q;
    if (id && !qarzdor_ol(id, q)) return 0;
    auto hl = hududlar();
    if (hl.empty()) {
        xabar(ota, L"Avval hudud qo'shing (Qarz daftari → Hududlar).", true);
        return 0;
    }
    std::vector<wstring> nomlar;
    int tan = -1;
    for (size_t i = 0; i < hl.size(); ++i) {
        nomlar.push_back(hl[i].nom);
        if (hl[i].id == q.hudud_id) tan = (int)i;
    }
    if (!id) {
        nomlar.insert(nomlar.begin(), L"— tanlang —");
        tan = 0;
    }
    std::vector<Maydon> ms = {Maydon::matn(L"Familiya", q.familiya), Maydon::matn(L"Ism", q.ism),
                              Maydon::matn(L"Telefon", q.telefon),
                              Maydon::tanla(L"Hudud", nomlar, tan < 0 ? 0 : tan),
                              Maydon::matn(L"Izoh", q.izoh)};
    long long saqlangan = 0;
    forma(ota, id ? L"Qarzdorni tahrirlash" : L"Yangi qarzdor", ms, [&](std::vector<Maydon>& m) {
        Qarzdor y = q;
        y.familiya = m[0].qiymat;
        y.ism = m[1].qiymat;
        y.telefon = m[2].qiymat;
        int t = m[3].tanlangan - (id ? 0 : 1);
        y.hudud_id = (t >= 0 && t < (int)hl.size()) ? hl[t].id : 0;
        y.izoh = m[4].qiymat;
        wstring x;
        saqlangan = qarzdor_saqla(y, x);
        return x;
    });
    return saqlangan;
}

bool tovar_formasi(HWND ota, long long id) {
    Tovar t;
    if (id && !tovar_ol(id, t)) return false;
    if (!id) t.kod = keyingi_kod();
    std::vector<wstring> birliklar = BIRLIKLAR;
    if (indeks(birliklar, t.birlik, -1) < 0) birliklar.push_back(t.birlik);
    std::vector<wstring> olish = {L"— sotuv birligining o'zi"};
    for (auto& b : birliklar) olish.push_back(b);
    std::vector<wstring> pullar = {L"so'm", L"dollar"};
    std::vector<wstring> kelish_pul = {L"—", L"so'm", L"dollar"};

    std::vector<Maydon> ms = {
        Maydon::matn(L"Kod (turgan joyi)", t.kod),
        Maydon::matn(L"Nomi", t.nom),
        Maydon::tanla(L"Sotuv birligi", birliklar, indeks(birliklar, t.birlik)),
        Maydon::tanla(L"Qaysi pulda keladi", pullar, t.dollarmi() ? 1 : 0),
        Maydon::son(L"Narxi (1 sotuv birligi)", t.narx > 0 ? pul(t.narx) : L""),
        Maydon::son(L"Kelish narxi", t.kelish_bor ? pul(t.kelish_narxi) : L""),
        Maydon::tanla(L"Kelish narxi valyutasi", kelish_pul,
                      !t.kelish_bor ? 0 : (t.kelish_valyutasi == L"dollar" ? 2 : 1)),
        Maydon::tanla(L"Olish birligi", olish,
                      t.olish_birligi.empty() ? 0 : indeks(birliklar, t.olish_birligi) + 1),
        Maydon::son(L"Bittasida (sotuv birl.)", son(t.olish_miqdori)),
        id ? Maydon::malumot(L"Qoldiq: " + son(t.qoldiq) + L" " + t.birlik +
                             L"  (Kirim yoki «Qoldiqni tuzatish» bilan o'zgaradi)")
           : Maydon::son(L"Boshlang'ich qoldiq", L"0"),
        Maydon::malumot(L"Kod bo'sh qolsa navbatdagi bo'sh kod beriladi (0001…9999).\n"
                        L"Rulonda olinib metrda sotilsa: Olish birligi = rulon, Bittasida = 100."),
    };
    bool ok = forma(ota, id ? L"Tovarni tahrirlash" : L"Yangi tovar", ms, [&](std::vector<Maydon>& m) {
        Tovar y = t;
        y.kod = m[0].qiymat;
        y.nom = m[1].qiymat;
        y.birlik = birliklar[m[2].tanlangan < 0 ? 0 : m[2].tanlangan];
        y.valyuta = m[3].tanlangan == 1 ? L"dollar" : L"som";
        if (!songa(m[4].qiymat, y.narx)) return wstring(L"Narx noto'g'ri yozilgan.");
        y.kelish_bor = !tozala(m[5].qiymat).empty();
        if (y.kelish_bor && !songa(m[5].qiymat, y.kelish_narxi))
            return wstring(L"Kelish narxi noto'g'ri yozilgan.");
        if (y.kelish_bor)
            y.kelish_valyutasi = m[6].tanlangan == 2 ? L"dollar"
                               : m[6].tanlangan == 1 ? L"som" : y.valyuta;
        y.olish_birligi = m[7].tanlangan <= 0 ? L"" : birliklar[m[7].tanlangan - 1];
        if (!songa(m[8].qiymat, y.olish_miqdori)) return wstring(L"«Bittasida» noto'g'ri.");
        double bosh = 0;
        if (!id && !songa(m[9].qiymat, bosh)) return wstring(L"Boshlang'ich qoldiq noto'g'ri.");
        wstring x;
        tovar_saqla(y, bosh, x);
        return x;
    });
    if (ok) ombor_eskirdi();
    return ok;
}

void hududlar_oynasi(HWND ota) {
    RoyxatSozlama s;
    s.sarlavha = L"Hududlar";
    s.qidiruv = false;
    s.ustunlar = {{L"Hudud nomi", 420}};
    s.en = 640;
    s.boy = 520;
    s.toldir = [](HWND lv, const wstring&) {
        for (auto& h : hududlar()) lv_qosh(lv, {h.nom}, (LPARAM)h.id);
    };
    s.tugmalar = {L"Nomini o'zgartirish", L"Yangi hudud"};
    s.bosildi = [](int i, LPARAM p, HWND lv) {
        if (i == 0) {
            if (p < 0) return false;
            wchar_t b[200] = {};
            ListView_GetItemText(lv, lv_tanlangan(lv), 0, b, 200);
            std::vector<Maydon> ms = {Maydon::matn(L"Yangi nomi", b)};
            forma(faol_oyna(), L"Hudud nomi", ms, [&](std::vector<Maydon>& m) {
                wstring x;
                hudud_nomla(p, m[0].qiymat, x);
                return x;
            });
        } else {
            std::vector<Maydon> ms = {Maydon::matn(L"Hudud nomi")};
            forma(faol_oyna(), L"Yangi hudud", ms, [&](std::vector<Maydon>& m) {
                wstring x;
                hudud_qosh(m[0].qiymat, x);
                return x;
            });
        }
        return false;
    };
    royxat_oyna(ota, s);
}

void chek_oynasi(HWND ota, long long sotuv_id) {
    Chek c;
    if (!chek_ol(sotuv_id, c)) {
        xabar(ota, L"Chek topilmadi.", true);
        return;
    }
    matn_oyna(ota, L"Chek #" + std::to_wstring(sotuv_id), chek_matni(c), L"Chop etish", [&]() {
        wstring x;
        if (chek_chop(c, x))
            xabar(faol_oyna(), L"Chek printerga yuborildi.");
        else
            xabar(faol_oyna(), L"Chop etilmadi:\n" + x, true);
    });
}
