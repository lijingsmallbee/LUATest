// templateFuncTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "templateFunc.h"
#include "string.h"
extern "C"
{
	#include "../Lua/Lua/lauxlib.h"
};

class TestClass
{
public:
	int func0(FunctionContext* pContext)
	{
		std::cout<<"0";
		return 1;
	}
	int func1(FunctionContext* pContext,double testA)
	{
		std::cout<<"1";
		return 1;
	}
	int func2(FunctionContext* pContext,double param1,double param2)
	{
		std::cout<<"2";
		return 2;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	lua_State* pState = lua_open();
//	luaL_openlibs(pState);
	TestClass test;
	int * p = NULL;
	const int& x = *p;
	const int * m = &x;
	RegisterFunc(pState,"func0",&test,&TestClass::func0);
	RegisterFunc(pState,"func1",&test,&TestClass::func1);
	RegisterFunc(pState,"func2",&test,&TestClass::func2);

	luaL_dostring(pState, "func1(2.0) func2(1.0,2.0)");


	return 0;
}

