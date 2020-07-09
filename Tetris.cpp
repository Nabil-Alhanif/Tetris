#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>
#include <Windows.h>

#define konst 4

namespace exUtil
{
	bool print(CHAR_INFO* cif, size_t mW, const wchar_t* wcar, int sWidth = 30, int x = 0, int y = 0)
	{
		size_t sie = std::wcslen(wcar);
		for (unsigned int i = 0; i < sie; i++)
		{
			cif[(y + (i / mW)) * sWidth + ((i % mW) + x)].Char.UnicodeChar = wcar[i];
			cif[(y + (i / mW)) * sWidth + ((i % mW) + x)].Attributes = 0x000f;
		}
		return 1;
	}
}

struct TETRO
{
public:
	int xPos;
	int yPos;
	std::wstring tets;

	TETRO()
	{
		this->tets += L"    ";
		this->tets += L"    ";
		this->tets += L"    ";
		this->tets += L"    ";
		this->xPos = 2;
		this->yPos = 0;
	}
	TETRO(std::wstring ws)
	{
		this->tets = ws;
		this->xPos = 2;
		this->yPos = 0;
	}
	void reset()
	{
		this->xPos = 2;
		this->yPos = 0;
	}
	char getChar(int x, int y)
	{
		char c = (char)tets.c_str()[y * 4 + x];
		return c;
	}
	// Check for collision
	bool isCollide(int x, int y, std::wstring map)
	{
		bool ret = 0;
		for (int nx = 0; nx < konst; nx++)
		{
			for (int ny = 0; ny < konst; ny++)
			{
				if (tets.c_str()[ny * konst + nx] != ' ')
				{
					if (map.c_str()[(yPos + ny + y) * 12 + (xPos + nx + x)] != ' ')
						ret = 1;
				}
			}
		}
		return ret;
	}
	bool rotate(bool l, bool r, std::wstring map)
	{
		std::wstring tmp, tmp2;
		tmp += L"    ";
		tmp += L"    ";
		tmp += L"    ";
		tmp += L"    ";
		if (l)
		{
			int nt = 3;
			for (int ny = 0; ny < konst; ny++)
			{
				for (int nx = 0; nx < konst; nx++)
				{
					tmp[ny * konst + nx] = this->tets[nt + nx * konst];
				}
				nt--;
			}
		}
		else
		{
			int nt = 12;
			for (int ny = 0; ny < konst; ny++)
			{
				for (int nx = 0; nx < konst; nx++)
				{
					tmp[ny * konst + nx] = this->tets[nt - nx * konst];
				}
				nt++;
			}
		}
		tmp2 = this->tets;
		this->tets = tmp;
		if (this->isCollide(0, 0, map))
			this->tets = tmp2;
		return 1;
	}
};

class TETRIS
{
public:
	TETRIS()
	{
		// Create tetris block
		// Blue
		block[0] += L"  I ";
		block[0] += L"  I ";
		block[0] += L"  I ";
		block[0] += L"  I ";

		// Orange
		block[1] += L"    ";
		block[1] += L" L  ";
		block[1] += L" L  ";
		block[1] += L" LL ";

		// Dark Blue
		block[2] += L"    ";
		block[2] += L"  J ";
		block[2] += L"  J ";
		block[2] += L" JJ ";

		// Red
		block[3] += L"    ";
		block[3] += L" ZZ ";
		block[3] += L"  ZZ";
		block[3] += L"    ";

		// Green
		block[4] += L"    ";
		block[4] += L"  SS";
		block[4] += L" SS ";
		block[4] += L"    ";

		// Purple
		block[5] += L"    ";
		block[5] += L"  T ";
		block[5] += L" TTT";
		block[5] += L"    ";

		// Yellow
		block[6] += L"    ";
		block[6] += L" OO ";
		block[6] += L" OO ";
		block[6] += L"    ";

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleWindowInfo(hOriConsole, TRUE, &oriConsoleInfo.srWindow);
		SetConsoleScreenBufferSize(hOriConsole, oriConsoleInfo.dwSize);

		nMapWidth = 12;
		nMapHeight = 25;

		TETRIS::CreateConsole((nMapWidth + 7) * konst, (nMapHeight - 4) * konst, 8, 8);
	}
	~TETRIS()
	{
		SetConsoleActiveScreenBuffer(hOriConsole);
		bGameRunning = 0;
	}
	int CreateConsole(int width, int height, int fontw, int fonth)
	{
		if (hConsole == INVALID_HANDLE_VALUE)
			return Error(L"Bad Handle");

		nScreenWidth = width;
		nScreenHeight = height;

		srRectWin = { 0,0,1,1 };
		SetConsoleWindowInfo(hConsole, TRUE, &srRectWin);

		// Set screen buffer size
		COORD coord = { (short)nScreenWidth, (short)nScreenHeight };
		if (!SetConsoleScreenBufferSize(hConsole, coord))
			Error(L"SetConsoleScreenBufferSize");

		// Assign screen buffer to the console
		if (!SetConsoleActiveScreenBuffer(hConsole))
			return Error(L"SetConsoleActiveScreenBuffer");

		// Set the font size
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(cfi);
		cfi.nFont = 0;
		cfi.dwFontSize.X = fontw;
		cfi.dwFontSize.Y = fonth;
		cfi.FontFamily = 0 << 4;
		cfi.FontWeight = 400;

		wcscpy_s(cfi.FaceName, L"Consolas");
		if (!SetCurrentConsoleFontEx(hConsole, false, &cfi))
			return Error(L"SetCurrentConsoleFontEx");

		// Set physical console window size
		srRectWin = { 0, 0, (short)nScreenWidth - 1, (short)nScreenHeight - 1 };
		if (!SetConsoleWindowInfo(hConsole, TRUE, &srRectWin))
			return Error(L"SetConsoleWindowInfo");

		// Allocate memory for screen buffer
		bufScreen = new CHAR_INFO[(size_t)nScreenWidth * nScreenHeight];
		memset(bufScreen, 0, sizeof(CHAR_INFO) * nScreenWidth * nScreenHeight);

		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CloseHandler, TRUE);
		return 1;
	}
	void blit(int x, int y, int px, int py, short c = 0x2588, short col = 0x000F)
	{
		if (x >= 0 && x < nScreenWidth && y >= 0 && y < nScreenHeight)
		{
			switch (c)
			{
			case 'I':
				col = 0x0009;
				break;
			case 'L':
				col = 0x0006;
				break;
			case 'J':
				col = 0x0001;
				break;
			case 'Z':
				col = 0x0004;
				break;
			case 'S':
				col = 0x000A;
				break;
			case 'T':
				col = 0x000D;
				break;
			case 'O':
				col = 0x000E;
				break;
			default:
				break;
			}
			int pos = y * nScreenWidth * py + x * px;
			if (c >= 'A' && c <= 'Z')c = 0x2588;
			for (int ny = 0; ny < py; ny++)
			{
				for (int nx = 0; nx < px; nx++)
				{
					bufScreen[pos + nx + nScreenWidth * ny].Char.UnicodeChar = c;
					bufScreen[pos + nx + nScreenWidth * ny].Attributes = col;
				}
			}
		}
	}
	void start()
	{
		bGameRunning = 1;
		bKPress = 0;
		bRot = 0;
		bUpdate = 0;
		bPause = 0;
		bHold = 0;
		bNext = 1;
		bool bDMove = 0;

		int curY;

		auto tp1 = std::chrono::system_clock::now();
		auto tp2 = std::chrono::system_clock::now();
		auto tp3 = tp2;
		auto tp4 = tp2;
		auto tp5 = tp2;

		// Create assets
		/**************************************************************/

			// Initialize 10 X 24 map
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"#          #";
		map += L"############";

		bBlock = 1;
		nIter = 0;
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 7; j++)
			{
				tmp[j] = { std::rand(),j };
			}
			std::sort(tmp, tmp + 7);
			for (int j = 0; j < 7; j++)
			{
				blocks[j + i * 7] = tmp[j].second;
			}
		}
		tetro = new TETRO(block[blocks[nIter]]);
		hold = nullptr;

		/**************************************************************/

		while (bGameRunning)
		{
			while (bGameRunning)
			{
				// Timing
				tp2 = std::chrono::system_clock::now();
				std::chrono::duration<float> elapsedTime = tp2 - tp1;
				tp1 = tp2;
				float fElapsedTime = elapsedTime.count();

				// Handle update
				/**************************************************************/

				// Pause for 0.5 second
				if (bUpdate)
					std::this_thread::sleep_for(std::chrono::milliseconds(500));

				// Delete complete line
				if (bUpdate)
				{
					int c = 0;
					for (int ny = konst - 1; ny >= 0; ny--)
					{
						int tmp = ny + curY + c;
						if (tmp > nMapHeight - 2)
							continue;
						if (map[tmp * nMapWidth + 1] == '=')
						{
							c++;
							while (tmp > 0)
							{
								for (int nx = 1; nx < nMapWidth - 1; nx++)
								{
									map[tmp * nMapWidth + nx] = map[(tmp - 1) * nMapWidth + nx];
									map[(tmp - 1) * nMapWidth + nx] = ' ';
								}
								tmp--;
							}
						}
					}
					bUpdate = 0;
				}

				// Display next 4 blocks
				for (int i = nIter + 1; i <= nIter + 4; i++)
				{
					int yPp = (i - nIter - 1) * 5;
					for (int nx = 0; nx < 4; nx++)
					{
						for (int ny = 0; ny < 4; ny++)
						{
							if (bNext)
								blit(nx + nMapWidth + 2, ny + yPp, konst, konst, block[blocks[i % 14]][ny * 4 + nx]);
							else 
								blit(nx + nMapWidth + 2, ny + yPp, konst, konst, ' ');
						}
					}
				}

				// Handle Input
				// Pause and unpause
				if ((GetAsyncKeyState((unsigned short)'P') & 0x8000) && !bKPress && !bUpdate)
				{
					bKPress = 1;
					tp4 = tp2;
					bPause = !bPause;
				}

				// Exit
				if (GetAsyncKeyState((unsigned short)'X') & 0x8000)
					bGameRunning = 0;

				// Hold
				if ((GetAsyncKeyState((unsigned short)'C') & 0x8000) && !bHold && !bPause)
				{
					bHold = 1;
					if (hold)
					{
						std::wstring tmp = hold->tets;
						hold = tetro;
						tetro->reset();
						tetro = new TETRO(tmp);
					}
					else
					{
						hold = tetro;
						tetro->reset();
						nIter++;
						tetro = new TETRO(block[blocks[nIter]]);
					}
				}

				// Show/Unshow next 4 blocks
				if ((GetAsyncKeyState((unsigned short)'T') & 0x8000) && !bKPress && !bPause)
				{
					bKPress = 1;
					tp4 = tp2;
					bNext = !bNext;
					sAppName = std::to_wstring(bNext);
				}

				// Down
				if ((GetAsyncKeyState((unsigned short)'S') & 0x8000) && !bKPress && !bUpdate && !bPause)
				{
					bKPress = 1;
					bDMove = 1;
					tp4 = tp2;
					tp3 = tp2;
				}

				// Right
				if ((GetAsyncKeyState((unsigned short)'D') & 0x8000) && !bKPress && !bUpdate && !bPause)
				{
					bKPress = 1;
					tp3 += std::chrono::milliseconds(100);
					if (tp2 < tp3)tp3 = tp2;
					tp4 = tp2;
					if (!tetro->isCollide(1, 0, this->map))
						tetro->xPos++;
				}

				// Left
				if ((GetAsyncKeyState((unsigned short)'A') & 0x8000) && !bKPress && !bUpdate && !bPause)
				{
					bKPress = 1;
					tp3 += std::chrono::milliseconds(100);
					if (tp2 < tp3)tp3 = tp2;
					tp4 = tp2;
					if (!tetro->isCollide(-1, 0, this->map))
						tetro->xPos--;
				}

				// Rotate Right
				if ((GetAsyncKeyState((unsigned short)'E') & 0x8000) && !bRot && !bUpdate && !bPause)
				{
					bRot = 1;
					tp3 += std::chrono::milliseconds(00);
					if (tp2 < tp3)tp3 = tp2;
					tp5 = tp2;
					tetro->rotate(0, 1, this->map);
				}

				// Rotate Left
				if ((GetAsyncKeyState((unsigned short)'Q') & 0x8000) && !bRot && !bUpdate && !bPause)
				{
					bRot = 1;
					tp3 += std::chrono::milliseconds(00);
					if (tp2 < tp3)tp3 = tp2;
					tp5 = tp2;
					tetro->rotate(1, 0, this->map);
				}

				if (tp4 + std::chrono::milliseconds(200) <= tp2)
					bKPress = 0;
				if (tp5 + std::chrono::milliseconds(150) <= tp2)
					bRot = 0;

				// Move down every second
				if (tp3 + std::chrono::seconds(1) <= tp2 && !bUpdate && !bPause)
				{
					bDMove = 1;
				}

				// Handle down movement
				if (bDMove)
				{
					tp3 = tp2;
					curY = tetro->yPos;
					if (!tetro->isCollide(0, 1, this->map))
						tetro->yPos++;
					else
					{
						bBlock = 0;
						nIter++;
						for (int nx = 0; nx < 4; nx++)
						{
							for (int ny = 0; ny < 4; ny++)
							{
								char c = tetro->getChar(nx, ny);
								if (c != ' ')
									map[(curY + ny) * nMapWidth + (tetro->xPos + nx)] = c;
							}
						}
						for (int ny = 0; ny < 4 && tetro->yPos + ny < 24; ny++)
						{
							int tmp = (curY + ny) * nMapWidth;
							bool b = 1;
							for (int nx = 1; nx < 11; nx++)
							{
								b = (b && map[tmp + nx] >= 'A' && map[tmp + nx] <= 'Z');
							}
							if (b)
							{
								bUpdate = 1;
								for (int nx = 1; nx < 11; nx++)
								{
									map[tmp + nx] = '=';
								}
							}
						}
						if (curY < 4)
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(500));
							bGameRunning = 0;
						}
						delete tetro;
					}
					bDMove = 0;
				}

				// Hold blocks
				exUtil::print(bufScreen, 8, L"HOLD: ", nScreenWidth, 1, 6);
				if (hold)
				{
					for (int nx = 0; nx < 4; nx++)
					{
						for (int ny = 0; ny < 4; ny++)
						{
							blit(nx, ny + 5, 2, 2, hold->getChar(nx, ny));
						}
					}
				}

				// Regenrate tetris block
				if (!bBlock)
				{
					bBlock = 1;
					bHold = 0;
					nIter %= 14;
					if (!(nIter % 7))
					{
						for (int i = 0; i < 7; i++)
						{
							tmp[i] = { std::rand(),i };
						}
						sort(tmp, tmp + 7);
						if (nIter == 0)
						{
							for (int i = 0; i < 7; i++)
							{
								blocks[i + 7] = tmp[i].second;
							}
						}
						else if (nIter == 7)
						{
							for (int i = 0; i < 7; i++)
							{
								blocks[i] = tmp[i].second;
							}
						}
					}
					tetro = new TETRO(block[blocks[nIter]]);
				}

				/**************************************************************/

				// Draw map
				for (int nx = 0; nx < nMapWidth; nx++)
				{
					for (int ny = 0; ny < nMapHeight - 4; ny++)
					{
						blit(nx + 2, ny, konst, konst, map[(ny + 4) * nMapWidth + nx]);
					}
				}

				// Draw block
				for (int nx = 0; nx < 4; nx++)
				{
					for (int ny = 0; ny < 4; ny++)
					{
						if (ny + tetro->yPos - 4 >= 0 && tetro->getChar(nx, ny) != ' ')
							blit(nx + tetro->xPos + 2, ny + tetro->yPos - 4, konst, konst, tetro->tets[ny * 4 + nx]);
					}
				}

				// Update screen title and buffer
				wchar_t title[256];
				swprintf_s(title, 256, L"%s - FPS: %3.2f", sAppName.c_str(), 1.0f / fElapsedTime);
				SetConsoleTitle(title);
				WriteConsoleOutput(hConsole, bufScreen, { (short)nScreenWidth, (short)nScreenHeight }, { 0,0 }, &srRectWin);
			}
			for (int nx = 0; nx * 4 < nScreenWidth; nx++)
			{
				for (int ny = 0; ny * 4 < nScreenHeight; ny++)
				{
					blit(nx, ny, konst, konst, ' ');
				}
			}
			WriteConsoleOutput(hConsole, bufScreen, { (short)nScreenWidth, (short)nScreenHeight }, { 0,0 }, &srRectWin);
			// Free memory
			if (bufScreen)delete[] bufScreen;
			if (tetro)delete tetro;
			delete hold;
			SetConsoleActiveScreenBuffer(hOriConsole);
		}
	}
protected:
	int Error(const wchar_t* msg)
	{
		// Error message
		wchar_t buf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
		SetConsoleActiveScreenBuffer(hOriConsole);
		wprintf(L"ERROR: %s\n\t%s\n", msg, buf);
		return 0;
	}
	static BOOL CloseHandler(DWORD event)
	{
		if (event == CTRL_CLOSE_EVENT)
		{
			bGameRunning = false;
		}
		return true;
	}
private:
	int nScreenWidth;
	int nScreenHeight;
	CHAR_INFO* bufScreen;
	std::wstring sAppName = L"Tetris";
	HANDLE hOriConsole;
	HANDLE hConsole;
	CONSOLE_SCREEN_BUFFER_INFO oriConsoleInfo;
	SMALL_RECT srRectWin;
	static bool bGameRunning;

	std::wstring map;
	std::wstring block[7];
	TETRO* tetro;
	TETRO* hold;
	bool bKPress;
	bool bRot;
	bool bBlock;
	bool bUpdate;
	bool bPause;
	bool bHold;
	bool bNext;
	int nIter;
	int nMapWidth;
	int nMapHeight;
	std::pair<int, int> tmp[7];
	int blocks[14];
};

bool TETRIS::bGameRunning;

int main()
{
	TETRIS tetris;
	tetris.start();
	return 0;
}