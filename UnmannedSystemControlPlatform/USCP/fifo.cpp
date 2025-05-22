﻿/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : fifo.cpp
    Project name : 
    Module name  :
    Date created : 2025年05月21日 12时56分15秒
    Author       : Ning.JianLi
    Description  : 提供FIFO功能。
*******************************************************************************/

#include "pch.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include "fifo.h"

/*******************************************************************************
- Function    : FIFO_Create
- Description : 本函数创建一个FIFO。
- Input       : uiSize : 
                    FIFO的大小（深度）。必需大于0。
- Output      : pstFifoDev :
                    FIFO设备（保存FIFO信息的结构体变量指针）。
- Return      : int :
                    0 : 创建成功。
                   -1 : 参数错误。
                   -2 : FIFO指针被占用。
                   -3 : 申请缓存失败。
- Others      :
*******************************************************************************/
int FIFO_Create(unsigned int uiSize, FIFO_DEVICE *pstFifoDev)
{
    /* 参数检查 */
    if(0 == uiSize || NULL == pstFifoDev)
    {
        return -1 ;
    }

    /* 检查指针是否被占用 */
    if(FIFO_FLAG_CREATED == pstFifoDev->uiFlag)
    {
        return -2 ;
    }

    /* 申请缓存 */
    pstFifoDev->pucMemory = (unsigned char *)malloc(uiSize) ;

    if(NULL == pstFifoDev->pucMemory)
    {
        return -3 ;
    }

    pstFifoDev->uiWrPtr = 0 ;
    pstFifoDev->uiRdPtr = 0 ;
    pstFifoDev->uiSize  = uiSize ;

    /* 标记 */
    pstFifoDev->uiFlag = FIFO_FLAG_CREATED ;

    return 0 ;
}

/*******************************************************************************
- Function    : FIFO_Delete
- Description : 本函数删除一个FIFO。
- Input       : pstFifoDev :
                    FIFO设备（保存FIFO信息的结构体变量指针）。
- Output      : NULL
- Return      : int :
                    0 : 创建成功。
                   -1 : 参数错误。
                   -2 : FIFO指针被占用。
- Others      :
*******************************************************************************/
int FIFO_Delete(FIFO_DEVICE *pstFifoDev)
{
    /* 参数检查 */
    if(NULL == pstFifoDev)
    {
        return -1 ;
    }

    /* 检查FIFO是否被创建 */
    if(FIFO_FLAG_CREATED != pstFifoDev->uiFlag)
    {
        return -2 ;
    }

    free(pstFifoDev->pucMemory) ;

    return 0 ;
}

/*******************************************************************************
- Function    : FIFO_Put
- Description : 本函数向FIFO中存入数据。
- Input       : pucSrc :
                    待存入的数据。
                uiSize :
                    存入数据量。
                pstFifoDev :
                    FIFO设备（保存FIFO信息的结构体变量指针）。
- Output      : NULL
- Return      : int :
                    返回存入的数据量。
                    0 : 未存入数据（FIFO满）。
                   -1 : 参数错误。
                   -2 : FIFO无效。
- Others      :
*******************************************************************************/
int FIFO_Put(unsigned char *pucSrc, unsigned int uiSize, FIFO_DEVICE *pstFifoDev)
{
    unsigned int uiCopySize ;
    unsigned int uiFreeSize ;
    unsigned int uiRetVal ;

    /* 参数检查 */
    if(NULL == pucSrc || 0 == uiSize ||NULL == pstFifoDev)
    {
        return -1 ;
    }

    /* 检查FIFO是否被创建 */
    if(FIFO_FLAG_CREATED != pstFifoDev->uiFlag)
    {
        return -2 ;
    }

    /* 计算FIFO中空闲位置大小 */
    if(pstFifoDev->uiRdPtr <= pstFifoDev->uiWrPtr)
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
        if (0 == pstFifoDev->uiRdPtr)
        {
            uiFreeSize = pstFifoDev->uiSize - pstFifoDev->uiWrPtr - 1 ; /* 确保最多写入到size-1大小，否则rdptr和wrptr会重合 */
        }
        else
        {
            uiFreeSize = pstFifoDev->uiSize - pstFifoDev->uiWrPtr ; /* 确保最多写入到size-1大小，否则rdptr和wrptr会重合 */
        }
        
        /* 计算拷贝大小 */
        uiCopySize = (uiFreeSize > uiSize)? uiSize : uiFreeSize ;

        if (0 != uiCopySize)
        {
            /* 拷贝数据 */
            memcpy(&(pstFifoDev->pucMemory[pstFifoDev->uiWrPtr]), pucSrc, uiCopySize) ;

            /* 调整指针 */
            pstFifoDev->uiWrPtr += uiCopySize ;

            if (pstFifoDev->uiSize <= pstFifoDev->uiWrPtr)
            {
                pstFifoDev->uiWrPtr = 0 ;
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
        if(0 != uiSize && 0 != pstFifoDev->uiRdPtr)
        {
            /* 从缓存头部到rdptr之间的空间为空闲空间 */
            uiFreeSize = pstFifoDev->uiRdPtr ;

            uiCopySize = (uiFreeSize > uiSize)? uiSize : uiFreeSize ;

            /* 拷贝数据 */
            memcpy(&(pstFifoDev->pucMemory[0]), pucSrc, uiCopySize) ;

            /* 调整指针 */ /* 理论上不会溢出 */
            pstFifoDev->uiWrPtr += uiCopySize ;

            uiRetVal += uiCopySize ;
        }

        return (int)uiRetVal ;
    }
    else /* if(pstFifoDev->uiRdPtr > pstFifoDev->uiWrPtr) */
    {
        /*  |-------------------------------------------------------------|
         *  |           %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              |
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            wr_ptr                              rd_ptr
         */
        /* 此时wrptr至rdptr之间的空间为空闲空间 */
        uiFreeSize = pstFifoDev->uiRdPtr - pstFifoDev->uiWrPtr - 1 ;

        /* 计算拷贝大小 */
        uiCopySize = (uiFreeSize > uiSize)? uiSize : uiFreeSize ;

        /* 拷贝数据 */
        memcpy(&pstFifoDev->pucMemory[pstFifoDev->uiWrPtr], pucSrc, uiCopySize) ;

        /* 调整指针 */
        pstFifoDev->uiWrPtr += uiCopySize ;

        return (int)uiCopySize ;
    }
}

/*******************************************************************************
- Function    : FIFO_Get
- Description : 本函数从FIFO中取出数据。
- Input       : uiSize :
                    取出数据量。
                pstFifoDev :
                    FIFO设备（保存FIFO信息的结构体变量指针）。
- Output      : pucDst :
                    保存取出的数据。
- Return      : int :
                    返回取出的数据量。
                    0 : 未取出数据（FIFO空）。
                   -1 : 参数错误。
                   -2 : FIFO无效。
- Others      :
*******************************************************************************/
int FIFO_Get(unsigned char *pucDst, unsigned int uiSize, FIFO_DEVICE *pstFifoDev)
{
    unsigned int uiStoreSize ;
    unsigned int uiCopySize ;
    unsigned int uiRetVal ;

    /* 参数检查 */
    if(NULL == pucDst || 0 == uiSize ||NULL == pstFifoDev)
    {
        return -1 ;
    }

    /* 检查FIFO是否被创建 */
    if(FIFO_FLAG_CREATED != pstFifoDev->uiFlag)
    {
        return -2 ;
    }

    /* 计算FIFO中数据大小 */
    if(pstFifoDev->uiRdPtr == pstFifoDev->uiWrPtr)
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
    else if(pstFifoDev->uiWrPtr > pstFifoDev->uiRdPtr)
    {
        /*  |-------------------------------------------------------------|
         *  |           ####################################              |
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            rd_ptr                              wr_ptr
         */
        /* 计算存储的数据的容量 */
        uiStoreSize = pstFifoDev->uiWrPtr - pstFifoDev->uiRdPtr ;

        /* 计算拷贝数据量 */
        uiCopySize = (uiStoreSize > uiSize)? uiSize : uiStoreSize ;

        /* 拷贝数据 */
        memcpy(pucDst, &(pstFifoDev->pucMemory[pstFifoDev->uiRdPtr]), uiCopySize) ;

        /* 修正指针 */
        pstFifoDev->uiRdPtr += uiCopySize ; /* 必定不会溢出 */

        return (int)uiCopySize ;
    }
    else /* pstFifoDev->uiWrPtr < pstFifoDev->uiRdPtr */
    {
        /*  |-------------------------------------------------------------|
         *  |###########                                    ##############|
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            wr_ptr                              rd_ptr
         */
        /* 计算从当前位置到FIFO尾部的数据量 */
        uiStoreSize = pstFifoDev->uiSize - pstFifoDev->uiRdPtr ;

        /* 计算拷贝数据量 */
        uiCopySize = (uiStoreSize > uiSize)? uiSize : uiStoreSize ;

        /* 拷贝数据 */
        memcpy(pucDst, &(pstFifoDev->pucMemory[pstFifoDev->uiRdPtr]), uiCopySize) ;

        /* 修正指针 */
        pstFifoDev->uiRdPtr += uiCopySize ;

        if(pstFifoDev->uiRdPtr >= pstFifoDev->uiSize)
        {
            /* 只可能等于，不会大于 */
            pstFifoDev->uiRdPtr = 0 ;
        }

        pucDst   += uiCopySize ;
        uiSize   -= uiCopySize ;
        uiRetVal = uiCopySize ;

        if(0 != uiSize)
        {
            /* 这种情况说明从rd_ptr到尾部的有效数据空间不够拷贝的，需要再从头拷贝 */
            uiStoreSize = pstFifoDev->uiWrPtr ;

            /* 计算拷贝数据量 */
            uiCopySize = (uiStoreSize > uiSize)? uiSize : uiStoreSize ;

            /* 拷贝数据 */
            memcpy(pucDst, &(pstFifoDev->pucMemory[0]), uiCopySize) ;

            /* 修正指针 */
            pstFifoDev->uiRdPtr += uiCopySize ;

            uiRetVal += uiCopySize ;
        }

        return (int)uiRetVal ;
    }
}

/*******************************************************************************
- Function    : FIFO_GetFifoDataCount
- Description : 本函数计算FIFO数据量。
- Input       : pstFifoDev :
                    FIFO设备（保存FIFO信息的结构体变量指针）。
- Output      : NULL
- Return      : int :
                    返回FIFO内存有的数据量。
                   -1 : 参数错误。
- Others      :
*******************************************************************************/
int FIFO_GetFifoDataCount(FIFO_DEVICE* pstFifoDev)
{
    unsigned int uiFifoDataCount ;
    /* 参数检查 */
    if (NULL == pstFifoDev)
    {
        return -1 ;
    }

    if (pstFifoDev->uiRdPtr <= pstFifoDev->uiWrPtr)
    {
        /*  |-------------------------------------------------------------|
         *  |           ####################################              |
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            rd_ptr                              wr_ptr
         */
         /* rd_ptr等于wr_ptr指针相同，FIFO为空 */
         /* rd_ptr小于wr_ptr，FIFO空闲空间分为2段 */
        uiFifoDataCount = pstFifoDev->uiWrPtr - pstFifoDev->uiRdPtr ;
    }
    else
    {
        /*  |-------------------------------------------------------------|
         *  |###########                                    ##############|
         *  |-------------------------------------------------------------|
         *              ^                                   ^
         *            wr_ptr                              rd_ptr
         */
        uiFifoDataCount = pstFifoDev->uiSize - pstFifoDev->uiRdPtr + pstFifoDev->uiWrPtr ;
    }

    return (int)uiFifoDataCount ;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
/******* End of file fifo.cpp. *******/
