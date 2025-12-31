#include <iostream>
#include <string>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <vector>   // 存储多根K线
#include <algorithm> // 用于查找最大/最小值

// 定义时间周期枚举
enum class TimeFrame {
    DAILY,    // 日线
    FOUR_HOUR // 4小时线
};

// K线数据结构体（单根K线的开/高/低/收/成交量）
struct KlineData {
    double open;   // 开盘价
    double high;   // 最高价
    double low;    // 最低价
    double close;  // 收盘价
    double volume; // 成交量（可选，用于密集成交区计算）
};

// 提前声明函数
double getInputValue(const std::string& prompt);
std::vector<KlineData> inputMultiKlineData(int klineCount);

// 多K线支撑压力位计算类
class SupportResistanceCalculator {
private:
    std::vector<KlineData> klineList; // 多根K线数据
    TimeFrame timeframe;              // 时间周期
    int klineCount;                   // K线数量
    // 历史高低点
    double highestHigh; // 阶段最高价
    double lowestLow;   // 阶段最低价
    // 枢轴点（取最新一根K线计算）
    double pivotPoint, s1, s2, s3, r1, r2, r3;
    // 密集成交区（收盘价均值±标准差）
    double avgClose;    // 收盘价均值
    double stdClose;    // 收盘价标准差
    double denseSupport; // 密集成交支撑位
    double denseResist;  // 密集成交阻力位

    // 校验多K线数据合法性
    void validateKlineList() const {
        if (klineList.empty()) {
            throw std::invalid_argument("K线数据不能为空");
        }
        for (const auto& kd : klineList) {
            if (kd.high < kd.low) {
                throw std::invalid_argument("存在最高价低于最低价的无效K线");
            }
        }
    }

    // 计算阶段历史高低点
    void calculateHistoryHighLow() {
        highestHigh = klineList[0].high;
        lowestLow = klineList[0].low;
        for (const auto& kd : klineList) {
            highestHigh = std::max(highestHigh, kd.high);
            lowestLow = std::min(lowestLow, kd.low);
        }
    }

    // 计算最新K线的枢轴点支撑阻力
    void calculatePivotPoint() {
        const KlineData& latestKline = klineList.back(); // 取最新一根K线
        pivotPoint = (latestKline.high + latestKline.low + latestKline.close) / 3.0;
        double range = latestKline.high - latestKline.low;
        // 枢轴点支撑位
        s1 = 2 * pivotPoint - latestKline.high;
        s2 = pivotPoint - range;
        s3 = pivotPoint - 2 * range;
        // 枢轴点阻力位
        r1 = 2 * pivotPoint - latestKline.low;
        r2 = pivotPoint + range;
        r3 = pivotPoint + 2 * range;
    }

    // 计算密集成交区支撑阻力（收盘价均值±标准差）
    void calculateDenseArea() {
        // 计算收盘价均值
        double sumClose = 0.0;
        for (const auto& kd : klineList) {
            sumClose += kd.close;
        }
        avgClose = sumClose / klineList.size();

        // 计算收盘价标准差
        double sumVar = 0.0;
        for (const auto& kd : klineList) {
            sumVar += pow(kd.close - avgClose, 2);
        }
        stdClose = sqrt(sumVar / klineList.size());

        // 密集成交区：均值±1倍标准差
        denseSupport = avgClose - stdClose;
        denseResist = avgClose + stdClose;
    }

public:
    // 构造函数：传入多根K线和时间周期
    SupportResistanceCalculator(std::vector<KlineData> klList, TimeFrame tf)
        : klineList(klList), timeframe(tf), klineCount(klList.size()) {
        validateKlineList();
        calculateHistoryHighLow();
        calculatePivotPoint();
        calculateDenseArea();
    }

    // 输出所有支撑阻力位结果
    void printAllSupportResistance() const {
        std::string tfName = (timeframe == TimeFrame::DAILY) ? "日线" : "4小时线";
        std::cout << "\n===== " << tfName << "支撑阻力位计算结果（共" << klineCount << "根K线）=====" << std::endl;

        std::cout << "\n【历史高低点支撑阻力】" << std::endl;
        std::cout << "阶段最高价（阻力）：" << highestHigh << " USDT" << std::endl;
        std::cout << "阶段最低价（支撑）：" << lowestLow << " USDT" << std::endl;

        std::cout << "\n【枢轴点支撑阻力（最新K线）】" << std::endl;
        std::cout << "枢轴点（P）：" << pivotPoint << std::endl;
        std::cout << "支撑位：S1=" << s1 << " | S2=" << s2 << " | S3=" << s3 << std::endl;
        std::cout << "阻力位：R1=" << r1 << " | R2=" << r2 << " | R3=" << r3 << std::endl;

        std::cout << "\n【密集成交区支撑阻力】" << std::endl;
        std::cout << "密集成交支撑位：" << denseSupport << " USDT" << std::endl;
        std::cout << "密集成交阻力位：" << denseResist << " USDT" << std::endl;
        std::cout << "===============================================\n" << std::endl;
    }

    // Getter方法
    double getHighestHigh() const { return highestHigh; }
    double getLowestLow() const { return lowestLow; }
    double getDenseSupport() const { return denseSupport; }
    double getDenseResist() const { return denseResist; }
};

// 辅助函数：选择时间周期
TimeFrame selectTimeframe() {
    int choice;
    std::cout << "请选择时间周期：" << std::endl;
    std::cout << "1. 日线（DAILY）" << std::endl;
    std::cout << "2. 4小时线（FOUR_HOUR）" << std::endl;
    std::cout << "请输入数字（1-2）：";
    std::cin >> choice;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        throw std::invalid_argument("输入不是有效数字");
    }
    return (choice == 1) ? TimeFrame::DAILY : TimeFrame::FOUR_HOUR;
}

// 辅助函数：输入多根K线数据
std::vector<KlineData> inputMultiKlineData(int klineCount) {
    std::vector<KlineData> klineList;
    for (int i = 0; i < klineCount; i++) {
        KlineData kd;
        std::cout << "\n请输入第" << i+1 << "根K线数据（USDT）：" << std::endl;
        kd.open = getInputValue("开盘价：");
        kd.high = getInputValue("最高价：");
        kd.low = getInputValue("最低价：");
        kd.close = getInputValue("收盘价：");
        kd.volume = getInputValue("成交量（可选，输入0即可）：");
        klineList.push_back(kd);
    }
    return klineList;
}

// 辅助函数：获取数值输入
double getInputValue(const std::string& prompt) {
    double value;
    std::cout << prompt;
    std::cin >> value;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        throw std::invalid_argument("输入不是有效数字");
    }
    return value;
}

// 主函数
int main() {
    char continueFlag;
    do {
        try {
            // 1. 选择时间周期
            TimeFrame tf = selectTimeframe();
            // 2. 输入K线数量
            int klineCount = static_cast<int>(getInputValue("请输入要分析的K线数量（如20根日线）："));
            if (klineCount <= 0) {
                throw std::invalid_argument("K线数量必须大于0");
            }
            // 3. 输入多根K线数据
            std::vector<KlineData> klineList = inputMultiKlineData(klineCount);
            // 4. 创建计算对象并输出结果
            SupportResistanceCalculator src(klineList, tf);
            src.printAllSupportResistance();

        } catch (const std::invalid_argument& e) {
            std::cerr << "错误：" << e.what() << "\n" << std::endl;
        }

        // 询问是否继续
        std::cout << "是否继续计算其他周期的支撑阻力位？(y/n)：";
        std::cin >> continueFlag;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    } while (continueFlag == 'y' || continueFlag == 'Y');

    std::cout << "程序结束！" << std::endl;
    return 0;
}
