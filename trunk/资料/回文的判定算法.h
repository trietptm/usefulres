������C++���ĵ��ж��㷨��

���ľ�������������һ�����ַ���

#include < iostream> 
#include < time.h> 
using namespace std;  

bool Find(char *p,int length)  
{  
	if(length< 0)  
		return true;  
	else if(*p==p[length-1])  
		Find(p+1,length-2);  
	else  
		return false;  
}  
void main()  
{  
	//long beginTime=clock();  
	char pa[3];  
	cin>>pa;  
	cout< < Find(pa,strlen(pa))< < endl;  
	//long endTime=clock();  
	//cout< < endTime-beginTime< < endl;  
} 