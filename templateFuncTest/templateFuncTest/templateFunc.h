#ifndef _TEMPLATE_FUNCTION_H_
#define _TEMPLATE_FUNCTION_H_
extern "C"
{
	#include "../lua/Lua/lua.h"
};

class FunctionContext
{
public:
	FunctionContext(lua_State* pState)
	{
		m_pState = pState;
	}
	~FunctionContext(){}
	//这里需要特化多种模版，做测试就写一种就ok了
	template<class T>
	bool GetParam(int index,T& value){ return true;}

	template<>
	bool GetParam(int index,double& value)
	{
		//这里可以加入一些校验逻辑，防止程序crash
		value = lua_tonumber(m_pState,index);
		return true;
	}
private:
	lua_State* m_pState;

};
template<typename caller>
static int CallFunction(caller* p,int (caller::*pFunc)(FunctionContext*),FunctionContext* pContext)
{
	return (p->*pFunc)(pContext);
}
template<typename caller,typename Param1>
static int CallFunction(caller* p,int (caller::*pFunc)(FunctionContext*,Param1),FunctionContext* pContext)
{
	Param1 p1;
	pContext->GetParam(1,p1);
	return (p->*pFunc)(pContext,p1);
}

template<typename caller,typename Param1,typename Param2>
static int CallFunction(caller* p,int (caller::*pFunc)(FunctionContext*,Param1,Param2),FunctionContext* pContext)
{
	Param1 p1;
	Param2 p2;
	pContext->GetParam(1,p1);
	pContext->GetParam(2,p2);
	return (p->*pFunc)(pContext,p1,p2);
}

//这个类的作用就是，当把this指针和function指针放入lua的userdata后，必须保存function的模版类型信息，比如，有几个参数，参数类型
//那么就可以利用这个dispatcher通过他的实例化得到一个静态函数，这个静态函数也放入到userdata中
//简化版本，userdata中有dispatcher的静态函数，真正c++回调函数的指针，对象的this指针，当然也可以加入其他一些内容，比如函数签名，跟类型相关的逻辑，防止重名等
typedef int (*dispatcherFunc)(FunctionContext* pContext,const char* pUserData);
template<typename caller,typename Func>
class dispatcher
{
public:
	static int CallFunc(FunctionContext* pContext,const char* pUserData)
	{
		//这个地方实现了参数展开的功能，通过注册的时候的模版参数实例化，创建了一个针对该参数个数和类型的回调函数的调用函数
		return CallFunction((caller*)pUserData,*(Func*)(pUserData+sizeof(caller*)),pContext);
	}
};

int luaShellFuc(lua_State* L)
{
	FunctionContext context(L);
	//从栈中取出userdata
	unsigned char *pBuffer = (unsigned char*)lua_touserdata(L,lua_upvalueindex(1));
	//从userdata中取出dispatcher函数
	dispatcherFunc* pfunc = (dispatcherFunc*)pBuffer;
	(*pfunc)(&context,(const char*)pBuffer+ sizeof(dispatcherFunc*));
	return 1;
}

template <typename caller,typename func>
void RegisterFunc(lua_State* pState,const char* funcName,caller*p,const func& function)
{
	unsigned nTotalSize = sizeof(dispatcherFunc*) + sizeof(caller*) + sizeof(func);
	unsigned char *pBuffer = (unsigned char*)lua_newuserdata(pState, nTotalSize );
	int offset = 0;
	dispatcherFunc disfunc = dispatcher<caller,func>::CallFunc;
	//这里一定要取地址，否则会把函数的一部分代码copy过去，而不是入口地址
	/*参考这个例子好理解，int* p = new int;
	memcpy(a,p,sizeof(int));
	memcpy(a,&p,sizeof(int*));
	一个是把int的值copy进去，一个是把指向这个int的地址copy过去
	拷贝p的值，就要对p取地址，拷贝p指向的值，就用p当地址，我们这边要得到函数地址，显然是拷贝disfunc的值，而不是他指向的函数
	*/
	memcpy(pBuffer,&disfunc,sizeof(dispatcherFunc));
	offset += sizeof(dispatcherFunc);
	//此处同理，拷贝指针的值，不是对象，奇怪的是，把p前边的&去掉依然是正确的，亲测。。。求达人解答
	memcpy(pBuffer+offset,&p,sizeof(caller*));
	offset += sizeof(caller*);
	memcpy(pBuffer+offset,&function,sizeof(function));
	lua_pushcclosure( pState,luaShellFuc,1 );
	//这里可以根据自己需要放到特定表里，做简化
	lua_setglobal(pState,funcName);
}
#endif
