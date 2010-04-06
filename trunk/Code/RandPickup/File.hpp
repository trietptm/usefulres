#pragma once
class File
{
protected:
	FILE *m_fp;
public:
	File()
	{
		m_fp = NULL;
	}
	File(LPCSTR lpFile,LPCSTR lpMod)
	{
		m_fp = ::fopen(lpFile,lpMod);
	}
	BOOL Open(LPCSTR lpFile,LPCSTR lpMod)
	{
		ASSERT(!m_fp);
		m_fp = ::fopen(lpFile,lpMod);
		return m_fp!=NULL;
	}
	operator FILE*()
	{
		return m_fp;
	}
	~File()
	{
		if(m_fp)
			::fclose(m_fp);
	}
};