#include "xlsx.h"

#include <cstdint>
#include <cstring>

// ======================= Fayl =======================

static bool fayl_oqi(const wstring& yol, string& d) {
    FILE* f = _wfopen(yol.c_str(), L"rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    d.resize(n > 0 ? n : 0);
    bool ok = n <= 0 || fread(d.data(), 1, n, f) == (size_t)n;
    fclose(f);
    return ok;
}

// ======================= Inflate (RFC 1951) =======================

namespace {

struct Bitlar {
    const uint8_t* p;
    size_t n, i = 0;
    uint32_t tag = 0;
    int soni = 0;
    bool xato = false;
    int bit() {
        if (!soni) {
            if (i >= n) { xato = true; return 0; }
            tag = p[i++];
            soni = 8;
        }
        int b = tag & 1;
        tag >>= 1;
        --soni;
        return b;
    }
    unsigned ol(int k) {
        unsigned v = 0;
        for (int j = 0; j < k; ++j) v |= (unsigned)bit() << j;
        return v;
    }
};

struct Daraxt {
    uint16_t sanoq[16];
    uint16_t belgi[320];
};

void daraxt_qur(Daraxt& t, const uint8_t* uz, int n) {
    memset(t.sanoq, 0, sizeof t.sanoq);
    for (int i = 0; i < n; ++i) t.sanoq[uz[i]]++;
    t.sanoq[0] = 0;
    uint16_t joy[16], s = 0;
    for (int i = 0; i < 16; ++i) { joy[i] = s; s += t.sanoq[i]; }
    for (int i = 0; i < n; ++i)
        if (uz[i]) t.belgi[joy[uz[i]]++] = (uint16_t)i;
}

int dekod(Bitlar& b, const Daraxt& t) {
    int s = 0, joriy = 0, uz = 0;
    do {
        joriy = 2 * joriy + b.bit();
        if (++uz > 15) { b.xato = true; return 0; }
        s += t.sanoq[uz];
        joriy -= t.sanoq[uz];
    } while (joriy >= 0);
    return t.belgi[s + joriy];
}

const uint16_t UZ_ASOS[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                              35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const uint8_t UZ_BIT[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                            3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const uint16_t MS_ASOS[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
                              193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193,
                              12289, 16385, 24577};
const uint8_t MS_BIT[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                            7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

bool blok(Bitlar& b, string& chiq, const Daraxt& lt, const Daraxt& dt) {
    for (;;) {
        int s = dekod(b, lt);
        if (b.xato) return false;
        if (s < 256) chiq += (char)s;
        else if (s == 256) return true;
        else {
            s -= 257;
            if (s >= 29) return false;
            size_t uz = UZ_ASOS[s] + b.ol(UZ_BIT[s]);
            int d = dekod(b, dt);
            if (d >= 30 || b.xato) return false;
            size_t ms = MS_ASOS[d] + b.ol(MS_BIT[d]);
            if (ms > chiq.size()) return false;
            size_t bosh = chiq.size() - ms;
            for (size_t k = 0; k < uz; ++k) chiq += chiq[bosh + k];
        }
    }
}

bool inflate(const uint8_t* p, size_t n, string& chiq) {
    Bitlar b{p, n};
    Daraxt lt, dt;
    int oxirgi;
    do {
        oxirgi = b.bit();
        int tur = (int)b.ol(2);
        if (tur == 0) {
            b.soni = 0;  // baytga tekislash
            if (b.i + 4 > n) return false;
            unsigned uz = p[b.i] | p[b.i + 1] << 8;
            b.i += 4;
            if (b.i + uz > n) return false;
            chiq.append((const char*)p + b.i, uz);
            b.i += uz;
        } else if (tur == 1) {
            uint8_t uz[288];
            for (int i = 0; i < 144; ++i) uz[i] = 8;
            for (int i = 144; i < 256; ++i) uz[i] = 9;
            for (int i = 256; i < 280; ++i) uz[i] = 7;
            for (int i = 280; i < 288; ++i) uz[i] = 8;
            daraxt_qur(lt, uz, 288);
            uint8_t du[30];
            for (int i = 0; i < 30; ++i) du[i] = 5;
            daraxt_qur(dt, du, 30);
            if (!blok(b, chiq, lt, dt)) return false;
        } else if (tur == 2) {
            static const uint8_t TARTIB[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
            int hlit = b.ol(5) + 257, hdist = b.ol(5) + 1, hclen = b.ol(4) + 4;
            uint8_t cl[19] = {};
            for (int i = 0; i < hclen; ++i) cl[TARTIB[i]] = (uint8_t)b.ol(3);
            Daraxt ct;
            daraxt_qur(ct, cl, 19);
            uint8_t uz[320] = {};
            int k = 0;
            while (k < hlit + hdist) {
                int s = dekod(b, ct);
                if (b.xato) return false;
                if (s < 16) uz[k++] = (uint8_t)s;
                else {
                    int takror = 0;
                    uint8_t q = 0;
                    if (s == 16) {
                        if (!k) return false;
                        q = uz[k - 1];
                        takror = 3 + b.ol(2);
                    } else if (s == 17) takror = 3 + b.ol(3);
                    else takror = 11 + b.ol(7);
                    if (k + takror > hlit + hdist) return false;
                    while (takror--) uz[k++] = q;
                }
            }
            daraxt_qur(lt, uz, hlit);
            daraxt_qur(dt, uz + hlit, hdist);
            if (!blok(b, chiq, lt, dt)) return false;
        } else
            return false;
        if (b.xato) return false;
    } while (!oxirgi);
    return true;
}

// ======================= ZIP =======================

uint32_t u32(const string& d, size_t i) {
    return (uint8_t)d[i] | (uint8_t)d[i + 1] << 8 | (uint8_t)d[i + 2] << 16 | (uint32_t)(uint8_t)d[i + 3] << 24;
}
uint16_t u16(const string& d, size_t i) { return (uint8_t)d[i] | (uint8_t)d[i + 1] << 8; }

bool zip_ichi(const string& z, std::map<string, string>& fayllar) {
    if (z.size() < 22) return false;
    size_t e = string::npos;
    for (size_t i = z.size() - 22 + 1; i-- > 0 && z.size() - i < 70000;)
        if (u32(z, i) == 0x06054b50) { e = i; break; }
    if (e == string::npos) return false;
    size_t soni = u16(z, e + 10), cd = u32(z, e + 16);
    for (size_t k = 0; k < soni; ++k) {
        if (cd + 46 > z.size() || u32(z, cd) != 0x02014b50) return false;
        int usul = u16(z, cd + 10);
        size_t siq = u32(z, cd + 20), ochiq = u32(z, cd + 24);
        size_t nl = u16(z, cd + 28), el = u16(z, cd + 30), kl = u16(z, cd + 32);
        size_t lh = u32(z, cd + 42);
        string nom = z.substr(cd + 46, nl);
        cd += 46 + nl + el + kl;
        if (lh + 30 > z.size() || u32(z, lh) != 0x04034b50) return false;
        size_t data = lh + 30 + u16(z, lh + 26) + u16(z, lh + 28);
        if (data + siq > z.size()) return false;
        string m;
        if (usul == 0) m = z.substr(data, siq);
        else if (usul == 8) {
            m.reserve(ochiq);
            if (!inflate((const uint8_t*)z.data() + data, siq, m)) return false;
        } else
            continue;
        fayllar[nom] = std::move(m);
    }
    return true;
}

uint32_t crc32(const string& d) {
    static uint32_t jadval[256];
    static bool tayyor = false;
    if (!tayyor) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = c & 1 ? 0xEDB88320 ^ (c >> 1) : c >> 1;
            jadval[i] = c;
        }
        tayyor = true;
    }
    uint32_t c = 0xFFFFFFFF;
    for (unsigned char b : d) c = jadval[(c ^ b) & 255] ^ (c >> 8);
    return c ^ 0xFFFFFFFF;
}

void q32(string& s, uint32_t v) { for (int i = 0; i < 4; ++i) s += (char)(v >> (8 * i)); }
void q16(string& s, uint16_t v) { s += (char)v; s += (char)(v >> 8); }

string zip_yasa(const std::vector<std::pair<string, string>>& f) {
    string z, cd;
    for (auto& [nom, m] : f) {
        uint32_t crc = crc32(m), joy = (uint32_t)z.size();
        q32(z, 0x04034b50); q16(z, 20); q16(z, 0); q16(z, 0); q16(z, 0); q16(z, 0x21);
        q32(z, crc); q32(z, (uint32_t)m.size()); q32(z, (uint32_t)m.size());
        q16(z, (uint16_t)nom.size()); q16(z, 0);
        z += nom;
        z += m;
        q32(cd, 0x02014b50); q16(cd, 20); q16(cd, 20); q16(cd, 0); q16(cd, 0); q16(cd, 0); q16(cd, 0x21);
        q32(cd, crc); q32(cd, (uint32_t)m.size()); q32(cd, (uint32_t)m.size());
        q16(cd, (uint16_t)nom.size()); q16(cd, 0); q16(cd, 0); q16(cd, 0); q16(cd, 0); q32(cd, 0);
        q32(cd, joy);
        cd += nom;
    }
    uint32_t cdj = (uint32_t)z.size();
    z += cd;
    q32(z, 0x06054b50); q16(z, 0); q16(z, 0); q16(z, (uint16_t)f.size()); q16(z, (uint16_t)f.size());
    q32(z, (uint32_t)cd.size()); q32(z, cdj); q16(z, 0);
    return z;
}

// ======================= XML =======================

struct Teg {
    string nom;  // prefikssiz: "x:c" -> "c"
    std::map<string, string> attr;  // atribut nomi ham prefikssiz
    bool yopuvchi = false, ozi_yopiq = false;
};

string xml_ochir(const string& s) {
    string r;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') { r += s[i]; continue; }
        size_t e = s.find(';', i);
        if (e == string::npos) { r += s[i]; continue; }
        string n = s.substr(i + 1, e - i - 1);
        if (n == "amp") r += '&';
        else if (n == "lt") r += '<';
        else if (n == "gt") r += '>';
        else if (n == "quot") r += '"';
        else if (n == "apos") r += '\'';
        else if (!n.empty() && n[0] == '#') {
            unsigned long c = n.size() > 1 && (n[1] == 'x' || n[1] == 'X') ? strtoul(n.c_str() + 2, nullptr, 16)
                                                                            : strtoul(n.c_str() + 1, nullptr, 10);
            r += U(c < 0x10000 ? wstring(1, (wchar_t)c) : L"?");
        } else {
            r += s.substr(i, e - i + 1);
        }
        i = e;
    }
    return r;
}

string prefikssiz(const string& s) {
    size_t p = s.find(':');
    return p == string::npos ? s : s.substr(p + 1);
}

// Keyingi tegni o'qiydi; oldidagi matn `matn` ga tushadi. Oxirida false.
bool keyingi_teg(const string& x, size_t& i, Teg& t, string& matn) {
    matn.clear();
    for (;;) {
        size_t a = x.find('<', i);
        if (a == string::npos) return false;
        matn += x.substr(i, a - i);
        if (x.compare(a, 4, "<!--") == 0) { i = x.find("-->", a); if (i == string::npos) return false; i += 3; continue; }
        if (x.compare(a, 9, "<![CDATA[") == 0) {
            size_t e = x.find("]]>", a);
            if (e == string::npos) return false;
            matn += x.substr(a + 9, e - a - 9);
            i = e + 3;
            continue;
        }
        if (x[a + 1] == '?' || x[a + 1] == '!') { i = x.find('>', a); if (i == string::npos) return false; ++i; continue; }
        size_t e = a + 1;
        char q = 0;
        for (; e < x.size(); ++e) {
            if (q) { if (x[e] == q) q = 0; }
            else if (x[e] == '"' || x[e] == '\'') q = x[e];
            else if (x[e] == '>') break;
        }
        if (e >= x.size()) return false;
        string ich = x.substr(a + 1, e - a - 1);
        i = e + 1;
        t = Teg();
        if (!ich.empty() && ich[0] == '/') { t.yopuvchi = true; ich = ich.substr(1); }
        if (!ich.empty() && ich.back() == '/') { t.ozi_yopiq = true; ich.pop_back(); }
        size_t k = 0;
        while (k < ich.size() && !isspace((unsigned char)ich[k])) ++k;
        t.nom = prefikssiz(ich.substr(0, k));
        while (k < ich.size()) {
            while (k < ich.size() && isspace((unsigned char)ich[k])) ++k;
            size_t tng = ich.find('=', k);
            if (tng == string::npos) break;
            string an = ich.substr(k, tng - k);
            while (!an.empty() && isspace((unsigned char)an.back())) an.pop_back();
            k = tng + 1;
            while (k < ich.size() && isspace((unsigned char)ich[k])) ++k;
            if (k >= ich.size()) break;
            char qq = ich[k];
            size_t o = ich.find(qq, k + 1);
            if (o == string::npos) break;
            t.attr[prefikssiz(an)] = xml_ochir(ich.substr(k + 1, o - k - 1));
            k = o + 1;
        }
        return true;
    }
}

int ustun_raqami(const string& r) {  // "AB12" -> 27
    int n = 0;
    for (char c : r) {
        if (c >= 'A' && c <= 'Z') n = n * 26 + (c - 'A' + 1);
        else if (c >= 'a' && c <= 'z') n = n * 26 + (c - 'a' + 1);
        else break;
    }
    return n - 1;
}

}  // namespace

// ======================= O'qish =======================

bool xlsx_oqi(const wstring& fayl, Varaq& v, wstring& xato) {
    string z;
    std::map<string, string> f;
    if (!fayl_oqi(fayl, z) || !zip_ichi(z, f)) {
        xato = L"Faylni o'qib bo'lmadi — u .xlsx Excel fayli bo'lishi kerak.";
        return false;
    }
    // birinchi varaq: workbook.xml -> rels
    string varaq_yoli = "xl/worksheets/sheet1.xml";
    {
        string wb = f["xl/workbook.xml"], rid;
        size_t i = 0;
        Teg t;
        string m;
        while (keyingi_teg(wb, i, t, m))
            if (t.nom == "sheet" && !t.yopuvchi) { rid = t.attr["id"]; break; }
        string rels = f["xl/_rels/workbook.xml.rels"];
        i = 0;
        while (!rid.empty() && keyingi_teg(rels, i, t, m))
            if (t.nom == "Relationship" && t.attr["Id"] == rid) {
                string tg = t.attr["Target"];
                varaq_yoli = tg.rfind("/", 0) == 0 ? tg.substr(1) : "xl/" + tg;
                break;
            }
    }
    // umumiy matnlar
    std::vector<wstring> umumiy;
    {
        string ss = f["xl/sharedStrings.xml"], m, joriy;
        size_t i = 0;
        Teg t;
        bool si = false, tt = false, rph = false;
        while (keyingi_teg(ss, i, t, m)) {
            if (tt && !rph) joriy += xml_ochir(m);
            if (t.nom == "si") {
                if (!t.yopuvchi) { si = true; joriy.clear(); }
                if (t.yopuvchi || t.ozi_yopiq) { umumiy.push_back(W(joriy)); si = false; }
            } else if (t.nom == "t" && si) tt = !t.yopuvchi && !t.ozi_yopiq;
            else if (t.nom == "rPh") rph = !t.yopuvchi && !t.ozi_yopiq;
        }
    }
    auto it = f.find(varaq_yoli);
    if (it == f.end()) {
        xato = L"Faylda varaq topilmadi.";
        return false;
    }
    const string& x = it->second;
    size_t i = 0;
    Teg t;
    string m;
    int qator = 0, ustun = -1;
    string ctur, vmatn, ismatn;
    bool ichida = false, vda = false, tda = false;
    auto katak_yoz = [&]() {
        Katak k;
        if (ctur == "s") {
            int n = atoi(vmatn.c_str());
            if (n >= 0 && n < (int)umumiy.size()) { k.tur = Katak::MATN; k.matn = umumiy[n]; }
        } else if (ctur == "inlineStr") { k.tur = Katak::MATN; k.matn = W(ismatn); }
        else if (ctur == "str" || ctur == "e") { k.tur = Katak::MATN; k.matn = W(vmatn); }
        else if (ctur == "b") { k.tur = Katak::MANTIQ; k.son = atoi(vmatn.c_str()); k.matn = k.son ? L"TRUE" : L"FALSE"; }
        else if (!vmatn.empty()) { k.tur = Katak::SON; k.matn = W(vmatn); k.son = atof(vmatn.c_str()); }
        if (k.tur == Katak::BOSH) return;
        auto& q = v.qatorlar[qator];
        if ((int)q.size() <= ustun) q.resize(ustun + 1);
        q[ustun] = k;
    };
    while (keyingi_teg(x, i, t, m)) {
        if (vda) vmatn += xml_ochir(m);
        if (tda) ismatn += xml_ochir(m);
        if (t.nom == "row" && !t.yopuvchi) {
            qator = t.attr.count("r") ? atoi(t.attr["r"].c_str()) : qator + 1;
            ustun = -1;
        } else if (t.nom == "c") {
            if (!t.yopuvchi) {
                ustun = t.attr.count("r") ? ustun_raqami(t.attr["r"]) : ustun + 1;
                ctur = t.attr["t"];
                vmatn.clear();
                ismatn.clear();
                ichida = !t.ozi_yopiq;
            } else if (ichida) {
                katak_yoz();
                ichida = false;
            }
        } else if (t.nom == "v") vda = !t.yopuvchi && !t.ozi_yopiq;
        else if (t.nom == "t" && ichida) tda = !t.yopuvchi && !t.ozi_yopiq;
    }
    return true;
}

// ======================= Yozish =======================

static string xml_esc(const wstring& w) {
    string s = U(w), r;
    for (char c : s) {
        if (c == '&') r += "&amp;";
        else if (c == '<') r += "&lt;";
        else if (c == '>') r += "&gt;";
        else if (c == '"') r += "&quot;";
        else r += c;
    }
    return r;
}

static string harf(int i) {
    string s;
    for (++i; i > 0; i = (i - 1) / 26) s.insert(s.begin(), (char)('A' + (i - 1) % 26));
    return s;
}

static string son_matn(double d) {
    char b[64];
    snprintf(b, sizeof b, "%.10g", d);
    return b;
}

bool xlsx_yoz(const wstring& fayl, const std::vector<YVaraq>& varaqlar, wstring& xato) {
    std::vector<std::pair<string, string>> f;
    string ct = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
                "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
                "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
                "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
                "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>";
    for (size_t i = 0; i < varaqlar.size(); ++i)
        ct += "<Override PartName=\"/xl/worksheets/sheet" + std::to_string(i + 1) +
              ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>";
    ct += "</Types>";
    f.push_back({"[Content_Types].xml", ct});
    f.push_back({"_rels/.rels",
                 "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                 "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
                 "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
                 "</Relationships>"});
    string wb = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
                "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheets>";
    string rels = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                  "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
    string nomlar;
    for (size_t i = 0; i < varaqlar.size(); ++i) {
        string n = std::to_string(i + 1);
        wb += "<sheet name=\"" + xml_esc(varaqlar[i].nom) + "\" sheetId=\"" + n + "\" r:id=\"rId" + n + "\"/>";
        rels += "<Relationship Id=\"rId" + n + "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet" + n + ".xml\"/>";
        if (!varaqlar[i].avtofiltr.empty()) {
            wstring a = varaqlar[i].avtofiltr;
            size_t k = a.find(L':');
            auto mutlaq = [](const wstring& s) {
                size_t j = 0;
                while (j < s.size() && iswalpha(s[j])) ++j;
                return L"$" + s.substr(0, j) + L"$" + s.substr(j);
            };
            nomlar += "<definedName name=\"_xlnm._FilterDatabase\" localSheetId=\"" + std::to_string(i) +
                      "\" hidden=\"1\">'" + xml_esc(varaqlar[i].nom) + "'!" +
                      U(mutlaq(a.substr(0, k)) + L":" + mutlaq(a.substr(k + 1))) + "</definedName>";
        }
    }
    wb += "</sheets>";
    if (!nomlar.empty()) wb += "<definedNames>" + nomlar + "</definedNames>";
    wb += "</workbook>";
    rels += "<Relationship Id=\"rId" + std::to_string(varaqlar.size() + 1) +
            "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
            "</Relationships>";
    f.push_back({"xl/workbook.xml", wb});
    f.push_back({"xl/_rels/workbook.xml.rels", rels});
    f.push_back({"xl/styles.xml",
                 "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                 "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
                 "<numFmts count=\"2\"><numFmt numFmtId=\"164\" formatCode=\"#,##0.###\"/>"
                 "<numFmt numFmtId=\"165\" formatCode=\"#,##0.####\"/></numFmts>"
                 "<fonts count=\"4\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font>"
                 "<font><b/><sz val=\"11\"/><color rgb=\"FFFFFFFF\"/><name val=\"Calibri\"/></font>"
                 "<font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font>"
                 "<font><b/><sz val=\"14\"/><name val=\"Calibri\"/></font></fonts>"
                 "<fills count=\"3\"><fill><patternFill patternType=\"none\"/></fill>"
                 "<fill><patternFill patternType=\"gray125\"/></fill>"
                 "<fill><patternFill patternType=\"solid\"><fgColor rgb=\"FF2F7AE0\"/><bgColor indexed=\"64\"/></patternFill></fill></fills>"
                 "<borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders>"
                 "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
                 "<cellXfs count=\"8\">"
                 "<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>"
                 "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"2\" borderId=\"0\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyAlignment=\"1\"><alignment horizontal=\"center\" vertical=\"center\"/></xf>"
                 "<xf numFmtId=\"49\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>"
                 "<xf numFmtId=\"164\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>"
                 "<xf numFmtId=\"165\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyNumberFormat=\"1\"/>"
                 "<xf numFmtId=\"0\" fontId=\"2\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"1\"/>"
                 "<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyAlignment=\"1\"><alignment vertical=\"top\" wrapText=\"1\"/></xf>"
                 "<xf numFmtId=\"0\" fontId=\"3\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"1\"/>"
                 "</cellXfs><cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
                 "</styleSheet>"});
    for (size_t vi = 0; vi < varaqlar.size(); ++vi) {
        const YVaraq& v = varaqlar[vi];
        string s = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                   "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">";
        s += "<sheetViews><sheetView workbookViewId=\"0\"";
        if (vi == 0) s += " tabSelected=\"1\"";
        s += ">";
        if (!v.muzlatish.empty()) {
            int ust = ustun_raqami(U(v.muzlatish)), qat = _wtoi(v.muzlatish.c_str() + (v.muzlatish.find_first_of(L"0123456789")));
            s += "<pane";
            if (ust > 0) s += " xSplit=\"" + std::to_string(ust) + "\"";
            if (qat > 1) s += " ySplit=\"" + std::to_string(qat - 1) + "\"";
            s += " topLeftCell=\"" + U(v.muzlatish) + "\" activePane=\"bottomRight\" state=\"frozen\"/>";
        }
        s += "</sheetView></sheetViews>";
        if (!v.enlar.empty()) {
            s += "<cols>";
            for (size_t i = 0; i < v.enlar.size(); ++i)
                s += "<col min=\"" + std::to_string(i + 1) + "\" max=\"" + std::to_string(i + 1) + "\" width=\"" +
                     son_matn(v.enlar[i]) + "\" customWidth=\"1\"/>";
            s += "</cols>";
        }
        s += "<sheetData>";
        for (size_t r = 0; r < v.qatorlar.size(); ++r) {
            string rn = std::to_string(r + 1);
            s += "<row r=\"" + rn + "\"";
            if (r == 0 && v.sarlavha_boyi > 0) s += " ht=\"" + son_matn(v.sarlavha_boyi) + "\" customHeight=\"1\"";
            s += ">";
            for (size_t c = 0; c < v.qatorlar[r].size(); ++c) {
                const YKatak& k = v.qatorlar[r][c];
                string ref = harf((int)c) + rn, st = k.uslub ? " s=\"" + std::to_string(k.uslub) + "\"" : "";
                if (k.tur == YKatak::SON)
                    s += "<c r=\"" + ref + "\"" + st + "><v>" + son_matn(k.son) + "</v></c>";
                else if (k.tur == YKatak::MATN)
                    s += "<c r=\"" + ref + "\"" + st + " t=\"inlineStr\"><is><t xml:space=\"preserve\">" + xml_esc(k.matn) +
                         "</t></is></c>";
                else if (k.uslub)
                    s += "<c r=\"" + ref + "\"" + st + "/>";
            }
            s += "</row>";
        }
        s += "</sheetData>";
        if (!v.avtofiltr.empty()) s += "<autoFilter ref=\"" + U(v.avtofiltr) + "\"/>";
        if (!v.tekshiruvlar.empty()) {
            s += "<dataValidations count=\"" + std::to_string(v.tekshiruvlar.size()) + "\">";
            for (auto& t : v.tekshiruvlar)
                s += "<dataValidation type=\"list\" allowBlank=\"1\" showErrorMessage=\"1\" sqref=\"" + U(t.oraliq) +
                     "\"><formula1>\"" + xml_esc(t.royxat) + "\"</formula1></dataValidation>";
            s += "</dataValidations>";
        }
        s += "<pageMargins left=\"0.75\" right=\"0.75\" top=\"1\" bottom=\"1\" header=\"0.5\" footer=\"0.5\"/></worksheet>";
        f.push_back({"xl/worksheets/sheet" + std::to_string(vi + 1) + ".xml", s});
    }
    string z = zip_yasa(f);
    FILE* fp = _wfopen(fayl.c_str(), L"wb");
    if (!fp) {
        xato = L"Faylni yozib bo'lmadi (u Excel'da ochiq emasmi?): " + fayl;
        return false;
    }
    bool ok = fwrite(z.data(), 1, z.size(), fp) == z.size();
    fclose(fp);
    if (!ok) xato = L"Faylni yozib bo'lmadi: " + fayl;
    return ok;
}
