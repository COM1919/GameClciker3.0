#pragma once
#include <random>
#include "Definitons.h"

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

static char random_char()
{
    int r = Random(0, 61);
    if (r < 10)
        return '0' + r;           // 数字: 0-9
    else if (r < 36)
        return 'A' + (r - 10);    // 大写字母: A-Z
    else
        return 'a' + (r - 36);    // 小写字母: a-z
}