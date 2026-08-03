#include "sft_schema.h"

#include <QHash>

namespace opentm::tm_core {

namespace {

struct row {
    const char* section;
    const char* key;
    const char* group;
    const char* label;
    sft_type    type;
    const char* choices;
    bool        control_only;
};

constexpr row kRows[] = {
{"System","nickname",        "System Settings","System Name",                sft_type::text,   "",false},
{"System","buttonAssign",    "Debug Settings", "O Button Behavior",          sft_type::choice,
                                                "0=Enter|1=Back", false},
{"System","debugGameType",   "Debug Settings", "Game Type (Debugger)",       sft_type::choice,
                                                "0=Disc Boot Game|1=HDD Boot Game|2=Patch|3=PARAM.SFO", false},
{"System","debugBootPath",   "Debug Settings", "GameContentUtil Boot Path",  sft_type::integer,"",false},
{"System","debugDirName",    "Debug Settings", "GameContentUtil dirName",    sft_type::text,   "",false},
{"System","appHomeBootPath", "Debug Settings", "GameContentUtil Boot Path (/app_home)", sft_type::integer,"",false},
{"System","dispHddSpace",    "Debug Settings", "Display HDD Free Space",     sft_type::boolean,"",false},
{"System","fakeFreeSpace",   "Debug Settings", "Fake Free Space",            sft_type::integer,"",false},
{"System","fakeLimitSize",   "Debug Settings", "Fake Limit Size (MB)",       sft_type::integer,"",false},
{"System","fakeSavedataOwner","Debug Settings","Fake Save Data Owner",       sft_type::boolean,"",false},
{"System","matEnable",       "Debug Settings", "Memory Access Trap",         sft_type::boolean,"",false},
{"System","powerOnReset",    "Debug Settings", "PowerOnReset",               sft_type::boolean,"",false},
{"System","disable15Timeout","Debug Settings", "Disable ExitGame Timeout",   sft_type::boolean,"",false},
{"System","updateServerUrl", "Debug Settings", "Update Server URL",          sft_type::text,   "",false},
{"System","wolDex",          "Debug Settings", "Wake On LAN",                sft_type::boolean,"",false},
{"System.tmp","fakeOtherRegion","Debug Settings","Fake Other Region",        sft_type::boolean,"",false},
{"System.tmp","dispMsgDialogUtilErrorcode","Debug Settings","MsgDialogUtil Display Errorcode", sft_type::boolean,"",false},
{"Display","gameResolution", "Debug Settings", "Game Output Resolution (Debugger)", sft_type::choice,
                                                "0=480 (4:3)|1=480 (16:9)|2=576 (4:3)|3=576 (16:9)|"
                                                "4=720|5=960 x 1080|6=1280 x 1080|7=1440 x 1080|"
                                                "8=1600 x 1080|9=1920 x 1080", false},
{"Display","hdcp",           "Debug Settings", "HDCP",                       sft_type::choice,
                                                "0=Off|1=On", false},
{"Sound","gameSound",        "Debug Settings", "Game Output Sound (Debugger)", sft_type::choice,
                                                "0=Maximum Number of Channels set on [Sound Settings] > "
                                                "[Audio Output Settings]|1=2 ch|"
                                                "2=2 ch (Downmix: 5.1 ch -> 2 ch)|"
                                                "3=2 ch (Downmix: 7.1 ch -> 2 ch)|4=5.1 ch|"
                                                "5=5.1 ch (Downmix: 7.1 ch -> 5.1 ch)", false},
{"Music","gameBgmPlayback",  "Debug Settings", "BGM Player (Debugger)",      sft_type::boolean,"",false},
{"Music","dummyBgmPlayer",   "Debug Settings", "Dummy BGM Player Debug",     sft_type::boolean,"",false},
{"Network","enable",         "Network Settings","Internet Connection",       sft_type::choice,
                                                "0=Disabled|1=Enabled", false},
{"Network","etherMode",      "Network Settings","Speed and Duplex",          sft_type::choice,
                                                "0=Auto-Detect|1=10BASE-T Half-Duplex|"
                                                "2=10BASE-T Full-Duplex|3=100BASE-TX Half-Duplex|"
                                                "4=100BASE-TX Full-Duplex|5=1000BASE-T Half-Duplex|"
                                                "6=1000BASE-T Full-Duplex", false},
{"Network","howToSetupIp",   "Network Settings","IP Address Setting",        sft_type::choice,
                                                "0=Automatic|1=Manual", false},
{"Network","setDhcpHostName","Network Settings","Set the DHCP host name",    sft_type::choice,
                                                "0=Do Not Set|1=Set", true},
{"Network","dhcpHostName",   "Network Settings","DHCP Host Name",            sft_type::text,   "",false},
{"Network","dnsFlag",        "Network Settings","DNS Setting",               sft_type::choice,
                                                "0=Automatic|1=Manual", false},
{"Network","primaryDns",     "Network Settings","Primary DNS",               sft_type::text,   "",false},
{"Network","secondaryDns",   "Network Settings","Secondary DNS",             sft_type::text,   "",false},
{"Network","ipAddress",      "Network Settings","IP Address",                sft_type::text,   "",false},
{"Network","netmask",        "Network Settings","Subnet Mask",               sft_type::text,   "",false},
{"Network","defaultRoute",   "Network Settings","Default Router",            sft_type::text,   "",false},
{"Network","mtu",            "Network Settings","MTU",                       sft_type::integer,"",false},
{"Network","httpProxyFlag",  "Network Settings","Proxy Server",              sft_type::choice,
                                                "0=Do Not Use|1=Use", false},
{"Network","httpProxyServer","Network Settings","Proxy Server Address",      sft_type::text,   "",false},
{"Network","httpProxyPort",  "Network Settings","Proxy Server Port",         sft_type::integer,"",false},
{"Network","upnpFlag",       "Network Settings","UPnP",                      sft_type::choice,
                                                "0=Enabled|1=Disabled", false},
{"Network","emulationType",  "Debug Settings", "Network Emulation Setting",  sft_type::integer,"",false},
{"Network","adhocSsidPrefix","Debug Settings", "Adhoc SSID Prefix",          sft_type::text,   "",false},
{"Network","device",         "Debug Settings", "WLAN Device",                sft_type::integer,"",false},
{"Network.eth2","setDhcpHostName","Network Settings (eth2)","Set the DHCP host name", sft_type::choice,
                                                "0=Do Not Set|1=Set", true},
{"Network.eth2","howToSetupIp","Network Settings (eth2)","IP Address Setting", sft_type::choice,
                                                "0=Automatic|1=Manual", false},
{"Network.eth2","dhcpHostName","Network Settings (eth2)","DHCP Host Name",   sft_type::text,   "",false},
{"Network.eth2","ipAddress", "Network Settings (eth2)","IP Address",         sft_type::text,   "",false},
{"Network.eth2","netmask",   "Network Settings (eth2)","Subnet Mask",        sft_type::text,   "",false},
{"NP","env",                 "Debug Settings", "NP Environment",             sft_type::text,   "", true},
{"NP","debug",               "Debug Settings", "NP Debug",                   sft_type::boolean,"",false},
{"NP","debugDrmError",       "Debug Settings", "NPDRM Debug",                sft_type::boolean,"",false},
{"NP","debugDrmClock",       "Debug Settings", "NPDRM Clock Debug",          sft_type::boolean,"",false},
{"NP","titleId",             "Debug Settings", "Service ID",                 sft_type::text,   "",false},
{"NP","npAdClockDiff",       "Debug Settings", "PlayStation(R)Store Ad Clock", sft_type::integer,"",false},
{"NP","gameUpdateImposeTest","Debug Settings", "GameUpdate Impose Test",     sft_type::boolean,"",false},
};

QHash<QString, const row*>& index() {
    static QHash<QString, const row*> map = [] {
        QHash<QString, const row*> m;
        for (const auto& r : kRows) {
            m.insert(QString::fromLatin1(r.section) + QLatin1Char('/')
                     + QString::fromLatin1(r.key), &r);
        }
        return m;
    }();
    return map;
}

} // namespace

sft_field describe(const QString& section, const QString& key) {
    const auto it = index().constFind(section + QLatin1Char('/') + key);
    if (it == index().constEnd()) {
        // unknown?
        return sft_field{QStringLiteral("Other"), key, sft_type::text, {}, false};
    }
    const row* r = *it;
    sft_field f;
    f.group        = QString::fromLatin1(r->group);
    f.label        = QString::fromLatin1(r->label);
    f.type         = r->type;
    f.control_only = r->control_only;
    const QString ch = QString::fromLatin1(r->choices);
    if (!ch.isEmpty()) {
        for (const auto& pair : ch.split(QLatin1Char('|'), Qt::SkipEmptyParts)) {
            const int eq = pair.indexOf(QLatin1Char('='));
            if (eq < 0) continue;
            f.choices.push_back(sft_choice{pair.left(eq), pair.mid(eq + 1)});
        }
    }
    return f;
}

QStringList coupled_keys(const QString& section) {
    if (section == QLatin1String("Network")) {
        return {QStringLiteral("upnpFlag"),       QStringLiteral("setDhcpHostName"),
                QStringLiteral("secondaryDns"),   QStringLiteral("primaryDns"),
                QStringLiteral("httpProxyFlag"),  QStringLiteral("howToSetupIp"),
                QStringLiteral("etherMode"),      QStringLiteral("enable"),
                QStringLiteral("dnsFlag"),        QStringLiteral("dhcpHostName")};
    }
    if (section == QLatin1String("Network.eth2")) {
        return {QStringLiteral("setDhcpHostName"), QStringLiteral("howToSetupIp")};
    }
    return {};
}

} // namespace opentm::tm_core
