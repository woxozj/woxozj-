#include <iostream>
#include <string>
#include <stdexcept>
#include <limits>
#include <cmath>

// 定义加密货币币种枚举
enum class CryptoCurrency {
    BTC,
    ETH,
    SOL,
    DOGE,
    UNKNOWN
};

// 定义交易方向枚举
enum class TradeDirection {
    LONG,  // 多单
    SHORT  // 空单
};

// 风险系数+强平价+保证金计算类
class CryptoRiskCalculator {
private:
    CryptoCurrency currency;    // 交易币种
    double leverage;            // 杠杆倍数
    double positionRatio;       // 单币仓位占总资金比例（0~100，百分比）
    double entryPrice;          // 入场价格（USDT）
    TradeDirection direction;   // 交易方向（多/空）
    double totalCapital;        // 总资金量（USDT）
    // 四种币种风险阈值统一为1000
    const double BTC_THRESHOLD = 1000.0;
    const double ETH_THRESHOLD = 1000.0;
    const double SOL_THRESHOLD = 1000.0;
    const double DOGE_THRESHOLD = 1000.0;
    // 永续合约核心参数（参考币安USDT本位永续合约）
    const double MAINTENANCE_MARGIN_RATE = 0.005; // 维持保证金率0.5%（可按交易所调整）

    // 获取币种风险阈值
    double getRiskThreshold() const {
        switch (currency) {
            case CryptoCurrency::BTC: return BTC_THRESHOLD;
            case CryptoCurrency::ETH: return ETH_THRESHOLD;
            case CryptoCurrency::SOL: return SOL_THRESHOLD;
            case CryptoCurrency::DOGE: return DOGE_THRESHOLD;
            default: throw std::invalid_argument("未知币种，无风险阈值");
        }
    }

    // 计算持仓价值（USDT）= 占用保证金 × 杠杆
    double getPositionValue() const {
        return getInitialMargin() * leverage;
    }

    // 计算持仓数量（币）= 持仓价值 / 入场价格
    double getPositionAmount() const {
        return getPositionValue() / entryPrice;
    }

public:
    // 构造函数
    CryptoRiskCalculator(CryptoCurrency cc, double lev, double posRatio, double entryP, TradeDirection dir, double totalCap)
        : currency(cc), leverage(lev), positionRatio(posRatio), entryPrice(entryP), direction(dir), totalCapital(totalCap) {
        if (leverage < 1) throw std::invalid_argument("杠杆倍数不能小于1（主流交易所最低1x）");
        if (positionRatio < 0 || positionRatio > 100) throw std::invalid_argument("仓位占比需在0~100之间（百分比）");
        if (entryPrice <= 0) throw std::invalid_argument("入场价格必须大于0");
        if (totalCapital <= 0) throw std::invalid_argument("总资金量必须大于0");
    }

    // 计算风险系数：杠杆 × 仓位占比
    double calculateRiskCoefficient() const {
        return leverage * positionRatio;
    }

    // 判定风险等级
    std::string judgeRiskLevel() const {
        double riskCoeff = calculateRiskCoefficient();
        double threshold = getRiskThreshold();

        if (riskCoeff == 0) return "无风险（未建仓/无杠杆）";
        else if (riskCoeff <= threshold * 0.8) return "安全（风险系数在阈值80%以内）";
        else if (riskCoeff <= threshold) return "预警（风险系数接近阈值）";
        else return "超标（风险系数超过阈值，禁止交易）";
    }

    // 计算初始保证金（占用保证金）= 总资金 × 仓位占比
    double getInitialMargin() const {
        return totalCapital * (positionRatio / 100.0);
    }

    // 计算维持保证金 = 持仓价值 × 维持保证金率
    double getMaintenanceMargin() const {
        return getPositionValue() * MAINTENANCE_MARGIN_RATE;
    }

    // 计算需补充保证金 = 维持保证金 - 剩余保证金（若剩余保证金不足则返回差值，否则0）
    double calculateMarginToAdd() const {
        double surplusMargin = getInitialMargin() - getUnrealizedLoss(); // 剩余保证金=初始保证金-未实现亏损
        double needAdd = getMaintenanceMargin() - surplusMargin;
        return needAdd > 0 ? needAdd : 0.0;
    }

    // 计算未实现亏损（用于强平价和保证金计算，简化版：按当前价格=强平价时的亏损）
    double getUnrealizedLoss() const {
        double liqPrice = calculateLiquidationPrice();
        double amount = getPositionAmount();
        if (direction == TradeDirection::LONG) {
            // 多单：价格低于入场价则亏损 = (入场价 - 强平价) × 持仓数量
            return (entryPrice - liqPrice) * amount;
        } else {
            // 空单：价格高于入场价则亏损 = (强平价 - 入场价) × 持仓数量
            return (liqPrice - entryPrice) * amount;
        }
    }

    // 计算强平价（USDT本位永续合约逐仓模式，主流交易所通用公式）
    double calculateLiquidationPrice() const {
        double im = getInitialMargin(); // 初始保证金
        double pv = getPositionValue();  // 持仓价值
        double mmr = MAINTENANCE_MARGIN_RATE; // 维持保证金率
        double amount = getPositionAmount(); // 持仓数量

        if (direction == TradeDirection::LONG) {
            // 多单强平价 = (entryPrice * (im + pv * mmr) - im * entryPrice) / (im + pv * mmr)
            // 简化后：
            return entryPrice - (im - pv * mmr) / amount;
        } else {
            // 空单强平价 = (entryPrice * (im + pv * mmr) + im * entryPrice) / (im + pv * mmr)
            // 简化后：
            return entryPrice + (im - pv * mmr) / amount;
        }
    }

    // 获取当前风险阈值
    double getThreshold() const {
        return getRiskThreshold();
    }
};

// 辅助函数：选择币种
CryptoCurrency selectCurrency() {
    int choice;
    std::cout << "请选择交易币种：" << std::endl;
    std::cout << "1. BTC" << std::endl;
    std::cout << "2. ETH" << std::endl;
    std::cout << "3. SOL" << std::endl;
    std::cout << "4. DOGE" << std::endl;
    std::cout << "请输入数字（1-4）：";
    std::cin >> choice;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        throw std::invalid_argument("输入不是有效数字，请重新选择");
    }

    switch (choice) {
        case 1: return CryptoCurrency::BTC;
        case 2: return CryptoCurrency::ETH;
        case 3: return CryptoCurrency::SOL;
        case 4: return CryptoCurrency::DOGE;
        default: throw std::invalid_argument("无效的币种选择，仅支持1-4");
    }
}

// 辅助函数：选择交易方向
TradeDirection selectTradeDirection() {
    int choice;
    std::cout << "请选择交易方向：" << std::endl;
    std::cout << "1. 多单（LONG）" << std::endl;
    std::cout << "2. 空单（SHORT）" << std::endl;
    std::cout << "请输入数字（1-2）：";
    std::cin >> choice;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        throw std::invalid_argument("输入不是有效数字，请重新选择");
    }

    switch (choice) {
        case 1: return TradeDirection::LONG;
        case 2: return TradeDirection::SHORT;
        default: throw std::invalid_argument("无效的方向选择，仅支持1-2");
    }
}

// 辅助函数：获取数值输入
double getInputValue(const std::string& prompt) {
    double value;
    std::cout << prompt;
    std::cin >> value;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        throw std::invalid_argument("输入不是有效数字，请重新输入");
    }
    return value;
}

// 主函数
int main() {
    char continueFlag;
    do {
        try {
            // 1. 基础参数输入
            CryptoCurrency currency = selectCurrency();
            TradeDirection direction = selectTradeDirection();
            double totalCapital = getInputValue("请输入总资金量（USDT）：");
            double leverage = getInputValue("请输入杠杆倍数（最小1x）：");
            double positionRatio = getInputValue("请输入仓位占比（0-100，百分比）：");
            double entryPrice = getInputValue("请输入入场价格（USDT）：");

            // 2. 创建计算器对象
            CryptoRiskCalculator riskCalc(currency, leverage, positionRatio, entryPrice, direction, totalCapital);

            // 3. 计算并输出结果
            std::cout << "\n===== 加密货币交易风险计算结果 =====" << std::endl;
            std::cout << "风险阈值：" << riskCalc.getThreshold() << std::endl;
            std::cout << "风险系数：" << riskCalc.calculateRiskCoefficient() << std::endl;
            std::cout << "风险等级：" << riskCalc.judgeRiskLevel() << std::endl;
            std::cout << "初始保证金（占用）：" << riskCalc.getInitialMargin() << " USDT" << std::endl;
            std::cout << "维持保证金要求：" << riskCalc.getMaintenanceMargin() << " USDT" << std::endl;
            std::cout << "需补充保证金数量：" << riskCalc.calculateMarginToAdd() << " USDT" << std::endl;
            std::cout << "强平价格：" << riskCalc.calculateLiquidationPrice() << " USDT" << std::endl;
            std::cout << "=====================================\n" << std::endl;

        } catch (const std::invalid_argument& e) {
            std::cerr << "错误：" << e.what() << "\n" << std::endl;
        }

        // 询问是否继续
        std::cout << "是否继续计算其他交易的风险？(y/n)：";
        std::cin >> continueFlag;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    } while (continueFlag == 'y' || continueFlag == 'Y');

    std::cout << "程序结束！" << std::endl;
    return 0;
}
