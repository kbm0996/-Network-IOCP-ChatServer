#ifndef __NETWORK_HEADER__
#define __NETWORK_HEADER__

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#include <io.h>
#include "CNPacket.h"
#include "CRingBuffer.h"
#include "CLFQueue.h"
#include "CLFStack.h"
#include <list>
#include <map>

#include "CommonProtocol.h"

#include "CParser.h"

#include "CNetServer.h"
#include "CLanClient.h"

#include <conio.h>

#endif // !__NETWORK_HEADER__
