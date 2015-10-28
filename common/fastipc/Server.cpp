#include "Server.h"

namespace fastipc{
	Server::Server(){
		evtWrited = NULL;
		evtReaded = NULL;
		mapFile = NULL;
		memBuf = NULL;
	};

	Server::~Server(void)	{
		close();
	};

	void Server::close(void){
		if (evtWrited) {// �ر��¼����
			HANDLE handle = evtWrited;
			evtWrited = NULL;
			CloseHandle(handle);
		}
		if (evtReaded) {// �ر��¼����
			HANDLE handle = evtReaded;
			evtReaded = NULL;
			CloseHandle(handle);
		}
		if (memBuf) {// ȡ���ڴ�ӳ��
			MemBuff *pBuff = memBuf;
			memBuf = NULL;
			UnmapViewOfFile(pBuff);
		}
		if (mapFile) {// �ر�ӳ���ļ�
			HANDLE handle = mapFile;
			mapFile = NULL;
			CloseHandle(handle);
		}
	};
	void Server::setListener(ReadListener* listener){
		this->listener = listener;
	};
	bool Server::isStable(){
		return memBuf != NULL;
	};
	int Server::create(const std::wstring serverName)
	{
		//this->serverName = name;
		// ���������¼��ֱ�����֪ͨ�ɶ�����д
		evtWrited = CreateEvent(NULL, FALSE, FALSE, LPTSTR(genWritedEventName(serverName).c_str()));
		if (evtWrited == NULL || evtWrited == INVALID_HANDLE_VALUE)  return ERR_EventOpen_W;
		evtReaded = CreateEvent(NULL, FALSE, FALSE, LPTSTR(genReadedEventName(serverName).c_str()));
		if (evtReaded == NULL || evtReaded == INVALID_HANDLE_VALUE) return ERR_EventOpen_R;

		// �����ڴ�ӳ���ļ�
		mapFile = CreateFileMapping(INVALID_HANDLE_VALUE,	// ����һ���������ļ��޹ص�ӳ���ļ�
			NULL,											// ��ȫ����
			PAGE_READWRITE,									// �򿪷�ʽ
			0,												// �ļ�ӳ�����󳤶ȵĸ�32λ��
			sizeof(MemBuff),								// �ļ�ӳ�����󳤶ȵĵ�32λ��
			LPTSTR(genMappingFileName(serverName).c_str()));// ӳ���ļ�������
		if (mapFile == NULL || mapFile == INVALID_HANDLE_VALUE)  return ERR_MappingCreate;

		// ӳ���ļ����ڴ�
		memBuf = (MemBuff*)MapViewOfFile(mapFile,			// ӳ���ļ����
			FILE_MAP_ALL_ACCESS,							// ��дȨ��
			0, 0,											// ӳ����ʼƫ�Ƶĸߡ���32λ
			sizeof(MemBuff));								// ָ��ӳ���ļ����ֽ���
		if (memBuf == NULL)return ERR_MappingMap;			// ӳ���ļ�ʧ��
		ZeroMemory(memBuf, sizeof(MemBuff));				// ��ջ�����
		return 0;
	};


	void Server::startRead(){
		// ��ȡΪ���̶߳�ȡ�����Բ���Ҫ������state��ǰ�����״̬λ�ļ��
		//if (memBuf->state != MEM_CAN_READ) continue;  // ��ǰ���������ǿɶ�״̬����������ʹ�ã������ȴ�
		//if (memBuf->state != MEM_IS_BUSY)continue;  // ������ú��ǿ���״̬����ô�����Ǳ��������̰����ݶ����ˣ���ʱ�����ȴ� 
		while (memBuf){
			DWORD dwWaitResult = WaitForSingleObject(evtWrited, INFINITE);// �ȴ�д���¼�
			if (dwWaitResult == WAIT_OBJECT_0){
				InterlockedCompareExchange(&memBuf->state, MEM_IS_BUSY, MEM_CAN_READ);// ͨ��ԭ�Ӳ��������ù�������״̬Ϊ��״̬
				MemBuff * rtn = new MemBuff();
				rtn->dataLen = memBuf->dataLen;
				rtn->msgType = memBuf->msgType;
				memcpy(rtn->data, memBuf->data, rtn->dataLen);
				if (memBuf->msgType > MSG_TYPE_NORMAL){
					ZeroMemory(rtn->packId, PACK_ID_LEN);
					memcpy(rtn->packId, memBuf->packId, PACK_ID_LEN);
				}
				if (listener != NULL)listener->onRead(rtn);
				InterlockedExchange(&memBuf->state, MEM_CAN_WRITE);// ���ݶ�ȡ֮������Ϊ��д
				SetEvent(evtReaded);
			}
			else{
				break;// ���������ѭ������
			}
		}
	};
}