#pragma once
#include <random>
#include <mutex>
#include <shared_mutex>

// 修复：random.h 的全局随机引擎（gen_mt 等）与 Random() 内的 static disMap 均非线程安全。
// RClickRun / LClickRun 两条连点线程会并发调用 Random/RandomBool → 数据竞争（UB）。
// 统一用互斥锁保护全部全局随机状态（调用频率=点击频率，锁开销可忽略）。
static mutex RandomMtx;

bool RandomBool(const float& probability, UINT uGen, UINT uDist);
int Random(int min, int max, UINT uGen, UINT uDist);
void InitRandom();

static random_device                                                      rd;
static mt19937                                                            gen_mt(rd());
static linear_congruential_engine  <uint_fast32_t, 16807, 0, 2147483647>  gen_lce(rd());
static ranlux24_base                                                      gen_rlx(rd());
static minstd_rand                                                        gen_ms(rd());
static discard_block_engine        <minstd_rand, 256, 256>                gen_db(rd());
static independent_bits_engine     <minstd_rand, 32, unsigned int>        gen_ib(rd());
static shuffle_order_engine        <minstd_rand, 256>                     gen_so(rd());
static subtract_with_carry_engine  <uint32_t, 32, 10, 24>                 gen_swc(rd());

static bool ClearRandom = false;

void InitRandom()
{
    ClearRandom = true;
    // 调用一次随机数生成器，使其重新初始化
    Random(0, 0, GenerateGen_mersenne_twister, GenerateDis_uniform_int_distribution);
    Random(0, 0, GenerateGen_mersenne_twister, GenerateDis_binomial_distribution);
    ClearRandom = false;
}

int Random(int min, int max, UINT uGen = GenerateGen_mersenne_twister, UINT uDist = GenerateDis_uniform_int_distribution)
{
    if (min > max)
        return -1;
    if (min == max)
        return min;
    uint64_t id = static_cast<uint32_t>(min) | static_cast<uint32_t>(max);

    if (uGen < 0 || uGen > 7)
        uGen = GenerateGen_mersenne_twister;

    if (uDist <= 0 || uDist > 3)//uDist不能为0（伯努利分布）
        uDist = GenerateDis_uniform_int_distribution;

    // 修复：保护 static disMap 与全局随机引擎（RClickRun/LClickRun 等多线程并发调用）
    lock_guard<mutex> lock(RandomMtx);
    switch (uDist)
    {
        case GenerateDis_uniform_int_distribution:
        {
            static map<long long, uniform_int_distribution<>> disMap;
            if (ClearRandom)
            {
                disMap.clear();
                return -2;
            }
            disMap[id].param(uniform_int_distribution<>::param_type(min, max));
            switch (uGen)
            {
                case GenerateGen_mersenne_twister:            return disMap[id](gen_mt);
                case GenerateGen_linear_congruential_engine:  return disMap[id](gen_lce);
                case GenerateGen_ranlux24_base:               return disMap[id](gen_rlx);
                case GenerateGen_minstd_rand:                 return disMap[id](gen_ms);
                case GenerateGen_discard_block_engine:        return disMap[id](gen_db);
                case GenerateGen_independent_bits_engine:     return disMap[id](gen_ib);
                case GenerateGen_shuffle_order_engine:        return disMap[id](gen_so);
                case GenerateGen_subtract_with_carry_engine:  return disMap[id](gen_swc);
            }
            return -3;
        }
        case GenerateDis_binomial_distribution:
        {
            static map<long long, binomial_distribution<>> disMap;
            if (ClearRandom)
            {
                disMap.clear();
                return -2;
            }
            disMap[id].param(binomial_distribution<>::param_type(min, max));
            switch (uGen)
            {
                case GenerateGen_mersenne_twister:            return disMap[id](gen_mt);
                case GenerateGen_linear_congruential_engine:  return disMap[id](gen_lce);
                case GenerateGen_ranlux24_base:               return disMap[id](gen_rlx);
                case GenerateGen_minstd_rand:                 return disMap[id](gen_ms);
                case GenerateGen_discard_block_engine:        return disMap[id](gen_db);
                case GenerateGen_independent_bits_engine:     return disMap[id](gen_ib);
                case GenerateGen_shuffle_order_engine:        return disMap[id](gen_so);
                case GenerateGen_subtract_with_carry_engine:  return disMap[id](gen_swc);
            }
            return -3;
        }
    }
    return -4;
}

bool RandomBool(const float& probability, UINT uGen = GenerateGen_mersenne_twister, UINT uDist = GenerateDis_bernoulli_distribution)
{
    if (probability <= 0.0f)
        return false;
    if(probability >= 1.0f)
        return true;

    if (uGen < 0 || uGen > 7)
        uGen = GenerateGen_mersenne_twister;

    if (uDist < 0 || uDist > 3)
        uDist = GenerateDis_uniform_int_distribution;

    if (uDist == GenerateDis_bernoulli_distribution)
    {
        bernoulli_distribution dis(probability);
        // 修复：保护全局随机引擎（多条连点线程并发调用）
        lock_guard<mutex> lock(RandomMtx);
        switch (uGen)
        {
            case GenerateGen_mersenne_twister:            return dis(gen_mt);
            case GenerateGen_linear_congruential_engine:  return dis(gen_lce);
            case GenerateGen_ranlux24_base:               return dis(gen_rlx);
            case GenerateGen_minstd_rand:                 return dis(gen_ms);
            case GenerateGen_discard_block_engine:        return dis(gen_db);
            case GenerateGen_independent_bits_engine:     return dis(gen_ib);
            case GenerateGen_shuffle_order_engine:        return dis(gen_so);
            case GenerateGen_subtract_with_carry_engine:  return dis(gen_swc);
        }
        return false;
    }
    return (Random(0, 100, uGen, uDist) <= (probability * 100));
}

class RandomBoolManager
{
private:
    RandomBoolMode current_mode = RandomBoolMode_BIONIC;
    double error_rate = 0.08;                     // 基础失误率

    UINT m_uGen = GenerateGen_mersenne_twister;  // 当前使用的引擎
    UINT m_uDist = GenerateDis_bernoulli_distribution; // 当前使用的分布（默认伯努利）

    chrono::steady_clock::time_point start_time;

    // 马尔可夫状态（BIONIC 模式）
    int recent_failures = 0;
    int consecutive_successes = 0;

    // 虚拟光标位置（BIONIC 模式模拟手抖）
    int virtual_x = 0, virtual_y = 0;

    // 虚拟推进的时间（预览用）：让疲劳因子不再只依赖真实流逝时间
    chrono::steady_clock::duration extra_elapsed = chrono::steady_clock::duration::zero();

    void ResetInternalState()
    {
        recent_failures = 0;
        consecutive_successes = 0;
        virtual_x = 0;
        virtual_y = 0;
        extra_elapsed = chrono::steady_clock::duration::zero();
        start_time = chrono::steady_clock::now();
    }

    // 疲劳因子（0.8 ~ 2.2）
    double CalcFatigueFactor() const
    {
        auto elapsed = (chrono::steady_clock::now() - start_time) + extra_elapsed;
        double minutes = chrono::duration<double>(elapsed).count() / 60.0;
        double factor = 1.0 + 0.8 * log10(minutes + 1.0);
        return clamp(factor, 0.8, 2.2);
    }

    // 距离因子（0.5 ~ 2.8），模拟手部移动导致的失误率变化
    double CalcDistanceFactor()
    {
        // 使用你封装的 Random 生成 -15 ~ 15 的偏移（整数均匀分布）
        int deltaX = Random(-15, 15, m_uGen, GenerateDis_uniform_int_distribution);
        int deltaY = Random(-15, 15, m_uGen, GenerateDis_uniform_int_distribution);
        virtual_x += deltaX;
        virtual_y += deltaY;
        virtual_x = clamp(virtual_x, -300, 300);
        virtual_y = clamp(virtual_y, -300, 300);
        double distance = sqrt(virtual_x * virtual_x + virtual_y * virtual_y);
        double factor = 0.6 + (distance / 250.0) * 1.9;
        return clamp(factor, 0.5, 2.8);
    }

public:
    // 构造函数：不再需要种子，全局引擎已经初始化
    RandomBoolManager() : start_time(chrono::steady_clock::now()) {}

    // 设置基础失误率（1% ~ 95%）
    void SetErrorRate(double rate)
    {
        error_rate = clamp(rate, 0.01, 0.95);
    }

    double GetErrorRate() const
    {
        return error_rate;
    }

    // 切换随机模式
    void SetMode(RandomBoolMode mode)
    {
        current_mode = mode;
        ResetInternalState();
    }

    // 自定义随机引擎（对应 GenerateGen_* 常量）
    void SetEngine(UINT uGen)
    {
        if (uGen < 0 || uGen > 7)
            uGen = GenerateGen_mersenne_twister;
        m_uGen = uGen;
    }

    // 自定义随机分布（对应 GenerateDis_* 常量，建议使用伯努利或均匀整数）
    void SetDistribution(UINT uDist)
    {
        if (uDist < 0 || uDist > 3)
            uDist = GenerateDis_bernoulli_distribution;
        m_uDist = uDist;
    }

    // 推进疲劳时钟（模拟流逝的时间）。曲线预览用：模拟真实点击节奏
    // （每次 rand 前推进一个点击间隔），让 FATIGUE/BIONIC 模式的"随时间失误率上升"在预览中可见。
    // 实际连点中无需调用——疲劳按真实时间自然累积。
    void AdvanceFatigueTime(const chrono::steady_clock::duration& d)
    {
        extra_elapsed += d;
    }

    // 核心函数：返回 true 表示“点击”，false 表示“失误跳过”
    bool rand()
    {
        double final_rate = error_rate;   // 最终失误率

        // 根据模式计算 final_rate
        switch (current_mode)
        {
            case RandomBoolMode_UNIFORM:
                // 保持基础失误率不变
                break;
            case RandomBoolMode_FATIGUE:
                final_rate *= CalcFatigueFactor();
                break;
            case RandomBoolMode_BIONIC:
            {
                double rate = error_rate * CalcFatigueFactor();
                rate *= CalcDistanceFactor();

                double markov_factor = 1.0;
                if (recent_failures >= 2)
                    markov_factor *= 0.35;   // 连续失误后纠错，降低失误率
                if (consecutive_successes >= 25)
                    markov_factor *= 1.7;    // 长时间无失误，大意，提高失误率
                rate *= markov_factor;
                final_rate = rate;
                break;
            }
        }

        // 对 FATIGUE 和 BIONIC 模式限制极端值，其他模式已在 error_rate 限制内
        if (current_mode == RandomBoolMode_FATIGUE || current_mode == RandomBoolMode_BIONIC)
            final_rate = clamp(final_rate, 0.01, 0.40);

        // 调用你封装的 RandomBool 生成失误标志（true=失误）
        bool is_fail = RandomBool(static_cast<float>(final_rate), m_uGen, m_uDist);

        // 更新马尔可夫状态（仅 BIONIC 模式）
        if (current_mode == RandomBoolMode_BIONIC)
        {
            if (is_fail)
            {
                ++recent_failures;
                consecutive_successes = 0;
            }
            else
            {
                recent_failures = 0;
                ++consecutive_successes;
            }
        }

        // 返回点击状态（true=点击，false=失误跳过）
        return !is_fail;
    }

    // 手动重置疲劳（例如用户暂停时调用）
    void ResetFatigue()
    {
        ResetInternalState();
    }
};
