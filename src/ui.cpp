#include "ui.h"

#include <commctrl.h>

#include <algorithm>

Ui ui;

unsigned id_hash(const string& s) {
    unsigned h = 2166136261u;
    for (unsigned char c : s) h = (h ^ c) * 16777619u;
    return h ? h : 1;
}

void Ui::boshla(HWND oyna) { h = oyna; }

void Ui::kadr_boshla() {
    RECT rc;
    GetClientRect(h, &rc);
    W = rc.right / gfx.masshtab();
    H = rc.bottom / gfx.masshtab();
    qatlam = 0;
    yuqori = 0;
    qol_kursor = false;
    qayta = false;
    kesimlar.clear();
    kesimlar.push_back({0, 0, W, H});
    scroll_joylar.clear();
    for (auto& [k, t] : tahrirlar) t.ishlatildi = false;
}

void Ui::kadr_tugat() {
    if (ochiq_bor) ochiluvchini_chiz();
    xabarlarni_chiz();
    if (yuqori != yuqori_oldingi) qayta = true;
    // Ishlatilmagan yoki pastki qatlamdagi maydonlarni yashiramiz
    for (auto& [k, t] : tahrirlar) {
        bool kor = t.ishlatildi && t.qatlam >= yuqori;
        if (kor && ochiq_bor) {  // ochiluvchi ro'yxat ustiga chiqmasin
            R a = t.ekran, b = ochiq.ekran;
            float ox = b.y + b.h, ro = std::min<float>(ochiq.v.size(), 8) * 36 + 8;
            if (a.x < b.o() && a.o() > b.x && a.y < ox + ro && a.p() > ox) kor = false;
        }
        if (!kor && IsWindowVisible(t.h)) {
            if (GetFocus() == t.h) SetFocus(h);
            ShowWindow(t.h, SW_HIDE);
        }
    }
    if (!fokus_kerak.empty()) {
        auto it = tahrirlar.find(fokus_kerak);
        if (it != tahrirlar.end() && IsWindowVisible(it->second.h)) {
            SetFocus(it->second.h);
            SendMessageW(it->second.h, EM_SETSEL, 0, -1);
            fokus_kerak.clear();
        } else if (it == tahrirlar.end() || !it->second.ishlatildi)
            fokus_kerak.clear();
        else
            qayta = true;
    }
    yuqori_oldingi = yuqori;
    scroll_joylar_oldingi = scroll_joylar;
    if (qoy) faol = 0;
    bos = qoy = false;
    gildirak = 0;
    tugmachalar.clear();
    enterlar.clear();
    ozgarganlar.clear();
    skaner.clear();
    belgilar.clear();
    if (qayta) InvalidateRect(h, nullptr, FALSE);
}

// ---------- urilish ----------

bool Ui::nuqtada(float px, float py, R r) {
    if (qatlam != yuqori_oldingi) return false;
    R e = ekranga(r);
    if (!e.ichida(px, py)) return false;
    return kesimlar.back().ichida(px, py);
}

bool Ui::ustida(R r) { return nuqtada(mx, my, r); }

bool Ui::bosildi(const string& id, R r) {
    unsigned i = id_hash(id);
    if (ustida(r)) qol_kursor = true;
    if (bos && nuqtada(bx, by, r)) {
        faol = i;
        if (ochiq_bor && ochiq_id != id) ochiq_bor = false;
    }
    return qoy && faol == i && nuqtada(qx, qy, r);
}

bool Ui::bosilmoqda(const string& id) { return ushlab && faol == id_hash(id); }

// ---------- tugma ----------

bool Ui::tugma(const string& id, R r, const wstring& matn, Uslub u, const char* ikon, bool kichik,
               bool ochiq_, float matn_olcham) {
    bool bos_ = ochiq_ && bosildi(id, r);
    bool hov = ochiq_ && ustida(r), bosil = ochiq_ && bosilmoqda(id) && hov;
    Rang fon = M->kechki ? M->panel3 : M->panel, chegara = M->chegara2, rang = M->matn;
    switch (u) {
        case ASOSIY: fon = chegara = hov ? M->urgu_quyuq : M->urgu; rang = M->asosiy_matn; break;
        case YASHIL: fon = chegara = hov ? M->tasdiq_quyuq : M->tasdiq; rang = M->tasdiq_matn; break;
        case TINCH:
            fon = hov ? M->panel2 : Rang{};
            chegara = M->chegara;
            rang = hov ? M->matn : M->matn2;
            break;
        case XATO_U:
            fon = hov ? M->xato_fon : Rang{};
            chegara = M->xato_chek;
            rang = M->kechki ? (hov ? hex("#ffb3b3") : hex("#ffa0a0")) : M->xato;
            break;
        default:
            if (hov) { fon = M->tugma_hover; chegara = M->tugma_hover_chegara; }
    }
    if (!ochiq_) { fon = M->panel2; chegara = M->chegara; rang = M->matn3; }
    if (bosil) r.y += 1;
    float rad = kichik ? 7 : 9;
    gfx.quti(r, fon, rad, chegara);
    float ol = matn_olcham > 0 ? matn_olcham : (kichik ? 13 : 15);
    float ik = ikon ? (kichik ? 16 : 19) : 0, ora = ikon && !matn.empty() ? (kichik ? 6 : 8) : 0;
    float en = gfx.matn_eni(matn, ol, 600);
    float jami = std::min(ik + ora + en, r.w - 8);
    float x = r.x + (r.w - jami) / 2;
    if (ikon) gfx.ikon(ikon, {x, r.y + (r.h - ik) / 2, ik, ik}, rang);
    gfx.matn({x + ik + ora, r.y, jami - ik - ora + 1, r.h}, matn, ol, 600, rang, CHAP);
    return bos_;
}

// ---------- matn maydonlari ----------

HFONT Ui::shrift(float olcham, int ogirlik) {
    int px = int(olcham * gfx.masshtab() + .5f);
    auto k = std::make_pair(px, ogirlik);
    auto it = shriftlar.find(k);
    if (it != shriftlar.end()) return it->second;
    HFONT f = CreateFontW(-px, 0, 0, 0, ogirlik, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0,
                          L"Segoe UI");
    shriftlar[k] = f;
    return f;
}

HWND Ui::tahrir(const string& id, R r, MaydonSoz s) {
    auto& t = tahrirlar[id];
    wstring oldingi;
    bool qayta_yarat = t.h && t.s.olcham == 0;  // qiymat_qoy() oldindan yaratgan — uslubi yo'q
    if (qayta_yarat) {
        int n = GetWindowTextLengthW(t.h);
        oldingi.resize(n);
        if (n) GetWindowTextW(t.h, oldingi.data(), n + 1);
        hwnd_id.erase(t.h);
        DestroyWindow(t.h);
        t.h = nullptr;
    }
    if (!t.h) {
        DWORD stil = WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | (s.ong ? ES_RIGHT : 0) |
                     (s.parol ? ES_PASSWORD : 0) |
                     (s.kop_qator ? ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL : 0);
        t.h = CreateWindowExW(0, L"EDIT", L"", stil, 0, 0, 10, 10, h, nullptr,
                              GetModuleHandleW(nullptr), nullptr);
        SendMessageW(t.h, WM_SETFONT, (WPARAM)shrift(s.olcham, s.ogirlik), TRUE);
        SendMessageW(t.h, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
        if (s.ishora) SendMessageW(t.h, EM_SETCUEBANNER, TRUE, (LPARAM)s.ishora);
        hwnd_id[t.h] = id;
        t.yangi = true;
        if (qayta_yarat) SetWindowTextW(t.h, oldingi.c_str());
    }
    t.s = s;
    t.qatlam = qatlam;
    t.ishlatildi = true;
    R e = ekranga(r);
    t.ekran = e;
    float ms = gfx.masshtab();
    float boy = s.kop_qator ? r.h : s.olcham * 1.36f;
    float y = s.kop_qator ? e.y : e.y + (e.h - boy) / 2;
    // Siljiydigan qism ichida qisman ko'rinsa — yashiramiz
    R k = kesimlar.back();
    bool kor = y >= k.y - 1 && y + boy <= k.p() + 1 && e.x >= k.x - 1 && e.o() <= k.o() + 1;
    // ustida modal oyna turgan bo'lsa (oldingi kadrda qatlam yuqoriroq edi) — ko'rsatmaymiz
    if (qatlam < yuqori_oldingi) kor = false;
    if (!kor) {
        t.ishlatildi = false;
        return t.h;
    }
    bool yashirin = !IsWindowVisible(t.h);
    SetWindowPos(t.h, HWND_TOP, int(e.x * ms + .5f), int(y * ms + .5f), int(e.w * ms + .5f),
                 int(boy * ms + .5f), SWP_NOACTIVATE | SWP_SHOWWINDOW);
    if (t.yangi || yashirin) {  // birinchi chizishda oq fon ko'rinmasin
        t.yangi = false;
        chizilsin.insert(t.h);
    }
    return t.h;
}

HWND Ui::maydon(const string& id, R r, MaydonSoz s) {
    auto it = tahrirlar.find(id);
    bool fok = it != tahrirlar.end() && GetFocus() == it->second.h;
    Rang fon = s.fon.bormi() ? s.fon : M->panel2;
    if (fok) gfx.quti({r.x - 3, r.y - 3, r.w + 6, r.h + 6}, M->urgu_fon, 12);
    Rang ch = fok ? M->urgu : (ustida(r) ? (M->kechki ? hex("#6e6e6e") : M->chegara2) : M->chegara2);
    gfx.quti(r, fon, 9, ch);
    if (ustida(r)) qol_kursor = false;
    if (s.kop_qator) return tahrir(id, r.ichki(13, 8), s);
    return tahrir(id, {r.x + 13, r.y, r.w - 26, r.h}, s);
}

wstring Ui::qiymat(const string& id) {
    auto it = tahrirlar.find(id);
    if (it == tahrirlar.end() || !it->second.h) return L"";
    int n = GetWindowTextLengthW(it->second.h);
    wstring s(n, L'\0');
    if (n) GetWindowTextW(it->second.h, s.data(), n + 1);
    return s;
}

void Ui::qiymat_qoy(const string& id, const wstring& v) {
    auto& t = tahrirlar[id];
    if (!t.h) {
        // hali yaratilmagan — yashirin yaratib qo'yamiz, keyingi kadrda joylashadi
        t.h = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10,
                              h, nullptr, GetModuleHandleW(nullptr), nullptr);
        hwnd_id[t.h] = id;
        t.s.olcham = 0;
    }
    wchar_t eski[512] = {};
    GetWindowTextW(t.h, eski, 512);
    if (v != eski) {
        SetWindowTextW(t.h, v.c_str());
        chizilsin.insert(t.h);
    }
}

void Ui::maydonlarni_chiz() {
    for (HWND e : chizilsin)
        if (IsWindow(e) && IsWindowVisible(e))
            RedrawWindow(e, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
    chizilsin.clear();
}

void Ui::fokus(const string& id) { fokus_kerak = id; qayta = true; }

bool Ui::fokusda(const string& id) {
    auto it = tahrirlar.find(id);
    return it != tahrirlar.end() && GetFocus() == it->second.h;
}

void Ui::enter_qosh(HWND e) {
    auto it = hwnd_id.find(e);
    if (it != hwnd_id.end()) enterlar.insert(it->second);
}
void Ui::ozgardi_qosh(HWND e) {
    auto it = hwnd_id.find(e);
    if (it != hwnd_id.end()) ozgarganlar.insert(it->second);
}
bool Ui::enter(const string& id) { return enterlar.erase(id) > 0; }
bool Ui::ozgardi(const string& id) { return ozgarganlar.count(id) > 0; }

void Ui::maydonlarni_tozala(const string& boshi) {
    for (auto it = tahrirlar.begin(); it != tahrirlar.end();) {
        if (it->first.rfind(boshi, 0) == 0) {
            hwnd_id.erase(it->second.h);
            DestroyWindow(it->second.h);
            it = tahrirlar.erase(it);
        } else
            ++it;
    }
}

HBRUSH Ui::edit_rangi(HDC dc, HWND e) {
    auto it = hwnd_id.find(e);
    Rang fon = M->panel2;
    if (it != hwnd_id.end()) {
        auto& t = tahrirlar[it->second];
        if (t.s.fon.bormi()) fon = t.s.fon;
    }
    SetTextColor(dc, M->matn.gdi());
    SetBkColor(dc, fon.gdi());
    COLORREF c = fon.gdi();
    auto& b = moyqalamlar[c];
    if (!b) b = CreateSolidBrush(c);
    return b;
}

// ---------- select ----------

bool Ui::tanlov(const string& id, R r, const std::vector<wstring>& v, int& i, float olcham) {
    bool ozg = false;
    auto n = tanlov_natija.find(id);
    if (n != tanlov_natija.end()) {
        ozg = n->second != i;
        i = n->second;
        tanlov_natija.erase(n);
    }
    bool och = ochiq_bor && ochiq_id == id;
    if (bosildi(id, r)) {
        if (och) ochiq_bor = false;
        else {
            ochiq_bor = true;
            ochiq_id = id;
        }
        qayta = true;
    }
    och = ochiq_bor && ochiq_id == id;
    if (och) ochiq = {id, ekranga(r), v, i, olcham};
    bool hov = ustida(r);
    Rang ch = och ? M->urgu : hov ? (M->kechki ? hex("#6e6e6e") : M->chegara2) : M->chegara2;
    gfx.quti(r, M->panel2, 9, ch);
    if (i >= 0 && i < (int)v.size())
        gfx.matn({r.x + 13, r.y, r.w - 44, r.h}, v[i], olcham, 400, M->matn);
    // ▾ belgisi
    float cx = r.o() - 20, cy = r.y + r.h / 2;
    gfx.chiziq(cx - 5, cy - 2.5f, cx, cy + 2.5f, M->matn2, 1.6f);
    gfx.chiziq(cx, cy + 2.5f, cx + 5, cy - 2.5f, M->matn2, 1.6f);
    return ozg;
}

void Ui::ochiluvchini_chiz() {
    // Ro'yxat eng yuqori qatlamda chiziladi; pastdagi narsalar unga javob bermaydi.
    gfx.siljit(-gfx.dx, -gfx.dy);
    float qb = 36;
    int n = (int)ochiq.v.size();
    float boy = std::min(n, 8) * qb + 8;
    R r = {ochiq.ekran.x, ochiq.ekran.p() + 4, ochiq.ekran.w, boy};
    if (r.p() > H - 4) r.y = ochiq.ekran.y - boy - 4;
    gfx.quti({r.x, r.y + 4, r.w, r.h}, {0, 0, 0, .25f}, 9);
    gfx.quti(r, M->panel2, 9, M->chegara2);
    static float sy = 0;
    float maks = std::max(0.f, n * qb + 8 - boy);
    if (r.ichida(mx, my) && gildirak != 0) {
        sy -= gildirak * 0.4f;
        gildirak = 0;
    }
    if (sy > maks) sy = maks;
    if (sy < 0) sy = 0;
    gfx.kes(r.ichki(1));
    for (int k = 0; k < n; ++k) {
        R q = {r.x + 4, r.y + 4 + k * qb - sy, r.w - 8, qb};
        if (q.p() < r.y || q.y > r.p()) continue;
        bool hov = q.ichida(mx, my) && r.ichida(mx, my);
        if (k == ochiq.i) gfx.quti(q, M->urgu_fon, 7);
        if (hov) gfx.quti(q, M->panel3, 7);
        gfx.matn({q.x + 10, q.y, q.w - 20, q.h}, ochiq.v[k], std::min(ochiq.olcham, 16.f), 400, M->matn);
        if (hov) qol_kursor = true;
        if (qoy && q.ichida(qx, qy) && r.ichida(qx, qy)) {
            tanlov_natija[ochiq.id] = k;
            ochiq_bor = false;
            qayta = true;
        }
    }
    gfx.kesishni_ol();
    if (bos && !r.ichida(bx, by) && !ochiq.ekran.ichida(bx, by)) {
        ochiq_bor = false;
        qayta = true;
    }
    if (tugmacha(VK_ESCAPE)) ochiq_bor = false;
    if (ochiq_bor) yuqori = std::max(yuqori, qatlam + 1);
}

// ---------- checkbox va richag ----------

bool Ui::belgi(const string& id, R r, const wstring& matn, bool& v) {
    bool b = bosildi(id, r);
    if (b) v = !v;
    R q = {r.x, r.y + (r.h - 20) / 2, 20, 20};
    if (v) {
        gfx.quti(q, M->urgu, 4);
        gfx.chiziq(q.x + 5, q.y + 10.5f, q.x + 8.5f, q.y + 14, hex("#ffffff"), 2.2f);
        gfx.chiziq(q.x + 8.5f, q.y + 14, q.x + 15, q.y + 6.5f, hex("#ffffff"), 2.2f);
    } else
        gfx.quti(q, M->kechki ? hex("#3b3b3b") : hex("#ffffff"), 4, M->kechki ? hex("#858585") : hex("#767676"));
    gfx.matn({r.x + 28, r.y, r.w - 28, r.h}, matn, 14, 500, M->matn);
    return b;
}

void Ui::richag_chiz(R r, bool yoniq, Rang rang) {
    gfx.quti(r, yoniq ? rang : M->chegara2, r.h / 2);
    float d = r.h - 6;
    float x = yoniq ? r.o() - 3 - d : r.x + 3;
    gfx.doira(x + d / 2, r.y + 3 + d / 2, d / 2, M->kechki ? hex("#d7d7d7") : hex("#ffffff"));
}

float Ui::nishon(float x, float y, const wstring& matn, Rang fon, Rang chegara, Rang rang, bool ong) {
    float en = gfx.matn_eni(matn, 11, 600) + 16;
    if (ong) x -= en;
    R r = {x, y, en, 19};
    gfx.quti(r, fon, 9.5f, chegara);
    gfx.matn(r, matn, 11, 600, rang, MARKAZ);
    return en;
}

// ---------- scroll ----------

float Ui::scroll(const string& id, R r, float kontent) {
    auto& s = scrollar[id];
    R e = ekranga(r);
    s.ekran = e;
    s.maks = std::max(0.f, kontent - r.h);
    scroll_joylar.push_back({id, e});
    // g'ildirak — sichqon ostidagi eng ichki qismga
    if (gildirak != 0 && qatlam == yuqori_oldingi) {
        string nishon_id;
        for (auto& [k, rr] : scroll_joylar_oldingi)
            if (rr.ichida(mx, my)) nishon_id = k;
        if (nishon_id == id) {
            s.y -= gildirak * 0.5f;
            gildirak = 0;
        }
    }
    // o'ng chetdagi sudralma
    if (s.maks > 0) {
        float th = std::max(30.f, r.h * r.h / kontent);
        float ty = r.y + (r.h - th) * (s.y / s.maks);
        R tutqich = {r.o() - 10, ty, 10, th};
        if (bos && nuqtada(bx, by, {r.o() - 12, r.y, 12, r.h})) {
            s.sudrash = true;
            s.sudrash_y = by;
            s.boshl_y = s.y;
        }
        if (s.sudrash) {
            if (!ushlab) s.sudrash = false;
            else s.y = s.boshl_y + (my - s.sudrash_y) * (s.maks / std::max(1.f, r.h - th));
        }
        (void)tutqich;
    }
    if (s.y > s.maks) s.y = s.maks;
    if (s.y < 0) s.y = 0;
    scroll_stek.push_back({id, r});
    gfx.kes(r);
    R k = kesimlar.back();
    R yangi = {std::max(k.x, e.x), std::max(k.y, e.y), 0, 0};
    yangi.w = std::min(k.o(), e.o()) - yangi.x;
    yangi.h = std::min(k.p(), e.p()) - yangi.y;
    kesimlar.push_back(yangi);
    gfx.siljit(0, -s.y);
    return s.y;
}

void Ui::scroll_tugat() {
    auto [id, r] = scroll_stek.back();
    scroll_stek.pop_back();
    auto& s = scrollar[id];
    gfx.siljit(0, s.y);
    kesimlar.pop_back();
    gfx.kesishni_ol();
    if (s.maks > 0) {
        float kontent = s.maks + r.h;
        float th = std::max(30.f, r.h * r.h / kontent);
        float ty = r.y + (r.h - th) * (s.y / s.maks);
        gfx.quti({r.o() - 10, ty, 8, th}, M->kechki ? M->chegara2 : hex("#c3cbd6"), 4);
    }
}

void Ui::scroll_tepaga(const string& id) { scrollar[id].y = 0; }

void Ui::scroll_korsat(const string& id, float y, float h) {
    auto& s = scrollar[id];
    float boy = s.ekran.h;
    if (y < s.y) s.y = y;
    else if (y + h > s.y + boy) s.y = y + h - boy;
    qayta = true;
}

// ---------- modal ----------

void Ui::oyna_fon() {
    ++qatlam;
    yuqori = std::max(yuqori, qatlam);
    gfx.quti({-gfx.dx, -gfx.dy, W, H}, M->kechki ? Rang{0, 0, 0, .66f} : hex("#101828", .45f));
}

// ---------- klaviatura ----------

bool Ui::tugmacha(UINT vk) {
    auto it = std::find(tugmachalar.begin(), tugmachalar.end(), vk);
    if (it == tugmachalar.end()) return false;
    tugmachalar.erase(it);
    return true;
}

// ---------- bildirishnomalar ----------

void Ui::xabar(const wstring& matn, XabarTur tur) {
    xabarlar.push_back({matn, tur, GetTickCount()});
    SetTimer(h, 77, 500, nullptr);
    qayta = true;
}

void Ui::xabarlarni_chiz() {
    DWORD hozir = GetTickCount();
    xabarlar.erase(std::remove_if(xabarlar.begin(), xabarlar.end(),
                                  [&](const Xabar& x) { return hozir - x.vaqt > 5000; }),
                   xabarlar.end());
    if (xabarlar.empty()) {
        KillTimer(h, 77);
        return;
    }
    gfx.siljit(-gfx.dx, -gfx.dy);
    float en = std::min(320.f, W - 28), y = 12;
    for (size_t i = 0; i < xabarlar.size(); ++i) {
        auto& x = xabarlar[i];
        float boy = std::max(36.f, gfx.matn_boyi(x.matn, 13, 500, en - 49) + 18);
        R r = {W - 14 - en, y, en, boy};
        Rang fon = M->tasdiq_fon, ch = M->tasdiq_chek, rang = M->kechki ? hex("#a9ecca") : hex("#0b5b35");
        if (x.tur == X_XATO) fon = M->xato_fon, ch = M->xato_chek, rang = M->kechki ? hex("#ffc2c2") : hex("#a01b12");
        if (x.tur == X_OGOH) fon = M->ogoh_fon, ch = M->ogoh_chek, rang = M->kechki ? hex("#ffd382") : hex("#8a5a00");
        if (x.tur == X_INFO) fon = M->kok_fon, ch = M->kok_chek, rang = M->kechki ? hex("#a9dcf5") : hex("#0a5680");
        gfx.quti({r.x, r.y + 6, r.w, r.h}, {0, 0, 0, .3f}, 9);
        gfx.quti(r, M->panel, 9);
        gfx.quti(r, fon, 9, ch);
        gfx.ikon(x.tur == X_XATO ? "i-ogoh" : "i-belgi", {r.x + 12, r.y + 10, 16, 16}, rang);
        gfx.matn({r.x + 37, r.y + 9, r.w - 49, r.h - 18}, x.matn, 13, 500, rang, CHAP, 0, true);
        if (bos && r.ichida(bx, by)) x.vaqt = 0;  // bosilsa yopiladi
        y += boy + 8;
    }
}
