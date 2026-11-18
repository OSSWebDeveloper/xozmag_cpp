// ombor/excel.py — C++ dagi nusxasi (qoidalar, xabarlar va JSON bir xil).
#include "excel.h"

#include <algorithm>
#include <cmath>

#include "xlsx.h"

const std::vector<string> HOLAT_MAYDONLARI = {"kod", "nom", "birlik", "qoldiq", "narx", "valyuta", "faol",
                                              "kelish_narxi", "kelish_valyutasi"};
static const std::vector<string> USTUNLAR = {"kod", "nom", "birlik", "qoldiq", "narx", "valyuta",
                                             "kelish_narxi", "kelish_valyutasi"};
static const std::map<string, wstring> USTUN_NOMI = {
    {"kod", L"Kod"}, {"nom", L"Tovar nomi"}, {"birlik", L"Birlik"}, {"qoldiq", L"Qoldiq"},
    {"narx", L"Narxi"}, {"valyuta", L"Valyuta"}, {"kelish_narxi", L"Kelish narxi"},
    {"kelish_valyutasi", L"Kelish valyutasi"}};

// ======================= JSON (tekis obyekt) =======================

static string json_matn(const wstring& w) {
    string s = U(w), r = "\"";
    for (unsigned char c : s) {
        if (c == '"') r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c < 0x20) { char b[8]; snprintf(b, 8, "\\u%04x", c); r += b; }
        else r += (char)c;
    }
    return r + "\"";
}

string json_yoz(const Holat& h) {
    // Django JSONField ko'rinishi: {"kod": "0001", "nom": "...", ...}
    string r = "{";
    bool bir = true;
    auto yoz = [&](const string& k, const wstring& v) {
        if (!bir) r += ", ";
        bir = false;
        r += "\"" + k + "\": " + (k == "faol" ? U(v == L"true" ? L"true" : L"false") : json_matn(v));
    };
    for (auto& k : HOLAT_MAYDONLARI)
        if (h.count(k)) yoz(k, h.at(k));
    for (auto& [k, v] : h)
        if (std::find(HOLAT_MAYDONLARI.begin(), HOLAT_MAYDONLARI.end(), k) == HOLAT_MAYDONLARI.end()) yoz(k, v);
    return r + "}";
}

Holat json_oqi(const string& s) {
    Holat h;
    size_t i = s.find('{');
    if (i == string::npos) return h;
    ++i;
    auto bosh = [&]() { while (i < s.size() && isspace((unsigned char)s[i])) ++i; };
    auto matn = [&]() {
        string r;
        ++i;  // "
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                char c = s[++i];
                if (c == 'n') r += '\n';
                else if (c == 't') r += '\t';
                else if (c == 'u' && i + 4 < s.size()) {
                    unsigned cp = (unsigned)strtoul(s.substr(i + 1, 4).c_str(), nullptr, 16);
                    i += 4;
                    if (cp >= 0xD800 && cp < 0xDC00 && i + 6 < s.size() && s[i + 1] == '\\' && s[i + 2] == 'u') {
                        unsigned lo = (unsigned)strtoul(s.substr(i + 3, 4).c_str(), nullptr, 16);
                        i += 6;
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    }
                    wstring w;
                    if (cp >= 0x10000) { cp -= 0x10000; w += (wchar_t)(0xD800 + (cp >> 10)); w += (wchar_t)(0xDC00 + (cp & 0x3FF)); }
                    else w += (wchar_t)cp;
                    r += U(w);
                } else r += c;
            } else r += s[i];
            ++i;
        }
        ++i;
        return r;
    };
    for (;;) {
        bosh();
        if (i >= s.size() || s[i] == '}') break;
        if (s[i] == ',') { ++i; continue; }
        if (s[i] != '"') break;
        string k = matn();
        bosh();
        if (i < s.size() && s[i] == ':') ++i;
        bosh();
        if (i >= s.size()) break;
        if (s[i] == '"') h[k] = W(matn());
        else {
            size_t e = i;
            while (e < s.size() && s[e] != ',' && s[e] != '}') ++e;
            string v = s.substr(i, e - i);
            while (!v.empty() && isspace((unsigned char)v.back())) v.pop_back();
            i = e;
            if (v == "null") h[k] = L"";
            else h[k] = W(v);  // true/false/son
        }
    }
    return h;
}

// ======================= Yordamchilar =======================

static wstring tekis(double d, int kasr) {
    double k = std::pow(10.0, kasr);
    d = std::nearbyint(d * k) / k;
    wchar_t b[64];
    swprintf(b, 64, L"%.*f", kasr, d);
    wstring r = b;
    if (r.find(L'.') != wstring::npos) {
        while (r.back() == L'0') r.pop_back();
        if (r.back() == L'.') r.pop_back();
    }
    if (r == L"-0") r = L"0";
    return r;
}

static Holat tovar_holati(const Tovar& t) {
    return {{"kod", t.kod}, {"nom", t.nom}, {"birlik", t.birlik}, {"qoldiq", tekis(t.qoldiq, 3)},
            {"narx", tekis(t.narx, 4)}, {"valyuta", t.valyuta}, {"faol", t.faol ? L"true" : L"false"},
            {"kelish_narxi", t.kelish_bor ? tekis(t.kelish_narxi, 4) : L""},
            {"kelish_valyutasi", t.kelish_valyutasi}};
}

Holat mahsulot_holati(long long id) {
    Tovar t;
    if (!tovar_ol(id, t)) return {};
    return tovar_holati(t);
}

static Holat birlashtir(Holat a, const Holat& b) {  // a | b
    for (auto& [k, v] : b) a[k] = v;
    return a;
}

static bool holat_mosmi(const Holat& h, const Holat& kutilgan) {
    for (auto& [k, v] : kutilgan) {
        auto it = h.find(k);
        if (it == h.end() || it->second != v) return false;
    }
    return true;
}

static wstring norm(const wstring& s) {
    wstring r = s;
    if (!r.empty()) CharLowerBuffW(r.data(), (DWORD)r.size());
    wstring t;
    for (wchar_t c : r)
        if (c != 0x02BB && c != 0x02BC && c != L'\'' && c != L'`' && c != 0x2019 && c != 0x2018) t += c;
    wstring z;
    bool bosh = false;
    for (wchar_t c : t) {
        if (iswspace(c) || c == 0xA0) { bosh = true; continue; }
        if (bosh && !z.empty()) z += L' ';
        bosh = false;
        z += c;
    }
    return z;
}

static const std::map<wstring, string>& sarlavhalar() {
    static std::map<wstring, string> m;
    if (m.empty()) {
        std::vector<std::pair<string, std::vector<wstring>>> s = {
            {"kod", {L"kod", L"код", L"tovar kodi", L"joy kodi"}},
            {"nom", {L"tovar nomi", L"tovar", L"nomi", L"товар", L"наименование", L"название"}},
            {"birlik", {L"birlik", L"birligi", L"olchov", L"ед", L"ед.", L"единица", L"ед. изм."}},
            {"qoldiq", {L"qoldiq", L"остаток", L"soni", L"miqdori", L"количество"}},
            {"narx", {L"narxi", L"narx", L"sotish narxi", L"цена", L"цена продажи"}},
            {"valyuta", {L"valyuta", L"валюта"}},
            {"kelish_narxi", {L"kelish narxi", L"kelish narx", L"xarid narxi", L"xarid narx", L"olish narxi",
                              L"закупочная цена", L"цена закупки", L"себестоимость"}},
            {"kelish_valyutasi", {L"kelish valyutasi", L"kelish valyuta", L"xarid valyutasi", L"валюта закупки"}},
        };
        for (auto& [k, v] : s)
            for (auto& x : v) m[norm(x)] = k;
    }
    return m;
}

static wstring valyutani_ani(const wstring& matn) {
    wstring s = norm(matn), t;
    for (wchar_t c : s)
        if (c != L'.') t += c;
    static const std::set<wstring> SOM = {L"uzs", L"som", L"sum", L"сум", L"сўм"};
    static const std::set<wstring> DOL = {L"usd", L"$", L"dollar", L"доллар", L"долл"};
    if (SOM.count(t)) return L"som";
    if (DOL.count(t)) return L"dollar";
    return L"";
}

static wstring birlikni_ani(const wstring& matn) {
    wstring s = norm(matn);
    for (auto& b : BIRLIKLAR) {
        if (s == norm(b)) return b;
        if (b == L"kvm" && s == norm(L"m²")) return b;
    }
    static const std::map<wstring, wstring> qoshimcha = {
        {L"шт", L"dona"}, {L"шт.", L"dona"}, {L"ta", L"dona"}, {L"кг", L"kg"},
        {L"м", L"metr"}, {L"m", L"metr"}, {L"метр", L"metr"}, {L"л", L"litr"}, {L"l", L"litr"}, {L"литр", L"litr"}};
    auto it = qoshimcha.find(s);
    return it == qoshimcha.end() ? L"" : it->second;
}

static wstring katak_matni(const Katak& k) {
    if (k.tur == Katak::SON) {
        if (std::floor(k.son) == k.son && std::fabs(k.son) < 1e15) return std::to_wstring((long long)k.son);
        return k.matn;
    }
    return k.matn;
}

// 0 — bo'sh, 1 — son, -1 — xato
static int katak_soni(const Katak& k, int kasr, double& d, wstring& val, wstring& xato) {
    val.clear();
    if (k.boshmi()) return 0;
    if (k.tur == Katak::SON) d = k.son;
    else {
        wstring s;
        for (wchar_t c : k.matn) s += c == 0xA0 ? L' ' : c;
        s = tozala(s);
        // oxiridagi harflar valyuta bo'lsa ajratiladi: «180726 UZS», «2.65 USD»
        size_t e = s.size();
        while (e > 0) {
            wchar_t c = s[e - 1];
            if (iswalpha(c) || c == L'$' || c == L'.' || c == L'\'' || c == 0x02BB || c == 0x2018) --e;
            else break;
        }
        if (e < s.size()) {
            wstring v = valyutani_ani(s.substr(e));
            if (!v.empty()) {
                val = v;
                s = tozala(s.substr(0, e));
            }
        }
        wstring t;
        for (wchar_t c : s)
            if (c != L' ') t += c;
        size_t vergul = std::count(t.begin(), t.end(), L',');
        if (vergul == 1 && t.find(L'.') == wstring::npos) std::replace(t.begin(), t.end(), L',', L'.');
        else t.erase(std::remove(t.begin(), t.end(), L','), t.end());
        wchar_t* oxir = nullptr;
        d = t.empty() ? NAN : wcstod(t.c_str(), &oxir);
        bool harf = false;
        for (wchar_t c : t) harf |= (c == L'n' || c == L'N' || c == L'i' || c == L'I');
        if (t.empty() || !oxir || *oxir || harf) {
            xato = L"«" + k.matn + L"» son emas";
            return -1;
        }
    }
    if (!std::isfinite(d)) {
        xato = L"«" + katak_matni(k) + L"» son emas";
        return -1;
    }
    double kk = std::pow(10.0, kasr);
    d = std::nearbyint(d * kk) / kk;
    if (std::fabs(d) >= std::pow(10.0, kasr == 3 ? 9 : 12)) {
        xato = L"«" + katak_matni(k) + L"» juda katta";
        return -1;
    }
    return 1;
}

static void qatorni_tekshir(const std::map<string, Katak>& xom, Holat& holat, wstring& xato, wstring& ogoh) {
    holat.clear();
    std::vector<wstring> xatolar, og;
    bool kelish_bor = xom.count("kelish_narxi") || xom.count("kelish_valyutasi"), boshqa = false;
    for (auto& [k, v] : xom)
        if (k != "kelish_narxi" && k != "kelish_valyutasi" && k != "kod" && k != "nom") boshqa = true;
    if (!kelish_bor || boshqa) holat["faol"] = L"true";
    if (xom.count("kod")) {
        const Katak& k = xom.at("kod");
        wstring s = k.boshmi() ? L"" : tozala(katak_matni(k));
        if (!s.empty()) {
            if (!raqammi(s) || s.size() > 9 || std::stoll(s) < 1 || std::stoll(s) > KOD_MAX) {
                xatolar.push_back(L"kod «" + s + L"» noto'g'ri — 0001 dan 9999 gacha bo'lishi kerak");
                holat["kod"] = s.substr(0, 10);
            } else
                holat["kod"] = kod_yasa(std::stoll(s));
        } else
            holat["kod"] = L"";
    }
    if (xom.count("nom")) {
        wstring nom = tozala(katak_matni(xom.at("nom")));
        if (nom.empty()) xatolar.push_back(L"tovar nomi bo'sh");
        else if (nom.size() > 120) xatolar.push_back(L"tovar nomi 120 belgidan uzun");
        holat["nom"] = nom;
    }
    if (xom.count("birlik") && !tozala(katak_matni(xom.at("birlik"))).empty()) {
        wstring x = katak_matni(xom.at("birlik"));
        wstring b = birlikni_ani(x);
        if (b.empty()) {
            wstring hamma;
            for (auto& z : BIRLIKLAR) hamma += (hamma.empty() ? L"" : L", ") + z;
            xatolar.push_back(L"birlik «" + x + L"» noma'lum — " + hamma);
        }
        holat["birlik"] = b.empty() ? x : b;
    }
    wstring narx_val;
    for (auto [m, kasr] : {std::pair<const char*, int>{"qoldiq", 3}, {"narx", 4}}) {
        if (!xom.count(m)) continue;
        double d = 0;
        wstring val, x;
        int r = katak_soni(xom.at(m), kasr, d, val, x);
        if (r == 1 && d < 0 && string(m) == "narx") {
            r = -1;
            x = L"«" + katak_matni(xom.at(m)) + L"» manfiy";
        }
        if (r < 0) {
            xatolar.push_back(USTUN_NOMI.at(m) + L": " + x);
            holat[m] = katak_matni(xom.at(m));
            continue;
        }
        holat[m] = r == 0 ? L"0" : tekis(d, kasr);
        if (string(m) == "narx" && !val.empty()) narx_val = val;
    }
    if (xom.count("valyuta") && !tozala(katak_matni(xom.at("valyuta"))).empty()) {
        wstring x = katak_matni(xom.at("valyuta")), v = valyutani_ani(x);
        if (v.empty()) xatolar.push_back(L"valyuta «" + x + L"» noma'lum — UZS yoki USD yozing");
        holat["valyuta"] = v.empty() ? x : v;
        if (!v.empty() && !narx_val.empty() && narx_val != v)
            og.push_back(L"narx yonidagi valyuta «Valyuta» ustunidan farq qiladi — ustundagisi olindi");
    } else if (!narx_val.empty())
        holat["valyuta"] = narx_val;
    wstring kel_val;
    if (xom.count("kelish_narxi")) {
        double d = 0;
        wstring x;
        int r = katak_soni(xom.at("kelish_narxi"), 4, d, kel_val, x);
        if (r == 1 && d < 0) {
            r = -1;
            x = L"«" + katak_matni(xom.at("kelish_narxi")) + L"» manfiy";
        }
        if (r < 0) {
            xatolar.push_back(L"Kelish narxi: " + x);
            holat["kelish_narxi"] = katak_matni(xom.at("kelish_narxi"));
        } else
            holat["kelish_narxi"] = r == 0 ? L"" : tekis(d, 4);
    }
    if (xom.count("kelish_valyutasi") && !tozala(katak_matni(xom.at("kelish_valyutasi"))).empty()) {
        wstring x = katak_matni(xom.at("kelish_valyutasi")), v = valyutani_ani(x);
        if (v.empty()) xatolar.push_back(L"kelish valyutasi «" + x + L"» noma'lum — UZS yoki USD yozing");
        holat["kelish_valyutasi"] = v.empty() ? x : v;
        if (!v.empty() && !kel_val.empty() && kel_val != v)
            og.push_back(L"kelish narxi yonidagi valyuta «Kelish valyutasi» ustunidan farq qiladi — ustundagisi olindi");
    } else if (!kel_val.empty())
        holat["kelish_valyutasi"] = kel_val;
    xato.clear();
    ogoh.clear();
    for (auto& x : xatolar) xato += (xato.empty() ? L"" : L"; ") + x;
    for (auto& x : og) ogoh += (ogoh.empty() ? L"" : L"; ") + x;
}

static Holat& kelishni_tekisla(Holat& h) {
    if (h["kelish_narxi"].empty()) h["kelish_valyutasi"] = L"";
    else if (h["kelish_valyutasi"].empty()) h["kelish_valyutasi"] = !h["valyuta"].empty() ? h["valyuta"] : L"som";
    return h;
}

static Holat bosh_holat() {
    return {{"kod", L""}, {"nom", L""}, {"birlik", L"dona"}, {"qoldiq", L"0"}, {"narx", L"0"},
            {"valyuta", L"som"}, {"faol", L"true"}, {"kelish_narxi", L""}, {"kelish_valyutasi", L""}};
}

struct KodBeruvchi {
    std::set<long long> band;
    long long keyingi;
    bool tugadi = false;
    explicit KodBeruvchi(const std::set<wstring>& b) {
        for (auto& k : b)
            if (raqammi(k) && k.size() < 10) band.insert(std::stoll(k));
        keyingi = (band.empty() ? 0 : *band.rbegin()) + 1;
    }
    wstring ber() {
        if (tugadi) return L"";
        while (keyingi <= KOD_MAX && band.count(keyingi)) ++keyingi;
        if (keyingi > KOD_MAX) {
            keyingi = 0;
            for (long long k = 1; k <= KOD_MAX; ++k)
                if (!band.count(k)) { keyingi = k; break; }
            if (!keyingi) {
                tugadi = true;
                return L"";
            }
        }
        band.insert(keyingi);
        return kod_yasa(keyingi);
    }
};

// ======================= Qoralama =======================

struct YangiQator {
    long long excel_qator = 0, mahsulot_id = 0;
    wstring amal, kod, xato, ogoh;
    Holat eski, yangi;
    bool eski_bor = false, yangi_bor = false;
};

static void qator_yoz(long long yid, const YangiQator& q) {
    Sorov s = baza.sor("INSERT INTO ombor_yangilanishqatori (excel_qator, amal, eski, yangi, xato, ogohlantirish,"
                       " chiqarilgan, mahsulot_id, yangilanish_id, kod) VALUES (?,?,?,?,?,?,0,?,?,?)");
    if (q.excel_qator) s.bog(1, q.excel_qator); else s.bosh(1);
    s.bog(2, q.amal);
    if (q.eski_bor) s.bog(3, json_yoz(q.eski)); else s.bosh(3);
    if (q.yangi_bor) s.bog(4, json_yoz(q.yangi)); else s.bosh(4);
    s.bog(5, q.xato.substr(0, 300)).bog(6, q.ogoh.substr(0, 300));
    if (q.mahsulot_id) s.bog(7, q.mahsulot_id); else s.bosh(7);
    s.bog(8, yid).bog(9, q.kod.substr(0, 4));
    s.bajar();
}

long long fayldan_qoralama(const wstring& fayl, bool yoqlarini_yashir, const wstring& izoh,
                           std::vector<wstring>& xatolar) {
    xatolar.clear();
    wstring nom = fayl.substr(fayl.find_last_of(L"\\/") + 1);
    {
        wstring k = nom;
        CharLowerBuffW(k.data(), (DWORD)k.size());
        if (k.size() < 5 || k.substr(k.size() - 5) != L".xlsx") {
            xatolar.push_back(L"Faqat .xlsx fayl qabul qilinadi (eski .xls bo'lsa Excel'da «Сохранить как» .xlsx qiling).");
            return 0;
        }
        WIN32_FILE_ATTRIBUTE_DATA a;
        if (GetFileAttributesExW(fayl.c_str(), GetFileExInfoStandard, &a) && a.nFileSizeLow > 15 * 1024 * 1024) {
            xatolar.push_back(L"Fayl 15 MB dan katta.");
            return 0;
        }
    }
    Varaq v;
    wstring x;
    if (!xlsx_oqi(fayl, v, x)) {
        xatolar.push_back(x);
        return 0;
    }
    // sarlavha qatori (birinchi 15 qatordan)
    std::map<string, int> ustun;
    int sarlavha = 0, sanoq = 0;
    for (auto& [r, q] : v.qatorlar) {
        if (r > 15) break;
        std::map<string, int> top;
        for (size_t j = 0; j < q.size(); ++j) {
            auto it = sarlavhalar().find(norm(katak_matni(q[j])));
            if (it != sarlavhalar().end() && !top.count(it->second)) top[it->second] = (int)j;
        }
        ++sanoq;
        if (top.count("nom") || (top.count("kod") && top.size() > 1)) {
            ustun = top;
            sarlavha = r;
            break;
        }
    }
    if (ustun.empty()) {
        xatolar.push_back(L"Sarlavha qatori topilmadi: «Tovar nomi» (yoki «Товар») yoki «Kod» ustuni bo'lishi kerak. "
                          L"Shablonni yuklab oling.");
        return 0;
    }
    std::vector<std::pair<int, std::map<string, Katak>>> xom;
    for (auto& [r, q] : v.qatorlar) {
        if (r <= sarlavha) continue;
        std::map<string, Katak> m;
        bool bosh = true;
        for (auto& [k, j] : ustun) {
            Katak c = j < (int)q.size() ? q[j] : Katak();
            if (!c.boshmi()) bosh = false;
            m[k] = c;
        }
        if (!bosh) xom.push_back({r, m});
    }
    if (xom.empty()) {
        xatolar.push_back(L"Faylda birorta ham tovar qatori yo'q.");
        return 0;
    }
    try {
        Tranzaksiya tr;
        baza.sor("INSERT INTO ombor_omboryangilanish (holat, fayl_nomi, izoh, yoqlarini_yashir, jami_qator, ozgarmagan,"
                 " yaratilgan, qollangan, bekor_qilingan) VALUES ('qoralama',?,?,?,0,0,?,NULL,NULL)")
            .bog(1, nom.substr(0, 255)).bog(2, tozala(izoh).substr(0, 200)).bog(3, yoqlarini_yashir ? 1 : 0)
            .bog(4, hozir_utc()).bajar();
        long long yid = baza.oxirgi_id();
        // ---- tahlil (tahlil_qil) ----
        auto tl = tovarlarni_ol(true);
        std::map<wstring, const Tovar*> kod_b, nom_b;
        for (auto& t : tl) { kod_b[t.kod] = &t; nom_b[t.nom] = &t; }
        struct T { int excel; Holat h; wstring xato, ogoh; };
        std::vector<T> tek;
        std::map<wstring, int> fk, fn;
        for (auto& [r, m] : xom) {
            T t{r};
            qatorni_tekshir(m, t.h, t.xato, t.ogoh);
            wstring kod = t.h.count("kod") ? t.h["kod"] : L"", nm = t.h.count("nom") ? t.h["nom"] : L"";
            if (t.xato.empty() && !kod.empty()) {
                if (fk.count(kod)) t.xato = L"kod " + kod + L" faylda takrorlangan (" + std::to_wstring(fk[kod]) + L"-qatorda ham bor)";
                else fk[kod] = r;
            }
            if (t.xato.empty() && !nm.empty()) {
                if (fn.count(nm)) t.xato = L"bu nom faylda takrorlangan (" + std::to_wstring(fn[nm]) + L"-qatorda ham bor)";
                else fn[nm] = r;
            }
            if (t.xato.empty() && kod.empty() && !t.h.count("nom")) t.xato = L"kod ham, nom ham yo'q";
            tek.push_back(t);
        }
        std::set<wstring> band;
        for (auto& [k, p] : kod_b) band.insert(k);
        for (auto& [k, p] : fk) band.insert(k);
        KodBeruvchi kodlar(band);
        std::set<long long> ishlatilgan;
        std::map<size_t, std::pair<bool, YangiQator>> natija;  // bool — o'zgarmagan
        long long ozgarmagan = 0;
        for (int otish = 1; otish <= 2; ++otish) {
            for (size_t idx = 0; idx < tek.size(); ++idx) {
                if (natija.count(idx)) continue;
                T& t = tek[idx];
                wstring kod = t.h.count("kod") ? t.h["kod"] : L"";
                if (!t.xato.empty()) {
                    if (otish == 2) {
                        YangiQator q;
                        q.amal = L"xato"; q.excel_qator = t.excel; q.kod = kod.substr(0, 4);
                        q.yangi = t.h; q.yangi_bor = true; q.xato = t.xato; q.ogoh = t.ogoh;
                        natija[idx] = {false, q};
                    }
                    continue;
                }
                const Tovar* m = nullptr;
                if (otish == 1) {
                    auto it = kod_b.find(kod);
                    if (kod.empty() || it == kod_b.end()) continue;
                    m = it->second;
                } else {
                    auto it = nom_b.find(t.h.count("nom") ? t.h["nom"] : L"\x01");
                    m = it == nom_b.end() ? nullptr : it->second;
                    if (m && ishlatilgan.count(m->id)) m = nullptr;
                }
                YangiQator q;
                q.excel_qator = t.excel;
                q.ogoh = t.ogoh;
                if (m) {
                    ishlatilgan.insert(m->id);
                    Holat eski = tovar_holati(*m), yangi = birlashtir(eski, t.h);
                    kelishni_tekisla(yangi);
                    if (kod.empty()) yangi["kod"] = eski["kod"];
                    if (yangi == eski) {
                        ++ozgarmagan;
                        natija[idx] = {true, q};
                    } else {
                        q.amal = L"ozgardi"; q.mahsulot_id = m->id; q.kod = yangi["kod"];
                        q.eski = eski; q.eski_bor = true; q.yangi = yangi; q.yangi_bor = true;
                        natija[idx] = {false, q};
                    }
                } else if (!t.h.count("nom") || t.h["nom"].empty()) {
                    q.amal = L"xato"; q.kod = kod; q.yangi = t.h; q.yangi_bor = true;
                    q.xato = kod + L" kodli tovar omborda yo'q, yangi tovar uchun nomi kerak";
                    natija[idx] = {false, q};
                } else {
                    Holat yangi = birlashtir(bosh_holat(), t.h);
                    kelishni_tekisla(yangi);
                    if (kod.empty()) {
                        yangi["kod"] = kodlar.ber();
                        if (yangi["kod"].empty()) {
                            q.amal = L"xato"; q.yangi = t.h; q.yangi_bor = true;
                            q.xato = L"bo'sh kod qolmadi (9999 to'lgan)";
                            natija[idx] = {false, q};
                            continue;
                        }
                    }
                    q.amal = L"yangi"; q.kod = yangi["kod"]; q.yangi = yangi; q.yangi_bor = true;
                    natija[idx] = {false, q};
                }
            }
        }
        for (auto& [idx, p] : natija)
            if (!p.first) qator_yoz(yid, p.second);
        if (yoqlarini_yashir)
            for (auto& t : tl)
                if (!ishlatilgan.count(t.id) && t.faol) {
                    YangiQator q;
                    q.amal = L"yashirildi"; q.mahsulot_id = t.id; q.kod = t.kod;
                    q.eski = tovar_holati(t); q.eski_bor = true;
                    q.yangi = q.eski; q.yangi["faol"] = L"false"; q.yangi_bor = true;
                    qator_yoz(yid, q);
                }
        baza.sor("UPDATE ombor_omboryangilanish SET jami_qator=?, ozgarmagan=? WHERE id=?")
            .bog(1, (long long)xom.size()).bog(2, ozgarmagan).bog(3, yid).bajar();
        tr.tasdiq();
        return yid;
    } catch (const BazaXato& e) {
        xatolar.push_back(L"Baza xatosi: " + W(e.what()));
        return 0;
    }
}

// ======================= O'qish =======================

static const char* Y_SQL =
    "SELECT y.id, y.holat, y.fayl_nomi, y.izoh, y.yoqlarini_yashir, y.jami_qator, y.ozgarmagan, y.yaratilgan,"
    " COALESCE(y.qollangan,''), COALESCE(y.bekor_qilingan,''),"
    " (SELECT count(*) FROM ombor_yangilanishqatori q WHERE q.yangilanish_id=y.id AND q.amal='yangi' AND q.chiqarilgan=0),"
    " (SELECT count(*) FROM ombor_yangilanishqatori q WHERE q.yangilanish_id=y.id AND q.amal='ozgardi' AND q.chiqarilgan=0),"
    " (SELECT count(*) FROM ombor_yangilanishqatori q WHERE q.yangilanish_id=y.id AND q.amal='yashirildi' AND q.chiqarilgan=0),"
    " (SELECT count(*) FROM ombor_yangilanishqatori q WHERE q.yangilanish_id=y.id AND q.amal='xato' AND q.chiqarilgan=0),"
    " (SELECT count(*) FROM ombor_yangilanishqatori q WHERE q.yangilanish_id=y.id AND q.chiqarilgan=1)"
    " FROM ombor_omboryangilanish y ";

static Yangilanish y_qatori(Sorov& s) {
    Yangilanish y;
    y.id = s.butun(0);
    y.holat = s.matn(1);
    y.fayl_nomi = s.matn(2);
    y.izoh = s.matn(3);
    y.yoqlarini_yashir = s.butun(4) != 0;
    y.jami_qator = s.butun(5);
    y.ozgarmagan = s.butun(6);
    y.yaratilgan = s.matnU(7);
    y.qollangan = s.matnU(8);
    y.bekor_qilingan = s.matnU(9);
    y.n_yangi = s.butun(10);
    y.n_ozgardi = s.butun(11);
    y.n_yashirildi = s.butun(12);
    y.n_xato = s.butun(13);
    y.n_chiqarilgan = s.butun(14);
    return y;
}

std::vector<Yangilanish> yangilanishlar_royxati(const wstring& holat) {
    std::vector<Yangilanish> r;
    Sorov s = baza.sor(string(Y_SQL) + (holat.empty() ? "" : "WHERE y.holat=? ") + "ORDER BY y.yaratilgan DESC, y.id DESC");
    if (!holat.empty()) s.bog(1, holat);
    while (s.qator()) r.push_back(y_qatori(s));
    return r;
}

bool yangilanish_ol(long long id, Yangilanish& y) {
    Sorov s = baza.sor(string(Y_SQL) + "WHERE y.id=?");
    s.bog(1, id);
    if (!s.qator()) return false;
    y = y_qatori(s);
    return true;
}

long long qoralamalar_soni() {
    Sorov s = baza.sor("SELECT count(*) FROM ombor_omboryangilanish WHERE holat='qoralama'");
    return s.qator() ? s.butun(0) : 0;
}

static YQator yq_qatori(Sorov& s) {
    YQator q;
    q.id = s.butun(0);
    q.excel_qator = s.bormi(1) ? s.butun(1) : 0;
    q.amal = s.matn(2);
    q.eski_bor = s.bormi(3);
    if (q.eski_bor) q.eski = json_oqi(s.matnU(3));
    q.yangi_bor = s.bormi(4);
    if (q.yangi_bor) q.yangi = json_oqi(s.matnU(4));
    q.xato = s.matn(5);
    q.ogoh = s.matn(6);
    q.chiqarilgan = s.butun(7) != 0;
    q.mahsulot_id = s.bormi(8) ? s.butun(8) : 0;
    q.kod = s.matn(9);
    return q;
}

static const char* YQ_SQL = "SELECT id, excel_qator, amal, eski, yangi, xato, ogohlantirish, chiqarilgan, mahsulot_id,"
                            " kod, yangilanish_id FROM ombor_yangilanishqatori ";

std::vector<YQator> yangilanish_qatorlari(long long id, const string& turi, const wstring& matn) {
    std::vector<YQator> r;
    string sh = "WHERE yangilanish_id=?";
    if (turi == "chiqarilgan") sh += " AND chiqarilgan=1";
    else if (!turi.empty()) sh += " AND amal=? AND chiqarilgan=0";
    Sorov s = baza.sor(string(YQ_SQL) + sh + " ORDER BY id");
    s.bog(1, id);
    if (!turi.empty() && turi != "chiqarilgan") s.bog(2, turi);
    wstring m = kichik(tozala(matn));
    while (s.qator()) {
        YQator q = yq_qatori(s);
        if (!m.empty()) {
            bool mos = q.kod.rfind(m, 0) == 0;
            if (!mos && q.yangi.count("nom")) mos = kichik(q.yangi["nom"]).find(m) != wstring::npos;
            if (!mos && q.eski.count("nom")) mos = kichik(q.eski["nom"]).find(m) != wstring::npos;
            if (!mos) continue;
        }
        r.push_back(q);
    }
    return r;
}

bool yqator_ol(long long qid, YQator& q, long long& yid) {
    Sorov s = baza.sor(string(YQ_SQL) + "WHERE id=?");
    s.bog(1, qid);
    if (!s.qator()) return false;
    q = yq_qatori(s);
    yid = s.butun(10);
    return true;
}

// ======================= Qo'llash va bekor qilish =======================

static wstring vaqtinchalik(int i) {
    const wchar_t* b = L"0123456789abcdefghijklmnopqrstuvwxyz";
    wstring s;
    for (int k = 0; k < 4; ++k) { s.insert(s.begin(), b[i % 36]); i /= 36; }
    return L"~" + s;
}

static void holatni_qoy(long long id, const Holat& h) {
    string sql = "UPDATE ombor_mahsulot SET ";
    std::vector<string> m;
    for (auto& k : HOLAT_MAYDONLARI)
        if (h.count(k)) m.push_back(k);
    for (size_t i = 0; i < m.size(); ++i) sql += (i ? ", " : "") + m[i] + "=?";
    if (m.empty()) return;
    Sorov s = baza.sor(sql + " WHERE id=?");
    int i = 1;
    for (auto& k : m) {
        const wstring& v = h.at(k);
        if (k == "faol") s.bog(i++, v == L"true" ? 1 : 0);
        else if (k == "qoldiq" || k == "narx") { double d = 0; songa(v, d); s.bog(i++, d); }
        else if (k == "kelish_narxi") { if (v.empty()) s.bosh(i++); else { double d = 0; songa(v, d); s.bog(i++, d); } }
        else s.bog(i++, v);
    }
    s.bog(i, id).bajar();
}

static void holatlarni_yoz(const std::vector<std::pair<long long, Holat>>& juftlar, const wstring& izoh) {
    std::map<long long, double> eski_qoldiq;
    int n = 0;
    for (auto& [id, h] : juftlar) {
        Tovar t;
        tovar_ol(id, t);
        eski_qoldiq[id] = t.qoldiq;
        bool alm = (h.count("kod") && h.at("kod") != t.kod) || (h.count("nom") && h.at("nom") != t.nom);
        if (alm)
            baza.sor("UPDATE ombor_mahsulot SET kod=?, nom=? WHERE id=?")
                .bog(1, vaqtinchalik(n++)).bog(2, L"~" + std::to_wstring(id) + L"~").bog(3, id).bajar();
    }
    string vaqt = hozir_utc();
    for (auto& [id, h] : juftlar) {
        holatni_qoy(id, h);
        Tovar t;
        tovar_ol(id, t);
        double farq = yaxlit(t.qoldiq - eski_qoldiq[id], 3);
        if (farq != 0)
            baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id, kiritilgan_birlik,"
                     " kiritilgan_miqdor) VALUES ('tuzatish',?,?,?,?,'',NULL)")
                .bog(1, farq).bog(2, izoh.substr(0, 200)).bog(3, vaqt).bog(4, id).bajar();
    }
}

static std::vector<YQator> amaldagi_qatorlar(long long id) {
    std::vector<YQator> r;
    for (auto& q : yangilanish_qatorlari(id, "", L""))
        if (!q.chiqarilgan && q.amal != L"xato") r.push_back(q);
    return r;
}

bool qollash(long long id, std::vector<wstring>& xatolar) {
    xatolar.clear();
    Yangilanish y;
    if (!yangilanish_ol(id, y) || y.holat != L"qoralama") {
        xatolar.push_back(L"Faqat qoralamani qo'llash mumkin.");
        return false;
    }
    wstring izoh = L"Excel #" + std::to_wstring(id) + L": " + y.nomi();
    try {
        Tranzaksiya tr;
        auto qs = amaldagi_qatorlar(id);
        wstring yoq;
        for (auto& q : qs)
            if (q.amal != L"yangi" && !q.mahsulot_id) yoq += (yoq.empty() ? L"" : L", ") + q.kod;
        if (!yoq.empty()) {
            xatolar.push_back(L"Bu tovarlar qoralama tuzilgandan keyin o'chirilgan: " + yoq + L". Ularni chiqarib tashlang.");
            return false;
        }
        // yakuniy to'qnashuvlar: bir kod yoki nom ikki tovarga tushmasin
        std::map<long long, const Holat*> ozg;
        for (auto& q : qs)
            if ((q.amal == L"ozgardi" || q.amal == L"yashirildi") && q.mahsulot_id) ozg[q.mahsulot_id] = &q.yangi;
        for (auto [maydon, nomi] : {std::pair<const char*, const wchar_t*>{"kod", L"kod"}, {"nom", L"nom"}}) {
            std::map<wstring, std::vector<wstring>> band;
            Sorov s = baza.sor(string("SELECT id, ") + maydon + " FROM ombor_mahsulot");
            while (s.qator()) {
                long long pk = s.butun(0);
                wstring v = s.matn(1);
                auto it = ozg.find(pk);
                if (it != ozg.end() && it->second->count(maydon)) v = it->second->at(maydon);
                band[v].push_back(L"tovar #" + std::to_wstring(pk));
            }
            for (auto& q : qs)
                if (q.amal == L"yangi")
                    band[q.yangi[maydon]].push_back((q.excel_qator ? std::to_wstring(q.excel_qator) : L"—") + L"-qator");
            for (auto& [k, v] : band)
                if (v.size() > 1 && xatolar.size() < 20) {
                    wstring j;
                    for (auto& x : v) j += (j.empty() ? L"" : L", ") + x;
                    xatolar.push_back(wstring(nomi) + L" «" + k + L"» ikki joyda: " + j);
                }
        }
        if (!xatolar.empty()) return false;
        std::vector<std::pair<long long, Holat>> mavjud;
        for (auto& q : qs) {
            if (q.amal == L"yangi") continue;
            Holat farq;
            for (auto& [k, v] : q.yangi) {
                auto it = q.eski.find(k);
                if (it == q.eski.end() || it->second != v) farq[k] = v;
            }
            q.eski = mahsulot_holati(q.mahsulot_id);
            q.yangi = birlashtir(q.eski, farq);
            mavjud.push_back({q.mahsulot_id, q.yangi});
            baza.sor("UPDATE ombor_yangilanishqatori SET eski=?, yangi=? WHERE id=?")
                .bog(1, json_yoz(q.eski)).bog(2, json_yoz(q.yangi)).bog(3, q.id).bajar();
        }
        holatlarni_yoz(mavjud, izoh);
        string vaqt = hozir_utc();
        for (auto& q : qs) {
            if (q.amal != L"yangi") continue;
            Holat& h = q.yangi;
            double qoldiq = 0, narx = 0, kelish = 0;
            songa(h["qoldiq"], qoldiq);
            songa(h["narx"], narx);
            Sorov s = baza.sor("INSERT INTO ombor_mahsulot (nom, birlik, qoldiq, faol, yaratilgan, olish_birligi, olish_miqdori,"
                               " valyuta, narx, kod, kelish_narxi, kelish_valyutasi, manba) VALUES (?,?,?,?,?,'',1,?,?,?,?,?,'{}')");
            s.bog(1, h["nom"]).bog(2, h["birlik"]).bog(3, qoldiq).bog(4, h["faol"] == L"false" ? 0 : 1).bog(5, vaqt);
            s.bog(6, h["valyuta"]).bog(7, narx).bog(8, h["kod"]);
            if (h["kelish_narxi"].empty()) s.bosh(9); else { songa(h["kelish_narxi"], kelish); s.bog(9, kelish); }
            s.bog(10, h["kelish_valyutasi"]).bajar();
            long long mid = baza.oxirgi_id();
            baza.sor("UPDATE ombor_yangilanishqatori SET mahsulot_id=? WHERE id=?").bog(1, mid).bog(2, q.id).bajar();
            if (qoldiq != 0)
                baza.sor("INSERT INTO ombor_omborharakati (tur, miqdor, izoh, sana, mahsulot_id, kiritilgan_birlik,"
                         " kiritilgan_miqdor) VALUES ('kirim',?,?,?,?,'',NULL)")
                    .bog(1, qoldiq).bog(2, (L"Boshlang'ich qoldiq (" + izoh + L")").substr(0, 200)).bog(3, vaqt)
                    .bog(4, mid).bajar();
        }
        baza.sor("UPDATE ombor_omboryangilanish SET holat='qollandi', qollangan=? WHERE id=?")
            .bog(1, hozir_utc()).bog(2, id).bajar();
        tr.tasdiq();
        return true;
    } catch (const BazaXato& e) {
        xatolar.push_back(L"Baza xatosi: " + W(e.what()));
        return false;
    }
}

std::vector<Toqnash> bekor_toqnashuvlari(long long id) {
    std::vector<Toqnash> r;
    Yangilanish y;
    if (!yangilanish_ol(id, y)) return r;
    auto qs = amaldagi_qatorlar(id);
    std::vector<std::pair<wstring, wstring>> t;
    std::set<long long> qaytadigan;
    for (auto& q : qs) {
        qaytadigan.insert(q.mahsulot_id);
        Tovar m;
        if (!q.mahsulot_id || !tovar_ol(q.mahsulot_id, m)) t.push_back({q.kod, L"tovar keyinroq o'chirilgan"});
        else if (!holat_mosmi(tovar_holati(m), q.yangi))
            t.push_back({m.kod, L"tovar keyin yana o'zgargan (sotuv, kirim yoki tahrir)"});
    }
    for (auto [maydon, nomi] : {std::pair<const char*, const wchar_t*>{"kod", L"kod"}, {"nom", L"nom"}}) {
        std::set<wstring> eskilar;
        for (auto& q : qs)
            if (q.amal == L"ozgardi" && q.eski[maydon] != q.yangi[maydon]) eskilar.insert(q.eski[maydon]);
        for (auto& e : eskilar) {
            Sorov s = baza.sor(string("SELECT id, kod FROM ombor_mahsulot WHERE ") + maydon + "=?");
            s.bog(1, e);
            while (s.qator())
                if (!qaytadigan.count(s.butun(0))) t.push_back({s.matn(1), wstring(L"eski ") + nomi + L"ni keyin boshqa tovar olgan"});
        }
    }
    for (auto& [kod, sabab] : t) {
        Toqnash x{kod, sabab, 0};
        Sorov s = baza.sor("SELECT DISTINCT y.id FROM ombor_omboryangilanish y JOIN ombor_yangilanishqatori q ON"
                           " q.yangilanish_id=y.id WHERE y.holat='qollandi' AND y.qollangan > ? AND (q.kod=? OR"
                           " json_extract(q.eski,'$.kod')=?) ORDER BY y.qollangan DESC LIMIT 1");
        s.bog(1, y.qollangan).bog(2, kod).bog(3, kod);
        if (s.qator()) x.keyingi = s.butun(0);
        r.push_back(x);
    }
    return r;
}

bool bekor_qil(long long id, std::vector<wstring>& xatolar) {
    xatolar.clear();
    Yangilanish y;
    if (!yangilanish_ol(id, y) || y.holat != L"qollandi") {
        xatolar.push_back(L"Faqat qo'llangan yangilanishni bekor qilish mumkin.");
        return false;
    }
    wstring izoh = L"Excel #" + std::to_wstring(id) + L" bekor qilindi";
    try {
        Tranzaksiya tr;
        auto t = bekor_toqnashuvlari(id);
        if (!t.empty()) {
            for (size_t i = 0; i < t.size() && i < 20; ++i) xatolar.push_back(t[i].kod + L": " + t[i].sabab);
            return false;
        }
        auto qs = amaldagi_qatorlar(id);
        for (auto& q : qs) {
            if (q.amal != L"yangi" || !q.mahsulot_id) continue;
            Sorov s = baza.sor("SELECT (SELECT count(*) FROM sotuv_sotuvqator WHERE mahsulot_id=?) +"
                               " (SELECT count(*) FROM qarz_qarzqator WHERE mahsulot_id=?)");
            s.bog(1, q.mahsulot_id).bog(2, q.mahsulot_id);
            if (s.qator() && s.butun(0) > 0) {
                xatolar.push_back(L"Yangi qo'shilgan tovarlarning ba'zisi allaqachon sotuv yoki qarzga yozilgan — bekor qilib bo'lmaydi.");
                return false;
            }
        }
        for (auto& q : qs) {
            if (q.amal != L"yangi" || !q.mahsulot_id) continue;
            baza.sor("DELETE FROM ombor_omborharakati WHERE mahsulot_id=?").bog(1, q.mahsulot_id).bajar();
            baza.sor("UPDATE ombor_yangilanishqatori SET mahsulot_id=NULL WHERE mahsulot_id=?").bog(1, q.mahsulot_id).bajar();
            baza.sor("DELETE FROM ombor_mahsulot WHERE id=?").bog(1, q.mahsulot_id).bajar();
        }
        std::vector<std::pair<long long, Holat>> j;
        for (auto& q : qs)
            if (q.amal != L"yangi") j.push_back({q.mahsulot_id, q.eski});
        holatlarni_yoz(j, izoh);
        baza.sor("UPDATE ombor_omboryangilanish SET holat='bekor', bekor_qilingan=? WHERE id=?")
            .bog(1, hozir_utc()).bog(2, id).bajar();
        tr.tasdiq();
        return true;
    } catch (const BazaXato& e) {
        xatolar.push_back(L"Baza xatosi: " + W(e.what()));
        return false;
    }
}

bool qoralamani_ochir(long long id, wstring& xato) {
    Yangilanish y;
    if (!yangilanish_ol(id, y) || y.holat != L"qoralama") {
        xato = L"Faqat qoralamani o'chirish mumkin.";
        return false;
    }
    Tranzaksiya tr;
    baza.sor("DELETE FROM ombor_yangilanishqatori WHERE yangilanish_id=?").bog(1, id).bajar();
    baza.sor("DELETE FROM ombor_omboryangilanish WHERE id=?").bog(1, id).bajar();
    tr.tasdiq();
    return true;
}

long long xatolarni_chiqar(long long id) {
    baza.sor("UPDATE ombor_yangilanishqatori SET chiqarilgan=1 WHERE yangilanish_id=? AND amal='xato' AND chiqarilgan=0")
        .bog(1, id).bajar();
    return sqlite3_changes(baza.h());
}

bool qator_chiqar(long long qid, wstring& xato) {
    YQator q;
    long long yid;
    Yangilanish y;
    if (!yqator_ol(qid, q, yid) || !yangilanish_ol(yid, y)) return false;
    if (y.holat != L"qoralama") {
        xato = L"Qo'llangan yangilanishni tahrirlab bo'lmaydi — kerak bo'lsa uni bekor qiling.";
        return false;
    }
    baza.sor("UPDATE ombor_yangilanishqatori SET chiqarilgan=? WHERE id=?").bog(1, q.chiqarilgan ? 0 : 1).bog(2, qid).bajar();
    return true;
}

// ======================= Qatorni tuzatish =======================

static long long mos_mahsulot(const Holat& h) {
    wstring kod = h.count("kod") ? h.at("kod") : L"", nom = h.count("nom") ? h.at("nom") : L"";
    if (!kod.empty()) {
        Sorov s = baza.sor("SELECT id FROM ombor_mahsulot WHERE kod=?");
        s.bog(1, kod);
        if (s.qator()) return s.butun(0);
    }
    if (!nom.empty()) {
        Sorov s = baza.sor("SELECT id FROM ombor_mahsulot WHERE nom=?");
        s.bog(1, nom);
        if (s.qator()) return s.butun(0);
    }
    return 0;
}

Holat qator_formasi_uchun(long long qid) {
    YQator q;
    long long yid;
    if (!yqator_ol(qid, q, yid)) return {};
    Holat asos = bosh_holat();
    if (q.eski_bor) asos = q.eski;
    else if (q.amal == L"xato") {
        Holat kn;
        for (auto k : {"kod", "nom"})
            if (q.yangi.count(k)) kn[k] = q.yangi[k];
        long long m = mos_mahsulot(kn);
        if (m) asos = mahsulot_holati(m);
    }
    return birlashtir(asos, q.yangi);
}

wstring qatorni_saqla(long long qid, const Holat& kirish) {
    YQator q;
    long long yid;
    Yangilanish y;
    if (!yqator_ol(qid, q, yid) || !yangilanish_ol(yid, y)) return L"Qator topilmadi.";
    if (y.holat != L"qoralama") return L"Qo'llangan yangilanishni tahrirlab bo'lmaydi — kerak bo'lsa uni bekor qiling.";
    // formadagi qiymatlar Excel kataklari kabi tekshiriladi (QatorForm.clean)
    std::map<string, Katak> xom;
    for (auto& m : USTUNLAR) {
        Katak k;
        auto it = kirish.find(m);
        if (it != kirish.end() && !tozala(it->second).empty()) { k.tur = Katak::MATN; k.matn = it->second; }
        xom[m] = k;
    }
    if (tozala(xom["nom"].matn).empty()) return L"Tovar nomi: Bu maydon to'ldirilishi shart.";
    Holat holat;
    wstring xato, ogoh;
    qatorni_tekshir(xom, holat, xato, ogoh);
    if (!xato.empty()) return xato;
    Holat asl = qator_formasi_uchun(qid);
    holat["faol"] = asl.count("faol") ? asl["faol"] : L"true";
    // qatorni_yangila
    auto boshqalar = yangilanish_qatorlari(yid, "", L"");
    boshqalar.erase(std::remove_if(boshqalar.begin(), boshqalar.end(), [&](const YQator& b) {
                        return b.id == qid || b.amal == L"xato" || b.chiqarilgan;
                    }),
                    boshqalar.end());
    long long mid = mos_mahsulot(holat);
    Holat eski, yangi;
    wstring amal;
    bool eski_bor = false;
    if (mid) {
        for (auto& b : boshqalar)
            if (b.mahsulot_id == mid && b.amal != L"yashirildi") {
                Tovar t;
                tovar_ol(mid, t);
                return L"Bu tovar (" + t.kod + L") qoralamaning boshqa qatorida ham bor" +
                       (b.excel_qator ? L" (" + std::to_wstring(b.excel_qator) + L"-qator)" : L"") + L".";
            }
        eski = mahsulot_holati(mid);
        eski_bor = true;
        yangi = birlashtir(eski, holat);
        kelishni_tekisla(yangi);
        if (holat["kod"].empty()) yangi["kod"] = eski["kod"];
        amal = L"ozgardi";
    } else {
        yangi = birlashtir(bosh_holat(), holat);
        kelishni_tekisla(yangi);
        amal = L"yangi";
        if (yangi["kod"].empty()) {
            std::set<wstring> band;
            Sorov s = baza.sor("SELECT kod FROM ombor_mahsulot");
            while (s.qator()) band.insert(s.matn(0));
            for (auto& b : boshqalar) band.insert(b.kod);
            yangi["kod"] = KodBeruvchi(band).ber();
        }
    }
    for (auto [maydon, nomi] : {std::pair<const char*, const wchar_t*>{"kod", L"Kod"}, {"nom", L"Nom"}}) {
        wstring v = yangi[maydon];
        for (auto& b : boshqalar)
            if (b.amal != L"yashirildi" && b.yangi.count(maydon) && b.yangi.at(maydon) == v)
                return wstring(nomi) + L" «" + v + L"» qoralamaning boshqa qatorida ham bor" +
                       (b.excel_qator ? L" (" + std::to_wstring(b.excel_qator) + L"-qator)" : L"") + L".";
        Sorov s = baza.sor(string("SELECT id, kod, nom FROM ombor_mahsulot WHERE ") + maydon + "=? AND id<>?");
        s.bog(1, v).bog(2, mid);
        if (s.qator()) {
            long long egasi = s.butun(0);
            bool ozgaradi = false;
            for (auto& b : boshqalar) ozgaradi |= b.mahsulot_id == egasi && b.amal == L"ozgardi";
            if (!ozgaradi)
                return wstring(nomi) + L" «" + v + L"» omborda boshqa tovarda band: " + s.matn(1) + L" " + s.matn(2) + L".";
        }
    }
    Tranzaksiya tr;
    if (mid)
        baza.sor("DELETE FROM ombor_yangilanishqatori WHERE yangilanish_id=? AND id<>? AND mahsulot_id=? AND amal='yashirildi'")
            .bog(1, yid).bog(2, qid).bog(3, mid).bajar();
    Sorov s = baza.sor("UPDATE ombor_yangilanishqatori SET mahsulot_id=?, amal=?, eski=?, yangi=?, kod=?, xato='',"
                       " ogohlantirish=?, chiqarilgan=0 WHERE id=?");
    if (mid) s.bog(1, mid); else s.bosh(1);
    s.bog(2, amal);
    if (eski_bor) s.bog(3, json_yoz(eski)); else s.bosh(3);
    s.bog(4, json_yoz(yangi)).bog(5, yangi["kod"].substr(0, 4)).bog(6, ogoh.substr(0, 300)).bog(7, qid).bajar();
    tr.tasdiq();
    return L"";
}

// ======================= Yuklab olish =======================

bool excel_eksport(const wstring& fayl, const std::vector<long long>& idlar, wstring& xato) {
    YVaraq v;
    v.nom = L"Ombor";
    v.enlar = {9, 60, 10, 12, 14, 10, 14, 16};
    v.sarlavha_boyi = 24;
    v.muzlatish = L"C2";
    std::vector<YKatak> sar;
    for (auto& m : USTUNLAR) sar.push_back({YKatak::MATN, USTUN_NOMI.at(m), 0, 1});
    v.qatorlar.push_back(sar);
    auto val = [](const wstring& x) { return x == L"dollar" ? L"USD" : L"UZS"; };
    for (long long id : idlar) {
        Tovar t;
        if (!tovar_ol(id, t)) continue;
        std::vector<YKatak> q = {{YKatak::MATN, t.kod, 0, 2}, {YKatak::MATN, t.nom, 0, 0},
                                 {YKatak::MATN, t.birlik, 0, 0}, {YKatak::SON, L"", t.qoldiq, 3},
                                 {YKatak::SON, L"", t.narx, 4}, {YKatak::MATN, val(t.valyuta), 0, 0}};
        if (t.kelish_bor) {
            q.push_back({YKatak::SON, L"", t.kelish_narxi, 4});
            q.push_back({YKatak::MATN, val(t.kelish_valyutasi), 0, 0});
        }
        v.qatorlar.push_back(q);
    }
    size_t n = v.qatorlar.size(), oxiri = std::max<size_t>(n, 2) + 2000;
    // Kod ustuni matn formatida — 0001 dagi nollar yo'qolmasin
    while (v.qatorlar.size() < oxiri) v.qatorlar.push_back({{YKatak::BOSH, L"", 0, 2}});
    v.avtofiltr = L"A1:H" + std::to_wstring(n);
    wstring o = std::to_wstring(oxiri), birl;
    for (auto& b : BIRLIKLAR) birl += (birl.empty() ? L"" : L",") + b;
    v.tekshiruvlar = {{L"UZS,USD", L"F2:F" + o}, {L"UZS,USD", L"H2:H" + o}, {birl, L"C2:C" + o}};

    YVaraq y;
    y.nom = L"Yo'riqnoma";
    y.enlar = {18, 95};
    y.qatorlar.push_back({{YKatak::MATN, L"Ombor shabloni", 0, 7}});
    wstring birl2;
    for (auto& b : BIRLIKLAR) birl2 += (birl2.empty() ? L"" : L", ") + b;
    std::vector<std::pair<wstring, wstring>> bandlar = {
        {L"Kod", L"4 xonali: 0001 … 9999 — tovar turgan joyi. Bo'sh qolsa navbatdagi bo'sh kod beriladi. Tovar avval kod bo'yicha topiladi."},
        {L"Tovar nomi", L"Kod bo'lmasa tovar nomi bo'yicha topiladi (harfma-harf bir xil)."},
        {L"Birlik", birl2 + L" — bo'sh bo'lsa dona."},
        {L"Qoldiq", L"Omborda qolgan miqdor (kasr bo'lishi mumkin: 12,5). O'zgarsa ombor tarixiga «Tuzatish» bo'lib yoziladi."},
        {L"Narxi", L"Faqat son (180726 yoki 2,65)."},
        {L"Valyuta", L"UZS (so'm) yoki USD (dollar)."},
        {L"Kelish narxi", L"Xarid narxi, 1 sotuv birligi uchun (2,2 yoki «2.2 USD»). Bo'sh katak — kelish narxi noma'lum."},
        {L"Kelish valyutasi", L"UZS yoki USD. Bo'sh bo'lsa tovarning valyutasi olinadi."},
        {L"", L""},
        {L"Qanday ishlaydi", L"1) Fayl yuklanadi — ombor hali o'zgarmaydi."},
        {L"", L"2) Nima o'zgarishi ko'rsatiladi: yangi, o'zgargan, xato qatorlar."},
        {L"", L"3) Kerak bo'lsa qatorni tuzatasiz yoki chiqarib tashlaysiz, keyin «Qo'llash»."},
        {L"", L"4) Xato bo'lsa «Bekor qilish» — ombor avvalgi holatiga qaytadi."},
        {L"", L""},
        {L"Muhim", L"Faylda yo'q ustun o'zgarmaydi: faqat «Kod» va «Qoldiq» ustunli fayl faqat qoldiqni yangilaydi."},
        {L"", L"Faqat «Kod» (yoki «Tovar nomi») va «Kelish narxi» ustunli fayl faqat xarid narxini yangilaydi — yashirilgan tovarlar yashirinligicha qoladi."},
        {L"", L"Do'kon dasturining «Products …xlsx» faylini ham to'g'ridan-to'g'ri yuklash mumkin."},
    };
    for (auto& [a, b] : bandlar)
        y.qatorlar.push_back({{a.empty() ? YKatak::BOSH : YKatak::MATN, a, 0, 5}, {b.empty() ? YKatak::BOSH : YKatak::MATN, b, 0, 6}});
    return xlsx_yoz(fayl, {v, y}, xato);
}
