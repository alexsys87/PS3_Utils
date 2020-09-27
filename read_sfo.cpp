#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>

#include "sfo_file.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"

int main(int argc, char* argv[])
{
	if (argc >= 3)
	{
		PrintSfo(argv[1], argv[2]);
	}
	else
	{
		PrintSfo(argv[1], ""); //Print All
	}
}