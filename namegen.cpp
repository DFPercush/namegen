
// Name generator
// Daniel Papenburg 2020
// Generates next letter based on the chance of that letter occurring
// in the English dictionary based on the previous 2 letters.
// No dialect or style is implemented, the goal is simply to be
// pronouncable.

#include <iostream>
#include <fstream>
#include <time.h>
#include <string>
#include <conio.h>
#include <filesystem>
#include "PRNG.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

char charAt(const string& str, int pos)
{
	if (pos < 0 || pos >= (int)str.length()) return 0;
	return str[pos];
}

bool isAlpha(const string& s)
{
	char c;
	for (unsigned int ci = 0; ci < s.length(); ci++)
	{
		c = s[ci];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c >= 'Z')){}
		else return false;
	}
	return true;
}


int nextOccur[256][256];
int next2[256][256][256];

struct Ended
{
	float averageLength;
	int count;
};
Ended ended[256];


struct Chance
{
	float minRoll;
	float maxRoll;
};

Chance chances[256][256]; // chance of each char given previous: [prev][cur]
Chance chances2[256][256][256]; // chance of each char given previous 2: [pos-2][pos-1][cur]

int main(int argc, char** argv)
{
	int namesPerBatch = 10;
	bool autoQuit = false;
	for (int iArg = 1; iArg < argc; iArg++)
	{
		if (!strcmp(argv[iArg], "-n") && (argc - 1 > iArg))
		{
			iArg++;
			namesPerBatch = atoi(argv[iArg]);
		}
		if (!strcmp(argv[iArg], "-q"))
		{
			autoQuit = true;
		}
	}

	memset(nextOccur, 0, 0x10000 * sizeof(int));
	memset(chances, 0, 0x10000 * sizeof(Chance));
	memset(ended, 0, 0x100 * sizeof(Ended));
	memset(chances2, 0, 0x1000000 * sizeof(Chance));
	memset(next2, 0, 0x1000000 * sizeof(int));

	if (!autoQuit) { cout << "Analyzing dictionary...\n\n"; }

	ifstream dic;
	
	wchar_t moduleFileName[256];
	#if defined(_WIN32)
		GetModuleFileNameW(NULL, moduleFileName, sizeof(moduleFileName) / 2);
	#else
		// TODO: readlink("/proc/self/exe" ...
		strcpy(argv[0], moduleFileName);
	#endif

	auto myPath = filesystem::absolute(filesystem::path(moduleFileName)).remove_filename();

	string wordsPath = myPath.string() + "words.txt";

	dic.open(wordsPath.c_str());
	if (!dic.is_open())
	{
		cout << "Can not open word dictionary at " << wordsPath << endl;
		return 1;
	}
	int maxValue = 0;
	string line;
	char cur = 0, prev = 0, prev2 = 0;

	while (!dic.eof())
	{
		cur = prev = prev2 = 0;
		getline(dic, line);
		if (!isAlpha(line)) continue;
		for (unsigned int i = 0; i < line.length(); i++)
		{
			prev2 = prev;
			prev = cur;
			cur = line[i];
			nextOccur[prev][cur]++;
			next2[prev2][prev][cur]++;
		}
		nextOccur[cur][0]++;
		next2[prev][cur][0]++;
		if (ended[cur].count == 0) ended[cur].averageLength = (float)line.length();
		else ended[cur].averageLength = ended[cur].averageLength + line.length() / ended[cur].count;
		ended[cur].count++;
	}
	dic.close();

	for (int iPrev2 = 0; iPrev2 < 256; iPrev2++)
	{
		for (int iPrev1 = 0; iPrev1 < 256; iPrev1++)
		{
			int ct2 = 0;
			for (int iNextLetter = 0; iNextLetter < 256; iNextLetter++)
			{
				ct2 += next2[iPrev2][iPrev1][iNextLetter];
			}
			float nextChanceStart = 0.0f;
			for (int iNextLetter = 0; iNextLetter < 256; iNextLetter++)
			{
				chances2[iPrev2][iPrev1][iNextLetter].minRoll = nextChanceStart;
				nextChanceStart += (float)next2[iPrev2][iPrev1][iNextLetter] / (float)ct2;
				chances2[iPrev2][iPrev1][iNextLetter].maxRoll = nextChanceStart;
			}
		}
	}

	auto now = time(nullptr);
	srand((unsigned int)now);
	int key;
	if (!autoQuit) { cout << "Press space to generate more, or q to quit.\n\n"; }
	PRNG rng;

	string name{""};
	do
	{
		for (int x = 0; x < namesPerBatch; x++)
		{
			char c, prev1, prev2;
			name.clear();
			c = 'a' + rng.getRandomChar(0, 25);
			prev1 = prev2 = 0;
			bool lengthOverride = false;
			int lengthToContinue;
			do
			{
				if (!lengthOverride) name += c;
				prev2 = prev1;
				prev1 = c;
				lengthOverride = false;
				float roll = rng.getRandomFloat();
				for (int iChance = 0; iChance < 256; iChance++)
				{
					if (roll >= chances2[prev2][prev1][iChance].minRoll && roll < chances2[prev2][prev1][iChance].maxRoll)
					{
						if (iChance == 0 && name.length() < 6)
						{
							// Too short, keep generating
							lengthOverride = true;
							c = 'a';
						}
						else
						{
							c = (char)iChance;
						}
						break;
					}
				}
				lengthToContinue = rng.getRandomInt(6, 16);
			} while (c != 0 && lengthToContinue > (int)name.length());
			cout << name << endl;
		}
		cout << endl;
		cout.flush();
		if (!autoQuit)
		{
			key = _getch();
		}
		
	} while ((!autoQuit) && (key != 'q'));
}
