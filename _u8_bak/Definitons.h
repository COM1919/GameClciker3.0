#pragma once
#include "framework.h"
#include "AgreementString.h"
#include "UDPSocket.h"
#include <atomic>

#define PI 3.14159265358979323846f

//Ĭ����ֵ
#define DEFAULT_CPS     12    //Ĭ�ϵ�CPS
#define DEFAULT_MCV     2     //Ĭ�ϵĵ������
#define DEFAULT_RCTIME  1000  //Ĭ�ϵĵ��ʱ��
#define DEFAULT_RCSLEEP 500   //Ĭ�ϵ�ֹͣ���ʱ��
#define DEFAULT_LCTIME  1000  //Ĭ�ϵĵ��ʱ��
#define DEFAULT_LCSLEEP 200   //Ĭ�ϵ�ֹͣ���ʱ��
#define RESEND_SLEEP    2     //���³��Է��͵��ӳ�
#define FILTER_SC_SLEEP 130   //���˵��ε�����ӳ�

//���ֵ����Сֵ
#define MAX_VALUE        4096 //��ߵ���ֵ
#define MAX_BUF          2048 //��ߵĻ����С
#define MAX_QUICK_SWITCH 6    //�����л�CPS��CPSֵ
#define MAX_RECORDS      1800 //����¼��
#define MAX_PERF_RECORDS 600  //������ܼ�¼��
#define MAX_CPS          40   //���CPS
#define MIN_CPS          1    //��СCPS
#define MAX_TRYVALUE     3    //����Դ���
#define MAX_SCRIPT_ERROR_RECORDS 100 //��ౣ���Ľű�������¼��

//����ͼ����Ϣ
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
#define URL_GITEE_SOUNDS   "https://gitee.com/com1919/gameclicker-information/releases/download/KeySound2/keys.zip"

#define MAIN_R_KEY    5       //Ĭ��������������Ҽ��Ŀ�ݼ�
#define MAIN_L_KEY    6       //Ĭ�����������������Ŀ�ݼ�
#define C_BUTTONMODE  0b0001  //���¿�ݼ����㣬�ɿ�ֹͣ
#define C_SWITCHMODE  0b0010  //���¿�ݼ����㣬�ٰ���ֹͣ
#define C_ASSISTMODE  0b0100  //���������������Ҽ����Զ����һ�λ��߶��Σ�����˫�����
#define C_MOUSESMODE  0b1000  //��ס������������

#define SC_MOUSE_EVENT  0     //ʹ��mouse_event��������keybd_event����
#define SC_SEND_INPUT   1     //ʹ��SendInput����
#define SC_POST_MESSAGE 2     //ʹ��PostMessage����

#define GK_HOOK 1             //ʹ��ȫ��ϵͳ���ӻ�ȡ����״̬
#define GK_API  2             //ʹ��ϵͳAPI��������״̬
#define GK_BOTH 3             //��ʹ��

#define MS_NC         10      //ƽ��
#define MS_JC         20      //�������
#define MS_OLD        40      //�ɰ���

#define FRM_GL        1       //������
#define FRM_GS        2       //�𽥼�С
#define FRM_SL        3       //С����
#define FRM_LS        4       //��С
#define FRM_SLS       5       //С���󣬴�С
#define FRM_LSL       6       //��С��С����

#define CP_NONE       0       //�����ȼ�
#define CP_RC         1       //�Ҽ���������
#define CP_LC         2       //�����������

#define WARNING_NONE    0     //�޾�����Ч
#define WARNING_SYSTEM  1     //ϵͳĬ�Ͼ�����Ч
#define WARNING_SOUND   2     //�Զ��徯����Ч

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

//���������������
enum GenerateDis
{
    GenerateDis_bernoulli_distribution = 0,     //��Ŭ���ֲ�
    GenerateDis_uniform_int_distribution,       //���ȷֲ�
    GenerateDis_binomial_distribution,          //����ֲ�
};

enum GenerateGen
{
    GenerateGen_mersenne_twister = 0,            //÷ɭ��ת�㷨
    GenerateGen_ranlux24_base,                   //RANLUX24�㷨
    GenerateGen_minstd_rand,                     //��С��׼������㷨
    GenerateGen_discard_block_engine,            //�������㷨
    GenerateGen_independent_bits_engine,         //����λ�㷨
    GenerateGen_shuffle_order_engine,            //��������㷨
    GenerateGen_linear_congruential_engine,      //����ͬ���㷨
    GenerateGen_subtract_with_carry_engine,      //��ȥ�����㷨
};

enum RandomBoolMode
{
    RandomBoolMode_UNIFORM,   // ��ͨ�������׼���ȸ���ֲ�����ͳ�����
    RandomBoolMode_FATIGUE,   // ƣ��ЧӦ����ʱ������ʧ��������
    RandomBoolMode_BIONIC     // �������飺ƣ�� + �ֶ� + �����ɷ����
};

// CPS����
struct WaveParam
{
    float amplitude;   // �����CPS�������ȣ�
    float frequency;   // Ƶ�ʣ�Hz��
    float phase;       // ��λ�����ȣ�
    bool enabled;      // �Ƿ����ã�����ʱ�رգ�
};

//--------------------��������------------------------------

class Config
{
public:
    int LKEY = 6;                           //�����ݼ�
    int RKEY = 5;                           //�Ҽ���ݼ�
    int LCPS = DEFAULT_CPS;                 //���CPS
    int RCPS = DEFAULT_CPS;                 //�Ҽ�CPS
    float RFluctuationCPS = 1;              //�Ҽ�CPS����
    float LFluctuationCPS = 1;              //���CPS����
    int MainClickSleep = 4;                 //������ѭ��ÿ��ѭ�����ӳ٣����룩
    int RMultipleClickValue = DEFAULT_MCV;  //����ģʽ�����
    int LMultipleClickValue = DEFAULT_MCV;  //����ģʽ�����
    int RQuickSwitch[MAX_QUICK_SWITCH]{};   //�Ҽ������л�CPS
    int LQuickSwitch[MAX_QUICK_SWITCH]{};   //��������л�CPS
    int QuickSwitchKey[MAX_QUICK_SWITCH]{}; //�����л�CPS��ݼ�
    int QuickSwitchTotal = 0;               //�����л�CPS�Ĵ�С
    int REmptyRateValue = 20;               //�Ҽ��յ�����(%)
    int LEmptyRateValue = 20;               //����յ�����(%)
    int RCTime = DEFAULT_RCTIME;            //�Ҽ����ʱ��(����)
    int RCSleep = DEFAULT_RCSLEEP;          //�Ҽ�ֹͣ���ʱ��(����)
    int LCTime = DEFAULT_LCTIME;            //������ʱ��(����)
    int LCSleep = DEFAULT_LCSLEEP;          //���ֹͣ���ʱ��(����)
    int RClickerMode = C_BUTTONMODE;        //�Ҽ�����ģʽ
    int LClickerMode = C_BUTTONMODE;        //�������ģʽ
    int RPressDuration = 20;                //�Ҽ�������㰴��ʱ��(����)
    int LPressDuration = 20;                //���������㰴��ʱ��(����)
    int EnableRightClickerKey;              //�����Ҽ������ݼ�
    int EnableLeftClickerKey;               //������������ݼ�
    int ChatBarKey = 'T';                   //��������ݼ�
    int BagBarKey = 'E';                    //��������ݼ�
    int MouseShakeSpeed = 300;              //��궶���ٶ�
    int MouseShakeRect = 8;                 //��궶����Χ
    array<int, 10>                          //��Ʒ�����������ݼ�
        ItemBarKey = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
    LRPair<array<int, 10>>                  //��Ʒ��CPSԤ��
        ItemBarPresetCPS = {
            { DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS },
            { DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS, DEFAULT_CPS }
        };
    LRPair<array<bool, 10>>
        ItemBarEnablePresetCPS = {          //��Ʒ��Ԥ��CPS
            { false, false, false, false, false, false, false, false, false },
            { false, false, false, false, false, false, false, false, false }
        };
    int MouseShake_RandomDist =             //��궶����������ɷֲ�
    GenerateDis_uniform_int_distribution;
    int EmptyRate_RandomDist =              //�յ�������������ɷֲ�
    GenerateDis_bernoulli_distribution;
    int EmptyRate_RandomMode =              //CPS�������������ģʽ
    RandomBoolMode_UNIFORM;
    int MouseSoundSleep = 5;                //�����������ѭ���ӳ�
    int AssistModeCPSThreshold = 4;         //����ģʽ��CPS����
    bool UseOverheatThrottle = false;       //���Ƚ�Ƶ����
    int Overheat_MinCPS = 10;               //���Ƚ�Ƶ��CPS������ƣ����ܵ��ڴ�ֵ��
    float Overheat_Time = 5.0f;             //�����ٶȣ���������������ﵽ���ȵ�
    float Overheat_ThrottleTime = 5.0f;     //��Ƶ�ٶȣ�������ȵ�󣬶������ڽ���CPS�������
    int SelfDestructionKey = 0;             //�Իٿ�ݼ�
    int UseLFluctuationRandMode = FRM_SL;   //���CPS�������ģʽ
    int UseRFluctuationRandMode = FRM_SL;   //�Ҽ�CPS�������ģʽ
    int RStartClickerDelay = 1000;          //�Ҽ������ӳ�(����)
    int LStartClickerDelay = 1000;          //��������ӳ�(����)
    int SetGameWindowKey = 0;               //������Ϸ���ڿ�ݼ�
    int HideClickerKey = 0;                 //������������ݼ�
    vector<int> RCPSChangeTrendList;        //�Ҽ�CPS�仯�����б�
    vector<int> LCPSChangeTrendList;        //���CPS�仯�����б�
    float OverheatTime = 5.0f;              //����ʱ��
    float OverheatCooldownTime = 3.0f;      //���ȱ�������ȴʱ��
	int OverheatSoundType = WARNING_SYSTEM; //���ȼ�ʱ����ʱ�������ٶ�
    int SimulateClickMode = SC_MOUSE_EVENT; //ģ����ģʽ
    int ReadKeyStateMode = GK_BOTH;         //��ȡ����״̬ģʽ
    int ClickerPriorityMode = CP_NONE;      //���Ҽ���������ȼ�ģʽ
   
    LRPair<vector<WaveParam>> CPSWaves;

    //--------------------�����������------------------------------

    bool UseMouseSound = false;             //�������������
    bool UseKeybdSound = false;             //���ؼ��̵������
    int MouseSoundVolume = 100;             //�������������
    int KeybdSoundVolume = 100;             //���̵����������
    int NoiseVolume = 90;                   //��������
    string MouseSoundType =                 //�������������
        "Corsair M65 Elite";
    string KeybdSoundType =                 //��������������
        "CherryMX Blue - PBT keycaps";

    //--------------------���ܿ���------------------------------

    bool UseREmptyRate = false;             //�����Ҽ��յ�����
    bool UseLEmptyRate = false;             //��������յ�����
    bool UseRFluctuation = false;           //�����Ҽ�CPS����
    bool UseLFluctuation = false;           //�������CPS����
    bool UseLClickTime = false;             //�������������
    bool UseRClickTime = false;             //�����Ҽ�������
    bool UseGUIDetection = false;           //����GUI���
    bool UseLClickPressTime = false;        //����������¼��
    bool UseRClickPressTime = false;        //�����Ҽ����¼��
    bool UseRAutoPressDuration = true;      //�����Զ������Ҽ�����ʱ��
    bool UseLAutoPressDuration = true;      //�����Զ������������ʱ��
    bool UseStartWithWindows = false;       //���ؿ�������
    bool EnableRightClicker = true;         //�����Ҽ�����
    bool EnableLeftClicker = true;          //�����������
    bool UseBroadcasterMode = false;        //���ط�ֹ����GC����
    bool UseAutoGuiDetect = false;          //����ͨ�����������Ϸui
    bool UseROnlyMoveClicker = false;       //���ؽ��ƶ�ʱ�Ҽ�����
    bool UseLOnlyMoveClicker = false;       //���ؽ��ƶ�ʱ�������
    bool UseDisableMoveOnStopMove = false;  //��������ֹͣ�ƶ�ʱ��������
    bool UseMouseShake = false;             //������궶��
    bool UseMouseShakeRawInput = false;     //�����Ƿ�����ƶ�ʱʹ��ԭʼ����
    bool UseMSPreventViewSliding = true;    //���ط�ֹ�ӽǻ���
    bool UseMSSmoothSpeed = true;           //����ƽ���ٶ�
    bool UseItembarUseMemSearch = true;     //������Ʒ������ʹ���ڴ�����
    LRPair<array<bool, 10>> ItemEnableClicker = {//��Ʒ����������
        { true, true, true, true, true, true, true, true, true },
        { true, true, true, true, true, true, true, true, true }
    };
    bool UseChinese = true;                 //��������ģʽ
    bool UseDriverProtect = false;          //������������
    bool UseHiddenProcess = false;          //�������ؽ���
    bool UseSuspendNetease = false;         //���ع���������Ϸ
    bool AllowSetLClicker = true;           //�������������������
    bool AllowSetRClicker = true;           //�������������Ҽ�����
    bool UseEnableGameGui = false;          //���ؿ�����Ϸ�Ӿ�Ч��
    bool UseROnlyJumpClicker = false;       //������Ծʱ�����Ҽ�����
    bool UseLOnlyJumpClicker = false;       //������Ծʱ�����������
    bool UseRestTimeOnStopClicker = true;   //����ֹͣ����ʱ��������������ʱ��
    bool UseBrutalMode = false;             //���ر���ģʽ
    bool UseLClickerTrigInter = false;      //��������ʱ�����ھ򷽿�
    bool UseRClickerTrigInter = false;      //��������ʱ����ʹ����Ʒ
    bool UseOldMethodToTrigInter = false;   //����ʹ���ϰ취ʵ���ƻ�����/ʹ����Ʒ
    bool UseRStartClickerDelay = false;     //�����Ҽ������ӳ�
    bool UseLStartClickerDelay = false;     //������������ӳ�
    bool UseFilterRSingleClick = false;     //���ع����Ҽ����ε��
    bool UseFilterLSingleClick = false;     //���ع���������ε��
    bool UsePerformanceMode = false;        //��������ģʽ
    bool UseRCPSChangeTrend = false;        //�����Ҽ�CPS�仯����
    bool UseLCPSChangeTrend = false;        //�������CPS�仯����
    bool UseOverheatProtec = false;         //���ع��ȱ���
    bool UseBEAdaptation = false;           //�Ƿ�����Bedrock Edition
    bool UseInGameOnlyClicker = false;      //������Ϸ��ʹ��������
    bool UseItembarSwitch = false;          //��Ʒ������
    bool UseHumanOnlyTrigger = false;       //�������ഥ��������
}config;
// config �ؽ���ǡ�ԭ������ detach �߳���ִ�� config.~Config() + placement new��
// �������̲߳�����д config ���ֶ� -> �������������жϣ�0xC0000005����
// ��Ϊ��̨�߳�ֻ���û�ȷ�ϣ�ȷ�Ϻ���λ�˱�ǣ�������Ⱦ�߳���֡�߽�ִ���ؽ���
atomic<bool> IsNeedResetConfig = false;
//shared_mutex ConfigAccessMutex;

//--------------------���Ի�����------------------------------
int BgOffsetX = 0;                          //����ƫ��X
int BgOffsetY = 0;                          //����ƫ��Y
int BgScale = 100;                          //�������Űٷֱ�
int BgLightness = 75;                       //�������Ȱٷֱ�
int GuiItemAlpha = 40;                      //�ؼ���͸���Ȱٷֱȣ����б���ͼƬʱ����ʹ�ã�
bool BgAutoResize = true;                   //�Զ�����������С
bool DisableScriptAutoStart = false;        //���ýű��Զ��������ر�ʱ�����������Զ������Ѱ�װ�ű���

inline const array<const char*, 2> RandomDistList = {
    (config.UseChinese ? u8"���������ֲ�"       : "Uniform Int Distribution"    ),
    (config.UseChinese ? u8"����ֲ�"           : "Binomial Distribution"       )
};
inline const array<const char*, 3> BoolRandomDistList = {
    (config.UseChinese ? u8"Bool���ͷֲ�"       : "Bernoulli Distribution"      ),
    (config.UseChinese ? u8"���������ֲ�"       : "Uniform Int Distribution"    ),
    (config.UseChinese ? u8"����ֲ�"           : "Binomial Distribution"       )
};
inline const array<const char*, 3> RandomBoolModeList = {
    (config.UseChinese ? u8"��ͨ�����ʹ���Զ�������ֲ�" : "Normal: Use custom random dist"                            ),
    (config.UseChinese ? u8"ƣ��ЧӦ����ʱ������ʧ��������" : "Fatigue: error rate increases over time"                 ),
    (config.UseChinese ? u8"�������飺ƣ�͡��ֶ��������ɷ����" : "Ghost: Fatigue / Hand Shaking / Markov Error Correction" )
};

//--------------------����------------------------------

string GameWindowTitle = config.UseChinese ? "��" : "None"; //��Ϸ���ڱ���

int FollowGameStartPID = 0;             //������Ϸ�����Ľ���ID
vector<float> RRecordsCPS;              //�Ҽ�CPS��¼��1�뻬������ͳ�ƣ�
vector<float> LRecordsCPS;              //���CPS��¼��1�뻬������ͳ�ƣ�
vector<float> RRecordsInstantCPS;       //�Ҽ�˲ʱCPS��¼��������ε��������㣬��RRecordsCPSͬ��д�룩
vector<float> LRecordsInstantCPS;       //���˲ʱCPS��¼��������ε��������㣬��LRecordsCPSͬ��д�룩
vector<float> CpuRates;                 //CPUռ���ʼ�¼
vector<float> GpuRates;                 //GPUռ���ʼ�¼
vector<float> MemRates;                 //�ڴ�ռ���ʼ�¼
vector<vector<float>> ScriptCpuRates;   //�ű�CPUռ���ʼ�¼

mutex RecordsCPSMutex;                  //RRecordsCPS/LRecordsCPS/RRecordsInstantCPS/LRecordsInstantCPS �Ļ�����

shared_mutex PerformanceMutex;          //CpuRates/GpuRates/MemRates �Ļ�������RecordsPCRate �߳�д������Ⱦ�̶߳���
atomic<bool> ThreadExitFlag(false);     //�߳��˳���־

constexpr array<const char*, 4> ScriptRestrictedFuncs = {
    "SetClipboardText", "GetClipboardText",
    "SimulatedInputText", "SetCursorPos"
};

struct ScriptInfo
{
    int ver = 1;
    string name;
    string info;
    string file;
    string texture; // ͼƬ���ļ�·��
    string title;
    string link;
    string docs; // �ĵ��ļ�·��
    HANDLE hThread{};
    vector<string> RestrictedFuncs;
    string RestrictedFuncsStr;
    bool IsInstalled = false;
    bool IsHasUpdate = false;
};

struct ScriptState
{
    int key = 0;
    int NetworkPermission = 0; // ����Ȩ��: 0=δ����, 1=������, 2=�Ѿܾ�
    bool IsPressedKey = false; // �������Ҫ�Լ�����
    bool IsClickedKey = false; // �����Ҫ�ڶ�ȡ������Ϊfalse
    bool IsReleaseKey = false; // �����Ҫ�ڶ�ȡ������Ϊfalse
    int LoopDelay = 0;
    bool IsExit = false;
    bool IsSuspend = false;
    bool IsRunning = false;    // �ű��߳��Ƿ�����������"����"�����ж��Ƿ���Ҫ�����ű���
};

// Lua �ű�������¼
struct ScriptErrorRecord
{
    string file;        // �����Ľű��ļ�·��
    string time;        // ����ʱ��
    string message;     // ������Ϣ�������ö�ջ��
};

namespace ScriptData
{
    shared_mutex ErrorRecordsMtx;       //�ű�������¼����
    vector<ScriptErrorRecord> ErrorRecords; //�ű�������ʷ��¼�����µ������
    shared_mutex InstalledMtx;          //�Ѱ�װ�Ľű���
    shared_mutex AvaiableMtx;           //���õĽű���
    shared_mutex StatesMtx;             //�ű�״̬����
    shared_mutex ShopListMtx;           //�ű��̵��б�������ˢ���߳� clear/������̱߳������������̱߳�����
    vector<ScriptInfo> Installed;       //�Ѱ�װ�Ľű�
    vector<ScriptInfo> Avaiable;        //���õĽű�
    vector<ScriptInfo> ShopList;        //�ű��̵�Ľű��б�
    map<string, ScriptState> States;    //�ű����״̬����
}

namespace Sponsor
{
    shared_mutex Mtx;                       
    vector<string> List;                //�����б�
}

const string version = "3.57";          //�汾��
string title = "GAMECLICKER 3.57 �ڲ����԰� / �����ű�API";      //��������
constexpr int ConfigVersion = 5;        //�����ļ��汾��
constexpr int AgreementVersion = 8;     //Э���ļ��汾��
bool IsNeedReshowAgreement = false;     //�Ƿ���Ҫ��ʾ���µ�Э�飬���������ļ�����İ汾�ŶԱ�

inline const int RenderFps = 60;        //��Ⱦ֡��

//#define GC_USE_D_DRIVE                //�Ƿ�ʹ��D�̵������ļ����������
//#define GC_NO_HOOK                    //�Ƿ����HOOK
//#define GC_DEBUG_VS                   //�Ƿ���VS�ĵ��Դ��������־
#define GC_DEBUG_CMD                    //�Ƿ��������������־
//#define GC_NO_ANIMATION               //�Ƿ������������
//#define GC_REMOTE_DEBUG               //�Ƿ�ʹ��Զ�̵���
//#define GC_MORE_DEBUG_INFO            //������ϸ������Ϣ
//#define GC_NO_MSGBOX                  //������Ϣ����
//#define GC_SERVER_DEBUG               //�Ƿ���������ͨѶ����
#define GC_ENABLE_QUICK_EXIT            //�������Կ����˳�
//#define GC_DEBUG_GUIDE_UI             //����������ʾ��UI

//#define GC_RELEASE_MODE               //�Ƿ�Ϊ�����棨����Ǿ��Զ��ر����е������ݣ�

#ifdef GC_RELEASE_MODE
#undef GC_DEBUG_VS
#undef GC_DEBUG_CMD
#undef GC_NO_ANIMATION
#undef GC_NO_SAFE_MODE
#undef GC_NO_TRY_CATCH
#undef GC_SERVER_DEBUG
#undef GC_REMOTE_DEBUG
#undef GC_MORE_DEBUG_INFO
#undef GC_ENABLE_QUICK_EXIT
#undef GC_NO_MSGBOX
#undef GC_NO_HOOK
#undef GC_DEBUG_GUIDE_UI
#undef GC_USE_D_DRIVE
#endif // GC_RELEASE_MODE

#define USE_IN_MICROSOFT              //�Ƿ���΢���̵귢�а汾��΢���̵��Ҫ�ر��Զ����£�

//=============================��������==========================================================
// ����Щ�ļ��ķ�װ��class FileDefines����������Ϊpath
#ifdef GC_USE_D_DRIVE
const string WorkPath = "D:";
#else
const string WorkPath = []() -> string {
    char buf[4096]{};
    if (GetModuleFileNameA(NULL, buf, 4096) == 0)
        return "";
    string str(buf);
    size_t pos = str.find_last_of("\\/");
    if (pos != string::npos)
        return str.substr(0, pos);
    else
        return "";
}();
#endif // GC_USE_D_DRIVE

inline const string FontFile =           "C:\\Windows\\Fonts\\msyhbd.ttc";
inline const string ConfigPath =         WorkPath + "\\GameClicker";
inline const string ConfigFile =         WorkPath + "\\GameClicker\\settings.ini";
inline const string CustomConfigFile =   WorkPath + "\\GameClicker\\custom.ini";
inline const string IniFileVeision =     WorkPath + "\\GameClicker\\Version.dat";
inline const string StatsRegPath =       "Software\\GameClicker\\Stats";
inline const string PresetDir =          WorkPath + "\\GameClicker\\configs";
inline const string PresetDefaultName =  "Default";

inline const string SoundPath =          "sound";
inline const string Sound_Noise =        "sound\\noise.wav";
inline const string Sound_MousePath =    "sound\\mouse";
inline const string Sound_Enable =       "sound\\func\\enable.wav";
inline const string Sound_Disable =      "sound\\func\\disable.wav";
inline const string Sound_Warning =      "sound\\func\\warning.wav";
inline const string Sound_CfgPath =      WorkPath + "\\GameClicker\\sound";
inline const string Sound_KeybdPath =    WorkPath + "\\GameClicker\\sound\\keys";
inline const string Tmp_KeySounds =      WorkPath + "\\GameClicker\\Temp\\KeySounds.zip";

inline const string PicsPath =           "res\\pics";
inline const string Pics_Selected =      "res\\pics\\selected.png";
inline const string Pics_Itembar =       "res\\pics\\itembar.png";
inline const string Pics_Background =    WorkPath + "\\GameClicker\\bg.gcbg";
inline const string Pics_GCLogo =        "res\\pics\\Logo.png";
inline const string Pics_About =         "res\\pics\\ico\\About.png";
inline const string Pics_Setting =       "res\\pics\\ico\\Setting.png";
inline const string Pics_Home =          "res\\pics\\ico\\Home.png";
inline const string Pics_Script =        "res\\pics\\ico\\Script.png";
inline const string Pics_Info =          "res\\pics\\ico\\Info.png";
inline const string Pics_Delete =        "res\\pics\\ico\\Delete.png";
inline const string Pics_Add =           "res\\pics\\ico\\Add.png";
inline const string Pics_Other =         "res\\pics\\ico\\Other.png";
inline const string Pics_Refresh =       "res\\pics\\ico\\Refresh.png";
inline const string Pics_FunSetting =    "res\\pics\\ico\\FunSetting.png";
inline const string Pics_GCIcon =        "res\\pics\\ico\\GCIcon.png";
inline const string Pics_ClearCache =    "res\\pics\\ico\\ClearCache.png";
inline const string Pics_TipInfo =       "res\\pics\\ico\\TipInfo.png";
inline const string Pics_TipError =      "res\\pics\\ico\\TipError.png";
inline const string Pics_TipWarning =    "res\\pics\\ico\\TipWarning.png";
inline const string Pics_Empyt =         "res\\pics\\ico\\Empyt.png";
inline const string Pics_Book =          "res\\pics\\ico\\Book.png";
inline const string Pics_Download =      "res\\pics\\ico\\Download.png";
inline const string Pics_Run =           "res\\pics\\ico\\run.png";
inline const string Pics_Stop =          "res\\pics\\ico\\stop.png";
inline const string Pics_GCTitle =       "res\\pics\\GCTitle.png";

inline const string StartupMov =         "res\\startup.webp";

inline const string ScriptPath =         WorkPath + "\\GameClicker\\Script";
inline const string ScriptTestFile =     WorkPath + "\\GameClicker\\Script\\test.lua";
inline const string ScriptTemp =         WorkPath + "\\GameClicker\\Script\\temp";
inline const string ScriptIconTemp =     WorkPath + "\\GameClicker\\Script\\shop\\icons";
inline const string ScriptInstallList =  WorkPath + "\\GameClicker\\Script\\InstallList.txt";
inline const string ScriptShopDir =      WorkPath + "\\GameClicker\\Script\\shop";
inline const string ScriptShopTemp =     WorkPath + "\\GameClicker\\Script\\shop\\temp";

inline const string LogPath =            WorkPath + "\\GameClicker\\Logs";
inline const string LogFile =            WorkPath + "\\GameClicker\\GC.log";
inline const string LogFileZip =         "GC_LOG-%Y-%m-%d %H-%M-%S.zip";

string NoticeStr = config.UseChinese ?
u8R"(�������ʧ��
===============================================
������ �� ȫ �� �� ���������ͨ�������õ�GC���������뼰ʱ�˿�ٱ���
)" :
R"(Notice loading failed
===============================================
This software is completely free! If you obtained the GameClicker through purchase, please refund and report it promptly.
)";

// �޸���NoticeStr �� GetInfo �߳�д�롢���߳�ÿ֡��ȡ��ʾ��c_str()��������ʱд�߳� realloc
// �������̳߳�����ָ�� �� 0xC0000005��
shared_mutex NoticeMutex;

inline const array<string, 48> DailyMottos = {
    config.UseChinese ? u8"&b���������Ƿ羰����ʵ������Ǽ�԰��" : "&bDigital world is scenery, real life is home.",
    config.UseChinese ? u8"&bÿһ��רעѧϰ�����Ƕ�δ����Ͷ�ʡ�" : "&bEach study session invests in your future.",
    config.UseChinese ? u8"&b��������ս����Ļ�⣬���ӱܳɳ���" : "&bReal challenges are off-screen - don't avoid growth.",
    config.UseChinese ? u8"&b��������һСʱ�������һ��ѡ��Ȩ��" : "&bOne less gaming hour today, more choices tomorrow.",
    config.UseChinese ? u8"&b������Ҫѵ����������Ҫ֪ʶ��" : "&bSkills need practice, mind needs knowledge.",
    config.UseChinese ? u8"&b���������ͱ���Ϸ��������Ҫ��" : "&bThree healthy meals beat gaming rankings.",
    config.UseChinese ? u8"&b�����ĳɾͲ��������������н�����" : "&bReal achievements aren't unlocked in virtual worlds.",
    config.UseChinese ? u8"&b��...���ٳ��֣��������ҵ�������" : "&bShe...Not appearing again, taking away my life",
    config.UseChinese ? u8"&b��ҹ���ǿձ���Ϸ�����׳����" : "&bNight stars outshine game graphics.",
    config.UseChinese ? u8"&b�˶��������ڷ��ı���Ϸ�̼����־á�" : "&bExercise endorphins last longer than gaming thrills.",
    config.UseChinese ? u8"&b..." : "&b...",
    config.UseChinese ? u8"&b������ĸ����������ջ�������ء�" : "&bHelp with chores, earn more respect.",
    config.UseChinese ? u8"&b����һ���������ã����ָ������á�" : "&bDevelop an art hobby, discover more beauty.",
    config.UseChinese ? u8"&b��Ϸ��ɫ�����ã�����û�����¿�ʼ��" : "&bGame characters reset, life doesn't restart.",
    config.UseChinese ? u8"&b�������ŶӺ�������ʵ��Ŀ��ĥ����" : "&bReal teamwork sharpens in actual projects.",
    config.UseChinese ? u8"&b��Ҫ...��Ҫ...��..." : "&bNo...NO..NOOO...",
    config.UseChinese ? u8"&bÿ���10ҳ�飬һ�����3650ҳ��" : "&bRead 10 pages daily, 3650 pages a year.",
    config.UseChinese ? u8"&b��Ϸ�����ˢ�£��������᲻�ȴ���" : "&bGame quests refresh, life chances don't wait.",
    config.UseChinese ? u8"&bͶ���Լ��ļ��ܣ�������õ�װ����" : "&bInvest in skills - your best equipment.",
    config.UseChinese ? u8"&b��ʵ��������Ҫ��ʵ��ʱ����������" : "&bReal friendships need real time to grow.",
    config.UseChinese ? u8"&b��...��..���ܡ�" : "&b...Asdfasd....It's uncomfortable.",
    config.UseChinese ? u8"&bѧ����⿣�Ϊ�Լ���һ�ٽ����Ĳ͡�" : "&bLearn cooking, make yourself a healthy meal.",
    config.UseChinese ? u8"&b��Ϸ����ֻ�����ݣ����˳ɳ�������ʵ��" : "&bGame ranks are data, personal growth is real.",
    config.UseChinese ? u8"&b����...����..." : "&bI really want to ... liberation ..",
    config.UseChinese ? u8"&b����ͬѧ������⣬����ø�����֡�" : "&bHelp classmates solve problems, gain more joy.",
    config.UseChinese ? u8"&b������...��ô�ˣ�" : "&bThis world..... What's wrong?",
    config.UseChinese ? u8"&b����һ�" : "&bEmpty nothing.",
    config.UseChinese ? u8"&bд�ռǼ�¼�ɳ�������Ϸ�ɾ͸������塣" : "&bKeep diary for growth, more meaningful than achievements.",
    config.UseChinese ? u8"&b��ע�Ƽ����ţ��˽���ʵ����Ĵ��¡�" : "&bFollow tech news, learn real-world innovation.",
    config.UseChinese ? u8"&b��...��ʹ��...�ܲ���..." : "&bAh...It's so painful...Can y....",
    config.UseChinese ? u8"&b���������Ӣ�ۣ�������ʵ�е���ʿ��" : "&bVirtual heroes can't match real warriors.",
    config.UseChinese ? u8"&b��...��ֲ�ס��..." : "&bI...I can't hold on anymore",
    config.UseChinese ? u8"&b�Ҳ��ܿ������ˡ�" : "&bI can't see the light anymore.",
    config.UseChinese ? u8"&b�μ�־Ը�߻�����ܸ���Ŀ��֡�" : "&bVolunteer to feel the joy of giving.",
    config.UseChinese ? u8"&b�����������������������ռ����ܡ�" : "&bDevelop problem-solving - the ultimate skill.",
    config.UseChinese ? u8"&b��ȥ��������ɣ�ʱ�䲻���ˡ�" : "&bGo out and see the world, time is running out.",
    config.UseChinese ? u8"&b�벻ͬ�������˽�������չ��Ұ��" : "&bTalk to diverse people, broaden horizons.",
    config.UseChinese ? u8"&b�Ҳ��������Լ��ˣ��ټ����硣" : "&bI am no longer myself, goodbye to the world.",
    config.UseChinese ? u8"&b�����������õģ�������... ��ʹ..." : "&bI believe you will be better, unlike me....It hurts so much...",
    config.UseChinese ? u8"&b��ϰ�����ݽ��������������š�" : "&bPractice public speaking, boost confidence.",
    config.UseChinese ? u8"&bŶ����������ĵ�����ķ��ཱུġ�" : "&bTIME ARE CHANGING",
    config.UseChinese ? u8"&b��Ϸ���ܻ��ʱ��ѧϰ������Զ��ֵ��" : "&bGame skills expire, learning ability lasts.",
    config.UseChinese ? u8"&b���������ԣ���������ǰ��" : "&bListen to the elderly more often, it's hard to progress.",
    config.UseChinese ? u8"&bʱ��..." : "&bTime...",
    config.UseChinese ? u8"&bѧϰ����ѧ�����õ������Լ������ˡ�" : "&bStudy psychology, understand yourself and others.",
    config.UseChinese ? u8"&b����ҵ����ѡ�" : "&bHello my friends.",
    config.UseChinese ? u8"&b��...������..." : "&bI... It's so uncomfortable ...",
    config.UseChinese ? u8"&bNothings" : "&bbNothings"
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
HANDLE WindowMsgThreadHandle = nullptr;   // ������Ϣ�߳̾��, �˳�ʱ�ȴ���������ͷ� ImGui/D3D

atomic<bool> IsWindowInitSuccessful  = false;           //�����Ƿ񴴽��ɹ�
atomic<bool> IsStopClicker           = false;           //�Ƿ�ֹͣ����
atomic<bool> IsGCExit                = false;           //�������˳�
atomic<bool> IsShowGui               = true;            //�Ƿ���ʾGUI
atomic<bool> IsFirstRun              = false;           //��һ�����У��Ƿ���ʾ�򵼣�
LRPair<atomic<bool>> IsClickering    = {};              //�Ƿ�����
atomic<bool> IsMoving                = false;           //����Ƿ��ƶ�
atomic<bool> IsJumping               = false;           //����Ƿ���Ծ
atomic<bool> IsOpenGui               = false;           //����Ϸ���Ƿ��GUI
atomic<bool> IsDownloading           = false;           //�Ƿ���������
atomic<bool> IsReloadClicker         = true;            //�Ƿ����������߳�
atomic<bool> IsClickerSuccess        = false;           //�����߳��Ƿ�׼���ɹ�
atomic<bool> IsRestartAL             = false;           //�Ƿ�����OpenAL
atomic<bool> IsServerClosed          = false;           //�������Ƿ�ر�
atomic<bool> AllowUseGameGui         = false;           //�Ƿ�����ʹ���Ӿ�Ч��
atomic<bool> AllowConnectServer      = false;           //�Ƿ��������ӷ�����
atomic<bool> IsDragingFile           = false;           //�Ƿ����������ļ�
atomic<bool> IsCPSWaveSettingChanged = false;           //�Ƿ������CPS��������
atomic<bool> IsScriptUseGitee        = false;           //�ű��̳��Ƿ�ʹ��gitee

int ClickTabCount = 11;                                 //��������ǩҳ�Ĵ���
int GameGuiTransparency = 40;                           //��λ%
int IsSearchState = 0;                                  //�ڴ�����״̬
int MemSearchCount = 0;                                 //�ڴ���������
int GettingGameState = -1;                              //��ȡ��Ϸ���ڵ�״̬
float SystemScale = 1.0f;                               //DPI������

vector<string> MouseSoundList;                          //���õ���������б�
vector<string> KeybdSoundList;                          //���õļ��������б�

UDPSocket NetworkClient;
string ClientID;
mutex ClientIDMtx;


namespace PresetConfig
{
    vector<string> Presets;
    string CurrentName = PresetDefaultName;
    bool ShowAdd = false;
    char NameBuf[128]{};
    float ErrorTime = 0.0f;
    string ErrorMsg;
}

namespace OverheatTimer
{
    float Heat = 0;                                     //���ȱ�����ʱ��
    float Cooldown = 0;                                 //���ȱ�����ȴ��ʱ��
}
int UseServerOfType = -1;                               //ʹ�õķ�����������
int ItemBarPos = 1;                                     //��Ʒ��λ��

shared_mutex CurrentCPSMutex;
LRPair<int> CurrentCPS = {};                            //��ʵ��CPS
shared_mutex HumanCPSMutex;
LRPair<int> HumanCPS = {};                              //����CPS��1�뻬�����ڣ��� HandleCPSRecord ά����

// ����ģʽר�ã����ఴ�µ�"�¼�����"��
// ����ÿ��ʶ��һ����ʵ"����"�¼���++���¼�����ȷ���������κ���ѯƵ�ʣ���
// ����ģʽͨ�� ConsumeHumanPresses ԭ�Ӷ�ȡ�����㣬���׽��
LRPair<atomic<int>> HumanPressCount = {};               //���ఴ���¼���Down�¼�������
struct FastCPSWindow
{
    shared_mutex             Mtx;
    LRPair<deque<ULONGLONG>> Stamps;    // ��ʵ����ʱ��������룩����/�ҷֿ�����������
    LRPair<int>              LastCPS = {}; // �ϴι��������棬��������ʱ����
};
FastCPSWindow FastHumanCPS;     // ����д��ʱ�����GetHumanCPSFast() ��ȡ������ CPS

inline const int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
inline const int ScreenHeigth = GetSystemMetrics(SM_CYSCREEN);
int WindowWidth = 800;
int WindowHeight = 500;

struct AppStaus
{
    LRPair<ULONGLONG> ClickerClickCount ;               //�������
    LRPair<ULONGLONG> MouseClickCount;                  //���������
    unsigned int StartCount = 0;                        //��������
    long double  UsageDuration = 0;                     //ʹ����������ʱ��(Сʱ)
} app_status;

shared_mutex InitSuccessTimeMutex;
ULONGLONG StartTime = 0;                                //GC����ʱ��ʱ��
ULONGLONG InitSuccessTime = 0;                          //��ʼ���ɹ���ʱ��

// ===== 使用时长统计（累计总时长，单位：小时）=====
// 把自上次累加以来经过的时间计入 app_status.UsageDuration。
// 在 ClickerControl 循环中定期调用；按实际增量累加，不会重复计数。
mutex UsageDurationMtx;
ULONGLONG UsageDurationLastTick = 0;
inline void FlushUsageDuration()
{
    lock_guard<mutex> lock(UsageDurationMtx);
    const ULONGLONG Now = GetTickCount64();
    if (UsageDurationLastTick == 0)   // 首次调用只记录起点
    {
        UsageDurationLastTick = Now;
        return;
    }
    if (Now > UsageDurationLastTick)
    {
        app_status.UsageDuration += (long double)(Now - UsageDurationLastTick) / 3600000.0L;   // 毫秒 -> 小时
        UsageDurationLastTick = Now;
    }
}

namespace MessageBoxMsg
{
    shared_mutex         Mtx;
    bool                 IsOpen;
    string               Message;
    string               TitleName;
    int                  ButtonType;
    map<string, UINT>    BtnDefines;
    map<string, ImVec4>  BtnColors;
    int                  ReturnCode;
};

namespace ProgressBarMsg
{
    shared_mutex Mtx;
    string       TitleName;
    int          ReturnCode = 0;
    double       MaxSize = 0.0;
    double       ReadSize = 0.0;
    bool         IsOpen = false;
    bool         IsUnzip = false;
}

shared_mutex PopTipMutex;
struct PopTipMsg
{
    string  message;
    UINT    type;
    float   timeout;
};
vector<PopTipMsg> pt_message;

inline const string ServerIP = "server.mcgc.top";
inline const int ServerPort = 25564;

inline const string ASCIITitle = R"(
   ______                        _________      __
  / ____/___ _____ ___  ___     / ____/ (_)____/ /_____  _____
 / / __/ __ `/ __ `__ \/ _ \   / /   / / / ___/ //_/ _ \/ ___/
/ /_/ / /_/ / / / / / /  __/  / /___/ / / /__/ ,< /  __/ /
\____/\__,_/_/ /_/ /_/\___/   \____/_/_/\___/_/|_|\___/_/
)";