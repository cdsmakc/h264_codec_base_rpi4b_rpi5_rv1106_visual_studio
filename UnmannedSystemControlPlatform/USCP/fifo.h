/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : fifo.h
    Project name : 
    Module name  :
    Date created : 2025年05月21日 12时56分15秒
    Author       : Ning.JianLi
    Description  : 提供FIFO功能。
*******************************************************************************/

#ifndef __FIFO_H__
#define __FIFO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 宏定义 */
#define FIFO_FLAG_CREATED           (0x4f50454e)

/* 结构体定义 */
typedef struct tagFIFO_DEVICE
{
    unsigned int    uiFlag ;
    unsigned char * pucMemory ;  /* FIFO存储主体 */
    unsigned int    uiSize ;     /* FIFO大小 */
    unsigned int    uiWrPtr ;    /* 写入指针 */
    unsigned int    uiRdPtr ;    /* 读取指针 */
} FIFO_DEVICE ;

/* 函数声明 */
int FIFO_Create(unsigned int uiSize, FIFO_DEVICE *pstFifoDev) ;
int FIFO_Delete(FIFO_DEVICE *pstFifoDev) ;
int FIFO_Put(unsigned char *pucSrc, unsigned int uiSize, FIFO_DEVICE *pstFifoDev) ;
int FIFO_Get(unsigned char *pucDst, unsigned int uiSize, FIFO_DEVICE *pstFifoDev) ;
int FIFO_GetFifoDataCount(FIFO_DEVICE* pstFifoDev) ;

#ifdef __cplusplus
} 
#endif /* __cplusplus */
#endif /* __FIFO_H__ */

/******* End of file fifo.h. *******/