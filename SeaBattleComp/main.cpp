#include <iostream>
#include <string>
#include <Windows.h>
#include <time.h>
#include <io.h> // Нужно для вывода Unicode
#include <fcntl.h> // Нужно для вывода Unicode
#include <cstdlib> // нужно для работы с клавиатурой
#include <Mmsystem.h>
#include <mciapi.h>
#include <fstream>
#include <cassert>
//#define DEBUG
#pragma comment (lib, "winmm.lib") // для вывода звука

using namespace std;

static const int size = 10; // размер игрового поля
int Pole_User[::size][::size]{ 0 }; // поле игрока
int Pole_Enemy[::size][::size]{ 0 }; // поле компьютера
static int step_order = 1; // порядок ходов игроков
static int difficulty; // простой или продвинутый алгоритм стрельбы противника
static int mode = 0;// режим игры: 1 - против компуктера; 2 - для двух игроков; 3 - авиация
static int player = 1; // номер текущего игрока
bool Game_Over = false; // флаг наступления конца игры
typedef int(Field)[::size][::size]; // для сохранения состояния игры
void Intro(); // вывод начальной заставки
void Menu(); // стартовое меню
void Arrangement(int); // автоматическая расстановка кораблей
void Show(); // печать состояния игровых полей
void Attack(); // ходы (стрельба)
void Brick(int type); // печать символа кирпичика
void RIP(); // печать сообщения о гибели корабля
void Hit(); // печать сообщения о подбитии корабля
bool Check_Position(int Pole[][::size], int Y, int X); // проверка пересечения с другими кораблями при стартовой расстановке
bool Check_Position(int* arr, int Y, int X); //проверка пересечения с другими кораблями, принимает указатель на массив
void Border(int[][::size]); // обводим подбитый корабль по контуру
bool Check_End_Game(int[][::size]); // проверка наступления конца игры
void Hand_of_Fate(); // играем в кости, чтобы определить очередность ходов
bool Valid_Input(char shot[]); // проверка корректности координаты выстрела игрока 
void Manual_placement(int& mode); // ручная расстановка
void Interface();

MCIERROR mciSendString(
    LPCTSTR lpszCommand,
    LPTSTR  lpszReturnString,
    UINT    cchReturn,
    HANDLE  hwndCallback
);

void Setcolor(int foregroundcolor, int backgroundcolor) //функция установки цвета символа. Первый - цвет шрифта, второй - цвет фона - по таблице от MS
{
    int finalcolor = (16 * backgroundcolor) + foregroundcolor;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), finalcolor);
}

void gotoxy(short x, short y) //функция передвижения курсора в нужное место

{
    COORD coord = { x, y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

bool saveGame(Field const& Pole_User, Field const& Pole_Enemy, int step_order, int difficulty, int mode, int player, const char* file)
{
    assert(file);
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (out)
    {
        out.write(reinterpret_cast<const char*>(&Pole_User), sizeof(Pole_User));
        out.write(reinterpret_cast<const char*>(&Pole_Enemy), sizeof(Pole_Enemy));
        out.write(reinterpret_cast<const char*>(&step_order), sizeof(step_order));
        out.write(reinterpret_cast<const char*>(&difficulty), sizeof(difficulty));
        out.write(reinterpret_cast<const char*>(&mode), sizeof(mode));
        out.write(reinterpret_cast<const char*>(&player), sizeof(player));
    }
    return static_cast<bool>(out);
}

bool loadGame(Field& Pole_User, Field& Pole_Enemy, int& step_order, int& difficulty, int& mode, int& player, const char* file)
{
    assert(file);
    std::ifstream in(file, std::ios::binary);
    if (in)
    {
        in.read(reinterpret_cast<char*>(&Pole_User), sizeof(Pole_User));
        in.read(reinterpret_cast<char*>(&Pole_Enemy), sizeof(Pole_Enemy));
        in.read(reinterpret_cast<char*>(&step_order), sizeof(step_order));
        in.read(reinterpret_cast<char*>(&difficulty), sizeof(difficulty));
        in.read(reinterpret_cast<char*>(&mode), sizeof(mode));
        in.read(reinterpret_cast<char*>(&player), sizeof(player));
    }
    return static_cast<bool>(in);
}

void Crew() { // печать сообщения о победе в игре
    _setmode(_fileno(stdout), _O_U16TEXT); // Нужно для вывода Unicode
    gotoxy(20, 15);
    wcout << L"    █████████████████████████████████████████████████████████████████████████████████████████" << endl;
    gotoxy(20, 16);
    wcout << L"   ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░██" << endl;
    gotoxy(20, 17);
    wcout << L"   ██                                                                                       ██" << endl;
    gotoxy(20, 18);
    wcout << L"   ██       ████████   █████████    ████████  ██          ██                                ██" << endl;
    gotoxy(20, 19);
    wcout << L"   ██      ██░░░░░░██  ██░░░░░░██   ██░░░░░░  ██          ██                                ██" << endl;
    gotoxy(20, 20);
    wcout << L"   ██      ██      ░░  ██      ██   ██        ██          ██         ";
    Setcolor(9, 0);
    wcout << L"██            ██       ";
    Setcolor(7, 0);
    wcout << L"██" << endl;
    gotoxy(20, 21);
    wcout << L"   ██      ██          ██      ██   ██        ██          ██         ";
    Setcolor(9, 0);
    wcout << L"██            ██       ";
    Setcolor(7, 0);
    wcout << L"██" << endl;
    gotoxy(20, 22);
    wcout << L"   ██      ██          ██     ██░   ████████  ░██        ██░     ";
    Setcolor(9, 0);
    wcout << L"██████████    ██████████   ";
    Setcolor(7, 0);
    wcout << L"██" << endl;
    gotoxy(20, 23);
    wcout << L"   ██      ██          ████████░    ██░░░░░░   ██   ██   ██      ░░░░";
    Setcolor(9, 0);
    wcout << L"██";
    Setcolor(7, 0);
    wcout << L"░░░░    ░░░░";
    Setcolor(9, 0);
    wcout << L"██";
    Setcolor(7, 0);
    wcout << L"░░░░   ██" << endl;
    gotoxy(20, 24);
    wcout << L"   ██      ██          ██░░░░░██    ██         ██   ██   ██          ";
    Setcolor(9, 0);
    wcout << L"██            ██       ";
    Setcolor(7, 0);
    wcout << L"██" << endl;
    gotoxy(20, 25);
    wcout << L"   ██      ██      ██  ██     ░██   ██         ░██ ████ ██░          ░░            ░░       ██" << endl;
    gotoxy(20, 26);
    wcout << L"   ██      ░████████░  ██      ░██  ████████    ████░░████                                  ██" << endl;
    gotoxy(20, 27);
    wcout << L"   ██       ░░░░░░░░   ░░       ░░  ░░░░░░░░    ░░░░  ░░░░                                  ██" << endl;
    gotoxy(20, 28);
    wcout << L"   ██                                                                                       ██" << endl;
    gotoxy(20, 29);
    wcout << L"   ░█████████████████████████████████████████████████████████████████████████████████████████░" << endl;
    gotoxy(20, 30);
    wcout << L"\n" << endl;
    gotoxy(20, 31);
    wcout << L"                 ██████  ██████  ██████  ██████  ██████  ██  ██  ██████  ██████" << endl;
    gotoxy(20, 32);
    wcout << L"                 ██  ██  ██  ██  ██      ██      ██      ███ ██    ██    ██" << endl;
    gotoxy(20, 33);
    wcout << L"                 ██████  ██████  ████    ██████  ████    ██████    ██    ██████" << endl;
    gotoxy(20, 34);
    wcout << L"                 ██      ██ ██   ██          ██  ██      ██ ███    ██        ██" << endl;
    gotoxy(20, 35);
    wcout << L"                 ██      ██  ██  ██████  ██████  ██████  ██  ██    ██    ██████" << endl << endl << endl;
    _setmode(_fileno(stdout), _O_TEXT); // Нужно для вывода Unicode
    Sleep(4000);
    system("cls");
}

void Intro() { // рисуем корабль белым, а звезду красным цветом
    mciSendStringA("stop Music", NULL, 0, NULL);
    PlaySound(TEXT("INTRO.wav"), NULL, SND_ASYNC); // проигрыш музыкальной заставки
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    cout << "\n";
    cout << "                          r                                                         \n";
    cout << "                     ..  iB@X                                                                                     \n";
    cout << "                     .277r;7@0                                                     \n";
    cout << "                      a@@B07aB@B@                                                                        \n";
    cout << "                         r@@B@B@B                                                  \n";
    cout << "                          X@  B                                                    \n";
    cout << "                          aB  @7                                                  \n";
    cout << "                      77ii70r.8B                                .i                 \n";
    cout << "                      7277r:7rWB2                            .r:XB2               \n";
    cout << "               i7iii:.i7rrrri:r@B@@:                       :i7a@B2                \n";
    cout << "               .2777777rrrrrri:7@B@@                    .ir7MB@r    ..7Br                                           \n";
    cout << "               .2777r7r7rrrrrr:iB@M@B.                .ir7B@M.    :70@BX           \n";
    cout << "               :7727777r7rrrrri.MBMM@BW.            :i70@Ba    .r2@@@r    :7@.\n";
    cout << "                   :i777rrrri;:.2@MMM@B@Br        :72MB@i    i7MB@2    .7MM8i          .....::iiir;rr777r7r7rriii:7:.\n";
    cout << "                       .7ii::.irXBM8MMB@@B2     :rSB@2    .7WB@S    :7ZBM7.  .:i77772722277777777777777777777777i7@B@B.\n";
    cout << "                        ::i728B@BMMMM@BB8@822;.;202.    :SB@a.   :772S27::i77277777777r7rr:";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "@2";
    SetConsoleTextAttribute(hConsole, 7);
    cout << ":irrrrrr7rrrrrrrr::2@B@B@\n";
    cout << "                     :BZMB@B@B@MMMMB@B2.XB@M::28@B2. .7XBXi   .7277rriiirrrrrrrr7rrrr;rii:r";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "B@i";
    SetConsoleTextAttribute(hConsole, 7);
    cout << ".:i:::;;rrrrrii.i8@@BMMB@\n";
    cout << "                    77M@B@BMMMMMMMM@7:..8@B8S@B@B@272MB@r..r7777r7rr;r;ri::i:i;rrrrrii::..";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "2@BM";
    SetConsoleTextAttribute(hConsole, 7);
    cout << ":ir72S7r;rrrii.7B@BBMMMBB\n";
    cout << "                    :7iB@MMMM8MMMM@S.:i:8BBB@B@B@B7727i:;r777r7rr;r;rrrirS77@2:irrr;";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "2M@@@M@BB@@B@@@2";
    SetConsoleTextAttribute(hConsole, 7);
    cout << "rirrr;:.XB@MMMMM@B@\n";
    cout << "                     i:7BM8M8M8M8MB7.;i:7@B@BMS7i::i:iirrrrr;r;riiiiiri::@B@B7:iirrri";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "72MB@BBMBB@X7";
    SetConsoleTextAttribute(hConsole, 7);
    cout << "::ir;ri::MB@MMMBM@@@\n";
    cout << "                      : 2MM8MMM8MM@S:iri:XX7i.::iir;rrrrr;riri:.::iii::7X2:.iB@W:ir;ri:..";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "W@B@B@Br";
    SetConsoleTextAttribute(hConsole, 7);
    cout << " i;rrri:i@BBMMMB@@B2\n";
    cout << "                  :8BMZ22MMM8M8MMBB@rirrii:::iir;rrr;iiri::i::7XM@B@@2.2B@:.M@B@:irrrri::";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "@B@2SB@B";
    SetConsoleTextAttribute(hConsole, 7);
    cout << "::r;ri.r@BBMBM@B@X\n";
    cout << "                 .27@B@BBMM8MMMB@B@Z7irrrir;rrri;iii::i::7@B:2@BZ27a@B::@Bi7@8@Bi:rrrrr:";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "MBX";
    SetConsoleTextAttribute(hConsole, 7);
    cout << ":..:";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "7M@";
    SetConsoleTextAttribute(hConsole, 7);
    cout << ":iri.7@@BMBB@B8\n";
    cout << "                  ri0@MMMMMBB@BM27::irrr;r;rii:::rr:0@0.:@@X.2B@...:B@7 M@X@M:@@7:;r;rir";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "Sr";
    SetConsoleTextAttribute(hConsole, 7);
    cout << ".:i; i:";
    SetConsoleTextAttribute(hConsole, 12);
    cout << "r7";
    SetConsoleTextAttribute(hConsole, 7);
    cout << ";i.7@@BM@@@S\n";
    cout << "                   ;7BBMBB@B@X7:::iiririii;i:i2W@B@.2B@:7B@..i@B7.:.0BM 0B@B7 @BW.rrr;rii:rrrrrrriii:7@B@B@B0\n";
    cout << "                  .7:BB@B@X7:::iiririi.:::.:X@BBX2;::@B@B@B7..8@8::i0@B.2@B0.:X@ai;rrrrrrr;rrrrr;ri:i@B@@@B.\n";
    cout << "                :77r:S@W2:::iir;rii.:rSB@@2 BB8 .:i:.2@@ir@BB:7B@B@B@M2.;7i.:..::irrr;rrrrrrrrrrr;::@B@B@7\n";
    cout << "               727rrir7::iiri;::::7WB@BZS@B7i@@7.:::.:@@2 :8@B:r227;:::i::27;B2::rrrrrrrrrrrrr;r;::BB@B@.\n";
    cout << "              .27rrrriii;i;:::r77.iB@8   S@B.X@B::28@72B0:::i:i:::::::;i::BB@B7:iirrrrrrrrrrr;rii.ZB@BM\n";
    cout << "               77r;rrrirri:;S@B@B0 ;B@27M@BX.iB@B@B@07:::iiiiii:.:.:ii::7X2:.:M@M:irrrrrrrrrrrrr.2B@B2\n";
    cout << "              .77rr;ri::iiMB@077@BS 2@@@07i:i:i227i.::i::.::ii:7aM@B@Ba.7@@: 8@B@:irr;rrrrrrrrr:rB@B7\n";
    cout << "            .777rrrrii2S.rB@7 ..7@B7 MB@.:iri;::::iii::r7XM@r:2@@8S7S@Bi.@@7r@8@Br:rrrrrrrrrrr::@@Br\n";
    cout << "          .7277;ii;i:M@B2 2B@r.: 7@Bi:@M7:rrrrr;rrri7M@B@BZ2:.2B@:..:B@2 M@0@B:@@2:irrrrrrrrri.S@Br\n";
    cout << "        .727rri:i:.:.W@@B2 XB@i:7BB@;:::irrrrrrrrrri:@B8.:r22r.@B2.:.XBB aB@B7 @@M.rrrrrrrrr;:r@@7\n";
    cout << "      .r277rri7M@M2i 2@0@B2 WB@B@M7:ii;ir;rrrrr;rrr;:7@BB@@B@B:a@M::ia@@:7@BM..a@0iirrrrrrr;i.@B2\n";
    cout << "     r27r7rr;ii@B@B@8ZBSi@@0.2a2:::i;rrrrrrrrrrrrrrri:B@W...B@XiB@B@B@B2:i77iiii::irrr;rrrrr:2BW\n";
    cout << "   .277rrrrrr;i.0B@20B@a.7@Wi.::iirrrrrrrrr;r;rrrrrrr.2B@0@B@B7.r227;:::ii::iiri;i;;riririri:B@\n";
    cout << "   277rrrrrrrrii.0B@::r:i:::irirrr;rrrrrrr;r;rrrrrrrii:@MZ27i::i::.::::iii::::::::::::.:.:..2@\n";
    cout << "  r77rrrrrrrrrrii.0@@r:iriiirir;r;ririri;iiiiii:i:i:::... ......:.::::::iii;rr77772222XaX08M@.\n";
    cout << "  27rrrr;riririri:.S2i:i:i::::::::::.:.::::::i:ii;;rr77772222XX0ZM8BM@B@@@@@B@B@B@B@B@B@B@B@7\n";
    cout << " .7ii:::i::::::::::.::rrr7772722SSXX88MM@B@B@B@@@@@B@B@B@@@@@@@@@B@@@B@B@@@@@@@@@@@B@B@B@B@8\n";
    cout << "\n\t\t\t\t\t\t\t\t\tCreator:Максим Родионов © 2021\n\n";
    for (int i = 0; i < 11; i++) {
        Sleep(1000);
        if (i == 8) {
            Sleep(200);
            Setcolor(14, 0);
            gotoxy(65, 5);
            cout << "'.\\|/.'";
            gotoxy(65, 6);
            cout << "(\\   /)";
            gotoxy(65, 7);
            cout << "- -O- -";
            gotoxy(65, 8);
            cout << "(/   \\)";
            gotoxy(65, 9);
            cout << ",'/|\\'.";
            Sleep(100);
            Setcolor(12, 0);
            gotoxy(70, 8);
            cout << "'.\\|/.'";
            gotoxy(70, 9);
            cout << "(\\   /)";
            gotoxy(70, 10);
            cout << "- -O- -";
            gotoxy(70, 11);
            cout << "(/   \\)";
            gotoxy(70, 12);
            cout << ",'/|\\'.";
        }
        if (GetAsyncKeyState(VK_ESCAPE)) {
            return;
        }
    }
}

void ClearSprite() {
    for (int i = 20; i < 32; i++) {
        gotoxy(5, i);
        wcout << L"                                                                                             ";
    }
}

void RIP() { // печать сообщения о потоплении корабля
    PlaySound(TEXT("RIP.wav"), NULL, SND_ASYNC);
    Setcolor(4, 0);
    _setmode(_fileno(stdout), _O_U16TEXT); // Нужно для вывода Unicode
    for (int i = 0; i < 4; i++) {
        gotoxy(32, 20);
        wcout << L"     █████████    ██  █████████   ██";
        gotoxy(32, 21);
        wcout << L"     ██░░░░░░██   ░░  ██░░░░░░██  ██";
        gotoxy(32, 22);
        wcout << L"     ██      ██   ██  ██      ██  ██";
        gotoxy(32, 23);
        wcout << L"     ██      ██   ██  ██      ██  ██";
        gotoxy(32, 24);
        wcout << L"     ██     ██░   ██  ██      ██  ██";
        gotoxy(32, 25);
        wcout << L"     ████████░    ██  █████████░  ██";
        gotoxy(32, 26);
        wcout << L"     ██░░░░░██    ██  ██░░░░░░░   ██";
        gotoxy(32, 27);
        wcout << L"     ██     ░██   ██  ██          ░░";
        gotoxy(32, 28);
        wcout << L"     ██      ░██  ██  ██          ██";
        gotoxy(32, 29);
        wcout << L"     ░░       ░░  ░░  ░░          ░░";
        Sleep(500);
        ClearSprite();
        Sleep(500);
    }
    _setmode(_fileno(stdout), _O_TEXT); // Нужно для вывода Unicode
    Setcolor(7, 0);
}

void Hit() { // печать сообщения о ранении корабля
    PlaySound(TEXT("HIT.wav"), NULL, SND_ASYNC);
    Setcolor(6, 0);
    _setmode(_fileno(stdout), _O_U16TEXT); // Нужно для вывода Unicode
    for (int i = 0; i < 3; i++) {
        gotoxy(32, 20);
        wcout << L"     ██      ██  ██  ██████████  ██";
        gotoxy(32, 21);
        wcout << L"     ██      ██  ░░  ░░░░██░░░░  ██";
        gotoxy(32, 22);
        wcout << L"     ██      ██  ██      ██      ██";
        gotoxy(32, 23);
        wcout << L"     ██      ██  ██      ██      ██";
        gotoxy(32, 24);
        wcout << L"     ██████████  ██      ██      ██";
        gotoxy(32, 25);
        wcout << L"     ██░░░░░░██  ██      ██      ██";
        gotoxy(32, 26);
        wcout << L"     ██      ██  ██      ██      ██";
        gotoxy(32, 27);
        wcout << L"     ██      ██  ██      ██      ░░";
        gotoxy(32, 28);
        wcout << L"     ██      ██  ██      ██      ██";
        gotoxy(32, 29);
        wcout << L"     ░░      ░░  ░░      ░░      ░░";
        Sleep(500);
        ClearSprite();
        Sleep(500);
    }
    _setmode(_fileno(stdout), _O_TEXT); // Нужно для вывода Unicode
    Setcolor(7, 0);

}

void Miss() { // печать сообщения о промахе
    PlaySound(TEXT("MISS.wav"), NULL, SND_ASYNC);
    Setcolor(9, 0);
    _setmode(_fileno(stdout), _O_U16TEXT); // Нужно для вывода Unicode
    gotoxy(27, 20);
    wcout << L"     ███     ███  ██   ████████    ████████   ██";
    gotoxy(27, 21);
    wcout << L"     ████   ████  ░░  ██░░░░░░██  ██░░░░░░██  ██";
    gotoxy(27, 22);
    wcout << L"     ██░██ ██░██  ██  ██      ░░  ██      ░░  ██";
    gotoxy(27, 23);
    wcout << L"     ██ ░███░ ██  ██  ██          ██          ██";
    gotoxy(27, 24);
    wcout << L"     ██  ░█░  ██  ██  ░████████   ░████████   ██";
    gotoxy(27, 25);
    wcout << L"     ██  ░░░  ██  ██   ░░░░░░░██   ░░░░░░░██  ██";
    gotoxy(27, 26);
    wcout << L"     ██       ██  ██          ██          ██  ██";
    gotoxy(27, 27);
    wcout << L"     ██       ██  ██  ██      ██  ██      ██  ░░";
    gotoxy(27, 28);
    wcout << L"     ██       ██  ██  ░████████░  ░████████░  ██";
    gotoxy(27, 29);
    wcout << L"     ░░       ░░  ░░   ░░░░░░░░    ░░░░░░░░   ░░";
    Sleep(1100);
    ClearSprite();
    _setmode(_fileno(stdout), _O_TEXT); // Нужно для вывода Unicode
    Setcolor(7, 0);
}

void Defeat() { // печать сообщения о проигранной игре
    Setcolor(4, 0);
    _setmode(_fileno(stdout), _O_U16TEXT); // Нужно для вывода Unicode
    mciSendStringA("stop Music", NULL, 0, NULL);
    PlaySound(TEXT("DEFEAT.wav"), NULL, SND_ASYNC);
    gotoxy(37, 14);
    wcout << L"НАЖМИТЕ ESC ДЛЯ ВЫХОДА В МЕНЮ";
    for (int i = 0; i < 5; i++) {
        if (GetAsyncKeyState(VK_ESCAPE)) {
            _setmode(_fileno(stdout), _O_TEXT);
            Setcolor(7, 0);
            system("cls");
            Intro();
            Menu();
        }
        ClearSprite();
        Sleep(500);
        gotoxy(15, 20);
        wcout << L"     ██████████   ████████  ████████  ████████      ██      ██████████  ██";
        gotoxy(15, 21);
        wcout << L"     ██░░░░░░░██  ██░░░░░░  ██░░░░░░  ██░░░░░░     ████     ░░░░██░░░░  ██";
        gotoxy(15, 22);
        wcout << L"     ██       ██  ██        ██        ██          ██░░██        ██      ██";
        gotoxy(15, 23);
        wcout << L"     ██       ██  ██        ██        ██         ██░  ░██       ██      ██";
        gotoxy(15, 24);
        wcout << L"     ██       ██  ████████  █████     ████████   ██    ██       ██      ██";
        gotoxy(15, 25);
        wcout << L"     ██       ██  ██░░░░░░  ██░░░     ██░░░░░░   ████████       ██      ██";
        gotoxy(15, 26);
        wcout << L"     ██       ██  ██        ██        ██        ██░░░░░░██      ██      ██";
        gotoxy(15, 27);
        wcout << L"     ██       ██  ██        ██        ██        ██      ██      ██      ░░";
        gotoxy(15, 28);
        wcout << L"     ██████████░  ████████  ██        ████████  ██      ██      ██      ██";
        gotoxy(15, 29);
        wcout << L"     ░░░░░░░░░░   ░░░░░░░░  ░░        ░░░░░░░░  ░░      ░░      ░░      ░░";
        Sleep(500);
    }
    _setmode(_fileno(stdout), _O_TEXT); // Нужно для вывода Unicode
    Setcolor(7, 0);
    gotoxy(37, 14);
    system("Pause");
    system("cls");
    Intro();
    Menu();
}

void Victory() { // печать сообщения о победе в игре

    _setmode(_fileno(stdout), _O_U16TEXT); // Нужно для вывода Unicode
    mciSendStringA("stop Music", NULL, 0, NULL);
    PlaySound(TEXT("VICTORY.wav"), NULL, SND_ASYNC);
    gotoxy(37, 14);
    wcout << L"НАЖМИТЕ ESC ДЛЯ ВЫХОДА В МЕНЮ";
    for (int i = 0, j = 1; i < 62; i++, j++) {
        if (GetAsyncKeyState(VK_ESCAPE)) {
            _setmode(_fileno(stdout), _O_TEXT);
            Setcolor(7, 0);
            system("cls");
            Intro();
            Menu();
        }
        if (j == 16) {
            j = 1;
        }
        Setcolor(j, 0);
        if (i == 61) {
            j = 14;
            Setcolor(j, 0);
        }
        gotoxy(13, 20);
        wcout << L" ██      ██  ██   ████████   ██████████   ████████   █████████    ██      ██  ██";
        gotoxy(13, 21);
        wcout << L" ██      ██  ░░  ██░░░░░░██  ░░░░██░░░░  ██░░░░░░██  ██░░░░░░██   ██      ██  ██";
        gotoxy(13, 22);
        wcout << L" ██      ██  ██  ██      ░░      ██      ██      ██  ██      ██   ░██    ██░  ██";
        gotoxy(13, 23);
        wcout << L" ░██    ██░  ██  ██              ██      ██      ██  ██      ██    ██    ██   ██";
        gotoxy(13, 24);
        wcout << L"  ██    ██   ██  ██              ██      ██      ██  ██     ██░    ░██  ██░   ██";
        gotoxy(13, 25);
        wcout << L"  ██    ██   ██  ██              ██      ██      ██  ████████░      ░░██░░    ██";
        gotoxy(13, 26);
        wcout << L"  ░██  ██░   ██  ██              ██      ██      ██  ██░░░░░██        ██      ██";
        gotoxy(13, 27);
        wcout << L"   ░████░    ██  ██      ██      ██      ██      ██  ██     ░██       ██      ░░";
        gotoxy(13, 28);
        wcout << L"     ██      ██  ░████████░      ██      ░████████░  ██      ░██      ██      ██";
        gotoxy(13, 29);
        wcout << L"     ░░      ░░   ░░░░░░░░       ░░       ░░░░░░░░   ░░       ░░      ░░      ░░";
        Sleep(407);
    }
    Setcolor(7, 0);
    gotoxy(37, 14);
    system("Pause");
    system("cls");
    _setmode(_fileno(stdout), _O_TEXT); // Нужно для вывода Unicode
    Intro();
    Menu();


}

void Brick(int type) { // печать символа кирпичика
    _setmode(_fileno(stdout), _O_U16TEXT); // Нужно для вывода Unicode
    if (type == 1) wcout << L" █"; // печатаем кирпичик
    if (type == 2) wcout << L" ░";
    if (type == 3) wcout << L"█";
    if (type == 4) {
        Setcolor(4, 0);
        wcout << L" ░";
        Setcolor(7, 0);
    }
    _setmode(_fileno(stdout), _O_TEXT); // Нужно для вывода Unicode
}

bool Check_End_Game(int Pole[][::size]) { // проверка остались ли еще корабли на проверяемом поле
    for (int i = 0; i < ::size; i++) {
        for (int j = 0; j < ::size; j++) {
            if (Pole[i][j] >= 1 && Pole[i][j] <= 4) {
                return false; // если еще оставлись непотопленные корабли
            }
        }
    }
    return true; // если все корабли потоплены
}

void Border(int Pole[][::size]) { // обводим подбитый корабль по контуру
    for (int i = 0; i < ::size; i++) { // сканируем поле
        for (int j = 0; j < ::size; j++) {
            if (Pole[i][j] == 7) { // если нашли подбитую часть корабля - проверяем есть ли рядом пустые клетки. Если есть - помечаем как простреленные
                if (Pole[i - 1][j] == 0 && (i - 1) >= 0) Pole[i - 1][j] = 5;
                if (Pole[i - 1][j + 1] == 0 && (i - 1) >= 0 && (j + 1) <= 9) Pole[i - 1][j + 1] = 5;
                if (Pole[i][j + 1] == 0 && (j + 1) <= 9) Pole[i][j + 1] = 5;
                if (Pole[i + 1][j + 1] == 0 && (i + 1) <= 9 && (j + 1) <= 9) Pole[i + 1][j + 1] = 5;
                if (Pole[i + 1][j] == 0 && (i + 1) <= 9) Pole[i + 1][j] = 5;
                if (Pole[i + 1][j - 1] == 0 && (i + 1) <= 9 && (j - 1) >= 0) Pole[i + 1][j - 1] = 5;
                if (Pole[i][j - 1] == 0 && (j - 1) >= 0) Pole[i][j - 1] = 5;
                if (Pole[i - 1][j - 1] == 0 && (i - 1) >= 0 && (j - 1) >= 0) Pole[i - 1][j - 1] = 5;
            }
        }
    }
}

bool Valid_Input(char shot[]) { // проверка корректности координаты выстрела игрока
    if ((tolower(shot[0])) >= 'a' && (tolower(shot[0])) <= 'j') {
        if (shot[1] >= '1' && shot[1] <= '9') {
            if (!shot[2] || shot[2] == '0') {
                return true;
            }
        }
    }
    if ((tolower(shot[0])) == 'a' && (tolower(shot[1])) == 'i' && (tolower(shot[2])) == 'r') return true;
    if ((tolower(shot[0])) == 's' && (tolower(shot[1])) == 'a' && (tolower(shot[2])) == 'v' && (tolower(shot[3])) == 'e') {
        return true;
    }
    if ((tolower(shot[0])) == 'q') {
        system("cls");
        Intro();
        Menu();
    }
    if ((tolower(shot[0])) == 'w' && (tolower(shot[1])) == 'i' && (tolower(shot[2])) == 'n' && shot[3] == '1') {
        gotoxy(37, 12);
        Setcolor(12, 0);
        cout << "ОООО! ЧИТЫ! НУ ВСЁ!";
        gotoxy(37, 13);
        cout << "Я ТАК БОЛЬШЕ С ВАМИ НЕ ИГРАЮ";
        Setcolor(7, 0);
        Sleep(4000);
        for (int i = 0; i < ::size; i++) {
            for (int j = 0; j < ::size; j++) {
                Pole_Enemy[i][j] = 0;
            }
        }
        Show();
        return true;
    }
    if ((tolower(shot[0])) == 'w' && (tolower(shot[1])) == 'i' && (tolower(shot[2])) == 'n' && shot[3] == '2') {
        for (int i = 0; i < ::size; i++) {
            for (int j = 0; j < ::size; j++) {
                Pole_User[i][j] = 0;
            }
        }
        Show();
        return true;
    }
    return false;

}


bool Torpedo_Valid_Input(char shot[]) { // проверка корректности координаты выстрела игрока
    if ((tolower(shot[0])) >= 'a' && (tolower(shot[0])) <= 'j') return true;
    else return false;
}

void Torpedo(int Pole[][::size]) {
    char shot[2]; // для записи координаты выстрела от пользователя
    int Y = 0, X = 0, crossfire = 0; // crossfire - случайное направление стрельбы противником вокруг нашего раненого корабля
    bool four_deck_defeated = NULL, alert = NULL; // объявляем флаг потопления четырёхпалубного корабля. alert - true если есть наш раненый корабль
    int counter = 0; // для продвижения супер-выстрелом по строке 

    Interface();
    cout << "ВЫБЕРИТЕ РЯД ДЛЯ";
    gotoxy(37, 10);
    cout << "ВЫЗОВА АВИАЦИИ: ";
    cin >> shot;

    while (Torpedo_Valid_Input(shot) == false) { // проверяем ввод пользователя 
        gotoxy(37, 11);
        cout << "НЕКОРРЕКТНЫЙ ВВОД.";
        gotoxy(37, 12);
        cout << "ПОВТОРИТЕ ПОПЫТКУ:     \b\b\b\b\b ";
        cin.clear(); // очищаем буфер
        cin.ignore(256, '\n');
        cin >> shot;

    }
    PlaySound(TEXT("PLANE.wav"), NULL, SND_SYNC);
    Y = (int)(tolower(shot[0])) - 97;

    while (counter < 10) { // 
        PlaySound(TEXT("AIRBOMB.wav"), NULL, SND_ASYNC);
        X = counter;
        for (int i = 0; i < 3; i++) {
            cout << " .";
            Sleep(200);
        }
        Show();
        // ЕСЛИ МЫ ПОПАЛИ МИМО ЦЕЛИ
        if (Pole[Y][X] == 0) { // если клетка незанята - помечаем ее как простреленную
            Pole[Y][X] = 5;
            step_order++; // выходим из цикла - дальше будем ходить мы
            Show();
            Miss();
        }
        // ТОПИМ 1-ПАЛУБНЫЙ КОРАБЛЬ
        if (Pole[Y][X] == 1) {
            Pole[Y][X] = 7;
            RIP();
            Show();
        }
        // ПРОВЕРКА ПОТОПЛЕНИЯ 2-ПАЛУБНОГО КОРАБЛЯ
        if (Pole[Y][X] == 2) { // если попали в 2-палубный корабль
            // сначала проверяем крестом, что это уже вторая пораженная клетка и помечаем ее убитой
            if (Pole[Y][X + 1] == 6) {
                Pole[Y][X] = 7;
                Pole[Y][X + 1] = 7;
                RIP();
                Show();
            }
            if (Pole[Y][X - 1] == 6) {
                Pole[Y][X] = 7;
                Pole[Y][X - 1] = 7;
                RIP();
                Show();
            }
            if (Pole[Y + 1][X] == 6) {
                Pole[Y][X] = 7;
                Pole[Y + 1][X] = 7;
                RIP();
                Show();
            }
            if (Pole[Y - 1][X] == 6) {
                Pole[Y][X] = 7;
                Pole[Y - 1][X] = 7;
                RIP();
                Show();
            }
            if (Pole[Y - 1][X] == 2 || Pole[Y][X + 1] == 2 || // если только ранили (т.е. по периметру есть еще значение 2) - отмечаем и ходим снова
                Pole[Y + 1][X] == 2 || Pole[Y][X - 1] == 2) {
                Pole[Y][X] = 6;
                Hit();
                Show();
            }
        }
        // ПРОВЕРКА ПОТОПЛЕНИЯ 3-ПАЛУБНОГО КОРАБЛЯ
        if (Pole[Y][X] == 3) { // если только первый раз попали в 3-палубный корабль
            if (Pole[Y - 1][X] == 3 || Pole[Y][X + 1] == 3 ||
                Pole[Y + 1][X] == 3 || Pole[Y][X - 1] == 3) {
                Pole[Y][X] = 6;
                Hit();
                Show();
            }
            // если попали в центр 3-палубного и по краям он уже ранен - проверяем позиции сверху и снизу
            if (Pole[Y - 1][X] == 6 && Pole[Y + 1][X] == 6) {
                Pole[Y][X] = 7;
                Pole[Y - 1][X] = 7;
                Pole[Y + 1][X] = 7;
                RIP();
                Show();
            }
            // проверяем позиции справа и слева
            if (Pole[Y][X + 1] == 6 && Pole[Y][X - 1] == 6) {
                Pole[Y][X] = 7;
                Pole[Y][X + 1] = 7;
                Pole[Y][X - 1] = 7;
                RIP();
                Show();
            }
            // проверка 3-палубного от края к центру - когда переместились в центр проверяем есть ли убитые слева и справа
            if (Pole[Y - 1][X] == 6) {
                Pole[Y][X] = 6;
                Y = Y - 1;
                if (Pole[Y - 1][X] == 6) {
                    Pole[Y][X] = 7;
                    Pole[Y - 1][X] = 7;
                    Pole[Y + 1][X] = 7;
                    RIP();
                    Show();
                }
                else {
                    Hit();
                    Show();
                }
            }
            if (Pole[Y][X + 1] == 6) {
                Pole[Y][X] = 6;
                X = X + 1;
                if (Pole[Y][X + 1] == 6) {
                    Pole[Y][X] = 7;
                    Pole[Y][X + 1] = 7;
                    Pole[Y][X - 1] = 7;
                    RIP();
                    Show();
                }
                else {
                    Hit();
                    Show();
                }
            }
            if (Pole[Y + 1][X] == 6) {
                Pole[Y][X] = 6;
                Y = Y + 1;
                if (Pole[Y + 1][X] == 6) {
                    Pole[Y][X] = 7;
                    Pole[Y + 1][X] = 7;
                    Pole[Y - 1][X] = 7;
                    RIP();
                    Show();
                }
                else {
                    Hit();
                    Show();
                }
            }
            if (Pole[Y][X - 1] == 6) {
                Pole[Y][X] = 6;
                X = X - 1;
                if (Pole[Y][X - 1] == 6) {
                    Pole[Y][X] = 7;
                    Pole[Y][X - 1] = 7;
                    Pole[Y][X + 1] = 7;
                    RIP();
                    Show();
                }
                else {
                    Hit();
                    Show();
                }
            }
        }
        // ПРОВЕРКА ПОТОПЛЕНИЯ 4-ПАЛУБНОГО КОРАБЛЯ
        if (Pole[Y][X] == 4) {
            Pole[Y][X] = 8;
            four_deck_defeated = true;

            for (int i = 0; i < ::size; i++) {
                for (int j = 0; j < ::size; j++) {
                    if (Pole[i][j] == 4) four_deck_defeated = false;
                }
            }
            if (!four_deck_defeated) {
                Hit();
                Show();
            }
            if (four_deck_defeated) {
                for (int i = 0; i < ::size; i++) {
                    for (int j = 0; j < ::size; j++) {
                        if (Pole[i][j] == 8) Pole[i][j] = 7;
                    }
                }
                RIP();
                Show();
            }
        }
        counter++;
    }
}
void Interface() {
    Setcolor(12, 0);
    gotoxy(35, 3);
    _setmode(_fileno(stdout), _O_U16TEXT);
    wcout << L"███████████████████████████████████ ";
    gotoxy(35, 4);
    wcout << L"█ ДОСТУПНЫЕ В ХОДЕ РАУНДА КОМАНДЫ:█░";
    gotoxy(35, 5);
    wcout << L"█";
    Setcolor(7, 0);
    wcout << L"'Q' - ВЫХОД | 'SAVE' - СОХРАНЕНИЕ";
    Setcolor(12, 0);
    wcout << L"█░";
    gotoxy(35, 6);
    wcout << L"█";
    Setcolor(7, 0);
    wcout << L"    'AIR' - ВЫЗОВ АВИАУДАРА      ";
    Setcolor(12, 0);
    wcout << L"█░";
    gotoxy(35, 7);
    wcout << L"███████████████████████████████████░";
    gotoxy(35, 8);
    wcout << L"█           ХОД ИГРЫ:             █░";
    gotoxy(35, 9);
    wcout << L"█                                 █░";
    gotoxy(35, 10);
    wcout << L"█                                 █░";
    gotoxy(35, 11);
    wcout << L"█                                 █░";
    gotoxy(35, 12);
    wcout << L"█                                 █░";
    gotoxy(35, 13);
    wcout << L"█                                 █░";
    gotoxy(35, 14);
    wcout << L"█                                 █░";
    gotoxy(35, 15);
    wcout << L"███████████████████████████████████░";
    gotoxy(35, 16);
    wcout << L" ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░";
    //gotoxy(36, 4);
    //wcout << L" ДОСТУПНЫЕ ДЛЯ ВВОДА КОМАНДЫ: ";
    _setmode(_fileno(stdout), _O_TEXT);
    /*
    _setmode(_fileno(stdout), _O_TEXT);
    cout << "   ДОСТУПНЫЕ ДЛЯ ВВОДА КОМАНДЫ:";
    _setmode(_fileno(stdout), _O_U16TEXT);
    gotoxy(35, 6);
    _setmode(_fileno(stdout), _O_TEXT);
    cout << " \"Q\" - ВЫХОД       \"AIR\" - АВИАЦИЯ";
    _setmode(_fileno(stdout), _O_U16TEXT);
    */
    gotoxy(37, 9);
    Setcolor(7, 0);
}

void Attack() { // функция выстрела (хода)
    PlaySound(NULL, 0, 0);
    mciSendStringA("open .\\INGAME.mp3 alias Music", NULL, 0, 0);
    mciSendStringA("play Music repeat", 0, 0, 0);
    char shot[6]; // для записи координаты выстрела от пользователя
    int Y = 0, X = 0, crossfire = 0; // crossfire - случайное направление стрельбы противником вокруг нашего раненого корабля
    bool four_deck_defeated = NULL, alert = NULL; // объявляем флаг потопления четырёхпалубного корабля. alert - true если есть наш раненый корабль
    int random_counter = 0; // чтобы не ушел в веный цикл, когда компьютер перебирает варианты ходов при добитии раненого корабля
    int temp_Y, temp_X; // для временного хранения координаты ранения для дальнейшего добивания цели
    int attempt = 1; // если == 1 - то это первая попытка соперника добить раненую цель и направление выстрела будет выбрано на угад. Если > 1 - то добивание будет происходить после опредения в каком направлении есть соседняя раненая клетка

    while (!Game_Over) { // цикл ходов
        while (step_order % 2 == 1) { // Ход игрока 1
            player = 1;
            if (mode == 2) {
                system("cls");
                Interface();
                cout << "ХОД ПЕРВОГО ИГРОКА.";
                gotoxy(37, 10);
                cout << "ВТОРОГО ИГРОКА ПРОСИМ";
                gotoxy(37, 11);
                cout << "ОТВЕРНУТЬСЯ.";
                for (int i = 0; i < 3; i++) { // задержка во время хода второго игрока
                    cout << " .";
                    Sleep(500);
                }
                gotoxy(37, 12);
                system("Pause");
                Show();
            }
            system("cls");
            Show();
            Interface();
            cout << "ВВЕДИТЕ КООРДИНАТУ ДЛЯ ЗАЛПА:";
            gotoxy(37, 10);
            cin >> shot;
            gotoxy(37, 10);
            while (Valid_Input(shot) == false) { // проверяем ввод пользователя 
                gotoxy(37, 11);
                cout << "НЕКОРРЕКТНЫЙ ВВОД.";
                gotoxy(37, 12);
                cout << "ПОВТОРИТЕ ПОПЫТКУ:     \b\b\b\b\b ";
                cin.clear(); // очищаем буфер
                cin.ignore(256, '\n');
                cin >> shot;
            }
            if ((tolower(shot[0])) == 'a' && (tolower(shot[1])) == 'i' && (tolower(shot[2])) == 'r') {
                Torpedo(Pole_Enemy);
                step_order = 2;
            }
            if ((tolower(shot[0])) == 's' && (tolower(shot[1])) == 'a' && (tolower(shot[2])) == 'v' && (tolower(shot[3])) == 'e') {
                saveGame(Pole_User, Pole_Enemy, ::step_order, ::difficulty, ::mode, ::player, "SAVE.sav");
                gotoxy(37, 10);
                cout << "ИДЕТ СОХРАНЕНИЕ...";
                Sleep(2000);
                Attack();
            }
            if (step_order == 2) break;
            else {
                PlaySound(TEXT("ATTACK.wav"), NULL, SND_SYNC);
                Y = (int)(tolower(shot[0])) - 97; // записываем в Y выбор строки (преобразовываем в нижний регистр)
                (shot[1] == '1' && shot[2] == '0') ? X = 9 : X = (int)shot[1] - 49; // если после буквы ввели 10 - обозначаем, что это последняя ячейка строки. В противном случае - конвертируем введенный символ в соответствующее число

                // ЕСЛИ НЕЛОГИЧНЫЙ ХОД
                if (Pole_Enemy[Y][X] >= 5 && Pole_Enemy[Y][X] <= 8) { // если ход нелогичный (если уже ходили, уже ранен или уже убит)
                    gotoxy(37, 13);
                    cout << "НЕЛОГИЧНЫЙ ХОД.";
                    gotoxy(37, 14);
                    cout << "ПОВТОРИТЕ ПОПЫТКУ";
                    Sleep(3000);
                    Show();
                    Attack(); // и ходим снова;
                }
                // ЕСЛИ МЫ ПОПАЛИ МИМО ЦЕЛИ
                if (Pole_Enemy[Y][X] == 0) { // если клетка незанята - помечаем ее как простреленную
                    Pole_Enemy[Y][X] = 5;
                    step_order++; // выходим из цикла - дальше будем ходить мы
                    Show();
                    Miss();
                }
                // ТОПИМ 1-ПАЛУБНЫЙ КОРАБЛЬ
                if (Pole_Enemy[Y][X] == 1) {
                    Pole_Enemy[Y][X] = 7;
                    Show();
                    RIP();
                    Attack(); // и ходим снова;
                }
                // ПРОВЕРКА ПОТОПЛЕНИЯ 2-ПАЛУБНОГО КОРАБЛЯ
                if (Pole_Enemy[Y][X] == 2) { // если попали в 2-палубный корабль
                    // сначала проверяем крестом, что это уже вторая пораженная клетка и помечаем ее убитой
                    if (Pole_Enemy[Y][X + 1] == 6) {
                        Pole_Enemy[Y][X] = 7;
                        Pole_Enemy[Y][X + 1] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    if (Pole_Enemy[Y][X - 1] == 6) {
                        Pole_Enemy[Y][X] = 7;
                        Pole_Enemy[Y][X - 1] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    if (Pole_Enemy[Y + 1][X] == 6) {
                        Pole_Enemy[Y][X] = 7;
                        Pole_Enemy[Y + 1][X] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    if (Pole_Enemy[Y - 1][X] == 6) {
                        Pole_Enemy[Y][X] = 7;
                        Pole_Enemy[Y - 1][X] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    if (Pole_Enemy[Y - 1][X] == 2 || Pole_Enemy[Y][X + 1] == 2 || // если только ранили (т.е. по периметру есть еще значение 2) - отмечаем и ходим снова
                        Pole_Enemy[Y + 1][X] == 2 || Pole_Enemy[Y][X - 1] == 2) {
                        Pole_Enemy[Y][X] = 6;
                        Show();
                        Hit();
                        Attack(); // и ходим снова
                    }
                }
                // ПРОВЕРКА ПОТОПЛЕНИЯ 3-ПАЛУБНОГО КОРАБЛЯ
                if (Pole_Enemy[Y][X] == 3) { // если только первый раз попали в 3-палубный корабль
                    if (Pole_Enemy[Y - 1][X] == 3 || Pole_Enemy[Y][X + 1] == 3 ||
                        Pole_Enemy[Y + 1][X] == 3 || Pole_Enemy[Y][X - 1] == 3) {
                        Pole_Enemy[Y][X] = 6;
                        Show();
                        Hit();
                        Attack(); // и ходим снова
                    }
                    // если попали в центр 3-палубного и по краям он уже ранен - проверяем позиции сверху и снизу
                    if (Pole_Enemy[Y - 1][X] == 6 && Pole_Enemy[Y + 1][X] == 6) {
                        Pole_Enemy[Y][X] = 7;
                        Pole_Enemy[Y - 1][X] = 7;
                        Pole_Enemy[Y + 1][X] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    // проверяем позиции справа и слева
                    if (Pole_Enemy[Y][X + 1] == 6 && Pole_Enemy[Y][X - 1] == 6) {
                        Pole_Enemy[Y][X] = 7;
                        Pole_Enemy[Y][X + 1] = 7;
                        Pole_Enemy[Y][X - 1] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    // проверка 3-палубного от края к центру - когда переместились в центр проверяем есть ли убитые слева и справа
                    if (Pole_Enemy[Y - 1][X] == 6) {
                        Pole_Enemy[Y][X] = 6;
                        Y = Y - 1;
                        if (Pole_Enemy[Y - 1][X] == 6) {
                            Pole_Enemy[Y][X] = 7;
                            Pole_Enemy[Y - 1][X] = 7;
                            Pole_Enemy[Y + 1][X] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                    if (Pole_Enemy[Y][X + 1] == 6) {
                        Pole_Enemy[Y][X] = 6;
                        X = X + 1;
                        if (Pole_Enemy[Y][X + 1] == 6) {
                            Pole_Enemy[Y][X] = 7;
                            Pole_Enemy[Y][X + 1] = 7;
                            Pole_Enemy[Y][X - 1] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                    if (Pole_Enemy[Y + 1][X] == 6) {
                        Pole_Enemy[Y][X] = 6;
                        Y = Y + 1;
                        if (Pole_Enemy[Y + 1][X] == 6) {
                            Pole_Enemy[Y][X] = 7;
                            Pole_Enemy[Y + 1][X] = 7;
                            Pole_Enemy[Y - 1][X] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                    if (Pole_Enemy[Y][X - 1] == 6) {
                        Pole_Enemy[Y][X] = 6;
                        X = X - 1;
                        if (Pole_Enemy[Y][X - 1] == 6) {
                            Pole_Enemy[Y][X] = 7;
                            Pole_Enemy[Y][X - 1] = 7;
                            Pole_Enemy[Y][X + 1] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                }
                // ПРОВЕРКА ПОТОПЛЕНИЯ 4-ПАЛУБНОГО КОРАБЛЯ
                if (Pole_Enemy[Y][X] == 4) {
                    Pole_Enemy[Y][X] = 8;
                    four_deck_defeated = true;

                    for (int i = 0; i < ::size; i++) {
                        for (int j = 0; j < ::size; j++) {
                            if (Pole_Enemy[i][j] == 4) four_deck_defeated = false;
                        }
                    }
                    if (!four_deck_defeated) {
                        Show();
                        Hit();
                        Attack(); // и ходим снова
                    }
                    if (four_deck_defeated) {
                        for (int i = 0; i < ::size; i++) {
                            for (int j = 0; j < ::size; j++) {
                                if (Pole_Enemy[i][j] == 8) Pole_Enemy[i][j] = 7;
                            }
                        }
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                }
            }
        }
        while (step_order % 2 != 1) { // ход соперника
            if (mode == 2) {
                system("cls");
                Interface();
                cout << "ХОД ВТОРОГО ИГРОКА.";
                gotoxy(37, 10);
                cout << "ПЕРВОГО ПРОСИМ";
                gotoxy(37, 11);
                cout << "ОТВЕРНУТЬСЯ.";
                ::player = 2;
                for (int i = 0; i < 3; i++) { // задержка во время хода второго игрока
                    cout << " .";
                    Sleep(500);
                }
                gotoxy(37, 12);
                system("Pause");
                Show();
                cout << "ВВЕДИТЕ КООРДИНАТУ ДЛЯ ЗАЛПА: ";
                cin >> shot;
                while (Valid_Input(shot) == false) { // проверяем ввод пользователя 
                    gotoxy(37, 11);
                    cout << "НЕКОРРЕКТНЫЙ ВВОД.";
                    gotoxy(37, 12);
                    cout << "ПОВТОРИТЕ ПОПЫТКУ:     \b\b\b\b\b ";
                    cin.clear(); // очищаем буфер
                    cin.ignore(256, '\n');
                    cin >> shot;
                }
                if ((tolower(shot[0])) == 'a' && (tolower(shot[1])) == 'i' && (tolower(shot[2])) == 'r') {
                    Torpedo(Pole_User);
                    step_order = 1;
                }
                if (step_order == 1) break;
                if ((tolower(shot[0])) == 's' && (tolower(shot[1])) == 'a' && (tolower(shot[2])) == 'v' && (tolower(shot[3])) == 'e') {
                    saveGame(Pole_User, Pole_Enemy, ::step_order, ::difficulty, ::mode, ::player, "SAVE.sav");
                    gotoxy(37, 10);
                    cout << "ИДЕТ СОХРАНЕНИЕ...";
                    Sleep(2000);
                    Attack();
                }
                else {
                    PlaySound(TEXT("ATTACK.wav"), NULL, SND_SYNC);
                    Y = (int)(tolower(shot[0])) - 97; // записываем в Y выбор строки (преобразовываем в нижний регистр)
                    (shot[1] == '1' && shot[2] == '0') ? X = 9 : X = (int)shot[1] - 49; // если после буквы ввели 10 - обозначаем, что это последняя ячейка строки. В противном случае - конвертируем введенный символ в соответствующее число

                    // ЕСЛИ НЕЛОГИЧНЫЙ ХОД
                    if (Pole_User[Y][X] >= 5 && Pole_User[Y][X] <= 8) { // если ход нелогичный (если уже ходили, уже ранен или уже убит)
                        gotoxy(37, 13);
                        cout << "НЕЛОГИЧНЫЙ ХОД.";
                        gotoxy(37, 14);
                        cout << "ПОВТОРИТЕ ПОПЫТКУ";
                        Sleep(3000);
                        Show();
                        Attack(); // и ходим снова;
                    }
                    // ЕСЛИ МЫ ПОПАЛИ МИМО ЦЕЛИ
                    if (Pole_User[Y][X] == 0) { // если клетка незанята - помечаем ее как простреленную
                        Pole_User[Y][X] = 5;
                        step_order++; // выходим из цикла - дальше будем ходить мы
                        Show();
                        Miss();
                    }
                    // ТОПИМ 1-ПАЛУБНЫЙ КОРАБЛЬ
                    if (Pole_User[Y][X] == 1) {
                        Pole_User[Y][X] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова;
                    }
                    // ПРОВЕРКА ПОТОПЛЕНИЯ 2-ПАЛУБНОГО КОРАБЛЯ
                    if (Pole_User[Y][X] == 2) { // если попали в 2-палубный корабль
                        // сначала проверяем крестом, что это уже вторая пораженная клетка и помечаем ее убитой
                        if (Pole_User[Y][X + 1] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y][X + 1] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова;
                        }
                        if (Pole_User[Y][X - 1] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y][X - 1] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова;
                        }
                        if (Pole_User[Y + 1][X] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y + 1][X] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова;
                        }
                        if (Pole_User[Y - 1][X] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y - 1][X] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова;
                        }
                        if (Pole_User[Y - 1][X] == 2 || Pole_User[Y][X + 1] == 2 || // если только ранили (т.е. по периметру есть еще значение 2) - отмечаем и ходим снова
                            Pole_User[Y + 1][X] == 2 || Pole_User[Y][X - 1] == 2) {
                            Pole_User[Y][X] = 6;
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                    // ПРОВЕРКА ПОТОПЛЕНИЯ 3-ПАЛУБНОГО КОРАБЛЯ
                    if (Pole_User[Y][X] == 3) { // если только первый раз попали в 3-палубный корабль
                        if (Pole_User[Y - 1][X] == 3 || Pole_User[Y][X + 1] == 3 ||
                            Pole_User[Y + 1][X] == 3 || Pole_User[Y][X - 1] == 3) {
                            Pole_User[Y][X] = 6;
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                        // если попали в центр 3-палубного и по краям он уже ранен - проверяем позиции сверху и снизу
                        if (Pole_User[Y - 1][X] == 6 && Pole_User[Y + 1][X] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y - 1][X] = 7;
                            Pole_User[Y + 1][X] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        // проверяем позиции справа и слева
                        if (Pole_User[Y][X + 1] == 6 && Pole_User[Y][X - 1] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y][X + 1] = 7;
                            Pole_User[Y][X - 1] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        // проверка 3-палубного от края к центру - когда переместились в центр проверяем есть ли убитые слева и справа
                        if (Pole_User[Y - 1][X] == 6) {
                            Pole_User[Y][X] = 6;
                            Y = Y - 1;
                            if (Pole_User[Y - 1][X] == 6) {
                                Pole_User[Y][X] = 7;
                                Pole_User[Y - 1][X] = 7;
                                Pole_User[Y + 1][X] = 7;
                                Show();
                                RIP();
                                Attack(); // и ходим снова
                            }
                            else {
                                Show();
                                Hit();
                                Attack(); // и ходим снова
                            }
                        }
                        if (Pole_User[Y][X + 1] == 6) {
                            Pole_User[Y][X] = 6;
                            X = X + 1;
                            if (Pole_User[Y][X + 1] == 6) {
                                Pole_User[Y][X] = 7;
                                Pole_User[Y][X + 1] = 7;
                                Pole_User[Y][X - 1] = 7;
                                Show();
                                RIP();
                                Attack(); // и ходим снова
                            }
                            else {
                                Show();
                                Hit();
                                Attack(); // и ходим снова
                            }
                        }
                        if (Pole_User[Y + 1][X] == 6) {
                            Pole_User[Y][X] = 6;
                            Y = Y + 1;
                            if (Pole_User[Y + 1][X] == 6) {
                                Pole_User[Y][X] = 7;
                                Pole_User[Y + 1][X] = 7;
                                Pole_User[Y - 1][X] = 7;
                                Show();
                                RIP();
                                Attack(); // и ходим снова
                            }
                            else {
                                Show();
                                Hit();
                                Attack(); // и ходим снова
                            }
                        }
                        if (Pole_User[Y][X - 1] == 6) {
                            Pole_User[Y][X] = 6;
                            X = X - 1;
                            if (Pole_User[Y][X - 1] == 6) {
                                Pole_User[Y][X] = 7;
                                Pole_User[Y][X - 1] = 7;
                                Pole_User[Y][X + 1] = 7;
                                Show();
                                RIP();
                                Attack(); // и ходим снова
                            }
                            else {
                                Show();
                                Hit();
                                Attack(); // и ходим снова
                            }
                        }
                    }
                    // ПРОВЕРКА ПОТОПЛЕНИЯ 4-ПАЛУБНОГО КОРАБЛЯ
                    if (Pole_User[Y][X] == 4) {
                        Pole_User[Y][X] = 8;
                        four_deck_defeated = true;

                        for (int i = 0; i < ::size; i++) {
                            for (int j = 0; j < ::size; j++) {
                                if (Pole_User[i][j] == 4) four_deck_defeated = false;
                            }
                        }
                        if (!four_deck_defeated) {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                        if (four_deck_defeated) {
                            for (int i = 0; i < ::size; i++) {
                                for (int j = 0; j < ::size; j++) {
                                    if (Pole_User[i][j] == 8) Pole_User[i][j] = 7;
                                }
                            }
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                    }
                }
            }
            if (mode == 1) {
                Interface();
                cout << "ХОД КОМПЬЮТЕРА";
                for (int i = 0; i < 3; i++) { // задержка во время хода компьютера
                    cout << " .";
                    Sleep(500);
                }
                PlaySound(TEXT("ATTACK.wav"), NULL, SND_SYNC);
                do { // генерация для выстрела компьютера
                    if (::difficulty == 1) {
                        Y = rand() % ::size;
                        X = rand() % ::size;

                    }
                    if (::difficulty == 2) { // если выбран продвинутый уровеь интеллекта компьютера
                        alert = false; // сброс состояния флага
                        attempt = 0; // сброс счётчика упрощения выбора места следующего выстрела компьютером
                        for (int i = 0; i < ::size; i++) {
                            for (int j = 0; j < ::size; j++) {
                                if (Pole_User[i][j] == 6 || Pole_User[i][j] == 8) {
                                    alert = true; // обнаружен раненый корабль
                                    Y = i, X = j; // записываем обнаруженные координаты как исходные
                                    break;
                                }
                            }
                        }
                        if (alert) { // если есть раненый корабль
                            do {
                                random_counter++; // увеличиваем счетчик когда прошли один цикл выбора случайного направления
                                if (random_counter > 50) { // если попали в тупик, то после 50 попыток меняем направление поиска раненного корабля на обратное (ведем поиск с конца массива)
                                    for (int i = 9; i >= 0; i--) {
                                        for (int j = 9; j >= 0; j--) {
                                            if (Pole_User[i][j] == 6 || Pole_User[i][j] == 8) {
                                                Y = i, X = j;
                                                break;
                                            }
                                        }
                                    }
                                }
                                crossfire = rand() % 4 + 1; // генерируем случайное направление выстрела по одной из сторон от координаты ранения
                                temp_Y = Y; // для временного хранения координаты ранения
                                temp_X = X;
                                attempt++;
                                if (attempt == 1) {
                                    switch (crossfire) { // идем по выбранному случайному направлению
                                    case 1:
                                        temp_X = X + 1; // вправо
                                        break;
                                    case 2:
                                        temp_X = X - 1; // влево
                                        break;
                                    case 3:
                                        temp_Y = Y + 1; // вниз
                                        break;
                                    case 4:
                                        temp_Y = Y - 1; // вверх
                                        break;
                                    }
                                }
                                if (attempt > 1) { // если первая попытка добить наш корабль была в "молоко" - переход на следующую раненую клетку
                                    crossfire = rand() % 4 + 1;
                                    switch (crossfire) {
                                    case 1:
                                        if (Pole_User[Y][X + 1] == 6 || Pole_User[Y][X + 1] == 8)
                                            X = X + 1; // переход вправо
                                        break;
                                    case 2:
                                        if (Pole_User[Y][X - 1] == 6 || Pole_User[Y][X - 1] == 8)
                                            X = X - 1; // переход влево
                                        break;
                                    case 3:
                                        if (Pole_User[Y + 1][X] == 6 || Pole_User[Y + 1][X] == 8)
                                            Y = Y + 1; // переход вниз
                                        break;
                                    case 4:
                                        if (Pole_User[Y - 1][X] == 6 || Pole_User[Y - 1][X] == 8)
                                            Y = Y - 1; // переход вверх
                                        break;
                                    }
                                    crossfire = rand() % 4 + 1;
                                    switch (crossfire) { // а теперь компьютер снова стреляет наугад
                                    case 1:
                                        if (Pole_User[Y][X + 1] == 6 || Pole_User[Y][X + 1] == 8)
                                            temp_X = X - 1; // наоборот идем влево
                                        break;
                                    case 2:
                                        if (Pole_User[Y][X - 1] == 6 || Pole_User[Y][X - 1] == 8)
                                            temp_X = X + 1; // наоборот идем вправо
                                        break;
                                    case 3:
                                        if (Pole_User[Y + 1][X] == 6 || Pole_User[Y + 1][X] == 8)
                                            temp_Y = Y - 1; // наоборот идем вверх
                                        break;
                                    case 4:
                                        if (Pole_User[Y - 1][X] == 6 || Pole_User[Y - 1][X] == 8)
                                            temp_Y = Y + 1; // наоборот идем вниз
                                        break;
                                    }
                                }
                            } while (temp_Y < 0 || temp_Y > 9 || temp_X < 0 || temp_X > 9); // выполняем пока полученные координаты не будут в пределах поля
                            Y = temp_Y; // если оказались здесь - значит временные координаты можно использовать и записываем их в Y и X
                            X = temp_X;
                        }
                        if (!alert) { // если флаг alert == false, т.е. в момент когда на поле нет наших раненых кораблей
                            Y = rand() % ::size; // генерируем случаные координаты выстрела
                            X = rand() % ::size;
                        }
                    }
                    if (::difficulty == 3) { // если выбран продвинутый уровеь интеллекта компьютера
                        alert = false; // сброс состояния флага, что обнаружен раненый корабль
                        for (int i = 0; i < ::size; i++) {
                            for (int j = 0; j < ::size; j++) {
                                if (!alert && (Pole_User[i][j] == 6 || Pole_User[i][j] == 8)) {
                                    Y = i, X = j; // записываем обнаруженные координаты как исходные
                                    alert = true; // обнаружен раненый корабль
                                }
                            }
                        }
                        temp_Y = Y; // для временного хранения координаты ранения
                        temp_X = X;
                        bool reverse = false; // для понимания, что произошел разворот поиска (с конца массива)
                        bool next_step = true; // чтобы прекратить проверку когда нашли подходящую клетку для выстрела
                        if (alert) { // если есть раненый корабль
                            if (next_step && (Pole_User[Y][X + 1] >= 2 && Pole_User[Y][X + 1] <= 4)) {
                                temp_X = X + 1;
                                next_step = false;
                            }
                            if (next_step && (Pole_User[Y][X - 1] >= 2 && Pole_User[Y][X - 1] <= 4)) {
                                temp_X = X - 1;
                                next_step = false;
                            }
                            if (next_step && (Pole_User[Y + 1][X] >= 2 && Pole_User[Y + 1][X] <= 4)) {
                                temp_Y = Y + 1;
                                next_step = false;
                            }
                            if (next_step && (Pole_User[Y - 1][X] >= 2 && Pole_User[Y - 1][X] <= 4)) {
                                temp_Y = Y - 1;
                                next_step = false;
                            }
                            if (next_step) { // используется для предотвращения вечного цикла при добитии 3-х и 4-х палубных кораблей, когда возможны патовые позиции
                                for (int i = 9; i >= 0; i--) {
                                    for (int j = 9; j >= 0; j--) {
                                        if (!reverse && (Pole_User[i][j] == 6 || Pole_User[i][j] == 8)) {
                                            Y = i, X = j;
                                            reverse = true;
                                        }
                                    }
                                }
                                temp_Y = Y; // для временного хранения координаты ранения
                                temp_X = X;
                                if (Pole_User[Y][X + 1] >= 2 && Pole_User[Y][X + 1] <= 4) temp_X = X + 1;
                                if (Pole_User[Y][X - 1] >= 2 && Pole_User[Y][X - 1] <= 4) temp_X = X - 1;
                                if (Pole_User[Y + 1][X] >= 2 && Pole_User[Y + 1][X] <= 4) temp_Y = Y + 1;
                                if (Pole_User[Y - 1][X] >= 2 && Pole_User[Y - 1][X] <= 4) temp_Y = Y - 1;
                            }
                            Y = temp_Y; // если оказались здесь - значит временные координаты можно использовать и записываем их в Y и X
                            X = temp_X;
                        }
                        if (!alert) { // если флаг alert == false, т.е. в момент когда на поле нет наших раненых кораблей
                            Y = rand() % ::size; // генерируем случаные координаты выстрела
                            X = rand() % ::size;
                        }
                    }
                } while (Pole_User[Y][X] >= 5 && Pole_User[Y][X] <= 8); // повтор хода, если выстрел пришелся в простреленное поле или раненый\убитый корабль
                // ЕСЛИ СОПЕРНИК СТРЕЛЬНУЛ МИМО
                if (Pole_User[Y][X] == 0) { // если компьютер попал в незанятое поле
                    Pole_User[Y][X] = 5;
                    step_order++; // выходим из цикла - дальше будем ходить мы
                    Show();
                    Miss();
                }
                // ЕСЛИ ПОТОПИЛ НАШ 1-ПАЛУБНЫЙ КОРАБЛЬ
                if (Pole_User[Y][X] == 1) { // если попали в наш 1-палубный корабль
                    Pole_User[Y][X] = 7; // отмечаем, что он убит
                    Show();
                    RIP();
                    Attack(); // и соперник ходит снова
                }
                // СТРЕЛЬБА ПО НАШЕМУ 2-ПАЛУБНИКУ
                if (Pole_User[Y][X] == 2) { // если попали в 2-палубный корабль
                    // сначала проверяем крестом, что это уже вторая пораженная клетка и помечаем ее убитой
                    if (Pole_User[Y][X + 1] == 6) {
                        Pole_User[Y][X] = 7;
                        Pole_User[Y][X + 1] = 7;
                        Show();
                        RIP();
                        Attack(); // и соперник ходит снова
                    }
                    if (Pole_User[Y][X - 1] == 6) {
                        Pole_User[Y][X] = 7;
                        Pole_User[Y][X - 1] = 7;
                        Show();
                        RIP();
                        Attack(); // и соперник ходит снова
                    }
                    if (Pole_User[Y + 1][X] == 6) {
                        Pole_User[Y][X] = 7;
                        Pole_User[Y + 1][X] = 7;
                        Show();
                        RIP();
                        Attack(); // и соперник ходит снова
                    }
                    if (Pole_User[Y - 1][X] == 6) {
                        Pole_User[Y][X] = 7;
                        Pole_User[Y - 1][X] = 7;
                        Show();
                        RIP();
                        Attack(); // и соперник ходит снова
                    }
                    if (Pole_User[Y - 1][X] == 2 || Pole_User[Y][X + 1] == 2 || // если только ранили (т.е. по периметру есть еще значение 2) - отмечаем и ходим снова
                        Pole_User[Y + 1][X] == 2 || Pole_User[Y][X - 1] == 2) {
                        Pole_User[Y][X] = 6;
                        Show();
                        Hit();
                        Attack(); // и ходим снова
                    }
                }
                // СТРЕЛЬБА ПО НАШЕМУ 3-ПАЛУБНИКУ
                if (Pole_User[Y][X] == 3) { // если только первый раз попали в 3-палубный корабль
                    if (Pole_User[Y - 1][X] == 3 || Pole_User[Y][X + 1] == 3 ||
                        Pole_User[Y + 1][X] == 3 || Pole_User[Y][X - 1] == 3) {
                        Pole_User[Y][X] = 6;
                        Show();
                        Hit();
                        Attack(); // и ходим снова
                    }
                    // если попали в центр 3-палубного и по краям он уже ранен - проверяем позиции сверху и снизу
                    if (Pole_User[Y - 1][X] == 6 && Pole_User[Y + 1][X] == 6) {
                        Pole_User[Y][X] = 7;
                        Pole_User[Y - 1][X] = 7;
                        Pole_User[Y + 1][X] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    // проверяем позиции справа и слева
                    if (Pole_User[Y][X + 1] == 6 && Pole_User[Y][X - 1] == 6) {
                        Pole_User[Y][X] = 7;
                        Pole_User[Y][X + 1] = 7;
                        Pole_User[Y][X - 1] = 7;
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                    // проверка 3-палубного от края к центру - когда переместились в центр проверяем есть ли убитые слева и справа
                    if (Pole_User[Y - 1][X] == 6) {
                        Pole_User[Y][X] = 6;
                        Y = Y - 1;
                        if (Pole_User[Y - 1][X] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y - 1][X] = 7;
                            Pole_User[Y + 1][X] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                    if (Pole_User[Y][X + 1] == 6) {
                        Pole_User[Y][X] = 6;
                        X = X + 1;
                        if (Pole_User[Y][X + 1] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y][X + 1] = 7;
                            Pole_User[Y][X - 1] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                    if (Pole_User[Y + 1][X] == 6) {
                        Pole_User[Y][X] = 6;
                        Y = Y + 1;
                        if (Pole_User[Y + 1][X] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y + 1][X] = 7;
                            Pole_User[Y - 1][X] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                    if (Pole_User[Y][X - 1] == 6) {
                        Pole_User[Y][X] = 6;
                        X = X - 1;
                        if (Pole_User[Y][X - 1] == 6) {
                            Pole_User[Y][X] = 7;
                            Pole_User[Y][X - 1] = 7;
                            Pole_User[Y][X + 1] = 7;
                            Show();
                            RIP();
                            Attack(); // и ходим снова
                        }
                        else {
                            Show();
                            Hit();
                            Attack(); // и ходим снова
                        }
                    }
                }
                // СТРЕЛЬБА ПО НАШЕМУ 4-ПАЛУБНИКУ
                if (Pole_User[Y][X] == 4) {
                    Pole_User[Y][X] = 8;
                    four_deck_defeated = true;

                    for (int i = 0; i < ::size; i++) {
                        for (int j = 0; j < ::size; j++) {
                            if (Pole_User[i][j] == 4) four_deck_defeated = false;
                        }
                    }
                    if (!four_deck_defeated) {
                        Show();
                        Hit();
                        Attack(); // и ходим снова
                    }
                    if (four_deck_defeated) {
                        for (int i = 0; i < ::size; i++) {
                            for (int j = 0; j < ::size; j++) {
                                if (Pole_User[i][j] == 8) Pole_User[i][j] = 7;
                            }
                        }
                        Show();
                        RIP();
                        Attack(); // и ходим снова
                    }
                }
            }
        }
        Show(); // печатаем состояние игровых полей
    }
}

void Show() { // вывод игровой картинки
    gotoxy(0, 0);
    //system("cls"); // очищаем игран
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // для работы с цветом
    int symb_col = 65, symb_row = 1; // symb_col = 65 - буквы по вертикали, symb_row - цифры по горизонтали

    if (!Game_Over) {
        if (!Game_Over == (Check_End_Game(Pole_Enemy))) {
            if (mode == 1) {
                Interface();
                cout << "ПОЗДРАВЛЯЕМ !!!";
                gotoxy(37, 10);
                cout << "ВЫ ПОБЕДИЛИ !!!";
                Victory();

            }
            if (mode == 2) {
                Interface();
                cout << "ИГРОК 1 ПОБЕДИЛ !!!";
                gotoxy(37, 10);
                cout << "ПОЗДРАВЛЯЕМ !!!";
                Victory();
            }
        }
        if (!Game_Over == (Check_End_Game(Pole_User))) {
            if (mode == 1) {
                Interface();
                cout << "ПОРАЖЕНИЕ...";
                gotoxy(37, 10);
                cout << "ПОБЕДИЛ КОМПЬЮТЕР !!! (умничка)";
                Defeat();
            }
            if (mode == 2) {
                Interface();
                cout << "ИГРОК 2 ПОБЕДИЛ !!!";
                gotoxy(37, 10);
                cout << "ПОЗДРАВЛЯЕМ !!!";
                Victory();
            }
        }

    }
    Border(Pole_User); // выставляем бордюры вокруг потопленных кораблей
    Border(Pole_Enemy);

    if (mode == 1) {
        cout << "\n\n\t     ИГРОК            \t\t\t\t\t                        КОМПЬЮТЕР              \n\n\t    ";
    }
    if (mode == 2) {
        cout << "\n\n\t     ИГРОК 1          \t\t\t\t\t                          ИГРОК 2              \n\n\t    ";
    }
    for (int i = 0; i <= ::size + 3; i++) {
        if (i == 0) {
            for (int j = 0; j < ::size; j++) cout << " " << symb_row++; // печатаем цифры у себя
            cout << "\t\t\t\t\t    "; // отступаем
            for (int j = 0, symb_row = 1; j < ::size; j++) cout << " " << symb_row++; // печатаем цифры у соперника
            cout << "\n\t  ";
        }
        if (i == 1) {
            for (int j = 0; j < ::size + 2; j++) cout << " ."; // печатаем точки у себя
            cout << "\t\t\t\t\t  "; // отступаем
            for (int j = 0; j < ::size + 2; j++) cout << " ."; // печатаем точки у соперника
            cout << endl;
        }
        if (i > 1 && i < ::size + 2) {
            cout << "\t" << (char)symb_col << "  ."; // печатаем букву и точку верникально у нас
            for (int j = 0; j < ::size; j++) {
                if (player == 1) { //открытое первое поле первого игрока
                    if (Pole_User[i - 2][j] == 1 || Pole_User[i - 2][j] == 2 ||
                        Pole_User[i - 2][j] == 3 || Pole_User[i - 2][j] == 4) { // если на нашем поле есть корабли (1, 2, 3 или 4), то печатем их
                        cout << " " << (char)299; // символ (+) наших расставленных кораблей
                    }
                    if (Pole_User[i - 2][j] == 5) { // если соперник выстрелил мимо или есть бордюр от потопленного корабля (5)
                        SetConsoleTextAttribute(hConsole, 11); // бирюзовый цвет
                        cout << " " << (char)149; // ставим жирную точку
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_User[i - 2][j] == 6 || Pole_User[i - 2][j] == 8) { // если соперник подбил наш корабль (6 - для всех, 8 - только для 4-палубного)
                        SetConsoleTextAttribute(hConsole, 12); // красный цвет
                        cout << " X"; // рисуем крестик
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_User[i - 2][j] == 7) Brick(1); // если наш корабль убит (7) - ставим белый блок
                    if (Pole_User[i - 2][j] == 0) cout << "  "; // если на нашем поле имеем нетронутую клетку, то печатаем пробел
                }
                if (player == 2 && mode == 2) {//закрытое первое поле первого игрока (если ходит второй игрок)
                    if (Pole_User[i - 2][j] == 5) { // если мимо мы выстрелили мимо (5), либо есть бордюр вокруг убитого корабля
                        SetConsoleTextAttribute(hConsole, 11); // бирюзовый цвет
                        cout << " " << (char)149; // ставим жирную точку
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_User[i - 2][j] == 6 || Pole_User[i - 2][j] == 8) { // если ранили вражеский корабль (6)
                        SetConsoleTextAttribute(hConsole, 12); // красный цвет
                        cout << " X"; // рисуем крестик
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_User[i - 2][j] == 7) Brick(4); // если корабль врага убит (7) - печатаем кирпичик
                    if (Pole_User[i - 2][j] == 0 || Pole_User[i - 2][j] == 1 || Pole_User[i - 2][j] == 2 ||
                        Pole_User[i - 2][j] == 3 || Pole_User[i - 2][j] == 4) cout << "  "; // если на поле соперника имеем нетронутую клетку или его нетронутый корабль, то печатаем пробел

                }
            }
            cout << " .\t\t\t\t\t" << (char)symb_col++ << "  ."; // печатаем букву и точку у соперника
            for (int j = 0; j < ::size; j++) {
                if (player == 1) { // закрытое второе поле второго игрока или компьютера (в зависимости от режима игры)
                    if (Pole_Enemy[i - 2][j] == 5) { // если мимо мы выстрелили мимо (5), либо есть бордюр вокруг убитого корабля
                        SetConsoleTextAttribute(hConsole, 11); // бирюзовый цвет
                        cout << " " << (char)149; // ставим жирную точку
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_Enemy[i - 2][j] == 6 || Pole_Enemy[i - 2][j] == 8) { // если ранили вражеский корабль (6)
                        SetConsoleTextAttribute(hConsole, 12); // красный цвет
                        cout << " X"; // рисуем крестик
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_Enemy[i - 2][j] == 7) Brick(4); // если корабль врага убит (7) - печатаем кирпичик
                    if (Pole_Enemy[i - 2][j] == 0 || Pole_Enemy[i - 2][j] == 1 || Pole_Enemy[i - 2][j] == 2 ||
                        Pole_Enemy[i - 2][j] == 3 || Pole_Enemy[i - 2][j] == 4) cout << "  "; // если на поле соперника имеем нетронутую клетку или его нетронутый корабль, то печатаем пробел
                }
                if (player == 2 && mode == 2) { // открытое второе игровое поле второго игрока
                    if (Pole_Enemy[i - 2][j] == 1 || Pole_Enemy[i - 2][j] == 2 ||
                        Pole_Enemy[i - 2][j] == 3 || Pole_Enemy[i - 2][j] == 4) { // если на нашем поле есть корабли (1, 2, 3 или 4), то печатем их
                        cout << " " << (char)299; // символ (+) наших расставленных кораблей
                    }
                    if (Pole_Enemy[i - 2][j] == 5) { // если соперник выстрелил мимо или есть бордюр от потопленного корабля (5)
                        SetConsoleTextAttribute(hConsole, 11); // бирюзовый цвет
                        cout << " " << (char)149; // ставим жирную точку
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_Enemy[i - 2][j] == 6 || Pole_Enemy[i - 2][j] == 8) { // если соперник подбил наш корабль (6 - для всех, 8 - только для 4-палубного)
                        SetConsoleTextAttribute(hConsole, 12); // красный цвет
                        cout << " X"; // рисуем крестик
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    if (Pole_Enemy[i - 2][j] == 7) Brick(1); // если наш корабль убит (7) - ставим белый блок
                    if (Pole_Enemy[i - 2][j] == 0) cout << "  "; // если на нашем поле имеем нетронутую клетку, то печатаем пробел
                }
            }
            cout << " .\n";
        }
        if (i == ::size + 3) { // печать точками нижней строки обоих полей
            cout << "\t  ";
            for (int j = 0; j < ::size + 2; j++) cout << " .";
            cout << "\t\t\t\t\t  ";
            for (int j = 0; j < ::size + 2; j++) cout << " .";
            cout << "\n\n";
        }
    }

#ifdef DEBUG
    for (int i = 0; i < ::size; i++) { // вывод на экран реального состояния нашего игрового поля
        for (int j = 0; j < ::size; j++) {
            cout << Pole_User[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl << endl;
    for (int i = 0; i < ::size; i++) { // вывод на экран реального состояния игрового поля соперника
        for (int j = 0; j < ::size; j++) {
            cout << Pole_Enemy[i][j] << " ";
        }
        cout << endl;
    }
#endif
    Interface();
}

void Menu() { // стартовое меню
    mciSendStringA("stop Music", NULL, 0, NULL);
    PlaySound(TEXT("MENU.wav"), NULL, SND_ASYNC | SND_LOOP);
    for (int i = 0; i < ::size; i++) { // очищаем игровые поля на случай повторной игры
        for (int j = 0; j < ::size; j++) {
            Pole_Enemy[i][j] = 0;
            Pole_User[i][j] = 0;
        }
    }
    int isselected = 1, size = 4, level1 = 0, level2 = 0, level3 = 0;
    string title = "[------ИГРОВОЕ МЕНЮ------|-----------------ИНФОРМАЦИЯ-----------------]", menuitem1 = "• ОДИН ИГРОК", menuitem2 = "• ДВА ИГРОКА", menuitem3 = "• ЗАГРУЗИТЬ СОХРАНЕНИЕ", menuitem4 = "• ВЫХОД ИЗ ИГРЫ";
    auto Drawmenu = [](string& title, string& menuitem1, string& menuitem2, string& menuitem3, string& menuitem4, int& isselected, int& level1, int& level2, int& level3) { //Лямбда цикла жизни меню. Асинхронно опрашиваются кнопки. В качестве параметров 
        auto Cleartip = []() {//еще одна хитрая лямбда очистки окошка подсказок (а зачем эти функции нужны за пределами Menu, или в данном случае за пределами Drawmenu?)                      //принимаются строки меню. Подсказка захардкожена. Через переменные типа levelX осуществляется
            gotoxy(46, 46);                                                                                                                                                                    //обмен информацией с кодом за ее пределами. Соответствуют выбранным пунктам на соотв. уровне меню
            cout << "                                            ";
            gotoxy(46, 47);
            cout << "                                            ";
            gotoxy(46, 48);
            cout << "                                            ";
            gotoxy(46, 49);
            cout << "                                            ";
        };
        gotoxy(20, 45);
        Setcolor(9, 0);
        cout << title;
        Setcolor(7, 0);
#ifdef DEBUG
        Setcolor(4, 0);
        cout << "\t\tDEBUG MODE";
        Setcolor(7, 0);
#endif
        gotoxy(20, 46);
        cout << "                        " << endl;
        gotoxy(20, 46);
        if (isselected == 1) {
            Cleartip();
            if (level1 == 0) {
                gotoxy(46, 46);
                cout << "Игра против компьютера." << endl;
            }
            if (level1 == 1 && level2 == 0) {
                gotoxy(46, 46);
                cout << "Выбирает ход случайным образом. Логики нет," << endl;
                gotoxy(46, 47);
                cout << "но мимо игрового поля не промахивается." << endl;
            }
            gotoxy(20, 46);
            Setcolor(4, 0);
            cout << menuitem1 << endl;
            Setcolor(7, 0);
        }
        else cout << menuitem1 << endl;
        gotoxy(20, 47);
        cout << "                        " << endl;
        gotoxy(20, 47);
        if (isselected == 2) {
            Cleartip();
            if (level1 == 0) {
                gotoxy(46, 46);
                cout << "Игроки ходят поочередно." << endl;
            }
            if (level1 == 1 && level2 == 0) {
                gotoxy(46, 46);
                cout << "Пытается потопить подбитый корабль," << endl;
                gotoxy(46, 47);
                cout << "но может не угадать его расположение." << endl;
            }
            gotoxy(20, 47);
            Setcolor(4, 0);
            cout << menuitem2 << endl;
            Setcolor(7, 0);
        }
        else cout << menuitem2 << endl;
        gotoxy(20, 48);
        cout << "                        " << endl;
        gotoxy(20, 48);
        if (isselected == 3) {
            Cleartip();
            if (level1 == 1 && level2 == 0) {
                gotoxy(46, 46);
                cout << "Ищет корабли противника как обычно," << endl;
                gotoxy(46, 47);
                cout << "но если находит - топит его 100%!" << endl;
            }
            if (level1 == 1 && level2 > 0 && level3 == 0) {
                gotoxy(46, 46);
                cout << "Вы поочередно с компьютером бросаете кубики." << endl;
                gotoxy(46, 47);
                cout << "Сначала бросаете Вы, затем - противник." << endl;
                gotoxy(46, 48);
                cout << "Первым будет тот, у кого на кубиках выпадет" << endl;
                gotoxy(46, 49);
                cout << "наибольшая сумма очков!" << endl;
            }
            if (level1 == 2 && level2 == 0) {
                gotoxy(46, 46);
                cout << "Игроки поочередно бросают кубики.           " << endl;
                gotoxy(46, 47);
                cout << "Сначала бросает игрок 1, затем - игрок 2. " << endl;
                gotoxy(46, 48);
                cout << "Первым будет тот, у кого на кубиках выпадет" << endl;
                gotoxy(46, 49);
                cout << "наибольшая сумма очков!" << endl;
            }
            if (level1 == 0) {
                gotoxy(46, 46);
                cout << "Загрузка сохраненной игры из файла." << endl;
            }
            gotoxy(20, 48);
            Setcolor(4, 0);
            cout << menuitem3 << endl;
            Setcolor(7, 0);
        }
        else cout << menuitem3 << endl;
        gotoxy(20, 49);
        cout << "                        " << endl;
        gotoxy(20, 49);
        if (isselected == 4) {
            Cleartip();
            if (level1 == 0) {
                gotoxy(46, 46);
                cout << "Уже сдаетесь?" << endl;
            }
            gotoxy(20, 49);
            Setcolor(4, 0);
            cout << menuitem4 << endl;
            Setcolor(7, 0);
        }
        else cout << menuitem4 << endl;
        gotoxy(20, 50);
        Setcolor(9, 0);
        cout << "[------------------------|--------------------------------------------]" << endl;
        Setcolor(7, 0);
#ifdef DEBUG
        cout << "Level1: " << level1 << " Level2: " << level2 << " Level3: " << level3 << endl;
#endif
    };
    Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
    while (true)
    {
        if (GetAsyncKeyState(VK_UP)) {
            if (isselected == 1) {
                Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
                Sleep(200);
            }
            else {
                isselected--;
                //PlaySound(TEXT("C:\\3.wav"), NULL, SND_ASYNC);
                Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
                Sleep(200);
            }
        }
        if (GetAsyncKeyState(VK_DOWN)) {
            if (isselected == size) {
                Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
                Sleep(200);
            }
            else {
                isselected++;
                //PlaySound(TEXT("C:\\3.wav"), NULL, SND_ASYNC);
                Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
                Sleep(200);
            }

        }
        if (GetAsyncKeyState(VK_ESCAPE)) {

            isselected = 1;
            level1 = 0, level2 = 0, level3 = 0;
            ::mode = 0;
            ::difficulty = 0;
            ::player = 1;
            size = 4;
            title = "[------ИГРОВОЕ МЕНЮ------|-----------------ИНФОРМАЦИЯ-----------------]", menuitem1 = "• ОДИН ИГРОК", menuitem2 = "• ДВА ИГРОКА", menuitem3 = "• ЗАГРУЗИТЬ СОХРАНЕНИЕ", menuitem4 = "• ВЫХОД ИЗ ИГРЫ";
            Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
            Sleep(200);
        }
        if (GetAsyncKeyState(VK_RETURN)) {

            if (isselected == 3 && level1 == 0) {//загружаемся
                loadGame(Pole_User, Pole_Enemy, ::step_order, ::difficulty, ::mode, ::player, "SAVE.sav");
                system("cls");
                Interface();
                if (::mode == 1) {
                    cout << "РЕЖИМ: 1 ИГРОК";
                    gotoxy(37, 10);
                    cout << "СЛОЖНОСТЬ: " << ::difficulty;
                    gotoxy(37, 11);
                    (::step_order % 2 == 1) ? cout << "ХОД ИГРОКА" : cout << "ХОД КОМПЬЮТЕРА";
                    gotoxy(37, 40);
                    system("Pause");
                }
                if (::mode == 2) {
                    cout << "РЕЖИМ: 2 ИГРОКА";
                    gotoxy(37, 10);
                    cout << "СЛЕДУЮЩИЙ ХОД:";
                    gotoxy(37, 11);
                    (::step_order % 2 == 1) ? cout << "ИГРОК 1" : cout << "ИГРОК 2";
                    gotoxy(37, 40);
                    system("Pause");
                }
                Show();
                Attack();
            }
            else if (isselected == 1 && level1 == 0) {//заполняем меню вторым уровнем для первого пункта (игра с компьютером)
                level1 = isselected;
                ::mode = 1;
                isselected = 1;
                title = "[-УРОВЕНЬ IQ КОМПЬЮТЕРА--|-----------------ИНФОРМАЦИЯ-----------------]", menuitem1 = "• СТРЕЛЯЕТ В СЛЕПУЮ", menuitem2 = "• СТРАТЕГ", menuitem3 = "• ЭКСТРАСЕНС";
                menuitem4.clear();
                size = 3;
                Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
                Sleep(200);
            }
            else if (level1 == 1 && level2 == 0) {//заполняем меню третьим уровнем для первого пункта (игра с компьютером)
                level2 = isselected;
                ::difficulty = isselected; //Передаем в глобальную переменную выбранную сложность компьютера
                isselected = 1;
                title = "[-----ЧЕЙ ХОД ПЕРВЫЙ?----|-----------------ИНФОРМАЦИЯ-----------------]", menuitem1 = "• ВЫ", menuitem2 = "• КОМПЬЮТЕР", menuitem3 = "• РЕШИТ СЛУЧАЙ";
                menuitem4.clear();
                size = 3;
                Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
                Sleep(200);
            }
            else if (level1 == 1 && level2 > 0 && level3 == 0) {//выбор очередности хода или кубики для игры с компьютером
                level3 = isselected;
                if (isselected == 1 || isselected == 2) {
                    ::player = isselected;
                    ::step_order = isselected;
                    Sleep(200);
                    Manual_placement(::mode);
                }
                if (isselected == 3) {
                    Hand_of_Fate();
                }
            }
            else if (level1 == 0 && isselected == 2) { //выбор очередности хода для двух игроков, заполняем меню второго уровня
                level1 = isselected;
                ::mode = 2;
                isselected = 1;
                title = "[-----ЧЕЙ ХОД ПЕРВЫЙ?----|-----------------ИНФОРМАЦИЯ-----------------]", menuitem1 = "• ИГРОК 1", menuitem2 = "• ИГРОК 2", menuitem3 = "• РЕШИТ СЛУЧАЙ";
                menuitem4.clear();
                size = 3;
                Drawmenu(title, menuitem1, menuitem2, menuitem3, menuitem4, isselected, level1, level2, level3);
                Sleep(200);
            }
            else if (level1 == 2) { //выбор очередности хода для двух игроков
                if (isselected == 1 || isselected == 2) {
                    ::player = 1;
                    ::step_order = isselected;
                    Sleep(200);
                    Manual_placement(::mode);
                }
                if (isselected == 3) {
                    Sleep(200);
                    Hand_of_Fate();
                }
            }
            else if (level1 == 0 && isselected == 4) {//выход
                exit(1);
            }
        }
    }
}
void Hand_of_Fate() { // играем в коcти, чтобы определить очередность ходов
    system("cls");
    const int kick = 2, step = 2; // kick - сколько раз бросаем, step - фаза броска игрока и фаза компьютера
    int game[kick][step]; // массив для хранения
    int summ = 0, counter = 0, summ_player = 0, summ_PC = 0, total_player = 0, total_PC = 0; // для учета статистики
    bool pattern; // варианты узора внутри кубика
    do {
        for (int i = 0; i < kick; i++) { // генерируем случаное число от 1 до 6
            for (int j = 0; j < step; j++) {
                game[i][j] = rand() % 6 + 1;
            }
        }
        for (int k = 0, w = 0; k < 2; k++) {
            for (int m = 0; m < 2; m++) { // рисуем раунд броска (два кубика)
                for (int i = 1; i <= 7; i++) {
                    cout << endl;
                    for (int j = 1; j <= 13; j++) {
                        switch (game[k][m]) { // в зависимости от выпавшего числа выводим соответствующую картинку кубика
                        case 1:
                            pattern = (j == 1 || i == 1 || i == 4 && j == 7 || i == 7 || j == 13);
                            break;
                        case 2:
                            pattern = (j == 1 || i == 1 || i == 3 && j == 5 || i == 5 && j == 9 || i == 7 || j == 13);
                            break;
                        case 3:
                            pattern = (j == 1 || i == 1 || i == 4 && j == 7 || i == 3 && j == 5 || i == 5 && j == 9 || i == 7 || j == 13);
                            break;
                        case 4:
                            pattern = (j == 1 || i == 1 || i == 5 && j == 5 || i == 3 && j == 9 || i == 3 && j == 5 || i == 5 && j == 9 || i == 7 || j == 13);
                            break;
                        case 5:
                            pattern = (j == 1 || i == 1 || i == 4 && j == 7 || i == 5 && j == 5 || i == 3 && j == 9 || i == 3 && j == 5 || i == 5 && j == 9 || i == 7 || j == 13);
                            break;
                        case 6:
                            pattern = (j == 1 || i == 1 || i == 4 && j == 5 || i == 4 && j == 9 || i == 5 && j == 5 || i == 3 && j == 9 || i == 3 && j == 5 || i == 5 && j == 9 || i == 7 || j == 13);
                            break;
                        }
                        if (pattern)
                            cout << "*"; // печать звездочек
                        else
                            cout << " ";
                    }
                }
                cout << endl;
                Sleep(1000);
            }
            if (!(counter % 2)) { // если наш ход
                summ_player = game[k][w] + game[k][w + 1]; // считаем сумму двух выпавших кубиков
                total_player += summ_player; // это больше нужно когда бросков больше одного, но удобно для понимания подсчета статистики
                cout << "\n\tУ Вас выпало " << summ_player << ". Теперь бросает противник!\n";
            }
            else { // если ход компьютера
                summ_PC = game[k][w] + game[k][w + 1];
                cout << "\n\tУ противника выпало " << summ_PC << ". ";
                total_PC += summ_PC;
                cout << endl;
            }
            counter++; // для перехода хода от одного игрока к другому
            cout << endl;
            //system("pause");
        }
        if (total_player > total_PC) { // печать статистики и переход к расстановке кораблей
            cout << "\n\tПоздравляем! У вас выпало " << total_player << " против " << total_PC << " у противника! Вы начинаете игру первым!\n";
            ::step_order = 1;
            Manual_placement(::mode);
        }
        if (total_player < total_PC) { // печать статистики и переход к расстановке кораблей
            cout << "\n\tС результатом " << total_PC << " против " << total_player << " победил противник! Он ходит первым!\n";
            ::step_order = 2;
            Manual_placement(::mode);
        }
        if (total_player == total_PC) { // если ничья
            cout << "\n\tКости выпали одинаково! Бросаем ещё раз!\n";
        }
    } while (total_player == total_PC); // если ничья - играем снова
}

//bool Check_Position(int Pole[][::size], int Y, int X) { // проверка пересечения с другими кораблями
bool Check_Position(int* arr, int Y, int X) { // проверка пересечения с другими кораблями
    //cout << "\nПроверка X " << X << " и Y " << Y;
    if ((*((arr + X) + (Y * ::size)) >= 1 && *((arr + X) + (Y * ::size)) <= 4)) {
        //cout << " БЛОКИРОВАНО 1";
        return true;
    }
    else if ((X > 0) && (Y > 0) && (X < (::size - 1)) && (Y < (::size - 1))) {
        if ((*((arr + X + 1) + (Y * ::size)) >= 1 && *((arr + X + 1) + (Y * ::size)) <= 4) ||
            (*((arr + X - 1) + (Y * ::size)) >= 1 && *((arr + X - 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y + 1) * ::size)) >= 1 && *((arr + X) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X - 1) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X) + ((Y - 1) * ::size)) >= 1 && *((arr + X) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X - 1) + ((Y - 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 2";
            return true;
        }
    }
    else if ((X == 0) && (Y > 0) && (Y < ::size - 1)) {
        if ((*((arr + X + 1) + (Y * ::size)) >= 1 && *((arr + X + 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y + 1) * ::size)) >= 1 && *((arr + X) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X) + ((Y - 1) * ::size)) >= 1 && *((arr + X) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y - 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 3";
            return true;
        }
    }
    else if ((Y == 0) && (X > 0) && (X < ::size - 1)) {
        if ((*((arr + X + 1) + (Y * ::size)) >= 1 && *((arr + X + 1) + (Y * ::size)) <= 4) ||
            (*((arr + X - 1) + (Y * ::size)) >= 1 && *((arr + X - 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y + 1) * ::size)) >= 1 && *((arr + X) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X - 1) + ((Y + 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 4";
            return true;
        }
    }
    else if ((X == ::size - 1) && (Y > 0) && (Y < ::size - 1)) {
        if ((*((arr + X - 1) + (Y * ::size)) >= 1 && *((arr + X - 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y + 1) * ::size)) >= 1 && *((arr + X) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X - 1) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X) + ((Y - 1) * ::size)) >= 1 && *((arr + X) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X - 1) + ((Y - 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 5";
            return true;
        }
    }
    else if ((Y == ::size - 1) && (X > 0) && (X < ::size - 1)) {
        if ((*((arr + X + 1) + (Y * ::size)) >= 1 && *((arr + X + 1) + (Y * ::size)) <= 4) ||
            (*((arr + X - 1) + (Y * ::size)) >= 1 && *((arr + X - 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y - 1) * ::size)) >= 1 && *((arr + X) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X - 1) + ((Y - 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 6";
            return true;
        }
    }
    else if ((X == 0) && (Y == 0)) {
        if ((*((arr + X + 1) + (Y * ::size)) >= 1 && *((arr + X + 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y + 1) * ::size)) >= 1 && *((arr + X) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y + 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 7";
            return true;
        }
    }
    else if ((X == ::size - 1) && (Y == ::size - 1)) {
        if ((*((arr + X - 1) + (Y * ::size)) >= 1 && *((arr + X - 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y - 1) * ::size)) >= 1 && *((arr + X) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X - 1) + ((Y - 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 8";
            return true;
        }
    }
    else if ((X == 0) && (Y == ::size - 1)) {
        if ((*((arr + X + 1) + (Y * ::size)) >= 1 && *((arr + X + 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y - 1) * ::size)) >= 1 && *((arr + X) + ((Y - 1) * ::size)) <= 4) ||
            (*((arr + X + 1) + ((Y - 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y - 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 7";
            return true;
        }

    }
    else if ((X == ::size - 1) && (Y == 0)) {
        if ((*((arr + X - 1) + (Y * ::size)) >= 1 && *((arr + X - 1) + (Y * ::size)) <= 4) ||
            (*((arr + X) + ((Y + 1) * ::size)) >= 1 && *((arr + X) + ((Y + 1) * ::size)) <= 4) ||
            (*((arr + X - 1) + ((Y + 1) * ::size)) >= 1 && *((arr + X + 1) + ((Y - 1) * ::size)) <= 4)) {
            //cout << " БЛОКИРОВАНО 7";
            return true;
        }
    }
    return false;
}

bool Check_Position(int Pole[][::size], int Y, int X) {
    if (Pole[Y][X] >= 1 && Pole[Y][X] <= 4) {
        //cout << " БЛОКИРОВАНО 1";
        return true;
    }
    else if ((X > 0) && (Y > 0) && (X < (::size - 1)) && (Y < (::size - 1))) {
        if (Pole[Y][X + 1] >= 1 && Pole[Y][X + 1] <= 4 ||
            Pole[Y][X - 1] >= 1 && Pole[Y][X - 1] <= 4 ||
            Pole[Y + 1][X] >= 1 && Pole[Y + 1][X] <= 4 ||
            Pole[Y + 1][X + 1] >= 1 && Pole[Y + 1][X + 1] <= 4 ||
            Pole[Y + 1][X - 1] >= 1 && Pole[Y + 1][X - 1] <= 4 ||
            Pole[Y - 1][X] >= 1 && Pole[Y - 1][X] <= 4 ||
            Pole[Y - 1][X + 1] >= 1 && Pole[Y - 1][X + 1] <= 4 ||
            Pole[Y - 1][X - 1] >= 1 && Pole[Y - 1][X - 1] <= 4) {
            //cout << " БЛОКИРОВАНО 2";
            return true;
        }
    }
    else if ((X == 0) && (Y > 0) && (Y < ::size - 1)) {
        if (Pole[Y][X + 1] >= 1 && Pole[Y][X + 1] <= 4 ||
            Pole[Y + 1][X] >= 1 && Pole[Y + 1][X] <= 4 ||
            Pole[Y + 1][X + 1] >= 1 && Pole[Y + 1][X + 1] <= 4 ||
            Pole[Y - 1][X] >= 1 && Pole[Y - 1][X] <= 4 ||
            Pole[Y - 1][X + 1] >= 1 && Pole[Y - 1][X + 1] <= 4) {
            //cout << " БЛОКИРОВАНО 3";
            return true;
        }
    }
    else if ((Y == 0) && (X > 0) && (X < ::size - 1)) {
        if (Pole[Y][X + 1] >= 1 && Pole[Y][X + 1] <= 4 ||
            Pole[Y][X - 1] >= 1 && Pole[Y][X - 1] <= 4 ||
            Pole[Y + 1][X] >= 1 && Pole[Y + 1][X] <= 4 ||
            Pole[Y + 1][X + 1] >= 1 && Pole[Y + 1][X + 1] <= 4 ||
            Pole[Y + 1][X - 1] >= 1 && Pole[Y + 1][X - 1] <= 4) {
            //cout << " БЛОКИРОВАНО 4";
            return true;
        }
    }
    else if ((X == ::size - 1) && (Y > 0) && (Y < ::size - 1)) {
        if (Pole[Y][X - 1] >= 1 && Pole[Y][X - 1] <= 4 ||
            Pole[Y + 1][X] >= 1 && Pole[Y + 1][X] <= 4 ||
            Pole[Y + 1][X - 1] >= 1 && Pole[Y + 1][X - 1] <= 4 ||
            Pole[Y - 1][X] >= 1 && Pole[Y - 1][X] <= 4 ||
            Pole[Y - 1][X - 1] >= 1 && Pole[Y - 1][X - 1] <= 4) {
            //cout << " БЛОКИРОВАНО 5";
            return true;
        }
    }
    else if ((Y == ::size - 1) && (X > 0) && (X < ::size - 1)) {
        if (Pole[Y][X + 1] >= 1 && Pole[Y][X + 1] <= 4 ||
            Pole[Y][X - 1] >= 1 && Pole[Y][X - 1] <= 4 ||
            Pole[Y - 1][X] >= 1 && Pole[Y - 1][X] <= 4 ||
            Pole[Y - 1][X + 1] >= 1 && Pole[Y - 1][X + 1] <= 4 ||
            Pole[Y - 1][X - 1] >= 1 && Pole[Y - 1][X - 1] <= 4) {
            //cout << " БЛОКИРОВАНО 6";
            return true;
        }
    }
    else if ((X == 0) && (Y == 0)) {
        if (Pole[Y][X + 1] >= 1 && Pole[Y][X + 1] <= 4 ||
            Pole[Y + 1][X] >= 1 && Pole[Y + 1][X] <= 4 ||
            Pole[Y + 1][X + 1] >= 1 && Pole[Y + 1][X + 1] <= 4) {
            //cout << " БЛОКИРОВАНО 7";
            return true;
        }
    }
    else if ((X == ::size - 1) && (Y == ::size - 1)) {
        if (Pole[Y][X - 1] >= 1 && Pole[Y][X - 1] <= 4 ||
            Pole[Y - 1][X] >= 1 && Pole[Y - 1][X] <= 4 ||
            Pole[Y - 1][X - 1] >= 1 && Pole[Y - 1][X - 1] <= 4) {
            //cout << " БЛОКИРОВАНО 8";
            return true;
        }
    }
    else if ((X == 0) && (Y == ::size - 1)) {
        if (Pole[Y][X + 1] >= 1 && Pole[Y][X + 1] <= 4 ||
            Pole[Y - 1][X] >= 1 && Pole[Y - 1][X] <= 4 ||
            Pole[Y - 1][X + 1] >= 1 && Pole[Y - 1][X + 1] <= 4) {
            //cout << " БЛОКИРОВАНО 8";
            return true;
        }
    }
    else if ((X == ::size - 1) && (Y == 0)) {
        if (Pole[Y][X - 1] >= 1 && Pole[Y][X - 1] <= 4 ||
            Pole[Y + 1][X] >= 1 && Pole[Y + 1][X] <= 4 ||
            Pole[Y + 1][X - 1] >= 1 && Pole[Y + 1][X - 1] <= 4) {
            //cout << " БЛОКИРОВАНО 8";
            return true;
        }
    }
    //cout << " СВОБОДНО ";
    return false;
}
void Manual_placement(int& mode) {
    system("cls");
    int player = 1;
    //int ship_size = 4;
    while (player < 3) {
        if (player == 1 && mode == 2) {
            system("cls");
            Interface();
            cout << "Расстановка кораблей первого";
            gotoxy(37, 10);
            cout << "игрока. Просим второго игрока";
            gotoxy(37, 11);
            cout << "отвернуться.";
            gotoxy(37, 40);
            system("Pause");
            system("cls");
        }
        if (player == 2 && mode == 2) {
            system("cls");
            Interface();
            cout << "Переход к расстановке кораблей";
            gotoxy(37, 10);
            cout << "второго игрока. Просим";
            gotoxy(37, 11);
            cout << "первого игрока отвенуться.";
            gotoxy(37, 40);
            system("Pause");
            system("cls");
        }
        int ship_end_x;
        int ship_end_y;
        int direction = 3; //1 - север, 2 - восток, 3 - юг, 4 - запад
        int pos_x = 0, pos_y = 0, cursor_x = 0, cursor_y = 6;
        int* arr = &Pole_Enemy[0][0];
        if (player == 1) {
            cursor_x = pos_x + 13;
            arr = &Pole_User[0][0];
        }
        if (player == 2) {
            cursor_x = pos_x + 77;
            arr = &Pole_Enemy[0][0];
        }
        auto Draw_field = [](int player) {
            gotoxy(0, 0);
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // для работы с цветом
            int symb_col = 65, symb_row = 1; // symb_col = 65 - буквы по вертикали, symb_row - цифры по горизонтали
            if (::mode == 1) {
                cout << "\n\n\t     ИГРОК            \t\t\t\t\t                        КОМПЬЮТЕР              \n\n\t    ";
            }
            if (::mode == 2) {
                cout << "\n\n\t     ИГРОК 1          \t\t\t\t\t                          ИГРОК 2              \n\n\t    ";
            }
            for (int i = 0; i <= ::size + 3; i++) {
                if (i == 0) {
                    for (int j = 0; j < ::size; j++) cout << " " << symb_row++; // печатаем цифры у себя
                    cout << "\t\t\t\t\t    "; // отступаем
                    for (int j = 0, symb_row = 1; j < ::size; j++) cout << " " << symb_row++; // печатаем цифры у соперника
                    cout << "\n\t  ";
                }
                if (i == 1) {
                    for (int j = 0; j < ::size + 2; j++) cout << " ."; // печатаем точки у себя
                    cout << "\t\t\t\t\t  "; // отступаем
                    for (int j = 0; j < ::size + 2; j++) cout << " ."; // печатаем точки у соперника
                    cout << endl;
                }
                if (i > 1 && i < ::size + 2) {
                    cout << "\t" << (char)symb_col << "  ."; // печатаем букву и точку верникально у нас
                    if (player == 1) {
                        for (int j = 0; j < ::size; j++) {
                            if (Pole_User[i - 2][j] == 1 || Pole_User[i - 2][j] == 2 ||
                                Pole_User[i - 2][j] == 3 || Pole_User[i - 2][j] == 4) { // если на нашем поле есть корабли (1, 2, 3 или 4), то печатем их
                                cout << " ";
                                Brick(3); // Печатаем кирпичик обозначающий все корабли
                            }
                            if (Pole_User[i - 2][j] == 0) cout << "  "; // если на нашем поле имеем нетронутую клетку, то печатаем пробел

                        }
                    }
                    else {
                        for (int j = 0; j < ::size; j++) {
                            if (Pole_User[i - 2][j] == 1 || Pole_User[i - 2][j] == 2 ||
                                Pole_User[i - 2][j] == 3 || Pole_User[i - 2][j] == 4 || Pole_User[i - 2][j] == 0) { // если на нашем поле есть корабли (1, 2, 3 или 4), то печатем их
                                Brick(2);
                            }
                        }
                    }

                    cout << " .\t\t\t\t\t" << (char)symb_col++ << "  ."; // печатаем букву и точку у соперника
                    for (int j = 0; j < ::size; j++) {
                        if (player == 2) {
                            if (Pole_Enemy[i - 2][j] == 1 || Pole_Enemy[i - 2][j] == 2 ||
                                Pole_Enemy[i - 2][j] == 3 || Pole_Enemy[i - 2][j] == 4) {
                                Brick(1); // если на поле соперника имеем нетронутую клетку или его нетронутый корабль, то печатаем кирпичик
                            }
                            if (Pole_Enemy[i - 2][j] == 0) cout << "  ";
                        }
                        else {
                            if (Pole_Enemy[i - 2][j] == 0 || Pole_Enemy[i - 2][j] == 1 || Pole_Enemy[i - 2][j] == 2 ||
                                Pole_Enemy[i - 2][j] == 3 || Pole_Enemy[i - 2][j] == 4) {
                                Brick(2); // если на поле соперника имеем нетронутую клетку или его нетронутый корабль, то печатаем кирпичик
                            }
                        }
                    }
                    cout << " .\n";
                }
                if (i == ::size + 3) { // печать точками нижней строки обоих полей
                    cout << "\t  ";
                    for (int j = 0; j < ::size + 2; j++) cout << " .";
                    cout << "\t\t\t\t\t  ";
                    for (int j = 0; j < ::size + 2; j++) cout << " .";
                    cout << "\n\n";
                }
            }
#ifdef DEBUG
            cout << "\n\n\n\n\n\n\n\n\n";
            for (int i = 0; i < ::size; i++) { // вывод на экран реального состояния нашего игрового поля
                for (int j = 0; j < ::size; j++) {
                    cout << Pole_User[i][j] << " ";
                }
                cout << endl;
            }
            cout << endl << endl;
            for (int i = 0; i < ::size; i++) { // вывод на экран реального состояния игрового поля соперника
                for (int j = 0; j < ::size; j++) {
                    cout << Pole_Enemy[i][j] << " ";
                }
                cout << endl;
            }
#endif
        };
        auto Onfield = [](int x, int y) {
            if (x >= 0 && x < ::size && y >= 0 && y < ::size) {
                return true;
            }
            else return false;
        };
        auto Draw_ship = [](int& ship_size, int& direction, int& pos_x, int& pos_y, int cursor_x, int cursor_y, int& ship_end_x, int& ship_end_y) {
            ship_end_x = pos_x, ship_end_y = pos_y;
            int temp_x = cursor_x, temp_y = cursor_y;
            gotoxy(cursor_x, cursor_y);
            Setcolor(4, 0);
            cout << "+";
            for (int i = 0; i < ship_size - 1; i++) {
                if (direction == 1) { //1 - север, 2 - восток, 3 - юг, 4 - запад
                    cursor_y--;
                    ship_end_y--;
                }
                if (direction == 2) {
                    cursor_x += 2;
                    ship_end_x++;
                }
                if (direction == 3) {
                    cursor_y++;
                    ship_end_y++;
                }
                if (direction == 4) {
                    cursor_x -= 2;
                    ship_end_x--;
                }
                gotoxy(cursor_x, cursor_y);
                cout << "+";
            }
            Setcolor(7, 0);

#ifdef DEBUG
            gotoxy(0, 20); cout << "Текущая позиция:\ncursor_x: " << cursor_x << "\ncursor_y: " << cursor_y << "\nкорма на поле X: " << pos_x << "\nкорма на поле Y: " << pos_y << "\nнос на поле X: " << ship_end_x << "\nнос на поле Y: " << ship_end_y;
#endif
            gotoxy(temp_x, temp_y);
        };
        auto Erase_ship = [](int& ship_size, int& direction, int cursor_x, int cursor_y) {
            for (int i = 0; i < ship_size; i++) {
                gotoxy(cursor_x, cursor_y);
                cout << " ";
                if (direction == 1) cursor_y--;
                if (direction == 2) cursor_x += 2;
                if (direction == 3) cursor_y++;
                if (direction == 4) cursor_x -= 2;
            }
        };
        int ship_size = 4, counter = 1;

        Draw_field(player);
        Draw_ship(ship_size, direction, pos_x, pos_y, cursor_x, cursor_y, ship_end_x, ship_end_y);
        Interface();
        cout << "U/D/L/R: ДВИЖЕНИЕ КУРСОРА";
        gotoxy(37, 10);
        cout << "SHIFT: УСТАНОВИТЬ КОРАБЛЬ";
        gotoxy(37, 11);
        cout << "SPACE: РАЗВОРОТ КОРАБЛЯ";
        gotoxy(37, 12);
        cout << "(вдали от границ поля)";
        gotoxy(37, 13);
        cout << "ESC: ОТМЕНА РУЧНОЙ РАССТАНОВКИ";
        while (true) {
            if (GetAsyncKeyState(VK_LEFT)) {
                if ((direction == 4 && pos_x - 1 >= 0 && pos_x - ship_size >= 0) || (direction != 4 && pos_x - 1 >= 0)) {
                    Erase_ship(ship_size, direction, cursor_x, cursor_y);
                    Draw_field(player);
                    pos_x--;
                    cursor_x -= 2;
                    Draw_ship(ship_size, direction, pos_x, pos_y, cursor_x, cursor_y, ship_end_x, ship_end_y);
                    Sleep(100);
                }
            }
            if (GetAsyncKeyState(VK_RIGHT)) {
                if ((direction == 2 && pos_x + 1 < ::size && pos_x + ship_size < ::size) || (direction != 2 && pos_x + 1 < ::size)) {
                    Erase_ship(ship_size, direction, cursor_x, cursor_y);
                    Draw_field(player);
                    pos_x++;
                    cursor_x += 2;
                    Draw_ship(ship_size, direction, pos_x, pos_y, cursor_x, cursor_y, ship_end_x, ship_end_y);
                    Sleep(100);
                }
            }
            if (GetAsyncKeyState(VK_DOWN)) {
                if ((direction == 3 && pos_y + 1 < ::size && pos_y + ship_size < ::size) || (direction != 3 && pos_y + 1 < ::size)) {
                    Erase_ship(ship_size, direction, cursor_x, cursor_y);
                    Draw_field(player);
                    pos_y++;
                    cursor_y++;
                    Draw_ship(ship_size, direction, pos_x, pos_y, cursor_x, cursor_y, ship_end_x, ship_end_y);
                    Sleep(100);
                }
            }
            if (GetAsyncKeyState(VK_UP)) {
                if ((direction == 1 && pos_y - 1 >= 0 && pos_y - ship_size >= 0) || (direction != 1 && pos_y - 1 >= 0)) {
                    Erase_ship(ship_size, direction, cursor_x, cursor_y);
                    Draw_field(player);
                    pos_y--;
                    cursor_y--;
                    Draw_ship(ship_size, direction, pos_x, pos_y, cursor_x, cursor_y, ship_end_x, ship_end_y);
                    Sleep(100);
                }
            }
            if (GetAsyncKeyState(VK_SPACE)) {
                if (pos_x - ship_size >= 0 && pos_y - ship_size >= 0 && pos_x + ship_size < ::size && pos_y + ship_size < ::size) { //лень прописывать полноценную логику. Пока такая система - корабль можно крутить только если его можно провернуть вокруг кормы не заходя за границы поля.
                    Erase_ship(ship_size, direction, cursor_x, cursor_y);
                    Draw_field(player);
                    if (direction < 4) direction++;
                    else direction = 1;
                    Draw_ship(ship_size, direction, pos_x, pos_y, cursor_x, cursor_y, ship_end_x, ship_end_y);
                    Sleep(200);
                }
            }
            if (GetAsyncKeyState(VK_ESCAPE)) {
                for (int i = 0; i < ::size; i++) { // очищаем игровые поля на случай повторной игры
                    for (int j = 0; j < ::size; j++) {
                        Pole_Enemy[i][j] = 0;
                        Pole_User[i][j] = 0;
                    }
                }
                Arrangement(1);
            }
            if (GetAsyncKeyState(VK_SHIFT)) {
                int check_x = pos_x, check_y = pos_y;
                bool flag = 0;
                for (int i = 0; i < ship_size; i++) {
                    if (Check_Position(arr, check_y, check_x) == false) {
                        flag = 1;
                        if (direction == 1) {
                            check_y--;
                            gotoxy(50, 20 + i);
                        }
                        if (direction == 2) {
                            check_x++;
                            gotoxy(50, 20 + i);
                        }
                        if (direction == 3) {
                            check_y++;
                            gotoxy(50, 20 + i);
                        }
                        if (direction == 4) {
                            check_x--;
                            gotoxy(40, 20 + i);
                        }
                    }
                    else flag = 0;
                }
                if (flag == 1) {
                    counter--;
                    int put_x = pos_x, put_y = pos_y;
                    Erase_ship(ship_size, direction, cursor_x, cursor_y);
                    Sleep(200);
                    for (int i = 0; i < ship_size; i++) {
                        if (direction == 1) {
                            *((arr + put_x) + (put_y * ::size)) = ship_size;
#ifdef DEBUG
                            gotoxy(20, 20 + i);
                            cout << "добавляем палубу в X:" << put_x << " Y:" << put_y;
#endif
                            put_y--;
                        }
                        if (direction == 2) {
                            *((arr + put_x) + (put_y * ::size)) = ship_size;
#ifdef DEBUG
                            gotoxy(20, 20 + i);
                            cout << "добавляем палубу в X:" << put_x << " Y:" << put_y;
#endif
                            put_x++;
                        }
                        if (direction == 3) {
                            *((arr + put_x) + (put_y * ::size)) = ship_size;
#ifdef DEBUG
                            gotoxy(20, 20 + i);
                            cout << "добавляем палубу в X:" << put_x << " Y:" << put_y;
#endif
                            put_y++;
                        }
                        if (direction == 4) {
                            *((arr + put_x) + (put_y * ::size)) = ship_size;
#ifdef DEBUG
                            gotoxy(20, 20 + i);
                            cout << "добавляем палубу в X:" << put_x << " Y:" << put_y;
#endif
                            put_x--;
                        }
                    }
                    if (counter == 0) {
                        if (ship_size == 4) counter = 2;
                        if (ship_size == 3) counter = 3;
                        if (ship_size == 2) counter = 4;
                        if (ship_size == 1) {
                            Draw_field(player);
                            if (player == 1 && mode == 1) {
                                Arrangement(2);
                            }
                            //return;
                            if (player == 1 && mode == 2) {
                                player++;
                                break;
                            }
                            if (player == 2 && mode == 2) {
                                ::step_order = 2;
                                Attack();
                            }
                        }
                        ship_size--;
                    }
                    Draw_field(player);
                    Sleep(200);
                    Draw_ship(ship_size, direction, pos_x, pos_y, cursor_x, cursor_y, ship_end_x, ship_end_y);

                }
                Sleep(200);
            }
        }
    }
}
void Arrangement(int step) { // автоматическая расстановка кораблей
    while (step < 3) { // пока не расставили корабли по двум полям
        int ships_installed = 0; // кол-во уже расставленных кораблей (из десяти)
        int four_deck = 1, three_deck = 2, double_deck = 3, single_deck = 4; // кол-во кораблей разного типа для сравнения со счётчиком counter в конце главного цикла while
        int ship_size = 4; // размер корабля (кол-во палуб), с которым в конкретный момент будет работать функция автоматической расстановки. Будет именьшаться на один пропорционально увеличению на один кол-ва таких кораблей
        int ship_type_counter = 1; // для контроля полной расстановки кораблей определенного типа (палубности)
        while (ships_installed < four_deck + three_deck + double_deck + single_deck) { // пока расставленных кораблей меньше общего кол-ва кораблей
            bool install = true; // true - если нет помех и установка корабля возможна
            int Y = 0, X = 0; // координаты для расстановки на поле
            int direction = 0; // направление расстановки (вправо, вниз, влево, вверх)
            int temp_Y; // для хранения значений Y на период проверки возможности установки корабля в эту ячейку
            int temp_X; // для хранения значений X на период проверки возможности установки корабля в эту ячейку
            int success = 0; // кол-во успешно расставленных кораблей (увеличивается на 1 если удалось установить корабль на поле)
            while (success < ship_type_counter) { // проверка, что установлены корабли всех типов (ship_type_counter увеличивается когда все корабли определенной палубности установлены - например, все трёхпалубные). success - кол-во успешно расставленных кораблей
                do {
                    Y = rand() % ::size; // генерируем псевдослучайные значения
                    X = rand() % ::size;
                    temp_Y = Y; // записываем координаты X, Y во временную переменную, чтобы получить к ним доступ снова если установка корабля в эту ячейку возможна
                    temp_X = X;
                    direction = rand() % 4; // генерируем случайное направления установки корабля
                    install = true; // обнуляем значение переменной

                    for (int i = 0; i < ship_size; i++) { // пока i меньше размера устанавливаемого корабля
                        if (X < ::size - ::size || Y < ::size - ::size || X >= ::size || Y >= ::size) { // проверяем, что координата не находится за пределами поля
                            install = false; // если попали за пределы поля
                            break; // выходим из цикла т.к. дальше проверять нет смысла
                        }
                        if (step == 1) { // проверка, что не попадаем на занятые другими кораблями ячейки и не граничим с кораблями вплотную (т.е. обходим корабль по контуру)
                            if (Check_Position(Pole_User, Y, X) == true) {
                                install = false; // если нашли занятую ячейку (со значением 1)
                                break; // выходим из цикла т.к. дальше проверять нет смысла
                            }
                        }
                        if (step == 2) { // проверка, что не попадаем на занятые другими кораблями ячейки и не граничим с кораблями вплотную (т.е. обходим корабль по контуру)
                            if (Check_Position(Pole_Enemy, Y, X) == true) {
                                install = false; // если нашли занятую ячейку (со значением 1)
                                break; // выходим из цикла т.к. дальше проверять нет смысла
                            }
                        }
                        if (direction == 0) X++; //если X, Y нас устраивают - идем в сгенерированном ранее направлении вправо 
                        if (direction == 1) Y++; // идём вниз
                        if (direction == 2) X--; // идём влево
                        if (direction == 3) Y--; // идём вверх
                    }
                } while (!(install)); // пока не попали в свободную ячейку (со значением 0), рядом также все ячейки свободны и нет выхода за пределы игрового поля

                if (install) { // если установка корабля возможна
                    Y = temp_Y; // возвращаем последние (подходящие для установки корабля) координаты
                    X = temp_X;
                    for (int i = 0; i < ship_size; i++) { // пока i меньше размера корабля
                        if (step == 1) Pole_User[Y][X] = ship_size; // ставим корабль в эту ячейку (меняем 0 на 1 в массиве)
                        if (step == 2) Pole_Enemy[Y][X] = ship_size;

                        if (direction == 0) X++; //если X, Y нас устраивают - идем в сгенерированном ранее направлении вправо 
                        if (direction == 1) Y++; // идём вниз
                        if (direction == 2) X--; // идём влево
                        if (direction == 3) Y--; // идём вверх
                    }
                }
                success++; // поскольку корабль установлен - радуемся и увеличиваем этот контрольный счётчик
                ships_installed++; // увеличиваем общий счетник установленных кораблей для главного цикла
            }
            if (ships_installed == four_deck) { // проверка, что установлены все 4-палубные корабли
                ship_size--; // уменьшаем размерность палуб (т.е. переходим к 3-палубным)
                ship_type_counter++; // отчитываемся, что этот тип кораблей полностью расставлен на поле
            }
            if (ships_installed == four_deck + three_deck) { // проверка, что установлены все 3-палубные корабли
                ship_size--; // уменьшаем размерность палуб (т.е. переходим к 2-палубным)
                ship_type_counter++;
            }
            if (ships_installed == four_deck + three_deck + double_deck) { // проверка, что установлены все 2-палубные корабли
                ship_size--; // уменьшаем размерность палуб (т.е. переходим к 1-палубным)
                ship_type_counter++;
            }
            if (ships_installed == four_deck + three_deck + double_deck + single_deck) { // проверка, что установлены все 1-палубные корабли
                ship_size--; // больше нет палуб - значит и нет кораблей
                ship_type_counter++;
            }
        }
        step++; // переход от расстовки на поле игрока к расстановке на поле соперника
    }
    Show();
    Attack();
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(0, "");
    srand(time(NULL)); // генератор случайных чисел
    SetConsoleDisplayMode(GetStdHandle(STD_OUTPUT_HANDLE), CONSOLE_FULLSCREEN_MODE, 0);

    // Легенда:
    // 0 - пустое стартовое поле (по умолчанию перед генерацией все ячейки в ноль)
    // 1 - 1-палубный корабль
    // 2 - 2-палубный корабль
    // 3 - 3-палубный корабль
    // 4 - 4-палубный корабль
    // 5 - выстрел мимо либо бордюр вокруг убитого корабля (ставится голубая точка)
    // 6 - ранен - для всех кроме 4-палубного (ставится красный Х)
    // 7 - убит (ставится белый блок)
    // 8 - ранен - для 4-палубного
    Crew(); // вывод стартового кадра
    Intro(); // выводим заставку
    Menu(); // выводим меню
}