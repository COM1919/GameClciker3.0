#pragma once
#include <random>
#include <unordered_map>
#include "random.h"
#include "Definitons.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif // PI
#include <unordered_set>

inline float DegreeToRadian(float degree);
int FindFirstMissingNumber(const vector<int>& numbers, int max);
float CalculateTwoPointAngle(const ImVec2& A, const ImVec2& B);
ImVec2 CalculateRotatePos2D(float centerx, float centery, float length, float angle_degrees);
ImVec2 CalculateRotatePos2D(ImVec2 center, float length, float angle_degrees);
void RotatePoint(float& x, float& y, float angleDegrees, float centerX, float centerY);
double CalculateDistance(double x1, double y1, double x2, double y2);
void ReplaceString(string& str, const string& to_find, const string& to_replace);
bool IsNumber(const string& str);
ULONGLONG GetNowMs();
void BeginSleep(const char* flag);
POINT Bezier(vector<POINT>& points, float t);
void EndSleep(const char* flag, int time);
float EvaluateWave(const WaveParam& w, float t);
template<typename T> inline T Lerp(T a, T b, float t);
string RemoveBeforString(const string& str, const string& before);
void SplitString(const string& str, const string& sep, vector<string>& res);
string RemoveEnter(const string& input);
string ReplaceSubString(string str, const string& from, const string& to);
string RemoveSubString(const string& original, const string& toRemove);
double SmoothValues(int maxNum, int tick, int maxTicks);
double FileTimeToDouble(const FILETIME* ft);
string ExtractString(const string& str, const string& start, const string& end);

// 计算单个波形在时间 t 处的值（振幅影响）
float EvaluateWave(const WaveParam& w, float t)
{
    return w.amplitude * sinf(2.0f * PI * w.frequency * t + w.phase);
}

// 计算叠加后的总 CPS（基频 + 所有启用波形的和）
float EvaluateTotalCPS(float t, float baseCPS, vector<WaveParam> waves)
{
    float sum = baseCPS;
    for (const auto& w : waves)
        if (w.enabled) sum += EvaluateWave(w, t);
    return sum;
}

class SmoothCPS
{
private:
    ULONGLONG StartTime;     // 启动时的时间戳（毫秒）
    vector<WaveParam> waves;

public:
    float BaseCPS = 0;      // 基准 CPS
    float MinCPS = 0;       // 最低 CPS
    float MaxCPS = 0;       // 最高 CPS

    // 构造函数：基频、波动范围
    SmoothCPS()
    {
        StartTime = GetTickCount64();  // 自动记录启动时刻
    }
    SmoothCPS(float base, float minVal, float maxVal) : BaseCPS(base), MinCPS(minVal), MaxCPS(maxVal)
    {
        StartTime = GetTickCount64();
    }

    void SetWaveList(vector<WaveParam> waves)
    {
        this->waves = waves;
    }

    void AddWave(float amplitude, float frequency, float phase = 0.0)
    {
        waves.push_back({ amplitude, frequency, phase });
    }
    int GetWaveCount() const { return waves.size(); }

    // 核心函数：返回当前应使用的延迟（毫秒），自动实时计算
    float CalcCPS() const
    {
        // 计算已运行秒数
        float elapsed = (GetTickCount64() - StartTime) / 1000.0 / 10;  // /10用于减慢变化速度
        return EvaluateTotalCPS(elapsed, BaseCPS, waves);
    }
};

string ReplaceSubString(string str, const string& from, const string& to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != string::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return str;
}

// a开始左边，b结束坐标，c过冲长度，t插值
template<typename T>
inline T LerpOvershoot(T a, T b, T c, float t)
{
    // 确定移动方向
    if (a < b)
    {
        // 从左向右移动（坐标增加）
        T lerp_val = a + (b + 2 * c - a) * t; // 插值到超过目标的位置
        return -abs(lerp_val - b - c) + b + c;
    }
    else
    {
        // 从右向左移动（坐标减少）
        T lerp_val = a + (b - 2 * c - a) * t; // 插值到低于目标的位置
        return abs(lerp_val - b + c) + b - c;
    }
}

void ReplaceString(string& str, const string& to_find, const string& to_replace)
{
    size_t pos = str.find(to_find);
    while (pos != string::npos)
    {
        str.replace(pos, to_find.length(), to_replace);
        // 从刚才替换位置的下一个字符开始，继续查找下一个匹配项
        // 这是为了避免在替换后的新字符串上陷入死循环（例如将 "a" 替换为 "aa"）
        pos = str.find(to_find, pos + to_replace.length());
    }
}

static float BinomialCoefficient(int n, int k)
{
    int result = 1;
    if (k > n - k)
        k = n - k;
    for (int i = 0; i < k; ++i)
    {
        result *= (n - i);
        result /= (i + 1);
    }
    return result;
}

POINT Bezier(vector<POINT>& points, float t)
{
    POINT result{};
    int n = points.size() - 1;
    for (int i = 0; i <= n; i++)
    {
        float b = BinomialCoefficient(n, i) * pow(1 - t, n - i) * pow(t, i);
        result.x += points[i].x * b;
        result.y += points[i].y * b;
    }
    return result;
}

template<typename T> inline T Lerp(T a, T b, float t)
{
    return (T)(a + (b - a) * t);
}

// 在 0 到 max-1 的范围内，找到第一个不在 numbers 数组中出现的数字并返回   如果都出现了，则返回 0。
int FindFirstMissingNumber(const vector<int>& numbers, int max)
{
    unordered_set<int> NumSet(numbers.begin(), numbers.end()); // 转换为哈希集合
    for (int i = 0; i < max; i++)
    {
        if (NumSet.find(i) == NumSet.end()) // 如果 i 不在集合中
            return i;
    }
    return 0;
}

string RemoveBeforString(const string& str, const string& before)
{
    size_t pos = str.find(before);
    if (pos == string::npos) return str;
    return str.substr(0, pos);
}

string RemoveEnter(const string& input)
{
    string output = input;
    output.erase(remove_if(output.begin(), output.end(), [](unsigned char c)
        {
            return c == '\n';
        }
    ), output.end());
    return output;
}

void SplitString(const string& str, const string& sep, vector<string>& res)
{
    string::size_type start = 0;
    string::size_type end = str.find(sep);

    while (end != string::npos)
    {
        // 将 [start, end) 区间的子串加入结果（不包含分隔符）
        res.push_back(str.substr(start, end - start));
        // 移动起始位置到当前分隔符之后
        start = end + sep.size();
        // 从新起点开始查找下一个分隔符
        end = str.find(sep, start);
    }

    // 处理最后一段（如果没有以分隔符结尾）
    if (start < str.length())
        res.push_back(str.substr(start));
}

string RemoveSubString(const string& original, const string& toRemove)
{
    string result = original;
    size_t pos = result.find(toRemove);
    while (pos != string::npos)
    {
        result.erase(pos, toRemove.length());
        pos = result.find(toRemove);
    }
    return result;
}

float CalculateTwoPointAngle(const ImVec2& A, const ImVec2& B)
{
    float dx = B.x - A.x;
    float dy = B.y - A.y;
    float radians = atan2f(dy, dx);
    float degrees = radians * 180.0f / PI;

    if (degrees > 180)
        degrees -= 360;
    else if (degrees < -180)
        degrees += 360;

    return degrees;
}

ImVec2 CalculateRotatePos2D(float centerx, float centery, float length, float angle_degrees)
{
    return CalculateRotatePos2D(ImVec2(centerx, centery), length, angle_degrees);
}

ImVec2 CalculateRotatePos2D(ImVec2 center, float length, float angle_degrees)
{
    // 将角度转换为弧度
    float angle_radians = DegreeToRadian(angle_degrees);

    // 计算终点坐标
    float half_length = length / 2.0f;
    float end_x = center.x + (half_length * cos(angle_radians));
    float end_y = center.y + (half_length * sin(angle_radians));

    return ImVec2(end_x, end_y);
}

inline float DegreeToRadian(float degree)
{
    return degree * (float)(PI / 180.0f);
}

void RotatePoint(float& x, float& y, float angleDegrees, float centerX, float centerY)
{
    // 将点平移到以旋转中心为原点的坐标系
    x -= centerX;
    y -= centerY;

    // 将角度转换为弧度
    float angleRadians = angleDegrees * (3.14159265358979323846f / 180.0f);
    float cosAngle = cosf(angleRadians);
    float sinAngle = sinf(angleRadians);

    // 应用旋转矩阵
    float newX = x * cosAngle - y * sinAngle;
    float newY = x * sinAngle + y * cosAngle;

    // 将点平移回原来的坐标系
    x = newX + centerX;
    y = newY + centerY;
}

double CalculateDistance(double x1, double y1, double x2, double y2)
{
    double deltaX = x2 - x1;
    double deltaY = y2 - y1;
    return sqrt(deltaX * deltaX + deltaY * deltaY);
}

bool IsNumber(const string& str)
{
    if (str.empty())
        return false;
    else
    {
        for (char c : str)
        {
            if (!isdigit(c))
            {
                return false;
            }
        }
    }
    return true;
}

double SmoothValues(int maxNum, int tick, int maxTicks)
{
    if (tick > maxTicks) return maxNum;
    // 计算位移
    double displacement = maxNum / 2.0;
    // 使用正弦函数计算当前tick的振幅
    double sinusoid = sin((double)tick / maxTicks * 1.5707963);
    // 限制sinusoid的范围
    double value = (sinusoid < 1) ? sinusoid : 0;
    // 使用正弦函数来计算当前tick的值
    value = displacement * sinusoid;

    return value * 2;
}

double FileTimeToDouble(const FILETIME* ft)
{
    ULARGE_INTEGER uli{};
    uli.LowPart = ft->dwLowDateTime;
    uli.HighPart = ft->dwHighDateTime;
    return static_cast<double>(uli.QuadPart);
}

string ExtractString(const string& str, const string& start, const string& end)
{
    size_t startPos = str.find(start);
    // 如果开始字符串不存在，则返回空字符串
    if (startPos == string::npos) return "";
    // 跳过开始字符串
    startPos += start.length();

    size_t endPos = str.find(end, startPos);
    // 如果结束字符串不存在，则返回空字符串
    if (endPos == string::npos) return "";
    // 提取并返回两个字符串之间的内容
    return str.substr(startPos, endPos - startPos);
}
