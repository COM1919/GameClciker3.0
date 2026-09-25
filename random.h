#pragma once
#include <random>
#include <cstdint>
#include <variant>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <cmath>

class RandomGenerator
{
public:
    RandomGenerator(UINT uGen = GenerateGen_mersenne_twister, UINT uDist = GenerateDis_uniform_int_distribution)
    {
        InitEngine(uGen);
        InitDist(uDist);
    }

    int Random(int min, int max)
    {
        if (min > max)
            return -1;
        if (min == max)
            return min;

        // Range draw: force uniform_int, do NOT use the configured dist
        return std::visit(
            [&](auto& eng) -> int
            {
                std::uniform_int_distribution<int> uni(min, max);
                return uni(eng);
            },
            engine
        );
    }

    bool RandomBool(const float& probability)
    {
        if (probability <= 0.0f)
            return false;
        if (probability >= 1.0f)
            return true;

        return std::visit(
            [&](auto& dist, auto& engine) -> bool
            {
                using Dist = std::decay_t<decltype(dist)>;

                if constexpr (std::is_same_v<Dist, std::bernoulli_distribution>)
                {
                    dist.param(typename Dist::param_type(probability));
                    return dist(engine);
                }
                else if constexpr (std::is_same_v<Dist, std::uniform_int_distribution<>>)
                {
                    dist.param(typename Dist::param_type(0, 100));
                    return dist(engine) <= (probability * 100);
                }
                else if constexpr (std::is_same_v<Dist, std::binomial_distribution<>>)
                {
                    // binomial(1, p) == bernoulli(p): miss when the single trial succeeds
                    dist.param(typename Dist::param_type(1, probability));
                    return dist(engine) == 1;
                }
                else
                {
                    return false;
                }
            },
            dist, engine
        );
    }

    // 对应原来的 InitRandom()：重新播种当前引擎
    void InitRandom()
    {
        std::random_device rd;
        std::visit([&](auto& engine) { engine.seed(rd()); }, engine);
    }

private:
    using EngineVariant = std::variant<
        std::mt19937,
        std::linear_congruential_engine<uint_fast32_t, 16807, 0, 2147483647>,
        std::ranlux24_base,
        std::minstd_rand,
        std::discard_block_engine<std::minstd_rand, 256, 256>,
        std::independent_bits_engine<std::minstd_rand, 32, unsigned int>,
        std::shuffle_order_engine<std::minstd_rand, 256>,
        std::subtract_with_carry_engine<uint32_t, 32, 10, 24>
    >;

    using DistVariant = std::variant<
        std::uniform_int_distribution<>,
        std::binomial_distribution<>,
        std::bernoulli_distribution
    >;

    EngineVariant engine;
    DistVariant   dist;

private:
    static std::random_device& GetRandomDevice()
    {
        // 每个线程一个 random_device，避免全局随机源竞争
        static thread_local std::random_device rd;
        return rd;
    }

    void InitEngine(UINT uGen)
    {
        if (uGen > 7)
            uGen = GenerateGen_mersenne_twister;

        switch (uGen)
        {
        case GenerateGen_mersenne_twister:
            engine.emplace<0>(GetRandomDevice()());
            break;

        case GenerateGen_linear_congruential_engine:
            engine.emplace<1>(GetRandomDevice()());
            break;

        case GenerateGen_ranlux24_base:
            engine.emplace<2>(GetRandomDevice()());
            break;

        case GenerateGen_minstd_rand:
            engine.emplace<3>(GetRandomDevice()());
            break;

        case GenerateGen_discard_block_engine:
            engine.emplace<4>(GetRandomDevice()());
            break;

        case GenerateGen_independent_bits_engine:
            engine.emplace<5>(GetRandomDevice()());
            break;

        case GenerateGen_shuffle_order_engine:
            engine.emplace<6>(GetRandomDevice()());
            break;

        case GenerateGen_subtract_with_carry_engine:
            engine.emplace<7>(GetRandomDevice()());
            break;

        default:
            engine.emplace<0>(GetRandomDevice()());
            break;
        }
    }

    void InitDist(UINT uDist)
    {
        if (uDist > 2)
            uDist = GenerateDis_uniform_int_distribution;

        switch (uDist)
        {
        case GenerateDis_uniform_int_distribution:
            dist.emplace<0>();
            break;

        case GenerateDis_binomial_distribution:
            dist.emplace<1>();
            break;

        case GenerateDis_bernoulli_distribution:
            dist.emplace<2>();
            break;

        default:
            dist.emplace<0>();
            break;
        }
    }
};

class RandomBoolManager
{
private:
    RandomGenerator rng;
    UINT m_uGen = GenerateGen_mersenne_twister;
    UINT m_uDist = GenerateDis_bernoulli_distribution;
    RandomBoolMode current_mode = RandomBoolMode_BIONIC;
    double error_rate = 0.08;                     // 基础失误率

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

    // 疲劳因子（1.0 ~ 1.30）
    double CalcFatigueFactor() const
    {
        auto elapsed = (chrono::steady_clock::now() - start_time) + extra_elapsed;
        double minutes = chrono::duration<double>(elapsed).count() / 60.0;
        double factor = 1.0 + 0.25 * log10(minutes + 1.0);
        return clamp(factor, 1.0, 1.30);
    }

    // 距离因子（0.70 ~ 1.30），模拟手部移动导致的失误率变化
    double CalcDistanceFactor()
    {
        // 使用你封装的 Random 生成 -15 ~ 15 的偏移（整数均匀分布）
        int deltaX = rng.Random(-15, 15);
        int deltaY = rng.Random(-15, 15);
        virtual_x += deltaX;
        virtual_y += deltaY;
        virtual_x = clamp(virtual_x, -300, 300);
        virtual_y = clamp(virtual_y, -300, 300);
        double distance = sqrt((double)virtual_x * virtual_x + (double)virtual_y * virtual_y);
        double factor = 1.0 + (distance - 176.0) / 150.0;
        return clamp(factor, 0.70, 1.30);
    }

public:
    // 构造函数：不再需要种子，全局引擎已经初始化
    RandomBoolManager() : start_time(chrono::steady_clock::now()) {}

    void SetEngine(UINT uGen)
    {
        if (uGen > 7)
            uGen = GenerateGen_mersenne_twister;
        m_uGen = uGen;
        rng = RandomGenerator(m_uGen, m_uDist);
    }

    void SetDistribution(UINT uDist)
    {
        if (uDist > 2)
            uDist = GenerateDis_uniform_int_distribution;
        m_uDist = uDist;
        rng = RandomGenerator(m_uGen, m_uDist);
    }

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
                    markov_factor *= 0.50;   // 连续失误后纠错，降低失误率
                if (consecutive_successes >= 25)
                    markov_factor *= 1.35;    // 长时间无失误，大意，提高失误率
                rate *= markov_factor;
                rate *= 1.0 / 1.10;
                final_rate = rate;
                break;
            }
        }

        // 对 FATIGUE 和 BIONIC 模式限制极端值，其他模式已在 error_rate 限制内
        if (current_mode == RandomBoolMode_FATIGUE || current_mode == RandomBoolMode_BIONIC)
            final_rate = clamp(final_rate, 0.01, 0.40);

        // 调用你封装的 RandomBool 生成失误标志（true=失误）
        bool is_fail = rng.RandomBool(static_cast<float>(final_rate));

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
RandomGenerator RandGentor;//全局随机数生成器，避免部分代码需要使用随机数时，因为创建随机数生成器导致性能问题
