#pragma once
#include "framework.h"
#include "AgreementString.h"

#define PI 3.14159265358979323846f

//默认数值
#define DEFAULT_CPS     12    //默认的CPS
#define DEFAULT_MCV     2     //默认的点击倍数
#define DEFAULT_RCTIME  1000  //默认的点击时长
#define DEFAULT_RCSLEEP 500   //默认的停止点击时长
#define DEFAULT_LCTIME  1000  //默认的点击时长
#define DEFAULT_LCSLEEP 200   //默认的停止点击时长
#define RESEND_SLEEP    2     //重新尝试发送的延迟
#define FILTER_SC_SLEEP 130   //过滤单次点击的延迟

//最大值和最小值
#define MAX_VALUE        4096 //最高的数值
#define MAX_BUF          2048 //最高的缓存大小
#define MAX_QUICK_SWITCH 6    //快速切换CPS的CPS值
#define MAX_RECORDS      1800 //最大记录数
#define MAX_PERF_RECORDS 600  //最大性能记录数
#define MAX_CPS          40   //最大CPS
#define MIN_CPS          1    //最小CPS
#define MAX_TRYVALUE     3    //最大尝试次数

//托盘图标消息
#define WM_TRAYICON (WM_USER + 1145)

#define URL_LZY_VER        "https://gameclicker.lanzouj.com/b00jdu3i5a"
#define URL_RESOLVE        "https://lz.qaiu.top/parser?url="
#define URL_LZY_NOTICE     "https://gameclicker.lanzouj.com/b00jdp078j"
#define URL_LZY_SPONS      "https://gameclicker.lanzouj.com/b00jdp071c"
#define URL_LZY_ADINFO1    "https://gameclicker.lanzouj.com/b00jdr5moh"
#define URL_LZY_ADINFO2    "https://gameclicker.lanzouj.com/b00jdr5mqj"
#define URL_LZY_ADINFO3    "https://gameclicker.lanzouj.com/b00jdr5mra"
#define URL_LZY_ADINFO4    "https://gameclicker.lanzouj.com/b00jdr5msb"
#define URL_LZY_SERTYPE    "https://gameclicker.lanzouu.com/b00jf38mnc"
#define URL_LZY_POPUP      "https://gameclicker.lanzouu.com/b00je6xzze"
#define URL_MCSERAPI       "https://api.mcstatus.io/v2/status"
#define URL_LZY_CONNECT    "https://gameclicker.lanzouu.com/b00jeqezoj"
#define URL_GITEE_INFO     "https://gitee.com/com1919/gameclicker-information/raw/master/info.txt"
#define URL_GITEE_UPDATA   "https://gitee.com/com1919/gameclicker-information/raw/master/updata.txt"
#define URL_GITEE_CONNECT  "https://gitee.com/com1919/gameclicker-information/raw/master/st.txt"
#define URL_GITEE_NOTICE   "https://gitee.com/com1919/gameclicker-information/raw/master/notice.txt"
#define URL_GITEE_POPUP    "https://gitee.com/com1919/gameclicker-information/raw/master/popup.txt"
#define URL_GITEE_SCRIPTS  "https://gitee.com/com1919/gameclicker-information/raw/master/scriptlist.txt"
#define URL_GITEE_SC_ROOT  "https://gitee.com/com1919/gameclicker-information/raw/master/script/"
#define URL_GITEE_SOUNDS   "https://gitee.com/com1919/gameclicker-information/releases/download/KeySound/keys.zip"

#define MAIN_R_KEY    5       //默认鼠标侧键，连点右键的快捷键
#define MAIN_L_KEY    6       //默认鼠标侧键，连点左键的快捷键
#define C_BUTTONMODE  0b0001  //按下快捷键连点，松开停止
#define C_SWITCHMODE  0b0010  //按下快捷键连点，再按下停止
#define C_ASSISTMODE  0b0100  //按下鼠标左键或者右键，自动点击一次或者二次，就是双倍点击
#define C_MOUSESMODE  0b1000  //按住鼠标键进行连点

#define SC_MOUSE_EVENT  0     //使用mouse_event函数或者keybd_event函数
#define SC_SEND_INPUT   1     //使用SendInput函数
#define SC_POST_MESSAGE 2     //使用PostMessage函数

#define GK_HOOK 1             //使用全局系统钩子获取按键状态
#define GK_API  2             //使用系统API监听按键状态
#define GK_BOTH 3             //都使用

#define MS_NC         10      //平点
#define MS_JC         20      //抖动点击
#define MS_OLD        40      //旧版点击

#define FRM_GL        1       //逐渐增大
#define FRM_GS        2       //逐渐减小
#define FRM_SL        3       //小到大
#define FRM_LS        4       //大到小
#define FRM_SLS       5       //小到大，大到小
#define FRM_LSL       6       //大到小，小到大

#define CP_NONE       0       //无优先级
#define CP_RC         1       //右键连点优先
#define CP_LC         2       //左键连点优先

typedef ID3D11ShaderResourceView* Texture2D;

typedef struct IntRange
{
    int v_min;
    int v_max;

    IntRange(int _min, int _max) : v_min(_min), v_max(_max) {}

    bool in_range(int value)  const { return value >= v_min && value <= v_max; }
    float get_average()       const { return (v_min + v_max) / 2.0f; }
};

typedef struct FloatRange
{
    float v_min;
    float v_max;

    FloatRange(float _min, float _max) : v_min(_min), v_max(_max) {}

    bool in_range(float value)  const { return value >= v_min && value <= v_max; }
    float get_average()         const { return (v_min + v_max) / 2.0f; }
};

template<typename T>
class LRPair
{
public:
    T left;
    T right;
    LRPair() : left(T()), right(T()) {}
    LRPair(T _left, T _right) : left(_left), right(_right) {}
};

//随机数生成器定义
enum GenerateDis
{
    GenerateDis_bernoulli_distribution = 0,     //伯努利分布
    GenerateDis_uniform_int_distribution,       //均匀分布
    GenerateDis_binomial_distribution,          //二项分布
};

enum GenerateGen
{
    GenerateGen_mersenne_twister = 0,            //梅森旋转算法
    GenerateGen_ranlux24_base,                   //RANLUX24算法
    GenerateGen_minstd_rand,                     //最小标准随机数算法
    GenerateGen_discard_block_engine,            //丢弃块算法
    GenerateGen_independent_bits_engine,         //独立位算法
    GenerateGen_shuffle_order_engine,            //随机排序算法
    GenerateGen_linear_congruential_engine,      //线性同余算法
    GenerateGen_subtract_with_carry_engine,      //减去余数算法
};

// CPS波动
struct WaveParam
{
    float amplitude;   // 振幅（CPS波动幅度）
    float frequency;   // 频率（Hz）
    float phase;       // 相位（弧度）
    bool enabled;      // 是否启用（可暂时关闭）
};

//--------------------基本设置------------------------------

class Config
{
public:
    int LKEY = 6;                           //左键快捷键
    int RKEY = 5;                           //右键快捷键
    int LCPS = DEFAULT_CPS;                 //左键CPS
    int RCPS = DEFAULT_CPS;                 //右键CPS
    float RFluctuationCPS = 1;              //右键CPS波动
    float LFluctuationCPS = 1;              //左键CPS波动
    int MainClickSleep = 4;                 //连点主循环每次循环的延迟（毫秒）
    int RMultipleClickValue = DEFAULT_MCV;  //辅助模式点击数
    int LMultipleClickValue = DEFAULT_MCV;  //辅助模式点击数
    int RQuickSwitch[MAX_QUICK_SWITCH]{};   //右键快速切换CPS
    int LQuickSwitch[MAX_QUICK_SWITCH]{};   //左键快速切换CPS
    int QuickSwitchKey[MAX_QUICK_SWITCH]{}; //快速切换CPS快捷键
    int QuickSwitchTotal = 0;               //快速切换CPS的大小
    int REmptyRateValue = 20;               //右键空刀概率(%)
    int LEmptyRateValue = 20;               //左键空刀概率(%)
    int RCTime = DEFAULT_RCTIME;            //右键点击时长(毫秒)
    int RCSleep = DEFAULT_RCSLEEP;          //右键停止点击时长(毫秒)
    int LCTime = DEFAULT_LCTIME;            //左键点击时长(毫秒)
    int LCSleep = DEFAULT_LCSLEEP;          //左键停止点击时长(毫秒)
    int RClickerMode = C_BUTTONMODE;        //右键连点模式
    int LClickerMode = C_BUTTONMODE;        //左键连点模式
    int RPressDuration = 20;                //右键鼠标连点按下时长(毫秒)
    int LPressDuration = 20;                //左键鼠标连点按下时长(毫秒)
    int EnableRightClickerKey;              //开关右键连点快捷键
    int EnableLeftClickerKey;               //开关左键连点快捷键
    int ChatBarKey = 'T';                   //聊天栏快捷键
    int BagBarKey = 'E';                    //背包栏快捷键
    int MouseShakeSpeed = 300;              //鼠标抖动速度
    int MouseShakeRect = 8;                 //鼠标抖动范围
    array<int, 10>                          //物品栏开关连点快捷键
        ItemEnableClickerKey = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
    LRPair<array<int, 10>>                  //物品栏CPS预设
        ItemBarPresetCPS = {
            { DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS },
            { DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS }
        };
    LRPair<array<bool, 10>>
        ItemBarEnablePresetCPS = {          //物品栏预设CPS
            { false, false, false, false, false, false, false, false, false },
            { false, false, false, false, false, false, false, false, false }
        };
    int MouseShake_RandomDist =             //鼠标抖动随机数生成分布
    GenerateDis_uniform_int_distribution;
    int MouseShake_RandomGen =              //鼠标抖动随机数生成器
    GenerateGen_mersenne_twister;
    int EmptyRate_RandomDist =              //空刀概率随机数生成分布
    GenerateDis_bernoulli_distribution;
    int EmptyRate_RandomGen =               //空刀概率随机数生成器
    GenerateGen_mersenne_twister;
    int Fluctuation_RandomDist =            //CPS波动随机数生成分布
    GenerateDis_uniform_int_distribution;
    int Fluctuation_RandomGen =             //CPS波动随机数生成器
    GenerateGen_mersenne_twister;
    int MemSearchSleep = 20;                //内存搜索延迟(毫秒)
    int MouseSoundSleep = 1;                //鼠标点击声音的循环延迟
    int AssistModeCPSThreshold = 4;         //辅助模式的CPS门限
    int SelfDestructionKey = 0;             //自毁快捷键
    int UseLFluctuationRandMode = FRM_SL;   //左键CPS波动随机模式
    int UseRFluctuationRandMode = FRM_SL;   //右键CPS波动随机模式
    int RStartClickerDelay = 1000;          //右键启动延迟(毫秒)
    int LStartClickerDelay = 1000;          //左键启动延迟(毫秒)
    int SetGameWindowKey = 0;               //设置游戏窗口快捷键
    int HideClickerKey = 0;                 //隐藏连点器快捷键
    vector<int> RCPSChangeTrendList;        //右键CPS变化趋势列表
    vector<int> LCPSChangeTrendList;        //左键CPS变化趋势列表
    float OverheatTime = 5;                 //过热时长
    float OHPTimerTimeLossSpeed = 0.1f;     //过热时长减少速度
    int SimulateClickMode = SC_MOUSE_EVENT; //模拟点击模式
    int ReadKeyStateMode = GK_BOTH;         //获取按键状态模式
    int ClickerPriorityMode = CP_NONE;      //左右键连点的优先级模式
   
    LRPair<vector<WaveParam>> CPSWaves;

    //--------------------鼠标声音设置------------------------------

    bool UseMouseSound = true;              //开关鼠标点击声音
    bool UseKeybdSound = true;              //开关键盘点击声音
    int MouseSoundVolume = 100;             //鼠标点击声音音量
    int KeybdSoundVolume = 100;             //键盘点击声音音量
    int NoiseVolume = 90;                   //噪音音量
    string MouseSoundType =                 //鼠标声音的类型
        "Corsair M65 Elite";
    string KeybdSoundType =                 //键盘声音的类型
        "CherryMX Blue - PBT keycaps";

    //--------------------功能开关------------------------------

    bool UseREmptyRate = false;             //开关右键空刀概率
    bool UseLEmptyRate = false;             //开关左键空刀概率
    bool UseRFluctuation = false;           //开关右键CPS波动
    bool UseLFluctuation = false;           //开关左键CPS波动
    bool UseLClickTime = false;             //开关左键连点间隔
    bool UseRClickTime = false;             //开关右键连点间隔
    bool UseGUIDetection = false;           //开关GUI检测
    bool UseLClickPressTime = true;         //开关左键仿人类点击
    bool UseRClickPressTime = true;         //开关右键仿人类点击
    bool UseRAutoPressDuration = true;      //开关自动设置右键按下时长
    bool UseLAutoPressDuration = true;      //开关自动设置左键按下时长
    bool UseStartWithWindows = false;       //开关开机启动
    bool EnableRightClicker = true;         //开关右键连点
    bool EnableLeftClicker = true;          //开关左键连点
    bool UseBroadcasterMode = false;        //开关防止捕获GC窗口
    bool UseAutoGuiDetect = false;          //开关通过按键检测游戏ui
    bool UseROnlyMoveClicker = false;       //开关仅移动时右键连点
    bool UseLOnlyMoveClicker = false;       //开关仅移动时左键连点
    bool UseDisableMoveOnStopMove = false;  //开关连点停止移动时禁用连点
    bool UseMouseShake = false;             //开关鼠标抖动
    bool UseMouseShakeRawInput = true;      //开关是否鼠标移动时使用原始输入
    bool UseMSPreventViewSliding = true;    //开关防止视角滑动
    bool UseMSSmoothSpeed = true;           //开关平滑速度
    bool UseItembarUseMemSearch = true;     //开关物品栏开关使用内存搜索
    LRPair<array<bool, 10>> ItemEnableClicker = {//物品栏开关连点
        { true, true, true, true, true, true, true, true, true },
        { true, true, true, true, true, true, true, true, true }
    };
    bool UseChinese = true;                 //开关中文模式
    bool UseDriverProtect = false;          //开关驱动保护
    bool UseHiddenProcess = false;          //开关隐藏进程
    bool UseSuspendNetease = false;         //开关挂起网易游戏
    bool UseAutoResetOnDisable = true;      //开关鼠标模式时禁用连点是否重置鼠标按下状态(是否自动松开)
    bool AllowSetLClicker = true;           //开关允许控制左键开关
    bool AllowSetRClicker = true;           //开关允许控制右键开关
    bool UseConciseMode = false;            //开关精简模式
    bool UseMinimalistMode = false;         //开关极简模式
    bool UseEnableGameGui = false;          //开关开启游戏视觉效果
    bool UseROnlyJumpClicker = false;       //开关跳跃时启用右键连点
    bool UseLOnlyJumpClicker = false;       //开关跳跃时启用左键连点
    bool UseRestTimeOnStopClicker = true;   //开关停止连点时，重置连点间隔的时间
    bool UseBrutalMode = false;             //开关暴力模式
    bool UseBreakBlocksWithClicker = false; //开关连点时可以挖掘方块
    bool UseItemWithClicker = false;        //开关连点时可以使用物品
    bool UseRStartClickerDelay = false;     //开关右键启动延迟
    bool UseLStartClickerDelay = false;     //开关左键启动延迟
    bool UseFilterRSingleClick = false;     //开关过滤右键单次点击
    bool UseFilterLSingleClick = false;     //开关过滤左键单次点击
    bool UsePerformanceMode = false;        //开关性能模式
    bool UseRCPSChangeTrend = false;        //开关右键CPS变化趋势
    bool UseLCPSChangeTrend = false;        //开关左键CPS变化趋势
    bool UseOverheatProtec = false;         //开关过热保护
    bool UseOHPResetTimerOnStop = false;    //开关过热保护的停止时重置计时器
    bool UseOHPPlaySound = false;           //开关过热保护时播放提示音
    bool UseBEAdaptation = false;           //是否适配Bedrock Edition
    bool UseInGameOnlyClicker = false;      //仅限游戏中使用连点器
    bool UseItembarSwitch = false;          //物品栏开关
    bool UseHumanOnlyTrigger = false;       //仅限人类触发连点器
}config;

//--------------------个性化设置------------------------------
int BgOffsetX = 0;                      //背景偏移X
int BgOffsetY = 0;                      //背景偏移Y
int BgScale = 100;                      //背景缩放百分比
int BgLightness = 75;                   //背景亮度百分比
int GuiItemAlpha = 65;                  //控件的透明度百分比（仅有背景图片时才能使用）
bool BgAutoResize = false;              //自动调整背景大小

static const array<const char*, 8> RandomGenList = {
    (config.UseChinese ? u8"梅森旋转算法"       :  "Mersenne Twister"           ),
    (config.UseChinese ? u8"Ranlux24基础算法"   :  "Ranlux24 Base"              ),
    (config.UseChinese ? u8"最小标准随机数算法" :  "Minstd Rand"                ),
    (config.UseChinese ? u8"丢弃块算法"         :  "Discard Block Engine"       ),
    (config.UseChinese ? u8"独立位算法"         :  "Independent Bits Engine"    ),
    (config.UseChinese ? u8"洗牌顺序算法"       :  "Shuffle Order Engine"       ),
    (config.UseChinese ? u8"线性同余算法"       :  "Linear Congruential Engine" ),
    (config.UseChinese ? u8"带进位减法算法"     :  "Subtract With Carry Engine" )
};
static const array<const char*, 2> RandomDistList = {
    (config.UseChinese ? u8"均匀整数分布"       : "Uniform Int Distribution"    ),
    (config.UseChinese ? u8"二项分布"           : "Binomial Distribution"       )
};
static const array<const char*, 3> BoolRandomDistList = {
    (config.UseChinese ? u8"Bool类型分布"       : "Bernoulli Distribution"      ),
    (config.UseChinese ? u8"均匀整数分布"       : "Uniform Int Distribution"    ),
    (config.UseChinese ? u8"二项分布"           : "Binomial Distribution"       )
};

//--------------------其他------------------------------

string GameWindowTitle = config.UseChinese ? "无" : "None"; //游戏窗口标题

int FollowGameStartPID = 0;             //跟随游戏启动的进程ID
vector<float> RRecordsCPS;              //右键CPS记录
vector<float> LRecordsCPS;              //左键CPS记录
vector<float> CpuRates;                 //CPU占用率记录
vector<float> GpuRates;                 //GPU占用率记录
vector<float> MemRates;                 //内存占用率记录
vector<vector<float>> ScriptCpuRates;   //脚本CPU占用率记录
mutex RecordsCPSMutex;                  //以上变量的互斥锁
atomic<bool> ThreadExitFlag(false);     //线程退出标志

const vector<string> ScriptRestrictedFuncs = {
        "SetClipboardText", "GetClipboardText",
        "WriteNamedPipe", "CloseNamedPipe", "ReadNamedPipe", "ConnectNamedPipe",
        "SimulatedInputText",
        "SetMousePos", "GetMousePos"
};

struct ScriptInfo
{
    int ver = 1;
    string name;
    string info;
    string file;
    string texture;// 图片的文件路径
    string title;
    string link;
    HANDLE hThread{};
    vector<string> RestrictedFuncs;
    string RestrictedFuncsStr;
    bool IsInstalled = false;
    bool IsHasUpdate = false;
};

struct ScriptState
{
    int key = 0;
    bool IsPressedKey = false; // 这个不需要自己控制
    bool IsClickedKey = false; // 这个需要在读取后设置为false
    bool IsReleaseKey = false; // 这个需要在读取后设置为false
    int LoopDelay = 0;
    bool IsExit = false;
    bool IsSuspend = false;
};

shared_mutex InstalledScriptsMtx;       //已安装的脚本锁
shared_mutex AvaiableScriptsMtx;        //可用的脚本锁
shared_mutex ScriptStatesMtx;           //脚本状态的锁
vector<ScriptInfo> InstalledScripts;    //已安装的脚本
vector<ScriptInfo> AvaiableScripts;     //可用的脚本
vector<ScriptInfo> ScriptShopList;      //脚本商店的脚本列表（几乎没有持续的其他线程，没必要lock）
map<string, ScriptState> ScriptStates;  //脚本相关状态控制

struct AdStruct
{
    string text;
    string link;
    Texture2D texture;
};
shared_mutex AdListMutex;
vector<AdStruct> AdList;                //广告列表

shared_mutex SponsorListMutex;
vector<string> SponsorList;             //赞助列表

const string version = "3.44";          //版本号
string title = "GAMECLICKER 3.44";      //标题名字
const int ConfigVersion = 5;            //配置文件版本号
const int AgreementVersion = 1;         //协议文件版本号
bool IsNeedReshowAgreement = false;     //是否需要显示更新的协议，根据配置文件里面的版本号对比

const int RenderFps = 60;               //渲染帧率

//#define GC_USE_D_DRIVE                //是否使用D盘的配置文件，方便调试
//#define GC_NO_HOOK                      //是否禁用HOOK
//#define GC_DEBUG_VS                   //是否在VS的调试窗口输出日志
#define GC_DEBUG_CMD                    //是否在命令行输出日志
//#define GC_NO_ANIMATION               //是否禁用启动动画
//#define GC_REMOTE_DEBUG               //是否使用远程调试
//#define GC_MORE_DEBUG_INFO            //更多详细调试信息
//#define GC_NO_MSGBOX                  //禁用消息弹窗
//#define GC_SERVER_DEBUG               //是否开启服务器通讯调试
#define GC_ENABLE_QUICK_EXIT            //开启调试快速退出
//#define GC_DEBUG_GUIDE_UI             //开启总是显示向导UI

//#define GC_RELEASE_MODE               //是否为发布版（如果是就自动关闭所有调试内容）

#ifdef GC_RELEASE_MODE
#undef GC_DEBUG_VS
#undef GC_DEBUG_CMD
#undef GC_NO_ANIMATION
#undef GC_NO_SAFE_MODE
#undef GC_NO_TRY_CATCH
#undef GC_SERVER_DEBUG
#undef GC_MORE_DEBUG_INFO
#undef GC_NO_HOOK
#undef GC_USE_D_DRIVE
#endif // GC_RELEASE_MODE

//=============================待更改项==========================================================
// 把这些文件的封装到class FileDefines，变量名字为path
#ifdef GC_USE_D_DRIVE
const string WorkPath = "D:";
#else
const string WorkPath = []() -> string {
    char buf[MAX_PATH]{};
    if (GetModuleFileName(NULL, buf, MAX_PATH) == 0)
        return "";
    string str(buf);
    size_t pos = str.find_last_of("\\/");
    if (pos != string::npos)
        return str.substr(0, pos);
    else
        return "";
}();
#endif // GC_USE_D_DRIVE

const string FontFile =           "C:\\Windows\\Fonts\\msyhbd.ttc";
const string ConfigPath =         WorkPath + "\\GameClicker";
const string ConfigFile =         WorkPath + "\\GameClicker\\settings.ini";
const string CustomConfigFile =   WorkPath + "\\GameClicker\\custom.ini";
const string QQNumFile =          WorkPath + "\\GameClicker\\RunData";
const string DownloadName =       config.UseChinese ? "GC连点器" : "GC Auto Click";
const string DownloadPath =       WorkPath + "\\GameClicker\\GCDownloadFile";
const string ConfigDownloadFile = WorkPath + "\\GameClicker\\GCDownloadFile\\Version.txt";
const string ZipFileName =        WorkPath + "\\GameClicker\\GCDownloadFile\\gc.zip";
const string IniFileVeision =     WorkPath + "\\GameClicker\\Version.dat";
const string TempFile =           WorkPath + "\\GameClicker\\GCDownloadFile\\temp.dat";
const string GCFollowBatFile =    WorkPath + "\\GameClicker\\GC Follow.bat";
const string GameGuiFile =        "GameGui.exe";
const string StatsRegPath =       "Software\\GameClicker\\Stats";

const string SoundPath =          "sound";
const string Sound_Noise =        "sound\\noise.wav";
const string Sound_MousePath =    "sound\\mouse";
const string Sound_Enable =       "sound\\func\\enable.wav";
const string Sound_Disable =      "sound\\func\\disable.wav";
const string Sound_KeybdPath =    WorkPath + "\\GameClicker\\sound\\keys";

const string Tmp_TempPath =       WorkPath + "\\GameClicker\\Temp";
const string Tmp_Ad =             WorkPath + "\\GameClicker\\Temp\\AdImage.dat";
const string Tmp_MCServer =       WorkPath + "\\GameClicker\\Temp\\mcserver%d.json";
const string Tmp_KeySounds =      WorkPath + "\\GameClicker\\Temp\\KeySounds.zip";

const string PicsPath =           "res\\pics";
const string Pics_Itembar =       "res\\pics\\item.png";
const string Pics_Background =    WorkPath + "\\GameClicker\\bg.gcbg";
const string Pics_HotBar =        "res\\pics\\hotbar.png";
const string Pics_GCLogo =        "res\\pics\\Logo.png";
const string Pics_About =         "res\\pics\\ico\\About.png";
const string Pics_Setting =       "res\\pics\\ico\\Setting.png";
const string Pics_Home =          "res\\pics\\ico\\Home.png";
const string Pics_Script =        "res\\pics\\ico\\Script.png";
const string Pics_Info =          "res\\pics\\ico\\Info.png";
const string Pics_Delete =        "res\\pics\\ico\\Delete.png";
const string Pics_Add =           "res\\pics\\ico\\Add.png";
const string Pics_Other =         "res\\pics\\ico\\Other.png";
const string Pics_Refresh =       "res\\pics\\ico\\Refresh.png";
const string Pics_FunSetting =    "res\\pics\\ico\\FunSetting.png";
const string Pics_GCIcon =        "res\\pics\\ico\\GCIcon.png";
const string Pics_ClearCache =    "res\\pics\\ico\\ClearCache.png";
const string Pics_TipInfo =       "res\\pics\\ico\\TipInfo.png";
const string Pics_TipError =      "res\\pics\\ico\\TipError.png";
const string Pics_TipWarning =    "res\\pics\\ico\\TipWarning.png";
const string Pics_Empyt =         "res\\pics\\ico\\Empyt.png";
const string Pics_Book =          "res\\pics\\ico\\Book.png";
const string Pics_Download =      "res\\pics\\ico\\Download.png";
const string Pics_Run =           "res\\pics\\ico\\run.png";
const string Pics_Stop =          "res\\pics\\ico\\stop.png";
const string Pics_GCTitle =       "res\\pics\\GCTitle.png";

const string StartupMov =         "res\\startup.webp";

const string ScriptPath =         WorkPath + "\\GameClicker\\Script";
const string ScriptTestFile =     WorkPath + "\\GameClicker\\Script\\test.lua";
const string ScriptTemp =         WorkPath + "\\GameClicker\\Script\\temp";
const string ScriptIconTemp =     WorkPath + "\\GameClicker\\Script\\shop\\icons";
const string ScriptInstallList =  WorkPath + "\\GameClicker\\Script\\InstallList.txt";
const string ScriptShopDir =      WorkPath + "\\GameClicker\\Script\\shop";
const string ScriptShopTemp =     WorkPath + "\\GameClicker\\Script\\shop\\temp";

const string LogPath =            WorkPath + "\\GameClicker\\Logs";
const string LogFile =            WorkPath + "\\GameClicker\\GC.log";
const string ShaderLogFile =      WorkPath + "\\GameClicker\\Shader.log";
const string LogFileZip =         "GC_LOG-%Y-%m-%d %H-%M-%S.zip";
const string GameGuiLogFile =     WorkPath + "\\GameClicker\\GameGui.log";

const string ExitFlagFile =       WorkPath + "\\GameClicker\\ExitFlag.gcf";

//用户协议 
const string UserAgreement = config.UseChinese ? AgreementCNString : AgreementENString;

string NoticeStr = config.UseChinese ? 
u8"\
GC连点器3.0！全新的UI体验，更好的设置内容。\n\
并且GC连点器官网也上线了，欢迎访问：https://bbs.mcgc.top\n\
本软件 完 全 免 费 ！如果你是通过购买获得的GC连点器，请及时退款并举报。"
:
"The official version 3.0\n\
A brand new UI experience with better content settings. \n\
And the GC connector official website has also been launched. \n\
Welcome to visit: https://bbs.mcgc.top \n\
This software is completely free! \n\
If you obtained the GC connector through purchase, \n\
please refund and report it promptly. \n\
GC developer: COM1919 and supports English now! \n\
using machine translation. If there are any errors, \n\
please feel free to provide feedback.";

array<string, 45> DailyMottos = {
    config.UseChinese ? u8"&b数字世界是风景，现实生活才是家园。" : "&bDigital world is scenery, real life is home.",
    config.UseChinese ? u8"&b每一次专注学习，都是对未来的投资。" : "&bEach study session invests in your future.",
    config.UseChinese ? u8"&b真正的挑战在屏幕外，别逃避成长。" : "&bReal challenges are off-screen - don't avoid growth.",
    config.UseChinese ? u8"&b今天少玩一小时，明天多一分选择权。" : "&bOne less gaming hour today, more choices tomorrow.",
    config.UseChinese ? u8"&b技能需要训练，大脑需要知识。" : "&bSkills need practice, mind needs knowledge.",
    config.UseChinese ? u8"&b健康的三餐比游戏排名更重要。" : "&bThree healthy meals beat gaming rankings.",
    config.UseChinese ? u8"&b真正的成就不会在虚拟世界中解锁。" : "&bReal achievements aren't unlocked in virtual worlds.",
    config.UseChinese ? u8"&b与真人面对面交流，感受真实情感。" : "&bFace-to-face talks bring real emotions.",
    config.UseChinese ? u8"&b深夜的星空比游戏画面更壮丽。" : "&bNight stars outshine game graphics.",
    config.UseChinese ? u8"&b运动产生的内啡肽比游戏刺激更持久。" : "&bExercise endorphins last longer than gaming thrills.",
    config.UseChinese ? u8"&b学习编程，创造游戏比玩游戏酷多了。" : "&bLearn coding - creating games beats playing them.",
    config.UseChinese ? u8"&b帮助父母做家务，你会收获更多尊重。" : "&bHelp with chores, earn more respect.",
    config.UseChinese ? u8"&b培养一项艺术爱好，发现更多美好。" : "&bDevelop an art hobby, discover more beauty.",
    config.UseChinese ? u8"&b游戏角色会重置，人生没有重新开始。" : "&bGame characters reset, life doesn't restart.",
    config.UseChinese ? u8"&b真正的团队合作在现实项目中磨练。" : "&bReal teamwork sharpens in actual projects.",
    config.UseChinese ? u8"&b放下手柄，拿起乐器，创造属于你的旋律。" : "&bDrop controller, pick instrument, create your melody.",
    config.UseChinese ? u8"&b每天读10页书，一年就是3650页。" : "&bRead 10 pages daily, 3650 pages a year.",
    config.UseChinese ? u8"&b游戏任务会刷新，人生机会不等待。" : "&bGame quests refresh, life chances don't wait.",
    config.UseChinese ? u8"&b投资自己的技能，这是最好的装备。" : "&bInvest in skills - your best equipment.",
    config.UseChinese ? u8"&b真实的友谊需要真实的时间来培养。" : "&bReal friendships need real time to grow.",
    config.UseChinese ? u8"&b屏幕外的世界有更精彩的冒险。" : "&bWorld outside screen has greater adventures.",
    config.UseChinese ? u8"&b学会烹饪，为自己做一顿健康的餐。" : "&bLearn cooking, make yourself a healthy meal.",
    config.UseChinese ? u8"&b游戏排名只是数据，个人成长才是真实。" : "&bGame ranks are data, personal growth is real.",
    config.UseChinese ? u8"&b保持好奇心，探索未知领域。" : "&bStay curious, explore unknown fields.",
    config.UseChinese ? u8"&b帮助同学解决问题，你会获得更多快乐。" : "&bHelp classmates solve problems, gain more joy.",
    config.UseChinese ? u8"&b每天练习英语10分钟，打开新世界大门。" : "&bPractice English 10 mins daily, open new doors.",
    config.UseChinese ? u8"&b游戏是设计的快乐，生活是创造的幸福。" : "&bGames are designed joy, life is created happiness.",
    config.UseChinese ? u8"&b写日记记录成长，比游戏成就更有意义。" : "&bKeep diary for growth, more meaningful than achievements.",
    config.UseChinese ? u8"&b关注科技新闻，了解真实世界的创新。" : "&bFollow tech news, learn real-world innovation.",
    config.UseChinese ? u8"&b游戏是短暂的娱乐，知识是永恒的财富。" : "&bGames are brief fun, knowledge is eternal wealth.",
    config.UseChinese ? u8"&b虚拟世界的英雄，不如现实中的勇士。" : "&bVirtual heroes can't match real warriors.",
    config.UseChinese ? u8"&b学习一门新语言，与更广阔的世界对话。" : "&bLearn new language, converse with wider world.",
    config.UseChinese ? u8"&b游戏里的任务有攻略，人生的问题靠思考。" : "&bGame quests have guides, life needs thinking.",
    config.UseChinese ? u8"&b参加志愿者活动，感受给予的快乐。" : "&bVolunteer to feel the joy of giving.",
    config.UseChinese ? u8"&b培养解决问题的能力，这是终极技能。" : "&bDevelop problem-solving - the ultimate skill.",
    config.UseChinese ? u8"&b游戏地图有限，世界地图无限。" : "&bGame maps are limited, world map is infinite.",
    config.UseChinese ? u8"&b与不同背景的人交流，拓展视野。" : "&bTalk to diverse people, broaden horizons.",
    config.UseChinese ? u8"&b学会理财知识，比游戏经济更有用。" : "&bLearn finance, more useful than game economy.",
    config.UseChinese ? u8"&b摄影记录生活，比截图更有温度。" : "&bPhotograph life, warmer than screenshots.",
    config.UseChinese ? u8"&b练习公共演讲，提升表达自信。" : "&bPractice public speaking, boost confidence.",
    config.UseChinese ? u8"&b种植一盆植物，见证生命的成长。" : "&bGrow a plant, witness life's growth.",
    config.UseChinese ? u8"&b游戏技能会过时，学习能力永远保值。" : "&bGame skills expire, learning ability lasts.",
    config.UseChinese ? u8"&b与长辈交谈，收获智慧与经验。" : "&bTalk to elders, gain wisdom and experience.",
    config.UseChinese ? u8"&b参与体育比赛，感受真实的竞技精神。" : "&bJoin sports, feel real competitive spirit.",
    config.UseChinese ? u8"&b学习心理学，更好地理解自己和他人。" : "&bStudy psychology, understand yourself and others."
};

HWND GCHWND;
HWND GameHwnd;
HWND TrayHwnd;
HDC GCHdc;
HANDLE GCDRAWHWND;
HANDLE GCMAINHWND;
HINSTANCE GCInstance;
NOTIFYICONDATA TrayNid = { 0 };
HANDLE AntiMultiOpenMutex;
HANDLE ClickRunThread;
HANDLE ClickSoundThread;

bool Starting                = false;           //是否正在启动
bool StopClicker             = false;           //是否停止连点
atomic<bool>GCExit           = false;           //连点器退出
bool IsDownloadRes           = false;           //是否正在下载资源
bool ShowGui                 = true;            //是否显示GUI
bool IsFirstRun              = false;           //第一次运行（是否显示向导）
bool SoundIsOpen             = false;           //音效是否正常加载
LRPair<bool> IsClickering    = {};              //是否连点
LRPair<bool> IsClickerDown   = {};              //连点器的键按下，CPS计数器专属变量，其他代码不能使用也不能访问
bool IsMoving                = false;           //玩家是否移动
bool IsJumping               = false;           //玩家是否跳跃
bool IsOpenGui               = false;           //在游戏中是否打开GUI
bool IsGameGuiInit           = false;           //GameGui是否初始化
bool GameGuiError            = false;           //GameGui是否出错
bool IsDownloading           = false;           //是否正在下载
bool ReloadClicker           = true;            //是否重启连点线程
bool IsClickerSuccess        = false;           //连点线程是否准备成功
bool AbnormalExit            = false;           //上一次程序是否非正常退出
bool EnterSafeMode           = false;           //是否进入安全模式
bool IsRestartAL             = false;           //是否重启OpenAL
bool IsSearchReload          = false;           //内存搜索是否重新加载
//bool IsMemSearch           = false;           //是否开始内存搜索
//bool IsMemSearchInit       = false;           //内存搜索是否未初始化
bool DebugInfo               = false;           //显示调试信息
bool IsCreateWindowSussce    = false;           //窗口是否创建成功
bool IsServerClosed          = false;           //服务器是否关闭
bool AllowUseGameGui         = false;           //是否允许使用视觉效果
bool AllowConnectServer      = false;           //是否允许连接服务器
bool IsDragingFile           = false;           //是否正在拖入文件
bool IsEnableScript          = true;            //是否启用LUA脚本系统，这个针对LUA系统崩溃后的处理
bool IsCPSWaveSettingChanged = false;           //是否更改了CPS波形设置
bool IsScriptUseGitee        = false;           //脚本商场是否使用gitee

float OverheatProtecTimer = 0;                  //过热保护计时器
int UseServerOfType = -1;                       //使用的服务器的类型
int ItemBarPos = 1;                             //物品栏位置
LRPair<int> CurrentCPS = {};                    //总实际CPS
LRPair<int> HumanCPS = {};                      //人类CPS
int ClickTabCount = 11;                         //连点器标签页的次数
int GameGuiTransparency = 40;                   //单位%
int IsSearchState = 0;                          //内存搜索状态
int MemSearchCount = 0;                         //内存搜索次数
int GettingGameState = -1;                      //获取游戏窗口的状态
float SystemScale = 1.0f;                       //DPI缩放率
string client_id;                               //客户端的ID
vector<string> MouseSoundList;                  //可用的鼠标声音列表
vector<string> KeybdSoundList;                  //可用的键盘声音列表

const int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
const int ScreenHeigth = GetSystemMetrics(SM_CYSCREEN);
int WindowWidth = 800;
int WindowHeight = 500;

struct AppStaus
{
    LRPair<ULONGLONG> ClickerClickCount ;       //连点次数
    LRPair<ULONGLONG> MouseClickCount;          //鼠标点击次数
    unsigned int StartCount = 0;                //启动次数
    long double UseTime = 0;                    //使用连点器的时长(小时)
} app_status;
shared_mutex InitSuccessTimeMutex;
ULONGLONG StartTime = 0;                 //GC启动时的时间
ULONGLONG InitSuccessTime = 0;           //初始化成功的时间

shared_mutex MBControlMutex;
shared_mutex MBChangeMutex;
struct MessageBoxMsg
{
    bool                 open;
    string               message;
    string               title;
    UINT                 type;
    map<string, UINT>    button;
    map<string, ImVec4>  colors;
    int                  ret_code;
}mb_message;

struct ProgressBarMsg
{
    string  title;
    int     ret_code = 0;
    double  maxmb = 0.0;
    double  mb = 0.0;
    bool    open = false;
    bool    unzip = false;
}pb_message;

shared_mutex PTChangeMutex;
struct PopTipMsg
{
    string  message;
    UINT    type;
    float   timeout;
};
vector<PopTipMsg> pt_message;

const string ServerIP = "server.mcgc.top";
const int ServerPort = 25564;

const string PipeName = "\\\\.\\pipe\\GameClicker";
const DWORD PipeBufSize = 4096;
const int PipeMaxConcurrentOps = 10;

typedef struct GameGuiPipeData
{
    string version = ::version;                  //版本号
    int RCPS;                                    //右键CPS
    int LCPS;                                    //左键CPS
    ULONGLONG LClickerClickCount;                //左键连点次数
    ULONGLONG LMouseClickCount;                  //左键鼠标点击次数
    ULONGLONG RClickerClickCount;                //右键连点次数
    ULONGLONG RMouseClickCount;                  //右键鼠标点击次数
    long double UseClickerTime;                  //使用连点器的时长(小时)
    ULONGLONG StartTime;                         //GC启动时的时间
    vector<float> RRecordsCPS;                   //右键CPS记录
    vector<float> LRecordsCPS;                   //左键CPS记录
    vector<float> CpuRates;                      //CPU占用率记录
    vector<float> GpuRates;                      //GPU占用率记录
    vector<float> MemRates;                      //内存占用率记录
    LRPair<bool> IsClickering = {};              //是否连点器
    LRPair<bool> IsClickerDown = {};             //连点器的键按下
    LRPair<bool> IsHumanDown = {};               //鼠标键是否按下
};