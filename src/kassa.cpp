// KASSA: supermarket kassasi kabi — kod/skaner, chek, raqamlar klaviaturasi o'ng tarafda.
//
// Ombor faqat chek yakunlanganda kamayadi. «Jami» — chekning to'liq summasi,
// savdolashib o'zgartirilishi mumkin. «To'landi» bo'sh bo'lsa — to'liq to'landi;
// kam bo'lsa qolgani tanlangan qarzdorning daftariga yoziladi.
#include "sahifa.h"

namespace {

enum {
    ID_SARLAVHA = 5, ID_KOD = 10, ID_MIQDOR, ID_QOSH, ID_QIDIR, ID_XABAR, ID_RO, ID_MIQ_OZG,
    ID_OCHIR, ID_TOZALA, ID_HISOB = 20, ID_QARZDOR, ID_QARZDOR_T, ID_JAMI, ID_JAMI_D, ID_TOL,
    ID_TOL_D, ID_QARZ, ID_CHOP, ID_YAKUN, ID_YORLIQ = 30, ID_TUGMA = 40
};

const wchar_t* TUGMA_MATNI[16] = {L"7", L"8", L"9", L"⌫", L"4", L"5",  L"6",   L"C",
                                  L"1", L"2", L"3", L",", L"0", L"00", L"000", L"↵"};

class Kassa : public Sahifa {
public:
    std::vector<SavatQator> savat;
    long long qarzdor = 0;
    wstring qarzdor_nomi;

    void yarat() override {
        hSarlavha = yorliq(h, L"KASSA", ID_SARLAVHA, fJuda);
        yKod = yorliq(h, L"Kod / skaner", ID_YORLIQ);
        yMiqdor = yorliq(h, L"Miqdor", ID_YORLIQ);
        edKod = maydon(h, ID_KOD, fKatta, 0, L"0027");
        edMiqdor = maydon(h, ID_MIQDOR, fKatta, ES_RIGHT, L"1");
        bQosh = tugma(h, L"Qo'shish (Enter)", ID_QOSH, T_KOK);
        bQidir = tugma(h, L"Tovar qidirish (F2)", ID_QIDIR);
        hXabar = yorliq(h, L"Kodni yozing yoki skanerlang.", ID_XABAR, fQalin);
        lv = royxat(h, ID_RO);
        ustun(lv, 0, L"№", 40);
        ustun(lv, 1, L"Kod", 60);
        ustun(lv, 2, L"Tovar", 250);
        ustun(lv, 3, L"Miqdor", 80, true);
        ustun(lv, 4, L"Birlik", 60);
        ustun(lv, 5, L"Narx", 100, true);
        ustun(lv, 6, L"Summa", 120, true);
        bMiq = tugma(h, L"Miqdorni o'zgartirish", ID_MIQ_OZG);
        bOchir = tugma(h, L"Qatorni o'chirish (Del)", ID_OCHIR);
        bTozala = tugma(h, L"Chekni tozalash", ID_TOZALA, T_QIZIL);

        hHisob = yorliq(h, L"", ID_HISOB, fKatta);
        hQarzdor = yorliq(h, L"Qarzdor tanlanmagan", ID_QARZDOR, fQalin);
        bQarzdor = tugma(h, L"Qarzdor tanlash (F4)", ID_QARZDOR_T);
        yJami = yorliq(h, L"JAMI, so'm", ID_YORLIQ, fQalin);
        yJamiD = yorliq(h, L"JAMI, $", ID_YORLIQ, fQalin);
        yTol = yorliq(h, L"To'landi, so'm", ID_YORLIQ);
        yTolD = yorliq(h, L"To'landi, $", ID_YORLIQ);
        edJami = maydon(h, ID_JAMI, fKatta, ES_RIGHT);
        edJamiD = maydon(h, ID_JAMI_D, fKatta, ES_RIGHT);
        edTol = maydon(h, ID_TOL, fKatta, ES_RIGHT, L"to'liq");
        edTolD = maydon(h, ID_TOL_D, fKatta, ES_RIGHT, L"to'liq");
        hQarz = yorliq(h, L"", ID_QARZ, fQalin);
        for (int i = 0; i < 16; ++i)
            klav[i] = tugma(h, TUGMA_MATNI[i], ID_TUGMA + i, i == 15 ? T_KOK : T_ODDIY, fJuda);
        chChop = belgi(h, L"Chek chop etilsin", ID_CHOP);
        belgila(chChop, sozlama("chek_chop", L"0") == L"1");
        bYakun = tugma(h, L"YAKUNLASH (F12)", ID_YAKUN, T_YASHIL, fJuda);
        oxirgi = edKod;
        yangila();
    }

    void joylash(int w, int hg) override {
        const int M = dp(16), G = dp(8), R = dp(380);
        int L = w - R - M * 3;
        int y = M;
        joy(hSarlavha, M, y, L, dp(36));
        y += dp(42);
        joy(yKod, M, y, dp(240), dp(20));
        joy(yMiqdor, M + dp(240) + G, y, dp(120), dp(20));
        y += dp(22);
        int x = M;
        joy(edKod, x, y, dp(240), dp(44)); x += dp(240) + G;
        joy(edMiqdor, x, y, dp(120), dp(44)); x += dp(120) + G;
        joy(bQosh, x, y, dp(150), dp(44)); x += dp(150) + G;
        joy(bQidir, x, y, std::max(dp(150), M + L - x), dp(44));
        y += dp(44) + G;
        joy(hXabar, M, y, L, dp(26));
        y += dp(30);
        int pastki = hg - M - dp(46);
        joy(lv, M, y, L, pastki - G - y);
        int bw = (L - G * 2) / 3;
        joy(bMiq, M, pastki, bw, dp(46));
        joy(bOchir, M + bw + G, pastki, bw, dp(46));
        joy(bTozala, M + (bw + G) * 2, pastki, bw, dp(46));

        // o'ng panel
        int px = w - M - R;
        y = M;
        joy(hHisob, px, y, R, dp(64));
        y += dp(66);
        joy(hQarzdor, px, y, R, dp(24));
        y += dp(26);
        joy(bQarzdor, px, y, R, dp(38));
        y += dp(38) + G;
        int yarim = (R - G) / 2;
        joy(yJami, px, y, yarim, dp(20));
        joy(yJamiD, px + yarim + G, y, yarim, dp(20));
        y += dp(21);
        joy(edJami, px, y, yarim, dp(40));
        joy(edJamiD, px + yarim + G, y, yarim, dp(40));
        y += dp(40) + dp(4);
        joy(yTol, px, y, yarim, dp(20));
        joy(yTolD, px + yarim + G, y, yarim, dp(20));
        y += dp(21);
        joy(edTol, px, y, yarim, dp(40));
        joy(edTolD, px + yarim + G, y, yarim, dp(40));
        y += dp(40) + dp(4);
        joy(hQarz, px, y, R, dp(42));
        y += dp(44);
        int kw = (R - dp(6) * 3) / 4, kh = dp(48);
        int qolgan = hg - M - y - dp(26) - G - dp(60);
        if (qolgan < kh * 4 + dp(18)) kh = std::max(dp(34), (qolgan - dp(18)) / 4);
        for (int i = 0; i < 16; ++i)
            joy(klav[i], px + (i % 4) * (kw + dp(6)), y + (i / 4) * (kh + dp(6)), kw, kh);
        y += kh * 4 + dp(18) + dp(4);
        joy(chChop, px, y, R, dp(24));
        y += dp(24) + G;
        joy(bYakun, px, y, R, std::max(dp(56), hg - M - y));
    }

    void korsatildi() override {
        matn_qoy(hSarlavha, qarzdor ? L"KASSA — qarzga yozish" : L"KASSA");
        qarzdor_yangila();
        SetFocus(edKod);
    }

    bool kalit(MSG& m) override {
        if (!IsWindowVisible(h)) return false;
        if (m.message == WM_KEYDOWN) {
            switch (m.wParam) {
                case VK_F2: qidir(); return true;
                case VK_F4: qarzdor_tugmasi(); return true;
                case VK_F12: yakunla(); return true;
                case VK_ESCAPE:
                    SetFocus(edKod);
                    SendMessageW(edKod, EM_SETSEL, 0, -1);
                    return true;
                case VK_RETURN:
                    if (m.hwnd == edKod || m.hwnd == edMiqdor) { qosh(); return true; }
                    if (m.hwnd == edJami || m.hwnd == edJamiD || m.hwnd == edTol || m.hwnd == edTolD) {
                        SetFocus(bYakun);
                        return true;
                    }
                    if (m.hwnd == lv) { miqdor_ozgartir(); return true; }
                    break;
                case VK_DELETE:
                    if (m.hwnd == lv) { ochir(); return true; }
                    break;
            }
        }
        // Skaner raqamlarni yozadi: fokus maydonda bo'lmasa ham kod maydoniga yo'naltiramiz.
        if (m.message == WM_CHAR && m.wParam >= L'0' && m.wParam <= L'9' && !maydonmi(m.hwnd)) {
            SetFocus(edKod);
            SendMessageW(edKod, WM_CHAR, m.wParam, m.lParam);
            return true;
        }
        return false;
    }

    void buyruq(int id, int kod, HWND) override {
        if (kod == EN_SETFOCUS && maydonmi(GetDlgItem(h, id))) oxirgi = GetDlgItem(h, id);
        if (kod == EN_CHANGE && !dasturiy) {
            if (id == ID_JAMI) jami_qolda = true;
            if (id == ID_JAMI_D) jamid_qolda = true;
            if (id >= ID_JAMI && id <= ID_TOL_D) qarz_yangila();
            return;
        }
        if (kod != BN_CLICKED) return;
        switch (id) {
            case ID_QOSH: qosh(); break;
            case ID_QIDIR: qidir(); break;
            case ID_MIQ_OZG: miqdor_ozgartir(); break;
            case ID_OCHIR: ochir(); break;
            case ID_TOZALA:
                if (savat.empty() || sorash(hAsosiy, L"Chekdagi hamma tovar olib tashlansinmi?"))
                    yangi_chek();
                break;
            case ID_QARZDOR_T: qarzdor_tugmasi(); break;
            case ID_YAKUN: yakunla(); break;
            case ID_CHOP: sozlama_yoz("chek_chop", belgilanganmi(chChop) ? L"1" : L"0"); break;
            default:
                if (id >= ID_TUGMA && id < ID_TUGMA + 16) klaviatura(id - ID_TUGMA);
        }
    }

    LRESULT xabarnoma(NMHDR* n) override {
        if (n->hwndFrom == lv && n->code == NM_DBLCLK) miqdor_ozgartir();
        return 0;
    }

    HBRUSH rang(HDC dc, HWND el) override {
        if (el == hXabar) SetTextColor(dc, xato_holat ? RANG_QIZIL : RANG_YASHIL);
        else if (el == hQarz) SetTextColor(dc, qarz_bor ? RANG_QIZIL : RANG_YASHIL);
        else if (el == hQarzdor) SetTextColor(dc, qarzdor ? RANG_QIZIL : RGB(110, 110, 110));
        else if (el == hSarlavha) SetTextColor(dc, qarzdor ? RANG_QIZIL : RGB(20, 20, 20));
        else return nullptr;
        SetBkColor(dc, RANG_FON);
        return oqFon;
    }

    void qarzdorni_qoy(long long id) {
        qarzdor = id;
        qarzdor_yangila();
        matn_qoy(hSarlavha, qarzdor ? L"KASSA — qarzga yozish" : L"KASSA");
        InvalidateRect(hSarlavha, nullptr, TRUE);
        qarz_yangila();
    }

private:
    HWND hSarlavha, yKod, yMiqdor, edKod, edMiqdor, bQosh, bQidir, hXabar, lv, bMiq, bOchir, bTozala;
    HWND hHisob, hQarzdor, bQarzdor, yJami, yJamiD, yTol, yTolD, edJami, edJamiD, edTol, edTolD;
    HWND hQarz, klav[16], chChop, bYakun, oxirgi;
    bool jami_qolda = false, jamid_qolda = false, dasturiy = false, xato_holat = false;
    bool qarz_bor = false;

    bool maydonmi(HWND e) {
        return e && (e == edKod || e == edMiqdor || e == edJami || e == edJamiD || e == edTol ||
                     e == edTolD);
    }

    void holat(const wstring& matn, bool xato) {
        xato_holat = xato;
        SetWindowTextW(hXabar, matn.c_str());
        InvalidateRect(hXabar, nullptr, TRUE);
        if (xato) MessageBeep(MB_ICONEXCLAMATION);
    }

    void hisob(double& som, double& dollar, bool& dollar_bor) {
        som = dollar = 0;
        dollar_bor = false;
        for (auto& q : savat) {
            if (q.t.dollarmi()) {
                dollar += q.summa();
                dollar_bor = true;
            } else
                som += q.summa();
        }
        som = yaxlit(som, 2);
        dollar = yaxlit(dollar, 2);
    }

    void yangila(int tanlash = -1) {
        SendMessageW(lv, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(lv);
        for (size_t i = 0; i < savat.size(); ++i) {
            auto& q = savat[i];
            wstring v = q.t.dollarmi() ? L" $" : L"";
            lv_qosh(lv, {std::to_wstring(i + 1), q.t.kod, q.t.nom, son(q.miqdor), q.t.birlik,
                         q.t.narx > 0 ? pul(q.t.narx) + v : L"—",
                         q.t.narx > 0 ? pul(q.summa()) + v : L"—"},
                    (LPARAM)i);
        }
        SendMessageW(lv, WM_SETREDRAW, TRUE, 0);
        if (tanlash >= 0) lv_tanla(lv, tanlash);

        double som, dollar;
        bool dbor;
        hisob(som, dollar, dbor);
        wstring hs = L"Hisob: " + pul(som) + L" so'm";
        if (dbor) hs += L"\n           " + pul(dollar) + L" $";
        else hs += L"\n" + std::to_wstring(savat.size()) + L" qator";
        SetWindowTextW(hHisob, hs.c_str());

        dasturiy = true;
        if (!jami_qolda) matn_qoy(edJami, savat.empty() ? L"" : pul(som));
        if (!jamid_qolda) matn_qoy(edJamiD, dbor ? pul(dollar) : L"");
        dasturiy = false;
        EnableWindow(edJamiD, dbor || jamid_qolda);
        EnableWindow(edTolD, dbor || jamid_qolda);
        qarz_yangila();
    }

    void qarzdor_yangila() {
        qarzdor_nomi.clear();
        Qarzdor q;
        if (qarzdor && qarzdor_ol(qarzdor, q)) {
            qarzdor_nomi = q.toliq();
            wstring s = L"Qarzdor: " + qarzdor_nomi;
            if (q.balans > 0 || q.balans_d > 0)
                s += L"  (qarzi " + pul(q.balans) + L" so'm" +
                     (q.balans_d > 0 ? L", " + pul(q.balans_d) + L" $" : L"") + L")";
            SetWindowTextW(hQarzdor, s.c_str());
            SetWindowTextW(bQarzdor, L"Qarzdorni olib tashlash (F4)");
        } else {
            qarzdor = 0;
            SetWindowTextW(hQarzdor, L"Qarzdor tanlanmagan — naqd sotuv");
            SetWindowTextW(bQarzdor, L"Qarzdor tanlash (F4)");
        }
        InvalidateRect(hQarzdor, nullptr, TRUE);
        InvalidateRect(bQarzdor, nullptr, TRUE);
    }

    // Maydondan son: bo'sh — `bosh` qiymati. Noto'g'ri bo'lsa false.
    bool maydon_soni(HWND e, double bosh, double& v) {
        wstring s = tozala(matn_ol(e));
        if (s.empty()) {
            v = bosh;
            return true;
        }
        return songa(s, v) && v >= 0;
    }

    void qarz_yangila() {
        double j = 0, jd = 0, t = 0, td = 0;
        bool ok = maydon_soni(edJami, 0, j) && maydon_soni(edJamiD, 0, jd);
        ok = ok && maydon_soni(edTol, j, t) && maydon_soni(edTolD, jd, td);
        wstring s;
        qarz_bor = false;
        if (!ok)
            s = L"Summada xato bor — faqat raqam yozing.", qarz_bor = true;
        else if (savat.empty())
            s = L"";
        else {
            double q = yaxlit(j - t, 2), qd = yaxlit(jd - td, 2);
            if (q < 0 || qd < 0) {
                s = L"To'langan pul chek summasidan ko'p!";
                qarz_bor = true;
            } else if (q > 0 || qd > 0) {
                qarz_bor = true;
                wstring sm = q > 0 ? pul(q) + L" so'm" : L"";
                if (qd > 0) sm += (sm.empty() ? L"" : L", ") + pul(qd) + L" $";
                s = qarzdor ? L"Qarzga: " + sm + L"\n→ " + qarzdor_nomi
                            : L"Qarzga " + sm + L" qoladi —\nqarzdorni tanlang (F4)";
            } else
                s = L"To'liq to'lanadi.";
        }
        SetWindowTextW(hQarz, s.c_str());
        InvalidateRect(hQarz, nullptr, TRUE);
    }

    void qosh() {
        wstring kod = tozala(matn_ol(edKod));
        if (kod.empty()) {
            holat(L"Kodni yozing yoki tovar qidiring (F2).", true);
            SetFocus(edKod);
            return;
        }
        Tovar t;
        double km;
        wstring x;
        if (!kod_bilan_top(kod, t, km, x)) {
            holat(x, true);
            SetFocus(edKod);
            SendMessageW(edKod, EM_SETSEL, 0, -1);
            return;
        }
        qoshish(t, km);
    }

    void qoshish(const Tovar& t, double kod_miqdori) {
        double miq = 1;
        wstring mq = tozala(matn_ol(edMiqdor));
        if (kod_miqdori > 0)
            miq = kod_miqdori;
        else if (!mq.empty() && (!songa(mq, miq) || miq <= 0)) {
            holat(L"Miqdor noto'g'ri: «" + mq + L"».", true);
            SetFocus(edMiqdor);
            SendMessageW(edMiqdor, EM_SETSEL, 0, -1);
            return;
        }
        miq = yaxlit(miq, 3);
        int idx = -1;
        for (size_t i = 0; i < savat.size(); ++i)
            if (savat[i].t.id == t.id) idx = (int)i;
        double yangi = (idx >= 0 ? savat[idx].miqdor : 0) + miq;
        if (yangi > t.qoldiq + 1e-9) {
            holat(L"Omborda yetarli emas. " + t.kod + L" " + t.nom + L": " + son(t.qoldiq) + L" " +
                      t.birlik + L" qoldi.",
                  true);
            SetFocus(edKod);
            SendMessageW(edKod, EM_SETSEL, 0, -1);
            return;
        }
        if (idx >= 0) {
            savat[idx].t = t;
            savat[idx].miqdor = yangi;
        } else {
            savat.push_back({t, miq});
            idx = (int)savat.size() - 1;
        }
        SetWindowTextW(edKod, L"");
        SetWindowTextW(edMiqdor, L"");
        yangila(idx);
        holat(L"Qo'shildi: " + t.kod + L" " + t.nom + L" — " + son(yangi) + L" " + t.birlik, false);
        SetFocus(edKod);
    }

    void qidir() {
        long long id = tovar_tanla(hAsosiy);
        if (!id) {
            SetFocus(edKod);
            return;
        }
        Tovar t;
        if (tovar_ol(id, t)) qoshish(t, -1);
    }

    void ochir() {
        int i = lv_tanlangan(lv);
        if (i < 0 || i >= (int)savat.size()) {
            holat(L"O'chirish uchun chekdagi qatorni tanlang.", true);
            return;
        }
        wstring nom = savat[i].t.nom;
        savat.erase(savat.begin() + i);
        yangila(std::min(i, (int)savat.size() - 1));
        holat(L"Olib tashlandi: " + nom, false);
        SetFocus(lv);
    }

    void miqdor_ozgartir() {
        int i = lv_tanlangan(lv);
        if (i < 0 || i >= (int)savat.size()) {
            holat(L"Chekdagi qatorni tanlang.", true);
            return;
        }
        Tovar t;
        if (!tovar_ol(savat[i].t.id, t)) return;
        std::vector<Maydon> ms = {
            Maydon::malumot(t.kod + L"  " + t.nom + L"\nOmborda: " + son(t.qoldiq) + L" " + t.birlik),
            Maydon::son(L"Miqdor (" + t.birlik + L")", son(savat[i].miqdor))};
        bool ok = forma(hAsosiy, L"Miqdorni o'zgartirish", ms, [&](std::vector<Maydon>& m) {
            double v;
            if (!songa(m[1].qiymat, v) || v <= 0) return wstring(L"Miqdor noldan katta son bo'lsin.");
            if (v > t.qoldiq + 1e-9)
                return L"Omborda yetarli emas: " + son(t.qoldiq) + L" " + t.birlik + L" qoldi.";
            return wstring();
        });
        if (ok) {
            double v;
            songa(ms[1].qiymat, v);
            savat[i].t = t;
            savat[i].miqdor = yaxlit(v, 3);
            yangila(i);
        }
        SetFocus(lv);
    }

    void qarzdor_tugmasi() {
        if (qarzdor) {
            qarzdorni_qoy(0);
        } else {
            long long id = qarzdor_tanla(hAsosiy);
            if (id) qarzdorni_qoy(id);
        }
        SetFocus(edKod);
    }

    void klaviatura(int i) {
        HWND e = oxirgi ? oxirgi : edKod;
        if (!IsWindowEnabled(e)) e = edKod;
        SetFocus(e);
        wstring t = TUGMA_MATNI[i];
        if (t == L"⌫") {
            DWORD a = 0, b = 0;
            SendMessageW(e, EM_GETSEL, (WPARAM)&a, (LPARAM)&b);
            SendMessageW(e, WM_CHAR, VK_BACK, 0);
        } else if (t == L"C") {
            SetWindowTextW(e, L"");
        } else if (t == L"↵") {
            if (e == edKod || e == edMiqdor) qosh();
            else SetFocus(bYakun);
        } else
            SendMessageW(e, EM_REPLACESEL, TRUE, (LPARAM)t.c_str());
    }

    void yangi_chek() {
        savat.clear();
        jami_qolda = jamid_qolda = false;
        dasturiy = true;
        SetWindowTextW(edJami, L"");
        SetWindowTextW(edJamiD, L"");
        SetWindowTextW(edTol, L"");
        SetWindowTextW(edTolD, L"");
        SetWindowTextW(edKod, L"");
        SetWindowTextW(edMiqdor, L"");
        dasturiy = false;
        qarzdorni_qoy(0);
        yangila();
        SetFocus(edKod);
    }

    void yakunla() {
        if (savat.empty()) {
            holat(L"Chek bo'sh — avval tovar qo'shing.", true);
            SetFocus(edKod);
            return;
        }
        double j, jd, t, td;
        if (!maydon_soni(edJami, 0, j) || !maydon_soni(edJamiD, 0, jd) ||
            !maydon_soni(edTol, j, t) || !maydon_soni(edTolD, jd, td)) {
            xabar(hAsosiy, L"Summa maydonlarida xato bor — faqat raqam yozing.", true);
            return;
        }
        ChekNatija n;
        wstring x;
        if (!sotuv_yakunla(savat, j, jd, t, td, dollar_kursi(), qarzdor, n, x)) {
            xabar(hAsosiy, x, true);
            return;
        }
        ombor_eskirdi();
        wstring msg = L"Chek #" + std::to_wstring(n.sotuv_id) + L" yakunlandi — naqd " +
                      pul(t) + L" so'm" + (td > 0 ? L", " + pul(td) + L" $" : L"");
        if (n.qarz_id) {
            double q = yaxlit(j - t, 2), qd = yaxlit(jd - td, 2);
            msg += L". Qarzga: " + (q > 0 ? pul(q) + L" so'm " : L"") +
                   (qd > 0 ? pul(qd) + L" $ " : L"") + L"→ " + qarzdor_nomi;
        }
        bool chop = belgilanganmi(chChop);
        yangi_chek();
        holat(msg, false);
        if (chop) {
            Chek c;
            wstring cx;
            if (chek_ol(n.sotuv_id, c) && !chek_chop(c, cx))
                xabar(hAsosiy, L"Chek saqlandi, lekin chop etilmadi:\n" + cx +
                                   L"\n\nHisobot bo'limidan qayta chop etish mumkin.",
                      true);
        }
    }
};

Kassa kassa;

}  // namespace

Sahifa* kassa_sahifa() { return &kassa; }
bool kassa_savat_bormi() { return !kassa.savat.empty(); }

void kassa_qarzdor_qoy(long long id) {
    kassa.qarzdorni_qoy(id);
    bolimga_ot(B_KASSA);
}
