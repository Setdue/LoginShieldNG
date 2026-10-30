#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commdlg.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

constexpr int SIDEBAR_W = 218;
constexpr int IP_LIMIT = 3;
constexpr int SPRAY_LIMIT = 3;
constexpr int WINDOW_SECONDS = 60;

const COLORREF BG = RGB(10, 12, 21);
const COLORREF SIDEBAR = RGB(15, 16, 28);
const COLORREF PANEL = RGB(24, 23, 36);
const COLORREF PANEL_ALT = RGB(34, 31, 44);
const COLORREF BORDER = RGB(69, 50, 59);
const COLORREF TEXT = RGB(245, 232, 214);
const COLORREF MUTED = RGB(179, 154, 146);
const COLORREF GREEN = RGB(93, 223, 195);
const COLORREF RED = RGB(255, 104, 86);
const COLORREF AMBER = RGB(255, 188, 93);
const COLORREF BLUE = RGB(159, 169, 255);

struct LoginEvent {
    std::string time;
    std::string username;
    std::string ip;
    std::string device;
    std::string status;
    int seconds = 0;
};

struct Alert {
    std::string title;
    std::string detail;
    std::string time;
    bool high = true;
};

struct IpSummary {
    std::string ip;
    int failures = 0;
    int accounts = 0;
    int devices = 0;
    std::string level;
};

struct DeviceSummary {
    std::string device;
    int recentFailures = 0;
    int accountCount = 0;
    int ipCount = 0;
    int score = 0;
    int health = 100;
    std::string level;
};

struct Analysis {
    std::vector<Alert> alerts;
    std::vector<IpSummary> ipRows;
    std::vector<DeviceSummary> deviceRows;
    int failed = 0;
    int success = 0;
    int targetedAccounts = 0;
    int reviewIps = 0;
};

struct WindowAttempt {
    int seconds;
    std::string username;
    std::string device;
};

std::vector<LoginEvent> g_events;
std::vector<LoginEvent> g_scenarioEvents;
Analysis g_analysis;
bool g_failedOnly = false;
std::string g_feedName = "DEMO FEED";
HWND g_window = nullptr;
double g_animationTick = 0.0;
size_t g_playbackIndex = 0;
bool g_playback = false;

std::wstring widen(const std::string& value) {
    if (value.empty()) return L"";
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 1) return std::wstring(value.begin(), value.end());
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    result.pop_back();
    return result;
}

void drawText(HDC dc, int x, int y, int width, int height, const std::wstring& value,
              COLORREF color = TEXT, int size = 12, bool bold = false,
              UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HFONT font = CreateFontW(-size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE,
                             FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HGDIOBJ old = SelectObject(dc, font);
    RECT rect{x, y, x + width, y + height};
    DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &rect, flags);
    SelectObject(dc, old);
    DeleteObject(font);
}

void drawTextA(HDC dc, int x, int y, int width, int height, const std::string& value,
               COLORREF color = TEXT, int size = 12, bool bold = false,
               UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    drawText(dc, x, y, width, height, widen(value), color, size, bold, flags);
}

void fillRect(HDC dc, int x, int y, int width, int height, COLORREF color) {
    RECT rect{x, y, x + width, y + height};
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void roundedPanel(HDC dc, int x, int y, int width, int height, COLORREF color = PANEL) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, BORDER);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, x, y, x + width, y + height, 16, 16);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void drawPainterlyWash(HDC dc, int x, int y, int width, int height, unsigned int seed) {
    if (width < 30 || height < 24) return;
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, x, y, x + width, y + height);
    const std::array<COLORREF, 5> pigments{
        RGB(39, 34, 51), RGB(50, 34, 42), RGB(27, 48, 48),
        RGB(58, 40, 35), RGB(39, 38, 59)
    };
    auto nextRandom = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    };
    for (int i = 0; i < 4; ++i) {
        const int baseY = y + height * (i + 1) / 5;
        const int sway = 5 + static_cast<int>(nextRandom() % static_cast<unsigned int>(std::max(6, height / 5)));
        POINT stroke[4]{
            {x - 12, baseY},
            {x + width / 3, baseY - sway},
            {x + width * 2 / 3, baseY + sway},
            {x + width + 12, baseY - sway / 2}
        };
        HPEN pen = CreatePen(PS_SOLID, 5 + (i % 3) * 3, pigments[i]);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        PolyBezier(dc, stroke, 4);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }
    for (int i = 0; i < 48; ++i) {
        const int px = x + static_cast<int>(nextRandom() % static_cast<unsigned int>(width));
        const int py = y + static_cast<int>(nextRandom() % static_cast<unsigned int>(height));
        SetPixelV(dc, px, py, pigments[nextRandom() % pigments.size()]);
    }
    RestoreDC(dc, saved);
}

void drawVortexPainting(HDC dc, int cx, int cy, int radiusX, int radiusY) {
    const int saved = SaveDC(dc);
    const std::array<COLORREF, 4> colors{
        RGB(71, 37, 49), RGB(194, 83, 56), RGB(255, 177, 91), RGB(65, 121, 111)
    };
    const std::array<std::array<int, 4>, 4> rings{{
        {{cx - radiusX, cy - radiusY, cx + radiusX, cy + radiusY}},
        {{cx - radiusX + 7, cy - radiusY + 5, cx + radiusX - 3, cy + radiusY + 1}},
        {{cx - radiusX + 17, cy - radiusY + 10, cx + radiusX - 14, cy + radiusY - 7}},
        {{cx - radiusX + 25, cy - radiusY + 14, cx + radiusX - 21, cy + radiusY - 12}}
    }};
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    for (int i = 0; i < 4; ++i) {
        HPEN pen = CreatePen(PS_SOLID, 8 - i * 2, colors[i]);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        Ellipse(dc, rings[i][0], rings[i][1], rings[i][2], rings[i][3]);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }
    SelectObject(dc, oldBrush);

    const std::array<POINT, 4> sweep1{{
        {cx - radiusX - 8, cy + 3}, {cx - radiusX / 3, cy - radiusY - 17},
        {cx + radiusX / 3, cy + radiusY + 14}, {cx + radiusX + 8, cy - 2}
    }};
    const std::array<POINT, 4> sweep2{{
        {cx - radiusX / 2, cy - radiusY - 5}, {cx + radiusX / 2, cy - radiusY + 12},
        {cx - radiusX / 3, cy + radiusY + 12}, {cx + radiusX / 2, cy + radiusY - 4}
    }};
    for (int i = 0; i < 2; ++i) {
        HPEN pen = CreatePen(PS_SOLID, i == 0 ? 3 : 2, colors[i + 2]);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        PolyBezier(dc, i == 0 ? sweep1.data() : sweep2.data(), 4);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    HBRUSH coreBrush = CreateSolidBrush(RGB(5, 7, 13));
    HPEN corePen = CreatePen(PS_SOLID, 2, RGB(249, 190, 111));
    oldBrush = SelectObject(dc, coreBrush);
    HGDIOBJ oldPen = SelectObject(dc, corePen);
    Ellipse(dc, cx - 18, cy - 11, cx + 18, cy + 11);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(coreBrush);
    DeleteObject(corePen);

    HBRUSH sparkBrush = CreateSolidBrush(RGB(255, 211, 153));
    HPEN sparkPen = CreatePen(PS_SOLID, 1, RGB(255, 211, 153));
    oldBrush = SelectObject(dc, sparkBrush);
    oldPen = SelectObject(dc, sparkPen);
    const std::array<POINT, 9> sparks{{
        {cx - radiusX + 4, cy - 3}, {cx - radiusX / 2, cy - radiusY + 1},
        {cx + radiusX / 2, cy + radiusY - 4}, {cx + radiusX - 2, cy + 5},
        {cx - radiusX / 3, cy + radiusY - 1}, {cx + radiusX / 3, cy - radiusY + 2},
        {cx - radiusX + 16, cy + radiusY / 3}, {cx + radiusX - 19, cy - radiusY / 3},
        {cx + 4, cy - radiusY - 4}
    }};
    for (size_t i = 0; i < sparks.size(); ++i) {
        const int r = (i % 3 == 0) ? 3 : 2;
        Ellipse(dc, sparks[i].x - r, sparks[i].y - r, sparks[i].x + r, sparks[i].y + r);
    }
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(sparkBrush);
    DeleteObject(sparkPen);
    RestoreDC(dc, saved);
}

void button(HDC dc, int x, int y, int width, int height, const wchar_t* label,
            bool primary = false) {
    const COLORREF background = primary ? GREEN : PANEL_ALT;
    const COLORREF foreground = primary ? RGB(7, 20, 15) : TEXT;
    HBRUSH brush = CreateSolidBrush(background);
    HPEN pen = CreatePen(PS_SOLID, 1, primary ? GREEN : BORDER);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, x, y, x + width, y + height, 9, 9);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
    drawText(dc, x + 4, y + 1, width - 8, height - 2, label, foreground, 11, true,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

std::string trim(std::string value) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

int timeSeconds(const std::string& value) {
    if (value.size() != 8 || value[2] != ':' || value[5] != ':') return -1;
    try {
        const int hour = std::stoi(value.substr(0, 2));
        const int minute = std::stoi(value.substr(3, 2));
        const int second = std::stoi(value.substr(6, 2));
        if (hour > 23 || minute > 59 || second > 59) return -1;
        return hour * 3600 + minute * 60 + second;
    } catch (...) {
        return -1;
    }
}

std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> cells;
    std::string cell;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"' && quoted && i + 1 < line.size() && line[i + 1] == '"') {
            cell += '"';
            ++i;
        } else if (ch == '"') {
            quoted = !quoted;
        } else if (ch == ',' && !quoted) {
            cells.push_back(trim(cell));
            cell.clear();
        } else {
            cell += ch;
        }
    }
    cells.push_back(trim(cell));
    return cells;
}

bool loadCsv(const std::wstring& path, std::vector<LoginEvent>& result, std::wstring& error) {
    std::ifstream file{std::filesystem::path(path)};
    if (!file) {
        error = L"Could not open the selected CSV file.";
        return false;
    }

    std::string line;
    if (!std::getline(file, line)) {
        error = L"The CSV file is empty.";
        return false;
    }
    if (!line.empty() && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
    auto headers = parseCsvRow(line);
    for (auto& header : headers) header = lower(header);
    const std::array<std::string, 5> required{"time", "username", "ip", "device", "status"};
    std::array<int, 5> index{};
    for (size_t i = 0; i < required.size(); ++i) {
        auto found = std::find(headers.begin(), headers.end(), required[i]);
        if (found == headers.end()) {
            error = L"CSV must have columns: time, username, ip, device, status";
            return false;
        }
        index[i] = static_cast<int>(std::distance(headers.begin(), found));
    }

    std::vector<LoginEvent> loaded;
    int lineNumber = 1;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (trim(line).empty()) continue;
        const auto cells = parseCsvRow(line);
        for (int column : index) {
            if (column >= static_cast<int>(cells.size())) {
                error = L"A CSV row is missing a field. Check row " + std::to_wstring(lineNumber) + L".";
                return false;
            }
        }
        LoginEvent event;
        event.time = cells[index[0]];
        event.username = cells[index[1]];
        event.ip = cells[index[2]];
        event.device = cells[index[3]];
        event.status = lower(cells[index[4]]);
        event.seconds = timeSeconds(event.time);
        if (event.seconds < 0 || event.username.empty() || event.ip.empty() ||
            event.device.empty() || (event.status != "failed" && event.status != "success")) {
            error = L"Invalid login data on CSV row " + std::to_wstring(lineNumber) +
                    L". Use HH:MM:SS and failed/success.";
            return false;
        }
        loaded.push_back(std::move(event));
    }
    if (loaded.empty()) {
        error = L"The CSV has a header but no login events.";
        return false;
    }
    std::stable_sort(loaded.begin(), loaded.end(),
                     [](const LoginEvent& a, const LoginEvent& b) { return a.seconds < b.seconds; });
    result = std::move(loaded);
    return true;
}

std::vector<LoginEvent> demoEvents() {
    const std::array<std::array<const char*, 5>, 9> rows{{
        {{"10:00:00", "user_01", "192.0.2.10", "device_A", "failed"}},
        {{"10:00:10", "user_02", "192.0.2.10", "device_B", "failed"}},
        {{"10:00:20", "user_03", "192.0.2.10", "device_B", "failed"}},
        {{"10:00:25", "user_01", "192.0.2.20", "device_C", "failed"}},
        {{"10:00:30", "user_01", "192.0.2.10", "device_A", "failed"}},
        {{"10:00:40", "user_03", "192.0.2.10", "device_B", "success"}},
        {{"10:00:50", "user_01", "192.0.2.20", "device_C", "failed"}},
        {{"10:00:58", "user_02", "192.0.2.20", "device_C", "failed"}},
        {{"10:00:59", "user_03", "192.0.2.20", "device_C", "failed"}}
    }};
    std::vector<LoginEvent> events;
    for (const auto& row : rows) {
        events.push_back({row[0], row[1], row[2], row[3], row[4], timeSeconds(row[0])});
    }
    return events;
}

Analysis analyze(const std::vector<LoginEvent>& events) {
    Analysis result;
    struct DeviceActivity {
        int recentFailures = 0;
        std::set<std::string> accounts;
        std::set<std::string> ips;
    };
    std::unordered_map<std::string, std::deque<WindowAttempt>> failures;
    std::unordered_map<std::string, std::deque<WindowAttempt>> targets;
    std::unordered_map<std::string, std::set<std::string>> accountIps;
    std::unordered_map<std::string, DeviceActivity> deviceActivity;
    std::unordered_set<std::string> failureAlerted;
    std::unordered_set<std::string> sprayAlerted;
    const int latestEventTime = events.empty() ? 0 : events.back().seconds;

    for (const auto& event : events) {
        accountIps[event.username].insert(event.ip);
        auto& device = deviceActivity[event.device];
        device.accounts.insert(event.username);
        device.ips.insert(event.ip);
        if (event.status == "failed" && latestEventTime - event.seconds <= WINDOW_SECONDS)
            ++device.recentFailures;
        if (event.status == "success") {
            ++result.success;
            continue;
        }
        ++result.failed;
        auto& ipFailures = failures[event.ip];
        auto& ipTargets = targets[event.ip];
        ipFailures.push_back({event.seconds, event.username, event.device});
        ipTargets.push_back({event.seconds, event.username, event.device});
        while (!ipFailures.empty() && event.seconds - ipFailures.front().seconds > WINDOW_SECONDS)
            ipFailures.pop_front();
        while (!ipTargets.empty() && event.seconds - ipTargets.front().seconds > WINDOW_SECONDS)
            ipTargets.pop_front();

        if (static_cast<int>(ipFailures.size()) >= IP_LIMIT && !failureAlerted.count(event.ip)) {
            std::set<std::string> devices;
            for (const auto& attempt : ipFailures) devices.insert(attempt.device);
            std::string deviceList;
            for (const auto& device : devices) {
                if (!deviceList.empty()) deviceList += ", ";
                deviceList += device;
            }
            result.alerts.push_back({event.ip + " - repeated failures",
                                     std::to_string(ipFailures.size()) +
                                         " failed attempts in 60 seconds; devices: " + deviceList,
                                     event.time, true});
            failureAlerted.insert(event.ip);
        }
        if (static_cast<int>(ipFailures.size()) < IP_LIMIT) failureAlerted.erase(event.ip);

        std::set<std::string> users;
        for (const auto& attempt : ipTargets) users.insert(attempt.username);
        if (static_cast<int>(users.size()) >= SPRAY_LIMIT && !sprayAlerted.count(event.ip)) {
            result.alerts.push_back({event.ip + " - multiple accounts targeted",
                                     std::to_string(users.size()) +
                                         " accounts had failed logins within 60 seconds",
                                     event.time, false});
            sprayAlerted.insert(event.ip);
        }
        if (static_cast<int>(users.size()) < SPRAY_LIMIT) sprayAlerted.erase(event.ip);
    }

    std::set<std::string> targetedAccounts;
    for (const auto& [ip, attempts] : failures) {
        std::set<std::string> users;
        std::set<std::string> devices;
        for (const auto& attempt : attempts) {
            users.insert(attempt.username);
            devices.insert(attempt.device);
        }
        for (const auto& user : users) targetedAccounts.insert(user);
        if (attempts.empty()) continue;
        std::string level = attempts.size() >= IP_LIMIT ? "ELEVATED" :
                            (users.size() > 1 ? "REVIEW" : "OBSERVED");
        result.ipRows.push_back({ip, static_cast<int>(attempts.size()),
                                 static_cast<int>(users.size()),
                                 static_cast<int>(devices.size()), level});
    }
    result.targetedAccounts = static_cast<int>(targetedAccounts.size());
    std::sort(result.ipRows.begin(), result.ipRows.end(),
              [](const IpSummary& a, const IpSummary& b) { return a.failures > b.failures; });
    for (const auto& [name, activity] : deviceActivity) {
        const int accountSignal = std::max(0, static_cast<int>(activity.accounts.size()) - 1) * 14;
        const int ipSignal = std::max(0, static_cast<int>(activity.ips.size()) - 1) * 12;
        const int score = std::min(100, activity.recentFailures * 22 + accountSignal + ipSignal);
        const int health = 100 - score;
        const std::string level = score >= 65 ? "HIGH ACTIVITY" :
                                  (score >= 35 ? "WATCH" : "STABLE");
        result.deviceRows.push_back({name, activity.recentFailures,
                                     static_cast<int>(activity.accounts.size()),
                                     static_cast<int>(activity.ips.size()), score, health, level});
    }
    std::sort(result.deviceRows.begin(), result.deviceRows.end(),
              [](const DeviceSummary& a, const DeviceSummary& b) {
                  return a.score > b.score || (a.score == b.score && a.device < b.device);
              });
    for (const auto& [username, ips] : accountIps) {
        if (ips.size() > 1) {
            std::string list;
            for (const auto& ip : ips) {
                if (!list.empty()) list += ", ";
                list += ip;
            }
            result.alerts.push_back({username + " - activity from multiple IPs",
                                     std::to_string(ips.size()) + " IP addresses observed: " + list,
                                     "ACCOUNT REVIEW", false});
        }
    }
    std::set<std::string> reviewIps;
    for (const auto& alert : result.alerts) {
        for (const auto& row : result.ipRows) {
            if (alert.title.rfind(row.ip, 0) == 0) reviewIps.insert(row.ip);
        }
    }
    result.reviewIps = static_cast<int>(reviewIps.size());
    return result;
}

void drawMetric(HDC dc, int x, int y, int width, const wchar_t* label,
                const std::wstring& value, const wchar_t* foot, COLORREF valueColor = TEXT) {
    roundedPanel(dc, x, y, width, 97);
    drawPainterlyWash(dc, x + 2, y + 2, width - 4, 93,
                      static_cast<unsigned int>(x * 31 + y * 17));
    drawText(dc, x + 15, y + 13, width - 30, 20, label, MUTED, 10, false);
    drawText(dc, x + 15, y + 34, width - 30, 37, value, valueColor, 27, true);
    drawText(dc, x + 15, y + 72, width - 30, 16, foot, MUTED, 9, false);
}

void drawPanelTitle(HDC dc, int x, int y, int width, const wchar_t* title,
                    const wchar_t* subtitle) {
    drawText(dc, x + 17, y + 13, width - 34, 20, title, TEXT, 12, true);
    drawText(dc, x + 17, y + 33, width - 34, 18, subtitle, MUTED, 9, false);
}

void drawChart(HDC dc, int x, int y, int width, int height) {
    const int chartTop = y + 63;
    const int chartBottom = y + height - 34;
    const int chartHeight = chartBottom - chartTop;
    const int chartLeft = x + 24;
    const int chartWidth = width - 48;
    std::array<int, 8> totals{};
    std::array<int, 8> failed{};
    for (size_t i = 0; i < g_events.size(); ++i) {
        const size_t bucket = std::min<size_t>(7, i * 8 / std::max<size_t>(1, g_events.size()));
        ++totals[bucket];
        if (g_events[i].status == "failed") ++failed[bucket];
    }
    const int peak = std::max(1, *std::max_element(totals.begin(), totals.end()));
    HPEN grid = CreatePen(PS_DOT, 1, BORDER);
    HGDIOBJ oldPen = SelectObject(dc, grid);
    for (int line = 0; line < 3; ++line) {
        const int gy = chartTop + line * chartHeight / 2;
        MoveToEx(dc, chartLeft, gy, nullptr);
        LineTo(dc, chartLeft + chartWidth, gy);
    }
    SelectObject(dc, oldPen);
    DeleteObject(grid);

    const int slot = chartWidth / 8;
    for (int i = 0; i < 8; ++i) {
        const int barW = std::max(12, std::min(32, slot - 15));
        const int bx = chartLeft + i * slot + (slot - barW) / 2;
        const int totalH = totals[i] ? std::max(5, totals[i] * (chartHeight - 5) / peak) : 3;
        const int failH = totals[i] ? failed[i] * totalH / totals[i] : 0;
        fillRect(dc, bx, chartBottom - totalH, barW, totalH - failH, GREEN);
        if (failH > 0) fillRect(dc, bx, chartBottom - failH, barW, failH, RED);
        drawText(dc, bx - 2, chartBottom + 6, barW + 4, 14,
                 L"B" + std::to_wstring(i + 1), MUTED, 8, false,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void drawDeviceHealthBar(HDC dc, int x, int y, int width, int height,
                         const DeviceSummary& device, int rowIndex) {
    fillRect(dc, x, y, width, height, RGB(42, 35, 46));
    const COLORREF levelColor = device.score >= 65 ? RED :
                                (device.score >= 35 ? AMBER : GREEN);
    const int healthWidth = width * device.health / 100;
    if (healthWidth > 2) {
        const int saved = SaveDC(dc);
        IntersectClipRect(dc, x, y - 2, x + healthWidth, y + height + 2);
        const std::array<int, 3> strokeWidths{11, 7, 3};
        const std::array<COLORREF, 3> colors{
            device.score >= 65 ? RGB(104, 42, 48) :
                (device.score >= 35 ? RGB(106, 73, 42) : RGB(32, 92, 82)),
            levelColor,
            RGB(255, 220, 177)
        };
        for (int layer = 0; layer < 3; ++layer) {
            POINT stroke[4]{
                {x, y + height / 2},
                {x + healthWidth / 3, y - 1 + ((rowIndex + layer) % 2) * 3},
                {x + healthWidth * 2 / 3, y + height + 1 - ((rowIndex + layer) % 2) * 3},
                {x + healthWidth, y + height / 2}
            };
            HPEN pen = CreatePen(PS_SOLID, strokeWidths[layer], colors[layer]);
            HGDIOBJ oldPen = SelectObject(dc, pen);
            PolyBezier(dc, stroke, 4);
            SelectObject(dc, oldPen);
            DeleteObject(pen);
        }
        RestoreDC(dc, saved);
    }

    const int particleCount = 1 + device.score / 12;
    const double speed = 10.0 + device.score * 1.25;
    const COLORREF sparkColor = device.score >= 65 ? RGB(255, 218, 166) : RGB(236, 247, 225);
    HPEN particlePen = CreatePen(PS_SOLID, 2, sparkColor);
    HBRUSH particleBrush = CreateSolidBrush(sparkColor);
    HGDIOBJ oldPen = SelectObject(dc, particlePen);
    HGDIOBJ oldBrush = SelectObject(dc, particleBrush);
    for (int i = 0; i < particleCount; ++i) {
        const double offset = g_animationTick * speed + rowIndex * 19.0 +
                              i * (width / static_cast<double>(particleCount));
        const int position = static_cast<int>(std::fmod(offset, static_cast<double>(width)));
        const int px = x + position;
        const int py = y + height / 2 + static_cast<int>(std::sin(g_animationTick * 2.4 + i * 1.8) * 2.0);
        const int tail = 4 + device.score / 16;
        MoveToEx(dc, std::max(x, px - tail), py + 1, nullptr);
        LineTo(dc, px, py);
        Ellipse(dc, px - 2, py - 2, px + 3, py + 3);
    }
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(particlePen);
    DeleteObject(particleBrush);
}

void paintDashboard(HWND hwnd, HDC dc) {
    RECT client{};
    GetClientRect(hwnd, &client);
    const int width = client.right;
    const int height = client.bottom;
    fillRect(dc, 0, 0, width, height, BG);
    drawPainterlyWash(dc, SIDEBAR_W, 0, width - SIDEBAR_W, height, 0xB1A4u);
    fillRect(dc, 0, 0, SIDEBAR_W, height, SIDEBAR);
    fillRect(dc, SIDEBAR_W - 1, 0, 1, height, BORDER);

    HBRUSH logoBrush = CreateSolidBrush(GREEN);
    RECT logo{24, 25, 62, 63};
    FillRect(dc, &logo, logoBrush);
    DeleteObject(logoBrush);
    drawText(dc, 24, 24, 38, 38, L"L", RGB(7, 20, 15), 21, true,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    drawText(dc, 74, 27, 126, 19, L"LOGINSHIELD NG", TEXT, 11, true);
    drawText(dc, 74, 47, 126, 16, L"SECURITY CONSOLE", MUTED, 8, false);

    drawText(dc, 23, 100, 170, 18, L"WORKSPACE", RGB(113, 129, 150), 9, true);
    const std::array<const wchar_t*, 4> nav{L"Overview", L"Login activity", L"IP intelligence", L"Review alerts"};
    const std::array<const wchar_t*, 4> icons{L"◫", L"⌁", L"◎", L"△"};
    for (int i = 0; i < 4; ++i) {
        const int ny = 130 + i * 42;
        if (i == 0) {
            HBRUSH active = CreateSolidBrush(RGB(18, 44, 39));
            RECT rect{12, ny, SIDEBAR_W - 12, ny + 35};
            FillRect(dc, &rect, active);
            DeleteObject(active);
        }
        drawText(dc, 24, ny, 22, 35, icons[i], i == 0 ? GREEN : MUTED, 15, true,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        drawText(dc, 53, ny, 145, 35, nav[i], i == 0 ? GREEN : RGB(154, 171, 189), 10, i == 0);
    }
    drawVortexPainting(dc, SIDEBAR_W / 2, height / 2 + 20, 72, 43);
    drawText(dc, 24, height / 2 + 77, SIDEBAR_W - 48, 18,
             L"SIGNAL FLOW", AMBER, 8, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    roundedPanel(dc, 14, height - 139, SIDEBAR_W - 28, 119, RGB(16, 25, 35));
    drawText(dc, 26, height - 127, SIDEBAR_W - 52, 19, L"REVIEW BEFORE ACTION", GREEN, 9, true);
    drawText(dc, 26, height - 103, SIDEBAR_W - 52, 69,
             L"Shared IPs can belong to different people and devices. This app flags activity for review; it never blocks users or IPs.",
             MUTED, 9, false, DT_LEFT | DT_TOP | DT_WORDBREAK);

    const int x = SIDEBAR_W + 28;
    const int contentW = width - x - 28;
    drawText(dc, x, 24, 330, 16, L"LOGINSHIELD NG  /  OVERVIEW", MUTED, 9, true);
    drawText(dc, x, 44, 390, 42, L"Threat monitoring", TEXT, 26, true);
    const int buttonY = 34;
    button(dc, x + contentW - 445, buttonY, 112, 34, L"Run live demo");
    button(dc, x + contentW - 323, buttonY, 92, 34, L"Reset demo");
    button(dc, x + contentW - 221, buttonY, 104, 34, L"Import CSV");
    button(dc, x + contentW - 109, buttonY, 109, 34, L"Export report", true);
    drawText(dc, x, 86, contentW, 17,
             widen(g_feedName) + L"  ·  review signals only", GREEN, 9, false);

    const int cardY = 112;
    const int gap = 11;
    const int cardW = (contentW - gap * 3) / 4;
    drawMetric(dc, x, cardY, cardW, L"LOGIN EVENTS", std::to_wstring(g_events.size()), L"Events in current feed", GREEN);
    drawMetric(dc, x + cardW + gap, cardY, cardW, L"FAILED ATTEMPTS", std::to_wstring(g_analysis.failed), L"Across observed IPs", RED);
    drawMetric(dc, x + (cardW + gap) * 2, cardY, cardW, L"ACCOUNTS TARGETED", std::to_wstring(g_analysis.targetedAccounts), L"Distinct accounts with failures", TEXT);
    drawMetric(dc, x + (cardW + gap) * 3, cardY, cardW, L"IPS FOR REVIEW", std::to_wstring(g_analysis.reviewIps), L"Signals, not identities", TEXT);

    const int topY = 222;
    const int topH = std::max(196, std::min(228, height / 3));
    const int gap2 = 13;
    const int chartW = static_cast<int>((contentW - gap2) * .56);
    const int alertX = x + chartW + gap2;
    const int alertW = contentW - chartW - gap2;
    roundedPanel(dc, x, topY, chartW, topH);
    roundedPanel(dc, alertX, topY, alertW, topH);
    drawPainterlyWash(dc, x + 3, topY + 3, chartW - 6, topH - 6, 0xC051u);
    drawPainterlyWash(dc, alertX + 3, topY + 3, alertW - 6, topH - 6, 0xA117u);
    drawPanelTitle(dc, x, topY, chartW, L"Login activity", L"Events across the loaded feed");
    drawText(dc, x + chartW - 168, topY + 14, 72, 15, L"● Success", GREEN, 8, false);
    drawText(dc, x + chartW - 91, topY + 14, 80, 15, L"● Failed", RED, 8, false);
    drawChart(dc, x, topY, chartW, topH);

    drawPanelTitle(dc, alertX, topY, alertW, L"Review alerts", L"Correlated behavior · inspect before action");
    drawText(dc, alertX + alertW - 73, topY + 14, 58, 18,
             std::to_wstring(g_analysis.alerts.size()) + L" alerts", RED, 8, true,
             DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    if (g_analysis.alerts.empty()) {
        drawText(dc, alertX + 18, topY + 88, alertW - 36, 44, L"No review alerts in this feed.", MUTED, 10, false,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        const int rowH = std::min(48, (topH - 61) / std::min<int>(3, g_analysis.alerts.size()));
        for (int i = 0; i < std::min<int>(3, g_analysis.alerts.size()); ++i) {
            const auto& alert = g_analysis.alerts[i];
            const int ay = topY + 58 + i * rowH;
            HBRUSH alertBrush = CreateSolidBrush(alert.high ? RGB(43, 28, 38) : RGB(42, 37, 26));
            HPEN alertPen = CreatePen(PS_SOLID, 1, alert.high ? RGB(77, 43, 56) : RGB(75, 65, 40));
            HGDIOBJ oldBrush = SelectObject(dc, alertBrush);
            HGDIOBJ oldPen = SelectObject(dc, alertPen);
            RoundRect(dc, alertX + 12, ay, alertX + alertW - 12, ay + rowH - 5, 9, 9);
            SelectObject(dc, oldBrush); SelectObject(dc, oldPen);
            DeleteObject(alertBrush); DeleteObject(alertPen);
            const COLORREF alertColor = alert.high ? RED : AMBER;
            drawText(dc, alertX + 21, ay + 3, alertW - 48, 17, widen(alert.title), alertColor, 9, true);
            drawText(dc, alertX + 21, ay + 20, alertW - 48, rowH - 25,
                     widen(alert.time + "  ·  " + alert.detail), MUTED, 8, false,
                     DT_LEFT | DT_TOP | DT_WORDBREAK);
        }
        if (g_analysis.alerts.size() > 3)
            drawText(dc, alertX + 18, topY + topH - 22, alertW - 36, 14,
                     L"Additional signals are included in the exported report.", MUTED, 8);
    }

    const int bottomY = topY + topH + 13;
    const int bottomH = std::max(165, height - bottomY - 20);
    const int leftW = static_cast<int>((contentW - gap2) * .43);
    const int rightX = x + leftW + gap2;
    const int rightW = contentW - leftW - gap2;
    roundedPanel(dc, x, bottomY, leftW, bottomH);
    roundedPanel(dc, rightX, bottomY, rightW, bottomH);
    drawPainterlyWash(dc, x + 3, bottomY + 3, leftW - 6, bottomH - 6, 0xB017u);
    drawPainterlyWash(dc, rightX + 3, bottomY + 3, rightW - 6, bottomH - 6, 0xA091u);
    drawPanelTitle(dc, x, bottomY, leftW, L"Connected devices", L"Higher signal = lower health and faster particles");
    drawPanelTitle(dc, rightX, bottomY, rightW, L"Recent login events", L"Synthetic demo or imported CSV data");

    const int sourceHeadY = bottomY + 59;
    drawText(dc, x + 15, sourceHeadY, leftW - 145, 14, L"DEVICE / LAST-MINUTE SIGNAL", MUTED, 8, true);
    drawText(dc, x + leftW - 119, sourceHeadY, 104, 14, L"DEVICE HEALTH", MUTED, 8, true,
             DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    int visibleDevices = 0;
    const int deviceRowH = 37;
    for (const auto& device : g_analysis.deviceRows) {
        if (visibleDevices >= 6 || sourceHeadY + 19 + visibleDevices * deviceRowH > bottomY + bottomH - 27) break;
        const int dy = sourceHeadY + 18 + visibleDevices * deviceRowH;
        fillRect(dc, x + 14, dy, leftW - 28, 1, BORDER);
        drawTextA(dc, x + 17, dy + 2, 108, 15, device.device, TEXT, 9, true);
        drawTextA(dc, x + 17, dy + 17, 126, 13,
                  std::to_string(device.recentFailures) + " fail · " +
                      std::to_string(device.accountCount) + " accounts · " +
                      std::to_string(device.ipCount) + " IPs",
                  MUTED, 7);

        const int barX = x + 148;
        const int barW = std::max(70, leftW - 246);
        const int barY = dy + 8;
        drawDeviceHealthBar(dc, barX, barY, barW, 11, device, visibleDevices);
        drawTextA(dc, barX, dy + 21, barW, 11,
                  std::to_string(device.health) + "% health", MUTED, 7);

        const COLORREF levelColor = device.score >= 65 ? RED :
                                    (device.score >= 35 ? AMBER : GREEN);
        drawTextA(dc, x + leftW - 88, dy + 5, 72, 15, device.level,
                  levelColor, 7, true, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        ++visibleDevices;
    }
    if (visibleDevices == 0)
        drawText(dc, x + 16, sourceHeadY + 32, leftW - 32, 40, L"No devices in this feed.", MUTED, 9);

    const int eventHeadY = bottomY + 59;
    drawText(dc, rightX + 15, eventHeadY, 65, 14, L"TIME", MUTED, 8, true);
    drawText(dc, rightX + 83, eventHeadY, 85, 14, L"ACCOUNT", MUTED, 8, true);
    drawText(dc, rightX + 172, eventHeadY, 120, 14, L"IP ADDRESS", MUTED, 8, true);
    drawText(dc, rightX + 295, eventHeadY, 91, 14, L"DEVICE", MUTED, 8, true);
    drawText(dc, rightX + rightW - 72, eventHeadY, 59, 14,
             g_failedOnly ? L"FAILED ONLY" : L"ALL EVENTS", BLUE, 8, true,
             DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    int eventRow = 0;
    for (auto it = g_events.rbegin(); it != g_events.rend(); ++it) {
        if (g_failedOnly && it->status != "failed") continue;
        if (eventRow >= 7 || eventHeadY + 18 + eventRow * 24 > bottomY + bottomH - 12) break;
        const int ey = eventHeadY + 17 + eventRow * 24;
        fillRect(dc, rightX + 14, ey, rightW - 28, 1, BORDER);
        drawTextA(dc, rightX + 16, ey + 3, 67, 19, it->time, MUTED, 8);
        drawTextA(dc, rightX + 84, ey + 3, 85, 19, it->username, TEXT, 8, true);
        drawTextA(dc, rightX + 173, ey + 3, 120, 19, it->ip, RGB(201, 212, 223), 8);
        drawTextA(dc, rightX + 296, ey + 3, 88, 19, it->device, MUTED, 8);
        drawText(dc, rightX + rightW - 76, ey + 3, 63, 19,
                 it->status == "failed" ? L"● FAILED" : L"● SUCCESS",
                 it->status == "failed" ? RED : GREEN, 8, true,
                 DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        ++eventRow;
    }
    if (eventRow == 0)
        drawText(dc, rightX + 16, eventHeadY + 30, rightW - 32, 35, L"No events match this view.", MUTED, 9);
    drawText(dc, x, height - 18, contentW, 15,
             L"LOCAL DEMO / CSV INPUT     ·     REVIEW ALERTS BEFORE ACTING     ·     NO AUTOMATIC BLOCKING",
             RGB(112, 128, 149), 8, false);
}

void showError(const std::wstring& message) {
    MessageBoxW(g_window, message.c_str(), L"LoginShield NG", MB_OK | MB_ICONWARNING);
}

void importCsv() {
    wchar_t fileName[MAX_PATH]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = L"CSV files (*.csv)\0*.csv\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    dialog.lpstrTitle = L"Import synthetic login events";
    if (!GetOpenFileNameW(&dialog)) return;
    std::vector<LoginEvent> loaded;
    std::wstring error;
    if (!loadCsv(fileName, loaded, error)) {
        showError(error);
        return;
    }
    g_events = std::move(loaded);
    KillTimer(g_window, 2);
    g_playback = false;
    const std::wstring path(fileName);
    const size_t slash = path.find_last_of(L"\\/");
    g_feedName = "CSV FEED: " + std::string(path.begin() + (slash == std::wstring::npos ? 0 : slash + 1), path.end());
    g_analysis = analyze(g_events);
    InvalidateRect(g_window, nullptr, TRUE);
}

void exportReport() {
    wchar_t fileName[MAX_PATH] = L"ip_alerts_report.txt";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = L"Text report (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = L"txt";
    dialog.lpstrTitle = L"Save LoginShield NG report";
    if (!GetSaveFileNameW(&dialog)) return;
    std::ofstream output{std::filesystem::path(fileName)};
    if (!output) {
        showError(L"Could not save the report to that location.");
        return;
    }
    output << "LOGINSHIELD NG SECURITY REPORT\n" << std::string(35, '=') << "\n";
    output << "Feed: " << g_feedName << "\n";
    output << "Total events: " << g_events.size() << "\n";
    output << "Failed logins: " << g_analysis.failed << "\n";
    output << "Successful logins: " << g_analysis.success << "\n";
    output << "Accounts targeted: " << g_analysis.targetedAccounts << "\n";
    output << "Review alerts: " << g_analysis.alerts.size() << "\n\n";
    output << "REVIEW ALERTS\n" << std::string(35, '=') << "\n";
    if (g_analysis.alerts.empty()) output << "No review alerts.\n";
    for (const auto& alert : g_analysis.alerts)
        output << "- " << alert.time << " | " << alert.title << " | " << alert.detail << "\n";
    output << "\nEVENTS\n" << std::string(35, '=') << "\n";
    output << "time | username | ip | device | status\n";
    for (const auto& event : g_events)
        output << event.time << " | " << event.username << " | " << event.ip << " | "
               << event.device << " | " << event.status << "\n";
    output << "\nReview alerts before acting. Shared IPs are not automatically blocked.\n";
    MessageBoxW(g_window, L"Report saved successfully.", L"LoginShield NG", MB_OK | MB_ICONINFORMATION);
}

void resetDemo() {
    KillTimer(g_window, 2);
    g_playback = false;
    g_events = demoEvents();
    g_feedName = "DEMO FEED";
    g_failedOnly = false;
    g_analysis = analyze(g_events);
    InvalidateRect(g_window, nullptr, TRUE);
}

void runLiveScenario() {
    g_scenarioEvents = demoEvents();
    g_events.clear();
    g_playbackIndex = 0;
    g_playback = true;
    g_failedOnly = false;
    g_feedName = "LIVE DEMO SCENARIO";
    g_analysis = analyze(g_events);
    SetTimer(g_window, 2, 850, nullptr);
    InvalidateRect(g_window, nullptr, TRUE);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 45, nullptr);
        return 0;
    case WM_TIMER:
        if (wParam == 1) {
            g_animationTick += 0.045;
            RECT client{};
            GetClientRect(hwnd, &client);
            const int contentX = SIDEBAR_W + 28;
            const int contentW = static_cast<int>(client.right) - contentX - 28;
            const int topH = std::max(196, std::min(228, static_cast<int>(client.bottom) / 3));
            const int bottomY = 222 + topH + 13;
            const int leftW = static_cast<int>((contentW - 13) * .43);
            RECT devicePanel{contentX, bottomY, contentX + leftW,
                             static_cast<int>(client.bottom) - 20};
            InvalidateRect(hwnd, &devicePanel, FALSE);
        } else if (wParam == 2 && g_playback) {
            if (g_playbackIndex < g_scenarioEvents.size()) {
                g_events.push_back(g_scenarioEvents[g_playbackIndex++]);
                g_analysis = analyze(g_events);
                InvalidateRect(hwnd, nullptr, TRUE);
            } else {
                KillTimer(hwnd, 2);
                g_playback = false;
                g_feedName = "DEMO SCENARIO COMPLETE";
                InvalidateRect(hwnd, nullptr, TRUE);
            }
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC target = BeginPaint(hwnd, &ps);
        RECT client{};
        GetClientRect(hwnd, &client);
        const int width = std::max(1, static_cast<int>(client.right));
        const int height = std::max(1, static_cast<int>(client.bottom));
        HDC buffer = CreateCompatibleDC(target);
        HBITMAP bitmap = CreateCompatibleBitmap(target, width, height);
        HGDIOBJ previous = SelectObject(buffer, bitmap);
        paintDashboard(hwnd, buffer);
        BitBlt(target, ps.rcPaint.left, ps.rcPaint.top,
               ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
               buffer, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
        SelectObject(buffer, previous);
        DeleteObject(bitmap);
        DeleteDC(buffer);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        const int mx = LOWORD(lParam);
        const int my = HIWORD(lParam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        const int contentX = SIDEBAR_W + 28;
        const int contentW = rect.right - contentX - 28;
        if (my >= 34 && my <= 68) {
            if (mx >= contentX + contentW - 445 && mx < contentX + contentW - 333) runLiveScenario();
            else if (mx >= contentX + contentW - 323 && mx < contentX + contentW - 231) resetDemo();
            else if (mx >= contentX + contentW - 221 && mx < contentX + contentW - 117) importCsv();
            else if (mx >= contentX + contentW - 109 && mx <= contentX + contentW) exportReport();
        } else {
            const int topY = 222;
            const int clientHeight = static_cast<int>(rect.bottom);
            const int topH = std::max(196, std::min(228, clientHeight / 3));
            const int bottomY = topY + topH + 13;
            const int bottomH = std::max(165, clientHeight - bottomY - 20);
            const int gap = 13;
            const int leftW = static_cast<int>((contentW - gap) * .43);
            const int rightX = contentX + leftW + gap;
            const int rightW = contentW - leftW - gap;
            const int headerY = bottomY + 59;
            if (mx >= rightX + rightW - 83 && mx <= rightX + rightW - 10 &&
                my >= headerY - 3 && my <= headerY + 19) {
                g_failedOnly = !g_failedOnly;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            (void)bottomH;
        }
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 1050;
        info->ptMinTrackSize.y = 690;
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    g_events = demoEvents();
    g_scenarioEvents = demoEvents();
    g_analysis = analyze(g_events);

    const wchar_t CLASS_NAME[] = L"LoginShieldNGWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassW(&wc)) return 1;

    g_window = CreateWindowExW(0, CLASS_NAME, L"LoginShield NG | Security Console",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               1220, 790, nullptr, nullptr, instance, nullptr);
    if (!g_window) return 1;
    ShowWindow(g_window, showCommand);
    UpdateWindow(g_window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

