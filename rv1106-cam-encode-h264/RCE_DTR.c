/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_DTR.c
    Project name : RV1106 Camera Encode with h264
    Module name  : Data transfer
    Date created : 2025年6月6日   17时51分46秒
    Author       : 
    Description  : 
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "RCE_defines.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块包含文件        ==") ;
CODE_SECTION("==========================") ;

/* 标准库 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/* 第三方 */

/* 内部 */
#include "RCE_config.h"
#include "RCE_types.h"
#include "RCE_DTR.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块全局变量        ==") ;
CODE_SECTION("==========================") ;

/* 模块工作区 */
RCE_DTR_WORKAREA_S  g_stDTRWorkarea ;

/* 待发送数据缓存 */
RCE_DTR_FIFO_S      g_stDTRSendBuf ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块内部函数        ==") ;
CODE_SECTION("==========================") ;

/*******************************************************************************
- Function    : __RCE_DTR_PullFIFO
- Description : 本函数从FIFO中取出数据。
- Input       : uiSize  : 取出数据量。
- Output      : pucDest : 取出数据的存储位置。
- Return      : UINT    : 实际取出的数据量。
- Others      :
*******************************************************************************/
UINT __RCE_DTR_PullFIFO(UCHAR *pucDest, UINT uiSize)
{
    UINT uiStoreSize ;
    UINT uiCopySize ;
    UINT uiRetVal ;

    /* 计算FIFO中数据大小 */
    if(g_stDTRSendBuf.uiRdPtr == g_stDTRSendBuf.uiWrPtr)
    {
        /*  |-------------------------------------------------------------|
         *  |                                                             |
         *  |-------------------------------------------------------------|
         *              ^
         *            wr_ptr
         *            rd_ptr
         */
        /* 读写指针相同，FIFO为空 */
        return 0 ;
    }
    else if(g_stDTRSendBuf.uiWrPtr > g_stDTRSendBuf.uiRdPtr)
    {
        /*  |-------------------------------------------------------------|
         *  |           ####################################              |
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            rd_ptr                              wr_ptr
         */
        /* 计算存储的数据的容量 */
        uiStoreSize = g_stDTRSendBuf.uiWrPtr - g_stDTRSendBuf.uiRdPtr ;

        /* 计算拷贝数据量 */
        uiCopySize = (uiStoreSize > uiSize)? uiSize : uiStoreSize ;

        /* 拷贝数据 */
        memcpy(pucDest, &g_stDTRSendBuf.aucData[g_stDTRSendBuf.uiRdPtr], uiCopySize) ;

        /* 修正指针 */
        g_stDTRSendBuf.uiRdPtr += uiCopySize ; /* 必定不会溢出 */

        return uiCopySize ;
    }
    else /* g_stDTRSendBuf.uiWrPtr < g_stDTRSendBuf.uiRdPtr */
    {
        /*  |-------------------------------------------------------------|
         *  |###########                                    ##############|
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            wr_ptr                              rd_ptr
         */
        /* 计算从当前位置到FIFO尾部的数据量 */
        uiStoreSize = RCE_DTR_SEND_BUFFER_SIZE - g_stDTRSendBuf.uiRdPtr ;

        /* 计算拷贝数据量 */
        uiCopySize = (uiStoreSize > uiSize)? uiSize : uiStoreSize ;

        /* 拷贝数据 */
        memcpy(pucDest, &g_stDTRSendBuf.aucData[g_stDTRSendBuf.uiRdPtr], uiCopySize) ;

        /* 修正指针 */
        g_stDTRSendBuf.uiRdPtr += uiCopySize ;

        if(g_stDTRSendBuf.uiRdPtr >= RCE_DTR_SEND_BUFFER_SIZE)
        {
            /* 只可能等于，不会大于 */
            g_stDTRSendBuf.uiRdPtr = 0 ;
        }

        pucDest                += uiCopySize ;
        uiSize                 -= uiCopySize ;
        uiRetVal                = uiCopySize ;

        if(0 != uiSize)
        {
            /* 这种情况说明从rd_ptr到尾部的有效数据空间不够拷贝的，需要再从头拷贝 */
            uiStoreSize = g_stDTRSendBuf.uiWrPtr ;

            /* 计算拷贝数据量 */
            uiCopySize = (uiStoreSize > uiSize)? uiSize : uiStoreSize ;

            /* 拷贝数据 */
            memcpy(pucDest, &g_stDTRSendBuf.aucData[0], uiCopySize) ;

            /* 修正指针 */
            g_stDTRSendBuf.uiRdPtr += uiCopySize ;

            uiRetVal += uiCopySize ;
        }

        return uiRetVal ;
    }
}


/*******************************************************************************
- Function    : __RCE_DTR_OpenSocket
- Description : 本函数打开一个socket。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_DTR_OpenSocket(VOID)
{
    g_stDTRWorkarea.iSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0) ;

    if(-1 == g_stDTRWorkarea.iSocket)
    {
        printf("File %s, func %s, line %d : Can not create socket. err : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_DTR_CloseSocket
- Description : 本函数关闭socket。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_DTR_CloseSocket(VOID)
{
    close(g_stDTRWorkarea.iSocket) ;

    return ;
}


/*******************************************************************************
- Function    : __RCE_DTR_BindLocal
- Description : 本函数绑定本地地址和端口到socekt。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_DTR_BindLocal(VOID)
{
    UINT                uiIPAddr ;
    struct sockaddr_in  stLocalSockAddr ;
    int                 iRetVal = 0 ;

    uiIPAddr                        = inet_addr(RCE_LOCAL_IP) ;
    stLocalSockAddr.sin_family      = AF_INET ;
    stLocalSockAddr.sin_addr.s_addr = uiIPAddr ;
    stLocalSockAddr.sin_port        = htons(RCE_LOCAL_PORT) ;

    iRetVal = bind(g_stDTRWorkarea.iSocket, (struct sockaddr *)&stLocalSockAddr, sizeof(stLocalSockAddr));

    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : Bind local ip_addr failed. err : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_DTR_BindRemote
- Description : 本函数绑定远端地址和端口到socekt。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_DTR_BindRemote(VOID)
{
    UINT                uiIPAddr ;
    struct sockaddr_in  stRemoteSockAddr ;
    int                 iRetVal = 0 ;

    uiIPAddr                         = inet_addr(RCE_REMOTE_IP) ;
    stRemoteSockAddr.sin_family      = AF_INET ;
    stRemoteSockAddr.sin_addr.s_addr = uiIPAddr ;
    stRemoteSockAddr.sin_port        = htons(RCE_REMOTE_PORT) ;

    iRetVal = connect(g_stDTRWorkarea.iSocket, (struct sockaddr *)&stRemoteSockAddr, sizeof(stRemoteSockAddr)) ;

    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : Bind remote ip_addr failed. err : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_DTR_UpdateStatistics
- Description : 本函数更新统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_DTR_UpdateStatistics(VOID)
{
    STATIC UINT64 ui64PrevDataCountFromFIFO = 0 ;
    STATIC UINT64 ui64PrevDataCountToSocket = 0 ;

    /* 定时器信号，计算统计信息 */
    g_stDTRWorkarea.stStatistics.uiRunTime += RCE_STATISTIC_PERIOD_USEC ;

    /* 计算从FIFO中获取数据的BPS */
    g_stDTRWorkarea.stStatistics.uiCurrentBPSFromFIFO = (FLOAT)((g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO - ui64PrevDataCountFromFIFO) * 8) / 
                                                        ((FLOAT)RCE_STATISTIC_PERIOD_USEC / 1000000.0f) ;

    /* 计算发送往socket的数据的BPS */
    g_stDTRWorkarea.stStatistics.uiCurrentBPSToSocket = (FLOAT)((g_stDTRWorkarea.stStatistics.ui64DataCountToSocket - ui64PrevDataCountToSocket) * 8) / 
                                                        ((FLOAT)RCE_STATISTIC_PERIOD_USEC / 1000000.0f) ;

    /* 更新静态变量以便下一次计算 */
    ui64PrevDataCountFromFIFO = g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO ;
    ui64PrevDataCountToSocket = g_stDTRWorkarea.stStatistics.ui64DataCountToSocket ;

    return ;
}


CODE_SECTION("==========================") ;
CODE_SECTION("==  模块外部接口        ==") ;
CODE_SECTION("==========================") ;

/*******************************************************************************
- Function    : RCE_DTR_PushFIFO
- Description : 本函数向FIFO中储存数据。
- Input       : pucSrc : 储存数据位置。
                uiSize : 取出数据量。
- Output      : NULL
- Return      : UINT    : 实际储存的数据量。
- Others      :
*******************************************************************************/
UINT RCE_DTR_PushFIFO(UCHAR *pucSrc, UINT uiSize)
{
    UINT uiCopySize ;
    UINT uiFreeSize ;
    UINT uiRetVal ;

    /* 计算FIFO中空闲位置大小 */
    if(g_stDTRSendBuf.uiRdPtr <= g_stDTRSendBuf.uiWrPtr)
    {
        /*  |-------------------------------------------------------------|
         *  |%%%%%%%%%%%                                    %%%%%%%%%%%%%%|
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            rd_ptr                              wr_ptr
         */
        /* rd_ptr等于wr_ptr指针相同，FIFO为空 */
        /* rd_ptr小于wr_ptr，FIFO空闲空间分为2段 */
        /* 这里需要小心处理：如果rdptr = 0，则wrptr只能指向size - 1，否则再写入，导致双指针重合，FIFO被误认为空。
         * 如果rdptr != 0, 则wrptr可以写到0。
         */
        if (0 == g_stDTRSendBuf.uiRdPtr)
        {
            uiFreeSize = RCE_DTR_SEND_BUFFER_SIZE - g_stDTRSendBuf.uiWrPtr - 1 ; /* 确保最多写入到size-1大小，否则rdptr和wrptr会重合 */
        }
        else
        {
            uiFreeSize = RCE_DTR_SEND_BUFFER_SIZE - g_stDTRSendBuf.uiWrPtr ; /* 确保最多写入到size-1大小，否则rdptr和wrptr会重合 */
        }

        /* 计算拷贝大小 */
        uiCopySize = (uiFreeSize > uiSize)? uiSize : uiFreeSize ;

        if (0 != uiCopySize)
        {
            /* 拷贝数据 */
            memcpy(&g_stDTRSendBuf.aucData[g_stDTRSendBuf.uiWrPtr], pucSrc, uiCopySize) ;

            /* 调整指针 */
            g_stDTRSendBuf.uiWrPtr += uiCopySize ;

            if (RCE_DTR_SEND_BUFFER_SIZE <= g_stDTRSendBuf.uiWrPtr)
            {
                g_stDTRSendBuf.uiWrPtr = 0 ;
            }
        }
        else
        {
            return 0 ;
        }
        
        pucSrc   += uiCopySize ;
        uiSize   -= uiCopySize ;
        uiRetVal =  uiCopySize ;

        /* 此时有2种情况，1. 如果uiSize == 0，说明数据拷贝完成。2. 如果uiSize != 0，说明数据拷贝到了缓存尾部，还未拷贝完成，需要再次拷贝 */
        if(0 != uiSize)
        {
            /* 从缓存头部到rdptr之间的空间为空闲空间 */
            uiFreeSize = g_stDTRSendBuf.uiRdPtr - 1 ;

            uiCopySize = (uiFreeSize > uiSize)? uiSize : uiFreeSize ;

            /* 拷贝数据 */
            memcpy(&g_stDTRSendBuf.aucData[0], pucSrc, uiCopySize) ;

            /* 调整指针 */ /* 理论上不会溢出 */
            g_stDTRSendBuf.uiWrPtr += uiCopySize ;

            uiRetVal += uiCopySize ;
        }
        
        return uiRetVal ;
    }
    else /* if(g_stDTRSendBuf.uiRdPtr > g_stDTRSendBuf.uiWrPtr) */
    {
        /*  |-------------------------------------------------------------|
         *  |           %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              |
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            wr_ptr                              rd_ptr
         */
        /* 此时wrptr至rdptr之间的空间为空闲空间 */
        uiFreeSize = g_stDTRSendBuf.uiRdPtr - g_stDTRSendBuf.uiWrPtr - 1 ;

        /* 计算拷贝大小 */
        uiCopySize = (uiFreeSize > uiSize)? uiSize : uiFreeSize ;

        /* 拷贝数据 */
        memcpy(&g_stDTRSendBuf.aucData[g_stDTRSendBuf.uiWrPtr], pucSrc, uiCopySize) ;

        /* 调整指针 */
        g_stDTRSendBuf.uiWrPtr += uiCopySize ;
        
        return uiCopySize ;
    }
}

CODE_SECTION("==========================") ;
CODE_SECTION("==  线程入口            ==") ;
CODE_SECTION("==========================") ;
/*******************************************************************************
- Function    : RCE_DTR_Thread
- Description : 本函数为DTR模块的线程函数。
- Input       : VOID
- Output      : NULL
- Return      : VOID *
- Others      :
*******************************************************************************/
VOID *RCE_DTR_Thread(VOID *pvArgs)
{
    UCHAR           aucSendBuf[1000] ;
    UINT            uiSendSize ;
    INT             iRetVal = 0 ;

    memset(&g_stDTRWorkarea, 0, sizeof(g_stDTRWorkarea)) ;
    
    /* 指示线程正在运行 */
    g_stDTRWorkarea.enThreadStatus = DTR_THREAD_STATUS_RUN ;

    /* 创建socket */
    if(0 != __RCE_DTR_OpenSocket())
    {
        /* 创建socket失败，退出线程 */
        g_stDTRWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_NO_PROCESS ;
    }

    /* bind本地地址 */
    if(0 != __RCE_DTR_BindLocal())
    {
        /* bind本地地址失败，退出线程 */
        g_stDTRWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_SOCKET ;
    }

    /* bind远端地址 */
    if(0 != __RCE_DTR_BindRemote())
    {
        /* bind远端地址失败，退出线程 */
        g_stDTRWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_SOCKET ;
    }

    while(1)
    {
        usleep(1000) ;
LABEL_RETRY_SEND :
        /* 循环获取数据 */
        uiSendSize = __RCE_DTR_PullFIFO(aucSendBuf, sizeof(aucSendBuf)) ;

        /* 发送数据 */
        if(0 != uiSendSize)
        {
            /* 统计信息：更新从FIFO中获取的数据 */
            g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO += uiSendSize ;
LABEL_RETRY_SOCKET_SEND :
            iRetVal = send(g_stDTRWorkarea.iSocket, &aucSendBuf[0], uiSendSize, 0) ;

            if(-1 == iRetVal)
            {
                if(EAGAIN == errno)
                {
                    /* 如果errno值是EAGAIN，系统提示资源不够，可以重试 */
                    goto LABEL_RETRY_SOCKET_SEND ;
                }

                /* 如果errno值是ECONNREFUSED，说明对端（server端）没有开启，可以丢弃当前数据并重试 */
                if(ECONNREFUSED != errno)
                {
                    printf("File %s, func %s, line %d : Send data failed. err : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

                    goto LABEL_EXIT_WITH_CLOSE_SOCKET ;
                }
            }
            else
            {
                /* 统计信息：更新发送到Socket的数据 */
                g_stDTRWorkarea.stStatistics.ui64DataCountToSocket += iRetVal ;
            }

            if(uiSendSize == sizeof(aucSendBuf))
            {
                /* 从FIFO中取出了刚好‘aucSendBuf’大小的缓冲，说明FIFO中可能还有数据待发送 */
                goto LABEL_RETRY_SEND ;
            }
        }

        /* 更新统计信息 */
        if(0 != g_stDTRWorkarea.uiUpdateStatistics)
        {
            g_stDTRWorkarea.uiUpdateStatistics = 0 ;
            
            __RCE_DTR_UpdateStatistics() ;
        }

        if(1 == g_stDTRWorkarea.uiStopThread)
        {
            g_stDTRWorkarea.enThreadStatus = DTR_THREAD_STATUS_END ;

            goto LABEL_EXIT_WITH_CLOSE_SOCKET ;
        }
    }

LABEL_EXIT_WITH_CLOSE_SOCKET :
    __RCE_DTR_CloseSocket() ;
LABEL_EXIT_WITH_NO_PROCESS :
    pthread_exit(NULL) ;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/******* End of file RCE_DTR.c. *******/  
