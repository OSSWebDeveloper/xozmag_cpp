// Oddiy «immediate mode» interfeys: har kadrda sahifa o'zini qayta chizadi,
// tugmalar bosilganini chizish paytida qaytaradi. Matn kiritish — haqiqiy
// Windows EDIT oynachalari (ular sayt maydonlari ostiga joylashtiriladi).
#pragma once
#include <functional>
#include <map>
#include <set>
#include <vector>

#include "gfx.h"

enum Uslub { ODDIY, ASOSIY, YASHIL, TINCH, XATO_U };

struct MaydonSoz {
    float olcham = 19;      // --katta-matn
    int ogirlik = 400;
    bool ong = false;       // raqam maydoni: o'ngga tekis
    bool parol = false;
    const wchar_t* ishora = nullptr;
    bool kop_qator = false;
    Rang fon;               // bo'sh — panel-2
};

enum XabarTur { X_OK, X_XATO, X_OGOH, X_INFO };

class Ui {
public:
    HWND h = nullptr;
    float W = 0, H = 0;  // CSS px

    void boshla(HWND oyna);
    void kadr_boshla();
    void kadr_tugat();

    // ---- kirish hodisalari (oyna jarayonidan) ----
    void sichqon(float x, float y) { mx = x; my = y; }
    void bosish(float x, float y) { bos = true; bx = x; by = y; ushlab = true; }
    void qoyish(float x, float y) { qoy = true; qx = x; qy = y; ushlab = false; }
    void gildirak_ayl(float d) { gildirak += d; }
    void tugmacha_qosh(UINT vk) { tugmachalar.push_back(vk); }
    void enter_qosh(HWND e);
    void ozgardi_qosh(HWND e);
    bool qol_kursor = false;
    wstring skaner;       // skaner o'qigan kod (tez yozilib Enter bosilgan)
    wstring belgilar;     // hech qaysi maydonda fokus yo'qligida yozilgan raqamlar
    bool qayta = false;
    float mx = -1, my = -1;

    // ---- primitivlar (gfx ga qisqa yo'l) ----
    bool ustida(R r);         // sichqon shu yerda va qatlam faol
    bool bosildi(const string& id, R r);  // to'liq bosish (bosib qo'yib yuborildi)
    bool bosilmoqda(const string& id);
    bool qatlam_faol() const { return qatlam == yuqori_oldingi; }

    // ---- vidjetlar (sayt CSS klasslari bo'yicha) ----
    // .tugma (+ .asosiy/.yashil/.tinch/.xato, .kichik)
    bool tugma(const string& id, R r, const wstring& matn, Uslub u = ODDIY, const char* ikon = nullptr,
               bool kichik = false, bool ochiq = true, float matn_olcham = 0);
    // .kirish maydoni (chegara, fokus halqasi) + ichida EDIT
    HWND maydon(const string& id, R r, MaydonSoz s = {});
    // Bezaksiz EDIT (o'zi chizgan quti ichiga)
    HWND tahrir(const string& id, R r, MaydonSoz s = {});
    wstring qiymat(const string& id);
    void qiymat_qoy(const string& id, const wstring& v);
    void fokus(const string& id);
    bool fokusda(const string& id);
    bool enter(const string& id);
    bool ozgardi(const string& id);
    void maydonlarni_tozala(const string& boshi);  // shu prefiksli maydonlarni yo'q qilish
    // select.kirish
    bool tanlov(const string& id, R r, const std::vector<wstring>& v, int& i, float olcham = 19);
    // checkbox + yozuv
    bool belgi(const string& id, R r, const wstring& matn, bool& v);
    // .richag (yo-l + dasta)
    void richag_chiz(R r, bool yoniq, Rang rang);
    // .nishon
    float nishon(float x, float y, const wstring& matn, Rang fon, Rang chegara, Rang rang,
                 bool ong = false);

    // ---- siljiydigan qism ----
    float scroll(const string& id, R r, float kontent);
    void scroll_tugat();
    void scroll_tepaga(const string& id);
    void scroll_korsat(const string& id, float y, float h);  // shu qator ko'rinsin

    // ---- modal oyna ----
    void oyna_fon();  // orqani xiralashtiradi, yangi qatlam

    // ---- klaviatura ----
    bool tugmacha(UINT vk);
    bool ctrl() const { return GetKeyState(VK_CONTROL) < 0; }

    // ---- bildirishnomalar (o'ng yuqorida, 5 soniya) ----
    void xabar(const wstring& matn, XabarTur tur = X_OK);

    // D2D kadri ekranga chiqqandan keyin: matni dastur tomonidan o'zgargan yoki
    // endi ko'ringan maydonlarni qayta chizdirish (aks holda eski rasm qoladi)
    void maydonlarni_chiz();

    // EDIT ranglari uchun (WM_CTLCOLOREDIT)
    HBRUSH edit_rangi(HDC dc, HWND e);

private:
    bool bos = false, qoy = false, ushlab = false;
    float bx = 0, by = 0, qx = 0, qy = 0, gildirak = 0;
    std::vector<UINT> tugmachalar;
    std::set<string> enterlar, ozgarganlar;
    unsigned faol = 0;
    int qatlam = 0, yuqori = 0, yuqori_oldingi = 0;

    struct Kesim { R ekran; };
    std::vector<R> kesimlar;  // ekran koordinatasida

    struct Tahrir {
        HWND h = nullptr;
        int qatlam = 0;
        bool ishlatildi = false;
        bool yangi = false;
        MaydonSoz s;
        R ekran;
    };
    std::map<string, Tahrir> tahrirlar;
    std::map<HWND, string> hwnd_id;
    std::map<std::pair<int, int>, HFONT> shriftlar;
    std::map<COLORREF, HBRUSH> moyqalamlar;
    string fokus_kerak;
    std::set<HWND> chizilsin;

    struct Scroll { float y = 0, maks = 0; R ekran; bool sudrash = false; float sudrash_y = 0, boshl_y = 0; };
    std::map<string, Scroll> scrollar;
    std::vector<std::pair<string, R>> scroll_stek;
    std::vector<std::pair<string, R>> scroll_joylar, scroll_joylar_oldingi;

    struct Ochiluvchi { string id; R ekran; std::vector<wstring> v; int i; float olcham; };
    bool ochiq_bor = false;
    Ochiluvchi ochiq;
    string ochiq_id;
    std::map<string, int> tanlov_natija;

    struct Xabar { wstring matn; XabarTur tur; DWORD vaqt; };
    std::vector<Xabar> xabarlar;

    bool nuqtada(float px, float py, R r);
    R ekranga(R r) const { return {r.x + gfx.dx, r.y + gfx.dy, r.w, r.h}; }
    HFONT shrift(float olcham, int ogirlik);
    void ochiluvchini_chiz();
    void xabarlarni_chiz();
};

extern Ui ui;
unsigned id_hash(const string& s);
