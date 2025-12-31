#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cmath>
using namespace std;

// 时间跨度枚举（K线周期）
enum class Timeframe {
    TF_4H,
    TF_DAY,
    TF_WEEK,
};

// 价格形态时间跨度枚举（形态持续周期）
enum class PatternTimeframe {
    SHORT,  // 短期（≤1周）
    MEDIUM, // 中期（1-4周）
    LONG,   // 长期（＞4周）
};

// 三角形突破方向枚举
enum class TriangleBreakDir {
    UP,    // 向上突破上沿
    DOWN,  // 向下突破下沿
    NONE   // 未突破
};

// 转换K线周期为字符串
string timeframeToString(Timeframe tf) {
    switch (tf) {
        case Timeframe::TF_4H: return "4小时";
        case Timeframe::TF_DAY: return "日线";
        case Timeframe::TF_WEEK: return "周线";
        default: return "未知周期";
    }
}

// 转换形态时间跨度为字符串
string patternTfToString(PatternTimeframe ptf) {
    switch (ptf) {
        case PatternTimeframe::SHORT: return "短期（≤1周）";
        case PatternTimeframe::MEDIUM: return "中期（1-4周）";
        case PatternTimeframe::LONG: return "长期（＞4周）";
        default: return "未知周期";
    }
}

// 转换三角形突破方向为字符串
string triangleBreakDirToString(TriangleBreakDir dir) {
    switch (dir) {
        case TriangleBreakDir::UP: return "向上突破上沿";
        case TriangleBreakDir::DOWN: return "向下突破下沿";
        case TriangleBreakDir::NONE: return "未突破";
        default: return "未知突破方向";
    }
}

// EMA单时间跨度数据
struct EMAData {
    Timeframe tf;
    int period;
    string trend;
    bool isTurn;
};

// KST单时间跨度数据
struct KSTData {
    Timeframe tf;
    vector<int> periods;
    string cross;
};

// 价格形态结构体（含方向+突破方向）
struct PricePattern {
    string name;                  // 形态名称（如"三角形（收敛）"）
    PatternTimeframe tf;          // 形态时间跨度
    TriangleBreakDir breakDir;    // 三角形突破方向（非三角形形态默认NONE）
};

// 核心交易分析结构体
struct TradeAnalysis {
    string coinType;        // 交易币种
    string openDir;         // 开单方向（多/空）
    int leverage;           // 杠杆倍数
    double openPrice;       // 目标开单价
    double liquidPrice;     // 强平价
    double stopLoss;        // 止损价
    double stopLossRate;    // 基础止损率
    double leverStopLossRisk; // 杠杆止损风险率

    // 道氏趋势
    string longTrend;
    string midTrend;
    string shortTrend;
    int shortTrendLineBreakTimes; // 短期趋势线突破次数（≥0）

    // RSI指标
    string rsiLevel;
    int rsiDuration;
    string rsiUnit;

    // 价格形态
    vector<PricePattern> pricePatterns;

    // EMA/KST多时间跨度数据
    vector<EMAData> emaList;
    vector<KSTData> kstList;
};

// 工具函数：趋势校验
bool checkTrend(string& trend) {
    vector<string> valid = {"上升", "下降", "横盘"};
    if (find(valid.begin(), valid.end(), trend) != valid.end()) return true;
    cout << " 错误：仅支持「上升/下降/横盘」三种输入！" << endl;
    return false;
}

// 工具函数：是/否校验
bool checkYesNo(string& input, bool& result) {
    if (input == "是") { result = true; return true; }
    if (input == "否") { result = false; return true; }
    cout << "错误：仅支持是/否两种输入！" << endl;
    return false;
}

// 工具函数：开单方向校验
bool checkOpenDir(string& dir) {
    if (dir == "多" || dir == "空") return true;
    cout << " 错误：仅支持「多」或「空」两种输入！" << endl;
    return false;
}

// 工具函数：杠杆数校验
bool checkLeverage(int& lev) {
    if (lev >= 1) return true;
    cout << "错误：杠杆倍数需为≥1的正整数（如1/5/10/20）！" << endl;
    return false;
}

// 工具函数：形态时间跨度校验（强化提示）
bool checkPatternTf(string& input, PatternTimeframe& ptf) {
    input.erase(remove_if(input.begin(), input.end(), ::isspace), input.end()); // 去除空格
    if (input == "短期" || input == "短") {
        ptf = PatternTimeframe::SHORT;
        return true;
    }
    if (input == "中期" || input == "中") {
        ptf = PatternTimeframe::MEDIUM;
        return true;
    }
    if (input == "长期" || input == "长") {
        ptf = PatternTimeframe::LONG;
        return true;
    }
    cout << " 错误：仅支持「短期」「中期」「长期」（可简写为「短」「中」「长」）！" << endl;
    return false;
}

// 工具函数：三角形突破方向校验（强化提示）
bool checkTriangleBreakDir(string& input, TriangleBreakDir& dir) {
    input.erase(remove_if(input.begin(), input.end(), ::isspace), input.end()); // 去除空格
    if (input == "向上" || input == "上沿" || input == "上") {
        dir = TriangleBreakDir::UP;
        return true;
    }
    if (input == "向下" || input == "下沿" || input == "下") {
        dir = TriangleBreakDir::DOWN;
        return true;
    }
    if (input == "未突破" || input == "无" || input == "0") {
        dir = TriangleBreakDir::NONE;
        return true;
    }
    cout << "错误：仅支持「向上（上沿/上）」「向下（下沿/下）」「未突破（无/0）」！" << endl;
    return false;
}

// 工具函数：短期趋势线突破次数校验
bool checkShortBreakTimes(int& times) {
    if (times >= 0) return true;
    cout << " 错误：突破次数需为≥0的整数（0=未突破，1=1次突破...）！" << endl;
    return false;
}

// 第一步：录入开单基础参数
void inputTradeParams(TradeAnalysis& ta) {
    cout << "===== 第一步：录入开单基础参数 =====" << endl;
    cout << " 提示：所有价格需输入正数，开单方向仅支持「多/空」" << endl;
    cout << "请输入交易币种（如SOL/USDT、BTC/USDT）：";
    cin >> ta.coinType;

    while (true) {
        cout << "请输入开单方向（多/空）：";
        cin >> ta.openDir;
        if (checkOpenDir(ta.openDir)) break;
    }

    while (true) {
        cout << "请输入杠杆倍数（如1/5/10/20）：";
        cin >> ta.leverage;
        if (checkLeverage(ta.leverage)) break;
    }

    auto checkPrice = [](double& price, const string& name) {
        while (true) {
            cout << "请输入" << name << "（正数）：";
            cin >> price;
            if (price > 0) break;
            cout << "错误：价格需为正数！" << endl;
        }
    };
    checkPrice(ta.openPrice, "目标开单价");
    checkPrice(ta.liquidPrice, "强平价");
    checkPrice(ta.stopLoss, "止损价");

    // 计算止损率
    if (ta.openDir == "多") {
        if (ta.stopLoss >= ta.openPrice) cout << "  警告：多单止损价应低于开单价，当前设置可能不合理！" << endl;
        ta.stopLossRate = fabs((ta.openPrice - ta.stopLoss) / ta.openPrice) * 100;
    } else {
        if (ta.stopLoss <= ta.openPrice) cout << " 警告：空单止损价应高于开单价，当前设置可能不合理！" << endl;
        ta.stopLossRate = fabs((ta.stopLoss - ta.openPrice) / ta.openPrice) * 100;
    }
    ta.leverStopLossRisk = ta.stopLossRate * ta.leverage;
    cout << " 基础止损率：" << fixed << setprecision(2) << ta.stopLossRate << "%" << endl;
    cout << "杠杆止损风险率（止损率×杠杆）：" << fixed << setprecision(2) << ta.leverStopLossRisk << "%" << endl;
    cout << endl;
}

// 第二步：录入道氏理论趋势
void inputDowTrend(TradeAnalysis& ta) {
    cout << "===== 第二步：录入道氏理论趋势 =====" << endl;
    cout << " 提示：趋势仅支持「上升/下降/横盘」，短期突破次数≥7次将判定趋势失效" << endl;
    auto inputSingleTrend = [](const string& period) {
        string trend;
        while (true) {
            cout << "请输入" << period << "趋势（上升/下降/横盘）：";
            cin >> trend;
            if (checkTrend(trend)) break;
        }
        return trend;
    };
    ta.longTrend = inputSingleTrend("长期");
    ta.midTrend = inputSingleTrend("中期");
    ta.shortTrend = inputSingleTrend("短期");

    // 录入短期趋势线突破次数
    while (true) {
        cout << "请输入短期趋势线突破次数（≥0，例：0=未突破，1=1次突破）：";
        cin >> ta.shortTrendLineBreakTimes;
        if (checkShortBreakTimes(ta.shortTrendLineBreakTimes)) break;
    }
    cout << endl;
}

// 第三步：录入RSI指标
void inputRSI(TradeAnalysis& ta) {
    cout << "===== 第三步：录入RSI指标 =====" << endl;
    cout << " 提示：RSI水平仅支持「超买/超卖/正常」，持续时间需为正整数" << endl;
    vector<string> validRSI = {"超买", "超卖", "正常"};
    while (true) {
        cout << "当前RSI处于什么水平（超买/超卖/正常）：";
        cin >> ta.rsiLevel;
        if (find(validRSI.begin(), validRSI.end(), ta.rsiLevel) != validRSI.end()) break;
        cout << " 错误：仅支持「超买/超卖/正常」三种输入！" << endl;
    }
    while (true) {
        cout << "该RSI水平持续时间（数值，例：3=3小时/3天）：";
        cin >> ta.rsiDuration;
        if (ta.rsiDuration > 0) break;
        cout << "错误：持续时间需为正整数！" << endl;
    }
    cout << "持续时间单位（小时/天）：";
    cin >> ta.rsiUnit;
    cout << endl;
}

// 第四步：录入价格形态（重点优化：大量引导提示+格式校验）
void inputPricePatterns(TradeAnalysis& ta) {
    cout << "===== 第四步：录入价格形态 =====" << endl;
    cout << " 核心提示：" << endl;
    cout << "1. 支持多选，输入对应数字（空格分隔），输入0结束选择" << endl;
    cout << "2. 选择「无」将清空之前所有形态，直接结束该步骤" << endl;
    cout << "3. 所有形态需选择时间跨度，三角形需额外选择突破方向" << endl;
    cout << "4. 输入时可忽略空格（例：输入「短期」「短」均可）" << endl;
    cout << endl;

    // 优化后可选形态（带类型标注）
    vector<string> patterns = {
        "头肩顶（看跌）", "头肩底（看涨）", "向上旗形（看涨）", "向下旗形（看跌）",
        "三角形（收敛，延续趋势）", "三角形（发散，反转趋势）", "双重顶（看跌）", "双重底（看涨）", "无"
    };
    cout << "可选价格形态列表（编号+名称+类型）：" << endl;
    for (int i = 0; i < patterns.size(); ++i) {
        cout << i + 1 << ". " << patterns[i] << endl;
    }
    cout << endl;
    cout << "请输入形态对应编号（可多选，空格分隔，输入0结束）：";
    int choice;
    while (cin >> choice) {
        if (choice == 0) {
            cout << "已结束形态选择，当前共选择" << ta.pricePatterns.size() << "个形态" << endl;
            break;
        }
        if (choice >= 1 && choice <= patterns.size()) {
            string patName = patterns[choice - 1];
            if (patName == "无") {
                // 选择"无"则清空并结束
                ta.pricePatterns.clear();
                PricePattern pat = {"无", PatternTimeframe::SHORT, TriangleBreakDir::NONE};
                ta.pricePatterns.push_back(pat);
                cout << " 已选择「无」，清空所有形态" << endl;
                break;
            }
            cout << endl << "正在录入：" << patName << endl;

            // 第一步：输入形态时间跨度（强化引导）
            string tfInput;
            PatternTimeframe ptf;
            cout << "请输入该形态的时间跨度（短期/中期/长期，可简写为短/中/长）：";
            cin >> tfInput;
            while (!checkPatternTf(tfInput, ptf)) {
                cout << "请重新输入时间跨度（例：短期/中）：";
                cin >> tfInput;
            }
            cout << " 已选择时间跨度：" << patternTfToString(ptf) << endl;

            // 第二步：三角形形态额外输入突破方向（强化引导）
            TriangleBreakDir breakDir = TriangleBreakDir::NONE;
            if (patName.find("三角形") != string::npos) {
                string breakInput;
                cout << "请输入突破方向（向上=突破上沿/向下=突破下沿/未突破=无，可简写为上/下/无）：";
                cin >> breakInput;
                while (!checkTriangleBreakDir(breakInput, breakDir)) {
                    cout << "请重新输入突破方向（例：向上/下/未突破）：";
                    cin >> breakInput;
                }
                cout << "已选择突破方向：" << triangleBreakDirToString(breakDir) << endl;
            }

            // 清理形态名称（去除括号内的类型说明，仅保留核心名称）
            size_t pos = patName.find("（");
            if (pos != string::npos) {
                patName = patName.substr(0, pos);
            }

            // 添加到形态列表
            ta.pricePatterns.push_back({patName, ptf, breakDir});
            cout << "成功添加形态：" << patternTfToString(ptf) << "「" << patName << "」"
                 << (patName.find("三角形") != string::npos ? "（" + triangleBreakDirToString(breakDir) + "）" : "") << endl;
            cout << endl << "继续选择形态（输入编号，空格分隔，输入0结束）：";
        } else {
            cout << "无效选项！请输入1-" << patterns.size() << "之间的编号，或输入0结束：";
        }
    }
    cin.ignore();
    cout << endl;
}

// 第五步：录入多时间跨度EMA
void inputEMAData(TradeAnalysis& ta) {
    cout << "===== 第五步：录入多时间跨度EMA =====" << endl;
    cout << "提示：EMA周期建议选择量化常用值（12/26/50/100/200），趋势仅支持「上升/下降/横盘」" << endl;
    vector<Timeframe> tfs = {Timeframe::TF_4H, Timeframe::TF_DAY, Timeframe::TF_WEEK};
    for (auto tf : tfs) {
        EMAData ema;
        ema.tf = tf;
        cout << "\n--- " << timeframeToString(tf) << "EMA ---" << endl;
        while (true) {
            cout << "请输入EMA周期（正整数，例：12/26/50）：";
            cin >> ema.period;
            if (ema.period > 0) break;
            cout << " 错误：周期需为正整数！" << endl;
        }
        while (true) {
            cout << timeframeToString(tf) << ema.period << "期EMA趋势（上升/下降/横盘）：";
            cin >> ema.trend;
            if (checkTrend(ema.trend)) break;
        }
        string turnChoice;
        bool isTurn;
        while (true) {
            cout << timeframeToString(tf) << ema.period << "期EMA是否转折（是/否）：";
            cin >> turnChoice;
            if (checkYesNo(turnChoice, isTurn)) break;
        }
        ema.isTurn = isTurn;
        ta.emaList.push_back(ema);
        cout << "已录入" << timeframeToString(tf) << ema.period << "期EMA：趋势=" << ema.trend << "，转折=" << (ema.isTurn ? "是" : "否") << endl;
    }
    cout << endl;
}

// 第六步：录入多时间跨度KST
void inputKSTData(TradeAnalysis& ta) {
    cout << "===== 第六步：录入多时间跨度KST =====" << endl;
    cout << "提示：KST周期组合为4个正整数（逗号分隔），例：默认10,15,20,30；短线6,9,12,15" << endl;
    vector<Timeframe> tfs = {Timeframe::TF_4H, Timeframe::TF_DAY, Timeframe::TF_WEEK};
    vector<string> validKSTCross = {"向上穿越", "向下穿越", "未穿越"};
    for (auto tf : tfs) {
        KSTData kst;
        kst.tf = tf;
        cout << "\n--- " << timeframeToString(tf) << "KST ---" << endl;
        cout << "请输入KST周期组合（4个数字，逗号分隔，例：10,15,20,30）：";
        string periodStr;
        cin >> periodStr;
        // 分割逗号获取周期（添加格式校验）
        size_t pos = 0;
        kst.periods.clear();
        while ((pos = periodStr.find(",")) != string::npos) {
            string numStr = periodStr.substr(0, pos);
            try {
                kst.periods.push_back(stoi(numStr));
            } catch (...) {
                cout << "错误：周期需为正整数！请重新输入该KST周期组合：";
                cin >> periodStr;
                pos = 0;
                kst.periods.clear();
                continue;
            }
            periodStr.erase(0, pos + 1);
        }
        try {
            kst.periods.push_back(stoi(periodStr));
        } catch (...) {
            cout << " 错误：周期需为正整数！请重新输入该KST周期组合：";
            cin >> periodStr;
            // 重新解析
            kst.periods.clear();
            pos = 0;
            while ((pos = periodStr.find(",")) != string::npos) {
                kst.periods.push_back(stoi(periodStr.substr(0, pos)));
                periodStr.erase(0, pos + 1);
            }
            kst.periods.push_back(stoi(periodStr));
        }
        // 确保周期数为4个
        while (kst.periods.size() != 4) {
            cout << " 错误：KST周期组合需为4个数字！请重新输入：";
            cin >> periodStr;
            kst.periods.clear();
            pos = 0;
            while ((pos = periodStr.find(",")) != string::npos) {
                kst.periods.push_back(stoi(periodStr.substr(0, pos)));
                periodStr.erase(0, pos + 1);
            }
            kst.periods.push_back(stoi(periodStr));
        }
        // 校验周期为正整数
        bool validPeriod = true;
        for (int p : kst.periods) {
            if (p <= 0) {
                validPeriod = false;
                break;
            }
        }
        while (!validPeriod) {
            cout << " 错误：周期需为正整数！请重新输入：";
            cin >> periodStr;
            kst.periods.clear();
            pos = 0;
            while ((pos = periodStr.find(",")) != string::npos) {
                kst.periods.push_back(stoi(periodStr.substr(0, pos)));
                periodStr.erase(0, pos + 1);
            }
            kst.periods.push_back(stoi(periodStr));
            validPeriod = true;
            for (int p : kst.periods) {
                if (p <= 0) {
                    validPeriod = false;
                    break;
                }
            }
        }
        // 输入穿越情况
        while (true) {
            cout << timeframeToString(tf) << "KST是否穿越均线（向上穿越/向下穿越/未穿越）：";
            cin >> kst.cross;
            if (find(validKSTCross.begin(), validKSTCross.end(), kst.cross) != validKSTCross.end()) break;
            cout << "错误：仅支持「向上穿越/向下穿越/未穿越」三种输入！" << endl;
        }
        ta.kstList.push_back(kst);
        cout << "已录入" << timeframeToString(tf) << "KST：周期=";
        for (size_t i = 0; i < kst.periods.size(); ++i) {
            if (i > 0) cout << ",";
            cout << kst.periods[i];
        }
        cout << "，穿越情况=" << kst.cross << endl;
    }
    cout << endl;
}

// 计算EMA信号一致性得分
int calculateEMAConsistency(const vector<EMAData>& emaList) {
    if (emaList.empty()) return 0;
    map<string, int> trendCount;
    for (const auto& ema : emaList) trendCount[ema.trend]++;
    int maxCount = 0;
    for (const auto& pair : trendCount) maxCount = max(maxCount, pair.second);
    return (maxCount * 100) / emaList.size();
}

// 计算KST信号一致性得分
int calculateKSTConsistency(const vector<KSTData>& kstList) {
    if (kstList.empty()) return 0;
    map<string, int> crossCount;
    for (const auto& kst : kstList) crossCount[kst.cross]++;
    int maxCount = 0;
    for (const auto& pair : crossCount) maxCount = max(maxCount, pair.second);
    return (maxCount * 100) / kstList.size();
}

// 计算基础止损率合理性得分
int calculateBaseStopLossScore(const TradeAnalysis& ta) {
    double rate = ta.stopLossRate;
    if (rate >= 3.0 && rate <= 8.0) return 10;
    else if ((rate >= 1.0 && rate < 3.0) || (rate > 8.0 && rate <= 10.0)) return 5;
    else return 0;
}

// 计算杠杆止损风险得分
int calculateLeverStopLossScore(const TradeAnalysis& ta, bool& isHighRisk) {
    double leverRisk = ta.leverStopLossRisk;
    isHighRisk = false;
    if (leverRisk <= 40.0) return 10;
    else if (leverRisk > 40.0 && leverRisk <= 60.0) return 5;
    else {
        isHighRisk = true;
        return 0;
    }
}

// 计算开单方向与趋势匹配度得分（含短期趋势线突破次数影响）
int calculateDirTrendMatchScore(const TradeAnalysis& ta) {
    int matchCount = 0;
    if (ta.openDir == "多") {
        if (ta.longTrend == "上升") matchCount++;
        if (ta.midTrend == "上升") matchCount++;
        if (ta.shortTrend == "上升") matchCount++;
    } else {
        if (ta.longTrend == "下降") matchCount++;
        if (ta.midTrend == "下降") matchCount++;
        if (ta.shortTrend == "下降") matchCount++;
    }
    int baseScore = 0;
    if (matchCount == 3) baseScore = 20;
    else if (matchCount == 2) baseScore = 15;
    else if (matchCount == 1) baseScore = 5;
    else baseScore = 0;

    // 短期趋势线突破次数扣分
    int breakTimes = ta.shortTrendLineBreakTimes;
    int penalty = 0;
    if (breakTimes == 3) penalty = 3;
    else if (breakTimes == 5) penalty = 8;
    else if (breakTimes >= 7) penalty = 15;

    return max(baseScore - penalty, 0);
}

// 综合一致性评分
int calculateTotalConsistency(const TradeAnalysis& ta, bool& isHighLeverRisk) {
    int emaScore = calculateEMAConsistency(ta.emaList);
    int kstScore = calculateKSTConsistency(ta.kstList);
    int baseSLScore = calculateBaseStopLossScore(ta);
    int leverSLScore = calculateLeverStopLossScore(ta, isHighLeverRisk);
    int dirMatchScore = calculateDirTrendMatchScore(ta);
    return (emaScore * 0.3) + (kstScore * 0.3) + baseSLScore + leverSLScore + dirMatchScore;
}

// 分析指标矛盾点
vector<string> analyzeContradictions(const TradeAnalysis& ta, bool isHighLeverRisk) {
    vector<string> contradictions;
    int emaScore = calculateEMAConsistency(ta.emaList);
    int kstScore = calculateKSTConsistency(ta.kstList);
    double baseSLRate = ta.stopLossRate;
    double leverSLRisk = ta.leverStopLossRisk;
    int shortBreakTimes = ta.shortTrendLineBreakTimes;

    // 趋势与RSI矛盾
    if ((ta.longTrend == "上升" || ta.midTrend == "上升") && ta.rsiLevel == "超买") {
        contradictions.push_back("长/中期趋势向上，但RSI超买，趋势延续性存疑");
    }
    if ((ta.longTrend == "下降" || ta.midTrend == "下降") && ta.rsiLevel == "超卖") {
        contradictions.push_back("长/中期趋势向下，但RSI超卖，趋势延续性存疑");
    }

    // 短期趋势线突破次数矛盾
    if (shortBreakTimes >= 2) {
        contradictions.push_back("短期趋势线突破次数≥2次，趋势有效性减弱，开单逻辑一致性下降");
    }
    if (shortBreakTimes >= 3) {
        contradictions.push_back("【高风险提醒】短期趋势线突破次数≥3次，趋势已失效，开单逻辑缺乏支撑");
    }

    // 价格形态矛盾（含突破方向）
    for (const auto& pat : ta.pricePatterns) {
        if (pat.name == "无") continue;
        string patTf = patternTfToString(pat.tf);

        // 看涨形态与下跌趋势矛盾
        if ((pat.name == "头肩底" || pat.name == "向上旗形" || pat.name == "双重底") &&
            ((pat.tf == PatternTimeframe::LONG && ta.longTrend == "下降") ||
             (pat.tf == PatternTimeframe::MEDIUM && ta.midTrend == "下降") ||
             (pat.tf == PatternTimeframe::SHORT && ta.shortTrend == "下降"))) {
            contradictions.push_back(patTf + "「" + pat.name + "」（看涨形态）与对应周期" +
                                  (pat.tf == PatternTimeframe::LONG ? "长期" : (pat.tf == PatternTimeframe::MEDIUM ? "中期" : "短期")) +
                                  "下降趋势冲突");
        }

        // 看跌形态与上涨趋势矛盾
        if ((pat.name == "头肩顶" || pat.name == "向下旗形" || pat.name == "双重顶") &&
            ((pat.tf == PatternTimeframe::LONG && ta.longTrend == "上升") ||
             (pat.tf == PatternTimeframe::MEDIUM && ta.midTrend == "上升") ||
             (pat.tf == PatternTimeframe::SHORT && ta.shortTrend == "上升"))) {
            contradictions.push_back(patTf + "「" + pat.name + "」（看跌形态）与对应周期" +
                                  (pat.tf == PatternTimeframe::LONG ? "长期" : (pat.tf == PatternTimeframe::MEDIUM ? "中期" : "短期")) +
                                  "上升趋势冲突");
        }

        // 三角形形态矛盾
        if (pat.name == "三角形（收敛）") {
            if (ta.longTrend == "横盘") {
                contradictions.push_back(patTf + "「收敛三角形」需依托明确趋势，长期横盘下形态有效性存疑");
            }
            if (ta.shortTrend == "上升" && pat.breakDir == TriangleBreakDir::DOWN) {
                contradictions.push_back(patTf + "「收敛三角形」短期趋势向上，但向下突破下沿，趋势延续性矛盾");
            }
            if (ta.shortTrend == "下降" && pat.breakDir == TriangleBreakDir::UP) {
                contradictions.push_back(patTf + "「收敛三角形」短期趋势向下，但向上突破上沿，趋势延续性矛盾");
            }
        }
        if (pat.name == "三角形（发散）") {
            if (ta.longTrend != "横盘" && pat.breakDir == TriangleBreakDir::NONE) {
                contradictions.push_back(patTf + "「发散三角形」预示趋势反转，但未突破，形态信号无效");
            }
            if (pat.breakDir == TriangleBreakDir::UP && ta.openDir == "空") {
                contradictions.push_back(patTf + "「发散三角形」向上突破，与空单开单方向冲突");
            }
            if (pat.breakDir == TriangleBreakDir::DOWN && ta.openDir == "多") {
                contradictions.push_back(patTf + "「发散三角形」向下突破，与多单开单方向冲突");
            }
        }
    }

    // EMA/KST一致性低矛盾
    if (emaScore < 60) contradictions.push_back("EMA多时间跨度信号一致性低（<60分），趋势判断混乱");
    if (kstScore < 60) contradictions.push_back("KST多时间跨度信号一致性低（<60分），穿越信号混乱");

    // 止损率矛盾
    if (baseSLRate > 10.0) contradictions.push_back("基础止损率超过10%，无杠杆时风险已偏高");
    if (baseSLRate < 1.0) contradictions.push_back("基础止损率低于1%，易被小幅波动扫损");

    // 杠杆止损风险矛盾
    if (leverSLRisk > 60.0) {
        contradictions.push_back("【高风险提醒】杠杆止损风险率＞60%，触发止损将亏损超60%保证金，极端风险！");
    } else if (leverSLRisk > 40.0) {
        contradictions.push_back("杠杆止损风险率40%-60%，止损风险偏高，需谨慎开单");
    }

    // 开单方向与趋势矛盾
    int dirMatch = calculateDirTrendMatchScore(ta);
    if (dirMatch == 0) contradictions.push_back("开单方向与趋势匹配度为0，且短期趋势线突破频繁，开单逻辑无效");

    return contradictions;
}

// 输出综合分析报告
void outputAnalysis(const TradeAnalysis& ta) {
    cout << "==============================================" << endl;
    cout << "========== 交易开单逻辑综合分析报告 ==========" << endl;
    cout << "==============================================" << endl;

    // 1. 开单基础参数
    cout << "\n【一、开单基础参数】" << endl;
    cout << "交易币种：" << ta.coinType << endl;
    cout << "开单方向：" << ta.openDir << endl;
    cout << "杠杆倍数：" << ta.leverage << "x" << endl;
    cout << "目标开单价：" << fixed << setprecision(4) << ta.openPrice << endl;
    cout << "强平价：" << fixed << setprecision(4) << ta.liquidPrice << endl;
    cout << "止损价：" << fixed << setprecision(4) << ta.stopLoss << endl;
    cout << "基础止损率：" << fixed << setprecision(2) << ta.stopLossRate << "%" << endl;
    cout << "杠杆止损风险率：" << fixed << setprecision(2) << ta.leverStopLossRisk << "%" << endl;

    // 2. 道氏趋势+RSI+价格形态
    cout << "\n【二、核心技术面分析】" << endl;
    cout << "道氏理论趋势：长期=" << ta.longTrend << "，中期=" << ta.midTrend << "，短期=" << ta.shortTrend << endl;
    cout << "短期趋势线突破次数：" << ta.shortTrendLineBreakTimes << "次（次数越多趋势越弱）" << endl;
    cout << "RSI指标：" << ta.rsiLevel << "（持续" << ta.rsiDuration << ta.rsiUnit << "）" << endl;
    cout << "价格形态：";
    if (ta.pricePatterns.empty() || ta.pricePatterns[0].name == "无") {
        cout << "无";
    } else {
        for (size_t i = 0; i < ta.pricePatterns.size(); ++i) {
            if (i > 0) cout << "、";
            cout << patternTfToString(ta.pricePatterns[i].tf) << "「" << ta.pricePatterns[i].name << "」";
            if (ta.pricePatterns[i].name.find("三角形") != string::npos) {
                cout << "（" << triangleBreakDirToString(ta.pricePatterns[i].breakDir) << "）";
            }
        }
    }
    cout << endl;

    // 3. 多时间跨度EMA详情
    cout << "\n【三、多时间跨度EMA分析】" << endl;
    int emaScore = calculateEMAConsistency(ta.emaList);
    for (const auto& ema : ta.emaList) {
        cout << timeframeToString(ema.tf) << ema.period << "期EMA：趋势=" << ema.trend << "，转折=" << (ema.isTurn ? "是" : "否") << endl;
    }
    cout << "EMA信号一致性得分：" << emaScore << "/100" << endl;

    // 4. 多时间跨度KST详情
    cout << "\n【四、多时间跨度KST分析】" << endl;
    int kstScore = calculateKSTConsistency(ta.kstList);
    for (const auto& kst : ta.kstList) {
        cout << timeframeToString(kst.tf) << "KST（周期：";
        for (size_t i = 0; i < kst.periods.size(); ++i) {
            if (i > 0) cout << ",";
            cout << kst.periods[i];
        }
        cout << "）：" << kst.cross << endl;
    }
    cout << "KST信号一致性得分：" << kstScore << "/100" << endl;

    // 5. 风险控制分析
    cout << "\n【五、风险控制分析】" << endl;
    int baseSLScore = calculateBaseStopLossScore(ta);
    bool isHighLeverRisk = false;
    int leverSLScore = calculateLeverStopLossScore(ta, isHighLeverRisk);
    int dirMatchScore = calculateDirTrendMatchScore(ta);
    cout << "基础止损率合理性得分：" << baseSLScore << "/10" << "（合理区间3%-8%）" << endl;
    cout << "杠杆止损风险得分：" << leverSLScore << "/10" << "（≤40%满分，40%-60%5分，＞60%0分）" << endl;
    if (isHighLeverRisk) {
        cout << "\033[31m【紧急提醒】杠杆止损风险率＞60%，触发止损将导致大幅亏损，建议立即调整杠杆/止损价！\033[0m" << endl;
    }
    cout << "开单方向与趋势匹配度得分：" << dirMatchScore << "/20" << "（短期突破次数已扣分）" << endl;

    // 6. 综合一致性评估
    cout << "\n【六、信号一致性综合评估】" << endl;
    int totalScore = calculateTotalConsistency(ta, isHighLeverRisk);
    cout << "综合一致性总分：" << totalScore << "/100" << endl;
    if (totalScore >= 80) cout << "评价：信号高度一致，风险控制合理，开单逻辑具备强技术支撑";
    else if (totalScore >= 60) cout << "评价：信号基本一致，风险控制尚可，开单逻辑有一定技术支撑";
    else cout << "评价：信号混乱或风险控制不合理，开单逻辑支撑弱，建议观望";
    cout << endl;

    // 7. 矛盾点分析
    cout << "\n【七、指标矛盾点识别】" << endl;
    vector<string> contradictions = analyzeContradictions(ta, isHighLeverRisk);
    if (contradictions.empty()) cout << "未识别到明显指标矛盾点";
    else {
        for (int i = 0; i < contradictions.size(); ++i) {
            cout << i + 1 << ". " << contradictions[i] << endl;
        }
    }

    cout << "\n==============================================" << endl;
}

int main() {
    TradeAnalysis ta;
    cout << "===== 交易开单逻辑分析程序（完美优化版）=====\n" << endl;
    cout << " 程序说明：" << endl;
    cout << "1. 全程带格式校验和引导提示，输入错误会明确告知正确格式" << endl;
    cout << "2. 价格形态支持多选，三角形需额外选择突破方向" << endl;
    cout << "3. 最终输出综合评分和矛盾点，辅助开单决策" << endl;
    cout << "4. 所有输入支持去除空格（例：输入「短期」「短」效果一致）" << endl;
    cout << endl;

    // 分步录入数据
    inputTradeParams(ta);
    inputDowTrend(ta);
    inputRSI(ta);
    inputPricePatterns(ta);
    inputEMAData(ta);
    inputKSTData(ta);

    // 输出分析报告
    outputAnalysis(ta);

    return 0;
}
