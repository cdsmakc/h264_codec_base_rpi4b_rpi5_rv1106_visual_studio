/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_types.h
    Project name : 公共模块
    Module name  : 
    Date created : 2025年4月30日   8时59分31秒
    Author       : 
    Description  : 基本数据类型。
*******************************************************************************/
  
#ifndef __RCE_TYPES_H__
#define __RCE_TYPES_H__
  
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef CHAR
#define CHAR char
#endif

#ifndef SHORT
#define SHORT short
#endif

#ifndef INT
#define INT int
#endif

#ifndef INT64
#define INT64 long long
#endif

#ifndef UCHAR
#define UCHAR unsigned char
#endif

#ifndef USHORT
#define USHORT unsigned short
#endif

#ifndef UINT
#define UINT unsigned int
#endif

#ifndef UINT64
#define UINT64 unsigned long long
#endif

#ifndef FLOAT
#define FLOAT float 
#endif

#ifndef DOUBLE
#define DOUBLE double 
#endif


#ifndef VOID
#define VOID void
#endif

#ifndef BOOL_T
#define BOOL_T UCHAR
#endif

#ifndef BOOL_TRUE 
#define BOOL_TRUE ((BOOL_T)1)
#endif

#ifndef BOOL_FALSE
#define BOOL_FALSE ((BOOL_T)0)
#endif

#ifndef IN
#define IN 
#endif

#ifndef OUT
#define OUT
#endif

#ifndef BIDIR
#define BIDIR
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __RCE_TYPES_H__ */
  
/******* End of file RCE_types.h. *******/  
