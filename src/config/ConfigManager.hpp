#pragma once

#include <map>
#include "../debug/Log.hpp"
#include <unordered_map>
#include "../defines.hpp"
#include <vector>
#include <deque>
#include <algorithm>
#include <regex>
#include "../Window.hpp"

#include "defaultConfig.hpp"

struct SConfigValue {
    int64_t intValue = -1;
    float floatValue = -1;
    std::string strValue = "";
};

struct SMonitorRule {
    std::string name = "";
    Vector2D    resolution = Vector2D(1280,720);
    Vector2D    offset = Vector2D(0,0);
    float       mfact = 0.5;
    float       scale = 1;
    float       refreshRate = 60;
    int         defaultWorkspaceID = -1;
};

struct SWindowRule {
    std::string szRule;
    std::string szValue;
};

class CConfigManager {
public:
    CConfigManager();

    void                tick();
    void                init();

    int                 getInt(std::string);
    float               getFloat(std::string);
    std::string         getString(std::string);
    void                setFloat(std::string, float);
    void                setInt(std::string, int);
    void                setString(std::string, std::string);

    SMonitorRule        getMonitorRuleFor(std::string);

    std::vector<SWindowRule> getMatchingRules(CWindow*);

    // no-op when done.
    void                dispatchExecOnce();

private:
    std::unordered_map<std::string, SConfigValue> configValues;
    time_t lastModifyTime = 0;  // for reloading the config if changed

    std::string currentCategory = "";  // For storing the category of the current item

    std::string parseError = "";  // For storing a parse error to display later

    bool isFirstLaunch = true;  // For exec-once

    std::deque<SMonitorRule> m_dMonitorRules;
    std::deque<SWindowRule> m_dWindowRules;

    bool firstExecDispatched = false;
    std::deque<std::string> firstExecRequests;

    // internal methods
    void                loadConfigLoadVars();
    SConfigValue        getConfigValueSafe(std::string);
    void                parseLine(std::string&);
    void                configSetValueSafe(const std::string&, const std::string&);
    void                handleRawExec(const std::string&, const std::string&);
    void                handleMonitor(const std::string&, const std::string&);
    void                handleBind(const std::string&, const std::string&);
    void                handleWindowRule(const std::string&, const std::string&);
    void                handleDefaultWorkspace(const std::string&, const std::string&);
};

inline std::unique_ptr<CConfigManager> g_pConfigManager;