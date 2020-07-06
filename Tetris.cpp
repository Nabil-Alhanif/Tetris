#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>
#include <Windows.h>

struct TETRO
{
public:
	int xPos;
	int yPos;
	std::wstring tets;

	TETRO(std::wstring ws)
	{
		tets = ws;
		xPos = 2;
		yPos = 0;
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
		for (int nx = 0; nx < 4; nx++)
		{
			for (int ny = 0; ny < 4; ny++)
			{
				if (tets.c_str()[ny * 4 + nx] != ' ')
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
			for (int ny = 0; ny < 4; ny++)
			{
				for (int nx = 0; nx < 4; nx++)
				{
					tmp[ny * 4 + nx] = this->tets[nt + nx * 4];
				}
				nt--;
			}
		}
		else
		{
			int nt = 12;
			for (int ny = 0; ny < 4; ny++)
			{
				for (int nx = 0; nx < 4; nx++)
				{
					tmp[ny * 4 + nx] = this->tets[nt - nx * 4];
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
		block[3] += L"    ";
		block[3] += L" ZZ ";
		block[3] += L"  ZZ";

		// Green
		block[4] += L"    ";
		block[4] += L"    ";
		block[4] += L" SS ";
		block[4] += L"SS  ";

		// Purple
		block[5] += L"    ";
		block[5] += L" T  ";
		block[5] += L"TTT ";
		block[5] += L"    ";
		
		// Yellow
		block[6] += L"    ";
		block[6] += L" OO ";
		block[6] += L" OO ";
		block[6] += L"    ";

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleWindowInfo(hOriConsole, TRUE, &oriConsoleInfo.srWindow);
		SetConsoleScreenBufferSize(hOriConsole, oriConsoleInfo.dwSize);

		TETRIS::CreateConsole(48, 84, 8, 8);
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
	void blit(int x, int y, short c = 0x2588, short col = 0x000F)
	{
		if (x >= 0 && x < nScreenWidth && y >= 0 && y < nScreenHeight)
		{
			int pos = y * nScreenWidth * 4 + x * 4;
			if (c == 'I')col = 0x0009;
			if (c == 'L')col = 0x0006;
			if (c == 'J')col = 0x0001;
			if (c == 'Z')col = 0x0004;
			if (c == 'S')col = 0x000A;
			if (c == 'T')col = 0x000D;
			if (c == 'O')col = 0x000E;
			if (c >= 'A' && c <= 'Z')c = 0x2588;
			for (int ny = 0; ny < 4; ny++)
			{
				for (int nx = 0; nx < 4; nx++)
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
		for (int i = 0; i < 7; i++)
		{
			blocks[i] = { std::rand(),i };
		}
		std::sort(blocks, blocks + 7);
		tetro = new TETRO(block[blocks[nIter].second]);

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
					for (int ny = 3; ny >= 0; ny--)
					{
						int tmp = ny + curY + c;
						if (tmp > 23)
							continue;
						if (map[tmp * 12 + 1] == '=')
						{
							c++;
							while (tmp > 0)
							{
								for (int nx = 1; nx < 11; nx++)
								{
									map[tmp * 12 + nx] = map[(tmp - 1) * 12 + nx];
									map[(tmp - 1) * 12 + nx] = ' ';
								}
								tmp--;
							}
						}
					}
					bUpdate = 0;
				}

				// Handle Input
				// Down
				if ((GetAsyncKeyState((unsigned short)'S') & 0x8000) && !bKPress && !bUpdate)
				{
					bKPress = 1;
					bDMove = 1;
					tp4 = tp2;
				}

				// Right
				if ((GetAsyncKeyState((unsigned short)'D') & 0x8000) && !bKPress && !bUpdate)
				{
					bKPress = 1;
					tp3 += std::chrono::milliseconds(100);
					if (tp2 < tp3)tp3 = tp2;
					tp4 = tp2;
					if (!tetro->isCollide(1, 0, this->map))
						tetro->xPos++;
				}

				// Left
				if ((GetAsyncKeyState((unsigned short)'A') & 0x8000) && !bKPress && !bUpdate)
				{
					bKPress = 1;
					tp3 += std::chrono::milliseconds(00);
					if (tp2 < tp3)tp3 = tp2;
					tp4 = tp2;
					if (!tetro->isCollide(-1, 0, this->map))
						tetro->xPos--;
				}

				// Rotate Right
				if ((GetAsyncKeyState((unsigned short)'E') & 0x8000) && !bRot && !bUpdate)
				{
					bRot = 1;
					tp3 += std::chrono::milliseconds(00);
					if (tp2 < tp3)tp3 = tp2;
					tp5 = tp2;
					tetro->rotate(0, 1, this->map);
				}

				// Rotate Left
				if ((GetAsyncKeyState((unsigned short)'Q') & 0x8000) && !bRot && !bUpdate)
				{
					bRot = 1;
					tp3 += std::chrono::milliseconds(00);
					if (tp2 < tp3)tp3 = tp2;
					tp5 = tp2;
					tetro->rotate(1, 0, this->map);
				}

				// Move down every second
				if (tp3 + std::chrono::seconds(1) <= tp2 && !bUpdate)
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
						for (int nx = 0; nx < 4; nx++)
						{
							for (int ny = 0; ny < 4; ny++)
							{
								char c = tetro->getChar(nx, ny);
								if (c != ' ')
									map[(curY + ny) * 12 + (tetro->xPos + nx)] = c;
							}
						}
						for (int ny = 0; ny < 4 && tetro->yPos + ny < 24; ny++)
						{
							int tmp = (curY + ny) * 12;
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

				// Regenrate tetris block
				if (!bBlock)
				{
					bBlock = 1;
					nIter++;
					if (nIter > 6)
					{
						nIter = 0;
						for (int i = 0; i < 7; i++)
						{
							blocks[i] = { std::rand(),i };
						}
						std::sort(blocks, blocks + 7);
					}
					tetro = new TETRO(block[blocks[nIter].second]);
				}

				if (tp4 + std::chrono::milliseconds(200) <= tp2)
					bKPress = 0;
				if (tp5 + std::chrono::milliseconds(150) <= tp2)
					bRot = 0;

				/**************************************************************/

				// Draw map
				for (int nx = 0; nx < 12; nx++)
				{
					for (int ny = 0; ny < 21; ny++)
					{
						blit(nx, ny, map[(ny+4) * 12 + nx]);
					}
				}

				// Draw block
				for (int nx = 0; nx < 4; nx++)
				{
					for (int ny = 0; ny < 4; ny++)
					{
						if (ny + tetro->yPos - 4 >= 0 && tetro->getChar(nx, ny) != ' ')
							blit(nx + tetro->xPos, ny + tetro->yPos - 4, tetro->tets[ny * 4 + nx]);
					}
				}

				// Update screen title and buffer
				wchar_t title[256];
				swprintf_s(title, 256, L"%s - FPS: %3.2f", sAppName.c_str(), 1.0f / fElapsedTime);
				SetConsoleTitle(title);
				WriteConsoleOutput(hConsole, bufScreen, { (short)nScreenWidth, (short)nScreenHeight }, { 0,0 }, &srRectWin);
			}
			for (int nx = 0; nx*4 < nScreenWidth; nx++)
			{
				for (int ny = 0; ny*4 < nScreenHeight; ny++)
				{
					blit(nx, ny, ' ');
				}
			}
			WriteConsoleOutput(hConsole, bufScreen, { (short)nScreenWidth, (short)nScreenHeight }, { 0,0 }, &srRectWin);
			// Free memory
			if (bufScreen)delete[] bufScreen;
			if (tetro)delete tetro;
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
	bool bKPress;
	bool bRot;
	bool bBlock;
	bool bUpdate;
	int nIter;
	std::pair<int, int> blocks[7];
};

bool TETRIS::bGameRunning;

int main()
{
	TETRIS tetris;
	tetris.start();
	return 0;
}