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
	//������Ҫ�ػ�����ģ�棬�����Ծ�дһ�־�ok��
	template<class T>
	bool GetParam(int index,T& value){ return true;}

	template<>
	bool GetParam(int index,double& value)
	{
		//������Լ���һЩУ���߼�����ֹ����crash
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

//���������þ��ǣ�����thisָ���functionָ�����lua��userdata�󣬱��뱣��function��ģ��������Ϣ�����磬�м�����������������
//��ô�Ϳ����������dispatcherͨ������ʵ�����õ�һ����̬�����������̬����Ҳ���뵽userdata��
//�򻯰汾��userdata����dispatcher�ľ�̬����������c++�ص�������ָ�룬�����thisָ�룬��ȻҲ���Լ�������һЩ���ݣ����纯��ǩ������������ص��߼�����ֹ������
typedef int (*dispatcherFunc)(FunctionContext* pContext,const char* pUserData);
template<typename caller,typename Func>
class dispatcher
{
public:
	static int CallFunc(FunctionContext* pContext,const char* pUserData)
	{
		//����ط�ʵ���˲���չ���Ĺ��ܣ�ͨ��ע���ʱ���ģ�����ʵ������������һ����Ըò������������͵Ļص������ĵ��ú���
		return CallFunction((caller*)pUserData,*(Func*)(pUserData+sizeof(caller*)),pContext);
	}
};

int luaShellFuc(lua_State* L)
{
	FunctionContext context(L);
	//��ջ��ȡ��userdata
	unsigned char *pBuffer = (unsigned char*)lua_touserdata(L,lua_upvalueindex(1));
	//��userdata��ȡ��dispatcher����
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
	//����һ��Ҫȡ��ַ�������Ѻ�����һ���ִ���copy��ȥ����������ڵ�ַ
	/*�ο�������Ӻ���⣬int* p = new int;
	memcpy(a,p,sizeof(int));
	memcpy(a,&p,sizeof(int*));
	һ���ǰ�int��ֵcopy��ȥ��һ���ǰ�ָ�����int�ĵ�ַcopy��ȥ
	����p��ֵ����Ҫ��pȡ��ַ������pָ���ֵ������p����ַ���������Ҫ�õ�������ַ����Ȼ�ǿ���disfunc��ֵ����������ָ��ĺ���
	*/
	memcpy(pBuffer,&disfunc,sizeof(dispatcherFunc));
	offset += sizeof(dispatcherFunc);
	//�˴�ͬ������ָ���ֵ�����Ƕ�����ֵ��ǣ���pǰ�ߵ�&ȥ����Ȼ����ȷ�ģ��ײ⡣��������˽��
	memcpy(pBuffer+offset,&p,sizeof(caller*));
	offset += sizeof(caller*);
	memcpy(pBuffer+offset,&function,sizeof(function));
	lua_pushcclosure( pState,luaShellFuc,1 );
	//������Ը����Լ���Ҫ�ŵ��ض��������
	lua_setglobal(pState,funcName);
}
#endif
