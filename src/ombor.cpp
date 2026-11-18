// OMBOR: tovarlar ro'yxati (10 000 ga yaqin — virtual ro'yxat), kirim, tuzatish, tarix.
#include <algorithm>

#include "sahifa.h"

namespace {

enum { ID_SARLAVHA = 5, ID_QIDIR = 10, ID_YASHIRIN, ID_RO, ID_SANOQ, ID_YANGI = 20, ID_TAHRIR,
       ID_KIRIM, ID_TUZAT, ID_YASHIR, ID_TARIX };

class Ombor : public Sahifa {
public:
    bool eskirgan = true;

    void yarat() override {
        hSarlavha = yorliq(h, L"OMBOR", ID_SARLAVHA, fJuda);
        edQidir = maydon(h, ID_QIDIR, fKatta, 0, L"Kod yoki nom bo'yicha qidirish");
        chYashirin = belgi(h, L"Yashirilganlar ham", ID_YASHIRIN);
        hSanoq = yorliq(h, L"", ID_SANOQ);
        lv = royxat(h, ID_RO, true);
        ustun(lv, 0, L"Kod", 70);
        ustun(lv, 1, L"Nomi", 420);
        ustun(lv, 2, L"Qoldiq", 100, true);
        ustun(lv, 3, L"Birlik", 70);
        ustun(lv, 4, L"Narx", 120, true);
        ustun(lv, 5, L"Kelish narxi", 120, true);
        ustun(lv, 6, L"Holat", 100);
        const wchar_t* nomlar[] = {L"Yangi tovar", L"Tahrirlash", L"Kirim (keldi)",
                                   L"Qoldiqni tuzatish", L"Yashirish / ochish", L"Harakatlar tarixi"};
        for (int i = 0; i < 6; ++i) b[i] = tugma(h, nomlar[i], ID_YANGI + i, i == 0 ? T_KOK : T_ODDIY);
    }

    void joylash(int w, int hg) override {
        const int M = dp(16), G = dp(8);
        joy(hSarlavha, M, M, dp(300), dp(36));
        joy(hSanoq, w - M - dp(400), M + dp(10), dp(400), dp(24));
        int y = M + dp(44);
        joy(edQidir, M, y, w - M * 2 - dp(220) - G, dp(40));
        joy(chYashirin, w - M - dp(220), y + dp(8), dp(220), dp(26));
        y += dp(48);
        int pastki = hg - M - dp(46);
        joy(lv, M, y, w - M * 2, pastki - G - y);
        int bw = (w - M * 2 - G * 5) / 6;
        for (int i = 0; i < 6; ++i) joy(b[i], M + i * (bw + G), pastki, bw, dp(46));
    }

    void korsatildi() override {
        if (eskirgan) yukla();
        SetFocus(edQidir);
    }

    bool kalit(MSG& m) override {
        if (!IsWindowVisible(h) || m.message != WM_KEYDOWN) return false;
        if (m.hwnd == edQidir && (m.wParam == VK_DOWN || m.wParam == VK_UP || m.wParam == VK_NEXT ||
                                  m.wParam == VK_PRIOR)) {
            SetFocus(lv);
            if (lv_tanlangan(lv) < 0 && !korinadi.empty()) lv_tanla(lv, 0);
            return true;
        }
        if (m.wParam == VK_RETURN && (m.hwnd == lv || m.hwnd == edQidir)) {
            if (m.hwnd == edQidir && !korinadi.empty()) {
                SetFocus(lv);
                if (lv_tanlangan(lv) < 0) lv_tanla(lv, 0);
            } else
                tahrir();
            return true;
        }
        return false;
    }

    void buyruq(int id, int kod, HWND) override {
        if (id == ID_QIDIR && kod == EN_CHANGE) { suzgich(); return; }
        if (kod != BN_CLICKED) return;
        switch (id) {
            case ID_YASHIRIN: yukla(); break;
            case ID_YANGI:
                if (tovar_formasi(hAsosiy, 0)) yukla();
                break;
            case ID_TAHRIR: tahrir(); break;
            case ID_KIRIM: kirim(); break;
            case ID_TUZAT: tuzat(); break;
            case ID_YASHIR: yashir(); break;
            case ID_TARIX: tarix(); break;
        }
    }

    LRESULT xabarnoma(NMHDR* n) override {
        if (n->hwndFrom != lv) return 0;
        if (n->code == LVN_GETDISPINFOW) {
            auto* d = (NMLVDISPINFOW*)n;
            if (!(d->item.mask & LVIF_TEXT) || d->item.iItem >= (int)korinadi.size()) return 0;
            const Tovar& t = tovarlar[korinadi[d->item.iItem]];
            switch (d->item.iSubItem) {
                case 0: buf = t.kod; break;
                case 1: buf = t.nom; break;
                case 2: buf = son(t.qoldiq); break;
                case 3: buf = t.birlik; break;
                case 4: buf = t.narx > 0 ? pul(t.narx) + L" " + t.belgi() : L"—"; break;
                case 5:
                    buf = t.kelish_bor ? pul(t.kelish_narxi) +
                                             (t.kelish_valyutasi == L"dollar" ? L" $" : L" so'm")
                                       : L"";
                    break;
                case 6: buf = !t.faol ? L"yashirilgan" : t.qoldiq <= 0 ? L"tugagan" : L""; break;
            }
            wcsncpy_s(d->item.pszText, d->item.cchTextMax, buf.c_str(), _TRUNCATE);
        } else if (n->code == NM_DBLCLK)
            tahrir();
        else if (n->code == LVN_COLUMNCLICK) {
            int u = ((NMLISTVIEW*)n)->iSubItem;
            tartib_teskari = (u == tartib) ? !tartib_teskari : false;
            tartib = u;
            suzgich();
        }
        return 0;
    }

private:
    HWND hSarlavha, edQidir, chYashirin, hSanoq, lv, b[6];
    std::vector<Tovar> tovarlar;
    std::vector<int> korinadi;
    wstring buf;
    int tartib = 0;
    bool tartib_teskari = false;

    void yukla() {
        tovarlar = tovarlarni_ol(belgilanganmi(chYashirin));
        eskirgan = false;
        suzgich();
    }

    void suzgich() {
        long long oldingi = tanlangan_id();
        auto bl = bolakla(matn_ol(edQidir));
        korinadi.clear();
        for (size_t i = 0; i < tovarlar.size(); ++i)
            if (mos_keladi(tovarlar[i].izlash, bl)) korinadi.push_back((int)i);
        auto& tl = tovarlar;
        int u = tartib;
        bool tes = tartib_teskari;
        std::stable_sort(korinadi.begin(), korinadi.end(), [&](int a, int b) {
            const Tovar &x = tl[tes ? b : a], &y = tl[tes ? a : b];
            switch (u) {
                case 1: return kichik(x.nom) < kichik(y.nom);
                case 2: return x.qoldiq < y.qoldiq;
                case 3: return x.birlik < y.birlik;
                case 4: return x.narx < y.narx;
                case 5: return x.kelish_narxi < y.kelish_narxi;
                case 6: return x.faol > y.faol;
                default: return x.kod < y.kod;
            }
        });
        ListView_SetItemCountEx(lv, (int)korinadi.size(), 0);
        InvalidateRect(lv, nullptr, TRUE);
        int tan = -1;
        for (size_t i = 0; i < korinadi.size(); ++i)
            if (tovarlar[korinadi[i]].id == oldingi) tan = (int)i;
        ListView_SetItemState(lv, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        if (tan >= 0) lv_tanla(lv, tan);
        wstring s = guruhla((long long)korinadi.size()) + L" ta ko'rinmoqda, jami " +
                    guruhla((long long)tovarlar.size()) + L" ta tovar";
        SetWindowTextW(hSanoq, s.c_str());
    }

    long long tanlangan_id() {
        int i = lv_tanlangan(lv);
        if (i < 0 || i >= (int)korinadi.size()) return 0;
        return tovarlar[korinadi[i]].id;
    }

    bool tanlangan(Tovar& t) {
        long long id = tanlangan_id();
        if (!id || !tovar_ol(id, t)) {
            xabar(hAsosiy, L"Ro'yxatdan tovarni tanlang.", true);
            return false;
        }
        return true;
    }

    void tahrir() {
        Tovar t;
        if (tanlangan(t) && tovar_formasi(hAsosiy, t.id)) yukla();
    }

    void kirim() {
        Tovar t;
        if (!tanlangan(t)) return;
        std::vector<wstring> birl = {t.birlik};
        bool ikki = !t.olish_birligi.empty() && t.olish_birligi != t.birlik;
        if (ikki) birl.push_back(t.olish_birligi);
        std::vector<Maydon> ms = {
            Maydon::malumot(t.kod + L"  " + t.nom + L"\nHozir omborda: " + son(t.qoldiq) + L" " +
                            t.birlik + (ikki ? L"   (1 " + t.olish_birligi + L" = " +
                                                   son(t.olish_miqdori) + L" " + t.birlik + L")"
                                             : L"")),
            Maydon::son(L"Keldi (miqdor)"), Maydon::tanla(L"Birligi", birl, ikki ? 1 : 0),
            Maydon::matn(L"Izoh", L"Kirim")};
        bool ok = forma(hAsosiy, L"Kirim — omborga keldi", ms, [&](std::vector<Maydon>& m) {
            double v;
            if (!songa(m[1].qiymat, v) || v <= 0) return wstring(L"Miqdor noldan katta son bo'lsin.");
            return wstring();
        });
        if (!ok) return;
        double v;
        songa(ms[1].qiymat, v);
        bool olishda = ikki && ms[2].tanlangan == 1;
        double sotuvda = olishda ? yaxlit(v * t.olish_miqdori, 3) : v;
        kirim_qil(t.id, sotuvda, v, olishda ? t.olish_birligi : L"", ms[3].qiymat);
        yukla();
    }

    void tuzat() {
        Tovar t;
        if (!tanlangan(t)) return;
        std::vector<Maydon> ms = {
            Maydon::malumot(t.kod + L"  " + t.nom + L"\nBazada: " + son(t.qoldiq) + L" " + t.birlik),
            Maydon::son(L"Haqiqiy qoldiq (" + t.birlik + L")", son(t.qoldiq)),
            Maydon::matn(L"Sababi", L"Sanab chiqildi")};
        bool ok = forma(hAsosiy, L"Qoldiqni tuzatish", ms, [&](std::vector<Maydon>& m) {
            double v;
            if (!songa(m[1].qiymat, v) || v < 0) return wstring(L"Qoldiq manfiy bo'lmagan son bo'lsin.");
            return wstring();
        });
        if (!ok) return;
        double v;
        songa(ms[1].qiymat, v);
        qoldiq_tuzat(t.id, yaxlit(v, 3), ms[2].qiymat);
        yukla();
    }

    void yashir() {
        Tovar t;
        if (!tanlangan(t)) return;
        wstring sor = t.faol ? L"«" + t.nom + L"» yashirilsinmi?\nKassada chiqmaydi, tarixi saqlanadi."
                             : L"«" + t.nom + L"» qayta ochilsinmi?";
        if (!sorash(hAsosiy, sor)) return;
        tovar_faol(t.id, !t.faol);
        if (t.faol && !belgilanganmi(chYashirin))
            xabar(hAsosiy, L"Yashirildi. Ko'rish uchun «Yashirilganlar ham» ni belgilang.");
        yukla();
    }

    void tarix() {
        Tovar t;
        if (!tanlangan(t)) return;
        auto hl = harakatlar(t.id);
        RoyxatSozlama s;
        s.sarlavha = L"Harakatlar: " + t.kod + L" " + t.nom;
        s.qidiruv = false;
        s.ustunlar = {{L"Sana", 140}, {L"Turi", 100}, {L"Miqdor", 220}, {L"Izoh", 380}};
        s.en = 900;
        s.pastki = L"Qoldiq: " + son(t.qoldiq) + L" " + t.birlik + L" · " +
                   std::to_wstring(hl.size()) + L" ta yozuv";
        s.toldir = [&](HWND lv, const wstring&) {
            for (auto& x : hl) lv_qosh(lv, {x.sana, x.tur, x.miqdor, x.izoh});
        };
        royxat_oyna(hAsosiy, s);
    }
};

Ombor ombor;

}  // namespace

Sahifa* ombor_sahifa() { return &ombor; }
void ombor_eskirdi() { ombor.eskirgan = true; }
