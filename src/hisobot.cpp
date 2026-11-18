// HISOBOT: tanlangan kunning tushumi (so'm va $ alohida), qarzlar, to'lovlar va cheklar.
#include "sahifa.h"

namespace {

enum { ID_SARLAVHA = 5, ID_SANA = 10, ID_BUGUN, ID_KECHA, ID_JAMI, ID_RO, ID_CHEK };

class Hisobot : public Sahifa {
public:
    void yarat() override {
        hSarlavha = yorliq(h, L"HISOBOT", ID_SARLAVHA, fJuda);
        dt = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | DTS_LONGDATEFORMAT,
                             0, 0, 10, 10, h, (HMENU)ID_SANA, hIlova, nullptr);
        SendMessageW(dt, WM_SETFONT, (WPARAM)fKatta, TRUE);
        bBugun = tugma(h, L"Bugun", ID_BUGUN, T_KOK);
        bKecha = tugma(h, L"Kecha", ID_KECHA);
        hJami = yorliq(h, L"", ID_JAMI, fKatta);
        lv = royxat(h, ID_RO);
        ustun(lv, 0, L"Chek", 70);
        ustun(lv, 1, L"Vaqt", 70);
        ustun(lv, 2, L"Qatorlar", 80, true);
        ustun(lv, 3, L"Naqd, so'm", 130, true);
        ustun(lv, 4, L"Naqd, $", 90, true);
        ustun(lv, 5, L"Qarzga, so'm", 130, true);
        ustun(lv, 6, L"Qarzga, $", 90, true);
        ustun(lv, 7, L"Qarzdor", 220);
        bChek = tugma(h, L"Chekni ko'rish / chop etish", ID_CHEK);
    }

    void joylash(int w, int hg) override {
        const int M = dp(16), G = dp(8);
        joy(hSarlavha, M, M, dp(240), dp(36));
        int y = M + dp(44);
        joy(dt, M, y, dp(360), dp(40));
        joy(bBugun, M + dp(360) + G, y, dp(120), dp(40));
        joy(bKecha, M + dp(480) + G * 2, y, dp(120), dp(40));
        y += dp(48);
        joy(hJami, M, y, w - M * 2, dp(96));
        y += dp(100);
        int pastki = hg - M - dp(46);
        joy(lv, M, y, w - M * 2, pastki - G - y);
        joy(bChek, M, pastki, dp(320), dp(46));
    }

    void korsatildi() override { yukla(); }

    void buyruq(int id, int kod, HWND) override {
        if (kod != BN_CLICKED) return;
        if (id == ID_BUGUN || id == ID_KECHA) {
            SYSTEMTIME t;
            GetLocalTime(&t);
            if (id == ID_KECHA) {
                FILETIME f;
                SystemTimeToFileTime(&t, &f);
                ULARGE_INTEGER x{{f.dwLowDateTime, f.dwHighDateTime}};
                x.QuadPart -= 864000000000ULL;
                f = {x.LowPart, x.HighPart};
                FileTimeToSystemTime(&f, &t);
            }
            DateTime_SetSystemtime(dt, GDT_VALID, &t);
            yukla();
        }
        if (id == ID_CHEK) chek();
    }

    LRESULT xabarnoma(NMHDR* n) override {
        if (n->hwndFrom == dt && n->code == DTN_DATETIMECHANGE) yukla();
        if (n->hwndFrom == lv && n->code == NM_DBLCLK) chek();
        return 0;
    }

private:
    HWND hSarlavha, dt, bBugun, bKecha, hJami, lv, bChek;

    void yukla() {
        SYSTEMTIME kun;
        DateTime_GetSystemtime(dt, &kun);
        std::vector<ChekQisqa> cl;
        KunHisobi k = kun_hisobi(kun, cl);
        wstring s = L"Naqd tushum:  " + pul(k.naqd) + L" so'm   ·   " + pul(k.naqd_d) + L" $     (" +
                    std::to_wstring(k.cheklar) + L" ta chek)\n" +
                    L"Qarzga yozildi:  " + pul(k.qarz) + L" so'm   ·   " + pul(k.qarz_d) + L" $\n" +
                    L"Qarz to'lovlari:  " + pul(k.tolov) + L" so'm   ·   " + pul(k.tolov_d) + L" $";
        SetWindowTextW(hJami, s.c_str());
        SendMessageW(lv, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(lv);
        for (auto& c : cl) {
            wstring vaqt = mahalliy_vaqt(c.sana_utc);
            lv_qosh(lv, {L"#" + std::to_wstring(c.id), vaqt.size() > 11 ? vaqt.substr(11) : vaqt,
                         std::to_wstring(c.qatorlar), pul(c.naqd), c.naqd_d ? pul(c.naqd_d) : L"",
                         c.qarz ? pul(c.qarz) : L"", c.qarz_d ? pul(c.qarz_d) : L"", c.qarzdor},
                    (LPARAM)c.id);
        }
        SendMessageW(lv, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(lv, nullptr, TRUE);
    }

    void chek() {
        int i = lv_tanlangan(lv);
        if (i < 0) {
            xabar(hAsosiy, L"Ro'yxatdan chekni tanlang.", true);
            return;
        }
        chek_oynasi(hAsosiy, (long long)lv_param(lv, i));
    }
};

Hisobot hisobot;

}  // namespace

Sahifa* hisobot_sahifa() { return &hisobot; }
