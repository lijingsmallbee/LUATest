// userDataTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
extern "C"
{
#include "../Lua/Lua/lauxlib.h"
};

class A
{
private:
	int a;
	float b;
public:
	A():
	a(0),
	b(0.0f)
	{

	}
	virtual ~A(){}
	void Set(int ia,float fb) { a = ia,b = fb;std::cout<<a<<b;}
};
void CreateType(lua_State* L,const char* type)
{
	luaL_newmetatable( L, type);
	lua_pushstring(L,type);
	lua_rawset(L,LUA_REGISTRYINDEX);

	luaL_getmetatable(L,type);
	lua_pushstring(L,"__index");
	lua_pushvalue(L,-2);
	lua_rawset(L,-3);   //设置__index
}
A* pStaticObj = NULL;
int NewA( lua_State* L )
{
	void* data =  lua_newuserdata( L, sizeof(A*) );
	A* pObject = new A;
	memcpy(data,&pObject,sizeof(A*));   //注意取地址，否则就把A对象的一部分copy过去了
	pStaticObj = pObject; 
	//在注册表内添加A* 到userdata的映射，这步骤不做，可能lua函数中调用后，userdata就          //被垃圾回收了，实际是可以再次使用的

	lua_pushlightuserdata(L,pObject);
	lua_pushvalue(L,-2);   //把userdata复制一份儿，否则settable会从栈顶移除

	lua_rawset(L,LUA_REGISTRYINDEX);          //  注册表[pObject] = userdata

	lua_pushvalue(L,-1);    // 再次复制userdata
	luaL_getmetatable( L, "A");
	lua_setmetatable( L, -2 );    //设置userdata的metatable        

	return 1;
}

int SetA(lua_State* L)
{
	if(lua_isuserdata(L,1))
	{
		if(lua_getmetatable(L,1))
		{
			lua_rawget(L,LUA_REGISTRYINDEX);//metatable的name到了栈-1位置
			const char*  name = lua_tostring(L,-1);
			bool isAType = strcmp(name,"A") == 0;
			lua_pop(L,1);              //弹出name
			if(isAType)
			{
				A* p = *(A**)lua_touserdata(L,1);
				if(p)
				{
					int a = (int)lua_tonumber(L,2);
					float b = lua_tonumber(L,3);
					p->Set(a,b);
					return 1;
				}
			}
		}
	}
	return 0;
}

void TestCallLUA(lua_State* L,A* p)
{
	lua_getglobal(L,"TestA");
	if(lua_isfunction(L,-1))
	{
		lua_pushlightuserdata(L,(void*)p);
		lua_rawget(L,LUA_REGISTRYINDEX);
		if(lua_isuserdata(L,-1))
		{
			lua_pcall(L,1,0,0);
		}
	}
}
void InitLUACallBack(lua_State* L)
{
	lua_pushcfunction( L, NewA );
	lua_setglobal(L,"CreateA");

	luaL_getmetatable( L, "A");
	lua_pushstring( L, "SetA" );
	lua_pushcfunction( L, SetA );
	lua_rawset( L, -3 ); // A.SetA = SetA;
}

int _tmain(int argc, _TCHAR* argv[])
{
	lua_State* pState = lua_open();
	//	luaL_openlibs(pState);
	CreateType(pState,"A");
	InitLUACallBack(pState);
	
	luaL_dostring(pState, "local x = CreateA(); x:SetA(2,3.0)");

	luaL_dostring(pState,"function TestA(x) x:SetA(5,6.0); end;");

	TestCallLUA(pState,pStaticObj);
	return 0;
}

