// MyTestAntiDebuger.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>


int main()
{
    while (true)
    {
        BOOL result = IsDebuggerPresent();
        printf("result = %d.\n", result);

        Sleep(1 * 1000);
    }
    return 0;
}

