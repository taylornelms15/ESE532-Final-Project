
#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include "lzw_sw.h"


#define CODE_LENGTH (13)
#define MAX_CHUNK_SIZE (1<<CODE_LENGTH)
#define TESTSIZE (MAX_CHUNK_SIZE)
static const int MAXCHAR = 256;

typedef std::vector<std::string> code_table;
typedef std::vector<std::string> chunk_list;
static code_table Code_table;

//static std::ifstream Input;
static uint8_t* dInput;
static int dInputLoc = 0;
static size_t Input_position;

static int Read_code(void)
{
  static unsigned char Byte;

  int Code = 0;
  int Length = CODE_LENGTH;
  for (int i = 0; i < Length; i++)
  {
	  if (Input_position % 8 == 0)
		  //Byte = Input.get();
		  Byte = dInput[dInputLoc++];
    Code = (Code << 1) | ((Byte >> (7 - Input_position % 8)) & 1);
    Input_position++;
  }
  return Code;
}

static const std::string Decompress(size_t Size)
{
  Input_position = 0;

  Code_table.clear();
  for (int i = 0; i < 256; i++)
    Code_table.push_back(std::string(1, (char) i));

  int Old = Read_code();
  std::string Symbol(1, Old);
  std::string Output = Symbol;
  while (Input_position / 8 < Size - 1)
  {
    int New = Read_code();
    std::string Symbols;
    if (New >= (int) Code_table.size())
      Symbols = Code_table[Old] + Symbol;
    else
      Symbols = Code_table[New];
    Output += Symbols;
    Symbol = std::string(1, Symbols[0]);
    Code_table.push_back(Code_table[Old] + Symbol);
    Old = New;
  }

  return Output;
}

void printCommandArgs(int argc, char** argv) {
	printf("Argc: %d\n", argc);
	printf("Argv: ");
	for (int i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
}

int main(int argc, char* argv[])
{
	srand(0xbadbad12);

	uint8_t* randomVals = (uint8_t*)malloc(TESTSIZE);
	uint8_t* randomValsCompressed = (uint8_t*)malloc((int)(TESTSIZE * 13.0 / 8.0));
	
	printCommandArgs(argc, argv);


	if (argc < 2) {
		for (int i = 0; i < TESTSIZE; i++) {
			randomVals[i] = rand() % MAXCHAR;
			if (randomVals[i] < '0' || randomVals[i] > 'z') {
				i--;
			}//force ascii input/output
		}
		randomVals[TESTSIZE - 2] = '\n';
		randomVals[TESTSIZE - 1] = 0;
		std::string frontString = std::string("This thin sentence reads, sphinx of black quartz, judge my vow.\n");
		memcpy(randomVals, frontString.data(), frontString.size());
	}
	else {
		std::FILE* f;
		fopen_s(&f, argv[1], "r");
		if (!f) {
			printf("errno %d\n", errno);
			exit(1);
		}
		int i = 0;
		while (i < TESTSIZE) {
			char nextChar;
			int numRead = std::fread(&nextChar, 1, 1, f);
			if (!numRead) {
				randomVals[i++] = '\n';
				randomVals[i++] = 0;
				break;
			}
			randomVals[i++] = nextChar;
		}
		randomVals[TESTSIZE - 2] = '\n';
		randomVals[TESTSIZE - 1] = 0;

	}


	std::string inString = std::string((char*)randomVals, TESTSIZE);
	printf(inString.c_str());

	int compressLength = lzwCompress(randomVals, TESTSIZE, randomValsCompressed);
	printf("=========SIZE %d========\n", compressLength);


	//decompress for testing
	dInput = randomValsCompressed + 4;
	std::string out = Decompress(compressLength - 4);

	printf(out.c_str());

	free(randomVals);
	free(randomValsCompressed);
	return 0;
}
