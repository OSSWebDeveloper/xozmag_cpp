// QARZ DAFTARI: qarzdorlar, ularning qarz/to'lov tarixi, to'lov qabul qilish.
// So'm va dollar alohida yuritiladi — hech qayerda qo'shilmaydi.
#include "sahifa.h"

namespace {

enum { ID_SARLAVHA = 5, ID_QIDIR = 10, ID_RO, ID_TARIX, ID_JAMI, ID_KARTA, ID_YANGI = 20,
       ID_TAHRIR, ID_TOLOV, ID_QARZ_YOZ, ID_HUDUD };

class Qarz : public Sahifa {
public:
    void yarat() override {
        hSarlavha = yorliq(h, L"QARZ DAFTARI", ID_SARLAVHA, fJuda);
        edQidir = maydon(h, ID_QIDIR, fKatta, 0, L"Familiya, ism, telefon yoki hudud");
        lv = royxat(h, ID_RO);
        ustun(lv, 0, L"Familiya Ism", 165);
        ustun(lv, 1, L"Telefon", 105);
        ustun(lv, 2, L"Hudud", 85);
        ustun(lv, 3, L"Qarz, so'm", 95, true);
        ustun(lv, 4, L"Qarz, $", 60, true);
        hJami = yorliq(h, L"", ID_JAMI, fQalin);
        hKarta = yorliq(h, L"Qarzdorni tanlang", ID_KARTA, fKatta);
        tv = royxat(h, ID_TARIX);
        ustun(tv, 0, L"Sana", 112);
        ustun(tv, 1, L"Hujjat", 130);
        ustun(tv, 2, L"Tafsilot", 215);
        ustun(tv, 3, L"So'm", 90, true);
        ustun(tv, 4, L"$", 55, true);
        const wchar_t* nomlar[] = {L"Yangi qarzdor", L"Tahrirlash", L"To'lov qabul qilish",
                                   L"Qarz yozish (kassada)", L"Hududlar"};
        TugmaRang r[] = {T_KOK, T_ODDIY, T_YASHIL, T_QIZIL, T_ODDIY};
        for (int i = 0; i < 5; ++i) b[i] = tugma(h, nomlar[i], ID_YANGI + i, r[i]);
    }

    void joylash(int w, int hg) override {
        const int M = dp(16), G = dp(8);
        int chap = std::max(dp(480), (w - M * 3) * 45 / 100);
        int ong = w - M * 3 - chap;
        joy(hSarlavha, M, M, chap, dp(36));
        int y = M + dp(44);
        joy(edQidir, M, y, chap, dp(40));
        joy(hKarta, M * 2 + chap, M, ong, dp(92));
        int pastki = hg - M - dp(46);
        joy(lv, M, y + dp(48), chap, pastki - G - dp(28) - (y + dp(48)));
        joy(hJami, M, pastki - G - dp(26), chap, dp(24));
        joy(tv, M * 2 + chap, y + dp(48), ong, pastki - G - (y + dp(48)));
        int bw = (w - M * 2 - G * 4) / 5;
        for (int i = 0; i < 5; ++i) joy(b[i], M + i * (bw + G), pastki, bw, dp(46));
    }

    void korsatildi() override {
        yukla();
        SetFocus(edQidir);
    }

    bool kalit(MSG& m) override {
        if (!IsWindowVisible(h) || m.message != WM_KEYDOWN) return false;
        if (m.hwnd == edQidir && (m.wParam == VK_DOWN || m.wParam == VK_RETURN)) {
            SetFocus(lv);
            if (lv_tanlangan(lv) < 0 && ListView_GetItemCount(lv)) lv_tanla(lv, 0);
            return true;
        }
        return false;
    }

    void buyruq(int id, int kod, HWND) override {
        if (id == ID_QIDIR && kod == EN_CHANGE) { yukla(); return; }
        if (kod != BN_CLICKED) return;
        long long q = tanlangan();
        switch (id) {
            case ID_YANGI:
                if (long long n = qarzdor_formasi(hAsosiy, 0)) { yukla(); tanla(n); }
                break;
            case ID_TAHRIR:
                if (!q) { xabar(hAsosiy, L"Qarzdorni tanlang.", true); break; }
                if (qarzdor_formasi(hAsosiy, q)) { yukla(); tanla(q); }
                break;
            case ID_TOLOV: tolov(q); break;
            case ID_QARZ_YOZ:
                if (!q) { xabar(hAsosiy, L"Qarzdorni tanlang.", true); break; }
                kassa_qarzdor_qoy(q);
                break;
            case ID_HUDUD:
                hududlar_oynasi(hAsosiy);
                yukla();
                break;
        }
    }

    LRESULT xabarnoma(NMHDR* n) override {
        if (n->hwndFrom == lv && n->code == LVN_ITEMCHANGED) {
            auto* x = (NMLISTVIEW*)n;
            if ((x->uNewState ^ x->uOldState) & LVIS_SELECTED) karta();
        }
        if (n->hwndFrom == lv && n->code == NM_DBLCLK) tolov(tanlangan());
        return 0;
    }

private:
    HWND hSarlavha, edQidir, lv, hJami, hKarta, tv, b[5];
    long long korsatilgan = -1;

    long long tanlangan() {
        int i = lv_tanlangan(lv);
        return i < 0 ? 0 : (long long)lv_param(lv, i);
    }

    void tanla(long long id) {
        for (int i = 0; i < ListView_GetItemCount(lv); ++i)
            if ((long long)lv_param(lv, i) == id) lv_tanla(lv, i);
    }

    void yukla() {
        long long oldingi = tanlangan();
        SendMessageW(lv, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(lv);
        double js = 0, jd = 0;
        int qarzli = 0;
        auto ql = qarzdorlar(matn_ol(edQidir));
        for (auto& d : ql) {
            lv_qosh(lv, {d.toliq(), d.telefon, d.hudud, pul(d.balans), pul(d.balans_d)}, (LPARAM)d.id);
            js += d.balans;
            jd += d.balans_d;
            if (d.balans > 0 || d.balans_d > 0) ++qarzli;
        }
        SendMessageW(lv, WM_SETREDRAW, TRUE, 0);
        wstring s = std::to_wstring(ql.size()) + L" ta qarzdor (" + std::to_wstring(qarzli) +
                    L" tasida qarz) · jami: " + pul(js) + L" so'm, " + pul(jd) + L" $";
        SetWindowTextW(hJami, s.c_str());
        korsatilgan = -1;
        if (oldingi) tanla(oldingi);
        karta();
    }

    void karta() {
        long long id = tanlangan();
        if (id == korsatilgan) return;
        korsatilgan = id;
        SendMessageW(tv, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(tv);
        Qarzdor q;
        if (!id || !qarzdor_ol(id, q)) {
            SetWindowTextW(hKarta, L"Qarzdorni tanlang — tarixi shu yerda chiqadi.");
        } else {
            wstring s = q.toliq() + L"   " + q.telefon + L"\n" + q.hudud +
                        (q.izoh.empty() ? L"" : L" · " + q.izoh) + L"\nQarzi: " + pul(q.balans) +
                        L" so'm,  " + pul(q.balans_d) + L" $";
            SetWindowTextW(hKarta, s.c_str());
            for (auto& t : qarzdor_tarixi(id))
                lv_qosh(tv, {mahalliy_vaqt(t.sana_utc), t.hujjat, t.tafsilot,
                             t.som ? pul(t.som) : L"", t.dollar ? pul(t.dollar) : L""});
        }
        SendMessageW(tv, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(tv, nullptr, TRUE);
    }

    void tolov(long long id) {
        Qarzdor q;
        if (!id || !qarzdor_ol(id, q)) {
            xabar(hAsosiy, L"Qarzdorni tanlang.", true);
            return;
        }
        std::vector<Maydon> ms = {
            Maydon::malumot(q.toliq() + L"\nQarzi: " + pul(q.balans) + L" so'm,  " +
                            pul(q.balans_d) + L" $"),
            Maydon::son(L"Summa"),
            Maydon::tanla(L"Valyuta", {L"so'm", L"dollar"}, q.balans <= 0 && q.balans_d > 0 ? 1 : 0),
            Maydon::matn(L"Izoh")};
        bool ok = forma(hAsosiy, L"To'lov qabul qilish", ms, [&](std::vector<Maydon>& m) {
            double v;
            if (!songa(m[1].qiymat, v) || v <= 0) return wstring(L"Summa noldan katta son bo'lsin.");
            double qoldi = m[2].tanlangan == 1 ? q.balans_d : q.balans;
            if (v > qoldi + 0.001 &&
                !sorash(faol_oyna(), L"To'lov qarzdan ko'p (" + pul(qoldi) +
                                         L"). Ortiqchasi oldindan to'lov bo'lib qoladi. Davom etilsinmi?"))
                return wstring(L"To'lov summasini tuzating.");
            return wstring();
        });
        if (!ok) return;
        double v;
        songa(ms[1].qiymat, v);
        tolov_qosh(id, yaxlit(v, 2), ms[2].tanlangan == 1 ? L"dollar" : L"som", ms[3].qiymat);
        yukla();
        tanla(id);
    }
};

Qarz qarz;

}  // namespace

Sahifa* qarz_sahifa() { return &qarz; }
